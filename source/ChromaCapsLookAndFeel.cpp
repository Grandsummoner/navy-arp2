#include "ChromaCapsLookAndFeel.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AppTheme.h"

ChromaCapsLookAndFeel::ChromaCapsLookAndFeel (PluginProcessor& p, juce::AudioProcessorEditor* editor)
    : processor (p), parentEditor (editor)
{
}

ChromaCapsLookAndFeel::~ChromaCapsLookAndFeel()
{
}

juce::Slider::SliderLayout ChromaCapsLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    juce::Slider::SliderLayout layout;
    
    // Completely hide standard textboxes to prevent any text overlaps on the vector faceplate
    layout.sliderBounds = slider.getLocalBounds();
    layout.textBoxBounds = juce::Rectangle<int> (0, 0, 0, 0);
    
    return layout;
}

void ChromaCapsLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                              juce::Slider& slider)
{
    juce::String cid = slider.getComponentID();
    int lfoIndex = -1;
    if (cid == "rhythmMorph") lfoIndex = 0;
    else if (cid == "rest") lfoIndex = 1;
    else if (cid == "legato") lfoIndex = 2;
    else if (cid == "rate") lfoIndex = 3;
    else if (cid == "entropy") lfoIndex = 4;
    else if (cid == "harmony") lfoIndex = 5;
    else if (cid == "chaos") lfoIndex = 6;
    else if (cid == "octaves") lfoIndex = 7;

    bool isLfoActive = false;
    float depth = 0.0f;
    double currentPhase = 0.0;
    if (lfoIndex != -1)
    {
        auto* ratePtr = processor.lfoRatePtrs[lfoIndex];
        auto* depthPtr = processor.lfoDepthPtrs[lfoIndex];
        if (ratePtr != nullptr && depthPtr != nullptr)
        {
            int rateChoice = static_cast<int> (ratePtr->load());
            depth = depthPtr->load();
            if (rateChoice > 0 && depth > 0.0f)
            {
                isLfoActive = true;
                currentPhase = processor.lfoPhases[lfoIndex];
            }
        }
    }

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());

    auto localBounds = slider.getLocalBounds().toFloat();
    float centerX = localBounds.getCentreX();
    float centerY = localBounds.getCentreY();
    
    const bool isMasterKnob = (cid == "masterVelocity" || cid == "masterSwing");
    const bool isSmallKnob = !isMasterKnob;

    float knobRadius = isMasterKnob ? 34.0f : 12.0f;
    float ledRadius = isMasterKnob ? (knobRadius + 5.0f) : (knobRadius + 3.5f);
    float ledDiameter = isMasterKnob ? 3.0f : 2.0f;

    // Colour One (Primary Accent): Palette routing matching active chosen panel theme
    juce::Colour activeColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
    if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
    else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

    // Colour Two (LFO Contrast Accent): Used for the Right Column knob halos
    juce::Colour lfoHaloColor = juce::Colour (0xFFFF6D00); // Navy Contrast: Amber/Orange
    if (themeIdx == 1)      lfoHaloColor = juce::Colour (0xFF00E5FF); // Monochrome Contrast: Tech Cyan
    else if (themeIdx == 2) lfoHaloColor = juce::Colour (0xFFD500F9); // Matrix Contrast: Hot Purple/Magenta

    // 1. Draw Custom LFO "Corona / Halo" Underglow Ring (Only for Small Knobs with an active LFO) [3]
    if (isSmallKnob && isLfoActive)
    {
        float glowOuterRadius = 17.0f; // Extends past the 12.0f knob radius to bleed onto the faceplate
        juce::Colour jewelGlowColor = (lfoIndex < 4) ? activeColor : lfoHaloColor;

        // Logarithmic / Square root boost so subtle depth levels (e.g. 10%) are instantly visible
        float boostedDepth = std::sqrt (depth);
        float pulseFactor = 0.25f + 0.75f * static_cast<float> (0.5 + 0.5 * std::sin (currentPhase * juce::MathConstants<double>::twoPi));
        float glowAlpha = boostedDepth * pulseFactor * 0.90f;

        // Multi-stop radial gradient: transparent in the center, peaking at the cap rim, fading out on faceplate
        juce::ColourGradient coronaGrad (juce::Colours::transparentBlack, centerX, centerY,
                                         juce::Colours::transparentBlack, centerX, centerY + glowOuterRadius,
                                         true);
        
        coronaGrad.addColour (0.0,  jewelGlowColor.withAlpha (0.0f));
        coronaGrad.addColour (0.55, jewelGlowColor.withAlpha (0.0f));       // Kept transparent under the center cap
        coronaGrad.addColour (0.75, jewelGlowColor.withAlpha (glowAlpha));  // Peaks right at the cap bezel transition (12.75px)
        coronaGrad.addColour (1.0,  jewelGlowColor.withAlpha (0.0f));       // Diffuses out smoothly on the panel faceplate
        
        g.setGradientFill (coronaGrad);
        g.fillEllipse (centerX - glowOuterRadius, centerY - glowOuterRadius, glowOuterRadius * 2.0f, glowOuterRadius * 2.0f);
    }

    // 2. Draw custom rotating metallic knob cap (Drawn over the gradient to mask its center)
    g.setColour (juce::Colour (0xFF1F2229)); // Bezel base
    g.fillEllipse (centerX - knobRadius, centerY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    juce::ColourGradient steelGrad (juce::Colour (0xFFECEFF1), centerX, centerY - knobRadius,
                                    juce::Colour (0xFF90A4AE), centerX, centerY + knobRadius,
                                    false);
    g.setGradientFill (steelGrad);
    g.fillEllipse (centerX - (knobRadius - 1.0f), centerY - (knobRadius - 1.0f), (knobRadius - 1.0f) * 2.0f, (knobRadius - 1.0f) * 2.0f);

    // Draw active rotating anisotropic light-reflection wedge matching parameter value
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path reflectPath;
    reflectPath.addPieSegment (centerX - knobRadius, centerY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, angle - 0.25f, angle + 0.25f, 0.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.fillPath (reflectPath);

    // [Needle Pointer Deleted] - No pointers drawn over the clean metallic surface.

    // 3. Draw the Concentric LED Ring (Strictly displays exactly ONE lit indicator LED in active theme color) [2]
    int closestLedIndex = static_cast<int> (std::round (sliderPos * 14.0f));

    for (int i = 0; i < 15; ++i)
    {
        float ledAngle = rotaryStartAngle + (static_cast<float> (i) / 14.0f) * (rotaryEndAngle - rotaryStartAngle);
        float ledX = centerX + ledRadius * std::sin (ledAngle) - ledDiameter * 0.5f;
        float ledY = centerY - ledRadius * std::cos (ledAngle) - ledDiameter * 0.5f;

        if (i == closestLedIndex)
        {
            // The single active dial position index dot glows sharply
            float outerRadius = isMasterKnob ? 5.0f : 3.0f;
            juce::ColourGradient glow (activeColor.withAlpha (0.85f), ledX, ledY,
                                       activeColor.withAlpha (0.0f), ledX + outerRadius, ledY + outerRadius,
                                       true);
            g.setGradientFill (glow);
            g.fillEllipse (ledX - outerRadius, ledY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

            float innerRadius = isMasterKnob ? 2.0f : 1.25f;
            g.setColour (juce::Colours::white.interpolatedWith (activeColor, 0.15f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
        else
        {
            // Faint, unlit background dots
            float innerRadius = 0.6f;
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.25f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
    }
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    const juce::String text = button.getButtonText();
    const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");
    const bool isPresetButton = (text == "1" || text == "2" || text == "3" || text == "4" || text == "5" || text == "6" || text == "7" || text == "8");
    const bool isStaticTopButton = (text == "Latch" || text == "Poly" || text == "Freeze" || text == "Seq" || text == "SEQ" || text == "Arp" || text == "ARP");
    const bool isInstrumentButton = (text == "ANALOG" || text == "FM" || text == "STRING" || text == "PULSE");

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());

    // Intercept Sync circular text drawing to keep it clean and unprinted [1.2.3]
    if (text == "Sync")
    {
        return; 
    }

    if (isInstrumentButton)
    {
        bool isToggled = button.getToggleState();
        if (isToggled || button.isDown())
            g.setColour (juce::Colours::white);
        else
            g.setColour (juce::Colour (0xFFA0A5B0));

        g.setFont (juce::FontOptions ("Courier New", 9.5f, juce::Font::bold));
        g.drawFittedText (text, button.getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    if (text == "A" || text == "B")
    {
        bool isLit = false;
        if (text == "A") isLit = !processor.isSceneBActive();
        if (text == "B") isLit = processor.isSceneBActive();

        if (isLit || button.isDown())
            g.setColour (juce::Colours::white); // Bright white inside the red glow
        else
            g.setColour (juce::Colour (0xFF757575)); // Dim white/grey when inactive

        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        g.drawFittedText (text, button.getLocalBounds(), juce::Justification::centred, 1);
        return; 
    }

    if (isPresetButton)
    {
        int pIdx = text.getIntValue() - 1;
        bool isSaved = false;
        int flashTimer = 0;
        int flashType = 0;

        if (auto* editor = dynamic_cast<PluginEditor*> (parentEditor))
        {
            if (pIdx >= 0 && pIdx < 8)
            {
                isSaved = editor->processor.presetSlotsSaved[pIdx];
                flashTimer = editor->presetFlashTimer[pIdx];
                flashType = editor->presetFlashType[pIdx];
            }
        }

        juce::Colour storedColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
        if (themeIdx == 1)      storedColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
        else if (themeIdx == 2) storedColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

        juce::Colour textCol = juce::Colour (0xFF4F525D); // State 1 (Empty): Dim grey default

        if (flashTimer > 0)
        {
            bool flashOn = ((flashTimer / 4) % 2 == 1);
            if (flashOn)
            {
                // State 3 (Save flash): Emerald Green | State 4 (Recall flash): Bright Yellow/Amber
                textCol = (flashType == 1) ? juce::Colour::fromString ("#FF00E676") : juce::Colour::fromString ("#FFFFB300");
            }
            else if (isSaved)
            {
                textCol = storedColor; // State 2 (Stored): Match active theme
            }
        }
        else if (isSaved)
        {
            textCol = button.isMouseOver() ? juce::Colours::white : storedColor.withAlpha (0.85f);
        }
        else if (button.isMouseOver())
        {
            textCol = juce::Colours::lightgrey;
        }

        g.setColour (textCol);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawFittedText (text, button.getLocalBounds(), juce::Justification::centred, 1);
        return;
    }

    if (isUtilButton || isDiceButton)
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto textRect = bounds.withTrimmedTop (6.0f); 

        g.setColour (button.getToggleState() || button.isDown() || button.isMouseOver() ? juce::Colours::white : juce::Colour (0xFF757575));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawFittedText (text, textRect.toNearestInt(), juce::Justification::centred, 1);
    }
    else if (isStaticTopButton)
    {
        auto bounds = button.getLocalBounds().toFloat();
        if (button.getToggleState() || button.isDown())
        {
            juce::Colour activeColor = juce::Colour (0xFF00E5FF);
            if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1);
            else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66);
            g.setColour (activeColor);
        }
        else
            g.setColour (juce::Colour (0xFFA0A5B0));

        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centred, 1);
    }
}

void ChromaCapsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float cornerSize = 4.0f;
    const juce::String text = button.getButtonText();
    const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");
    const bool isStaticTopButton = (text == "Latch" || text == "Poly" || text == "Freeze" || text == "Seq" || text == "SEQ" || text == "Arp" || text == "ARP");
    const bool isPresetButton = (text == "1" || text == "2" || text == "3" || text == "4" || text == "5" || text == "6" || text == "7" || text == "8");
    const bool isInstrumentButton = (text == "ANALOG" || text == "FM" || text == "STRING" || text == "PULSE");

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());

    // Color routing matching active chosen panel theme
    juce::Colour activeColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
    if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
    else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

    // Renders the Sync button as a sleek circular toggle [1.2.3]
    if (text == "Sync")
    {
        float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto circleBounds = juce::Rectangle<float> (bounds.getCentreX() - radius, bounds.getCentreY() - radius, radius * 2.0f, radius * 2.0f);
        
        bool isLit = button.getToggleState() || shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
        if (isLit)
        {
            // Soft halo glow inside circular bezel [1.2.3]
            g.setColour (activeColor.withAlpha (0.25f));
            g.fillEllipse (circleBounds);
            g.setColour (activeColor.withAlpha (0.8f));
            g.drawEllipse (circleBounds.reduced (0.5f), 1.25f);

            // Draw glowing centered micro-LED dot [1.2.3]
            float dotRad = 2.5f;
            g.setColour (juce::Colours::white);
            g.fillEllipse (bounds.getCentreX() - dotRad, bounds.getCentreY() - dotRad, dotRad * 2.0f, dotRad * 2.0f);
        }
        else
        {
            // Faint unlit circular bezel
            g.setColour (juce::Colour (0xFF181C20));
            g.fillEllipse (circleBounds);
            g.setColour (juce::Colour (0xFF37474F).withAlpha (0.4f));
            g.drawEllipse (circleBounds.reduced (0.5f), 1.0f);

            // Dim centered micro-LED dot [1.2.3]
            float dotRadius = 2.5f;
            g.setColour (juce::Colour (0xFF4F525D));
            g.fillEllipse (bounds.getCentreX() - dotRadius, bounds.getCentreY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        }
        return;
    }

    if (isInstrumentButton)
    {
        bool isToggled = button.getToggleState();
        if (isToggled)
        {
            // Sleek flat high-tech active button block [3]
            g.setColour (activeColor.withAlpha (0.18f));
            g.fillRoundedRectangle (bounds, 2.0f);
            g.setColour (activeColor);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 1.25f);
        }
        else
        {
            // Sleek dark unselected button block [3]
            g.setColour (juce::Colour (0xFF0E1116));
            g.fillRoundedRectangle (bounds, 2.0f);
            g.setColour (juce::Colour (0xFF1E2229));
            g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 1.0f);
        }
        return;
    }

    if (isUtilButton || isDiceButton)
    {
        float ledWidth = bounds.getWidth() * 0.5f;
        float ledX = (bounds.getWidth() - ledWidth) * 0.5f;
        auto ledRect = juce::Rectangle<float> (ledX, 3.0f, ledWidth, 2.0f);

        bool isLit = button.getToggleState() || shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;

        if (auto* editor = dynamic_cast<PluginEditor*> (parentEditor))
        {
            if (&button == &(editor->saveButton) && editor->saveFlashTimer > 0)
                isLit = ((editor->saveFlashTimer / 4) % 2 == 1);
            else if (&button == &(editor->recallButton) && editor->recallFlashTimer > 0)
                isLit = ((editor->recallFlashTimer / 4) % 2 == 1);
            else if (&button == &(editor->copyButton) && editor->copyFlashTimer > 0)
                isLit = ((editor->copyFlashTimer / 4) % 2 == 1);
            else if (&button == &(editor->initButton) && editor->initFlashTimer > 0)
                isLit = ((editor->initFlashTimer / 4) % 2 == 1);
        }

        juce::Colour ledColor = juce::Colour (0xFF00D2FF); // Default Cyan
        if (text == "Save") ledColor = juce::Colour::fromString ("#FFFFB300");        // Gold/Amber
        else if (text == "Recall") ledColor = juce::Colour::fromString ("#FF00E676");   // Emerald Green
        else if (text == "Copy") ledColor = juce::Colour::fromString ("#FF00B0FF");     // Sky Blue
        else if (text == "Init") ledColor = juce::Colour::fromString ("#FFFF1744");     // Crimson/Red
        else if (text == "Melo") ledColor = juce::Colour::fromString ("#FFD500F9");     // Purple/Violet
        else if (text == "Arti") ledColor = ledColor = juce::Colour::fromString ("#FFFF6D00"); // Orange/Coral
        else if (text == "Time") ledColor = ledColor = juce::Colour::fromString ("#FFFFEA00"); // Bright Yellow
        else if (text == "Navy") ledColor = ledColor = juce::Colour::fromString ("#FF18FFFF"); // Electric Teal/Cyan

        if (isLit)
        {
            g.setColour (ledColor);
            g.fillRoundedRectangle (ledRect, 1.0f);
            g.setColour (ledColor.withAlpha (0.4f));
            g.drawRoundedRectangle (ledRect.expanded (1.5f), 1.5f, 0.75f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF181C20));
            g.fillRoundedRectangle (ledRect, 1.0f);
        }
        return; 
    }

    if (text == "Arp")
    {
        g.setColour (activeColor.withAlpha (0.25f));
        g.fillRoundedRectangle (bounds, cornerSize);
        g.setColour (activeColor.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced(0.5f), cornerSize, 1.25f);
        return;
    }
    
    if (text == "A" || text == "B")
    {
        bool isLit = false;
        if (text == "A") isLit = !processor.isSceneBActive();
        if (text == "B") isLit = processor.isSceneBActive();

        if (isLit || shouldDrawButtonAsDown) {
            // Distinct red glowing background & solid red border
            g.setColour (juce::Colour (0xFFFF0000).withAlpha (0.15f));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (juce::Colour (0xFFFF0000).withAlpha (0.8f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.5f);
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (juce::Colour (0xFFFF0000).withAlpha (0.2f)); // Subtle red outline on hover
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
        } else {
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.4f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
        }
        return;
    }

    if (isPresetButton) {
        int pIdx = text.getIntValue() - 1;
        bool isSaved = false;
        int flashTimer = 0;
        int flashType = 0;

        if (auto* editor = dynamic_cast<PluginEditor*> (parentEditor)) {
            if (pIdx >= 0 && pIdx < 8) {
                isSaved = editor->processor.presetSlotsSaved[pIdx];
                flashTimer = editor->presetFlashTimer[pIdx];
                flashType = editor->presetFlashType[pIdx];
            }
        }

        juce::Colour storedColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
        if (themeIdx == 1)      storedColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
        else if (themeIdx == 2) storedColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

        auto innerBounds = bounds.reduced (1.0f);
        juce::Colour outlineCol = juce::Colour (0xFF1F2229).withAlpha (0.4f); // State 1 (Empty): Dim border
        juce::Colour fillCol = juce::Colours::transparentBlack;

        if (flashTimer > 0) {
            bool flashOn = ((flashTimer / 4) % 2 == 1);
            if (flashOn) {
                // State 3 (Save flash): Emerald Green | State 4 (Recall flash): Bright Yellow/Amber
                outlineCol = (flashType == 1) ? juce::Colour::fromString ("#FF00E676") : juce::Colour::fromString ("#FFFFB300");
                fillCol = outlineCol.withAlpha (0.15f);
            } else if (isSaved) {
                outlineCol = storedColor.withAlpha (0.4f); // State 2 (Stored): Theme match
            }
        } else if (isSaved) {
            outlineCol = storedColor.withAlpha (0.35f); // State 2 (Stored): Theme match
            if (button.isMouseOver())
                outlineCol = storedColor.withAlpha (0.7f);
        } else if (button.isMouseOver()) {
            outlineCol = juce::Colours::white.withAlpha (0.25f);
        }

        if (button.isDown()) {
            fillCol = juce::Colours::white.withAlpha (0.05f);
            outlineCol = storedColor.withAlpha (0.9f);
        }

        if (fillCol != juce::Colours::transparentBlack) {
            g.setColour (fillCol);
            g.fillRoundedRectangle (innerBounds, cornerSize);
        }

        g.setColour (outlineCol);
        g.drawRoundedRectangle (innerBounds, cornerSize, 1.25f);
        return;
    }

    if (isStaticTopButton)
    {
        if (button.getClickingTogglesState() && button.getToggleState())
        {
            g.setColour (activeColor.withAlpha (0.25f));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (activeColor.withAlpha (0.6f));
            g.drawRoundedRectangle (bounds.reduced(0.5f), cornerSize, 1.25f);
        }
        else if (shouldDrawButtonAsDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }
        else if (shouldDrawButtonAsHighlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }
    }
}

void ChromaCapsLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos, sliderPos);

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    const bool isVertical = (style == juce::Slider::LinearVertical);
    const juce::String cid = slider.getComponentID();

    if (cid == "morph")
    {
        const float trackHeight = 6.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;

        float startX = static_cast<float>(x);
        float endX = static_cast<float>(x + width);
        float totalW = static_cast<float>(width);

        // Visual travel compressed by exactly 5% on both ends to hide the orange strip at extremes [1.2.3]
        float margin = totalW * 0.05f;
        float activeStartX = startX + margin;
        float activeEndX = endX - margin;
        float activeW = totalW - (margin * 2.0f);

        // Continuous Value Proportions mapping based strictly on slider getValue() values
        float progress = static_cast<float>((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        progress = juce::jlimit (0.0f, 1.0f, progress);

        float visualThumbX = activeStartX + progress * activeW;

        float alphaA = 1.0f - progress;
        g.setColour (t.crossfaderTrackA.withAlpha (alphaA * 0.6f + 0.15f));
        g.fillRoundedRectangle (startX, trackY, visualThumbX - startX, trackHeight, 2.0f);

        float alphaB = progress;
        g.setColour (t.crossfaderTrackB.withAlpha (alphaB * 0.6f + 0.15f));
        g.fillRoundedRectangle (visualThumbX, trackY, endX - visualThumbX, trackHeight, 2.0f);

        // Crossfader cap drawn bigger and taller
        const float thumbWidth = 32.0f;  
        const float thumbHeight = 32.0f; 
        const float thumbX = visualThumbX - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY + 2.0f, thumbWidth, thumbHeight, 1.5f);

        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, thumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, thumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (thumbX + 0.5f, thumbY + 0.5f, thumbWidth - 1.0f, thumbHeight - 1.0f, 1.5f, 1.0f);
        g.setColour (juce::Colour (0xFF37474F).withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f, 1.0f);

        const float grooveWidth = 4.0f;
        const float grooveHeight = thumbHeight - 4.0f;
        const float grooveX = thumbX + (thumbWidth - grooveWidth) * 0.5f;
        const float grooveY = thumbY + 2.0f;

        g.setColour (juce::Colour (0xFF212121));
        g.fillRect (grooveX, grooveY, grooveWidth, grooveHeight);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillRect (grooveX + grooveWidth, grooveY, 1.0f, grooveHeight);
    }
    else if (isVertical)
    {
        const float thumbHeight = 12.0f;
        const float thumbWidth = 18.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        
        // Continuous Value Proportions mapping strictly via Slider value
        float progress = static_cast<float>((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        progress = juce::jlimit (0.0f, 1.0f, progress);

        float clampedThumbY = static_cast<float>(y + height - thumbHeight) - progress * static_cast<float>(height - thumbHeight);
        clampedThumbY = juce::jlimit (static_cast<float>(y), static_cast<float>(y + height - thumbHeight), clampedThumbY);

        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (thumbX + 1.0f, clampedThumbY + 1.5f, thumbWidth, thumbHeight, 1.5f);

        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, clampedThumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, clampedThumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, clampedThumbY, thumbWidth, thumbHeight, 1.5f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (thumbX + 0.5f, clampedThumbY + 0.5f, thumbWidth - 1.0f, thumbHeight - 1.0f, 1.5f, 1.0f);
        g.setColour (juce::Colour (0xFF37474F).withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, clampedThumbY, thumbWidth, thumbHeight, 1.5f, 1.0f);

        const float grooveHeight = 3.0f;
        const float grooveWidth = thumbWidth - 4.0f;
        const float grooveX = thumbX + 2.0f;
        const float grooveY = clampedThumbY + (thumbHeight - grooveHeight) * 0.5f;

        g.setColour (juce::Colour (0xFF212121));
        g.fillRect (grooveX, grooveY, grooveWidth, grooveHeight);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillRect (grooveX + grooveWidth, grooveY, 1.0f, grooveHeight);
    }
    else // Horizontal voice sliders in the left panel
    {
        const float trackHeight = 3.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;
        const float startX = static_cast<float>(x);
        const float totalW = static_cast<float>(width);

        // Fetch theme colors dynamically
        juce::Colour activeColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
        if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
        else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

        // Progress bar proportion
        float progress = static_cast<float>((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
        progress = juce::jlimit (0.0f, 1.0f, progress);
        float visualThumbX = startX + progress * totalW;

        // Draw recessed track background
        g.setColour (juce::Colour (0xFF0B0E14));
        g.fillRoundedRectangle (startX, trackY, totalW, trackHeight, 1.5f);
        g.setColour (juce::Colour (0xFF263238).withAlpha (0.25f));
        g.drawRoundedRectangle (startX, trackY, totalW, trackHeight, 1.5f, 1.0f);

        // Draw active glowing track fill
        g.setColour (activeColor.withAlpha (0.75f));
        g.fillRoundedRectangle (startX, trackY, visualThumbX - startX, trackHeight, 1.5f);
        
        // Glow effect
        g.setColour (activeColor.withAlpha (0.12f));
        g.drawRoundedRectangle (startX, trackY - 1.0f, visualThumbX - startX, trackHeight + 2.0f, 1.5f, 1.25f);

        // Custom capsule metallic thumb
        const float thumbWidth = 10.0f;
        const float thumbHeight = 14.0f;
        const float thumbX = visualThumbX - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        // Shadow
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY + 1.0f, thumbWidth, thumbHeight, 1.5f);

        // Machined Silver metallic body
        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, thumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, thumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f);

        // Bezel outline
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (thumbX + 0.5f, thumbY + 0.5f, thumbWidth - 1.0f, thumbHeight - 1.0f, 1.5f, 0.75f);
        g.setColour (juce::Colour (0xFF37474F).withAlpha (0.35f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f, 0.75f);

        // Center glowing micro indicator
        float dotSize = 2.0f;
        g.setColour (activeColor);
        g.fillEllipse (visualThumbX - (dotSize * 0.5f), thumbY + (thumbHeight - dotSize) * 0.5f, dotSize, dotSize);
    }
}

void ChromaCapsLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    g.setFont (label.getFont());
    g.setColour (label.findColour (juce::Label::textColourId));
    
    auto textArea = label.getBorderSize().subtractedFrom (label.getLocalBounds());
    g.drawFittedText (label.getText(), textArea.getX(), textArea.getY(), 
                      textArea.getWidth(), textArea.getHeight(), 
                      label.getJustificationType(), 1);
}