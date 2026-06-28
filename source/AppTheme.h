#pragma once

#include <JuceHeader.h>

namespace AppTheme
{
    struct Theme
    {
        juce::Colour background;
        juce::Colour border;
        juce::Colour textDim;
        juce::Colour slotOutline;
    };

    inline Theme get (int index)
    {
        switch (index)
        {
            case 0: // Navy Cyber
                return {
                    juce::Colour::fromString ("#FF0A0B10"), // background
                    juce::Colour::fromString ("#FF00D2FF"), // border
                    juce::Colour::fromString ("#FF888888"), // textDim
                    juce::Colour::fromString ("#FF181C24")  // slotOutline
                };
            case 1: // Skyline Eurorack
                return {
                    juce::Colour::fromString ("#FF1E2127"),
                    juce::Colour::fromString ("#FFFFB300"),
                    juce::Colour::fromString ("#FF9E9E9E"),
                    juce::Colour::fromString ("#FF303030")
                };
            case 2: // Monochrome Minimal
                return {
                    juce::Colour::fromString ("#FF111111"),
                    juce::Colour::fromString ("#FFFFFFFF"),
                    juce::Colour::fromString ("#FF888888"),
                    juce::Colour::fromString ("#FF222222")
                };
            case 3: // Matrix Terminal
                return {
                    juce::Colour::fromString ("#FF050A05"),
                    juce::Colour::fromString ("#FF33FF33"),
                    juce::Colour::fromString ("#FF558855"),
                    juce::Colour::fromString ("#FF112211")
                };
            default:
                return {
                    juce::Colour::fromString ("#FF1E2127"),
                    juce::Colour::fromString ("#FFFFB300"),
                    juce::Colour::fromString ("#FF9E9E9E"),
                    juce::Colour::fromString ("#FF303030")
                };
        }
    }
}