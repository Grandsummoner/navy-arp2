#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "AppTheme.h"
#include "ChromaCapsLookAndFeel.h"
#include "OledDisplay.h"

class PluginEditor;

struct AppTheme
{
    juce::Colour background;
    juce::Colour border;
    juce::Colour leftAccent;
    juce::Colour rightAccent;
    juce::Colour textDim;
    juce::Colour trackBg;
    juce::Colour slotOutline;
    juce::Colour faderCap;
    juce::Colour unlitLed;

    static AppTheme get (int index)
    {
        AppTheme t;
        if (index == 1) // Skyline Eurorack (Light Panel)
        {
            t.background    = juce::Colour (0xFFE2E0D8);
            t.border        = juce::Colour (0xFFB8B5AB);
            t.leftAccent    = juce::Colour (0xFFFF3B30);
            t.rightAccent   = juce::Colour (0xFF3A3A38);
            t.textDim       = juce::Colour (0xFF1A1A18);
            t.trackBg       = juce::Colour (0xFFD4D1C9);
            t.slotOutline   = juce::Colour (0xFFA8A59C);
            t.faderCap      = juce::Colour (0xFF1E1E1E);
            t.unlitLed      = juce::Colour (0xFFB8B5AB);
        }
        else if (index == 2) // Monochrome Minimal
        {
            t.background    = juce::Colour (0xFF101010);
            t.border        = juce::Colour (0xFF242424);
            t.leftAccent    = juce::Colour (0xFFFFFFFF);
            t.rightAccent   = juce::Colour (0xFF88888A);
            t.textDim       = juce::Colour (0xFF66666A);
            t.trackBg       = juce::Colour (0xFF080808);
            t.slotOutline   = juce::Colour (0xFF2D2D32);
            t.faderCap      = juce::Colour (0xFF222222);
            t.unlitLed      = juce::Colour (0xFF1C1C1F);
        }
        else if (index == 3) // Matrix Terminal
        {
            t.background    = juce::Colour (0xFF030A05);
            t.border        = juce::Colour (0xFF112A18);
            t.leftAccent    = juce::Colour (0xFF33FF33);
            t.rightAccent   = juce::Colour (0xFF22AA22); 
            t.textDim       = juce::Colour (0xFF1E5F2E);
            t.trackBg       = juce::Colour (0xFF020603);
            t.slotOutline   = juce::Colour (0xFF0E451E);
            t.faderCap      = juce::Colour (0xFF112A18);
            t.unlitLed      = juce::Colour (0xFF0E1A11);
        }
        else // Default Navy Cyber
        {
            t.background    = juce::Colour (0xFF16181F);
            t.border        = juce::Colour (0xFF2A2E3D);
            t.leftAccent    = juce::Colour (0xFF00D2FF);
            t.rightAccent   = juce::Colour (0xFFFFB300);
            t.textDim       = juce::Colour (0xFF55555C);
            t.trackBg       = juce::Colour (0xFF181C24);
            t.slotOutline   = juce::Colour (0xFF242835);
            t.faderCap      = juce::Colour (0xFF1E212A);
            t.unlitLed      = juce::Colour (0xFF1C1E24);
        }
        return t;
    }
};

class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel (PluginProcessor& p, PluginEditor* e = nullptr);

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                           float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                           juce::Slider& slider) override;

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // Custom Textbox styling override for the floating readouts
    void drawTextBox (juce::Graphics& g, juce::Slider& slider, int x, int y, int width, int height) override;

private:
    void drawVectorDice (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour pipColour);

    PluginProcessor& processor;
    PluginEditor* editor;
};

class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p);
    ~OledDisplay() override;

    void timerCallback() override;
    void paint (juce::Graphics& g) override;

private:
    PluginProcessor& processor;
    int lastLfoRates[8] = { 0 }; float lastLfoDepths[8] = { 0.0f };
    int lfoOverlayTimer = 0, lfoActiveParamIdx = -1;
};

class PluginEditor : public juce::AudioProcessorEditor, public juce::Timer, public juce::AudioProcessorValueTreeState::Listener 
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

    juce::ComboBox rootKeyBox, scaleTypeBox, cycleLengthBox;

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment, scaleTypeAttachment, cycleLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};