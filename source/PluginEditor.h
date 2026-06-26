#ifndef NAVY_ARP_EDITOR_H
#define NAVY_ARP_EDITOR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// Dynamically Compiled Global Color Themes Structure [NEW]
// ==============================================================================
struct AppTheme
{
    juce::Colour background;
    juce::Colour border;
    juce::Colour leftAccent;
    juce::Colour rightAccent;
    juce::Colour textDim;
    juce::Colour trackBg;
    juce::Colour slotOutline;
    juce::Colour faderCap;
    juce::Colour unlitLed;

    static AppTheme get (int index)
    {
        AppTheme t;
        if (index == 1) // Skyline Eurorack (Light Beige Panel) [5]
        {
            t.background    = juce::Colour (0xFFE2E0D8);
            t.border        = juce::Colour (0xFFB8B5AB);
            t.leftAccent    = juce::Colour (0xFFFF3B30); // Eurorack Red LED
            t.rightAccent   = juce::Colour (0xFF3A3A38); // Matte Charcoal
            t.textDim       = juce::Colour (0xFF1A1A18); // Calibrated dark grey for high contrast [NEW]
            t.trackBg       = juce::Colour (0xFFD4D1C9);
            t.slotOutline   = juce::Colour (0xFFA8A59C);
            t.faderCap      = juce::Colour (0xFF1E1E1E);
            t.unlitLed      = juce::Colour (0xFFB8B5AB);
        }
        else if (index == 2) // Monochrome Minimal
        {
            t.background    = juce::Colour (0xFF101010);
            t.border        = juce::Colour (0xFF242424);
            t.leftAccent    = juce::Colour (0xFFFFFFFF); // Clean White
            t.rightAccent   = juce::Colour (0xFF88888A); // Mid Grey
            t.textDim       = juce::Colour (0xFF66666A);
            t.trackBg       = juce::Colour (0xFF080808);
            t.slotOutline   = juce::Colour (0xFF2D2D32);
            t.faderCap      = juce::Colour (0xFF222222);
            t.unlitLed      = juce::Colour (0xFF1C1C1F);
        }
        else if (index == 3) // Matrix Terminal
        {
            t.background    = juce::Colour (0xFF030A05);
            t.border        = juce::Colour (0xFF112A18);
            t.leftAccent    = juce::Colour (0xFF33FF33); // Matrix Green
            t.rightAccent   = juce::Colour (0xFF22AA22); 
            t.textDim       = juce::Colour (0xFF1E5F2E);
            t.trackBg       = juce::Colour (0xFF020603);
            t.slotOutline   = juce::Colour (0xFF0E451E);
            t.faderCap      = juce::Colour (0xFF112A18);
            t.unlitLed      = juce::Colour (0xFF0E1A11);
        }
        else // Default Navy Cyber (Dark default) [5]
        {
            t.background    = juce::Colour (0xFF16181F);
            t.border        = juce::Colour (0xFF2A2E3D);
            t.leftAccent    = juce::Colour (0xFF00D2FF); // Cyan
            t.rightAccent   = juce::Colour (0xFFFFB300); // Amber
            t.textDim       = juce::Colour (0xFF55555C);
            t.trackBg       = juce::Colour (0xFF181C24);
            t.slotOutline   = juce::Colour (0xFF242835);
            t.faderCap      = juce::Colour (0xFF1E212A);
            t.unlitLed      = juce::Colour (0xFF1C1E24);
        }
        return t;
    }
};

