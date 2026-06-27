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
    DECLARE_ID(rate); DECLARE_ID(octaves); DECLARE_ID(panelTheme);

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
    float rhythmMorph = 0.0f, rest = 0.1f, legato = 0.5f, rate = 2.0f;
    float entropy = 0.0f, harmony = 0.0f, chaos = 0.0f, octaves = 0.0f;
    int lfoRates[8] = { 0 }; float lfoDepths[8] = { 0.0f };
};

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Boilerplate overrides inlined directly to reduce file sizes
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; } 
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    const juce::String getProgramName (int index) override { juce::ignoreUnused (index); return {}; }
    void changeProgramName (int index, const juce::String& newName) override { juce::ignoreUnused (index, newName); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void saveSceneA() { captureScene (0); }
    void saveSceneB() { captureScene (1); }
    void clearSceneA() { hasSceneA = false; }
    void clearSceneB() { hasSceneB = false; }

    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    bool isPresetSaved (int slotIndex) const { return presetSlotsSaved[slotIndex]; }

    void setActiveAnchor (bool useSceneB);
    void captureActiveParametersToActiveScene();

    SceneState sceneAPresets[8], sceneBPresets[8];
    bool sceneASlotsSaved[8] = { false }, sceneBSlotsSaved[8] = { false };
    SceneState sceneA, sceneB;
    bool hasSceneA = false, hasSceneB = false;

    std::atomic<int> activePresetIndex { 0 }; 
    std::atomic<bool> isSceneBActiveAnchor { false }; 

    void diceMelody(); void diceArticulation(); void diceTime(); void diceNavy();
    void diceActiveSceneA(); void diceActiveSceneB(); void resetAccumulator(); void resetRhythm();
    void triggerDiatonicChordPad (int padIndex);

    int currentStep = 0, currentBarInCycle = 1;
    std::atomic<bool> isCurrentlyPlayingUI { false };
    std::atomic<int> activeChordExtensionType { 0 }; 

    float activeMorph = 0.0f, activeRest = 0.1f, activeLegato = 0.5f, activeEntropy = 0.0f, activeHarmony = 0.0f, activeChaos = 0.0f;
    int activeRateIdx = 2, activeOctavesVal = 0;

    std::vector<int> activeHeldNotes, latchedNotes;
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

    double lfoPhases[8] = { 0.0 }, lfoPhaseEntropy = 0.0, lfoPhaseChaos = 0.0, lfoPhaseMorph = 0.0, lfoPhaseLegato = 0.0;
    float modRest = 0.1f, modLegato = 0.5f, modEntropy = 0.0f, modHarmony = 0.0f, modChaos = 0.0f, accumulatedPitchOffset = 0.0f;
    std::vector<int> lastChordPitches;

    SceneState presets[8];
    bool presetSlotsSaved[8] = { false };
    float currentSlewTarget[24] = { 0.0f }, currentSlewValue[24] = { 0.0f };
    bool lastSceneBActiveState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};