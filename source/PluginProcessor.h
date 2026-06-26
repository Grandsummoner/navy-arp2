#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <atomic>

namespace IDs 
{
    #define DECLARE_ID(name) const juce::ParameterID name (#name, 1)
    DECLARE_ID(fader1); DECLARE_ID(fader2); DECLARE_ID(fader3); DECLARE_ID(fader4);
    DECLARE_ID(fader5); DECLARE_ID(fader6); DECLARE_ID(fader7); DECLARE_ID(fader8);
    DECLARE_ID(rhythmMorph); DECLARE_ID(rest); DECLARE_ID(legato);
    DECLARE_ID(entropy); DECLARE_ID(harmony); DECLARE_ID(chaos);
    DECLARE_ID(morph); DECLARE_ID(latch); DECLARE_ID(chordMode);
    DECLARE_ID(rootKey); DECLARE_ID(scaleType); DECLARE_ID(cycleLength);
    DECLARE_ID(rate); DECLARE_ID(octaves); 

    // Dynamic Panel Theme selector parameter [1]
    DECLARE_ID(panelTheme);

    // LFO Parameters
    DECLARE_ID(rhythmMorphLfoRate); DECLARE_ID(rhythmMorphLfoDepth);
    DECLARE_ID(restLfoRate);        DECLARE_ID(restLfoDepth);
    DECLARE_ID(legatoLfoRate);      DECLARE_ID(legatoLfoDepth);
    DECLARE_ID(rateLfoRate);        DECLARE_ID(rateLfoDepth);
    DECLARE_ID(entropyLfoRate);     DECLARE_ID(entropyLfoDepth);
    DECLARE_ID(harmonyLfoRate);     DECLARE_ID(harmonyLfoDepth);
    DECLARE_ID(chaosLfoRate);       DECLARE_ID(chaosLfoDepth);
    DECLARE_ID(octavesLfoRate);     DECLARE_ID(octavesLfoDepth);
    #undef DECLARE_ID
}

struct SceneState {
    float faders[8] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    float rhythmMorph = 0.0f;
    float rest = 0.1f;
    float legato = 0.5f;
    float rate = 2.0f; // Standard index (1/16)
    float entropy = 0.0f;
    float harmony = 0.0f;
    float chaos = 0.0f;
    float octaves = 0.0f; // Default 0 (no octave shift)

    // Full 16-channel LFO parameter states [5]
    int lfoRates[8] = { 0 };
    float lfoDepths[8] = { 0.0f };
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

    // Inline forwards delegating to the private consolidated helpers [5]
    void saveSceneA (int slotIndex) { saveScene (slotIndex, 0); }
    void loadSceneA (int slotIndex) { loadScene (slotIndex, 0); }
    void saveSceneB (int slotIndex) { saveScene (slotIndex, 1); }
    void loadSceneB (int slotIndex) { loadScene (slotIndex, 1); }

    bool isSceneASaved (int slotIndex) const { return sceneASlotsSaved[slotIndex]; }
    bool isSceneBSaved (int slotIndex) const { return sceneBSlotsSaved[slotIndex]; }

    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    bool isPresetSaved (int slotIndex) const { return presetSlotsSaved[slotIndex]; }

    void captureSceneA();
    void captureSceneB();
    void clearSceneA() { hasSceneA = false; }
    void clearSceneB() { hasSceneB = false; }

    // Generative triggers
    void diceMelody();
    void diceRhythm();
    
    // Explicit background dice randomizers (No secret focus variables) [NEW]
    void diceActiveSceneA(); 
    void diceActiveSceneB(); 

    void resetAccumulator();
    void resetRhythm();
    void triggerDiatonicChordPad (int padIndex);

    SceneState sceneA;
    SceneState sceneB;
    bool hasSceneA = false;
    bool hasSceneB = false;

    // 4x4 Octatrack Scene Memory [5]
    SceneState sceneAPresets[4];
    SceneState sceneBPresets[4];
    bool sceneASlotsSaved[4] = { false };
    bool sceneBSlotsSaved[4] = { false };
    
    std::atomic<int> activeSceneAIndex { 0 }; 
    std::atomic<int> activeSceneBIndex { 0 }; 

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

    // Private consolidated scene management helpers [5]
    void saveScene (int slotIndex, int side);
    void loadScene (int slotIndex, int side);

    double mSampleRate = 44100.0;
    int mTimeInSamples = 0;
    double mSongPositionPPQ = 0.0;
    
    int mLastStep = -1;
    int mLastNotePlayed = -1;
    int mNoteOffTime = 0; 
    
    std::vector<std::pair<int, int>> scheduledNoteOffs;

    // 8 Independent LFO phases
    double lfoPhases[8] = { 0.0 };

    double lfoPhaseEntropy = 0.0;
    double lfoPhaseChaos = 0.0;
    double lfoPhaseMorph = 0.0;
    double lfoPhaseLegato = 0.0;

    float modRest = 0.1f;
    float modLegato = 0.5f;
    float modEntropy = 0.0f;
    float modHarmony = 0.0f;
    float modChaos = 0.0f;
    float accumulatedPitchOffset = 0.0f;

    std::vector<int> lastChordPitches;

    SceneState presets[8];
    bool presetSlotsSaved[8] = { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};