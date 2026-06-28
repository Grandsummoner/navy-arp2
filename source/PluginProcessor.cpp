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
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate; mLastStep = -1; mLastNotePlayed = -1; mNoteOffTime = 0; mTimeInSamples = 0;
    activeHeldNotes.clear(); latchedNotes.clear(); scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true; 
    lastSceneBActiveState = isSceneBActiveAnchor.load();
    lastFreezeState = false;
    std::fill (std::begin (currentSlewValue), std::end (currentSlewValue), 0.5f);
    std::fill (std::begin (currentSlewTarget), std::end (currentSlewTarget), 0.5f);
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0) midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    else scheduledNoteOffs.push_back ({ pitch, delaySamples });
}

// Deep Scene Clear Reset Functions
void PluginProcessor::clearSceneA()
{
    hasSceneA = false;
    sceneA = SceneState(); // Reset stored parameters in A's memory buffer to defaults
}

void PluginProcessor::clearSceneB()
{
    hasSceneB = false;
    sceneB = SceneState(); // Reset stored parameters in B's memory buffer to defaults
}

void PluginProcessor::clearPreset (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        presetSlotsSaved[slotIndex] = false;
        presets[slotIndex] = SceneState(); // Reset to default empty state
        sceneASlotsSaved[slotIndex] = false;
        sceneBSlotsSaved[slotIndex] = false;
        sceneAPresets[slotIndex] = SceneState();
        sceneBPresets[slotIndex] = SceneState();
    }
}

void PluginProcessor::setActiveAnchor (bool useSceneB)
{
    if (isSceneBActiveAnchor.load() == useSceneB) return;
    captureScene (isSceneBActiveAnchor.load() ? 1 : 0);
    isSceneBActiveAnchor.store (useSceneB); lastSceneBActiveState = useSceneB;
    
    SceneState& targetScene = useSceneB ? sceneB : sceneA;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (targetScene.rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (targetScene.rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (targetScene.legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((targetScene.entropy + 1.0f) * 0.5f);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (targetScene.harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (targetScene.chaos);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate / 3.0f);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);
}

void PluginProcessor::captureActiveParametersToActiveScene()
{
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    for (int i = 0; i < 8; ++i)
        activeScene.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    activeScene.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    activeScene.rest        = *apvts.getRawParameterValue (IDs::rest.getParamID());
    activeScene.legato      = *apvts.getRawParameterValue (IDs::legato.getParamID());
    activeScene.entropy     = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    activeScene.harmony     = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    activeScene.chaos       = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    activeScene.rate        = *apvts.getRawParameterValue (IDs::rate.getParamID());
    activeScene.octaves     = static_cast<float> (*apvts.getRawParameterValue (IDs::octaves.getParamID()));
    
    // Save LFO preset selections to active scene state
    juce::ParameterID lfoIds[] = { IDs::rhythmMorphLfoPreset, IDs::restLfoPreset, IDs::legatoLfoPreset, IDs::rateLfoPreset, IDs::entropyLfoPreset, IDs::harmonyLfoPreset, IDs::chaosLfoPreset, IDs::octavesLfoPreset };
    for (int i = 0; i < 8; ++i)
        activeScene.lfoPresets[i] = static_cast<int> (*apvts.getRawParameterValue (lfoIds[i].getParamID()));
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    captureActiveParametersToActiveScene();
    bool isSceneBActive = isSceneBActiveAnchor.load();
    if (isSceneBActive != lastSceneBActiveState)
    {
        lastSceneBActiveState = isSceneBActive;
        SceneState& targetScene = isSceneBActive ? sceneB : sceneA;
        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (targetScene.rhythmMorph);
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (targetScene.rest);
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (targetScene.legato);
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((targetScene.entropy + 1.0f) * 0.5f);
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (targetScene.harmony);
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (targetScene.chaos);
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate / 3.0f);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
        
        juce::ParameterID lfoIds[] = { IDs::rhythmMorphLfoPreset, IDs::restLfoPreset, IDs::legatoLfoPreset, IDs::rateLfoPreset, IDs::entropyLfoPreset, IDs::harmonyLfoPreset, IDs::chaosLfoPreset, IDs::octavesLfoPreset };
        for (int i = 0; i < 8; ++i)
            apvts.getParameter (lfoIds[i].getParamID())->setValueNotifyingHost (static_cast<float>(targetScene.lfoPresets[i]) / 4.0f);

        for (int i = 0; i < 8; ++i)
            apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);
    }

    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0)), sampleDelta = numSamples;
    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % ((cycleIndex == 0) ? 1 : (cycleIndex == 1) ? 2 : (cycleIndex == 2) ? 4 : 8)) + 1;

    float morphVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
    auto morphValue = [&](float valA, float valB) -> float { return (valA * (1.0f - morphVal)) + (valB * morphVal); };

    float baseMorph = morphValue (sceneA.rhythmMorph, sceneB.rhythmMorph), baseRest = morphValue (sceneA.rest, sceneB.rest), baseLegato = morphValue (sceneA.legato, sceneB.legato), baseRate = morphValue (sceneA.rate, sceneB.rate);
    float baseEntropy = morphValue (sceneA.entropy, sceneB.entropy), baseHarmony = morphValue (sceneA.harmony, sceneB.harmony), baseChaos = morphValue (sceneA.chaos, sceneB.chaos), baseOctaves = morphValue (sceneA.octaves, sceneB.octaves);

    // Audio-thread 5-way LFO evaluation loop (Off + 4 Curated sweet spots)
    auto applyPresetLfo = [&](int index, float baseVal, juce::ParameterID presetId, float minVal, float maxVal) -> float {
        int presetChoice = static_cast<int> (*apvts.getRawParameterValue (presetId.getParamID()));
        if (presetChoice == 0) return baseVal;
        
        double ratePPQ = 1.0; float depth = 0.0f;
        if (presetChoice == 1)      { ratePPQ = 1.0;   depth = 0.15f; } // Drift (1/4 Note, 15% depth)
        else if (presetChoice == 2) { ratePPQ = 0.5;   depth = 0.35f; } // Bounce (1/8 Note, 35% depth)
        else if (presetChoice == 3) { ratePPQ = 0.25;  depth = 0.50f; } // Vibe (1/16 Note, 50% depth)
        else if (presetChoice == 4) { ratePPQ = 0.125; depth = 0.75f; } // Tremor (1/32 Note, 75% depth)

        double periodSamples = samplesPerBeat * ratePPQ;
        lfoPhases[index] += (sampleDelta / periodSamples); if (lfoPhases[index] >= 1.0) lfoPhases[index] -= 1.0;
        float sineVal = static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi));
        return juce::jlimit (minVal, maxVal, baseVal + (sineVal * depth * ((maxVal - minVal) * 0.5f)));
    };

    activeMorph   = applyPresetLfo (0, baseMorph,   IDs::rhythmMorphLfoPreset, 0.0f, 1.0f);
    activeRest    = applyPresetLfo (1, baseRest,    IDs::restLfoPreset,        0.0f, 1.0f);
    activeLegato  = applyPresetLfo (2, baseLegato,  IDs::legatoLfoPreset,      0.1f, 1.0f);
    modLegato = activeLegato; modRest = (activeLegato >= 0.8f) ? (activeRest * juce::jlimit (0.0f, 1.0f, (1.0f - activeLegato) / 0.2f)) : activeRest;

    float rawRate = applyPresetLfo (3, baseRate, IDs::rateLfoPreset, 0.0f, 3.0f);
    activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate)));
    activeEntropy = applyPresetLfo (4, baseEntropy, IDs::entropyLfoPreset, -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyPresetLfo (5, baseHarmony, IDs::harmonyLfoPreset, 0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos   = applyPresetLfo (6, baseChaos,   IDs::chaosLfoPreset,   0.0f, 1.0f); modChaos = activeChaos;

    float rawOctaves = applyPresetLfo (7, baseOctaves, IDs::octavesLfoPreset, -3.0f, 3.0f);
    activeOctavesVal = juce::jlimit (-3, 3, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear(); bool isPlaying = false; double bpm = 120.0; mSongPositionPPQ = 0.0;
#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead()) { if (auto pos = playhead->getPosition()) { isPlaying = pos->getIsPlaying(); auto bpmOpt = pos->getBpm(); if (bpmOpt.hasValue()) bpm = *bpmOpt; auto ppqOpt = pos->getPpqPosition(); if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt; } }
#else
    if (auto* playhead = getPlayHead()) { juce::AudioPlayHead::CurrentPositionInfo info; if (playhead->getCurrentPositionInfo (info)) { isPlaying = info.isPlaying; bpm = info.bpm; mSongPositionPPQ = info.ppqPosition; } }
#endif
    int numSamples = buffer.getNumSamples(); updateLfoModulations (numSamples, bpm);
    float slewFactor = static_cast<float>(1.0 - std::exp (-1.0 / (0.15 * mSampleRate)));
    for (int i = 0; i < 24; ++i) currentSlewValue[i] += (currentSlewTarget[i] - currentSlewValue[i]) * slewFactor;

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f, isArpActive = *apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f;
    bool isPolyActive = *apvts.getRawParameterValue (IDs::poly.getParamID()) > 0.5f, isFreezeActive = *apvts.getRawParameterValue (IDs::freeze.getParamID()) > 0.5f;

    // Snapshot Freeze Caching (Saves complete active state)
    if (isFreezeActive && !lastFreezeState) {
        lastFreezeState = true;
        frozenActiveHeldNotes = activeHeldNotes;
        frozenLatchedNotes = latchedNotes;
        frozenMorph = activeMorph;
        frozenRest = modRest;
        frozenLegato = modLegato;
        frozenEntropy = modEntropy;
        frozenHarmony = modHarmony;
        frozenChaos = modChaos;
        frozenRateIdx = activeRateIdx;
        frozenOctavesVal = activeOctavesVal;
        float mVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
        for (int i = 0; i < 8; ++i) frozenFaders[i] = (sceneA.faders[i] * (1.0f - mVal)) + (sceneB.faders[i] * mVal);
    } else if (!isFreezeActive && lastFreezeState) {
        lastFreezeState = false;
    }

    if (!isLatchActive) { latchedNotes.clear(); isFirstNoteOfNewChord = true; }
    
    float morphedFaders[8];
    float mVal = *apvts.getRawParameterValue (IDs::morph.getParamID());
    for (int i = 0; i < 8; ++i) morphedFaders[i] = (sceneA.faders[i] * (1.0f - mVal)) + (sceneB.faders[i] * mVal);

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) { processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), juce::jlimit (0, numSamples - 1, it->second + numSamples)); it = scheduledNoteOffs.erase(it); }
        else ++it;
    }

    if (isLatchActive && latchedNotes.empty() && !activeHeldNotes.empty()) { latchedNotes = activeHeldNotes; isFirstNoteOfNewChord = false; }

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn() && !isFreezeActive) { 
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
    
    std::vector<int> notesToPlay = isFreezeActive ? (isLatchActive ? frozenLatchedNotes : frozenActiveHeldNotes) : (isLatchActive ? latchedNotes : activeHeldNotes);
    isCurrentlyPlayingUI.store (!notesToPlay.empty() || isFreezeActive);

    if (!notesToPlay.empty() || isFreezeActive) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        int currentRate = isFreezeActive ? frozenRateIdx : activeRateIdx;
        double stepLengthPPQ = (currentRate == 0) ? 1.0 : (currentRate == 1) ? 0.5 : (currentRate == 2) ? 0.25 : 0.125, stepSamples = samplesPerBeat * stepLengthPPQ;
        float currentMorph = isFreezeActive ? frozenMorph : activeMorph;
        double swingOffset = 0.08 * currentMorph * stepLengthPPQ, triggerThreshold = stepLengthPPQ;
        if (currentStep % 2 == 1) triggerThreshold += swingOffset;

        if (isPlaying) {
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8;
            if (stepIndex != mLastStep) { mLastStep = stepIndex; currentStep = stepIndex; stepTriggered = true; }
        } else {
            double activeStepSamples = stepSamples;
            if (currentStep % 2 == 0) activeStepSamples *= (1.0 + (swingOffset * 4.0)); else activeStepSamples *= (1.0 - (swingOffset * 4.0));
            mTimeInSamples += numSamples; if (mTimeInSamples >= activeStepSamples) { mTimeInSamples = 0; stepTriggered = true; }
        }

        if (stepTriggered) {
            float currentEntropy = isFreezeActive ? frozenEntropy : modEntropy;
            int playDirection = 0; 
            if (currentEntropy >= -0.1f && currentEntropy <= 0.1f) playDirection = 0;
            else if (currentEntropy > 0.1f && currentEntropy <= 0.5f) playDirection = 1;
            else if (currentEntropy > 0.5f) playDirection = 2;
            else if (currentEntropy < -0.1f && currentEntropy >= -0.5f) playDirection = 3;
            else if (currentEntropy < -0.5f) playDirection = 4;

            static bool goingForward = true;
            if (playDirection == 1) { 
                if (goingForward) { currentStep++; if (currentStep >= 7) { currentStep = 7; goingForward = false; } }
                else { currentStep--; if (currentStep <= 0) { currentStep = 0; goingForward = true; } }
            } else if (playDirection == 2) { 
                float rVal = juce::Random::getSystemRandom().nextFloat();
                if (rVal < 0.7f) currentStep = (currentStep + 1) % 8; else if (rVal < 0.9f) currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 3) { 
                currentStep = (currentStep - 1 + 8) % 8;
            } else if (playDirection == 4) { 
                currentStep = (juce::Random::getSystemRandom().nextFloat() < 0.2f) ? (currentStep + 2) % 8 : (currentStep + 1) % 8;
            } else { 
                currentStep = isPlaying ? (static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8) : ((currentStep + 1) % 8);
            }
            mLastStep = currentStep;

            float faderProb = isFreezeActive ? frozenFaders[currentStep] : morphedFaders[currentStep];
            float currentRest = isFreezeActive ? frozenRest : modRest;
            if (juce::Random::getSystemRandom().nextFloat() <= faderProb && !(juce::Random::getSystemRandom().nextFloat() <= currentRest)) {
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

                int rawPitch = 60, octaveBase = 60;
                if (isArpActive && !notesToPlay.empty()) { rawPitch = notesToPlay[currentStep % notesToPlay.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }
                else { rawPitch = 48 + rootKeyIdx + scaleOffsets[currentStep % scaleOffsets.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }

                std::vector<int> pitchList; pitchList.push_back (rawPitch);
                float currentHarmony = isFreezeActive ? frozenHarmony : modHarmony;
                if (isPolyActive) {
                    int maxAllowedNotes = (currentHarmony > 0.25f && currentHarmony < 0.5f) ? 2 : (currentHarmony >= 0.5f && currentHarmony < 0.75f) ? 3 : (currentHarmony >= 0.75f) ? 4 : 1;
                    if (maxAllowedNotes > 1) { for (int n = 1; n < maxAllowedNotes; ++n) pitchList.push_back (octaveBase + scaleOffsets[(currentStep + n * 2) % scaleOffsets.size()]); }
                }

                int midiChVal = juce::jlimit (1, 16, static_cast<int> (*apvts.getRawParameterValue (IDs::midiChannel.getParamID())) + 1);

                for (auto& pitch : pitchList) {
                    int noteOffset = (pitch - rootKeyIdx) % 12, octBase = ((pitch - rootKeyIdx) / 12) * 12 + rootKeyIdx, nearestVal = scaleOffsets[0], minDiff = 12;
                    for (int offset : scaleOffsets) { int diff = std::abs (offset - noteOffset); if (diff < minDiff) { minDiff = diff; nearestVal = offset; } }
                    pitch = octBase + nearestVal;
                }

                int rangeShift = isFreezeActive ? frozenOctavesVal : activeOctavesVal;
                int octaveShiftCount = (rangeShift > 0) ? ((currentStep / 2) % (rangeShift + 1)) : ((rangeShift < 0) ? -((currentStep / 2) % (std::abs(rangeShift) + 1)) : 0);
                float currentChaos = isFreezeActive ? frozenChaos : modChaos;
                float currentLegato = isFreezeActive ? frozenLegato : modLegato;
                for (auto pitch : pitchList) {
                    int targetPitch = juce::jlimit(0, 127, pitch + (octaveShiftCount * 12) + ((currentChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= currentChaos) ? (juce::Random::getSystemRandom().nextBool() ? 12 : -12) : 0));
                    
                    // Direct midiChannel routing mapping implemented
                    processedMidi.addEvent (juce::MidiMessage::noteOn (midiChVal, targetPitch, static_cast<juce::uint8>(100)), 0);
                    mLastNotePlayed = targetPitch; mNoteOffTime = static_cast<int>(stepSamples * currentLegato); scheduleNoteOff (processedMidi, targetPitch, mNoteOffTime);
                }
            }
        }
    } else { if (mLastStep != -1) { if (mLastNotePlayed != -1) { int midiChVal = juce::jlimit (1, 16, static_cast<int> (*apvts.getRawParameterValue (IDs::midiChannel.getParamID())) + 1); processedMidi.addEvent (juce::MidiMessage::noteOff (midiChVal, mLastNotePlayed), 0); mLastNotePlayed = -1; } mLastStep = -1; } currentStep = 0; }
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
    int baseRoot = 48 + rootIdx, r = getScalePitch (padIndex), t = getScalePitch (padIndex + 2), f = getScalePitch (padIndex + 4);

    std::vector<int> newChord;
    if (activeHarmony >= 0.34f && activeHarmony < 0.67f) { t = getScalePitch (padIndex + 3); newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }
    else if (activeHarmony >= 0.67f) { int s = getScalePitch (padIndex + 6); newChord = { baseRoot + r, baseRoot + t, baseRoot + f, baseRoot + s }; }
    else { newChord = { baseRoot + r, baseRoot + t, baseRoot + f }; }

    if (!lastChordPitches.empty() && newChord.size() == lastChordPitches.size()) {
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
        sceneAPresets[slotIndex] = sceneA; sceneBPresets[slotIndex] = sceneB;
        sceneASlotsSaved[slotIndex] = hasSceneA; sceneBSlotsSaved[slotIndex] = hasSceneB;

        for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); 
        presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); 
        presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); 
        presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); 
        presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); 
        presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); 
        presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); 
        presets[slotIndex].rate = *apvts.getRawParameterValue (IDs::rate.getParamID());
        presets[slotIndex].octaves = static_cast<float> (*apvts.getRawParameterValue (IDs::octaves.getParamID()));
        
        juce::ParameterID lfoIds[] = { IDs::rhythmMorphLfoPreset, IDs::restLfoPreset, IDs::legatoLfoPreset, IDs::rateLfoPreset, IDs::entropyLfoPreset, IDs::harmonyLfoPreset, IDs::chaosLfoPreset, IDs::octavesLfoPreset };
        for (int i = 0; i < 8; ++i) { 
            presets[slotIndex].lfoPresets[i] = static_cast<int> (*apvts.getRawParameterValue (lfoIds[i].getParamID())); 
        }
        presetSlotsSaved[slotIndex] = true; activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::loadPreset (int slotIndex) 
{ 
    if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex]) 
    { 
        sceneA = sceneAPresets[slotIndex]; sceneB = sceneBPresets[slotIndex];
        hasSceneA = sceneASlotsSaved[slotIndex]; hasSceneB = sceneBSlotsSaved[slotIndex];

        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph); 
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest); 
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato); 
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((presets[slotIndex].entropy + 1.0f) * 0.5f); 
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony); 
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos); 
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (presets[slotIndex].rate / 3.0f);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((presets[slotIndex].octaves + 3.0f) / 6.0f); 

        juce::ParameterID lfoIds[] = { IDs::rhythmMorphLfoPreset, IDs::restLfoPreset, IDs::legatoLfoPreset, IDs::rateLfoPreset, IDs::entropyLfoPreset, IDs::harmonyLfoPreset, IDs::chaosLfoPreset, IDs::octavesLfoPreset };
        for (int i = 0; i < 8; ++i) { 
            apvts.getParameter (lfoIds[i].getParamID())->setValueNotifyingHost (static_cast<float>(presets[slotIndex].lfoPresets[i]) / 4.0f); 
        }
        for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]); 
        activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::captureScene (int side) 
{ 
    SceneState& s = (side == 0) ? sceneA : sceneB;
    for (int i = 0; i < 8; ++i) s.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1))); 
    s.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID()); s.rest = *apvts.getRawParameterValue (IDs::rest.getParamID()); s.legato = *apvts.getRawParameterValue (IDs::legato.getParamID()); 
    s.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); s.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID()); s.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID()); 
    s.rate = *apvts.getRawParameterValue (IDs::rate.getParamID()); s.octaves = static_cast<float> (*apvts.getRawParameterValue (IDs::octaves.getParamID()));
    
    juce::ParameterID lfoIds[] = { IDs::rhythmMorphLfoPreset, IDs::restLfoPreset, IDs::legatoLfoPreset, IDs::rateLfoPreset, IDs::entropyLfoPreset, IDs::harmonyLfoPreset, IDs::chaosLfoPreset, IDs::octavesLfoPreset };
    for (int i = 0; i < 8; ++i) { s.lfoPresets[i] = static_cast<int> (*apvts.getRawParameterValue (lfoIds[i].getParamID())); }
    if (side == 0) hasSceneA = true; else hasSceneB = true; 
}

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (1.0f); }
void PluginProcessor::diceMelody() { auto* r = &juce::Random::getSystemRandom(); for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (r->nextFloat()); }
void PluginProcessor::diceArticulation() { auto* r = &juce::Random::getSystemRandom(); apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (r->nextFloat() * 0.5f); apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.1f + r->nextFloat() * 0.9f); }
void PluginProcessor::diceTime() { auto* r = &juce::Random::getSystemRandom(); apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(4)) / 3.0f); apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(7)) / 6.0f); apvts.getParameter (IDs::cycleLength.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(4)) / 3.0f); }
void PluginProcessor::diceNavy() { auto* r = &juce::Random::getSystemRandom(); apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (r->nextFloat()); apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (r->nextFloat()); apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (r->nextFloat()); apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (r->nextFloat()); }

void PluginProcessor::diceActiveSceneA()
{
    auto* random = &juce::Random::getSystemRandom(); for (int i = 0; i < 8; ++i) sceneA.faders[i] = random->nextFloat();
    sceneA.rhythmMorph = random->nextFloat(); sceneA.rest = random->nextFloat() * 0.5f; sceneA.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneA.entropy = -1.0f + random->nextFloat() * 2.0f; sceneA.harmony = random->nextFloat(); sceneA.chaos = random->nextFloat();
    sceneA.rate = static_cast<float> (random->nextInt (4)); sceneA.octaves = static_cast<float> (random->nextInt (7) - 3); hasSceneA = true;
}

void PluginProcessor::diceActiveSceneB()
{
    auto* random = &juce::Random::getSystemRandom(); for (int i = 0; i < 8; ++i) sceneB.faders[i] = random->nextFloat();
    sceneB.rhythmMorph = random->nextFloat(); sceneB.rest = random->nextFloat() * 0.5f; sceneB.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneB.entropy = -1.0f + random->nextFloat() * 2.0f; sceneB.harmony = random->nextFloat(); sceneB.chaos = random->nextFloat();
    sceneB.rate = static_cast<float> (random->nextInt (4)); sceneB.octaves = static_cast<float> (random->nextInt (7) - 3); hasSceneB = true;
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml());
    auto* presetsNodeA = xml->createNewChildElement ("SCENE_A_PRESETS"), *presetsNodeB = xml->createNewChildElement ("SCENE_B_PRESETS");
    for (int i = 0; i < 8; ++i) {
        auto* childA = presetsNodeA->createNewChildElement ("SLOT_" + juce::String (i)); childA->setAttribute ("saved", sceneASlotsSaved[i]);
        if (sceneASlotsSaved[i]) {
            childA->setAttribute ("morph", sceneAPresets[i].rhythmMorph); childA->setAttribute ("rest", sceneAPresets[i].rest); childA->setAttribute ("legato", sceneAPresets[i].legato);
            childA->setAttribute ("rate", sceneAPresets[i].rate); childA->setAttribute ("entropy", sceneAPresets[i].entropy); childA->setAttribute ("harmony", sceneAPresets[i].harmony);
            childA->setAttribute ("chaos", sceneAPresets[i].chaos); childA->setAttribute ("octaves", sceneAPresets[i].octaves);
            for (int f = 0; f < 8; ++f) childA->setAttribute ("fader_" + juce::String (f), sceneAPresets[i].faders[f]);
            for (int l = 0; l < 8; ++l) { childA->setAttribute ("lfo_r_" + juce::String (l), sceneAPresets[i].lfoPresets[l]); }
        }
        auto* childB = presetsNodeB->createNewChildElement ("SLOT_" + juce::String (i)); childB->setAttribute ("saved", sceneBSlotsSaved[i]);
        if (sceneBSlotsSaved[i]) {
            childB->setAttribute ("morph", sceneBPresets[i].rhythmMorph); childB->setAttribute ("rest", sceneBPresets[i].rest); childB->setAttribute ("legato", sceneBPresets[i].legato);
            childB->setAttribute ("rate", sceneBPresets[i].rate); childB->setAttribute ("entropy", sceneBPresets[i].entropy); childB->setAttribute ("harmony", sceneBPresets[i].harmony);
            childB->setAttribute ("chaos", sceneBPresets[i].chaos); childB->setAttribute ("octaves", sceneBPresets[i].octaves);
            for (int f = 0; f < 8; ++f) childB->setAttribute ("fader_" + juce::String (f), sceneBPresets[i].faders[f]);
            for (int l = 0; l < 8; ++l) { childB->setAttribute ("lfo_r_" + juce::String (l), sceneBPresets[i].lfoPresets[l]); }
        }
    }
    auto* banksNode = xml->createNewChildElement ("GLOBAL_BANKS");
    for (int i = 0; i < 8; ++i) {
        auto* childBank = banksNode->createNewChildElement ("BANK_" + juce::String (i)); childBank->setAttribute ("saved", presetSlotsSaved[i]);
        if (presetSlotsSaved[i]) {
            childBank->setAttribute ("morph", presets[i].rhythmMorph); childBank->setAttribute ("rest", presets[i].rest); childBank->setAttribute ("legato", presets[i].legato);
            childBank->setAttribute ("rate", presets[i].rate); childBank->setAttribute ("entropy", presets[i].entropy); childBank->setAttribute ("harmony", presets[i].harmony);
            childBank->setAttribute ("chaos", presets[i].chaos); childBank->setAttribute ("octaves", presets[i].octaves);
            for (int f = 0; f < 8; ++f) childBank->setAttribute ("fader_" + juce::String (f), presets[i].faders[f]);
            for (int l = 0; l < 8; ++l) { childBank->setAttribute ("lfo_r_" + juce::String (l), presets[i].lfoPresets[l]); }
        }
    }
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType())) {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        if (auto* presetsNodeA = xmlState->getChildByName ("SCENE_A_PRESETS")) {
            for (int i = 0; i < 8; ++i) {
                if (auto* childA = presetsNodeA->getChildByName ("SLOT_" + juce::String (i))) {
                    sceneASlotsSaved[i] = childA->getBoolAttribute ("saved");
                    if (sceneASlotsSaved[i]) {
                        sceneAPresets[i].rhythmMorph = static_cast<float> (childA->getDoubleAttribute ("morph")); sceneAPresets[i].rest = static_cast<float> (childA->getDoubleAttribute ("rest")); sceneAPresets[i].legato = static_cast<float> (childA->getDoubleAttribute ("legato"));
                        sceneAPresets[i].rate = static_cast<float> (childA->getDoubleAttribute ("rate")); sceneAPresets[i].entropy = static_cast<float> (childA->getDoubleAttribute ("entropy")); sceneAPresets[i].harmony = static_cast<float> (childA->getDoubleAttribute ("harmony"));
                        sceneAPresets[i].chaos = static_cast<float> (childA->getDoubleAttribute ("chaos")); sceneAPresets[i].octaves = static_cast<float> (childA->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) sceneAPresets[i].faders[f] = static_cast<float> (childA->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { sceneAPresets[i].lfoPresets[l] = childA->getIntAttribute ("lfo_r_" + juce::String (l)); }
                    }
                }
            }
        }
        if (auto* presetsNodeB = xmlState->getChildByName ("SCENE_B_PRESETS")) {
            for (int i = 0; i < 8; ++i) {
                if (auto* childB = presetsNodeB->getChildByName ("SLOT_" + juce::String (i))) {
                    sceneBSlotsSaved[i] = childB->getBoolAttribute ("saved");
                    if (sceneBSlotsSaved[i]) {
                        sceneBPresets[i].rhythmMorph = static_cast<float> (childB->getDoubleAttribute ("morph")); sceneBPresets[i].rest = static_cast<float> (childB->getDoubleAttribute ("rest")); sceneBPresets[i].legato = static_cast<float> (childB->getDoubleAttribute ("legato"));
                        sceneBPresets[i].rate = static_cast<float> (childB->getDoubleAttribute ("rate")); sceneBPresets[i].entropy = static_cast<float> (childB->getDoubleAttribute ("entropy")); sceneBPresets[i].harmony = static_cast<float> (childB->getDoubleAttribute ("harmony"));
                        sceneBPresets[i].chaos = static_cast<float> (childB->getDoubleAttribute ("chaos")); sceneBPresets[i].octaves = static_cast<float> (childB->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) sceneBPresets[i].faders[f] = static_cast<float> (childB->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { sceneBPresets[i].lfoPresets[l] = childB->getIntAttribute ("lfo_r_" + juce::String (l)); }
                    }
                }
            }
        }
        if (auto* banksNode = xmlState->getChildByName ("GLOBAL_BANKS")) {
            for (int i = 0; i < 8; ++i) {
                if (auto* childBank = banksNode->getChildByName ("BANK_" + juce::String (i))) {
                    presetSlotsSaved[i] = childBank->getBoolAttribute ("saved");
                    if (presetSlotsSaved[i]) {
                        presets[i].rhythmMorph = static_cast<float> (childBank->getDoubleAttribute ("morph")); presets[i].rest = static_cast<float> (childBank->getDoubleAttribute ("rest")); presets[i].legato = static_cast<float> (childBank->getDoubleAttribute ("legato"));
                        presets[i].rate = static_cast<float> (childBank->getDoubleAttribute ("rate")); presets[i].entropy = static_cast<float> (childBank->getDoubleAttribute ("entropy")); presets[i].harmony = static_cast<float> (childBank->getDoubleAttribute ("harmony"));
                        presets[i].chaos = static_cast<float> (childBank->getDoubleAttribute ("chaos")); presets[i].octaves = static_cast<float> (childBank->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) presets[i].faders[f] = static_cast<float> (childBank->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { presets[i].lfoPresets[l] = childBank->getIntAttribute ("lfo_r_" + juce::String (l)); }
                    }
                }
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int i = 1; i <= 8; ++i) params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("fader" + juce::String (i), 1), "Fader " + juce::String (i), 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rhythmMorph, "Rhythm Morph", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rest, "Rest", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::legato, "Legato", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", -1.0f, 1.0f, 0.0f)); 
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::arpSeq, "SEQ/ARP Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::poly, "Poly Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::freeze, "Freeze Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); 
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rate, "Rate", juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2)); 
    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", -3, 3, 0)); 
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::panelTheme, "Panel Theme", juce::StringArray { "Navy Cyber", "Skyline Eurorack", "Monochrome Minimal", "Matrix Terminal" }, 1));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::midiChannel, "Midi Channel", juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 0));

    auto regLfo = [&](juce::ParameterID rId, juce::String nm) {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (rId, nm + " LFO Vibe", juce::StringArray { "Off", "Drift", "Bounce", "Vibe", "Tremor" }, 0));
    };
    regLfo (IDs::rhythmMorphLfoPreset, "Morph"); regLfo (IDs::restLfoPreset, "Rest"); regLfo (IDs::legatoLfoPreset, "Legato"); regLfo (IDs::rateLfoPreset, "Rate");
    regLfo (IDs::entropyLfoPreset, "Entropy"); regLfo (IDs::harmonyLfoPreset, "Harmony"); regLfo (IDs::chaosLfoPreset, "Chaos"); regLfo (IDs::octavesLfoPreset, "Octaves");

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }