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
    
    // Renders the coordinate grid and the floating CC indicators in MIDI Map mode
    void paintOverChildren (juce::Graphics& g) override;
    
    // Tracks real-time cursor coordinates
    void mouseMove (const juce::MouseEvent& event) override;
    
    void resized() override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

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

    // Repurposed Master Knobs
    juce::Slider masterVelocityKnob, masterSwingKnob;
    juce::Label masterVelocityTitle, masterSwingTitle;

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
    juce::ComboBox rootKeyBox, scaleTypeBox, cycleLengthBox, panelThemeBox;

    // Symmetrical Help Sidebar UI Toggle Members [3]
    juce::TextButton helpButton;
    bool isHelpPanelOpen = false;
    void toggleHelpPanel();

    // Symmetrical Sound Engine Sidebar UI Toggle Members [3]
    juce::TextButton soundButton; // Displays the music note "♫" glyph
    bool isLeftPanelOpen = false;
    void toggleLeftPanel();

    // Symmetrical Left Panel Sound Engine UI Controls [3]
    juce::ComboBox midiInBox, midiOutBox;
    juce::ComboBox audioRoutingBox;

    // Symmetrical Button Picks for Synthesis Models (Layerable toggles) [3]
    juce::TextButton v1AnalogBtn, v1FmBtn, v1StringBtn, v1PulseBtn;
    juce::TextButton v2AnalogBtn, v2FmBtn, v2StringBtn, v2PulseBtn;

    // Symmetrical Rotary Knobs for ADSR, Timbre, Delay, Reverb, and Volume [3]
    juce::Slider v1AttackKnob, v1DecayKnob, v1SustainKnob, v1ReleaseKnob;
    juce::Slider v1TimbreKnob, v1DelayKnob, v1ReverbKnob, v1VolumeKnob;

    juce::Slider v2AttackKnob, v2DecayKnob, v2SustainKnob, v2ReleaseKnob;
    juce::Slider v2TimbreKnob, v2DelayKnob, v2ReverbKnob, v2VolumeKnob;

    juce::Label midiInLabel, midiOutLabel;
    juce::Label voice1SynthLabel, voice1DecayLabel, voice1TimbreLabel, voice1ReverbLabel;
    juce::Label voice2SynthLabel, voice2DecayLabel, voice2TimbreLabel, voice2ReverbLabel;
    juce::Label audioRoutingLabel;

    // Public Flash Timers for LookAndFeel Animation Access
    int sceneAFlashTimer = 0;
    int sceneBFlashTimer = 0;
    int saveFlashTimer = 0;
    int recallFlashTimer = 0;
    int copyFlashTimer = 0;
    int initFlashTimer = 0;

    int presetFlashTimer[8] { 0 };
    int presetFlashType[8] { 0 };

    // Tracks the source preset slot index during copy operations
    int copySourcePresetIndex = -1;

private:
    void timerCallback() override;
    void updateSliderTextBoxThemeColors(); 

    // Background Image Container
    juce::Image backgroundImage;

    // Slide Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader1Attachment, fader2Attachment, fader3Attachment, fader4Attachment, fader5Attachment, fader6Attachment, fader7Attachment, fader8Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rhythmMorphAttachment, restAttachment, legatoAttachment, rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment, harmonyAttachment, chaosAttachment, octavesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;

    // Master slide attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVelocityAttachment, masterSwingAttachment;

    // Toggle Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment, arpSeqAttachment, polyButtonAttachment, freezeButtonAttachment;

    // Dropdown Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment, scaleTypeAttachment, cycleLengthAttachment, panelThemeAttachment;

    // Symmetrical Left Panel Parameter Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> midiInAttachment, midiOutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> audioRoutingAttachment;

    // Multi-Select Layerable Instrument attachments [3]
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> v1AnalogAttachment, v1FmAttachment, v1StringAttachment, v1PulseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> v2AnalogAttachment, v2FmAttachment, v2StringAttachment, v2PulseAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v1AttackAttachment, v1DecayAttachment, v1SustainAttachment, v1ReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v1TimbreAttachment, v1DelayAttachment, v1ReverbAttachment, v1VolumeAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v2AttackAttachment, v2DecayAttachment, v2SustainAttachment, v2ReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> v2TimbreAttachment, v2DelayAttachment, v2ReverbAttachment, v2VolumeAttachment;

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
    bool sceneBActiveState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};