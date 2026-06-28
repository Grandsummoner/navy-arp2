#include "OledDisplay.h"
#include "PluginProcessor.h"

OledDisplay::OledDisplay (PluginProcessor& p)
    : processor (p)
{
    // Initialize VU meters to dummy low levels
    leftVuLevel = 0.05f;
    rightVuLevel = 0.08f;
}

OledDisplay::~OledDisplay()
{
}

void OledDisplay::showParameterOverlay (const juce::String& paramName, float baseValue, const juce::String& lfoVibeText)
{
    activeParamName = paramName;
    activeParamValue = baseValue;
    activeLfoVibe = lfoVibeText;
    isOverlayActive = true;
    
    repaint();
    
    // Trigger / restart 1.5-second (1500ms) timer for overlay timeout
    startTimer (1500);
}

void OledDisplay::setFreezeActive (bool shouldBeActive)
{
    if (isFreezeActive != shouldBeActive)
    {
        isFreezeActive = shouldBeActive;
        repaint();
    }
}

void OledDisplay::timerCallback()
{
    isOverlayActive = false;
    stopTimer();
    repaint();
}

void OledDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Fill Screen Background (Obsidian Obsidian #0F1116)
    g.setColour (juce::Colour::fromString ("#FF0F1116"));
    g.fillRoundedRectangle (bounds, 4.0f);

    // 2. Draw Bezel with Dual Offset Lines (Inset Glass Look)
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
    g.drawVerticalLine (0, 0.0f, bounds.getHeight());

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int> (bounds.getHeight() - 1.0f), 0.0f, bounds.getWidth());
    g.drawVerticalLine (static_cast<int> (bounds.getWidth() - 1.0f), 0.0f, bounds.getHeight());

    auto displayArea = bounds.reduced (10.0f);

    if (isOverlayActive)
    {
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colours::white);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (activeParamName.toUpperCase(), displayArea.removeFromTop (18.0f), juce::Justification::left, true);

        displayArea.removeFromTop (4.0f);

        auto valueBarArea = displayArea.removeFromTop (12.0f);
        g.setColour (juce::Colours::darkgrey.withAlpha (0.3f));
        g.fillRoundedRectangle (valueBarArea, 2.0f);
        
        float fillWidth = valueBarArea.getWidth() * activeParamValue;
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour::fromString ("#FF2196F3"));
        g.fillRoundedRectangle (valueBarArea.withWidth (fillWidth), 2.0f);

        displayArea.removeFromTop (6.0f);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (9.5f, juce::Font::plain));
        
        juce::String valuePercentStr = "VAL: " + juce::String (static_cast<int> (activeParamValue * 100.0f)) + "%";
        g.drawText (valuePercentStr, displayArea.removeFromTop (12.0f), juce::Justification::left, true);

        g.drawText ("LFO: " + activeLfoVibe, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
    }
    else
    {
        g.setColour (juce::Colours::grey);
        g.setFont (juce::FontOptions (9.0f, juce::Font::plain));
        
        auto stepNumbersArea = displayArea.removeFromTop (12.0f);
        float stepWidth = stepNumbersArea.getWidth() / 8.0f;
        for (int i = 0; i < 8; ++i)
        {
            g.drawText (juce::String (i + 1), 
                        stepNumbersArea.removeFromLeft (stepWidth), 
                        juce::Justification::centred, true);
        }

        displayArea.removeFromTop (8.0f);

        float vuBarHeight = 6.0f;
        auto leftVuArea = displayArea.removeFromTop (vuBarHeight);
        displayArea.removeFromTop (4.0f);
        auto rightVuArea = displayArea.removeFromTop (vuBarHeight);

        g.setColour (juce::Colours::darkgrey.withAlpha (0.2f));
        g.fillRoundedRectangle (leftVuArea, 2.0f);
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour::fromString ("#FF4CAF50"));
        g.fillRoundedRectangle (leftVuArea.withWidth (leftVuArea.getWidth() * leftVuLevel), 2.0f);

        g.setColour (juce::Colours::darkgrey.withAlpha (0.2f));
        g.fillRoundedRectangle (rightVuArea, 2.0f);
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour::fromString ("#FF4CAF50"));
        g.fillRoundedRectangle (rightVuArea.withWidth (rightVuArea.getWidth() * rightVuLevel), 2.0f);

        leftVuLevel = juce::jlimit (0.01f, 0.99f, leftVuLevel + juce::Random::getSystemRandom().nextFloat() * 0.1f - 0.05f);
        rightVuLevel = juce::jlimit (0.01f, 0.99f, rightVuLevel + juce::Random::getSystemRandom().nextFloat() * 0.1f - 0.05f);
    }
}

void OledDisplay::resized()
{
}