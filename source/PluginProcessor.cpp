#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState(); sceneB = SceneState(); lastChordPitches = { 60, 64, 67 }; 
    for (int i = 0; i < 8; ++i) { sceneAPresets[i] = SceneState(); sceneBPresets[i] = SceneState(); }
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
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate; mLastStep = -1; mLastNotePlayed = -1; mNoteOffTime = 0; mTimeInSamples = 0;
    activeHeldNotes.clear(); latchedNotes.clear(); scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true; juce::ignoreUnused (samplesPerBlock);

    // Initialize slew smoothing values to prevent jumps
    std::fill (std::begin (currentSlewValue), std::end (currentSlewValue), 0.5f);
    std::fill (std::begin (currentSlewTarget), std::end (currentSlewTarget), 0.5f);
}

void PluginProcessor::releaseResources() {}
bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

std::vector<int> PluginProcessor::generateEuclideanPattern (int steps, int pulses)
{
    std::vector<int> pattern(steps, 0); if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), 1); return pattern; }
    int bucket = 0;
    for (int i = 0; i < steps; ++i) { bucket += pulses; if (bucket >= steps) { bucket -= steps; pattern[i] = 1; } }
    return pattern;
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0)); double sampleDelta = numSamples;
    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    int cycleBars = (cycleIndex == 0) ? 1 : (cycleIndex == 1) ? 2 : (cycleIndex == 2) ? 4 : 8;
    currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % cycleBars) + 1;

    // Real-Time Crossfader Morphing Interpolation
    float morphVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
    auto morphValue = [&](float valA, float valB) -> float { return (valA * (1.0f - morphVal)) + (valB * morphVal); };

    // Symmetrically interpolate base parameters from our active Scene A and Scene B snapshots
    float baseMorph   = morphValue (sceneA.rhythmMorph, sceneB.rhythmMorph);
    float baseRest    = morphValue (sceneA.rest,        sceneB.rest);
    float baseLegato  = morphValue (sceneA.legato,      sceneB.legato);
    float baseRate    = morphValue (sceneA.rate,        sceneB.rate);
    float baseEntropy = morphValue (sceneA.entropy,     sceneB.entropy);
    float baseHarmony = morphValue (sceneA.harmony,     sceneB.harmony);
    float baseChaos   = morphValue (sceneA.chaos,       sceneB.chaos);
    float baseOctaves = morphValue (sceneA.octaves,     sceneB.octaves);

    // 8-Channel LFO Modulation Matrix Lambda
    auto applyLfo = [&](int index, float baseVal, juce::ParameterID rateId, juce::ParameterID depthId, float minVal, float maxVal) -> float {
        int rateChoice = static_cast<int> (*apvts.getRawParameterValue (rateId.getParamID()));
        float depth = *apvts.getRawParameterValue (depthId.getParamID());
        if (rateChoice == 0) return baseVal;
        double divPPQ = (rateChoice == 1) ? 1.0 : (rateChoice == 2) ? 0.5 : (rateChoice == 3) ? 0.25 : 0.125;
        double periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples); if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        float sineVal = static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi));
        return juce::jlimit (minVal, maxVal, baseVal + (sineVal * depth * ((maxVal - minVal) * 0.5f)));
    };

    activeMorph   = applyLfo (0, baseMorph,   IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, 0.0f, 1.0f);
    activeRest    = applyLfo (1, baseRest,    IDs::restLfoRate,        IDs::restLfoDepth,        0.0f, 1.0f);
    activeLegato  = applyLfo (2, baseLegato,  IDs::legatoLfoRate,      IDs::legatoLfoDepth,      0.1f, 1.0f);
    
    // Smart Legato Overrides (CW turn above 0.8 automatically dampens and silences Rests for Pads)
    modLegato = activeLegato;
    if (activeLegato >= 0.8f)
    {
        float dampFactor = juce::jlimit (0.0f, 1.0f, (1.0f - activeLegato) / 0.2f);
        modRest = activeRest * dampFactor;
    }
    else
    {
        modRest = activeRest;
    }

    float rawRate = applyLfo (3, baseRate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));
    activeEntropy = applyLfo (4, baseEntropy, IDs::entropyLfoRate, IDs::entropyLfoDepth, -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyLfo (5, baseHarmony, IDs::harmonyLfoRate, IDs::harmonyLfoDepth, 0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos   = applyLfo (6, baseChaos,   IDs::chaosLfoRate,   IDs::chaosLfoDepth,   0.0f, 1.0f); modChaos = activeChaos;

    // Symmetrical 7-point Octaves selection (-3 to +3)
    float rawOctaves = applyLfo (7, baseOctaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, -3.0f, 3.0f);
    activeOctavesVal = juce::jlimit (-3, 3, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0) midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    else scheduledNoteOffs.push_back ({ pitch, delaySamples });
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear(); bool isPlaying = false; double bpm = 120.0; mSongPositionPPQ = 0.0;
#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead()) {
        if (auto pos = playhead->getPosition()) {
            isPlaying = pos->getIsPlaying();
            auto bpmOpt = pos->getBpm(); if (bpmOpt.hasValue()) bpm = *bpmOpt;
            auto ppqOpt = pos->getPpqPosition(); if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt;
        }
    }
#else
    if (auto* playhead = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (playhead->getCurrentPositionInfo (info)) { isPlaying = info.isPlaying; bpm = info.bpm; mSongPositionPPQ = info.ppqPosition; }
    }
#endif
    int numSamples = buffer.getNumSamples(); updateLfoModulations (numSamples, bpm);
    
    // Parameter Slew calculations over ~150ms window
    float slewFactor = static_cast<float>(1.0 - std::exp (-1.0 / (0.15 * mSampleRate)));
    for (int i = 0; i < 24; ++i) {
        currentSlewValue[i] += (currentSlewTarget[i] - currentSlewValue[i]) * slewFactor;
    }

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;
    bool isArpActive = *apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f; // SEQ/ARP dynamic switcher
    bool isPolyActive = *apvts.getRawParameterValue (IDs::poly.getParamID()) > 0.5f; // POLY mode
    bool isFreezeActive = *apvts.getRawParameterValue (IDs::freeze.getParamID()) > 0.5f; // FREEZE mode

    float activeFaderProb[8]; for (int i = 0; i < 8; ++i) activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0); it = scheduledNoteOffs.erase(it); }
        else ++it;
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn() && !isFreezeActive) { // Ignore new keyboard events when FREEZE is armed
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end()) { activeHeldNotes.push_back (note); std::sort (activeHeldNotes.begin(), activeHeldNotes.end()); }
            if (isLatchActive) {
                if (isFirstNoteOfNewChord) { for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0); latchedNotes.clear(); isFirstNoteOfNewChord = false; }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end()) { latchedNotes.push_back (note); std::sort (latchedNotes.begin(), latchedNotes.end()); }
            }
        } else if (msg.isNoteOff() && !isFreezeActive) {
            int note = msg.getNoteNumber(); activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear();
    
    // Raw inputs setup
    std::vector<int> notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    isCurrentlyPlayingUI.store (!notesToPlay.empty() || isFreezeActive);

    if (! notesToPlay.empty() || isFreezeActive) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        double stepLengthPPQ = (activeRateIdx == 0) ? 1.0 : (activeRateIdx == 1) ? 0.5 : (activeRateIdx == 2) ? 0.25 : 0.125;
        double stepSamples = samplesPerBeat * stepLengthPPQ;

        // Dynamic Swing timing offset
        double swingOffset = 0.08 * activeMorph * stepLengthPPQ; 
        double triggerThreshold = stepLengthPPQ;
        if (currentStep % 2 == 1) {
            triggerThreshold += swingOffset;
        }

        if (isPlaying) {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep) { mLastStep = stepIndex; currentStep = stepIndex; stepTriggered = true; }
        } else {
            double activeStepSamples = stepSamples;
            if (currentStep % 2 == 0) activeStepSamples *= (1.0 + (swingOffset * 4.0));
            else activeStepSamples *= (1.0 - (swingOffset * 4.0));

            mTimeInSamples += numSamples;
            if (mTimeInSamples >= activeStepSamples) { mTimeInSamples = 0; stepTriggered = true; }
        }

        if (stepTriggered) {
            // Playhead directions inside Entropy
            int playDirection = 0; 
            if (activeEntropy >= -0.1f && activeEntropy <= 0.1f) playDirection = 0;
            else if (activeEntropy > 0.1f && activeEntropy <= 0.5f) playDirection = 1;
            else if (activeEntropy > 0.5f) playDirection = 2;
            else if (activeEntropy < -0.1f && activeEntropy >= -0.5f) playDirection = 3;
            else if (activeEntropy < -0.5f) playDirection = 4;

            static bool goingForward = true;
            if (playDirection == 1) { 
                if (goingForward) { currentStep++; if (currentStep >= 7) { currentStep = 7; goingForward = false; } }
                else { currentStep--; if (currentStep <= 0) { currentStep = 0; goingForward = true; } }
            } else if (playDirection == 2) { 
                float rVal = juce::Random::getSystemRandom().nextFloat();
                if (rVal < 0.7f) currentStep = (currentStep + 1) % 8;
                else if (rVal < 0.9f) currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 3) { 
                currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 4) { 
                currentStep = (juce::Random::getSystemRandom().nextFloat() < 0.2f) ? (currentStep + 2) % 8 : (currentStep + 1) % 8;
            } else { 
                if (isPlaying) {
                    currentStep = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
                } else {
                    currentStep = (currentStep + 1) % 8;
                }
            }
            mLastStep = currentStep;

            float faderProb = activeFaderProb[currentStep];
            if (juce::Random::getSystemRandom().nextFloat() <= faderProb && !(juce::Random::getSystemRandom().nextFloat() <= modRest)) {
                if (mLastNotePlayed != -1) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0); mLastNotePlayed = -1; }
                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));
                std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; 
                if (scaleIdx == 1) scaleOffsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; 
                else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; 
                else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  
                else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  
                else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  
                else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  
                else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  
                else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  

                // 1. Get raw base note based on active play mode 
                int rawPitch = 60;
                if (isArpActive && !notesToPlay.empty())
                {
                    // ARPEGGIATOR: cycles dynamically through held note pool 
                    rawPitch = notesToPlay[currentStep % notesToPlay.size()];
                }
                else
                {
                    // SEQUENCER: plays fixed scale-degree from active preset faders 
                    int scaleDeg = scaleOffsets[currentStep % scaleOffsets.size()];
                    rawPitch = 48 + rootKeyIdx + scaleDeg;
                }

                // 2. Diatonic Note Thinning (Harmony knob filters chords from 5 down to 1 note) 
                std::vector<int> pitchList;
                pitchList.push_back (rawPitch);
                
                if (isPolyActive && notesToPlay.size() > 1) // Polyphonic Cluster Mode 
                {
                    int maxAllowedNotes = 1;
                    if (modHarmony > 0.25f && modHarmony < 0.5f)       maxAllowedNotes = 2; // Intervals
                    else if (modHarmony >= 0.5f && modHarmony < 0.75f) maxAllowedNotes = 3; // Triads
                    else if (modHarmony >= 0.75f)                      maxAllowedNotes = 5; // Full chords
                    
                    for (size_t n = 1; n < notesToPlay.size() && pitchList.size() < static_cast<size_t>(maxAllowedNotes); ++n)
                    {
                        pitchList.push_back (notesToPlay[n]);
                    }
                }

                // 3. Symmetrical Scale-locking semitone quantizer (Snaps Eb -> E inside C Major) 
                for (auto& pitch : pitchList)
                {
                    int noteOffset = (pitch - rootKeyIdx) % 12;
                    int octaveBase = ((pitch - rootKeyIdx) / 12) * 12 + rootKeyIdx;
                    
                    // Simple round-to-nearest semitone quantizer 
                    int nearestVal = scaleOffsets[0];
                    int minDiff = 12;
                    for (int offset : scaleOffsets)
                    {
                        int diff = std::abs (offset - noteOffset);
                        if (diff < minDiff) { minDiff = diff; nearestVal = offset; }
                    }
                    pitch = octaveBase + nearestVal;
                }

                // 4. Intellijel Metropolis Range additions (Octaves, ratchets, slides, etc.) 
                int rangeShift = activeOctavesVal; 
                int octaveShiftCount = 0;
                if (rangeShift > 0) octaveShiftCount = (currentStep / 2) % (rangeShift + 1); 
                else if (rangeShift < 0) octaveShiftCount = -((currentStep / 2) % (std::abs(rangeShift) + 1)); 

                for (auto pitch : pitchList)
                {
                    int targetPitch = pitch + (octaveShiftCount * 12);
                    if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos) 
                        targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);
                    
                    targetPitch = juce::jlimit(0, 127, targetPitch); 
                    int durationSamples = static_cast<int>(stepSamples * modLegato);

                    processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                    mLastNotePlayed = targetPitch; mNoteOffTime = durationSamples; 
                    scheduleNoteOff (processedMidi, targetPitch, durationSamples);
                }
            }
        }
    } else { if (mLastStep != -1) { if (mLastNotePlayed != -1) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0); mLastNotePlayed = -1; } mLastStep = -1; } currentStep = 0; }
    midiMessages.swapWith (processedMidi);
}

