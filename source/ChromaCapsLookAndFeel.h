#pragma once

#include <JuceHeader.h>

class PluginProcessor; // Forward declaration

class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel (PluginProcessor& p, juce::AudioProcessorEditor* editor);
    ~ChromaCapsLookAndFeel() override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider) override;
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

private:
    PluginProcessor& processor;
    juce::AudioProcessorEditor* parentEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChromaCapsLookAndFeel)
};