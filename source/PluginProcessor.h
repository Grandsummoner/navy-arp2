#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

//==============================================================================
namespace IDs
{
    inline const juce::ParameterID fader1             { "fader1", 1 };
    inline const juce::ParameterID fader2             { "fader2", 1 };
    inline const juce::ParameterID fader3             { "fader3", 1 };
    inline const juce::ParameterID fader4             { "fader4", 1 };
    inline const juce::ParameterID fader5             { "fader5", 1 };
    inline const juce::ParameterID fader6             { "fader6", 1 };
    inline const juce::ParameterID fader7             { "fader7", 1 };
    inline const juce::ParameterID fader8             { "fader8", 1 };

    inline const juce::ParameterID rhythmMorph         { "rhythmMorph", 1 };
    inline const juce::ParameterID rest                { "rest", 1 };
    inline const juce::ParameterID legato              { "legato", 1 };
    inline const juce::ParameterID entropy             { "entropy", 1 };
    inline const juce::ParameterID harmony             { "harmony", 1 };
    inline const juce::ParameterID chaos               { "chaos", 1 };
    inline const juce::ParameterID morph               { "morph", 1 };
    inline const juce::ParameterID latch               { "latch", 1 };
    inline const juce::ParameterID arpSeq              { "arpSeq", 1 };
    inline const juce::ParameterID poly                { "poly", 1 };
    inline const juce::ParameterID freeze              { "freeze", 1 };
    inline const juce::ParameterID rootKey             { "rootKey", 1 };
    inline const juce::ParameterID scaleType           { "scaleType", 1 };
    inline const juce::ParameterID cycleLength         { "cycleLength", 1 };
    inline const juce::ParameterID rate                { "rate", 1 };
    inline const juce::ParameterID octaves             { "octaves", 1 };
    inline const juce::ParameterID panelTheme          { "panelTheme", 1 };

    // Master Parameter IDs
    inline const juce::ParameterID masterVelocity     { "masterVelocity", 1 };
    inline const juce::ParameterID masterSwing        { "masterSwing", 1 };

    // Left Panel Sound Engine Parameter IDs
    inline const juce::ParameterID midiInChannel      { "midiInChannel", 1 };
    inline const juce::ParameterID midiOutChannel     { "midiOutChannel", 1 };
    
    // Voice 1 Multi-Select Instrument Parameter IDs [3]
    inline const juce::ParameterID voice1Analog       { "voice1Analog", 1 };
    inline const juce::ParameterID voice1Fm           { "voice1Fm", 1 };
    inline const juce::ParameterID voice1String       { "voice1String", 1 };
    inline const juce::ParameterID voice1Pulse        { "voice1Pulse", 1 };

    inline const juce::ParameterID voice1Attack       { "voice1Attack", 1 };
    inline const juce::ParameterID voice1Decay        { "voice1Decay", 1 };
    inline const juce::ParameterID voice1Sustain      { "voice1Sustain", 1 };
    inline const juce::ParameterID voice1Release      { "voice1Release", 1 };
    inline const juce::ParameterID voice1Timbre       { "voice1Timbre", 1 };
    inline const juce::ParameterID voice1Reverb       { "voice1Reverb", 1 };
    inline const juce::ParameterID voice1Delay        { "voice1Delay", 1 }; // New [3]
    
    // Voice 2 Multi-Select Instrument Parameter IDs [3]
    inline const juce::ParameterID voice2Analog       { "voice2Analog", 1 };
    inline const juce::ParameterID voice2Fm           { "voice2Fm", 1 };
    inline const juce::ParameterID voice2String       { "voice2String", 1 };
    inline const juce::ParameterID voice2Pulse        { "voice2Pulse", 1 };

    inline const juce::ParameterID voice2Attack       { "voice2Attack", 1 };
    inline const juce::ParameterID voice2Decay        { "voice2Decay", 1 };
    inline const juce::ParameterID voice2Sustain      { "voice2Sustain", 1 };
    inline const juce::ParameterID voice2Release      { "voice2Release", 1 };
    inline const juce::ParameterID voice2Timbre       { "voice2Timbre", 1 };
    inline const juce::ParameterID voice2Reverb       { "voice2Reverb", 1 };
    inline const juce::ParameterID voice2Delay        { "voice2Delay", 1 }; // New [3]
    
    inline const juce::ParameterID audioRouting       { "audioRouting", 1 };
    inline const juce::ParameterID voice1Gain         { "voice1Gain", 1 };
    inline const juce::ParameterID voice2Gain         { "voice2Gain", 1 };

