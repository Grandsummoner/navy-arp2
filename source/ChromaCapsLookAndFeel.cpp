--- START OF FILE ChromaCapsLookAndFeel.cpp ---

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
    auto localBounds = slider.getLocalBounds();
    
    const bool isMasterKnob = (slider.getComponentID() == "masterVelocity" || slider.getComponentID() == "masterSwing");

    if (isMasterKnob)
    {
        layout.sliderBounds = localBounds;
        layout.textBoxBounds = juce::Rectangle<int> (0, 0, 0, 0);
    }
    else
    {
        // Small knobs total height is 116.
        // Allocate top 86px to the dial and bottom 26px to the textbox (with a 4px gap)
        layout.sliderBounds = juce::Rectangle<int> (0, 0, localBounds.getWidth(), 86);
        layout.textBoxBounds = juce::Rectangle<int> (0, 90, localBounds.getWidth(), 26);
    }
    
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

    // 1. Calculate LFO status and dynamic LED target value
    float targetVal = sliderPos;
    if (lfoIndex != -1)
    {
        auto* ratePtr = processor.lfoRatePtrs[lfoIndex];
        auto* depthPtr = processor.lfoDepthPtrs[lfoIndex];
        
        if (ratePtr != nullptr && depthPtr != nullptr)
        {
            int rateChoice = static_cast<int> (ratePtr->load());
            float depth = depthPtr->load();
            if (rateChoice > 0 && depth > 0.02f)
            {
                double currentPhase = processor.lfoPhases[lfoIndex];
                targetVal = sliderPos + (static_cast<float> (std::sin (currentPhase * juce::MathConstants<double>::twoPi)) * depth * 0.5f);
                targetVal = juce::jlimit (0.0f, 1.0f, targetVal);
            }
        }
    }
    int litCount = static_cast<int> (std::round (targetVal * 15.0f));

    // 2. Fetch Theme details
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    auto localBounds = slider.getLocalBounds().toFloat();
    float centerX = localBounds.getCentreX();
    float centerY = localBounds.getCentreY();
    
    const bool isMasterKnob = (cid == "masterVelocity" || cid == "masterSwing");
    float knobRadius = 24.0f; // Sized exactly for the small knobs at new 1519x1035 resolution

    if (isMasterKnob)
    {
        knobRadius = 50.0f; // Sized for the big master master velocity/swing knobs
    }
    else
    {
        // Small knobs utilize the full component vertical center of the 86px rotary layout
        centerY = 43.0f;
        knobRadius = 24.0f; 
    }

    // Symmetrical LED rings tightly wrapped to your knob caps
    float ledRadius = isMasterKnob ? (knobRadius + 12.0f) : (knobRadius + 8.5f);
    float ledDiameter = isMasterKnob ? 5.0f : 3.0f;

    bool isLeftKnob = (cid == "rhythmMorph" || cid == "rest" || cid == "legato" || cid == "rate" || cid == "masterVelocity");
    juce::Colour activeColor = isLeftKnob ? t.knobFillLeft : t.knobFillRight;

    // 3. Draw 15 surrounding LEDs with Neon Glow Radial Halo
    for (int i = 0; i < 15; ++i)
    {
        float angle = rotaryStartAngle + (static_cast<float> (i) / 14.0f) * (rotaryEndAngle - rotaryStartAngle);
        float ledX = centerX + ledRadius * std::sin (angle) - ledDiameter * 0.5f;
        float ledY = centerY - ledRadius * std::cos (angle) - ledDiameter * 0.5f;

        if (i < litCount)
        {
            float outerRadius = isMasterKnob ? 8.5f : 4.5f;
            juce::ColourGradient glow (activeColor.withAlpha (0.6f), ledX, ledY,
                                       activeColor.withAlpha (0.0f), ledX + outerRadius, ledY + outerRadius,
                                       true);
            g.setGradientFill (glow);
            g.fillEllipse (ledX - outerRadius, ledY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

            float innerRadius = isMasterKnob ? 2.5f : 1.2f;
            g.setColour (juce::Colours::white.interpolatedWith (activeColor, 0.2f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
        else
        {
            float innerRadius = 0.8f;
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.25f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
    }

    // 4. Draw pointer needle centered over the asset knob
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path path;
    float pointerThickness = isMasterKnob ? 3.5f : 2.0f;
    float pointerLength = knobRadius * 0.65f;
    path.addRectangle (-pointerThickness * 0.5f, -knobRadius + 2.0f, pointerThickness, pointerLength);
    path.applyTransform (juce::AffineTransform::rotation (angle).translated (centerX, centerY));
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.fillPath (path);
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    const juce::String text = button.getButtonText();
    const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");
    const bool isPresetButton = (text == "1" || text == "2" || text == "3" || text == "4" || text == "5" || text == "6" || text == "7" || text == "8");
    const bool isStaticTopButton = (text == "Latch" || text == "Poly" || text == "Freeze" || text == "Seq" || text == "SEQ" || text == "Arp" || text == "ARP");

    if (isPresetButton)
    {
        return; 
    }

    // Render clean dynamic hardware-style text inside the blank 2x2 corner buttons
    if (isUtilButton || isDiceButton)
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto textRect = bounds.withTrimmedTop (8.0f); // Shift down to clear the LED indicator bar

        g.setColour (button.getToggleState() || button.isDown() || button.isMouseOver() ? juce::Colours::white : juce::Colour (0xFF757575));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawFittedText (text, textRect.toNearestInt(), juce::Justification::centred, 1);
    }
    else if (isStaticTopButton)
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Render top bar buttons with high-contrast unlit grey (0xFFA0A5B0) and bold font size
        if (button.getToggleState() || button.isDown())
            g.setColour (juce::Colour (0xFF00D2FF));
        else
            g.setColour (juce::Colour (0xFFA0A5B0));

        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (text, bounds.toNearestInt(), juce::Justification::centred, 1);
    }
    else if (text == "A" || text == "B")
    {
        auto textRect = button.getLocalBounds().toFloat();

        bool isSceneActive = false;
        if (text == "A") isSceneActive = !processor.isSceneBActive();
        else if (text == "B") isSceneActive = processor.isSceneBActive();
        
        if (isSceneActive || button.isDown())
            g.setColour (juce::Colour (0xFF00D2FF)); // Glowing Active Cyan
        else
            g.setColour (juce::Colour (0xFF757575)); // Inactive Grey
            
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawFittedText (text, textRect.toNearestInt(), juce::Justification::centred, 1);
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

    // 1. Process active 2x2 grid utility and dice states
    if (isUtilButton || isDiceButton)
    {
        // Draw the LED bar centered at the top of the blank buttons
        float ledWidth = bounds.getWidth() * 0.5f;
        float ledX = (bounds.getWidth() - ledWidth) * 0.5f;
        auto ledRect = juce::Rectangle<float> (ledX, 4.0f, ledWidth, 2.5f);

        bool isLit = button.getToggleState() || shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;

        // Process flash timers from editor
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

        if (isLit)
        {
            g.setColour (juce::Colour (0xFF00D2FF));
            g.fillRoundedRectangle (ledRect, 1.0f);
            
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.4f));
            g.drawRoundedRectangle (ledRect.expanded (1.5f), 1.5f, 0.75f);
        }
        else
        {
            g.setColour (juce::Colour (0xFF181C20));
            g.fillRoundedRectangle (ledRect, 1.0f);
        }
        return; 
    }

    // 2. Render background overlays for top-row, scene and preset buttons
    if (text == "Arp")
    {
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.25f));
        g.fillRoundedRectangle (bounds, cornerSize);
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced(0.5f), cornerSize, 1.25f);
    }
    else if (text == "A" || text == "B")
    {
        // Symmetrically light up Button A and B frames when active scene focus is held
        bool isSceneActive = false;
        if (text == "A") isSceneActive = !processor.isSceneBActive();
        else if (text == "B") isSceneActive = processor.isSceneBActive();
        
        if (isSceneActive || shouldDrawButtonAsDown)
        {
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.15f));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.5f));
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.25f);
        }
        return;
    }
    else if (isStaticTopButton)
    {
        if (button.getClickingTogglesState() && button.getToggleState())
        {
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.25f));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.6f));
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
    else if (button.getClickingTogglesState() && button.getToggleState())
    {
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.25f));
        g.fillRoundedRectangle (bounds, cornerSize);
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.6f));
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

void ChromaCapsLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    const bool isVertical = (style == juce::Slider::LinearVertical);
    const juce::String cid = slider.getComponentID();

    if (cid == "morph") // Active Glow Vector Crossfader centerpiece
    {
        const float trackHeight = 8.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;

        float progress = (sliderPos - static_cast<float>(x)) / static_cast<float>(width);
        progress = juce::jlimit (0.0f, 1.0f, progress);

        float alphaA = 1.0f - progress;
        g.setColour (t.crossfaderTrackA.withAlpha (alphaA * 0.6f + 0.15f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, sliderPos - static_cast<float>(x), trackHeight, 3.0f);

        float alphaB = progress;
        g.setColour (t.crossfaderTrackB.withAlpha (alphaB * 0.6f + 0.15f));
        g.fillRoundedRectangle (sliderPos, trackY, static_cast<float>(x + width) - sliderPos, trackHeight, 3.0f);

        // Render horizontal metallic crossfader cap
        const float thumbWidth = 22.0f;
        const float thumbHeight = 18.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        // Outer drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY + 2.0f, thumbWidth, thumbHeight, 2.0f);

        // Silver gradient body
        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, thumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, thumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // 3D Bevel highlight
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (thumbX + 0.5f, thumbY + 0.5f, thumbWidth - 1.0f, thumbHeight - 1.0f, 2.0f, 1.0f);
        g.setColour (juce::Colour (0xFF37474F).withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);

        // Vertical central groove stripe
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
        // Render vertical upfader caps (26px wide, 18px tall)
        const float thumbHeight = 18.0f;
        const float thumbWidth = 26.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        
        // Exact travel restriction to sit between top absolute dot and bottom absolute dot of fader tracks
        float travelRange = maxSliderPos - minSliderPos;
        float progress = (sliderPos - minSliderPos) / (travelRange > 0.0f ? travelRange : 1.0f);
        
        float restrictedMinY = 22.0f;
        float restrictedMaxY = 102.0f;
        float restrictedSliderPos = restrictedMinY + progress * (restrictedMaxY - restrictedMinY);
        
        const float thumbY = restrictedSliderPos - (thumbHeight * 0.5f);

        // Soft Outer drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY + 2.0f, thumbWidth, thumbHeight, 2.0f);

        // Silver metallic body gradient
        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, thumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, thumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Inner Highlights and Bevel Borders
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (thumbX + 0.5f, thumbY + 0.5f, thumbWidth - 1.0f, thumbHeight - 1.0f, 2.0f, 1.0f);
        g.setColour (juce::Colour (0xFF37474F).withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);

        // Horizontal slot notch down the center of the vertical upfader
        const float grooveHeight = 4.0f;
        const float grooveWidth = thumbWidth - 4.0f;
        const float grooveX = thumbX + 2.0f;
        const float grooveY = thumbY + (thumbHeight - grooveHeight) * 0.5f;

        g.setColour (juce::Colour (0xFF212121));
        g.fillRect (grooveX, grooveY, grooveWidth, grooveHeight);

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.fillRect (grooveX + grooveWidth, grooveY, 1.0f, grooveHeight);
    }
    else
    {
        const float thumbWidth = 14.0f;
        const float thumbHeight = 22.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        g.setColour (t.faderCap);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f);

        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + (thumbWidth * 0.5f) - 1.0f, thumbY + 1.0f, 2.0f, thumbHeight - 2.0f);
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
--- END OF FILE ChromaCapsLookAndFeel.cpp ---