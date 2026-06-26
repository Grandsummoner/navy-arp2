#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState();
    sceneB = SceneState();
    lastChordPitches = { 60, 64, 67 }; 

    // Initialize 4x4 scene defaults to standard values [NEW]
    for (int i = 0; i < 4; ++i)
    {
        sceneAPresets[i] = SceneState();
        sceneBPresets[i] = SceneState();
    }
}

PluginProcessor::~PluginProcessor() {}

const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }
bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }
bool PluginProcessor::isMidiEffect() const { return false; } 
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }
int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String PluginProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void PluginProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mLastStep = -1;
    mLastNotePlayed = -1;
    mNoteOffTime = 0;
    mTimeInSamples = 0;
    activeHeldNotes.clear();
    latchedNotes.clear();
    scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true;
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

std::vector<int> PluginProcessor::generateEuclideanPattern (int steps, int pulses)
{
    std::vector<int> pattern(steps, 0);
    if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), 1); return pattern; }

    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps) { bucket -= steps; pattern[i] = 1; }
    }
    return pattern;
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
    double sampleDelta = numSamples;

    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    int cycleBars = 4;
    if (cycleIndex == 0)      cycleBars = 1;
    else if (cycleIndex == 1) cycleBars = 2;
    else if (cycleIndex == 2) cycleBars = 4;
    else if (cycleIndex == 3) cycleBars = 8;

    if (cycleBars > 0)
        currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % cycleBars) + 1;
    else
        currentBarInCycle = 1;

    // Bipolar Entropy Accumulator
    float baseEntropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());

    float absEntropy = std::abs(baseEntropy);
    int stepInterval = 1; 
    if (absEntropy > 0.33f && absEntropy <= 0.66f) stepInterval = 2; 
    else if (absEntropy > 0.66f) stepInterval = 4; 

    if (currentStep == 0 && mLastStep == 7) 
    {
        if (baseEntropy > 0.05f) accumulatedPitchOffset += stepInterval;
        else if (baseEntropy < -0.05f) accumulatedPitchOffset -= stepInterval;
        
        if (currentBarInCycle == 1) accumulatedPitchOffset = 0.0f;
    }

    // 8-Channel LFO Modulation Matrix
    auto applyLfo = [&](int index, juce::ParameterID baseId, juce::ParameterID rateId, juce::ParameterID depthId, float minVal, float maxVal) -> float {
        float baseVal = *apvts.getRawParameterValue (baseId.getParamID());
        int rateChoice = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depth = *apvts.getRawParameterValue (depthId.getParamID());
        
        if (rateChoice == 0) 
            return baseVal;
            
        double divPPQ = 0.25; 
        if (rateChoice == 1)      divPPQ = 1.0;   
        else if (rateChoice == 2) divPPQ = 0.5;   
        else if (rateChoice == 3) divPPQ = 0.25;  
        else if (rateChoice == 4) divPPQ = 0.125; 
        
        double periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples);
        if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        
        float sineVal = static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi));
        float range = maxVal - minVal;
        float mod = (sineVal * depth * (range * 0.5f));
        return juce::jlimit (minVal, maxVal, baseVal + mod);
    };

    // Capture user manual edits and write them directly to the focused background target [NEW]
    int focusedA = activeSceneAIndex.load();
    int focusedB = activeSceneBIndex.load();
    int focusSide = editFocusSide.load();

    auto updateFocusValue = [&](juce::ParameterID baseId, int lfoIndex, juce::ParameterID rateId, juce::ParameterID depthId, int index) {
        float val = *apvts.getRawParameterValue (baseId.getParamID());
        int rateVal = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depthVal = *apvts.getRawParameterValue (depthId.getParamID());

        if (focusSide == 0) {
            if (index < 8) sceneAPresets[focusedA].faders[index] = val;
            else if (index == 8)  sceneAPresets[focusedA].rhythmMorph = val;
            else if (index == 9)  sceneAPresets[focusedA].rest = val;
            else if (index == 10) sceneAPresets[focusedA].legato = val;
            else if (index == 11) sceneAPresets[focusedA].rate = val;
            else if (index == 12) sceneAPresets[focusedA].entropy = val;
            else if (index == 13) sceneAPresets[focusedA].harmony = val;
            else if (index == 14) sceneAPresets[focusedA].chaos = val;
            else if (index == 15) sceneAPresets[focusedA].octaves = val;
            
            sceneAPresets[focusedA].lfoRates[lfoIndex] = rateVal;
            sceneAPresets[focusedA].lfoDepths[lfoIndex] = depthVal;
        } else {
            if (index < 8) sceneBPresets[focusedB].faders[index] = val;
            else if (index == 8)  sceneBPresets[focusedB].rhythmMorph = val;
            else if (index == 9)  sceneBPresets[focusedB].rest = val;
            else if (index == 10) sceneBPresets[focusedB].legato = val;
            else if (index == 11) sceneBPresets[focusedB].rate = val;
            else if (index == 12) sceneBPresets[focusedB].entropy = val;
            else if (index == 13) sceneBPresets[focusedB].harmony = val;
            else if (index == 14) sceneBPresets[focusedB].chaos = val;
            else if (index == 15) sceneBPresets[focusedB].octaves = val;

            sceneBPresets[focusedB].lfoRates[lfoIndex] = rateVal;
            sceneBPresets[focusedB].lfoDepths[lfoIndex] = depthVal;
        }
    };

    // Keep active parameter edit focus synced
    updateFocusValue (IDs::rhythmMorph, 0, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 8);
    updateFocusValue (IDs::rest,        1, IDs::restLfoRate,        IDs::restLfoDepth,        9);
    updateFocusValue (IDs::legato,      2, IDs::legatoLfoRate,      IDs::legatoLfoDepth,      10);
    updateFocusValue (IDs::rate,        3, IDs::rateLfoRate,        IDs::rateLfoDepth,        11);
    updateFocusValue (IDs::entropy,     4, IDs::entropyLfoRate,     IDs::entropyLfoDepth,     12);
    updateFocusValue (IDs::harmony,     5, IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     13);
    updateFocusValue (IDs::chaos,       6, IDs::chaosLfoRate,       IDs::chaosLfoDepth,       14);
    updateFocusValue (IDs::octaves,     7, IDs::octavesLfoRate,     IDs::octavesLfoDepth,     15);

    for (int i = 0; i < 8; ++i)
        updateFocusValue (juce::ParameterID ("fader" + juce::String (i + 1), 1), i, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, i);

    activeMorph   = applyLfo (0, IDs::rhythmMorph, IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 0.0f, 1.0f);
    activeRest    = applyLfo (1, IDs::rest,        IDs::restLfoRate,        IDs::restLfoDepth,        0.0f, 1.0f);
    
    activeLegato  = applyLfo (2, IDs::legato,      IDs::legatoLfoRate,      IDs::legatoLfoDepth,      0.1f, 1.0f);
    modLegato = activeLegato;
    modRest = activeRest;

    float rawRate = applyLfo (3, IDs::rate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));

    activeEntropy = applyLfo (4, IDs::entropy,     IDs::entropyLfoRate,     IDs::entropyLfoDepth,     -1.0f, 1.0f);
    modEntropy = activeEntropy;

    activeHarmony = applyLfo (5, IDs::harmony,     IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     0.0f, 1.0f);
    modHarmony = activeHarmony;

    activeChaos   = applyLfo (6, IDs::chaos,       IDs::chaosLfoRate,       IDs::chaosLfoDepth,       0.0f, 1.0f);
    modChaos = activeChaos;

    float rawOctaves = applyLfo (7, IDs::octaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, 1.0f, 4.0f);
    activeOctavesVal = juce::jlimit (1, 4, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    }
    else
    {
        scheduledNoteOffs.push_back ({ pitch, delaySamples });
    }
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    bool isPlaying = false;
    double bpm = 120.0;
    mSongPositionPPQ = 0.0;