    // LFO Parameter IDs
    inline const juce::ParameterID rhythmMorphLfoRate  { "rhythmMorphLfoRate", 1 };
    inline const juce::ParameterID rhythmMorphLfoDepth { "rhythmMorphLfoDepth", 1 };
    inline const juce::ParameterID restLfoRate         { "restLfoRate", 1 };
    inline const juce::ParameterID restLfoDepth        { "restLfoDepth", 1 };
    inline const juce::ParameterID legatoLfoRate       { "legatoLfoRate", 1 };
    inline const juce::ParameterID legatoLfoDepth      { "legatoLfoDepth", 1 };
    inline const juce::ParameterID rateLfoRate         { "rateLfoRate", 1 };
    inline const juce::ParameterID rateLfoDepth        { "rateLfoDepth", 1 };
    inline const juce::ParameterID entropyLfoRate      { "entropyLfoRate", 1 };
    inline const juce::ParameterID entropyLfoDepth     { "entropyLfoDepth", 1 };
    inline const juce::ParameterID harmonyLfoRate      { "harmonyLfoRate", 1 };
    inline const juce::ParameterID harmonyLfoDepth     { "harmonyLfoDepth", 1 };
    inline const juce::ParameterID chaosLfoRate        { "chaosLfoRate", 1 };
    inline const juce::ParameterID chaosLfoDepth       { "chaosLfoDepth", 1 };
    inline const juce::ParameterID octavesLfoRate      { "octavesLfoRate", 1 };
    inline const juce::ParameterID octavesLfoDepth     { "octavesLfoDepth", 1 };
}

//==============================================================================
struct SceneState
{
    float faders[8] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    float rhythmMorph { 0.0f };
    float rest { 0.1f };
    float legato { 0.5f };
    float rate { 2.0f / 3.0f };
    float entropy { 0.0f };
    float harmony { 0.0f };
    float chaos { 0.0f };
    float octaves { 0.0f };
    int lfoRates[8] { 0, 0, 0, 0, 0, 0, 0, 0 };
    float lfoDepths[8] { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};

// =====================================================================
// LOW-LEVEL EMBEDDED DSP SYNTH VOICE (ZERO ALLOCATION, STANDALONE-SAFE)
// =====================================================================
struct SynthVoice
{
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    
    // FM synthesis modulator phase and frequency states
    float fmModPhase = 0.0f;
    float fmModFreq = 0.0f;
    
    // Karplus-Strong physical modeling resonator delay ring buffer
    float resBuffer[2048] { 0.0f };
    int resWriteIdx = 0;
    float noiseImpulse = 0.0f;
    float lastFilterOut = 0.0f; // Stable 1-pole state variable for noise damping
    
    // Subtractive resonant feedback lowpass filter states
    float s1 = 0.0f, s2 = 0.0f;

    // ADSR State parameters
    float attack = 0.01f;
    float decay = 0.35f;
    float sustain = 0.70f;
    float release = 0.25f;

    enum class EnvState { Idle, Attack, Decay, Sustain, Release };
    EnvState envState = EnvState::Idle;
    float envVal = 0.0f;
    float releaseLevel = 0.0f;
    double stateTime = 0.0;

    void triggerNote (int pitch)
    {
        float freq = 440.0f * std::pow (2.0f, (pitch - 69.0f) / 12.0f);
        phaseIncrement = freq / static_cast<float> (sampleRate);
        
        // Symmetrical ADSR trigger initialization
        envState = EnvState::Attack;
        stateTime = 0.0;
        envVal = 0.0f;
        
        // Feed the physical model noise generator impulse
        noiseImpulse = 1.0f;
        lastFilterOut = 0.0f; // Reset feedback state
    }

    void releaseNote()
    {
        if (envState != EnvState::Idle && envState != EnvState::Release)
        {
            envState = EnvState::Release;
            releaseLevel = envVal;
            stateTime = 0.0;
        }
    }

