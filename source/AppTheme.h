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
        juce::Colour knobFillLeft;   // Custom left knob indicator color (no more bright neon blue)
        juce::Colour knobFillRight;  // Custom right knob indicator color
        juce::Colour faderCap;       // Custom deep Navy Blue fader cap color
    };

    inline Theme get (int index)
    {
        switch (index)
        {
            case 0: // Navy Cyber
                return {
                    juce::Colour::fromString ("#FF0A0B10"), // background (obsidian deep navy)
                    juce::Colour::fromString ("#FF5D8AA8"), // border (softer slate blue-grey instead of bright electric blue)
                    juce::Colour::fromString ("#FF888888"), // textDim
                    juce::Colour::fromString ("#FF181C24"), // slotOutline
                    juce::Colour::fromString ("#FFFFB300"), // knobFillLeft (Premium Amber/Gold indicator instead of neon cyan)
                    juce::Colour::fromString ("#FFFFB300"), // knobFillRight (Premium Amber/Gold)
                    juce::Colour::fromString ("#FF0A1D33")  // faderCap (Deep Navy Blue)
                };
            case 1: // Skyline Eurorack (Beige Faceplate)
                return {
                    juce::Colour::fromString ("#FFE8E4DB"), // background (classic warm eurorack beige)
                    juce::Colour::fromString ("#FF55555C"), // border (aluminum anthracite grey)
                    juce::Colour::fromString ("#FF4A4B50"), // textDim (high contrast dark grey for beige)
                    juce::Colour::fromString ("#FFC8C3BC"), // slotOutline
                    juce::Colour::fromString ("#FFFF5533"), // knobFillLeft (Accented Red-Orange matching the Skyline aesthetic)
                    juce::Colour::fromString ("#FFFF5533"), // knobFillRight (Accented Red-Orange)
                    juce::Colour::fromString ("#FF1B2838")  // faderCap (Clean Navy Blue on beige faceplate)
                };
            case 2: // Monochrome Minimal
                return {
                    juce::Colour::fromString ("#FF1E2127"), // background
                    juce::Colour::fromString ("#FFFFB300"), // border
                    juce::Colour::fromString ("#FF9E9E9E"), // textDim
                    juce::Colour::fromString ("#FF303030"), // slotOutline
                    juce::Colour::fromString ("#FFFFB300"), // knobFillLeft (Amber/Gold)
                    juce::Colour::fromString ("#FFFFB300"), // knobFillRight (Amber/Gold)
                    juce::Colour::fromString ("#FF0A1D33")  // faderCap (Deep Navy Blue)
                };
            case 3: // Matrix Terminal
                return {
                    juce::Colour::fromString ("#FF030803"), // background
                    juce::Colour::fromString ("#FF33FF33"), // border
                    juce::Colour::fromString ("#FF558855"), // textDim
                    juce::Colour::fromString ("#FF112211"), // slotOutline
                    juce::Colour::fromString ("#FF33FF33"), // knobFillLeft (Matrix Green)
                    juce::Colour::fromString ("#FF33FF33"), // knobFillRight (Matrix Green)
                    juce::Colour::fromString ("#FF051505")  // faderCap (Very Dark Greenish Navy)
                };
            default:
                return {
                    juce::Colour::fromString ("#FFE5E0D8"), // background
                    juce::Colour::fromString ("#FF55555C"), // border
                    juce::Colour::fromString ("#FF3A3A3D"), // textDim
                    juce::Colour::fromString ("#FFC4BCB4"), // slotOutline
                    juce::Colour::fromString ("#FFFF5533"), // knobFillLeft (Skyline default Red-Orange)
                    juce::Colour::fromString ("#FFFF5533"), // knobFillRight (Skyline default Red-Orange)
                    juce::Colour::fromString ("#FF1B2838")  // faderCap (Deep Navy Blue)
                };
        }
    }
}