void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));
    std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11 };
    if (scaleIdx == 1) scaleOffsets = { 0, 2, 3, 5, 7, 8, 10 };
    else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 14 };
    else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14 };
    else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10 };
    else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10 };
    else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11 };
    else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10 };
    else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11 };
    else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11 };

    auto getScalePitch = [&](int degree) -> int { return scaleOffsets[degree % 7] + ((degree / 7) * 12); };
    int baseRoot = 48 + rootIdx;
    int r = getScalePitch (padIndex), t = getScalePitch (padIndex + 2), f = getScalePitch (padIndex + 4);

    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f) { t = getScalePitch (padIndex + 3); newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }
    else if (activeHarmony >= 0.67f) { int s = getScalePitch (padIndex + 6); newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s }; }
    else { newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }

    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size()) {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; else if (pitchDiff < -5) newChord[0] += 12; 
    }
    std::sort(newChord.begin(), newChord.end()); lastChordPitches = newChord;
    latchedNotes = newChord; apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

void PluginProcessor::savePreset (int slotIndex) 
{ 
    if (slotIndex >= 0 && slotIndex < 8) 
    { 
        // Save the active scene profiles directly into the Project 
        sceneAPresets[slotIndex] = sceneA;
        sceneBPresets[slotIndex] = sceneB;
        sceneASlotsSaved[slotIndex] = hasSceneA;
        sceneBSlotsSaved[slotIndex] = hasSceneB;

        for (int i = 0; i < 8; ++i) 
            presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); 
        presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); 
        presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); 
        presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); 
        presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); 
        presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); 
        presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); 
        presets[slotIndex].rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
        presets[slotIndex].octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());
        
        juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
        juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
        for (int i = 0; i < 8; ++i) { 
            presets[slotIndex].lfoRates[i] = static_cast<int> (*apvts.getRawParameterValue (rates[i].getParamID())); 
            presets[slotIndex].lfoDepths[i] = *apvts.getRawParameterValue (depths[i].getParamID()); 
        }
        
        presetSlotsSaved[slotIndex] = true; 
        activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::loadPreset (int slotIndex) 
{ 
    if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex]) 
    { 
        // Load the stored project scenes into the active buffers 
        sceneA = sceneAPresets[slotIndex];
        sceneB = sceneBPresets[slotIndex];
        hasSceneA = sceneASlotsSaved[slotIndex];
        hasSceneB = sceneBSlotsSaved[slotIndex];

        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph); 
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest); 
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato); 
        
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((presets[slotIndex].entropy + 1.0f) * 0.5f); 
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony); 
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos); 
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (presets[slotIndex].rate / 3.0f);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((presets[slotIndex].octaves + 3.0f) / 6.0f); // -3 to 3 normalizer 

        juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
        juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
        for (int i = 0; i < 8; ++i) { 
            apvts.getParameter (rates[i].getParamID())->setValueNotifyingHost (static_cast<float>(presets[slotIndex].lfoRates[i]) / 4.0f); 
            apvts.getParameter (depths[i].getParamID())->setValueNotifyingHost (presets[slotIndex].lfoDepths[i]); 
        }

        for (int i = 0; i < 8; ++i) 
            apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]); 
            
        activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::captureScene (int side) 
{ 
    SceneState& s = (side == 0) ? sceneA : sceneB;
    for (int i = 0; i < 8; ++i) s.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); 
    s.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); 
    s.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); 
    s.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); 
    s.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); 
    s.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); 
    s.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); 
    s.rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
    s.octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());
    
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) { 
        s.lfoRates[i] = static_cast<int> (*apvts.getRawParameterValue (rates[i].getParamID())); 
        s.lfoDepths[i] = *apvts.getRawParameterValue (depths[i].getParamID()); 
    }
    if (side == 0) hasSceneA = true; else hasSceneB = true; 
}

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