    float process (bool analogActive, bool fmActive, bool stringActive, bool pulseActive, float timbre)
    {
        // ADSR State Machine Processing
        double dt = 1.0 / sampleRate;
        stateTime += dt;

        switch (envState)
        {
            case EnvState::Idle:
                envVal = 0.0f;
                break;
            case EnvState::Attack:
            {
                float dur = std::max (0.001f, attack);
                envVal = static_cast<float> (stateTime / dur);
                if (envVal >= 1.0f) { envVal = 1.0f; envState = EnvState::Decay; stateTime = 0.0; }
                break;
            }
            case EnvState::Decay:
            {
                float dur = std::max (0.001f, decay);
                float progress = static_cast<float> (stateTime / dur);
                if (progress >= 1.0f) { envVal = sustain; envState = EnvState::Sustain; stateTime = 0.0; }
                else { envVal = 1.0f - (1.0f - sustain) * progress; }
                break;
            }
            case EnvState::Sustain:
                envVal = sustain;
                break;
            case EnvState::Release:
            {
                float dur = std::max (0.001f, release);
                float progress = static_cast<float> (stateTime / dur);
                if (progress >= 1.0f) { envVal = 0.0f; envState = EnvState::Idle; }
                else { envVal = releaseLevel * (1.0f - progress); }
                break;
            }
        }

        if (envVal <= 0.0001f) { envVal = 0.0f; return 0.0f; }

        float totalOutput = 0.0f;
        int activeCount = 0;

        // Sync and phase advancement flag for oscillator paths [3]
        bool needsPhaseAdvance = analogActive || fmActive || pulseActive;

        if (analogActive)
        {
            float wave = 2.0f * phase - 1.0f;
            float cutoff = 50.0f + timbre * 4000.0f;
            float wd = juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (sampleRate);
            float g = std::tan (wd * 0.5f);
            float h = g / (1.0f + g);
            
            float resonance = 0.5f; 
            float filterOut = (wave - s1 * resonance) * h + s1;
            s1 = filterOut;
            
            totalOutput += filterOut;
            activeCount++;
        }

        if (fmActive)
        {
            float modMultiplier = 3.5f; 
            float fmModPhaseIncrement = phaseIncrement * modMultiplier;
            
            fmModPhase += fmModPhaseIncrement;
            if (fmModPhase >= 1.0f) fmModPhase -= 1.0f;
            
            float modOut = std::sin (fmModPhase * juce::MathConstants<double>::twoPi);
            float modIndex = timbre * 8.0f * envVal;
            
            float carrierPhase = phase + modOut * modIndex * phaseIncrement;
            totalOutput += std::sin (carrierPhase * juce::MathConstants<double>::twoPi);
            activeCount++;
        }

        if (stringActive)
        {
            int delayLength = static_cast<int> (1.0f / phaseIncrement);
            if (delayLength >= 2048) delayLength = 2047;
            if (delayLength < 4) delayLength = 4;
            
            int readIdx = resWriteIdx - delayLength;
            if (readIdx < 0) readIdx += 2048;
            
            float delayedVal = resBuffer[readIdx];
            float excitation = 0.0f;
            
            if (noiseImpulse > 0.001f)
            {
                excitation = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * noiseImpulse;
                noiseImpulse *= 0.94f; // Clean, realistic strike decay
            }
            
            // Symmetrical 1-pole feedback lowpass filtering
            float filteredVal = 0.5f * (delayedVal + lastFilterOut);
            lastFilterOut = delayedVal;

            // DC offset and low frequency drift blocker
            filteredVal -= lastFilterOut * 0.005f; 
            
            float feedback = 0.85f + timbre * 0.11f; // Cap max feedback at 0.96f to prevent overloading
            float currentVal = excitation + filteredVal * feedback;
            
            resBuffer[resWriteIdx] = currentVal;
            resWriteIdx = (resWriteIdx + 1) % 2048;
            
            totalOutput += currentVal * 0.22f; // Low level string plucked wave
            activeCount++;
        }

        if (pulseActive)
        {
            float width = 0.15f + timbre * 0.7f; // PWM sweep from 15% to 85%
            float wave = (phase < width) ? 0.4f : -0.4f;
            
            float cutoff = 80.0f + timbre * 3500.0f;
            float wd = juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (sampleRate);
            float g = std::tan (wd * 0.5f);
            float h = g / (1.0f + g);
            
            float resonance = 0.45f;
            float filterOut = (wave - s1 * resonance) * h + s1;
            s1 = filterOut;
            
            totalOutput += filterOut * 0.6f;
            activeCount++;
        }

        if (needsPhaseAdvance)
        {
            phase += phaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
        }

        // Apply equal-power normalization scaling to prevent clipping on multiple selections [3]
        if (activeCount > 1)
        {
            totalOutput /= std::sqrt (static_cast<float> (activeCount));
        }

        return totalOutput * envVal;
    }
};

//==============================================================================
class PluginProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    PluginProcessor();
    ~PluginProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        return true;
    }

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "Navy Arp"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    const juce::String getProgramName (int index) override { juce::ignoreUnused (index); return {}; }
    void changeProgramName (int index, const juce::String& newName) override { juce::ignoreUnused (index, newName); }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Generative Engine & Preset Utilities
    //==============================================================================
    void scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples);
    void setActiveAnchor (bool useSceneB);
    void captureActiveParametersToActiveScene();
    void updateLfoModulations (int numSamples, double bpm);
    void triggerDiatonicChordPad (int padIndex);
    
    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    void captureScene (int side);

    void clearSceneA()
    {
        sceneA = SceneState();
        hasSceneA = false;
        
        if (!isSceneBActive())
        {
            apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (sceneA.rhythmMorph);
            apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (sceneA.rest);
            apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (sceneA.legato);
            apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((sceneA.entropy + 1.0f) * 0.5f);
            apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (sceneA.harmony);
            apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (sceneA.chaos);
            apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (sceneA.rate / 3.0f);
            apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((sceneA.octaves + 3.0f) / 6.0f);
            for (int i = 0; i < 8; ++i)
                apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (sceneA.faders[i]);
        }
    }

    void saveSceneA()
    {
        captureScene (0);
    }

    void clearSceneB()
    {
        sceneB = SceneState();
        hasSceneB = false;
        
        if (isSceneBActive())
        {
            apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (sceneB.rhythmMorph);
            apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (sceneB.rest);
            apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (sceneB.legato);
            apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((sceneB.entropy + 1.0f) * 0.5f);
            apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (sceneB.harmony);
            apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (sceneB.chaos);
            apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (sceneB.rate / 3.0f);
            apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost ((sceneB.octaves + 3.0f) / 6.0f);
            for (int i = 0; i < 8; ++i)
                apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (sceneB.faders[i]);
        }
    }

    void saveSceneB()
    {
        captureScene (1);
    }

    void resetAccumulator();
    void resetRhythm();
    void diceMelody();
    void diceArticulation();
    void diceTime();
    void diceNavy();

    void diceActiveSceneA();
    void diceActiveSceneB();

    // Shared State & Scene Snapshot Registers
    SceneState sceneA;
    SceneState sceneB;
    SceneState sceneAPresets[8];
    SceneState sceneBPresets[8];
    SceneState presets[8];

    bool sceneASlotsSaved[8] { false };
    bool sceneBSlotsSaved[8] { false };
    bool presetSlotsSaved[8] { false };

    bool hasSceneA { false };
    bool hasSceneB { false };

    std::atomic<int> currentStep { 0 };
    std::atomic<int> currentBarInCycle { 1 };

    std::atomic<int> activePresetIndex { 0 };
    std::atomic<bool> isSceneBActiveAnchor { false };
    std::atomic<bool> isCurrentlyPlayingUI { false };

    float currentSlewValue[24] { 0.5f };
    float currentSlewTarget[24] { 0.5f };

    bool isSceneBActive() const { return isSceneBActiveAnchor.load(); }
    void setSceneBActive (bool shouldBeB) { isSceneBActiveAnchor.store (shouldBeB); }

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<int> pendingPresetToLoad { -1 };

    // Pre-Cached Parameter Pointers to avoid hashing on the Audio Thread
    std::atomic<float>* faderPtrs[8] { nullptr };
    std::atomic<float>* rhythmMorphPtr { nullptr };
    std::atomic<float>* restPtr { nullptr };
    std::atomic<float>* legatoPtr { nullptr };
    std::atomic<float>* entropyPtr { nullptr };
    std::atomic<float>* harmonyPtr { nullptr };
    std::atomic<float>* chaosPtr { nullptr };
    std::atomic<float>* morphPtr { nullptr };
    std::atomic<float>* latchPtr { nullptr };
    std::atomic<float>* arpSeqPtr { nullptr };
    std::atomic<float>* polyPtr { nullptr };
    std::atomic<float>* freezePtr { nullptr };
    std::atomic<float>* rootKeyPtr { nullptr };
    std::atomic<float>* scaleTypePtr { nullptr };
    std::atomic<float>* cycleLengthPtr { nullptr };
    std::atomic<float>* ratePtr { nullptr };
    std::atomic<float>* octavesPtr { nullptr };

    // Master Parameter cached atomic pointers
    std::atomic<float>* masterVelocityPtr { nullptr };
    std::atomic<float>* masterSwingPtr { nullptr };

    // Left Panel Sound Engine cached atomic pointers
    std::atomic<float>* midiInChannelPtr { nullptr };
    std::atomic<float>* midiOutChannelPtr { nullptr };
    
    // Voice 1 Multi-Select instrument cache pointers [3]
    std::atomic<float>* voice1AnalogPtr { nullptr };
    std::atomic<float>* voice1FmPtr { nullptr };
    std::atomic<float>* voice1StringPtr { nullptr };
    std::atomic<float>* voice1PulsePtr { nullptr };

    std::atomic<float>* voice1AttackPtr { nullptr };
    std::atomic<float>* voice1DecayPtr { nullptr };
    std::atomic<float>* voice1SustainPtr { nullptr };
    std::atomic<float>* voice1ReleasePtr { nullptr };
    std::atomic<float>* voice1TimbrePtr { nullptr };
    std::atomic<float>* voice1ReverbPtr { nullptr };
    std::atomic<float>* voice1DelayPtr { nullptr }; // New [3]
    
    // Voice 2 Multi-Select instrument cache pointers [3]
    std::atomic<float>* voice2AnalogPtr { nullptr };
    std::atomic<float>* voice2FmPtr { nullptr };
    std::atomic<float>* voice2StringPtr { nullptr };
    std::atomic<float>* voice2PulsePtr { nullptr };

    std::atomic<float>* voice2AttackPtr { nullptr };
    std::atomic<float>* voice2DecayPtr { nullptr };
    std::atomic<float>* voice2SustainPtr { nullptr };
    std::atomic<float>* voice2ReleasePtr { nullptr };
    std::atomic<float>* voice2TimbrePtr { nullptr };
    std::atomic<float>* voice2ReverbPtr { nullptr };
    std::atomic<float>* voice2DelayPtr { nullptr }; // New [3]
    
    std::atomic<float>* audioRoutingPtr { nullptr };
    std::atomic<float>* voice1GainPtr { nullptr };
    std::atomic<float>* voice2GainPtr { nullptr };

    std::atomic<float>* lfoRatePtrs[8] { nullptr };
    std::atomic<float>* lfoDepthPtrs[8] { nullptr };

    double lfoPhases[8] { 0.0 };

    // Thread-Safe Standalone MIDI CC Mapping & Learn Registers
    std::atomic<int> midiCcMappings[18];        // Unmapped = -1. CC number = 0-127.
    std::atomic<int> activeMidiLearnIndex { -1 }; // Listening index (0-17) or -1.

    // Internal Symmetrical Polyphonic/Monophonic Audio Synths
    SynthVoice voice1;
    SynthVoice voice2;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double mSampleRate = 44100.0;
    int mLastStep = -1;
    int mLastNotePlayed = -1;
    int mNoteOffTime = 0;
    int mTimeInSamples = 0;

    std::vector<int> activeHeldNotes;
    std::vector<int> latchedNotes;
    std::vector<std::pair<int, int>> scheduledNoteOffs;
    std::vector<int> lastChordPitches;

    bool isFirstNoteOfNewChord = true;
    bool lastSceneBActiveState = false;

    double mSongPositionPPQ = 0.0;

    // Modulator States
    float activeMorph = 0.0f;
    float activeRest = 0.0f;
    float activeLegato = 0.0f;
    int activeRateIdx = 0;
    float activeEntropy = 0.0f;
    float activeHarmony = 0.0f;
    float activeChaos = 0.0f;
    int activeOctavesVal = 0;

    float modLegato = 0.5f;
    float modRest = 0.1f;
    float modEntropy = 0.0f;
    float modHarmony = 0.0f;
    float modChaos = 0.0f;

    // Freeze Snapshot States
    std::vector<int> frozenActiveHeldNotes;
    std::vector<int> frozenLatchedNotes;
    float frozenMorph = 0.0f;
    float frozenRest = 0.0f;
    float frozenLegato = 0.0f;
    float frozenEntropy = 0.0f;
    float frozenHarmony = 0.0f;
    float frozenChaos = 0.0f;
    int frozenRateIdx = 0;
    int frozenOctavesVal = 0;
    float frozenFaders[8] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    bool lastFreezeState = false;

    float accumulatedPitchOffset = 0.0f;

    // Basic internal Delay & Reverb effect buffers
    float delayBufferL[44100] { 0.0f };
    float delayBufferR[44100] { 0.0f };
    int delayWriteIdx = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};