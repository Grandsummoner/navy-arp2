#include "PluginProcessor.h"
#include "PluginEditor.h"

// Symmetrical Scene MIDI Routing Caches [3]
static int sceneMidiInChannels[2] { 0, 0 };      // Default Omni for both
static int sceneMidiOutChannels[2] { 1, 2 };     // Default Scene A = Ch 1, Scene B = Ch 2

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                      ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState(); sceneB = SceneState(); lastChordPitches = { 60, 64, 67 }; 
    for (int i = 0; i < 8; ++i) { sceneAPresets[i] = SceneState(); sceneBPresets[i] = SceneState(); }

    // Cache APVTS Parameter pointers to avoid real-time string lookups/hashing in processBlock [43]
    for (int i = 0; i < 8; ++i)
        faderPtrs[i] = apvts.getRawParameterValue ("fader" + juce::String (i + 1));
        
    rhythmMorphPtr = apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    restPtr        = apvts.getRawParameterValue (IDs::rest.getParamID());
    legatoPtr      = apvts.getRawParameterValue (IDs::legato.getParamID());
    entropyPtr     = apvts.getRawParameterValue (IDs::entropy.getParamID());
    harmonyPtr     = apvts.getRawParameterValue (IDs::harmony.getParamID());
    chaosPtr       = apvts.getRawParameterValue (IDs::chaos.getParamID());
    morphPtr       = apvts.getRawParameterValue (IDs::morph.getParamID());
    latchPtr       = apvts.getRawParameterValue (IDs::latch.getParamID());
    arpSeqPtr      = apvts.getRawParameterValue (IDs::arpSeq.getParamID());
    polyPtr        = apvts.getRawParameterValue (IDs::poly.getParamID());
    freezePtr      = apvts.getRawParameterValue (IDs::freeze.getParamID());
    rootKeyPtr     = apvts.getRawParameterValue (IDs::rootKey.getParamID());
    scaleTypePtr   = apvts.getRawParameterValue (IDs::scaleType.getParamID());
    cycleLengthPtr = apvts.getRawParameterValue (IDs::cycleLength.getParamID());
    ratePtr        = apvts.getRawParameterValue (IDs::rate.getParamID());
    octavesPtr     = apvts.getRawParameterValue (IDs::octaves.getParamID());

    // Cache master raw pointers [43]
    masterVelocityPtr = apvts.getRawParameterValue (IDs::masterVelocity.getParamID());
    masterSwingPtr    = apvts.getRawParameterValue (IDs::masterSwing.getParamID());

    // Cache Left Panel Sound parameters [3]
    midiInChannelPtr  = apvts.getRawParameterValue (IDs::midiInChannel.getParamID());
    midiOutChannelPtr = apvts.getRawParameterValue (IDs::midiOutChannel.getParamID());
    voice1SynthPtr    = apvts.getRawParameterValue (IDs::voice1Synth.getParamID());
    voice1DecayPtr    = apvts.getRawParameterValue (IDs::voice1Decay.getParamID());
    voice1TimbrePtr   = apvts.getRawParameterValue (IDs::voice1Timbre.getParamID());
    voice1ReverbPtr   = apvts.getRawParameterValue (IDs::voice1Reverb.getParamID());
    voice2SynthPtr    = apvts.getRawParameterValue (IDs::voice2Synth.getParamID());
    voice2DecayPtr    = apvts.getRawParameterValue (IDs::voice2Decay.getParamID());
    voice2TimbrePtr   = apvts.getRawParameterValue (IDs::voice2Timbre.getParamID());
    voice2ReverbPtr   = apvts.getRawParameterValue (IDs::voice2Reverb.getParamID());
    audioRoutingPtr   = apvts.getRawParameterValue (IDs::audioRouting.getParamID());

    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
    for (int i = 0; i < 8; ++i) {
        lfoRatePtrs[i]  = apvts.getRawParameterValue (rates[i].getParamID());
        lfoDepthPtrs[i] = apvts.getRawParameterValue (depths[i].getParamID());
    }

    // Initialize standalone MIDI CC mappings to unmapped state
    for (int i = 0; i < 18; ++i)
        midiCcMappings[i].store (-1);
}

PluginProcessor::~PluginProcessor() {}
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate; mLastStep = -1; mLastNotePlayed = -1; mNoteOffTime = 0; mTimeInSamples = 0;
    activeHeldNotes.clear(); latchedNotes.clear(); scheduledNoteOffs.clear();
    std::fill (std::begin (lfoPhases), std::end (lfoPhases), 0.0);
    isFirstNoteOfNewChord = true; lastSceneBActiveState = isSceneBActiveAnchor.load();
    std::fill (std::begin (currentSlewValue), std::end (currentSlewValue), 0.5f);
    std::fill (std::begin (currentSlewTarget), std::end (currentSlewTarget), 0.5f);

    // Set sample rate on polyphonic synthesis voices [3]
    voice1.sampleRate = sampleRate;
    voice2.sampleRate = sampleRate;

    // Clear wet reverb feedback delay lines
    std::fill (std::begin (delayBufferL), std::end (delayBufferL), 0.0f);
    std::fill (std::begin (delayBufferR), std::end (delayBufferR), 0.0f);
    delayWriteIdx = 0;

    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    // Retrieve correct MIDI out channel configured for active step note
    int outChA = sceneMidiOutChannels[0];
    int outChB = sceneMidiOutChannels[1];
    float morphVal = morphPtr->load();
    int activeMidiCh = (morphVal >= 0.5f) ? outChB : outChA;

    if (delaySamples <= 0) midi.addEvent (juce::MidiMessage::noteOff (activeMidiCh, pitch), 0);
    else scheduledNoteOffs.push_back ({ pitch, delaySamples });
}

