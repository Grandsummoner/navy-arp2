#pragma once

#include <JuceHeader.h>

class PluginProcessor; // Forward declaration

class OledDisplay : public juce::Component,
                    private juce::Timer
{
public:
    OledDisplay (PluginProcessor& p);
    ~OledDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Triggered when any parameter is adjusted in the editor
    void showParameterOverlay (const juce::String& paramName, float baseValue, const juce::String& lfoVibeText);

    // Track active freeze visual state
    void setFreezeActive (bool shouldBeActive);

private:
    void timerCallback() override;

    PluginProcessor& processor;

    bool isOverlayActive = false;
    bool isFreezeActive = false;

    juce::String activeParamName;
    float activeParamValue = 0.0f;
    juce::String activeLfoVibe;

    float leftVuLevel = 0.0f;
    float rightVuLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OledDisplay)
};