// ==============================================================================
// Custom DJ TechTools / Chroma Caps Style Rotary & Linear LookAndFeel [5]
// ==============================================================================
class ChromaCapsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromaCapsLookAndFeel (PluginProcessor& p) : processor (p)
    {
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                           float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        auto knobBounds = bounds.reduced (16.0f); // Leave room on boundaries for the 15 LEDs [5]
        auto radius = juce::jmin (knobBounds.getWidth(), knobBounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();

        // Load active theme colors dynamically [NEW]
        int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (themeIdx);

        // 1. Subtle drop shadow beneath the rubber cap [5]
        g.setColour (juce::Colour (themeIdx == 1 ? 0x25000000 : 0x45000000));
        g.fillEllipse (toX - radius + 2.0f, toY - radius + 4.0f, radius * 2.0f, radius * 2.0f);

        // 2. Matte rubberized cylindrical body [5]
        juce::Colour rubberBaseCol = (themeIdx == 1) ? juce::Colour (0xFF1E212A) : juce::Colour (0xFF1A1C24); 
        juce::ColourGradient grad (rubberBaseCol.brighter (0.12f), toX, toY - radius, 
                                   rubberBaseCol.darker (0.25f), toX, toY + radius, false);
        g.setGradientFill (grad);
        g.fillEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        // 3. Highlighted outer lip edge of the cap
        g.setColour (juce::Colour (0xFF2D313D));
        g.drawEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // 4. Colored rubber indicator pointer strip [5]
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        
        // Symmetrically map accents based on left and right slider columns
        juce::Colour accentCol = (slider.getComponentID() == "rhythmMorph" || slider.getComponentID() == "rest" || 
                                  slider.getComponentID() == "legato" || slider.getComponentID() == "rate")
                                 ? t.leftAccent : t.rightAccent;

        juce::Path pointerPath;
        float pointerLength = radius * 0.95f;
        float pointerThickness = radius * 0.16f;
        pointerPath.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.3f);

        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (angle).translated (toX, toY));
        g.setColour (accentCol);
        g.fillPath (pointerPath);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.strokePath (pointerPath, juce::PathStrokeType (0.7f)); 
        g.restoreState();

        // Center Cap dome
        float centerRadius = radius * 0.38f;
        g.setColour (rubberBaseCol.darker (0.5f));
        g.fillEllipse (toX - centerRadius, toY - centerRadius, centerRadius * 2.0f, centerRadius * 2.0f);

        // 5. Circular 15-LED rings with real-time modulation visual updates [5]
        float ledRingRadius = radius + 9.5f; 
        int numLeds = 15;
        juce::Colour ledActiveCol = accentCol;
        float visualValue = sliderPos;
        bool lfoActive = false;

        // Safely identify the rotary slider using the component ID [5]
        juce::String pId = slider.getComponentID();
        if (pId.isNotEmpty())
        {
            int lfoRateVal = 0;
            
            if (pId == "rhythmMorph") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rhythmMorphLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeMorph : sliderPos;
            }
            else if (pId == "rest") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::restLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeRest : sliderPos;
            }
            else if (pId == "legato") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::legatoLfoRate.getParamID()));
                float baseLegato = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::legato.getParamID()));
                float rawLegato = (lfoRateVal > 0) ? processor.activeLegato : baseLegato;
                visualValue = (rawLegato - 0.1f) / 0.9f;
            }
            else if (pId == "rate") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rateLfoRate.getParamID()));
                float baseRate = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::rate.getParamID()));
                float rawRate = (lfoRateVal > 0) ? static_cast<float>(processor.activeRateIdx) : baseRate;
                visualValue = rawRate / 3.0f;
            }
            else if (pId == "entropy") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::entropyLfoRate.getParamID()));
                float baseEntropy = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::entropy.getParamID()));
                float rawEntropy = (lfoRateVal > 0) ? processor.activeEntropy : baseEntropy;
                visualValue = (rawEntropy + 1.0f) * 0.5f;
            }
            else if (pId == "harmony") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::harmonyLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeHarmony : sliderPos;
            }
            else if (pId == "chaos") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::chaosLfoRate.getParamID()));
                visualValue = (lfoRateVal > 0) ? processor.activeChaos : sliderPos;
            }
            else if (pId == "octaves") {
                lfoRateVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octavesLfoRate.getParamID()));
                float baseOctaves = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
                float rawOctaves = (lfoRateVal > 0) ? static_cast<float>(processor.activeOctavesVal) : baseOctaves;
                visualValue = (rawOctaves + 2.0f) / 5.0f; // Symmetrical scale layout matching -2 to +3 [NEW]
            }
            
            lfoActive = (lfoRateVal > 0);
            
            // Shift neon LED colors to indicate focus
            if (lfoActive)
            {
                ledActiveCol = (pId == "rhythmMorph" || pId == "rest" || pId == "legato" || pId == "rate") 
                               ? juce::Colour (0xFFFF00D2)  // Neon Magenta (Left)
                               : juce::Colour (0xFF9933FF); // Deep Purple (Right)
            }
        }

        // Draw 15 discrete circular LED dots [5]
        for (int i = 0; i < numLeds; ++i)
        {
            float pct = static_cast<float>(i) / static_cast<float>(numLeds - 1);
            float ledAngle = rotaryStartAngle + pct * (rotaryEndAngle - rotaryStartAngle);
            
            float ledX = toX + ledRingRadius * std::sin (ledAngle);
            float ledY = toY - ledRingRadius * std::cos (ledAngle);
            
            bool isLit = (pct <= visualValue);
            
            if (isLit)
            {
                g.setColour (ledActiveCol);
                g.fillEllipse (ledX - 1.8f, ledY - 1.8f, 3.6f, 3.6f);
                
                g.setColour (ledActiveCol.withAlpha (0.22f));
                g.drawEllipse (ledX - 3.2f, ledY - 3.2f, 6.4f, 6.4f, 1.2f);
            }
            else
            {
                g.setColour (t.unlitLed); // Custom unlit led color mapped to current panel theme [NEW]
                g.fillEllipse (ledX - 1.5f, ledY - 1.5f, 3.0f, 3.0f);
            }
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Custom vertical mixer-style faders [5]
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 4.0f;
            auto trackX = x + width * 0.5f - trackWidth * 0.5f;
            
            // Load active theme colors dynamically [NEW]
            int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
            auto t = AppTheme::get (themeIdx);

            // Recessed slot
            g.setColour (t.trackBg);
            g.fillRoundedRectangle (trackX, (float)y, trackWidth, (float)height, trackWidth * 0.5f);
            
            g.setColour (t.slotOutline);
            g.drawRoundedRectangle (trackX - 1.0f, (float)y, trackWidth + 2.0f, (float)height, trackWidth * 0.5f, 1.0f);

            // Tactile Chroma Fader Cap
            float capWidth = juce::jmin (26.0f, width * 0.8f);
            float capHeight = 14.0f;
            float capX = x + width * 0.5f - capWidth * 0.5f;
            float capY = sliderPos - capHeight * 0.5f;

            // Shadow
            g.setColour (juce::Colour (themeIdx == 1 ? 0x15000000 : 0x45000000));
            g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 2.0f);

            // Tactile Rubberized Fader Cap Body
            juce::Colour capBaseCol = t.faderCap;
            juce::ColourGradient capGrad (capBaseCol.brighter (0.1f), capX, capY,
                                         capBaseCol.darker (0.2f), capX, capY + capHeight, false);
            g.setGradientFill (capGrad);
            g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f);

            // Borders
            g.setColour (juce::Colour (0xFF3A3F4E));
            g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f, 1.0f);

            // Horizontal high-contrast stripe [5]
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            float stripeHeight = 2.0f;
            g.fillRect (capX + 2.0f, capY + capHeight * 0.5f - stripeHeight * 0.5f, capWidth - 4.0f, stripeHeight);
        }
        else if (style == juce::Slider::LinearHorizontal) // Crossfader [5] [NEW]
        {
            // Load active theme colors dynamically
            int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
            auto t = AppTheme::get (themeIdx);

            auto trackHeight = 4.0f;
            auto trackY = y + height * 0.5f - trackHeight * 0.5f;

            // Recessed horizontal slot [5]
            g.setColour (t.trackBg);
            g.fillRoundedRectangle ((float)x, trackY, (float)width, trackHeight, trackHeight * 0.5f);
            g.setColour (t.slotOutline);
            g.drawRoundedRectangle ((float)x, trackY, (float)width, trackHeight, trackHeight * 0.5f, 1.0f);

            // 3D DJ-Style Crossfader Cap [5] [NEW]
            float capWidth = 28.0f;
            float capHeight = 16.0f;
            float capX = sliderPos - capWidth * 0.5f;
            float capY = y + height * 0.5f - capHeight * 0.5f;

            // Fader shadow
            g.setColour (juce::Colour (themeIdx == 1 ? 0x15000000 : 0x45000000));
            g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 2.0f);

            // Tactile Rubberized Cap Body
            juce::Colour capBaseCol = t.faderCap;
            juce::ColourGradient capGrad (capBaseCol.brighter (0.1f), capX, capY,
                                         capBaseCol.darker (0.2f), capX, capY + capHeight, false);
            g.setGradientFill (capGrad);
            g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f);

            // Borders
            g.setColour (juce::Colour (0xFF3A3F4E));
            g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f, 1.0f);

            // Vertical high-contrast indicator stripe [5] [NEW]
            // Morph Color-Coding: dynamically blends between Cyan/Amber
            float blendVal = (sliderPos - minSliderPos) / (maxSliderPos - minSliderPos);
            juce::Colour indicatorCol = t.leftAccent.interpolatedWith (t.rightAccent, blendVal);
            g.setColour (indicatorCol);
            float stripeWidth = 2.0f;
            g.fillRect (capX + capWidth * 0.5f - stripeWidth * 0.5f, capY + 2.0f, stripeWidth, capHeight - 4.0f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    // ==============================================================================
    // Custom Vector Dice Renderer (Eliminates OS Unicode Font Limitations) [NEW]
    // ==============================================================================
    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::String id = button.getComponentID();
        int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (themeIdx);
        
        if (id == "dice_scene_a" || id == "dice_scene_b")
        {
            auto bounds = button.getLocalBounds().toFloat();
            juce::Colour pipCol = (id == "dice_scene_a") ? t.leftAccent : t.rightAccent;
            if (shouldDrawButtonAsDown) pipCol = pipCol.brighter (0.2f);
            drawVectorDice (g, bounds, pipCol);
        }
        else if (id == "dice_melody" || id == "dice_rhythm")
        {
            auto bounds = button.getLocalBounds().toFloat();
            auto diceBounds = bounds.removeFromLeft (bounds.getHeight()); // Make square for vector dice placement
            
            juce::Colour pipCol = t.rightAccent;
            if (shouldDrawButtonAsDown) pipCol = pipCol.brighter (0.2f);
            drawVectorDice (g, diceBounds.reduced (4.0f), pipCol);
            
            // Draw text dynamically next to it
            g.setColour (button.findColour (juce::TextButton::textColourOffId));
            g.setFont (getTextButtonFont (button, button.getHeight()));
            g.drawFittedText (button.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, 1);
        }
        else
        {
            juce::LookAndFeel_V4::drawButtonText (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        }
    }

private:
    void drawVectorDice (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour pipColour)
    {
        auto diceFace = bounds.reduced (bounds.getWidth() * 0.22f);
        
        // Face background
        g.setColour (pipColour.withAlpha (0.12f));
        g.fillRoundedRectangle (diceFace, 4.0f);
        
        // Face border
        g.setColour (pipColour.withAlpha (0.75f));
        g.drawRoundedRectangle (diceFace, 4.0f, 1.2f);
        
        // Symmetrical pips
        float pipSize = juce::jmax (1.5f, diceFace.getWidth() * 0.16f);
        float cx = diceFace.getCentreX();
        float cy = diceFace.getCentreY();
        float w = diceFace.getWidth();
        float h = diceFace.getHeight();
        float offset = 0.25f; 
        
        g.setColour (pipColour);
        
        // Render 5 pips
        g.fillEllipse (cx - pipSize * 0.5f, cy - pipSize * 0.5f, pipSize, pipSize); // Center
        g.fillEllipse (cx - w * offset - pipSize * 0.5f, cy - h * offset - pipSize * 0.5f, pipSize, pipSize); // Top-Left
        g.fillEllipse (cx + w * offset - pipSize * 0.5f, cy - h * offset - pipSize * 0.5f, pipSize, pipSize); // Top-Right
        g.fillEllipse (cx - w * offset - pipSize * 0.5f, cy + h * offset - pipSize * 0.5f, pipSize, pipSize); // Bottom-Left
        g.fillEllipse (cx + w * offset - pipSize * 0.5f, cy + h * offset - pipSize * 0.5f, pipSize, pipSize); // Bottom-Right
    }

    PluginProcessor& processor;
};

