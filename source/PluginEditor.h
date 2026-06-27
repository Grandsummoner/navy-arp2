#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Forward declaration of editor for LookAndFeel bindings
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
    ChromaCapsLookAndFeel (PluginProcessor& p, PluginEditor* e = nullptr) : processor (p), editor (e)
    {
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                           float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        auto knobBounds = bounds.reduced (16.0f);
        auto radius = juce::jmin (knobBounds.getWidth(), knobBounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX(), toY = bounds.getCentreY();

        int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (themeIdx);

        g.setColour (juce::Colour (themeIdx == 1 ? 0x25000000 : 0x45000000));
        g.fillEllipse (toX - radius + 2.0f, toY - radius + 4.0f, radius * 2.0f, radius * 2.0f);

        juce::Colour rubberBaseCol = (themeIdx == 1) ? juce::Colour (0xFF1E212A) : juce::Colour (0xFF1A1C24); 
        juce::ColourGradient grad (rubberBaseCol.brighter (0.12f), toX, toY - radius, rubberBaseCol.darker (0.25f), toX, toY + radius, false);
        g.setGradientFill (grad); g.fillEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        g.setColour (juce::Colour (0xFF2D313D)); g.drawEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Colour accentCol = (slider.getComponentID() == "rhythmMorph" || slider.getComponentID() == "rest" || 
                                  slider.getComponentID() == "legato" || slider.getComponentID() == "rate") ? t.leftAccent : t.rightAccent;

        juce::Path pointerPath;
        float pointerLength = radius * 0.95f, pointerThickness = radius * 0.16f;
        pointerPath.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.3f);

        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (angle).translated (toX, toY));
        g.setColour (accentCol); g.fillPath (pointerPath);
        g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (pointerPath, juce::PathStrokeType (0.7f)); 
        g.restoreState();

        float centerRadius = radius * 0.38f; g.setColour (rubberBaseCol.darker (0.5f)); g.fillEllipse (toX - centerRadius, toY - centerRadius, centerRadius * 2.0f, centerRadius * 2.0f);

        float ledRingRadius = radius + 9.5f; int numLeds = 15;
        juce::Colour ledActiveCol = accentCol; float visualValue = sliderPos; bool lfoActive = false;

        juce::String pId = slider.getComponentID();
        if (pId.isNotEmpty()) {
            int lfoRateVal = 0;
            if (pId == "rhythmMorph") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rhythmMorphLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeMorph : sliderPos;
            }
            else if (pId == "rest") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::restLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeRest : sliderPos;
            }
            else if (pId == "legato") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::legatoLfoRate.getParamID()));
                float baseLegato = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::legato.getParamID()));
                float rawLegato = (lfoRateVal > 0) ? processor.activeLegato : baseLegato;
                visualValue = (rawLegato - 0.1f) / 0.9f;
            }
            else if (pId == "rate") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rateLfoRate.getParamID()));
                float baseRate = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::rate.getParamID()));
                float rawRate = (lfoRateVal > 0) ? static_cast<float>(processor.activeRateIdx) : baseRate;
                visualValue = rawRate / 3.0f;
            }
            else if (pId == "entropy") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::entropyLfoRate.getParamID()));
                float baseEntropy = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::entropy.getParamID()));
                float rawEntropy = (lfoRateVal > 0) ? processor.activeEntropy : baseEntropy;
                visualValue = (rawEntropy + 1.0f) * 0.5f;
            }
            else if (pId == "harmony") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::harmonyLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeHarmony : sliderPos;
            }
            else if (pId == "chaos") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::chaosLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeChaos : sliderPos;
            }
            else if (pId == "octaves") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octavesLfoRate.getParamID()));
                float baseOctaves = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
                float rawOctaves = (lfoRateVal > 0) ? static_cast<float>(processor.activeOctavesVal) : baseOctaves;
                visualValue = (rawOctaves + 3.0f) / 6.0f;
            }
            lfoActive = (lfoRateVal > 0);
            if (lfoActive) ledActiveCol = (pId == "rhythmMorph" || pId == "rest" || pId == "legato" || pId == "rate") ? juce::Colour (0xFFFF00D2) : juce::Colour (0xFF9933FF);
        }

        for (int i = 0; i < numLeds; ++i) {
            float pct = static_cast<float>(i) / static_cast<float>(numLeds - 1);
            float ledAngle = rotaryStartAngle + pct * (rotaryEndAngle - rotaryStartAngle);
            float ledX = toX + ledRingRadius * std::sin (ledAngle), ledY = toY - ledRingRadius * std::cos (ledAngle);
            if (pct <= visualValue) {
                g.setColour (ledActiveCol); g.fillEllipse (ledX - 1.8f, ledY - 1.8f, 3.6f, 3.6f);
                g.setColour (ledActiveCol.withAlpha (0.22f)); g.drawEllipse (ledX - 3.2f, ledY - 3.2f, 6.4f, 6.4f, 1.2f);
            } else {
                g.setColour (t.unlitLed); g.fillEllipse (ledX - 1.5f, ledY - 1.5f, 3.0f, 3.0f);
            }
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    void drawVectorDice (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour pipColour)
    {
        auto diceFace = bounds.reduced (1.0f);
        g.setColour (pipColour.withAlpha (0.12f)); g.fillRoundedRectangle (diceFace, 2.0f);
        g.setColour (pipColour); g.drawRoundedRectangle (diceFace, 2.0f, 1.2f);
        
        float cx = diceFace.getCentreX(), cy = diceFace.getCentreY();
        float d = diceFace.getWidth() * 0.25f, r = 1.3f;
        
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.fillEllipse (cx - d - r, cy - d - r, r * 2.0f, r * 2.0f);
        g.fillEllipse (cx + d - r, cy - d - r, r * 2.0f, r * 2.0f);
        g.fillEllipse (cx - d - r, cy + d - r, r * 2.0f, r * 2.0f);
        g.fillEllipse (cx + d - r, cy + d - r, r * 2.0f, r * 2.0f);
    }

    PluginProcessor& processor;
    PluginEditor* editor;
};

class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p) : processor (p) { startTimerHz (30); }
    ~OledDisplay() override { stopTimer(); }

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

    // Direct access to private preset saving/loading indicators
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