void PluginProcessor::setActiveAnchor (bool useSceneB)
{
    if (isSceneBActiveAnchor.load() == useSceneB) return;
    captureScene (isSceneBActiveAnchor.load() ? 1 : 0);
    
    // Cache the MIDI In/Out settings of the active scene we are leaving
    int leavingIndex = useSceneB ? 0 : 1;
    sceneMidiInChannels[leavingIndex]  = static_cast<int> (midiInChannelPtr->load());
    sceneMidiOutChannels[leavingIndex] = static_cast<int> (midiOutChannelPtr->load()) + 1;

    isSceneBActiveAnchor.store (useSceneB); lastSceneBActiveState = useSceneB;
    
    SceneState& targetScene = useSceneB ? sceneB : sceneA;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (targetScene.rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (targetScene.rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (targetScene.legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((targetScene.entropy + 1.0f) * 0.5f);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (targetScene.harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (targetScene.chaos);
    apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate);
    apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);

    // Restore the MIDI In/Out settings of the active scene we are entering
    int enteringIndex = useSceneB ? 1 : 0;
    apvts.getParameter (IDs::midiInChannel.getParamID())->setValueNotifyingHost (static_cast<float> (sceneMidiInChannels[enteringIndex]) / 16.0f);
    apvts.getParameter (IDs::midiOutChannel.getParamID())->setValueNotifyingHost (static_cast<float> (sceneMidiOutChannels[enteringIndex] - 1) / 15.0f);
}

void PluginProcessor::captureActiveParametersToActiveScene()
{
    // Handled directly and safely in the editor's timerCallback 
    // during active drag states to prevent feedback loops.
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
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (targetScene.rate);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((targetScene.octaves + 3.0f) / 6.0f);
        for (int i = 0; i < 8; ++i)
            apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetScene.faders[i]);
    }

    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0)), sampleDelta = numSamples;
    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (cycleLengthPtr->load()));
    currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % ((cycleIndex == 0) ? 1 : (cycleIndex == 1) ? 2 : (cycleIndex == 2) ? 4 : 8)) + 1;

    float morphVal = morphPtr->load();
    auto morphValue = [&](float valA, float valB) -> float { return (valA * (1.0f - morphVal)) + (valB * morphVal); };

    float baseMorph = morphValue (sceneA.rhythmMorph, sceneB.rhythmMorph), baseRest = morphValue (sceneA.rest, sceneB.rest), baseLegato = morphValue (sceneA.legato, sceneB.legato), baseRate = morphValue (sceneA.rate, sceneB.rate);
    float baseEntropy = morphValue (sceneA.entropy, sceneB.entropy), baseHarmony = morphValue (sceneA.harmony, sceneB.harmony), baseChaos = morphValue (sceneA.chaos, sceneB.chaos), baseOctaves = morphValue (sceneA.octaves, sceneB.octaves);

    auto applyLfo = [&](int index, float baseVal, std::atomic<float>* rPtr, std::atomic<float>* dPtr, float minVal, float maxVal) -> float {
        int rateChoice = static_cast<int> (rPtr->load()); float depth = dPtr->load();
        if (rateChoice == 0) return baseVal;
        double divPPQ = (rateChoice == 1) ? 1.0 : (rateChoice == 2) ? 0.5 : (rateChoice == 3) ? 0.25 : 0.125, periodSamples = samplesPerBeat * divPPQ;
        lfoPhases[index] += (sampleDelta / periodSamples); 
        if (lfoPhases[index] >= 1.0) lfoPhases[index] = std::fmod (lfoPhases[index], 1.0); 
        return juce::jlimit (minVal, maxVal, baseVal + (static_cast<float> (std::sin (lfoPhases[index] * juce::MathConstants<double>::twoPi)) * depth * ((maxVal - minVal) * 0.5f)));
    };

    activeMorph = applyLfo (0, baseMorph, lfoRatePtrs[0], lfoDepthPtrs[0], 0.0f, 1.0f);
    activeRest = applyLfo (1, baseRest, lfoRatePtrs[1], lfoDepthPtrs[1], 0.0f, 1.0f);
    activeLegato = applyLfo (2, baseLegato, lfoRatePtrs[2], lfoDepthPtrs[2], 0.1f, 1.0f);
    modLegato = activeLegato; modRest = (activeLegato >= 0.8f) ? (activeRest * juce::jlimit (0.0f, 1.0f, (1.0f - activeLegato) / 0.2f)) : activeRest;

    // Rate mapped continuously based on Sync Mode status
    float rawRate = applyLfo (3, baseRate, lfoRatePtrs[3], lfoDepthPtrs[3], 0.0f, 1.0f);
    
    auto* syncPtr = apvts.getRawParameterValue ("sync");
    bool syncActive = (syncPtr != nullptr && syncPtr->load() > 0.5f);
    if (syncActive) {
        activeRateIdx = juce::jlimit (0, 3, static_cast<int> (std::round (rawRate * 3.0f)));
    } else {
        activeRateIdx = 2; // Fixed to 1/16 trigger grid in free-run internal mode
    }

    activeEntropy = applyLfo (4, baseEntropy, lfoRatePtrs[4], lfoDepthPtrs[4], -1.0f, 1.0f); modEntropy = activeEntropy;
    activeHarmony = applyLfo (5, baseHarmony, lfoRatePtrs[5], lfoDepthPtrs[5], 0.0f, 1.0f); modHarmony = activeHarmony;
    activeChaos = applyLfo (6, baseChaos, lfoRatePtrs[6], lfoDepthPtrs[6], 0.0f, 1.0f); modChaos = activeChaos;

    float rawOctaves = applyLfo (7, baseOctaves, lfoRatePtrs[7], lfoDepthPtrs[7], -3.0f, 3.0f);
    activeOctavesVal = juce::jlimit (-3, 3, static_cast<int> (std::round (rawOctaves)));
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // =====================================================================
    // 1. STANDALONE MIDI LEARN & CC ROUTING INTERCEPTOR [3]
    // =====================================================================
    juce::MidiBuffer filteredMidiIn;
    int inChChoice = static_cast<int> (midiInChannelPtr->load()); // 0 = Omni, 1-16

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        
        if (msg.isController())
        {
            int ccNumber = msg.getControllerNumber();
            int ccValue  = msg.getControllerValue();
            
            // Case A: Parameter MIDI Learn state is currently active
            int learnIndex = activeMidiLearnIndex.load();
            if (learnIndex >= 0 && learnIndex < 18)
            {
                midiCcMappings[learnIndex].store (ccNumber);
                activeMidiLearnIndex.store (-1); // Clear active learning state
            }
            // Case B: MIDI CC Mapping Trigger checks
            else
            {
                for (int i = 0; i < 18; ++i)
                {
                    if (midiCcMappings[i].load() == ccNumber)
                    {
                        float normVal = static_cast<float> (ccValue) / 127.0f;
                        
                        // Map physical index throw directly to targeted APVTS parameters
                        if (i >= 0 && i < 8) // Faders 1-8
                            apvts.getParameter ("fader" + juce::String (i + 1))->setValueNotifyingHost (normVal);
                        else if (i == 8)  apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 9)  apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 10) apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 11) apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 12) apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((normVal * 2.0f) - 1.0f);
                        else if (i == 13) apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 14) apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 15) apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((normVal * 6.0f) - 3.0f);
                        else if (i == 16) apvts.getParameter (IDs::masterVelocity.getParamID())->setValueNotifyingHost (normVal);
                        else if (i == 17) apvts.getParameter (IDs::masterSwing.getParamID())->setValueNotifyingHost (normVal);
                    }
                }
            }
        }
        
        // Filter note inputs based on standalone MIDI In Channel configuration
        if (msg.isNoteOn() || msg.isNoteOff())
        {
            int msgChannel = msg.getChannel();
            if (inChChoice == 0 || msgChannel == inChChoice)
            {
                filteredMidiIn.addEvent (msg, metadata.samplePosition);
            }
        }
        else
        {
            // Always pass through non-note MIDI messages safely
            filteredMidiIn.addEvent (msg, metadata.samplePosition);
        }
    }
    midiMessages.swapWith (filteredMidiIn);

    // =====================================================================
    // 2. STANDARD PRESETS LOADING & TIMING BLOCKS
    // =====================================================================
    int presetToLoad = pendingPresetToLoad.exchange (-1);
    if (presetToLoad >= 0 && presetToLoad < 8)
    {
        sceneA = sceneAPresets[presetToLoad];
        sceneB = sceneBPresets[presetToLoad];
        hasSceneA = sceneASlotsSaved[presetToLoad];
        hasSceneB = sceneBSlotsSaved[presetToLoad];
    }

    buffer.clear(); bool isPlaying = false; double hostBpm = 120.0; mSongPositionPPQ = 0.0;
