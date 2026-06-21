#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace IDs 
{
    #define DECLARE_ID(name) const juce::ParameterID name (#name, 1)
    DECLARE_ID(fader1); DECLARE_ID(fader2); DECLARE_ID(fader3); DECLARE_ID(fader4);
    DECLARE_ID(fader5); DECLARE_ID(fader6); DECLARE_ID(fader7); DECLARE_ID(fader8);
    DECLARE_ID(rhythmMorph); DECLARE_ID(rest); DECLARE_ID(legato);
    DECLARE_ID(entropy); DECLARE_ID(harmony); DECLARE_ID(chaos);
    DECLARE_ID(morph); DECLARE_ID(latch);
    #undef DECLARE_ID
}

struct SceneState {
    float faders[8] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    float rhythmMorph = 0.0f;
    float rest = 0.1f;
    float legato = 0.5f;
    float entropy = 0.0f;
    float harmony = 0.0f;
    float chaos = 0.0f;
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
    bool hasEditor() const override;

    const juce::String PluginProcessor::getName() const override;
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

    void savePreset (int slotIndex);
    void loadPreset (int slotIndex);
    bool isPresetSaved (int slotIndex) const { return presetSlotsSaved[slotIndex]; }

    void diceMelody();
    void diceRhythm();

    void captureSceneA();
    void captureSceneB();

    void triggerArpStep (float stepProbability, float activeRest, float activeLegato, const std::vector<int>& notesToPlay, juce::MidiBuffer& processedMidi, double bpm);

    SceneState sceneA;
    SceneState sceneB;
    bool hasSceneA = false;
    bool hasSceneB = false;

    int currentStep = 0;
    std::vector<int> activeHeldNotes;
    std::vector<int> latchedNotes;
    bool isFirstNoteOfNewChord = true;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double mSampleRate = 44100.0;
    
    int mLastStep = -1;
    int mLastNotePlayed = -1;
    int mNoteOffTime = 0; 
    int mTimeInSamples = 0;
    
    SceneState presets[8];
    bool presetSlotsSaved[8] = { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};