#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdint> // Standard integer types
#include "PluginProcessor.h"

// ==============================================================================
// Active OLED Display
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
        g.fillAll (juce::Colour (0xFF000000));
        g.setColour (juce::Colour (0xFF112233));
        g.drawRect (getLocalBounds().toFloat(), 1.5f);

        g.setColour (juce::Colour (0xFF00D2FF));
        g.setFont (juce::FontOptions ("Consolas", 14.0f, juce::Font::bold));
        
        juce::String headerText = "--- NAVY-ARP OLED ACTIVE ---";
        g.drawFittedText (headerText, getLocalBounds().removeFromTop (30), juce::Justification::centred, 1);

        // Real-time Step VU-meter pulse lines
        auto area = getLocalBounds().reduced (15);
        area.removeFromTop (30);
        int barWidth = area.getWidth() / 8;
        int spacing = 4;

        for (int i = 0; i < 8; ++i)
        {
            float probability = *processor.apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
            int barHeight = static_cast<int>(area.getHeight() * probability * 0.7f);
            
            juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, 
                                      area.getBottom() - barHeight - 15, 
                                      barWidth - (spacing * 2), 
                                      barHeight);

            // Bright Sharp Neon-Glow pulse on the active step
            if (i == processor.currentStep && (! processor.activeHeldNotes.empty() || *processor.apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f))
            {
                g.setColour (juce::Colour (0xFF00FFFF)); // Highlight electric blue
                g.fillRect (bar.expanded(2, 2));
            }
            else
            {
                if (i % 3 == 0) g.setColour (juce::Colour (0xFF00D2FF)); // Aqua
                else if (i % 4 == 0) g.setColour (juce::Colour (0xFFB080FF)); // Lavender
                else g.setColour (juce::Colour (0xFFFFB300)); // Amber
                g.fillRect (bar);
            }
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

private:
    PluginProcessor& processor;
    OledDisplay oledDisplay;

    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    juce::Slider rhythmMorphKnob, restKnob, legatoKnob;
    juce::Slider entropyKnob, harmonyKnob, chaosKnob;
    juce::Slider morphCrossfader;

    juce::TextButton latchButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    juce::TextButton sceneAButton;
    juce::TextButton sceneBButton;
    juce::TextButton presetButtons[8];

    // Standard cross-platform integer array (prevents MSVC parser errors)
    uint32_t presetPressStartTime[8] = { 0 };

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

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment;
    s