#if JUCE_MAJOR_VERSION >= 7
    if (auto* playhead = getPlayHead()) { if (auto pos = playhead->getPosition()) { isPlaying = pos->getIsPlaying(); auto bpmOpt = pos->getBpm(); if (bpmOpt.hasValue()) hostBpm = *bpmOpt; auto ppqOpt = pos->getPpqPosition(); if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt; } }
#else
    if (auto* playhead = getPlayHead()) { juce::AudioPlayHead::CurrentPositionInfo info; if (playhead->getCurrentPositionInfo (info)) { isPlaying = info.isPlaying; hostBpm = info.bpm; mSongPositionPPQ = info.ppqPosition; } }
#endif
    int numSamples = buffer.getNumSamples(); 

    // Synchronize LFO and sequencer clock based on Sync Mode toggle [1.2.3]
    auto* syncPtr = apvts.getRawParameterValue ("sync");
    bool syncActive = (syncPtr != nullptr && syncPtr->load() > 0.5f);
    
    double activeBpm = 120.0;
    if (syncActive) {
        activeBpm = hostBpm;
    } else {
        // Map continuous Float rate parameter (0.0 to 1.0) to smooth free-run BPM: 40 to 240 [1.2.3]
        activeBpm = 40.0 + (ratePtr->load() * 200.0); 
    }

    updateLfoModulations (numSamples, activeBpm);

    float slewFactor = static_cast<float>(1.0 - std::exp (-1.0 / (0.15 * mSampleRate)));
    for (int i = 0; i < 24; ++i) currentSlewValue[i] += (currentSlewTarget[i] - currentSlewValue[i]) * slewFactor;

    bool isLatchActive = latchPtr->load() > 0.5f;
    bool isArpActive = arpSeqPtr->load() > 0.5f;
    bool isPolyActive = polyPtr->load() > 0.5f;
    bool isFreezeActive = freezePtr->load() > 0.5f;

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
        
        float morphVal = morphPtr->load();
        for (int i = 0; i < 8; ++i) {
            frozenFaders[i] = (sceneA.faders[i] * (1.0f - morphVal)) + (sceneB.faders[i] * morphVal);
        }
    } else if (!isFreezeActive && lastFreezeState) {
        lastFreezeState = false;
    }

    // Process and count down note-off events
    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();) {
        it->second -= numSamples;
        if (it->second <= 0) {
            int outChA = sceneMidiOutChannels[0];
            int outChB = sceneMidiOutChannels[1];
            float mVal = morphPtr->load();
            int activeCh = (mVal >= 0.5f) ? outChB : outChA;

            processedMidi.addEvent (juce::MidiMessage::noteOff (activeCh, it->first), juce::jlimit (0, numSamples - 1, it->second + numSamples)); 
            it = scheduledNoteOffs.erase(it); 
        }
        else ++it;
    }

    static bool wasLatchActive = false;
    if (wasLatchActive && !isLatchActive)
    {
        int outChA = sceneMidiOutChannels[0];
        int outChB = sceneMidiOutChannels[1];
        float mVal = morphPtr->load();
        int activeCh = (mVal >= 0.5f) ? outChB : outChA;

        for (int pitch : latchedNotes)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (activeCh, pitch), 0);
        }
        latchedNotes.clear();
        isFirstNoteOfNewChord = true;
    }
    wasLatchActive = isLatchActive;

    if (!isLatchActive) { latchedNotes.clear(); isFirstNoteOfNewChord = true; }

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

    // =====================================================================
    // 3. STEP SEQUENCER CLOCK & NOTE TRIGGERING
    // =====================================================================
    if (!notesToPlay.empty() || isFreezeActive) {
        bool stepTriggered = false; double samplesPerBeat = mSampleRate * (60.0 / (activeBpm > 0 ? activeBpm : 120.0));
        
        // Synced subdivision triggers vs Free-Run grid timing
        int currentRate = isFreezeActive ? frozenRateIdx : activeRateIdx;
        double stepLengthPPQ = 0.25; // Default 1/16 notes for free-run mode
        if (syncActive) {
            stepLengthPPQ = (currentRate == 0) ? 1.0 : (currentRate == 1) ? 0.5 : (currentRate == 2) ? 0.25 : 0.125;
        }
        
        double stepSamples = samplesPerBeat * stepLengthPPQ;
        
        // Decouple Swing calculation from Morph. Controlled globally by masterSwingPtr
        float currentSwing = masterSwingPtr->load(); 
        double swingFraction = 0.45 * currentSwing; // Support up to 45% delay offset for solid groove

        if (isPlaying && syncActive) {
            double swingAmtPPQ = swingFraction * stepLengthPPQ;
            double periodLengthPPQ = 2.0 * stepLengthPPQ;
            double ppqNormalized = (mSongPositionPPQ < 0.0) ? 0.0 : mSongPositionPPQ;
            
            double positionInPeriod = std::fmod (ppqNormalized, periodLengthPPQ);
            double periodIndex = std::floor (ppqNormalized / periodLengthPPQ);
            
            int stepIndexWithinPeriod = (positionInPeriod >= (stepLengthPPQ + swingAmtPPQ)) ? 1 : 0;
            int absoluteStepIndex = static_cast<int> (periodIndex) * 2 + stepIndexWithinPeriod;
            int stepIndex = absoluteStepIndex % 8;

            if (stepIndex != mLastStep) { 
                mLastStep = stepIndex; 
                currentStep.store (stepIndex); 
                stepTriggered = true; 
            }
        } else {
            double swingAmtSamples = swingFraction * stepSamples;
            double activeStepSamples = stepSamples;
            
            if (currentStep.load() % 2 == 0) {
                activeStepSamples += swingAmtSamples;
            } else {
                activeStepSamples -= swingAmtSamples;
            }
            
            mTimeInSamples += numSamples; 
            if (mTimeInSamples >= activeStepSamples) { 
                mTimeInSamples = 0; 
                stepTriggered = true; 
            }
        }

        if (stepTriggered) {
            float currentEntropy = isFreezeActive ? frozenEntropy : modEntropy;
            int playDirection = 0; 
            if (currentEntropy >= -0.1f && currentEntropy <= 0.1f) playDirection = 0;
            else if (currentEntropy > 0.1f && currentEntropy <= 0.5f) playDirection = 1;
            else if (currentEntropy > 0.5f) playDirection = 2;
            else if (currentEntropy < -0.1f && currentEntropy >= -0.5f) playDirection = 3;
            else if (currentEntropy < -0.5f) playDirection = 4;

            int localStep = currentStep.load(); 
            static bool goingForward = true;
            if (playDirection == 1) { 
                if (goingForward) { localStep++; if (localStep >= 7) { localStep = 7; goingForward = false; } }
                else { localStep--; if (localStep <= 0) { localStep = 0; goingForward = true; } }
            } else if (playDirection == 2) { 
                float rVal = juce::Random::getSystemRandom().nextFloat();
                if (rVal < 0.7f) localStep = (localStep + 1) % 8; else if (rVal < 0.9f) localStep = (localStep - 1 + 8) % 8;
            } else if (playDirection == 3) { 
                localStep = (localStep - 1 + 8) % 8;
            } else if (playDirection == 4) { 
                localStep = (juce::Random::getSystemRandom().nextFloat() < 0.2f) ? (localStep + 2) % 8 : (localStep + 1) % 8;
            } else { 
                localStep = (isPlaying && syncActive) ? (static_cast<int> (std::floor (mSongPositionPPQ / stepLengthPPQ)) % 8) : ((localStep + 1) % 8);
            }
            currentStep.store (localStep);
            mLastStep = localStep;

            float morphedFaders[8];
            float morphVal = morphPtr->load();
            for (int i = 0; i < 8; ++i) {
                morphedFaders[i] = (sceneA.faders[i] * (1.0f - morphVal)) + (sceneB.faders[i] * morphVal);
            }

            // Real-time 3-Zone Note Density (Fill) Calculation
            float rawProb = isFreezeActive ? frozenFaders[localStep] : morphedFaders[localStep];
            float density = masterVelocityPtr->load(); // Note Density parameter (0.0 to 1.0)
            float faderProb = rawProb;
            
            if (density < 0.5f)
            {
                float factor = density / 0.5f;
                faderProb = rawProb * factor;
            }
            else if (density > 0.5f)
            {
                float factor = (density - 0.5f) / 0.5f;
                faderProb = rawProb + (1.0f - rawProb) * factor;
            }

            float currentRest = isFreezeActive ? frozenRest : modRest;
            if (juce::Random::getSystemRandom().nextFloat() <= faderProb && !(juce::Random::getSystemRandom().nextFloat() <= currentRest)) {
                if (mLastNotePlayed != -1) { scheduleNoteOff (processedMidi, mLastNotePlayed, 0); mLastNotePlayed = -1; }
                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (rootKeyPtr->load()));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (scaleTypePtr->load()));
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
                if (isArpActive && !notesToPlay.empty()) { rawPitch = notesToPlay[localStep % notesToPlay.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }
                else { rawPitch = 48 + rootKeyIdx + scaleOffsets[localStep % scaleOffsets.size()]; octaveBase = ((rawPitch - rootKeyIdx) / 12) * 12 + rootKeyIdx; }

                std::vector<int> pitchList; pitchList.push_back (rawPitch);
                float currentHarmony = isFreezeActive ? frozenHarmony : modHarmony;
                if (isPolyActive) {
                    int maxAllowedNotes = (currentHarmony > 0.25f && currentHarmony < 0.5f) ? 2 : (currentHarmony >= 0.5f && currentHarmony < 0.75f) ? 3 : (currentHarmony >= 0.75f) ? 4 : 1;
                    if (maxAllowedNotes > 1) { for (int n = 1; n < maxAllowedNotes; ++n) pitchList.push_back (octaveBase + scaleOffsets[(localStep + n * 2) % scaleOffsets.size()]); }
                }

                for (auto& pitch : pitchList) {
                    int noteOffset = (pitch - rootKeyIdx) % 12, octBase = ((pitch - rootKeyIdx) / 12) * 12 + rootKeyIdx, nearestVal = scaleOffsets[0], minDiff = 12;
                    for (int offset : scaleOffsets) { int diff = std::abs (offset - noteOffset); if (diff < minDiff) { minDiff = diff; nearestVal = offset; } }
                    pitch = octBase + nearestVal;
                }

                int rangeShift = isFreezeActive ? frozenOctavesVal : activeOctavesVal;
                int octaveShiftCount = (rangeShift > 0) ? ((localStep / 2) % (rangeShift + 1)) : ((rangeShift < 0) ? -((localStep / 2) % (std::abs(rangeShift) + 1)) : 0);
                float currentChaos = isFreezeActive ? frozenChaos : modChaos;
                float currentLegato = isFreezeActive ? frozenLegato : modLegato;

                // Equal-Power Volume crossfade scales computed strictly based on morph slider value
                float morphFactor = morphPtr->load();
                float vA = std::cos (morphFactor * juce::MathConstants<float>::halfPi);
                float vB = std::sin (morphFactor * juce::MathConstants<float>::halfPi);

                for (auto pitch : pitchList) {
                    int targetPitch = juce::jlimit(0, 127, pitch + (octaveShiftCount * 12) + ((currentChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= currentChaos) ? (juce::Random::getSystemRandom().nextBool() ? 12 : -12) : 0));
                    
                    // Fetch symmetric multi-timbral output channels
                    int outChA = sceneMidiOutChannels[0];
                    int outChB = sceneMidiOutChannels[1];

                    // Strike on separate channels scaled dynamically by equal-power morph factors [3]
                    juce::uint8 velA = static_cast<juce::uint8> (juce::jlimit (0.0f, 127.0f, 100.0f * vA));
                    juce::uint8 velB = static_cast<juce::uint8> (juce::jlimit (0.0f, 127.0f, 100.0f * vB));

                    if (velA > 0) processedMidi.addEvent (juce::MidiMessage::noteOn (outChA, targetPitch, velA), 0);
                    if (velB > 0) processedMidi.addEvent (juce::MidiMessage::noteOn (outChB, targetPitch, velB), 0);

                    mLastNotePlayed = targetPitch; mNoteOffTime = static_cast<int>(stepSamples * currentLegato); scheduleNoteOff (processedMidi, targetPitch, mNoteOffTime);

                    // Real-Time Note triggers on our local dual audio synthesizers [3]
                    voice1.triggerNote (targetPitch, voice1DecayPtr->load());
                    voice2.triggerNote (targetPitch, voice2DecayPtr->load());
                }
            }
        }
    } else { if (mLastStep != -1) { if (mLastNotePlayed != -1) { scheduleNoteOff (processedMidi, mLastNotePlayed, 0); mLastNotePlayed = -1; } mLastStep = -1; } currentStep.store (0); }
    midiMessages.swapWith (processedMidi);

    // =====================================================================
    // 4. EMBEDDED REALTIME AUDIO SYNTHESIS RENDER LOOP [3]
    // =====================================================================
    float* channelL = buffer.getWritePointer (0);
    float* channelR = buffer.getWritePointer (1);

    int routeChoice = static_cast<int> (audioRoutingPtr->load());

    // Morph scaling factors
    float mVal = morphPtr->load();
    float vA = std::cos (mVal * juce::MathConstants<float>::halfPi);
    float vB = std::sin (mVal * juce::MathConstants<float>::halfPi);

    float reverbSend1 = voice1ReverbPtr->load();
    float reverbSend2 = voice2ReverbPtr->load();

    for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
    {
        // Render raw synthesis samples
        float s1 = voice1.process (static_cast<int> (voice1SynthPtr->load()), voice1TimbrePtr->load());
        float s2 = voice2.process (static_cast<int> (voice2SynthPtr->load()), voice2TimbrePtr->load());

        float drySample = 0.0f;
        float wetInput = 0.0f;

        if (routeChoice == 0) // Split A->1 / B->2
        {
            drySample = (s1 * vA) + (s2 * vB);
            wetInput  = (s1 * vA * reverbSend1) + (s2 * vB * reverbSend2);
        }
        else if (routeChoice == 1) // Layered (Voice 1)
        {
            drySample = s1;
            wetInput  = s1 * reverbSend1;
        }
        // RouteChoice 2 (External Out Only) remains 0.0f (Muted) saving CPU

        // Simple stereo-spreading delay feedback line
        int delayLenL = static_cast<int> (0.35f * mSampleRate);
        int delayLenR = static_cast<int> (0.50f * mSampleRate);

        int readIdxL = delayWriteIdx - delayLenL;
        if (readIdxL < 0) readIdxL += 44100;
        int readIdxR = delayWriteIdx - delayLenR;
        if (readIdxR < 0) readIdxR += 44100;

        float delayOutL = delayBufferL[readIdxL];
        float delayOutR = delayBufferR[readIdxR];

        delayBufferL[delayWriteIdx] = wetInput + (delayOutL * 0.45f);
        delayBufferR[delayWriteIdx] = wetInput + (delayOutR * 0.45f);

        delayWriteIdx = (delayWriteIdx + 1) % 44100;

        channelL[sampleIdx] = drySample + (delayOutL * 0.4f);
        channelR[sampleIdx] = drySample + (delayOutR * 0.4f);
    }
}

void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (rootKeyPtr->load()));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (scaleTypePtr->load()));
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
        hasSceneA = true;
        hasSceneB = true;
        sceneASlotsSaved[slotIndex] = true; 
        sceneBSlotsSaved[slotIndex] = true;

        sceneAPresets[slotIndex] = sceneA; 
        sceneBPresets[slotIndex] = sceneB;

        for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = faderPtrs[i]->load(); 
        presets[slotIndex].rhythmMorph = rhythmMorphPtr->load(); 
        presets[slotIndex].rest = restPtr->load(); 
        presets[slotIndex].legato = legatoPtr->load(); 
        presets[slotIndex].entropy = entropyPtr->load(); 
        presets[slotIndex].harmony = harmonyPtr->load(); 
        presets[slotIndex].chaos = chaosPtr->load(); 
        presets[slotIndex].rate = ratePtr->load();
        presets[slotIndex].octaves = octavesPtr->load();
        
        for (int i = 0; i < 8; ++i) { 
            presets[slotIndex].lfoRates[i] = static_cast<int> (lfoRatePtrs[i]->load()); 
            presets[slotIndex].lfoDepths[i] = lfoDepthPtrs[i]->load(); 
        }
        presetSlotsSaved[slotIndex] = true; activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::loadPreset (int slotIndex) 
{ 
    if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex]) 
    { 
        pendingPresetToLoad.store (slotIndex);

        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph); 
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest); 
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato); 
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((presets[slotIndex].entropy + 1.0f) * 0.5f); 
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony); 
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos); 
        apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (presets[slotIndex].rate);
        apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((presets[slotIndex].octaves + 3.0f) / 6.0f); 

        juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
        juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };
        for (int i = 0; i < 8; ++i) { 
            apvts.getParameter (rates[i].getParamID())->setValueNotifyingHost (static_cast<float>(presets[slotIndex].lfoRates[i]) / 4.0f); 
            apvts.getParameter (depths[i].getParamID())->setValueNotifyingHost (presets[slotIndex].lfoDepths[i]); 
        }
        for (int i = 0; i < 8; ++i) apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]); 
        activePresetIndex.store (slotIndex); 
    } 
}

