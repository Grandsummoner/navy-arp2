#include "OledDisplay.h"

OledDisplay::OledDisplay (PluginProcessor& p) : processor (p) 
{ 
    startTimerHz (30); 
}

OledDisplay::~OledDisplay() 
{ 
    stopTimer(); 
}

void OledDisplay::timerCallback()
{
    // Handle momentary parameter card overlay timer decay [NEW]
    if (overlayTimer > 0)
    {
        overlayTimer--;
        repaint();
    }
}

void OledDisplay::showParameter (const juce::String& paramName, float value, const juce::String& lfoName)
{
    activeParamName = paramName;
    activeParamValue = value;
    activeLfoPresetName = lfoName;
    overlayTimer = 45; // 1.5 seconds at 30Hz
    repaint();
}

void OledDisplay::paint (juce::Graphics& g)
{
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);
    g.fillAll (juce::Colour (0xFF08080A)); g.setColour (t.border); g.drawRect (getLocalBounds().toFloat(), 2.0f);
    auto area = getLocalBounds().reduced (15);

    // 1. Draw Momentary Parameter Card Overlay if active [NEW]
    if (overlayTimer > 0)
    {
        g.setColour (juce::Colour (0xFF181C24)); // Dark-charcoal backing card
        g.fillRoundedRectangle (area.toFloat().reduced(5.0f), 4.0f);
        g.setColour (t.leftAccent);
        g.drawRoundedRectangle (area.toFloat().reduced(5.0f), 4.0f, 1.2f);
        
        // Parameter Name Header
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        g.drawText ("[" + activeParamName.toUpperCase() + "]", area.getX(), area.getY() + 15, area.getWidth(), 25, juce::Justification::centred);

        // Parameter Live Value
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.setColour (juce::Colour (0xFFFFB300)); // Saturated Amber
        juce::String valStr = juce::String (activeParamValue, 2);
        if (activeParamName == "Rate")
        {
            int rateVal = static_cast<int> (activeParamValue);
            valStr = (rateVal == 0) ? "1/4" : (rateVal == 1) ? "1/8" : (rateVal == 2) ? "1/16" : "1/32";
        }
        else if (activeParamName == "Octaves")
        {
            int octVal = static_cast<int> (activeParamValue);
            valStr = (octVal >= 0) ? "+" + juce::String (octVal) : juce::String (octVal);
        }
        g.drawText ("Value: " + valStr, area.getX(), area.getY() + 45, area.getWidth(), 20, juce::Justification::centred);

        // Active LFO Vibe Preset Name
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.setColour (juce::Colour (0xFF80D8FF)); // Ice Blue
        g.drawText ("LFO Preset: " + activeLfoPresetName, area.getX(), area.getY() + 72, area.getWidth(), 20, juce::Justification::centred);

        // Diagonal glass glare overlay
        juce::Path glint; glint.startNewSubPath (0.0f, 0.0f); glint.lineTo (static_cast<float>(getWidth() * 0.45f), 0.0f); glint.lineTo (0.0f, static_cast<float>(getHeight() * 0.95f)); glint.closeSubPath();
        juce::ColourGradient gr (juce::Colours::white.withAlpha (0.02f), 0.0f, 0.0f, juce::Colours::white.withAlpha (0.0f), static_cast<float>(getWidth() * 0.3f), static_cast<float>(getHeight() * 0.6f), false);
        g.setGradientFill (gr); g.fillPath (glint);
        return;
    }

    // 2. Standard View (8-Step VU Meters & Display)
    auto headerArea = getLocalBounds().removeFromTop (25); g.setColour (juce::Colour (0xFF181C24)); g.fillRect (headerArea);
    g.setColour (juce::Colour (0xFFFFFFFF)); g.setFont (juce::Font (juce::FontOptions (12.5f).withStyle ("Bold")));
    g.drawText ("NAVY-ARP MONITOR", headerArea, juce::Justification::centred, true);

    juce::String scaleName = processor.apvts.getParameter(IDs::scaleType.getParamID())->getCurrentValueAsText();
    juce::String keyName = processor.apvts.getParameter(IDs::rootKey.getParamID())->getCurrentValueAsText();
    juce::String speedRate = processor.apvts.getParameter(IDs::rate.getParamID())->getCurrentValueAsText();
    int octValue = static_cast<int>(*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
    juce::String activeOcts = (octValue >= 0) ? "+" + juce::String (octValue) : juce::String (octValue);
    g.setFont (juce::Font (juce::FontOptions (9.5f))); g.setColour (juce::Colour (0xFFFFB300));
    g.drawText ("KEY: " + keyName + " | SCALE: " + scaleName + " | VOICE: TRIAD | RATE: " + speedRate + " | OCT: " + activeOcts, 10, 27, getWidth() - 20, 15, juce::Justification::centred);

    area.removeFromTop (27);
    int barWidth = area.getWidth() / 8, spacing = 6;
    for (int i = 0; i < 8; ++i) {
        float faderProb = *processor.apvts.getRawParameterValue ("fader" + juce::String (i + 1));
        int stepSegments = static_cast<int>((area.getHeight() - 20) * faderProb * 0.75f) / 8;
        juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, area.getBottom() - (stepSegments * 8) - 15, barWidth - (spacing * 2), stepSegments * 8);
        
        bool isPlaying = processor.isCurrentlyPlayingUI.load();
        for (int seg = 0; seg < stepSegments; ++seg) {
            int segY = bar.getBottom() - (seg * 8) - 6;
            g.setColour ((isPlaying && processor.currentStep == i) ? t.leftAccent : t.leftAccent.withAlpha (0.4f));
            g.fillRect (bar.getX(), segY, bar.getWidth(), 6);
        }
        juce::String stepNumStr = juce::String (i + 1); g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.setColour ((isPlaying && processor.currentStep == i) ? juce::Colours::white : juce::Colour (0xFF3A3F4E));
        g.drawText (stepNumStr, area.getX() + (i * barWidth), area.getBottom() - 12, barWidth, 12, juce::Justification::centred);
    }
    juce::Path glint; glint.startNewSubPath (0.0f, 0.0f); glint.lineTo (static_cast<float>(getWidth() * 0.45f), 0.0f); glint.lineTo (0.0f, static_cast<float>(getHeight() * 0.95f)); glint.closeSubPath();
    juce::ColourGradient gr (juce::Colours::white.withAlpha (0.02f), 0.0f, 0.0f, juce::Colours::white.withAlpha (0.0f), static_cast<float>(getWidth() * 0.3f), static_cast<float>(getHeight() * 0.6f), false);
    g.setGradientFill (gr); g.fillPath (glint);
}