// ==============================================================================
// Active OLED Display with lock-free atomic visual synchronization
// ==============================================================================
class OledDisplay : public juce::Component, public juce::Timer
{
public:
    OledDisplay (PluginProcessor& p) : processor (p) 
    {
        startTimerHz (30);
    }
    
    ~OledDisplay() override { stopTimer(); }

    void timerCallback() override
    {
        // Self-contained LFO parameter modification tracking loop [NEW]
        juce::String prefixes[] = { "rhythmMorph", "rest", "legato", "rate", "entropy", "harmony", "chaos", "octaves" };
        bool lfoParamChanged = false;
        int changedIdx = -1;

        for (int i = 0; i < 8; ++i)
        {
            int rate = static_cast<int> (*processor.apvts.getRawParameterValue (prefixes[i] + "LfoRate"));
            float depth = *processor.apvts.getRawParameterValue (prefixes[i] + "LfoDepth");
            
            if (rate != lastLfoRates[i] || depth != lastLfoDepths[i])
            {
                lastLfoRates[i] = rate;
                lastLfoDepths[i] = depth;
                lfoParamChanged = true;
                changedIdx = i;
            }
        }

        // Set LFO preview oscilloscope fade out triggers [NEW]
        if (lfoParamChanged)
        {
            lfoOverlayTimer = 45; // 1.5 seconds at 30Hz
            lfoActiveParamIdx = changedIdx;
        }
        else if (lfoOverlayTimer > 0)
        {
            lfoOverlayTimer--;
        }

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (themeIdx);

        // Keep OLED screen background dark and professional even on light beige panels!
        g.fillAll (t.background.darker (0.9f)); 
        g.setColour (t.border); 
        g.drawRect (getLocalBounds().toFloat(), 2.0f);

        // OLED Grid Area Calculations
        auto area = getLocalBounds().reduced (15);

        // Render real-time LFO Waveform Oscilloscope Overlay on active parameter change [NEW]
        if (lfoOverlayTimer > 0 && lfoActiveParamIdx >= 0)
        {
            juce::String prefixes[] = { "MORPH", "REST", "LEGATO", "RATE", "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };
            
            g.setColour (t.leftAccent);
            g.setFont (juce::Font ("Consolas", 14.0f, juce::Font::bold));
            g.drawText ("LFO PREVIEW: " + prefixes[lfoActiveParamIdx], 15, 12, getWidth() - 30, 20, juce::Justification::centred);

            g.setFont (juce::Font ("Consolas", 10.0f, juce::Font::plain));
            g.setColour (juce::Colour (0xFF888A90));
            juce::String speedRate = processor.apvts.getParameter(prefixes[lfoActiveParamIdx].toLowerCase() + "LfoRate")->getCurrentValueAsText();
            g.drawText ("RATE: " + speedRate + " | DEPTH: " + juce::String(static_cast<int>(lastLfoDepths[lfoActiveParamIdx] * 100.0f)) + "%",
                        15, 25, getWidth() - 30, 15, juce::Justification::centred);

            // Draw clean real-time scrolling sine wave representation [NEW]
            juce::Path wavePath;
            float waveYCenter = area.getCentreY() + 10.0f;
            float waveHeight = (area.getHeight() - 40.0f) * lastLfoDepths[lfoActiveParamIdx] * 0.45f;
            
            int rateChoice = lastLfoRates[lfoActiveParamIdx];
            float frequencyFactor = 1.0f;
            if (rateChoice == 1)      frequencyFactor = 0.25f; // Slow
            else if (rateChoice == 2) frequencyFactor = 0.5f;
            else if (rateChoice == 3) frequencyFactor = 1.0f;  // Medium
            else if (rateChoice == 4) frequencyFactor = 2.0f;  // Fast

            static float phaseOffset = 0.0f;
            phaseOffset += 0.15f * frequencyFactor;
            if (phaseOffset >= juce::MathConstants<float>::twoPi) phaseOffset -= juce::MathConstants<float>::twoPi;

            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                float pct = static_cast<float>(x - area.getX()) / static_cast<float>(area.getWidth());
                float sineVal = std::sin (pct * juce::MathConstants<float>::twoPi * 2.0f * frequencyFactor + phaseOffset);
                float yVal = waveYCenter + sineVal * waveHeight;
                
                if (x == area.getX()) wavePath.startNewSubPath (static_cast<float>(x), yVal);
                else                  wavePath.lineTo (static_cast<float>(x), yVal);
            }

            // Choose symmetrical left/right accent color
            g.setColour (lfoActiveParamIdx < 4 ? t.leftAccent : t.rightAccent);
            g.strokePath (wavePath, juce::PathStrokeType (1.8f));
            return; // Intercept drawing to skip standard meters
        }

        g.setColour (t.leftAccent);
        g.setFont (juce::Font ("Consolas", 14.0f, juce::Font::bold));
        juce::String headerText = "NAVY-ARP GRAPHIC MONITOR";
        g.drawFittedText (headerText, getLocalBounds().removeFromTop (25), juce::Justification::centred, 1);

        // Real-time OLED Context Information
        juce::String scaleName = processor.apvts.getParameter(IDs::scaleType.getParamID())->getCurrentValueAsText();
        juce::String keyName = processor.apvts.getParameter(IDs::rootKey.getParamID())->getCurrentValueAsText();
        int extType = processor.activeChordExtensionType.load();
        juce::String extText = (extType == 0) ? "TRIAD" : (extType == 1) ? "SUS" : "7TH/9TH";

        juce::String speedRate = processor.apvts.getParameter(IDs::rate.getParamID())->getCurrentValueAsText();
        juce::String activeOcts = processor.apvts.getParameter(IDs::octaves.getParamID())->getCurrentValueAsText();

        g.setFont (juce::Font ("Consolas", 11.0f, juce::Font::plain));
        g.setColour (juce::Colour (0xFF888A90));
        g.drawText ("KEY: " + keyName + " | SCALE: " + scaleName + " | EXT: " + extText + " | RATE: " + speedRate + " | OCT: " + activeOcts, 
                    10, 25, getWidth() - 20, 15, juce::Justification::centred);

        area.removeFromTop (35); 
        
        // 1. Draw subtle horizontal grid thresholds
        g.setColour (juce::Colour (0xFF141822));
        for (float pct : { 0.25f, 0.50f, 0.75f })
        {
            float gridY = area.getBottom() - 15.0f - (area.getHeight() - 15.0f) * pct * 0.75f;
            g.drawHorizontalLine (static_cast<int>(gridY), (float)area.getX(), (float)area.getRight());
        }

        int barWidth = area.getWidth() / 8;
        int spacing = 6;

        for (int i = 0; i < 8; ++i)
        {
            float faderProb = *processor.apvts.getRawParameterValue ("fader" + juce::String (i + 1));
            int barHeight = static_cast<int>((area.getHeight() - 15) * faderProb * 0.75f);
            
            juce::Rectangle<int> bar (area.getX() + (i * barWidth) + spacing, 
                                      area.getBottom() - barHeight - 15, 
                                      barWidth - (spacing * 2), 
                                      barHeight);

            bool isPlaying = processor.isCurrentlyPlayingUI.load();

            // 2. Draw dynamic step bars
            if (i == processor.currentStep && isPlaying)
            {
                if (i == 0)      g.setColour (juce::Colour (0xFF4CFF4C)); // Beat 1: Green
                else if (i == 4) g.setColour (juce::Colour (0xFFFF4C4C)); // Beat 5: Red
                else             g.setColour (t.leftAccent); // Matches the current theme's primary left accent
                g.fillRect (bar.expanded(1, 1));
            }
            else
            {
                if (i % 3 == 0)      g.setColour (t.leftAccent.withAlpha (0.85f)); 
                else if (i % 4 == 0) g.setColour (juce::Colour (0xFF9966FF)); 
                else                 g.setColour (juce::Colour (0xFFFFAA00)); 
                g.fillRect (bar);
            }

            // 3. Symmetrical Sequencer Step indicators
            juce::String stepNumStr = juce::String (i + 1);
            g.setFont (juce::Font ("Consolas", 10.0f, juce::Font::bold));
            
            if (i == processor.currentStep && isPlaying)
            {
                if (i == 0)      g.setColour (juce::Colour (0xFF4CFF4C)); 
                else if (i == 4) g.setColour (juce::Colour (0xFFFF4C4C)); 
                else             g.setColour (t.leftAccent);
            }
            else
            {
                g.setColour (juce::Colour (0xFF3A3F4E));
            }
            
            int numX = area.getX() + (i * barWidth);
            g.drawText (stepNumStr, numX, area.getBottom() - 12, barWidth, 12, juce::Justification::centred);
        }
    }

private:
    PluginProcessor& processor;
    int lastLfoRates[8] = { 0 };
    float lastLfoDepths[8] = { 0.0f };
    int lfoOverlayTimer = 0;
    int lfoActiveParamIdx = -1;
};