void PluginProcessor::captureScene (int side) 
{ 
    // Handled directly and safely in the editor's timerCallback 
    // during active drag states to prevent feedback loops.
}

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }

void PluginProcessor::resetRhythm() 
{ 
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    for (int i = 0; i < 8; ++i) 
        activeScene.faders[i] = 1.0f; 
}

void PluginProcessor::diceMelody() 
{ 
    auto* r = &juce::Random::getSystemRandom(); 
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    for (int i = 0; i < 8; ++i) 
        activeScene.faders[i] = r->nextFloat();
}

void PluginProcessor::diceArticulation() 
{ 
    auto* r = &juce::Random::getSystemRandom(); 
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    activeScene.rest = r->nextFloat() * 0.5f; 
    activeScene.legato = 0.1f + r->nextFloat() * 0.9f; 
}

void PluginProcessor::diceTime() 
{ 
    auto* r = &juce::Random::getSystemRandom(); 
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    activeScene.rate = r->nextFloat(); 
    activeScene.octaves = static_cast<float> (r->nextInt (7) - 3); 
    apvts.getParameter (IDs::cycleLength.getParamID())->setValueNotifyingHost (static_cast<float>(r->nextInt(4)) / 3.0f); 
}

void PluginProcessor::diceNavy() 
{ 
    auto* r = &juce::Random::getSystemRandom(); 
    SceneState& activeScene = isSceneBActiveAnchor.load() ? sceneB : sceneA;
    activeScene.rhythmMorph = r->nextFloat(); 
    activeScene.entropy = -1.0f + r->nextFloat() * 2.0f; 
    activeScene.harmony = r->nextFloat(); 
    activeScene.chaos = r->nextFloat(); 
}

