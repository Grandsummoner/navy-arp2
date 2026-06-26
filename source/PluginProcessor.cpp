#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState(); sceneB = SceneState(); lastChordPitches = { 60, 64, 67 }; 
    for (int i = 0; i < 4; ++i) { sceneAPresets[i] = SceneState(); sceneBPresets[i] = SceneState(); }
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

    // Real-Time Crossfader Morphing Interpolation [NEW]
    float morphVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
    int activeA = activeSceneAIndex.load(), activeB = activeSceneBIndex.load();
    auto morphValue = [&](float valA, float valB) -> float { return (valA * (1.0f - morphVal)) + (valB * morphVal); };

    // Symmetrically interpolate base parameters [NEW]
    float baseMorph   = morphValue (sceneAPresets[activeA].rhythmMorph, sceneBPresets[activeB].rhythmMorph);
    float baseRest    = morphValue (sceneAPresets[activeA].rest,        sceneBPresets[activeB].rest);
    float baseLegato  = morphValue (sceneAPresets[activeA].legato,      sceneBPresets[activeB].legato);
    float baseRate    = morphValue (sceneAPresets[activeA].rate,        sceneBPresets[activeB].rate);
    float baseEntropy = morphValue (sceneAPresets[activeA].entropy,     sceneBPresets[activeB].entropy);
    float baseHarmony = morphValue (sceneAPresets[activeA].harmony,     sceneBPresets[activeB].harmony);
    float baseChaos   = morphValue (sceneAPresets[activeA].chaos,       sceneBPresets[activeB].chaos);
    float baseOctaves = morphValue (sceneAPresets[activeA].octaves,     sceneBPresets[activeB].octaves);

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
    modRest = activeRest; modLegato = activeLegato;

    float rawRate = applyLfo (3, baseRate, IDs::rateLfoRate, IDs::rateLfoDepth, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));
    activeEntropy = applyLfo (4, baseEntropy, IDs::entropyLfoRate, IDs::entropyLfoDepth, -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyLfo (5, baseHarmony, IDs::harmonyLfoRate, IDs::harmonyLfoDepth, 0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos   = applyLfo (6, baseChaos,   IDs::chaosLfoRate,   IDs::chaosLfoDepth,   0.0f, 1.0f); modChaos = activeChaos;

    float rawOctaves = applyLfo (7, baseOctaves, IDs::octavesLfoRate, IDs::octavesLfoDepth, -2.0f, 3.0f);
    activeOctavesVal = juce::jlimit (-2, 3, static_cast<int> (std::round (rawOctaves)));
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
    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;
    float activeFaderProb[8]; for (int i = 0; i < 8; ++i) activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0); it = scheduledNoteOffs.erase(it); }
        else ++it;
    }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end()) { activeHeldNotes.push_back (note); std::sort (activeHeldNotes.begin(), activeHeldNotes.end()); }
            if (isLatchActive) {
                if (isFirstNoteOfNewChord) { for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0); latchedNotes.clear(); isFirstNoteOfNewChord = false; }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end()) { latchedNotes.push_back (note); std::sort (latchedNotes.begin(), latchedNotes.end()); }
            }
        } else if (msg.isNoteOff()) {
            int note = msg.getNoteNumber(); activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }
    midiMessages.clear(); const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    isCurrentlyPlayingUI.store (!notesToPlay.empty());

    if (! notesToPlay.empty()) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        double stepLengthPPQ = (activeRateIdx == 0) ? 1.0 : (activeRateIdx == 1) ? 0.5 : (activeRateIdx == 2) ? 0.25 : 0.125;
        double stepSamples = samplesPerBeat * stepLengthPPQ;

        // Built-In performance MPC Swing clock calculation [NEW]
        double swingOffset = 0.08 * activeMorph * stepLengthPPQ; // up to 33% swing offset delay
        double triggerThreshold = stepLengthPPQ;
        if (currentStep % 2 == 1) {
            triggerThreshold += swingOffset;
        }

        if (isPlaying) {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep) { mLastStep = stepIndex; currentStep = stepIndex; stepTriggered = true; }
        } else {
            // Apply swing to standalone sample clock
            double activeStepSamples = stepSamples;
            if (currentStep % 2 == 0) activeStepSamples *= (1.0 + (swingOffset * 4.0));
            else activeStepSamples *= (1.0 - (swingOffset * 4.0));

            mTimeInSamples += numSamples;
            if (mTimeInSamples >= activeStepSamples) { mTimeInSamples = 0; stepTriggered = true; }
        }

        if (stepTriggered) {
            // Generative Playhead Directions inside Entropy [NEW]
            int playDirection = 0; // 0 = forward, 1 = ping-pong, 2 = drunkard, 3 = reverse, 4 = step-skip
            if (activeEntropy >= -0.1f && activeEntropy <= 0.1f) playDirection = 0;
            else if (activeEntropy > 0.1f && activeEntropy <= 0.5f) playDirection = 1;
            else if (activeEntropy > 0.5f) playDirection = 2;
            else if (activeEntropy < -0.1f && activeEntropy >= -0.5f) playDirection = 3;
            else if (activeEntropy < -0.5f) playDirection = 4;

            static bool goingForward = true;
            if (playDirection == 1) { // Symmetrical Ping-Pong
                if (goingForward) { currentStep++; if (currentStep >= 7) { currentStep = 7; goingForward = false; } }
                else { currentStep--; if (currentStep <= 0) { currentStep = 0; goingForward = true; } }
            } else if (playDirection == 2) { // Drunkard Walk [5]
                float rVal = juce::Random::getSystemRandom().nextFloat();
                if (rVal < 0.7f) currentStep = (currentStep + 1) % 8;
                else if (rVal < 0.9f) currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 3) { // Reverse
                currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 4) { // Step-skip
                currentStep = (juce::Random::getSystemRandom().nextFloat() < 0.2f) ? (currentStep + 2) % 8 : (currentStep + 1) % 8;
            } else { // Standard Forward
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

                int rawPitch = notesToPlay[currentStep % notesToPlay.size()];
                int octave = (rawPitch / 12) * 12;

                // Symmetrical octave climbing range (-2 to +3) [NEW]
                int rangeShift = activeOctavesVal; 
                int octaveShiftCount = 0;
                if (rangeShift > 0) octaveShiftCount = (currentStep / 2) % (rangeShift + 1); 
                else if (rangeShift < 0) octaveShiftCount = -((currentStep / 2) % (std::abs(rangeShift) + 1)); 

                int targetPitch = octave + rootKeyIdx + scaleOffsets[currentStep] + static_cast<int>(accumulatedPitchOffset) + (octaveShiftCount * 12);
                if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos) targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);
                targetPitch = juce::jlimit(0, 127, targetPitch); int durationSamples = static_cast<int>(stepSamples * modLegato);

                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetPitch; mNoteOffTime = durationSamples; scheduleNoteOff (processedMidi, targetPitch, durationSamples);
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