#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead())
    {
        if (auto pos = playhead->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            auto bpmOpt = pos->getBpm();
            if (bpmOpt.hasValue()) bpm = *bpmOpt;
            auto ppqOpt = pos->getPpqPosition();
            if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt;
        }
    }
#else
    if (auto* playhead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playhead->getCurrentPositionInfo (info))
        {
            isPlaying = info.isPlaying;
            bpm = info.bpm;
            mSongPositionPPQ = info.ppqPosition;
        }
    }
#endif

    int numSamples = buffer.getNumSamples();
    updateLfoModulations (numSamples, bpm);

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;

    float activeFaderProb[8];
    for (int i = 0; i < 8; ++i)
        activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    // 1. Process Decrementing Note-Off Queue
    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();)
    {
        it->second -= numSamples;
        if (it->second <= 0)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0);
            it = scheduledNoteOffs.erase(it);
        }
        else ++it;
    }

    // 2. Monitor physical keyboard pressed MIDI keys
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end())
            {
                activeHeldNotes.push_back (note);
                std::sort (activeHeldNotes.begin(), activeHeldNotes.end());
            }

            if (isLatchActive)
            {
                if (isFirstNoteOfNewChord)
                {
                    for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0);
                    latchedNotes.clear();
                    isFirstNoteOfNewChord = false;
                }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end())
                {
                    latchedNotes.push_back (note);
                    std::sort (latchedNotes.begin(), latchedNotes.end());
                }
            }
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear();

    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    
    isCurrentlyPlayingUI.store (!notesToPlay.empty());

    // 3. Dual-Clock Step Generation
    if (! notesToPlay.empty())
    {
        bool stepTriggered = false;
        double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        
        // Compute active step rate dynamically scaling with continuous modulated LFO
        double stepLengthPPQ = 0.25; 
        if (activeRateIdx == 0)      stepLengthPPQ = 1.0;   // 1/4 Note
        else if (activeRateIdx == 1) stepLengthPPQ = 0.5;   // 1/8 Note
        else if (activeRateIdx == 2) stepLengthPPQ = 0.25;  // 1/16 Note
        else if (activeRateIdx == 3) stepLengthPPQ = 0.125; // 1/32 Note

        double stepSamples = samplesPerBeat * stepLengthPPQ;

        if (isPlaying)
        {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep)
            {
                mLastStep = stepIndex;
                currentStep = stepIndex;
                stepTriggered = true;
            }
        }
        else
        {
            mTimeInSamples += numSamples;
            if (mTimeInSamples >= stepSamples)
            {
                mTimeInSamples = 0;
                currentStep = (currentStep + 1) % 8;
                mLastStep = currentStep;
                stepTriggered = true;
            }
        }

        // 4. Arpeggiator Step & Euclidean Execution
        if (stepTriggered)
        {
            float faderProb = activeFaderProb[currentStep];
            
            int ratchetPulses = static_cast<int>(std::round(activeMorph * 8.0f));
            std::vector<int> euclidRatchets = generateEuclideanPattern (8, ratchetPulses);
            bool isRatchetStep = euclidRatchets[currentStep] == 1;
            juce::ignoreUnused(isRatchetStep);

            bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= faderProb);
            bool isRest = (juce::Random::getSystemRandom().nextFloat() <= modRest);

            if (shouldPlay && ! isRest)
            {
                if (mLastNotePlayed != -1)
                {
                    processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                    mLastNotePlayed = -1;
                }

                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

                std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; 
                if (scaleIdx == 1)      scaleOffsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; 
                else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; 
                else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  
                else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  
                else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  
                else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  
                else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  

                int rawPitch = notesToPlay[currentStep % notesToPlay.size()];
                int octave = (rawPitch / 12) * 12;

                // Modulate active arpeggiation octave limit continuously
                int octaveShiftCount = (currentStep / 2) % activeOctavesVal;

                int targetPitch = octave + rootKeyIdx + scaleOffsets[currentStep] + static_cast<int>(accumulatedPitchOffset) + (octaveShiftCount * 12);

                if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos)
                    targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);

                targetPitch = juce::jlimit(0, 127, targetPitch);
                int durationSamples = static_cast<int>(stepSamples * modLegato);

                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetPitch;
                mNoteOffTime = durationSamples;
                
                scheduleNoteOff (processedMidi, targetPitch, durationSamples);
            }
        }
    }
    else 
    { 
        if (mLastStep != -1)
        {
            if (mLastNotePlayed != -1)
            {
                processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                mLastNotePlayed = -1;
            }
            mLastStep = -1; 
        }
        currentStep = 0; 
    }

    midiMessages.swapWith (processedMidi);
}

void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

    std::vector<int> scaleOffsets;
    switch (scaleIdx)
    {
        case 1:  scaleOffsets = { 0, 2, 3, 5, 7, 8, 10 }; break; 
        case 2:  scaleOffsets = { 0, 3, 5, 7, 10, 12, 14 }; break; 
        case 3:  scaleOffsets = { 0, 2, 4, 7, 9, 12, 14 }; break; 
        case 4:  scaleOffsets = { 0, 2, 3, 5, 7, 9, 10 }; break; 
        case 5:  scaleOffsets = { 0, 1, 3, 5, 7, 8, 10 }; break; 
        case 6:  scaleOffsets = { 0, 2, 4, 6, 7, 9, 11 }; break; 
        case 7:  scaleOffsets = { 0, 2, 4, 5, 7, 9, 10 }; break; 
        case 8:  scaleOffsets = { 0, 2, 3, 5, 7, 8, 11 }; break; 
        case 9:  scaleOffsets = { 0, 2, 3, 5, 7, 9, 11 }; break; 
        default: scaleOffsets = { 0, 2, 4, 5, 7, 9, 11 }; break; 
    }

    auto getScalePitch = [&](int degree) -> int {
        int octaveShift = (degree / 7) * 12;
        return scaleOffsets[degree % 7] + octaveShift;
    };

    int baseRoot = 48 + rootIdx;
    int r = getScalePitch (padIndex);
    int t = getScalePitch (padIndex + 2); 
    int f = getScalePitch (padIndex + 4); 

    // Apply modulated continuous LFO parameter
    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f)
    {
        t = getScalePitch (padIndex + 3); 
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f };
    }
    else if (activeHarmony >= 0.67f)
    {
        int s = getScalePitch (padIndex + 6); 
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s };
    }
    else
    {
        newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; 
    }

    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size())
    {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; 
        else if (pitchDiff < -5) newChord[0] += 12; 
    }
    
    std::sort(newChord.begin(), newChord.end());
    lastChordPitches = newChord;

    latchedNotes = newChord;
    apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

