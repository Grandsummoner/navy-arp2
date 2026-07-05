#pragma once

#include <JuceHeader.h>

class PluginProcessor; // Forward declaration

class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel (PluginProcessor& p, juce::AudioProcessorEditor* editor);
    ~ChromaCapsLookAndFeel() override;

    // Overridden to draw custom hardware text dynamically inside the empty buttons
    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // Overridden to draw the 3D-beveled button cap and unlit/lit dynamic LED indicators
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // Overridden for vertical upfaders and the horizontal crossfader centerpiece
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider) override;
    
    // Overridden to align vector needles and LEDs with the new 1000x680 panel coordinates
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

private:
    PluginProcessor& processor;
    juce::AudioProcessorEditor* parentEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChromaCapsLookAndFeel)
};