void PluginProcessor::savePreset (int slotIndex) { if (slotIndex >= 0 && slotIndex < 8) { for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); presetSlotsSaved[slotIndex] = true; } }
void PluginProcessor::loadPreset (int slotIndex) { if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex]) { apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph); apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest); apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato); apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy); apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony); apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos); for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]); } }
void PluginProcessor::captureSceneA() { for (int i = 0; i < 8; ++i) sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); sceneA.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); sceneA.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); sceneA.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); sceneA.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); sceneA.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); sceneA.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); hasSceneA = true; }
void PluginProcessor::captureSceneB() { for (int i = 0; i < 8; ++i) sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); sceneB.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); sceneB.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); sceneB.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); sceneB.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); sceneB.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); sceneB.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); hasSceneB = true; }
void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

void PluginProcessor::diceMelody() {
    auto* r = &juce::Random::getSystemRandom();
    for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (r->nextFloat());
}

void PluginProcessor::diceRhythm() {
    auto* r = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (r->nextFloat());
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (r->nextFloat() * 0.5f);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.2f + r->nextFloat() * 0.8f);
}

// Symmetrical Direct background randomizers (No focus state variables) [NEW]
void PluginProcessor::diceActiveSceneA()
{
    auto* random = &juce::Random::getSystemRandom();
    int focusedA = activeSceneAIndex.load();
    
    for (int i = 0; i < 8; ++i) sceneAPresets[focusedA].faders[i] = random->nextFloat();
    sceneAPresets[focusedA].rhythmMorph = random->nextFloat(); 
    sceneAPresets[focusedA].rest = random->nextFloat() * 0.5f; 
    sceneAPresets[focusedA].legato = 0.2f + random->nextFloat() * 0.8f;
    sceneAPresets[focusedA].entropy = -1.0f + random->nextFloat() * 2.0f; 
    sceneAPresets[focusedA].harmony = random->nextFloat(); 
    sceneAPresets[focusedA].chaos = random->nextFloat();
    sceneAPresets[focusedA].rate = static_cast<float> (random->nextInt (4)); 
    sceneAPresets[focusedA].octaves = static_cast<float> (random->nextInt (6) - 2); 
    
    for (int i = 0; i < 8; ++i) { 
        sceneAPresets[focusedA].lfoRates[i] = random->nextInt (5); 
        sceneAPresets[focusedA].lfoDepths[i] = random->nextFloat() * 0.5f; 
    }
    
    sceneASlotsSaved[focusedA] = true; 
    loadSceneA (focusedA);
}

void PluginProcessor::diceActiveSceneB()
{
    auto* random = &juce::Random::getSystemRandom();
    int focusedB = activeSceneBIndex.load();
    
    for (int i = 0; i < 8; ++i) sceneBPresets[focusedB].faders[i] = random->nextFloat();
    sceneBPresets[focusedB].rhythmMorph = random->nextFloat(); 
    sceneBPresets[focusedB].rest = random->nextFloat() * 0.5f; 
    sceneBPresets[focusedB].legato = 0.2f + random->nextFloat() * 0.8f;
    sceneBPresets[focusedB].entropy = -1.0f + random->nextFloat() * 2.0f; 
    sceneBPresets[focusedB].harmony = random->nextFloat(); 
    sceneBPresets[focusedB].chaos = random->nextFloat();
    sceneBPresets[focusedB].rate = static_cast<float> (random->nextInt (4)); 
    sceneBPresets[focusedB].octaves = static_cast<float> (random->nextInt (6) - 2); 
    
    for (int i = 0; i < 8; ++i) { 
        sceneBPresets[focusedB].lfoRates[i] = random->nextInt (5); 
        sceneBPresets[focusedB].lfoDepths[i] = random->nextFloat() * 0.5f; 
    }
    
    sceneBSlotsSaved[focusedB] = true; 
    loadSceneB (focusedB);
}

void PluginProcessor::saveScene (int slotIndex, int side) {
    SceneState& s = (side == 0) ? sceneAPresets[slotIndex] : sceneBPresets[slotIndex];
    for (int i = 0; i < 8; ++i) s.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    s.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); s.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); s.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    s.rate = *apvts.getRawParameterValue (IDs::rate.getParamID()); s.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); s.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    s.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); s.octaves = *apvts.getRawParameterValue (IDs::octaves.getParamID());
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) { s.lfoRates[i] = static_cast<int> (*apvts.getRawParameterValue (rates[i].getParamID())); s.lfoDepths[i] = *apvts.getRawParameterValue (depths[i].getParamID()); }
    if (side == 0) sceneASlotsSaved[slotIndex] = true; else sceneBSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadScene (int slotIndex, int side) {
    if ((side == 0 && !sceneASlotsSaved[slotIndex]) || (side == 1 && !sceneBSlotsSaved[slotIndex])) return;
    SceneState& s = (side == 0) ? sceneAPresets[slotIndex] : sceneBPresets[slotIndex];
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (s.rhythmMorph); apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (s.rest); apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (s.legato);
    
    // Symmetrical Normalizations for state loading [5] [NEW]
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (s.rate / 3.0f); 
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((s.entropy + 1.0f) * 0.5f); 
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (s.harmony); apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (s.chaos); 
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((s.octaves + 2.0f) / 5.0f); // Map -2 to 3 onto 0.0 to 1.0 range [5] [NEW]
    
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) { apvts.getParameter (rates[i].getParamID())->setValueNotifyingHost (static_cast<float>(s.lfoRates[i]) / 4.0f); apvts.getParameter (depths[i].getParamID())->setValueNotifyingHost (s.lfoDepths[i]); }
    for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (s.faders[i]);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
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

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); 
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rate, "Rate", juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); 
    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", -2, 3, 0)); // Scaled symmetrically from -2 to +3 [NEW]

    // Default choice index changed to 1 (Skyline Eurorack) [NEW]
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