void PluginProcessor::saveSceneA (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 4) return;
    for (int i = 0; i < 8; ++i) sceneAPresets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneAPresets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneAPresets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneAPresets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneAPresets[slotIndex].rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
    sceneAPresets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneAPresets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneAPresets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    sceneAPresets[slotIndex].octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());

    // Serialize LFO matrix states
    sceneAPresets[slotIndex].lfoRates[0] = static_cast<int> (*apvts.getRawParameterValue (IDs::rhythmMorphLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[0] = *apvts.getRawParameterValue (IDs::rhythmMorphLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[1] = static_cast<int> (*apvts.getRawParameterValue (IDs::restLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[1] = *apvts.getRawParameterValue (IDs::restLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[2] = static_cast<int> (*apvts.getRawParameterValue (IDs::legatoLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[2] = *apvts.getRawParameterValue (IDs::legatoLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[3] = static_cast<int> (*apvts.getRawParameterValue (IDs::rateLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[3] = *apvts.getRawParameterValue (IDs::rateLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[4] = static_cast<int> (*apvts.getRawParameterValue (IDs::entropyLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[4] = *apvts.getRawParameterValue (IDs::entropyLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[5] = static_cast<int> (*apvts.getRawParameterValue (IDs::harmonyLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[5] = *apvts.getRawParameterValue (IDs::harmonyLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[6] = static_cast<int> (*apvts.getRawParameterValue (IDs::chaosLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[6] = *apvts.getRawParameterValue (IDs::chaosLfoDepth.getParamID());
    sceneAPresets[slotIndex].lfoRates[7] = static_cast<int> (*apvts.getRawParameterValue (IDs::octavesLfoRate.getParamID()));
    sceneAPresets[slotIndex].lfoDepths[7] = *apvts.getRawParameterValue (IDs::octavesLfoDepth.getParamID());

    sceneASlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadSceneA (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 4 || ! sceneASlotsSaved[slotIndex]) return;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].legato);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].rate);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].entropy);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].chaos);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].octaves);

    // Restore LFOs
    apvts.getParameter (IDs::rhythmMorphLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[0]) / 4.0f);
    apvts.getParameter (IDs::rhythmMorphLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[0]);
    apvts.getParameter (IDs::restLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[1]) / 4.0f);
    apvts.getParameter (IDs::restLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[1]);
    apvts.getParameter (IDs::legatoLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[2]) / 4.0f);
    apvts.getParameter (IDs::legatoLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[2]);
    apvts.getParameter (IDs::rateLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[3]) / 4.0f);
    apvts.getParameter (IDs::rateLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[3]);
    apvts.getParameter (IDs::entropyLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[4]) / 4.0f);
    apvts.getParameter (IDs::entropyLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[4]);
    apvts.getParameter (IDs::harmonyLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[5]) / 4.0f);
    apvts.getParameter (IDs::harmonyLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[5]);
    apvts.getParameter (IDs::chaosLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[6]) / 4.0f);
    apvts.getParameter (IDs::chaosLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[6]);
    apvts.getParameter (IDs::octavesLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneAPresets[slotIndex].lfoRates[7]) / 4.0f);
    apvts.getParameter (IDs::octavesLfoDepth.getParamID())->setValueNotifyingHost (sceneAPresets[slotIndex].lfoDepths[7]);

    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (sceneAPresets[slotIndex].faders[i]);
}

void PluginProcessor::saveSceneB (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 4) return;
    for (int i = 0; i < 8; ++i) sceneBPresets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneBPresets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneBPresets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneBPresets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneBPresets[slotIndex].rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
    sceneBPresets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneBPresets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneBPresets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    sceneBPresets[slotIndex].octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());

    // Serialize LFO matrix states
    sceneBPresets[slotIndex].lfoRates[0] = static_cast<int> (*apvts.getRawParameterValue (IDs::rhythmMorphLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[0] = *apvts.getRawParameterValue (IDs::rhythmMorphLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[1] = static_cast<int> (*apvts.getRawParameterValue (IDs::restLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[1] = *apvts.getRawParameterValue (IDs::restLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[2] = static_cast<int> (*apvts.getRawParameterValue (IDs::legatoLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[2] = *apvts.getRawParameterValue (IDs::legatoLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[3] = static_cast<int> (*apvts.getRawParameterValue (IDs::rateLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[3] = *apvts.getRawParameterValue (IDs::rateLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[4] = static_cast<int> (*apvts.getRawParameterValue (IDs::entropyLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[4] = *apvts.getRawParameterValue (IDs::entropyLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[5] = static_cast<int> (*apvts.getRawParameterValue (IDs::harmonyLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[5] = *apvts.getRawParameterValue (IDs::harmonyLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[6] = static_cast<int> (*apvts.getRawParameterValue (IDs::chaosLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[6] = *apvts.getRawParameterValue (IDs::chaosLfoDepth.getParamID());
    sceneBPresets[slotIndex].lfoRates[7] = static_cast<int> (*apvts.getRawParameterValue (IDs::octavesLfoRate.getParamID()));
    sceneBPresets[slotIndex].lfoDepths[7] = *apvts.getRawParameterValue (IDs::octavesLfoDepth.getParamID());

    sceneBSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadSceneB (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 4 || ! sceneBSlotsSaved[slotIndex]) return;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].legato);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].rate);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].entropy);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].chaos);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].octaves);

    // Restore LFOs
    apvts.getParameter (IDs::rhythmMorphLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[0]) / 4.0f);
    apvts.getParameter (IDs::rhythmMorphLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[0]);
    apvts.getParameter (IDs::restLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[1]) / 4.0f);
    apvts.getParameter (IDs::restLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[1]);
    apvts.getParameter (IDs::legatoLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[2]) / 4.0f);
    apvts.getParameter (IDs::legatoLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[2]);
    apvts.getParameter (IDs::rateLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[3]) / 4.0f);
    apvts.getParameter (IDs::rateLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[3]);
    apvts.getParameter (IDs::entropyLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[4]) / 4.0f);
    apvts.getParameter (IDs::entropyLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[4]);
    apvts.getParameter (IDs::harmonyLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[5]) / 4.0f);
    apvts.getParameter (IDs::harmonyLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[5]);
    apvts.getParameter (IDs::chaosLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[6]) / 4.0f);
    apvts.getParameter (IDs::chaosLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[6]);
    apvts.getParameter (IDs::octavesLfoRate.getParamID())->setValueNotifyingHost (static_cast<float>(sceneBPresets[slotIndex].lfoRates[7]) / 4.0f);
    apvts.getParameter (IDs::octavesLfoDepth.getParamID())->setValueNotifyingHost (sceneBPresets[slotIndex].lfoDepths[7]);

    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (sceneBPresets[slotIndex].faders[i]);
}

void PluginProcessor::savePreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;
    for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    presetSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadPreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8 || ! presetSlotsSaved[slotIndex]) return;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos);

    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]);
}