void PluginProcessor::diceActiveSceneA()
{
    auto* random = &juce::Random::getSystemRandom(); for (int i = 0; i < 8; ++i) sceneA.faders[i] = random->nextFloat();
    sceneA.rhythmMorph = random->nextFloat(); sceneA.rest = random->nextFloat() * 0.5f; sceneA.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneA.entropy = -1.0f + random->nextFloat() * 2.0f; sceneA.harmony = random->nextFloat(); sceneA.chaos = random->nextFloat();
    sceneA.rate = random->nextFloat(); sceneA.octaves = static_cast<float> (random->nextInt (7) - 3); hasSceneA = true;
}

void PluginProcessor::diceActiveSceneB()
{
    auto* random = &juce::Random::getSystemRandom(); for (int i = 0; i < 8; ++i) sceneB.faders[i] = random->nextFloat();
    sceneB.rhythmMorph = random->nextFloat(); sceneB.rest = random->nextFloat() * 0.5f; sceneB.legato = 0.1f + random->nextFloat() * 0.9f;
    sceneB.entropy = -1.0f + random->nextFloat() * 2.0f; sceneB.harmony = random->nextFloat(); sceneB.chaos = random->nextFloat();
    sceneB.rate = random->nextFloat(); sceneB.octaves = static_cast<float> (random->nextInt (7) - 3); hasSceneB = true;
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState(); 
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    
    if (xml != nullptr)
    {
        // 1. Serialize active workspace focus and presence flags
        xml->setAttribute ("isSceneBActiveAnchor", isSceneBActiveAnchor.load());
        xml->setAttribute ("hasSceneA", hasSceneA);
        xml->setAttribute ("hasSceneB", hasSceneB);

        // 2. Serialize current active Scene A profile
        auto* activeSceneANode = xml->createNewChildElement ("CURRENT_SCENE_A");
        if (activeSceneANode != nullptr)
        {
            activeSceneANode->setAttribute ("morph", sceneA.rhythmMorph);
            activeSceneANode->setAttribute ("rest", sceneA.rest);
            activeSceneANode->setAttribute ("legato", sceneA.legato);
            activeSceneANode->setAttribute ("rate", sceneA.rate);
            activeSceneANode->setAttribute ("entropy", sceneA.entropy);
            activeSceneANode->setAttribute ("harmony", sceneA.harmony);
            activeSceneANode->setAttribute ("chaos", sceneA.chaos);
            activeSceneANode->setAttribute ("octaves", sceneA.octaves);
            for (int f = 0; f < 8; ++f) 
                activeSceneANode->setAttribute ("fader_" + juce::String (f), sceneA.faders[f]);
            for (int l = 0; l < 8; ++l) {
                activeSceneANode->setAttribute ("lfo_r_" + juce::String (l), sceneA.lfoRates[l]);
                activeSceneANode->setAttribute ("lfo_d_" + juce::String (l), sceneA.lfoDepths[l]);
            }
        }

        // 3. Serialize current active Scene B profile
        auto* activeSceneBNode = xml->createNewChildElement ("CURRENT_SCENE_B");
        if (activeSceneBNode != nullptr)
        {
            activeSceneBNode->setAttribute ("morph", sceneB.rhythmMorph);
            activeSceneBNode->setAttribute ("rest", sceneB.rest);
            activeSceneBNode->setAttribute ("legato", sceneB.legato);
            activeSceneBNode->setAttribute ("rate", sceneB.rate);
            activeSceneBNode->setAttribute ("entropy", sceneB.entropy);
            activeSceneBNode->setAttribute ("harmony", sceneB.harmony);
            activeSceneBNode->setAttribute ("chaos", sceneB.chaos);
            activeSceneBNode->setAttribute ("octaves", sceneB.octaves);
            for (int f = 0; f < 8; ++f) 
                activeSceneBNode->setAttribute ("fader_" + juce::String (f), sceneB.faders[f]);
            for (int l = 0; l < 8; ++l) {
                activeSceneBNode->setAttribute ("lfo_r_" + juce::String (l), sceneB.lfoRates[l]);
                activeSceneBNode->setAttribute ("lfo_d_" + juce::String (l), sceneB.lfoDepths[l]);
            }
        }

        // 4. Serialize scene presets (Slot matrices)
        auto* presetsNodeA = xml->createNewChildElement ("SCENE_A_PRESETS");
        auto* presetsNodeB = xml->createNewChildElement ("SCENE_B_PRESETS");
        
        if (presetsNodeA != nullptr && presetsNodeB != nullptr)
        {
            for (int i = 0; i < 8; ++i) 
            {
                auto* childA = presetsNodeA->createNewChildElement ("SLOT_" + juce::String (i)); 
                if (childA != nullptr) {
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
                        for (int f = 0; f < 8; ++f) 
                            childA->setAttribute ("fader_" + juce::String (f), sceneAPresets[i].faders[f]);
                        for (int l = 0; l < 8; ++l) { 
                            childA->setAttribute ("lfo_r_" + juce::String (l), sceneAPresets[i].lfoRates[l]); 
                            childA->setAttribute ("lfo_d_" + juce::String (l), sceneAPresets[i].lfoDepths[l]); 
                        }
                    }
                }
                
                auto* childB = presetsNodeB->createNewChildElement ("SLOT_" + juce::String (i)); 
                if (childB != nullptr) {
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
                        
                        for (int f = 0; f < 8; ++f) 
                            childB->setAttribute ("fader_" + juce::String (f), sceneBPresets[i].faders[f]);
                        for (int l = 0; l < 8; ++l) { 
                            childB->setAttribute ("lfo_r_" + juce::String (l), sceneBPresets[i].lfoRates[l]); 
                            childB->setAttribute ("lfo_d_" + juce::String (l), sceneBPresets[i].lfoDepths[l]); 
                        }
                    }
                }
            }
        }
        
        // 5. Serialize global performance preset recall banks
        auto* banksNode = xml->createNewChildElement ("GLOBAL_BANKS");
        if (banksNode != nullptr)
        {
            for (int i = 0; i < 8; ++i) {
                auto* childBank = banksNode->createNewChildElement ("BANK_" + juce::String (i)); 
                if (childBank != nullptr) {
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
                        for (int f = 0; f < 8; ++f) 
                            childBank->setAttribute ("fader_" + juce::String (f), presets[i].faders[f]);
                        for (int l = 0; l < 8; ++l) { 
                            childBank->setAttribute ("lfo_r_" + juce::String (l), presets[i].lfoRates[l]); 
                            childBank->setAttribute ("lfo_d_" + juce::String (l), presets[i].lfoDepths[l]); 
                        }
                    }
                }
            }
        }

        // 6. Serialize standalone custom MIDI CC mapping tables [3]
        auto* mappingsNode = xml->createNewChildElement ("MIDI_CC_MAPPINGS");
        if (mappingsNode != nullptr)
        {
            for (int i = 0; i < 18; ++i)
            {
                mappingsNode->setAttribute ("param_" + juce::String (i), midiCcMappings[i].load());
            }
        }
        
        copyXmlToBinary (*xml, destData);
    }
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType())) {
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        
        // 1. Restore active workspace focus and presence flags
        isSceneBActiveAnchor.store (xmlState->getBoolAttribute ("isSceneBActiveAnchor", false));
        hasSceneA = xmlState->getBoolAttribute ("hasSceneA", false);
        hasSceneB = xmlState->getBoolAttribute ("hasSceneB", false);

        // 2. Deserialization of current active Scene A state
        if (auto* activeSceneANode = xmlState->getChildByName ("CURRENT_SCENE_A"))
        {
            sceneA.rhythmMorph = static_cast<float> (activeSceneANode->getDoubleAttribute ("morph"));
            sceneA.rest = static_cast<float> (activeSceneANode->getDoubleAttribute ("rest"));
            sceneA.legato = static_cast<float> (activeSceneANode->getDoubleAttribute ("legato"));
            sceneA.rate = static_cast<float> (activeSceneANode->getDoubleAttribute ("rate"));
            sceneA.entropy = static_cast<float> (activeSceneANode->getDoubleAttribute ("entropy"));
            sceneA.harmony = static_cast<float> (activeSceneANode->getDoubleAttribute ("harmony"));
            sceneA.chaos = static_cast<float> (activeSceneANode->getDoubleAttribute ("chaos"));
            sceneA.octaves = static_cast<float> (activeSceneANode->getDoubleAttribute ("octaves"));
            for (int f = 0; f < 8; ++f) 
                sceneA.faders[f] = static_cast<float> (activeSceneANode->getDoubleAttribute ("fader_" + juce::String (f), 1.0f));
            for (int l = 0; l < 8; ++l) {
                sceneA.lfoRates[l] = activeSceneANode->getIntAttribute ("lfo_r_" + juce::String (l));
                sceneA.lfoDepths[l] = static_cast<float> (activeSceneANode->getDoubleAttribute ("lfo_d_" + juce::String (l)));
            }
        }

        // 3. Deserialization of current active Scene B state
        if (auto* activeSceneBNode = xmlState->getChildByName ("CURRENT_SCENE_B"))
        {
            sceneB.rhythmMorph = static_cast<float> (activeSceneBNode->getDoubleAttribute ("morph"));
            sceneB.rest = static_cast<float> (activeSceneBNode->getDoubleAttribute ("rest"));
            sceneB.legato = static_cast<float> (activeSceneBNode->getDoubleAttribute ("legato"));
            sceneB.rate = static_cast<float> (activeSceneBNode->getDoubleAttribute ("rate"));
            sceneB.entropy = static_cast<float> (activeSceneBNode->getDoubleAttribute ("entropy"));
            sceneB.harmony = static_cast<float> (activeSceneBNode->getDoubleAttribute ("harmony"));
            sceneB.chaos = static_cast<float> (activeSceneBNode->getDoubleAttribute ("chaos"));
            sceneB.octaves = static_cast<float> (activeSceneBNode->getDoubleAttribute ("octaves"));
            for (int f = 0; f < 8; ++f) 
                sceneB.faders[f] = static_cast<float> (activeSceneBNode->getDoubleAttribute ("fader_" + juce::String (f), 1.0f));
            for (int l = 0; l < 8; ++l) {
                sceneB.lfoRates[l] = activeSceneBNode->getIntAttribute ("lfo_r_" + juce::String (l));
                sceneB.lfoDepths[l] = static_cast<float> (activeSceneBNode->getDoubleAttribute ("lfo_d_" + juce::String (l)));
            }
        }

        // 4. Deserialization of slot presets matrices
        if (auto* presetsNodeA = xmlState->getChildByName ("SCENE_A_PRESETS")) {
            for (int i = 0; i < 8; ++i) {
                if (auto* childA = presetsNodeA->getChildByName ("SLOT_" + juce::String (i))) {
                    sceneASlotsSaved[i] = childA->getBoolAttribute ("saved");
                    if (sceneASlotsSaved[i]) {
                        sceneAPresets[i].rhythmMorph = static_cast<float> (childA->getDoubleAttribute ("morph")); 
                        sceneAPresets[i].rest = static_cast<float> (childA->getDoubleAttribute ("rest")); 
                        sceneAPresets[i].legato = static_cast<float> (childA->getDoubleAttribute ("legato"));
                        sceneAPresets[i].rate = static_cast<float> (childA->getDoubleAttribute ("rate")); 
                        sceneAPresets[i].entropy = static_cast<float> (childA->getDoubleAttribute ("entropy")); 
                        sceneAPresets[i].harmony = static_cast<float> (childA->getDoubleAttribute ("harmony"));
                        sceneAPresets[i].chaos = static_cast<float> (childA->getDoubleAttribute ("chaos")); 
                        sceneAPresets[i].octaves = static_cast<float> (childA->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) 
                            sceneAPresets[i].faders[f] = static_cast<float> (childA->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { 
                            sceneAPresets[i].lfoRates[l] = childA->getIntAttribute ("lfo_r_" + juce::String (l)); 
                            sceneAPresets[i].lfoDepths[l] = static_cast<float> (childA->getDoubleAttribute ("lfo_d_" + juce::String (l))); 
                        }
                    }
                }
            }
        }
        
        if (auto* presetsNodeB = xmlState->getChildByName ("SCENE_B_PRESETS")) { 
            for (int i = 0; i < 8; ++i) {
                if (auto* childB = presetsNodeB->getChildByName ("SLOT_" + juce::String (i))) {
                    sceneBSlotsSaved[i] = childB->getBoolAttribute ("saved");
                    if (sceneBSlotsSaved[i]) {
                        sceneBPresets[i].rhythmMorph = static_cast<float> (childB->getDoubleAttribute ("morph")); 
                        sceneBPresets[i].rest = static_cast<float> (childB->getDoubleAttribute ("rest")); 
                        sceneBPresets[i].legato = static_cast<float> (childB->getDoubleAttribute ("legato"));
                        sceneBPresets[i].rate = static_cast<float> (childB->getDoubleAttribute ("rate")); 
                        sceneBPresets[i].entropy = static_cast<float> (childB->getDoubleAttribute ("entropy")); 
                        sceneBPresets[i].harmony = static_cast<float> (childB->getDoubleAttribute ("harmony"));
                        sceneBPresets[i].chaos = static_cast<float> (childB->getDoubleAttribute ("chaos")); 
                        sceneBPresets[i].octaves = static_cast<float> (childB->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) 
                            sceneBPresets[i].faders[f] = static_cast<float> (childB->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { 
                            sceneBPresets[i].lfoRates[l] = childB->getIntAttribute ("lfo_r_" + juce::String (l)); 
                            sceneBPresets[i].lfoDepths[l] = static_cast<float> (childB->getDoubleAttribute ("lfo_d_" + juce::String (l))); 
                        }
                    }
                }
            }
        }
        
        // 5. Deserialization of global banks recall slots
        if (auto* banksNode = xmlState->getChildByName ("GLOBAL_BANKS")) {
            for (int i = 0; i < 8; ++i) {
                if (auto* childBank = banksNode->getChildByName ("BANK_" + juce::String (i))) {
                    presetSlotsSaved[i] = childBank->getBoolAttribute ("saved");
                    if (presetSlotsSaved[i]) {
                        presets[i].rhythmMorph = static_cast<float> (childBank->getDoubleAttribute ("morph")); 
                        presets[i].rest = static_cast<float> (childBank->getDoubleAttribute ("rest")); 
                        presets[i].legato = static_cast<float> (childBank->getDoubleAttribute ("legato"));
                        presets[i].rate = static_cast<float> (childBank->getDoubleAttribute ("rate")); 
                        presets[i].entropy = static_cast<float> (childBank->getDoubleAttribute ("entropy")); 
                        presets[i].harmony = static_cast<float> (childBank->getDoubleAttribute ("harmony"));
                        presets[i].chaos = static_cast<float> (childBank->getDoubleAttribute ("chaos")); 
                        presets[i].octaves = static_cast<float> (childBank->getDoubleAttribute ("octaves"));
                        for (int f = 0; f < 8; ++f) 
                            presets[i].faders[f] = static_cast<float> (childBank->getDoubleAttribute ("fader_" + juce::String (f)));
                        for (int l = 0; l < 8; ++l) { 
                            presets[i].lfoRates[l] = childBank->getIntAttribute ("lfo_r_" + juce::String (l)); 
                            presets[i].lfoDepths[l] = static_cast<float> (childBank->getDoubleAttribute ("lfo_d_" + juce::String (l))); 
                        }
                    }
                }
            }
        }

        // 6. Deserialization of standalone custom MIDI CC mappings [3]
        if (auto* mappingsNode = xmlState->getChildByName ("MIDI_CC_MAPPINGS"))
        {
            for (int i = 0; i < 18; ++i)
            {
                midiCcMappings[i].store (mappingsNode->getIntAttribute ("param_" + juce::String (i), -1));
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
    
    // Updated continuous Float rate dial to map BPM or Synced Subdivision rate dynamically [1.2.3]
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rate, "BPM or Rate", 0.0f, 1.0f, 0.5f)); 
    params.push_back (std::make_unique<juce::AudioParameterInt> (IDs::octaves, "Octaves", -3, 3, 0)); 
    
    // Theme choices
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::panelTheme, "Panel Theme", juce::StringArray { "Navy", "Monochrome", "Matrix" }, 0));

    // Register Master Parameters [1.2.0]
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::masterVelocity, "Note Density", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::masterSwing, "Master Swing", 0.0f, 1.0f, 0.0f));

    // Register Sync Toggle Parameter [1.2.3]
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID ("sync", 1), "Sync Mode", true));

    // Register Left Panel Sound Parameters [3]
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::midiInChannel, "MIDI In Channel", juce::StringArray { "Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::midiOutChannel, "MIDI Out Channel", juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::voice1Synth, "Voice 1 Synth", juce::StringArray { "Analog", "FM", "Resonator" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice1Decay, "Voice 1 Decay", 0.05f, 2.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice1Timbre, "Voice 1 Timbre", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice1Reverb, "Voice 1 Reverb", 0.0f, 1.0f, 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::voice2Synth, "Voice 2 Synth", juce::StringArray { "Analog", "FM", "Resonator" }, 2));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice2Decay, "Voice 2 Decay", 0.05f, 2.0f, 0.8f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice2Timbre, "Voice 2 Timbre", 0.0f, 1.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::voice2Reverb, "Voice 2 Reverb", 0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::audioRouting, "Audio Routing", juce::StringArray { "Split A->1 / B->2", "Layered (Voice 1)", "External Out Only" }, 0));

    auto regLfo = [&](juce::ParameterID rId, juce::ParameterID dId, juce::String nm) {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (rId, nm + " LFO Speed", juce::StringArray { "Off", "1/4", "1/8", "1/16", "1/32" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (dId, nm + " LFO Depth", 0.0f, 1.0f, 0.0f));
    };
    regLfo (IDs::rhythmMorphLfoRate, IDs::rhythmMorphLfoDepth, "Morph"); regLfo (IDs::restLfoRate, IDs::restLfoDepth, "Rest"); regLfo (IDs::legatoLfoRate, IDs::legatoLfoDepth, "Legato"); regLfo (IDs::rateLfoRate, IDs::rateLfoDepth, "Rate");
    regLfo (IDs::entropyLfoRate, IDs::entropyLfoDepth, "Entropy"); regLfo (IDs::harmonyLfoRate, IDs::harmonyLfoDepth, "Harmony"); regLfo (IDs::chaosLfoRate, IDs::chaosLfoDepth, "Chaos"); regLfo (IDs::octavesLfoRate, IDs::octavesLfoDepth, "Octaves");

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}