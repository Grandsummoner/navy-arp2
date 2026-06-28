#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "AppTheme.h"
#include "ChromaCapsLookAndFeel.h"
#include "OledDisplay.h"

class PluginEditor : public juce::AudioProcessorEditor, 
                     public juce::Timer,
                     public juce::AudioProcessorValueTreeState::Listener 
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    // Grant look and feel engine direct access to private animation timers
    friend class ChromaCapsLookAndFeel;

private:
    PluginProcessor& processor;
    OledDisplay oledDisplay;
    ChromaCapsLookAndFeel chromaLookAndFeel; 

    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;
    juce::Slider rhythmMorphKnob, restKnob, legatoKnob, rateKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle, rateTitle;
    juce::Slider entropyKnob, harmonyKnob, chaosKnob, octavesKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle, octavesTitle;
    juce::Slider morphCrossfader;

    juce::TextButton latchButton, arpSeqButton, polyButton, freezeButton;
    juce::TextButton sceneAButton, sceneBButton;
    juce::TextButton saveButton, recallButton, copyButton, initButton;
    juce::TextButton diceMeloButton, diceArtiButton, diceTimeButton, diceNavyButton;
    juce::TextButton presetButtons[8]; 

    // Added midiChannelBox
    juce::ComboBox rootKeyBox, scaleTypeBox, cycleLengthBox, midiChannelBox; 

    uint32_t sceneAPressStartTime = 0, sceneBPressStartTime = 0;
    bool sceneAAlreadySaved = false, sceneBAlreadySaved = false;
    int sceneAFlashTimer = 0, sceneBFlashTimer = 0;

    uint32_t presetPressStartTime[8] = { 0 };
    bool presetAlreadySaved[8] = { false };
    int presetFlashTimer[8] = { 0 }, presetFlashType[8] = { 0 }; 

    uint32_t savePressStartTime = 0, recallPressStartTime = 0, copyPressStartTime = 0, initPressStartTime = 0;
    bool saveAlreadySaved = false, recallAlreadySaved = false, copyAlreadySaved = false, initAlreadySaved = false;
    int saveFlashTimer = 0, recallFlashTimer = 0, copyFlashTimer = 0, initFlashTimer = 0;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader1Attachment, fader2Attachment, fader3Attachment, fader4Attachment, fader5Attachment, fader6Attachment, fader7Attachment, fader8Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rhythmMorphAttachment, restAttachment, legatoAttachment, rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment, harmonyAttachment, chaosAttachment, octavesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment, arpSeqAttachment, polyAttachment, freezeAttachment;
    
    // Added midiChannelAttachment
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment, scaleTypeAttachment, cycleLengthAttachment, midiChannelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};