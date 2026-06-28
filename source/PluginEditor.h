#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ChromaCapsLookAndFeel.h"
#include "OledDisplay.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::AudioProcessorValueTreeState::Listener,
                     private juce::Timer
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;

    PluginProcessor& processor;
    OledDisplay oledDisplay;
    ChromaCapsLookAndFeel chromaLookAndFeel;

    // Matrix sliders and label elements
    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    // Left sidebar rotary components
    juce::Slider rhythmMorphKnob, restKnob, legatoKnob, rateKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle, rateTitle;

    // Right sidebar rotary components
    juce::Slider entropyKnob, harmonyKnob, chaosKnob, octavesKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle, octavesTitle;

    juce::Slider morphCrossfader;

    // Performance Mode Modifiers
    juce::TextButton latchButton, arpSeqButton, polyButton, freezeButton;
    juce::TextButton sceneAButton, sceneBButton;
    juce::TextButton saveButton, recallButton, copyButton, initButton;

    // Vector Dice buttons
    juce::TextButton diceMeloButton, diceArtiButton, diceTimeButton, diceNavyButton;

    // Preset Matrix Switches
    juce::TextButton presetButtons[8];

    // Top Control bar Dropdowns
    juce::ComboBox rootKeyBox, scaleTypeBox, cycleLengthBox;

    // Slide Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader1Attachment, fader2Attachment, fader3Attachment, fader4Attachment, fader5Attachment, fader6Attachment, fader7Attachment, fader8Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rhythmMorphAttachment, restAttachment, legatoAttachment, rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment, harmonyAttachment, chaosAttachment, octavesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;

    // Toggle Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment, arpSeqAttachment, polyAttachment, freezeAttachment;

    // Dropdown Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment, scaleTypeAttachment, cycleLengthAttachment;

    // Timer Flash Variables
    int sceneAFlashTimer = 0;
    int sceneBFlashTimer = 0;
    int saveFlashTimer = 0;
    int recallFlashTimer = 0;
    int copyFlashTimer = 0;
    int initFlashTimer = 0;

    int presetFlashTimer[8] { 0 };
    int presetFlashType[8] { 0 };

    // Timing States
    std::uint32_t presetPressStartTime[8] { 0 };
    bool presetAlreadySaved[8] { false };

    std::uint32_t savePressStartTime = 0;
    bool saveAlreadySaved = false;

    std::uint32_t recallPressStartTime = 0;
    bool recallAlreadySaved = false;

    std::uint32_t copyPressStartTime = 0;
    bool copyAlreadySaved = false;

    std::uint32_t initPressStartTime = 0;
    bool initAlreadySaved = false;

    std::uint32_t sceneAPressStartTime = 0;
    bool sceneAAlreadySaved = false;

    std::uint32_t sceneBPressStartTime = 0;
    bool sceneBAlreadySaved = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};