// ==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, 
                     public juce::Timer,
                     public juce::AudioProcessorValueTreeState::Listener // Inherit from Parameter Listener [3]
{
public:
    PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    // Trigger instant GUI repaint when active theme parameter is changed [3]
    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    PluginProcessor& processor;
    OledDisplay oledDisplay;
    ChromaCapsLookAndFeel chromaLookAndFeel; // Custom rubber-cap style controls [5]

    juce::Slider fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8;
    juce::Label faderLabel1, faderLabel2, faderLabel3, faderLabel4, faderLabel5, faderLabel6, faderLabel7, faderLabel8;

    juce::Slider rhythmMorphKnob, restKnob, legatoKnob, rateKnob;
    juce::Label rhythmMorphTitle, restTitle, legatoTitle, rateTitle;

    juce::Slider entropyKnob, harmonyKnob, chaosKnob, octavesKnob;
    juce::Label entropyTitle, harmonyTitle, chaosTitle, octavesTitle;

    juce::Slider morphCrossfader;

    juce::TextButton latchButton;
    juce::TextButton chordModeButton;
    juce::TextButton diceMelodyButton;
    juce::TextButton diceRhythmButton;
    
    // Octatrack Scene Buttons [5]
    juce::TextButton sceneAButtons[4];
    juce::TextButton sceneBButtons[4];
    juce::TextButton diceSceneAButton;
    juce::TextButton diceSceneBButton;

    juce::TextButton saveButton;
    juce::TextButton recallButton;
    juce::TextButton presetButtons[8]; // Declared Preset Slots Array [CRITICAL FIX]

    juce::ComboBox rootKeyBox;
    juce::ComboBox scaleTypeBox;
    juce::ComboBox cycleLengthBox;

    // Symmetrical time trackers for hold-to-save logic [5]
    uint32_t sceneAPressStartTime[4] = { 0 };
    uint32_t sceneBPressStartTime[4] = { 0 };

    // Symmetrical holds state tracker for real-time saving [NEW]
    bool sceneAAlreadySaved[4] = { false };
    bool sceneBAlreadySaved[4] = { false };

    // Flash counters for visual confirm [5]
    int sceneAFlashTimer[4] = { 0 };
    int sceneBFlashTimer[4] = { 0 };

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> entropyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> octavesAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chordModeAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cycleLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor) // Corrected leak detector macro [5]
};

#endif // NAVY_ARP_EDITOR_H