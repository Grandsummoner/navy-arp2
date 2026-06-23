#ifndef NAVY_ARP_EDITOR_H
#define NAVY_ARP_EDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// Active OLED Display with Beat-division visual color coding
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
            bool isLatchActive = *processor.apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;
            bool isPlaying = isLatchActive ? !processor.latchedNotes.empty() : !processor.activeHeldNotes.empty();

            if (i == processor.currentStep && isPlaying)
            {
                // Color-code the active beat division to prevent gaudy layout
                if (i == 0)      g.setColour (juce::Colour (0xFF33FF33)); // Beat 1: Emerald Green
                else if (i == 4) g.setColour (juce::Colour (0xFFFF3333)); // Beat 2: Ruby Red
                else             g.setColour (juce::Colour (0xFF00FFFF)); // Others: Electric Blue
                g.fillRect (bar.expanded(2, 2));
            }
            else
            {
                if (i % 3 == 0)      g.setColour (juce::Colour (0xFF00D2FF)); // Aqua
                else if (i % 4 == 0) g.setColour (juce::Colour (0xFFB080FF)); // Lavender
                else                 g.setColour (juce::Colour (0xFFFFB300)); // Amber
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

    // Custom Right-Click Preset Saving callback
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

    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    // Rotary Knobs and static Titles
    juce::Slider rhythmMorphKnob, restKnob, legatoKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle;

    juce::Slider entropyKnob, harmonyKnob, chaosKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle;

    juce::Slider morphCrossfader;

    juce::TextButton latchButton;
    juce::TextButton chordModeButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    juce::TextButton sceneAButton;
    juce::TextButton sceneBButton;
    juce::TextButton presetButtons[8];

    // Key & Scale Dropdowns (Positioned neatly inside the central OLED)
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

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chordModeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cycleLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

#endif // NAVY_ARP_EDITOR_H