void PluginProcessor::captureSceneA()
{
    for (int i = 0; i < 8; ++i) sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneA.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneA.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneA.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneA.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneA.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneA.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneA = true;
}

void PluginProcessor::captureSceneB()
{
    for (int i = 0; i < 8; ++i) sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    sceneB.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneB.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneB.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneB.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneB.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneB.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneB = true;
}

// Background-focused scene randomization [NEW]
void PluginProcessor::diceActiveScene()
{
    auto* random = &juce::Random::getSystemRandom();
    int focusedA = activeSceneAIndex.load();
    int focusedB = activeSceneBIndex.load();
    int focusSide = editFocusSide.load();

    auto randomizeScene = [&](SceneState& scene) {
        for (int i = 0; i < 8; ++i) scene.faders[i] = random->nextFloat();
        scene.rhythmMorph = random->nextFloat();
        scene.rest = random->nextFloat() * 0.5f;
        scene.legato = 0.2f + random->nextFloat() * 0.8f;
        scene.entropy = -1.0f + random->nextFloat() * 2.0f;
        scene.harmony = random->nextFloat();
        scene.chaos = random->nextFloat();
        scene.rate = static_cast<float> (random->nextInt (4));
        scene.octaves = static_cast<float> (1 + random->nextInt (4));

        for (int i = 0; i < 8; ++i) {
            scene.lfoRates[i] = random->nextInt (5); // Off to 1/32
            scene.lfoDepths[i] = random->nextFloat() * 0.5f; // Soft-medium depths
        }
    };

    if (focusSide == 0) {
        randomizeScene (sceneAPresets[focusedA]);
        sceneASlotsSaved[focusedA] = true;
        loadSceneA (focusedA); // Instantly update active parameters on GUI [NEW]
    } else {
        randomizeScene (sceneBPresets[focusedB]);
        sceneBSlotsSaved[focusedB] = true;
        loadSceneB (focusedB); // Instantly update active parameters on GUI [NEW]
    }
}

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

bool PluginProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

// Serializing 4x4 scenes dynamically inside the project session state ValueTree [1] [NEW]
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    
    // Add custom XML sub-elements for both Scene A and Scene B memory banks [NEW]
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    auto* presetsNodeA = xml->createNewChildElement ("SCENE_A_PRESETS");
    auto* presetsNodeB = xml->createNewChildElement ("SCENE_B_PRESETS");

    for (int i = 0; i < 4; ++i)
    {
        auto* childA = presetsNodeA->createNewChildElement ("SLOT_" + juce::String (i));
        childA->setAttribute ("saved", sceneASlotsSaved[i]);
        if (sceneASlotsSaved[i]) {
            childA->setAttribute ("morph", sceneAPresets[i].rhythmMorph);
            childA->setAttribute ("rest", sceneAPresets[i].rest);
            childA->setAttribute ("legato", sceneAPresets[i].legato);
            childA->setAttribute ("rate", sceneAPresets[i].rate);
            childA->setAttribute ("entropy", sceneAPresets[i].entropy);
            childA->setAttribute ("harmony", sceneAPresets[i].harmony);
            childA->setAttribute ("chaos", sceneAPresets[i].chaos);
            childA->setAttribute ("octaves", sceneAPresets[i].octaves);
            for (int f = 0; f < 8; ++f) childA->setAttribute ("fader_" + juce::String (f), sceneAPresets[i].faders[f]);
            for (int l = 0; l < 8; ++l) {
                childA->setAttribute ("lfo_r_" + juce::String (l), sceneAPresets[i].lfoRates[l]);
                childA->setAttribute ("lfo_d_" + juce::String (l), sceneAPresets[i].lfoDepths[l]);
            }
        }

        auto* childB = presetsNodeB->createNewChildElement ("SLOT_" + juce::String (i));
        childB->setAttribute ("saved", sceneBSlotsSaved[i]);
        if (sceneBSlotsSaved[i]) {
            childB->setAttribute ("morph", sceneBPresets[i].rhythmMorph);
            childB->setAttribute ("rest", sceneBPresets[i].rest);
            childB->setAttribute ("legato", sceneBPresets[i].legato);
            childB->setAttribute ("rate", sceneBPresets[i].rate);
            childB->setAttribute ("entropy", sceneBPresets[i].entropy);
            childB->setAttribute ("harmony", sceneBPresets[i].harmony);
            childB->setAttribute ("chaos", sceneBPresets[i].chaos);
            childB->setAttribute ("octaves", sceneBPresets[i].octaves);
            for (int f = 0; f < 8; ++f) childB->setAttribute ("fader_" + juce::String (f), sceneBPresets[i].faders[f]);
            for (int l = 0; l < 8; ++l) {
                childB->setAttribute ("lfo_r_" + juce::String (l), sceneBPresets[i].lfoRates[l]);
                childB->setAttribute ("lfo_d_" + juce::String (l), sceneBPresets[i].lfoDepths[l]);
            }
        }
    }

    copyXmlToBinary (*xml, destData);
}

