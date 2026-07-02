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

    // Reconstructed Inline Helpers accessed by PluginEditor
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

    // Public sequencer position tracking accessed by UI thread (Atomically Thread-Safe) [43]
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

    // Thread-safe deferred preset exchange flag [43]
    std::atomic<int> pendingPresetToLoad { -1 };

    // Pre-Cached Parameter Pointers to avoid hashing on the Audio Thread [43]
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
    std::atomic<float>* lfoRatePtrs[8] { nullptr };
    std::atomic<float>* lfoDepthPtrs[8] { nullptr };

private:
    //==============================================================================
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

    double lfoPhases[8] { 0.0 };
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};