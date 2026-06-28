#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <atomic>
#include <array>

namespace IDs 
{
    #define DECLARE_ID(name) const juce::ParameterID name (#name, 1)
    DECLARE_ID(fader1); DECLARE_ID(fader2); DECLARE_ID(fader3); DECLARE_ID(fader4);
    DECLARE_ID(fader5); DECLARE_ID(fader6); DECLARE_ID(fader7); DECLARE_ID(fader8);
    DECLARE_ID(rhythmMorph); DECLARE_ID(rest); DECLARE_ID(legato);
    DECLARE_ID(entropy); DECLARE_ID(harmony); DECLARE_ID(chaos);
    DECLARE_ID(morph); DECLARE_ID(latch); DECLARE_ID(arpSeq); DECLARE_ID(poly); DECLARE_ID(freeze);
    DECLARE_ID(rootKey); DECLARE_ID(scaleType); DECLARE_ID(cycleLength);
    DECLARE_ID(rate); DECLARE_ID(octaves); 

    // Dynamic Panel Theme selector parameter
    DECLARE_ID(panelTheme);
    DECLARE_ID(midiChannel); // MIDI Output Channel Selector

    // Unified 8-channel LFO Preset Choice Parameters (Off, Drift, Bounce, Vibe, Tremor)
    DECLARE_ID(rhythmMorphLfoPreset);
    DECLARE_ID(restLfoPreset);
    DECLARE_ID(legatoLfoPreset);
    DECLARE_ID(rateLfoPreset);
    DECLARE_ID(entropyLfoPreset);
    DECLARE_ID(harmonyLfoPreset);
    DECLARE_ID(chaosLfoPreset);
    DECLARE_ID(octavesLfoPreset);
    #undef DECLARE_ID
}

struct SceneState {
    float faders[8] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    float rhythmMorph = 0.0f, rest = 0.1f, legato = 0.5f, rate = 2.0f;
    float entropy = 0.0f, harmony = 0.0f, chaos = 0.0f, octaves = 0.0f;
    int lfoPresets[8] = { 0 }; // Stored LFO preset choices per scene
};

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Symmetrical Scene Management
    void saveSceneA() { captureScene (0); }
    void saveSceneB() { captureScene (1); }
    void clearSceneA();
    void clearSceneB();

    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    void clearPreset (int slotIndex); // Clears preset slot back to empty
    bool isPresetSaved (int slotIndex) const { return presetSlotsSaved[slotIndex]; }

    // Active Anchor Parameter Management Method Declarations
    void setActiveAnchor (bool useSceneB);
    void captureActiveParametersToActiveScene();

    // Unified Project-Based Scene Arrays
    SceneState sceneAPresets[8];
    SceneState sceneBPresets[8];
    bool sceneASlotsSaved[8] = { false };
    bool sceneBSlotsSaved[8] = { false };

    SceneState sceneA;
    SceneState sceneB;
    bool hasSceneA = false;
    bool hasSceneB = false;

    std::atomic<int> activePresetIndex { 0 }; 
    std::atomic<bool> isSceneBActiveAnchor { false }; 

    // Generative triggers
    void diceMelody();
    void diceArticulation();
    void diceTime();
    void diceNavy();
    void diceActiveSceneA(); 
    void diceActiveSceneB(); 
    void resetAccumulator();
    void resetRhythm();
    void triggerDiatonicChordPad (int padIndex);

    int currentStep = 0;
    int currentBarInCycle = 1;

    std::atomic<bool> isCurrentlyPlayingUI { false };
    std::atomic<int> activeChordExtensionType { 0 }; 

    // Public active values modulated in real-time by the internal LFOs
    float activeMorph = 0.0f;
    float activeRest = 0.1f;
    float activeLegato = 0.5f;
    int activeRateIdx = 2; 
    float activeEntropy = 0.0f;
    float activeHarmony = 0.0f;
    float activeChaos = 0.0f;
    int activeOctavesVal = 0;

    std::vector<int> activeHeldNotes;
    std::vector<int> latchedNotes;
    bool isFirstNoteOfNewChord = true;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateLfoModulations (int numSamples, double bpm);
    std::vector<int> generateEuclideanPattern (int steps, int pulses);
    void scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples);
    void captureScene (int side);

    double mSampleRate = 44100.0, mSongPositionPPQ = 0.0;
    int mTimeInSamples = 0, mLastStep = -1, mLastNotePlayed = -1, mNoteOffTime = 0; 
    
    std::vector<std::pair<int, int>> scheduledNoteOffs;

    // 8 Independent LFO phases
    double lfoPhases[8] = { 0.0 };

    float modRest = 0.1f;
    float modLegato = 0.5f;
    float modEntropy = 0.0f;
    float modHarmony = 0.0f;
    float modChaos = 0.0f;
    float accumulatedPitchOffset = 0.0f;

    std::vector<int> lastChordPitches;

    // Presets 1-8 Master Kits
    SceneState presets[8];
    bool presetSlotsSaved[8] = { false };

    // Slew smoothing arrays for transitions
    float currentSlewTarget[24] = { 0.0f };
    float currentSlewValue[24] = { 0.0f };

    bool lastSceneBActiveState = false;

    // Snapshot freeze cache variables
    float frozenFaders[8] = { 0.5f };
    std::vector<int> frozenActiveHeldNotes;
    std::vector<int> frozenLatchedNotes;
    float frozenMorph = 0.0f, frozenRest = 0.1f, frozenLegato = 0.5f, frozenEntropy = 0.0f, frozenHarmony = 0.0f, frozenChaos = 0.0f;
    int frozenRateIdx = 2, frozenOctavesVal = 0;
    bool lastFreezeState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};