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

    // Retrieve full component bounds for the center point calculation
    auto localBounds = slider.getLocalBounds().toFloat();
    float centerX = localBounds.getCentreX();
    float centerY = localBounds.getCentreY();
    
    const bool isMasterKnob = (cid == "masterVelocity" || cid == "masterSwing");
    float knobRadius = 22.0f;

    if (isMasterKnob)
    {
        // Master knobs have no text boxes underneath and center perfectly in their bounds
        centerY = localBounds.getCentreY();
        knobRadius = 38.0f; // Sized for the master knobs
    }
    else
    {
        // Small knobs offset upwards slightly to sit centered above the dynamic textboxes
        centerY = localBounds.getCentreY() - 7.0f;
        knobRadius = 22.0f; // Sized for the side knobs
    }

    float ledRadius = knobRadius + 6.5f;
    float ledDiameter = isMasterKnob ? 4.0f : 3.0f;

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
            // Outer soft radial bloom halo
            float outerRadius = isMasterKnob ? 6.0f : 4.5f;
            juce::ColourGradient glow (activeColor.withAlpha (0.6f), ledX, ledY,
                                       activeColor.withAlpha (0.0f), ledX + outerRadius, ledY + outerRadius,
                                       true);
            g.setGradientFill (glow);
            g.fillEllipse (ledX - outerRadius, ledY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

            // Bright inner core
            float innerRadius = isMasterKnob ? 1.8f : 1.2f;
            g.setColour (juce::Colours::white.interpolatedWith (activeColor, 0.2f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
        else
        {
            // Dim inactive LED
            float innerRadius = 0.8f;
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.25f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
    }

    // 4. Draw pointer needle centered over the asset knob
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path path;
    float pointerThickness = isMasterKnob ? 2.5f : 1.75f;
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

    // Skip drawing standard text for buttons that use pre-baked panel lettering or top buttons
    if (isPresetButton || isStaticTopButton || text == "A" || text == "B")
    {
        return; 
    }

    // Render clean dynamic hardware-style text inside the blank 2x2 corner buttons
    if (isUtilButton || isDiceButton)
    {
        auto bounds = button.getLocalBounds().toFloat();
        // Shift text bounding area down slightly to sit under the dynamic LED bar
        auto textRect = bounds.withTrimmedTop (8.0f);

        g.setColour (button.getToggleState() || button.isDown() || button.isMouseOver() ? juce::Colours::white : juce::Colour (0xFF757575));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
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
            // Active neon cyan glow
            g.setColour (juce::Colour (0xFF00D2FF));
            g.fillRoundedRectangle (ledRect, 1.0f);
            
            // Soft radial glow bloom
            g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.4f));
            g.drawRoundedRectangle (ledRect.expanded (1.5f), 1.5f, 0.75f);
        }
        else
        {
            // Inactive unlit dark-grey slot
            g.setColour (juce::Colour (0xFF181C20));
            g.fillRoundedRectangle (ledRect, 1.0f);
        }
        return; // Retain transparency on the button body to let the designer's 3D cap show through
    }

    // 2. Render background overlays for top-row and preset buttons
    if (text == "Arp")
    {
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.25f));
        g.fillRoundedRectangle (bounds, cornerSize);
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced(0.5f), cornerSize, 1.25f);
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
        const float trackHeight = 3.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;

        float progress = sliderPos / static_cast<float>(width);
        float alphaA = 1.0f - progress;
        g.setColour (t.crossfaderTrackA.withAlpha (alphaA * 0.5f + 0.1f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, sliderPos - static_cast<float>(x), trackHeight, 1.5f);

        float alphaB = progress;
        g.setColour (t.crossfaderTrackB.withAlpha (alphaB * 0.5f + 0.1f));
        g.fillRoundedRectangle (sliderPos, trackY, static_cast<float>(x + width) - sliderPos, trackHeight, 1.5f);

        // Render beveled horizontal crossfader handle matching vertical ones
        const float thumbWidth = 22.0f;
        const float thumbHeight = 18.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        // Soft outer drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (thumbX + 1.0f, thumbY + 2.0f, thumbWidth, thumbHeight, 2.0f);

        // Silver gradient base
        juce::ColourGradient silverBody (juce::Colour (0xFFECEFF1), thumbX, thumbY,
                                         juce::Colour (0xFF90A4AE), thumbX, thumbY + thumbHeight,
                                         false);
        g.setGradientFill (silverBody);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Inner beveled highlights
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
        // Render 3D Silver Metallic vertical fader caps
        const float thumbHeight = 18.0f;
        const float thumbWidth = 26.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        const float thumbY = sliderPos - (thumbHeight * 0.5f);

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
        g.fillRect (grooveX, grooveY + grooveHeight, grooveWidth, 1.0f);
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
    // Draw solid dark background over text boxes to block out any background artifact peeking
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    g.setFont (label.getFont());
    g.setColour (label.findColour (juce::Label::textColourId));

    auto textArea = label.getBorderSize().subtractedFrom (label.getLocalBounds());
    g.drawFittedText (label.getText(), textArea.getX(), textArea.getY(), 
                      textArea.getWidth(), textArea.getHeight(), 
                      label.getJustificationType(), 1);
}