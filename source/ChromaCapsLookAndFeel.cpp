#include "ChromaCapsLookAndFeel.h"
#include "PluginEditor.h" 

ChromaCapsLookAndFeel::ChromaCapsLookAndFeel (PluginProcessor& p, PluginEditor* e) 
    : processor (p), editor (e)
{
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
}

void ChromaCapsLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, 
                                              float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle, 
                                              juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    auto knobBounds = bounds.reduced (16.0f);
    auto radius = juce::jmin (knobBounds.getWidth(), knobBounds.getHeight()) / 2.0f;
    auto toX = bounds.getCentreX(), toY = bounds.getCentreY();

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    g.setColour (juce::Colour (themeIdx == 1 ? 0x25000000 : 0x45000000));
    g.fillEllipse (toX - radius + 2.0f, toY - radius + 4.0f, radius * 2.0f, radius * 2.0f);

    juce::Colour rubberBaseCol = (themeIdx == 1) ? juce::Colour (0xFF1E212A) : juce::Colour (0xFF1A1C24); 
    juce::ColourGradient grad (rubberBaseCol.brighter (0.12f), toX, toY - radius, rubberBaseCol.darker (0.25f), toX, toY + radius, false);
    g.setGradientFill (grad); g.fillEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xFF2D313D)); g.drawEllipse (toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Colour accentCol = (slider.getComponentID() == "rhythmMorph" || slider.getComponentID() == "rest" || 
                              slider.getComponentID() == "legato" || slider.getComponentID() == "rate") ? t.leftAccent : t.rightAccent;

    juce::Path pointerPath;
    float pointerLength = radius * 0.95f, pointerThickness = radius * 0.16f;
    pointerPath.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.3f);

    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (angle).translated (toX, toY));
    g.setColour (accentCol); g.fillPath (pointerPath);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (pointerPath, juce::PathStrokeType (0.7f)); 
    g.restoreState();

    float centerRadius = radius * 0.38f; g.setColour (rubberBaseCol.darker (0.5f)); g.fillEllipse (toX - centerRadius, toY - centerRadius, centerRadius * 2.0f, centerRadius * 2.0f);

    float ledRingRadius = radius + 9.5f; int numLeds = 15;
    juce::Colour ledActiveCol = accentCol; float visualValue = sliderPos; bool lfoActive = false;

    juce::String pId = slider.getComponentID();
    if (pId.isNotEmpty()) {
        int lfoPresetVal = 0;
        if (pId == "rhythmMorph") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rhythmMorphLfoPreset.getParamID()));
            visualValue = (lfoPresetVal > 0) ? processor.activeMorph : sliderPos;
        }
        else if (pId == "rest") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::restLfoPreset.getParamID()));
            visualValue = (lfoPresetVal > 0) ? processor.activeRest : sliderPos;
        }
        else if (pId == "legato") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::legatoLfoPreset.getParamID()));
            float baseLegato = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::legato.getParamID()));
            float rawLegato = (lfoPresetVal > 0) ? processor.activeLegato : baseLegato;
            visualValue = (rawLegato - 0.1f) / 0.9f;
        }
        else if (pId == "rate") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::rateLfoPreset.getParamID()));
            float baseRate = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::rate.getParamID()));
            float rawRate = (lfoPresetVal > 0) ? static_cast<float>(processor.activeRateIdx) : baseRate;
            visualValue = rawRate / 3.0f;
        }
        else if (pId == "entropy") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::entropyLfoPreset.getParamID()));
            float baseEntropy = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::entropy.getParamID()));
            float rawEntropy = (lfoPresetVal > 0) ? processor.activeEntropy : baseEntropy;
            visualValue = (rawEntropy + 1.0f) * 0.5f;
        }
        else if (pId == "harmony") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::harmonyLfoPreset.getParamID()));
            visualValue = (lfoPresetVal > 0) ? processor.activeHarmony : sliderPos;
        }
        else if (pId == "chaos") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::chaosLfoPreset.getParamID()));
            visualValue = (lfoPresetVal > 0) ? processor.activeChaos : sliderPos;
        }
        else if (pId == "octaves") {
            lfoPresetVal = static_cast<int> (*processor.apvts.getRawParameterValue (IDs::octavesLfoPreset.getParamID()));
            float baseOctaves = static_cast<float> (*processor.apvts.getRawParameterValue (IDs::octaves.getParamID()));
            float rawOctaves = (lfoPresetVal > 0) ? static_cast<float>(processor.activeOctavesVal) : baseOctaves;
            visualValue = (rawOctaves + 3.0f) / 6.0f;
        }
        lfoActive = (lfoPresetVal > 0);
        if (lfoActive) ledActiveCol = (pId == "rhythmMorph" || pId == "rest" || pId == "legato" || pId == "rate") ? juce::Colour (0xFFFF00D2) : juce::Colour (0xFF9933FF);
    }

    for (int i = 0; i < numLeds; ++i) {
        float pct = static_cast<float>(i) / static_cast<float>(numLeds - 1);
        float ledAngle = rotaryStartAngle + pct * (rotaryEndAngle - rotaryStartAngle);
        float ledX = toX + ledRingRadius * std::sin (ledAngle), ledY = toY - ledRingRadius * std::cos (ledAngle);
        if (pct <= visualValue) {
            g.setColour (ledActiveCol); g.fillEllipse (ledX - 1.8f, ledY - 1.8f, 3.6f, 3.6f);
            g.setColour (ledActiveCol.withAlpha (0.22f)); g.drawEllipse (ledX - 3.2f, ledY - 3.2f, 6.4f, 6.4f, 1.2f);
        } else {
            g.setColour (t.unlitLed); g.fillEllipse (ledX - 1.5f, ledY - 1.5f, 3.0f, 3.0f);
        }
    }
}

void ChromaCapsLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    if (style == juce::Slider::LinearVertical) {
        auto trackWidth = 4.0f, trackX = x + width * 0.5f - trackWidth * 0.5f;
        
        // Recessed 3D track slot drawing
        g.setColour (t.trackBg.darker (0.15f)); g.fillRoundedRectangle (trackX, (float)y, trackWidth, (float)height, trackWidth * 0.5f);
        
        // Subtle 3D inner bevel shadow and highlight lines
        g.setColour (juce::Colours::black.withAlpha (0.22f)); g.drawLine (trackX, (float)y, trackX, (float)(y + height), 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawLine (trackX + trackWidth, (float)y, trackX + trackWidth, (float)(y + height), 1.0f);
        
        g.setColour (t.slotOutline); g.drawRoundedRectangle (trackX - 1.0f, (float)y, trackWidth + 2.0f, (float)height, trackWidth * 0.5f, 1.0f);

        float capWidth = juce::jmin (26.0f, width * 0.8f), capHeight = 14.0f;
        float capX = x + width * 0.5f - capWidth * 0.5f, capY = sliderPos - capHeight * 0.5f;
        g.setColour (juce::Colour (themeIdx == 1 ? 0x15000000 : 0x45000000)); g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 2.0f);

        juce::Colour capBaseCol = t.faderCap;
        juce::ColourGradient capGrad (capBaseCol.brighter (0.1f), capX, capY, capBaseCol.darker (0.2f), capX, capY + capHeight, false);
        g.setGradientFill (capGrad); g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f);
        g.setColour (juce::Colour (0xFF3A3F4E)); g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f, 1.0f);
        g.setColour (slider.findColour (juce::Slider::thumbColourId)); g.fillRect (capX + 2.0f, capY + capHeight * 0.5f - 1.0f, capWidth - 4.0f, 2.0f);
    }
    else if (style == juce::Slider::LinearHorizontal) {
        auto trackHeight = 4.0f, trackY = y + height * 0.5f - trackHeight * 0.5f;
        
        // Recessed 3D track slot drawing
        g.setColour (t.trackBg.darker (0.15f)); g.fillRoundedRectangle ((float)x, trackY, (float)width, trackHeight, trackHeight * 0.5f);
        
        // Subtle 3D inner bevel shadow and highlight lines
        g.setColour (juce::Colours::black.withAlpha (0.22f)); g.drawLine ((float)x, trackY, (float)(x + width), trackY, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawLine ((float)x, trackY + trackHeight, (float)(x + width), trackY + trackHeight, 1.0f);
        
        g.setColour (t.slotOutline); g.drawRoundedRectangle ((float)x, trackY, (float)width, trackHeight, trackHeight * 0.5f, 1.0f);

        float capWidth = 28.0f, capHeight = 16.0f;
        float capX = sliderPos - capWidth * 0.5f, capY = y + height * 0.5f - capHeight * 0.5f;
        g.setColour (juce::Colour (themeIdx == 1 ? 0x15000000 : 0x45000000)); g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 2.0f);

        juce::Colour capBaseCol = t.faderCap;
        juce::ColourGradient capGrad (capBaseCol.brighter (0.1f), capX, capY, capBaseCol.darker (0.2f), capX, capY + capHeight, false);
        g.setGradientFill (capGrad); g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f);
        g.setColour (juce::Colour (0xFF3A3F4E)); g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 2.0f, 1.0f);

        float blendVal = (sliderPos - minSliderPos) / (maxSliderPos - minSliderPos);
        g.setColour (t.leftAccent.interpolatedWith (t.rightAccent, blendVal));
        g.fillRect (capX + capWidth * 0.5f - 1.0f, capY + 2.0f, 2.0f, capHeight - 4.0f);
    }
    else {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

void ChromaCapsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (backgroundColour); auto bounds = button.getLocalBounds().toFloat();
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);

    bool isToggled = button.getToggleState(), isDown = shouldDrawButtonAsDown, isOver = shouldDrawButtonAsHighlighted;
    juce::String id = button.getComponentID();

    // 1. Symmetrical Scene Target Highlight (Only active anchor is lit, otherwise dark)
    if (button.getButtonText() == "A") {
        bool isBActive = processor.isSceneBActiveAnchor.load();
        g.setColour (!isBActive ? t.leftAccent.withAlpha (isDown ? 0.8f : (isOver ? 0.5f : 0.35f)) : juce::Colour (0xFF151515)); g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (!isBActive ? t.leftAccent : t.border); g.drawRoundedRectangle (bounds, 4.0f, 1.2f); return;
    }
    else if (button.getButtonText() == "B") {
        bool isBActive = processor.isSceneBActiveAnchor.load();
        g.setColour (isBActive ? t.leftAccent.withAlpha (isDown ? 0.8f : (isOver ? 0.5f : 0.35f)) : juce::Colour (0xFF151515)); g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (isBActive ? t.leftAccent : t.border); g.drawRoundedRectangle (bounds, 4.0f, 1.2f); return;
    }

    // 2. Preset buttons with active storage light states & tap flash confirmations
    int presetIdx = button.getButtonText().getIntValue() - 1;
    if (presetIdx >= 0 && presetIdx < 8 && id.isEmpty()) {
        if (editor != nullptr) {
            int flashTimer = editor->presetFlashTimer[presetIdx], flashType = editor->presetFlashType[presetIdx];
            if (flashTimer > 0) {
                float alpha = static_cast<float> (flashTimer) / 24.0f; juce::Colour flashCol = (flashType == 1) ? juce::Colour (0xFFFFB300) : juce::Colour (0xFF00D2FF);
                g.setColour (flashCol.withAlpha (alpha * 0.4f)); g.fillRoundedRectangle (bounds, 4.0f);
                g.setColour (flashCol.withAlpha (alpha)); g.drawRoundedRectangle (bounds, 4.0f, 1.8f); return;
            }
        }
        bool hasPreset = processor.isPresetSaved (presetIdx);
        g.setColour (hasPreset ? t.leftAccent.withAlpha (0.12f) : juce::Colour (0xFF141416)); g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (hasPreset ? t.leftAccent.withAlpha (0.6f) : t.unlitLed); g.drawRoundedRectangle (bounds, 4.0f, 1.2f); return;
    }

    // 3. Performance & Utility Buttons (Save, Recall, Copy, Init, Latch, SEQ, Poly, Freeze)
    juce::Colour baseCol = (themeIdx == 1) ? juce::Colour (0xFFE2E0D8).darker (0.05f) : juce::Colour (0xFF181C22);
    if (isToggled) {
        juce::Colour activeAccent = t.leftAccent;
        if (id == "dice_melody" || id == "dice_articulation" || id == "dice_time" || id == "dice_navy")
            activeAccent = t.rightAccent;
        else if (button.getButtonText() == "Freeze")
            activeAccent = juce::Colour (0xFF80D8FF); // Luminous Ice Blue color for Freeze latch button!

        g.setColour (activeAccent.withAlpha (0.15f)); g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (activeAccent); g.drawRoundedRectangle (bounds, 4.0f, 1.8f);
    } else {
        g.setColour (baseCol.darker (isDown ? 0.2f : (isOver ? 0.05f : 0.0f))); g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (t.border); g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    juce::String id = button.getComponentID();
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);
    bool isToggled = button.getToggleState();

    juce::Colour activeAccent = (id == "dice_melody" || id == "dice_articulation" || id == "dice_time" || id == "dice_navy" || button.getButtonText() == "B") ? t.rightAccent : t.leftAccent;

    if (id == "dice_melody" || id == "dice_articulation" || id == "dice_time" || id == "dice_navy") {
        auto bounds = button.getLocalBounds().toFloat();
        float diceSize = 14.0f, diceX = 6.0f, diceY = (bounds.getHeight() - diceSize) * 0.5f;
        auto diceBounds = juce::Rectangle<float> (diceX, diceY, diceSize, diceSize);
        drawVectorDice (g, diceBounds, activeAccent);
        
        auto textBounds = button.getLocalBounds().toFloat().withTrimmedLeft (24.0f);
        g.setColour (isToggled ? activeAccent.brighter (0.1f) : (themeIdx == 1 ? juce::Colour(0xFF1E1E1E) : t.textDim.brighter(0.2f)));
        g.setFont (getTextButtonFont (button, button.getHeight())); g.drawFittedText (button.getButtonText(), textBounds.toNearestInt(), juce::Justification::centredLeft, 1);
    } else {
        int presetIdx = button.getButtonText().getIntValue() - 1; bool isPreset = (presetIdx >= 0 && presetIdx < 8 && id.isEmpty());
        juce::Colour textCol = isToggled ? activeAccent.brighter (0.1f) : (themeIdx == 1 ? juce::Colour (0xFF1E1E1E) : juce::Colour (0xFFCCCCCC));
        if (isPreset) textCol = processor.isPresetSaved (presetIdx) ? t.leftAccent.brighter(0.2f) : (themeIdx == 1 ? juce::Colour (0xFF7A7870) : juce::Colour (0xFF55555C));
        g.setColour (textCol); g.setFont (getTextButtonFont (button, button.getHeight())); g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
    }
}

void ChromaCapsLookAndFeel::drawVectorDice (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour pipColour)
{
    auto diceFace = bounds.reduced (1.0f);
    g.setColour (pipColour.withAlpha (0.12f)); g.fillRoundedRectangle (diceFace, 2.0f);
    g.setColour (pipColour); g.drawRoundedRectangle (diceFace, 2.0f, 1.2f);
    
    float cx = diceFace.getCentreX(), cy = diceFace.getCentreY();
    float d = diceFace.getWidth() * 0.25f, r = 1.3f;
    
    g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    g.fillEllipse (cx - d - r, cy - d - r, r * 2.0f, r * 2.0f);
    g.fillEllipse (cx + d - r, cy - d - r, r * 2.0f, r * 2.0f);
    g.fillEllipse (cx - d - r, cy + d - r, r * 2.0f, r * 2.0f);
    g.fillEllipse (cx + d - r, cy + d - r, r * 2.0f, r * 2.0f);
}