void PluginProcessor::diceMelody() {
    auto* r = &juce::Random::getSystemRandom();
    for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (r->nextFloat());
}

// Scoped LFO-protected randomizers
void PluginProcessor::diceArticulation() {
    auto* r = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (r->nextFloat() * 0.5f);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.1f + r->nextFloat() * 0.9f);
}

void PluginProcessor::diceTime() {
    auto* r = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(4)) / 3.0f);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(7)) / 6.0f); // -3 to 3 
    apvts.getParameter (IDs::cycleLength.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(4)) / 3.0f);
}

void PluginProcessor::diceNavy() {
    auto* r = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (r->nextFloat());
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (r->nextFloat());
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (r->nextFloat());
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (r->nextFloat());
}

// Active Anchor Direct background Scene Randomizers
void PluginProcessor::diceActiveSceneA()
{
    auto* random = &juce::Random::getSystemRandom();
    for (int i = 0; i < 8; ++i) sceneA.faders[i] = random->nextFloat();
    sceneA.rhythmMorph = random->nextFloat(); 
    sceneA.rest = random->nextFloat() * 0.5f; 
    sceneA.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneA.entropy = -1.0f + random->nextFloat() * 2.0f; 
    sceneA.harmony = random->nextFloat(); 
    sceneA.chaos = random->nextFloat();
    sceneA.rate = static_cast<float> (random->nextInt (4)); 
    sceneA.octaves = static_cast<float> (random->nextInt (7) - 3); // -3 to 3 
    hasSceneA = true;
}