// Deserializing 4x4 scenes from project session state ValueTree [1] [NEW]
void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
            
            // Restore Scene memory arrays [NEW]
            if (auto* presetsNodeA = xmlState->getChildByName ("SCENE_A_PRESETS"))
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (auto* childA = presetsNodeA->getChildByName ("SLOT_" + juce::String (i))) {
                        sceneASlotsSaved[i] = childA->getBoolAttribute ("saved");
                        if (sceneASlotsSaved[i]) {
                            sceneAPresets[i].rhythmMorph = childA->getDoubleAttribute ("morph");
                            sceneAPresets[i].rest = childA->getDoubleAttribute ("rest");
                            sceneAPresets[i].legato = childA->getDoubleAttribute ("legato");
                            sceneAPresets[i].rate = childA->getDoubleAttribute ("rate");
                            sceneAPresets[i].entropy = childA->getDoubleAttribute ("entropy");
                            sceneAPresets[i].harmony = childA->getDoubleAttribute ("harmony");
                            sceneAPresets[i].chaos = childA->getDoubleAttribute ("chaos");
                            sceneAPresets[i].octaves = childA->getDoubleAttribute ("octaves");
                            for (int f = 0; f < 8; ++f) sceneAPresets[i].faders[f] = childA->getDoubleAttribute ("fader_" + juce::String (f));
                            for (int l = 0; l < 8; ++l) {
                                sceneAPresets[i].lfoRates[l] = childA->getIntAttribute ("lfo_r_" + juce::String (l));
                                sceneAPresets[i].lfoDepths[l] = childA->getDoubleAttribute ("lfo_d_" + juce::String (l));
                            }
                        }
                    }
                }
            }

            if (auto* presetsNodeB = xmlState->getChildByName ("SCENE_B_PRESETS"))
            {
                for (int i = 0; i < 4; ++i)
                {
                    if (auto* childB = presetsNodeB->getChildByName ("SLOT_" + juce::String (i))) {
                        sceneBSlotsSaved[i] = childB->getBoolAttribute ("saved");
                        if (sceneBSlotsSaved[i]) {
                            sceneBPresets[i].rhythmMorph = childB->getDoubleAttribute ("morph");
                            sceneBPresets[i].rest = childB->getDoubleAttribute ("rest");
                            sceneBPresets[i].legato = childB->getDoubleAttribute ("legato");
                            sceneBPresets[i].rate = childB->getDoubleAttribute ("rate");
                            sceneBPresets[i].entropy = childB->getDoubleAttribute ("entropy");
                            sceneBPresets[i].harmony = childB->getDoubleAttribute ("harmony");
                            sceneBPresets[i].chaos = childB->getDoubleAttribute ("chaos");
                            sceneBPresets[i].octaves = childB->getDoubleAttribute ("octaves");
                            for (int f = 0; f < 8; ++f) sceneBPresets[i].faders[f] = childB->getDoubleAttribute ("fader_" + juce::String (f));
                            for (int l = 0; l < 8; ++l) {
                                sceneBPresets[i].lfoRates[l] = childB->getIntAttribute ("lfo_r_" + juce::String (l));
                                sceneBPresets[i].lfoDepths[l] = childB->getDoubleAttribute ("lfo_d_" + juce::String (l));
                            }
                        }
                    }
                }
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int i = 1; i <= 8; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("fader" + juce::String (i), 1), "Fader " + juce::String (i), 0.0f, 1.0f, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rhythmMorph, "Rhythm Morph", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rest, "Rest", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::legato, "Legato", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", -1.0f, 1.0f, 0.0f)); 
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::chordMode, "Chord Mode", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", 
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", 
        juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", 
        juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); 

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rate, "Rate", 
        juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); 

    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", 1, 4, 1)); 

    // Dynamic Panel Theme selector parameter [1]
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::panelTheme, "Panel Theme", 
        juce::StringArray { "Navy Cyber", "Skyline Eurorack", "Monochrome Minimal", "Matrix Terminal" }, 0));

    // Helper lambda to register LFO parameters
    auto registerLfoParams = [&](juce::ParameterID rateId, juce::ParameterID depthId, juce::String name) {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (rateId, name + " LFO Speed", 
            juce::StringArray { "Off", "1/4", "1/8", "1/16", "1/32" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (depthId, name + " LFO Depth", 0.0f, 1.0f, 0.0f));
    };

    registerLfoParams (IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, "Morph");
    registerLfoParams (IDs::restLfoRate,        IDs::restLfoDepth,        "Rest");
    registerLfoParams (IDs::legatoLfoRate,      IDs::legatoLfoDepth,      "Legato");
    registerLfoParams (IDs::rateLfoRate,        IDs::rateLfoDepth,        "Rate");
    registerLfoParams (IDs::entropyLfoRate,     IDs::entropyLfoDepth,     "Entropy");
    registerLfoParams (IDs::harmonyLfoRate,     IDs::harmonyLfoDepth,     "Harmony");
    registerLfoParams (IDs::chaosLfoRate,       IDs::chaosLfoDepth,       "Chaos");
    registerLfoParams (IDs::octavesLfoRate,     IDs::octavesLfoDepth,     "Octaves");

    return { params.begin(), params.end() };
}

// JUCE Entry Point [5] [NEW]
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() 
{ 
    return new PluginProcessor(); 
}