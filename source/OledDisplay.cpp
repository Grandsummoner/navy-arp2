#include "OledDisplay.h"
#include "PluginProcessor.h"

OledDisplay::OledDisplay (PluginProcessor& p)
    : processor (p)
{
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
    startTimer (1500); // 1.5 second display timeout
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

    // 1. Fill Screen Background (Obsidian #0F1116)
    g.setColour (juce::Colour::fromString ("#FF0F1116"));
    g.fillRoundedRectangle (bounds, 4.0f);

    // 2. Draw Bezel with Dual Offset Lines (Inset Glass Look)
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawHorizontalLine (0, 0.0f, bounds.getWidth());
    g.drawVerticalLine (0, 0.0f, bounds.getHeight());

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (static_cast<int> (bounds.getHeight() - 1.0f), 0.0f, bounds.getWidth());
    g.drawVerticalLine (static_cast<int> (bounds.getWidth() - 1.0f), 0.0f, bounds.getHeight());

    auto displayArea = bounds.reduced (12.0f);

    if (isOverlayActive)
    {
        // =====================================================================
        // RENDER: MOMENTARY PARAMETER OVERLAY SCREEN (ACTIVE ONLY ON INTERACTION)
        // =====================================================================
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour (0xFFFF5533));
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (activeParamName.toUpperCase(), displayArea.removeFromTop (18.0f), juce::Justification::left, true);

        displayArea.removeFromTop (4.0f);

        auto valueBarArea = displayArea.removeFromTop (12.0f);
        g.setColour (juce::Colours::darkgrey.withAlpha (0.3f));
        g.fillRoundedRectangle (valueBarArea, 2.0f);
        
        float fillWidth = valueBarArea.getWidth() * activeParamValue;
        g.setColour (isFreezeActive ? juce::Colour::fromString ("#FF80D8FF") : juce::Colour (0xFFFF5533));
        g.fillRoundedRectangle (valueBarArea.withWidth (fillWidth), 2.0f);

        displayArea.removeFromTop (6.0f);

        g.setColour (juce::Colours::lightgrey);
        g.setFont (juce::FontOptions (9.5f, juce::Font::plain));
        
        juce::String valuePercentStr = "VALUE: " + juce::String (static_cast<int> (activeParamValue * 100.0f)) + "%";
        g.drawText (valuePercentStr, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
        g.drawText ("LFO: " + activeLfoVibe, displayArea.removeFromTop (12.0f), juce::Justification::left, true);
    }
    else
    {
        // =====================================================================
        // RENDER: HIGH-CONTRAST METADATA HEADER & TITLE
        // =====================================================================
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("NAVY-ARP MONITOR", displayArea.removeFromTop (16.0f), juce::Justification::centred, true);

        displayArea.removeFromTop (3.0f);

        // Fetch scale, root key, voice, rates, and octaves choices dynamically from the APVTS parameters
        int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rootKey.getParamID())));
        juce::StringArray keys { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, scales { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" };
        juce::String keyStr = keys[rootKeyIdx];

        int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::scaleType.getParamID())));
        juce::String scaleStr = scales[scaleIdx];

        bool isPolyActive = *processor.apvts.getRawParameterValue (IDs::poly.getParamID()) > 0.5f;
        float currentHarmony = *processor.apvts.getRawParameterValue (IDs::harmony.getParamID());
        juce::String voiceStr = "MONO";
        if (isPolyActive)
        {
            if (currentHarmony >= 0.25f && currentHarmony < 0.5f) voiceStr = "DUO";
            else if (currentHarmony >= 0.5f) voiceStr = "TRIAD";
            else voiceStr = "MONO";
        }

        int rateIdx = juce::jlimit (0, 3, static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rate.getParamID())));
        juce::StringArray rates { "1/4", "1/8", "1/16", "1/32" };
        juce::String rateStr = rates[rateIdx];

        int rangeShift = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
        juce::String octStr = (rangeShift >= 0 ? "+" : "") + juce::String (rangeShift);

        juce::String metaText = "KEY: " + keyStr + " | SCALE: " + scaleStr + " | VOICE: " + voiceStr + " | RATE: " + rateStr + " | OCT: " + octStr;
        
        g.setColour (juce::Colour (0xFFFFB300)); // Gold/Yellow metadata font
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (metaText, displayArea.removeFromTop (12.0f), juce::Justification::centred, true);

        // =====================================================================
        // RENDER: 8 SEGMENTED LED LADDERS (STEP LEVEL MONITOR)
        // =====================================================================
        const float spacing = 6.0f;
        const float colWidth = (displayArea.getWidth() - (7.0f * spacing)) / 8.0f;
        const int activeStep = processor.currentStep.load(); // Thread-safe atomic access
        const bool isPlaying = processor.isCurrentlyPlayingUI.load();

        const float morphVal = *processor.apvts.getRawParameterValue (IDs::morph.getParamID());
        const int numSegments = 16;
        const float segmentHeight = 6.0f;      // Chunky, bold hardware-style LEDs
        const float segmentSpacing = 2.0f;
        const float maxLaddersHeight = (numSegments * segmentHeight) + ((numSegments - 1) * segmentSpacing);

        // Position fader monitor area to sit right above the bottom step numbers
        float fadersY = bounds.getHeight() - maxLaddersHeight - 24.0f;
        auto laddersArea = juce::Rectangle<float> (displayArea.getX(), fadersY, displayArea.getWidth(), maxLaddersHeight);

        for (int i = 0; i < 8; ++i)
        {
            auto colBounds = laddersArea.removeFromLeft (colWidth);
            
            // Fetch morphed fader level for this step dynamically
            float faderVal = (processor.sceneA.faders[i] * (1.0f - morphVal)) + (processor.sceneB.faders[i] * morphVal);
            int activeSegments = static_cast<int> (std::round (faderVal * static_cast<float> (numSegments)));

            const bool isActiveStep = isPlaying && (i == activeStep);

            // Draw individual segmented horizontal blocks bottom-up
            for (int seg = 0; seg < numSegments; ++seg)
            {
                float segY = colBounds.getY() + maxLaddersHeight - ((seg + 1) * (segmentHeight + segmentSpacing));
                auto segmentRect = juce::Rectangle<float> (colBounds.getX() + 2.0f, segY, colBounds.getWidth() - 4.0f, segmentHeight);

                if (seg < activeSegments)
                {
                    // Active filled segments
                    if (isActiveStep)
                        g.setColour (juce::Colour (0xFFFF4500)); // Glowing orange-red playhead
                    else
                        g.setColour (juce::Colour (0xFF441105)); // Muted dim red/brown
                }
                else
                {
                    // Unfilled empty segments
                    g.setColour (juce::Colour (0xFF111317));
                }

                g.fillRect (segmentRect);
            }

            // Draw small step number indicator sitting exactly at the bottom screen boundary
            float textY = colBounds.getY() + maxLaddersHeight + 4.0f;
            auto stepNumRect = juce::Rectangle<float> (colBounds.getX(), textY, colBounds.getWidth(), 12.0f);
            
            if (isActiveStep)
            {
                g.setColour (juce::Colour (0xFFFF4500));
                g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
            }
            else
            {
                g.setColour (juce::Colours::grey.withAlpha (0.6f));
                g.setFont (juce::FontOptions (9.0f, juce::Font::plain));
            }

            g.drawText (juce::String (i + 1), stepNumRect, juce::Justification::centred, true);

            laddersArea.removeFromLeft (spacing); // Advance column offset
        }
    }
}

void OledDisplay::resized()
{
}