void PluginProcessor::diceActiveSceneB()
{
    auto* random = &juce::Random::getSystemRandom();
    for (int i = 0; i < 8; ++i) sceneB.faders[i] = random->nextFloat();
    sceneB.rhythmMorph = random->nextFloat(); 
    sceneB.rest = random->nextFloat() * 0.5f; 
    sceneB.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneB.entropy = -1.0f + random->nextFloat() * 2.0f; 
    sceneB.harmony = random->nextFloat(); 
    sceneB.chaos = random->nextFloat();
    sceneB.rate = static_cast<float> (random->nextInt (4)); 
    sceneB.octaves = static_cast<float> (random->nextInt (7) - 3); // -3 to 3 
    hasSceneB = true;
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    auto* presetsNodeA = xml->createNewChildElement ("SCENE_A_PRESETS");
    auto* presetsNodeB = xml->createNewChildElement ("SCENE_B_PRESETS");

    for (int i = 0; i < 8; ++i) 
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

    auto* banksNode = xml->createNewChildElement ("GLOBAL_BANKS");
    for (int i = 0; i < 8; ++i)
    {
        auto* childBank = banksNode->createNewChildElement ("BANK_" + juce::String (i));
        childBank->setAttribute ("saved", presetSlotsSaved[i]);
        if (presetSlotsSaved[i]) {
            childBank->setAttribute ("morph", presets[i].rhythmMorph);
            childBank->setAttribute ("rest", presets[i].rest);
            childBank->setAttribute ("legato", presets[i].legato);
            childBank->setAttribute ("rate", presets[i].rate);
            childBank->setAttribute ("entropy", presets[i].entropy);
            childBank->setAttribute ("harmony", presets[i].harmony);
            childBank->setAttribute ("chaos", presets[i].chaos);
            childBank->setAttribute ("octaves", presets[i].octaves);
            for (int f = 0; f < 8; ++f) childBank->setAttribute ("fader_" + juce::String (f), presets[i].faders[f]);
            for (int l = 0; l < 8; ++l) {
                childBank->setAttribute ("lfo_r_" + juce::String (l), presets[i].lfoRates[l]);
                childBank->setAttribute ("lfo_d_" + juce::String (l), presets[i].lfoDepths[l]);
            }
        }
    }

    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        if (auto* presetsNodeA = xmlState->getChildByName ("SCENE_A_PRESETS"))
        {
            for (int i = 0; i < 8; ++i) 
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
            for (int i = 0; i < 8; ++i) 
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
        if (auto* banksNode = xmlState->getChildByName ("GLOBAL_BANKS"))
        {
            for (int i = 0; i < 8; ++i)
            {
                if (auto* childBank = banksNode->getChildByName ("BANK_" + juce::String (i))) {
                    presetSlotsSaved[i] = childBank->getBoolAttribute ("saved");
                    if (presetSlotsSaved[i]) {
                        presets[i].rhythmMorph = childBank->getDoubleAttribute ("morph");
                        presets[i].rest = childBank->getDoubleAttribute ("rest");
                        presets[i].legato = childBank->getDoubleAttribute ("legato");
                        presets[i].rate = childBank->getDoubleAttribute ("rate");
                        presets[i].entropy = childBank->getDoubleAttribute ("entropy");
                        presets[i].harmony = childBank->getDoubleAttribute ("harmony");
                        presets[i].chaos = childBank->getDoubleAttribute ("chaos");
                        presets[i].octaves = childBank->getDoubleAttribute ("octaves");
                        for (int f = 0; f < 8; ++f) presets[i].faders[f] = childBank->getDoubleAttribute ("fader_" + juce::String (f));
                        for (int l = 0; l < 8; ++l) {
                            presets[i].lfoRates[l] = childBank->getIntAttribute ("lfo_r_" + juce::String (l));
                            presets[i].lfoDepths[l] = childBank->getDoubleAttribute ("lfo_d_" + juce::String (l));
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
    
    // Performance Deck Parameters 
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::arpSeq, "SEQ/ARP Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::poly, "Poly Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::freeze, "Freeze Mode", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); 
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rate, "Rate", juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); 
    
    // Unified -3 to +3 Octaves parameter
    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", -3, 3, 0)); 

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::panelTheme, "Panel Theme", juce::StringArray { "Navy Cyber", "Skyline Eurorack", "Monochrome Minimal", "Matrix Terminal" }, 1));

    auto registerLfoParams = [&](juce::ParameterID rateId, juce::ParameterID depthId, juce::String name) {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (rateId, name + " LFO Speed", juce::StringArray { "Off", "1/4", "1/8", "1/16", "1/32" }, 0));
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }