#ifndef NAVY_ARP_EDITOR_H
#define NAVY_ARP_EDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// Custom DJ TechTools / Chroma Caps Style Rotary & Linear LookAndFeel [5]
// ==============================================================================
class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel()
    {
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                           float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (8.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();

        // 1. Subtle drop shadow beneath the rubber cap [5]
        g.setColour (juce::Colour (0x45000000));
        g.fillEllipse (toX - radius + 2.0f, toY - radius + 4.0f, radius * 2.0f, radius * 2.0f);

        // 2. Matte rubberized cylindrical body (3D Gradient for roundness) [5]
        juce::Colour rubberBaseCol = juce::Colour (0xFF1A1C24); 
        juce::ColourGradient grad (rubberBaseCol.brighter (0.12f), toX, toY - radius, 
                                   rubberBaseCol.darker (0.25f), toX, toY + radius, false);
        g.setGradientFill (grad);
        g.fillEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        // 3. Highlighted outer lip edge of the cap
        g.setColour (juce::Colour (0xFF2D313D));
        g.drawEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // 4. Colored rubber indicator pointer strip (Iconic Chroma Cap high-visibility neon needle) [5]
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto accentCol = slider.findColour (juce::Slider::rotarySliderFillColourId);

        juce::Path pointerPath;
        float pointerLength = radius * 0.95f;
        float pointerThickness = radius * 0.16f;
        pointerPath.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.3f);

        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (angle).translated (toX, toY));
        
        // Solid Neon Pointer [5]
        g.setColour (accentCol);
        g.fillPath (pointerPath);
        
        // Glossy Highlights on Pointer Cap [5]
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.strokePath (pointerPath, juce::PathStrokeType (0.7f)); 
        g.restoreState();

        // 5. Matte Center Cap dome [5]
        float centerRadius = radius * 0.38f;
        g.setColour (rubberBaseCol.darker (0.5f));
        g.fillEllipse (toX - centerRadius, toY - centerRadius, centerRadius * 2.0f, centerRadius * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillEllipse (toX - centerRadius, toY - centerRadius - 2.0f, centerRadius * 2.0f, centerRadius * 1.5f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Custom vertical mixer-style faders to match Chroma style [5]
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 4.0f;
            auto trackX = x + width * 0.5f - trackWidth * 0.5f;
            
            // 1. Draw Recessed Fader Slot/Track [5]
            g.setColour (juce::Colour (0xFF090A0D));
            g.fillRoundedRectangle (trackX, (float)y, trackWidth, (float)height, trackWidth * 0.5f);
            
            g.setColour (juce::Colour (0xFF242835));
            g.drawRoundedRectangle (trackX - 1.0f, (float)y, trackWidth + 2.0f, (float)height, trackWidth * 0.5f, 1.0f);

            // 2. Draw Chroma Fader Cap [5]
            float capWidth = juce::jmin (26.0f, width * 0.8f);
            float capHeight = 14.0f;
            float capX = x + width * 0.5f - capWidth * 0.5f;
            float capY = sliderPos - capHeight * 0.5f;

            // Fader shadow [5]
            g.setColour (juce::Colour (0x45000000));
            g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 2.0f);

            // Tactile Rubberized Fader Cap Body [5]
            juce::Colour capBaseCol = juce::Colour (0xFF1E212A);
            juce::ColourGradient capGrad (capBaseCol.brighter (0.1f), capX, capY,
                                         capBaseCol.darker (0.2f), capX, capY + capHeight, false);
            g.setGradientFill (capGrad);
            g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f);

            // Outer cap bezel highlights
            g.setColour (juce::Colour (0xFF3A3F4E));
            g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f, 1.0f);

            // Colored central neon strip indicator [5]
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            float stripeHeight = 2.0f;
            g.fillRect (capX + 2.0f, capY + capHeight * 0.5f - stripeHeight * 0.5f, capWidth - 4.0f, stripeHeight);
        }
        else
        {
            // Fallback for horizontal sliders
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }
};

// ==============================================================================
// Active OLED Display with lock-free atomic visual synchronization
// ==============================================================================
class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p) : processor (p) 
    {
        startTimerHz (30);
    }
    
    ~OledDisplay() override { stopTimer(); }

    void timerCallback() override { repaint(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xFF0A0A0C)); 
        g.setColour (juce::Colour (0xFF1A2B3C)); 
        g.drawRect (getLocalBounds().toFloat(), 2.0f);

        g.setColour (juce::Colour (0xFF00D2FF));
        g.setFont (juce::Font ("Consolas", 14.0f, juce::Font::bold));
        
        juce::String headerText = "NAVY-ARP GRAPHIC MONITOR";
        g.drawFittedText (headerText, getLocalBounds().removeFromTop (25), juce::Justification::centred, 1);

        // Real-time OLED Context Information
        juce::String scaleName = processor.apvts.getParameter(IDs::scaleType.getParamID())->getCurrentValueAsText();
        juce::String keyName = processor.apvts.getParameter(IDs::rootKey.getParamID())->getCurrentValueAsText();
        int extType = processor.activeChordExtensionType.load();
        juce::String extText = (extType == 0) ? "TRIAD" : (extType == 1) ? "SUS" : "7TH/9TH";

        juce::String speedRate = processor.apvts.getParameter(IDs::rate.getParamID())->getCurrentValueAsText();
        juce::String activeOcts = processor.apvts.getParameter(IDs::octaves.getParamID())->getCurrentValueAsText();

        g.setFont (juce::Font ("Consolas", 11.0f, juce::Font::plain));
        g.setColour (juce::Colour (0xFF888A90));
        g.drawText ("KEY: " + keyName + " | SCALE: " + scaleName + " | EXT: " + extText + " | RATE: " + speedRate + " | OCT: " + activeOcts, 
                    10, 25, getWidth() - 20, 15, juce::Justification::centred);

        // Grid Area Calculations
        auto area = getLocalBounds().reduced (15);
        area.removeFromTop (35); 
        
        // 1. Subtle horizontal level lines under the meters (25%, 50%, 75% thresholds)
        g.setColour (juce::Colour (0xFF141822));
        for (float pct : { 0.25f, 0.50f, 0.75f })
        {
            float gridY = area.getBottom() - 15.0f - (area.getHeight() - 15.0f) * pct * 0.75f;
            g.drawHorizontalLine (static_cast<int>(gridY), (float)area.getX(), (float)area.getRight());
        }

        int barWidth = area.getWidth() / 8;
        int spacing = 6;

        for (int i = 0; i < 8; ++i)
        {
            float faderProb = *processor.apvts.getRawParameterValue ("fader" + juce::String (i + 1));
            
            // Scaled down vertically to fit step markers neatly underneath the bars
            int barHeight = static_cast<int>((area.getHeight() - 15) * faderProb * 0.75f);
            
            juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, 
                                      area.getBottom() - barHeight - 15, 
                                      barWidth - (spacing * 2), 
                                      barHeight);

            bool isPlaying = processor.isCurrentlyPlayingUI.load();

            // 2. Draw dynamic meter bars
            if (i == processor.currentStep && isPlaying)
            {
                if (i == 0)      g.setColour (juce::Colour (0xFF4CFF4C)); // Beat 1: Green
                else if (i == 4) g.setColour (juce::Colour (0xFFFF4C4C)); // Beat 5: Red
                else             g.setColour (juce::Colour (0xFF00FFFF)); // Others: Cyan
                g.fillRect (bar.expanded(1, 1));
            }
            else
            {
                if (i % 3 == 0)      g.setColour (juce::Colour (0xFF00BCFF)); 
                else if (i % 4 == 0) g.setColour (juce::Colour (0xFF9966FF)); 
                else                 g.setColour (juce::Colour (0xFFFFAA00)); 
                g.fillRect (bar);
            }

            // 3. Symmetrical Dim Sequencer Step LED Labels [NEW]
            juce::String stepNumStr = juce::String (i + 1);
            g.setFont (juce::Font ("Consolas", 10.0f, juce::Font::bold));
            
            if (i == processor.currentStep && isPlaying)
            {
                if (i == 0)      g.setColour (juce::Colour (0xFF4CFF4C)); 
                else if (i == 4) g.setColour (juce::Colour (0xFFFF4C4C)); 
                else             g.setColour (juce::Colour (0xFF00FFFF));
            }
            else
            {
                g.setColour (juce::Colour (0xFF3A3F4E));
            }
            
            int numX = area.getX() + (i * barWidth);
            g.drawText (stepNumStr, numX, area.getBottom() - 12, barWidth, 12, juce::Justification::centred);
        }
    }

private:
    PluginProcessor& processor;
};

// ==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& event) override
    {
        for (int i = 0; i < 8; ++i)
        {
            if (event.eventComponent == &presetButtons[i])
            {
                if (event.mods.isRightButtonDown())
                {
                    processor.savePreset(i);
                    presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF003344));
                }
            }
        }
    }

private:
    PluginProcessor& processor;
    OledDisplay oledDisplay;
    ChromaCapsLookAndFeel chromaLookAndFeel; // Custom rubber-cap style controls [5]

    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    juce::Slider rhythmMorphKnob, restKnob, legatoKnob, rateKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle, rateTitle;

    juce::Slider entropyKnob, harmonyKnob, chaosKnob, octavesKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle, octavesTitle;

    juce::Slider morphCrossfader;

    juce::TextButton latchButton;
    juce::TextButton chordModeButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    juce::TextButton sceneAButton;
    juce::TextButton sceneBButton;
    juce::TextButton presetButtons[8];

    juce::ComboBox rootKeyBox;
    juce::ComboBox scaleTypeBox;
    juce::ComboBox cycleLengthBox;

    int presetPressStartTime[8] = { 0 };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader3Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader4Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader5Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader6Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader7Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fader8Attachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rhythmMorphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> restAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> legatoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> octavesAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chordModeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cycleLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

#endif // NAVY_ARP_EDITOR_H