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

    // 1. Calculate LFO status and dynamic LED target value [43]
    float targetVal = sliderPos;
    if (lfoIndex != -1)
    {
        auto* ratePtr = processor.lfoRatePtrs[lfoIndex];
        auto* depthPtr = processor.lfoDepthPtrs[lfoIndex];
        
        // Safety check: only read if the pointers are initialized [43]
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

    float centerX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    float centerY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    float knobRadius = juce::jmin (static_cast<float> (width), static_cast<float> (height)) * 0.28f;
    float ledRadius = knobRadius + 8.0f;

    bool isLeftKnob = (cid == "rhythmMorph" || cid == "rest" || cid == "legato" || cid == "rate");
    juce::Colour activeColor = isLeftKnob ? t.knobFillLeft : t.knobFillRight;

    // 3. Draw 15 surrounding LEDs with Neon Glow Radial Halo [43]
    for (int i = 0; i < 15; ++i)
    {
        float angle = rotaryStartAngle + (static_cast<float> (i) / 14.0f) * (rotaryEndAngle - rotaryStartAngle);
        float ledX = centerX + ledRadius * std::sin (angle);
        float ledY = centerY - ledRadius * std::cos (angle);

        if (i < litCount)
        {
            // Draw Level 3: Outer soft radial bloom halo [43]
            float outerRadius = 4.5f;
            juce::ColourGradient glow (activeColor.withAlpha (0.45f), ledX, ledY,
                                       activeColor.withAlpha (0.0f), ledX + outerRadius, ledY + outerRadius,
                                       true);
            g.setGradientFill (glow);
            g.fillEllipse (ledX - outerRadius, ledY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

            // Draw Level 2: Bright inner core [43]
            float innerRadius = 1.25f;
            g.setColour (juce::Colours::white.interpolatedWith (activeColor, 0.3f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
        else
        {
            // Dim inactive LED
            float innerRadius = 1.0f;
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.35f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
    }

    // 4. Draw Matte Black Encoders with a clean white pointer line
    g.setColour (juce::Colour (0xFF121417));
    g.fillEllipse (centerX - knobRadius, centerY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (centerX - knobRadius, centerY - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path path;
    float pointerThickness = 1.75f;
    float pointerLength = knobRadius * 0.75f;
    path.addRectangle (-pointerThickness * 0.5f, -knobRadius, pointerThickness, pointerLength);
    path.applyTransform (juce::AffineTransform::rotation (angle).translated (centerX, centerY));
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.fillPath (path);
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto& lf = button.getLookAndFeel();
    
    const juce::String text = button.getButtonText();
    const bool isButtonA = (text == "A");
    const bool isButtonB = (text == "B");
    const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");

    // Scale down text inside 2x2 grid buttons to keep them elegant and clean [43]
    if (isUtilButton || isDiceButton)
    {
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    }
    else
    {
        g.setFont (lf.getTextButtonFont (button, button.getHeight()));
    }

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    
    if (themeIdx == 1) // Skyline Eurorack (Light Beige Theme)
    {
        if (isButtonA || isButtonB)
        {
            const bool isSceneB = processor.isSceneBActiveAnchor.load();
            const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);
            
            if (isActiveAnchor)
                g.setColour (juce::Colour (0xFFFF5533)); // High contrast orange-red text
            else
                g.setColour (juce::Colour (0xFF55555C));
        }
        else if (isUtilButton)
        {
            if (button.getToggleState())
                g.setColour (juce::Colour (0xFFFF5533)); // Red-orange text when active
            else
                g.setColour (juce::Colours::white); // High contrast white on dark charcoal
        }
        else if (isDiceButton)
        {
            g.setColour (juce::Colours::white); // Clean white text on dark charcoal
        }
        else if (button.getClickingTogglesState() && button.getToggleState())
        {
            g.setColour (juce::Colour (0xFFFF5533)); // Orange-red text on active toggle
        }
        else
        {
            g.setColour (button.findColour (juce::TextButton::textColourOffId).withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
        }
    }
    else // Dark Themes
    {
        if (isButtonA || isButtonB)
        {
            const bool isSceneB = processor.isSceneBActiveAnchor.load();
            const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);
            
            if (isActiveAnchor)
                g.setColour (juce::Colours::black); 
            else
                g.setColour (juce::Colours::white.withAlpha (0.7f));
        }
        else if (button.getClickingTogglesState() && button.getToggleState())
        {
            g.setColour (juce::Colours::black); // Black text on active Cyan toggles
        }
        else
        {
            g.setColour (button.findColour (juce::TextButton::textColourOffId).withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
        }
    }
    
    const int indent = 2;
    g.drawFittedText (button.getButtonText(), 
                      indent, indent, 
                      button.getWidth() - 2 * indent, 
                      button.getHeight() - 2 * indent, 
                      juce::Justification::centred, 2);
}

void ChromaCapsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float cornerSize = 4.0f;
    auto baseColour = backgroundColour;
    bool isFlashing = false;

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());

    // 1. Process active preset and utility flash states
    if (auto* editor = dynamic_cast<PluginEditor*> (parentEditor))
    {
        for (int i = 0; i < 8; ++i)
        {
            if (&button == &(editor->presetButtons[i]))
            {
                if (editor->presetFlashTimer[i] > 0)
                {
                    isFlashing = true;
                    int flashState = (editor->presetFlashTimer[i] / 4) % 2; // Alternates every 4 frames (3-blink on/off)
                    if (flashState == 1)
                    {
                        if (editor->presetFlashType[i] == 1) // Save flash
                            baseColour = juce::Colour (0xFFFF5533); // Red-Orange flash
                        else if (editor->presetFlashType[i] == 2) // Recall flash
                            baseColour = juce::Colour (0xFF00D2FF); // Ice Blue/Cyan flash
                    }
                    else
                    {
                        baseColour = backgroundColour;
                    }
                }
                break;
            }
        }

        if (!isFlashing)
        {
            if (&button == &(editor->saveButton) && editor->saveFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->saveFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
            else if (&button == &(editor->recallButton) && editor->recallFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->recallFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFF00D2FF) : backgroundColour;
            }
            else if (&button == &(editor->copyButton) && editor->copyFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->copyFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFFFFFF33) : backgroundColour;
            }
            else if (&button == &(editor->initButton) && editor->initFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->initFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
            else if (&button == &(editor->sceneAButton) && editor->sceneAFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->sceneAFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
            else if (&button == &(editor->sceneBButton) && editor->sceneBFlashTimer > 0)
            {
                isFlashing = true;
                int flashState = (editor->sceneBFlashTimer / 4) % 2;
                baseColour = (flashState == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
        }
    }

    // 2. Render standard static or active background based on theme selections
    if (!isFlashing)
    {
        const juce::String text = button.getButtonText();
        const bool isButtonA = (text == "A");
        const bool isButtonB = (text == "B");
        const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
        const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");

        if (themeIdx == 1) // Skyline Eurorack (Light Beige Theme)
        {
            if (isButtonA || isButtonB)
            {
                const bool isSceneB = processor.isSceneBActiveAnchor.load();
                const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);

                if (isActiveAnchor)
                    baseColour = juce::Colour (0xFFFFE5DD); // Gentle light red-orange highlight background
                else
                    baseColour = juce::Colour (0xFFD8D4CC);
            }
            else if (isUtilButton || isDiceButton)
            {
                // Both utility and dice grids share the same dark charcoal background
                if (isUtilButton && button.getToggleState())
                    baseColour = juce::Colour (0xFFFFE5DD); // Gentle highlight background when active
                else
                    baseColour = juce::Colour (0xFF2D313A); // Shared dark charcoal base
            }
            else if (button.getClickingTogglesState())
            {
                if (button.getToggleState())
                    baseColour = juce::Colour (0xFFFFE5DD); // Gentle highlight background
                else
                    baseColour = juce::Colour (0xFFD8D4CC);
            }
        }
        else // Dark Themes
        {
            if (isButtonA || isButtonB)
            {
                const bool isSceneB = processor.isSceneBActiveAnchor.load();
                const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);

                if (isActiveAnchor)
                    baseColour = juce::Colour (0xFF00D2FF); // Active anchor Cyan
                else
                    baseColour = juce::Colour (0xFF242730);
            }
            else if (button.getClickingTogglesState())
            {
                if (button.getToggleState())
                    baseColour = juce::Colour (0xFF00D2FF); // Cyan active toggle
                else
                    baseColour = juce::Colour (0xFF2D313A);
            }
        }
    }

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, cornerSize);

    // 3. Draw thin outline (Only active on the Skyline theme, completely borderless on dark themes) [43]
    if (themeIdx == 1 && !isFlashing)
    {
        const juce::String text = button.getButtonText();
        const bool isButtonA = (text == "A");
        const bool isButtonB = (text == "B");
        const bool isSceneB = processor.isSceneBActiveAnchor.load();
        const bool isActiveAnchor = (isButtonA && !isSceneB) || (isButtonB && isSceneB);
        const bool isToggleOn = button.getClickingTogglesState() && button.getToggleState();

        if (isActiveAnchor || isToggleOn)
            g.setColour (juce::Colour (0xFFFF5533)); // Red-Orange border outline
        else if (text == "Save" || text == "Recall" || text == "Copy" || text == "Init" ||
                 text == "Melo" || text == "Arti" || text == "Time" || text == "Navy")
            g.setColour (juce::Colour (0xFF181C24)); // Clean dark outline for both grids
        else
            g.setColour (juce::Colour (0xFF70757D).withAlpha (0.4f));
        
        g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
    }
}

void ChromaCapsLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos);

    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRect (x, y, width, height);
        return;
    }

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    const bool isVertical = (style == juce::Slider::LinearVertical);
    const juce::String cid = slider.getComponentID();

    if (cid == "morph") // Active Glow Vector Crossfader centerpiece [43]
    {
        const float trackHeight = 4.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;

        // Draw Left (Scene A) track with dynamic glow based on morph position
        float progress = sliderPos / static_cast<float>(width);
        float alphaA = 1.0f - progress;
        g.setColour (t.crossfaderTrackA.withAlpha (alphaA * 0.6f + 0.1f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, sliderPos - static_cast<float>(x), trackHeight, 2.0f);

        // Draw Right (Scene B) track with dynamic glow based on morph position
        float alphaB = progress;
        g.setColour (t.crossfaderTrackB.withAlpha (alphaB * 0.6f + 0.1f));
        g.fillRoundedRectangle (sliderPos, trackY, static_cast<float>(x + width) - sliderPos, trackHeight, 2.0f);

        // Draw Custom Widescreen Fader Cap
        const float thumbWidth = 14.0f;
        const float thumbHeight = 22.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        // Metallic dark-grey cap background
        g.setColour (juce::Colour (0xFF1E222A));
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);

        // Vertical indicator notch blending from Scene A to Scene B color
        juce::Colour notchColor = t.crossfaderTrackA.interpolatedWith (t.crossfaderTrackB, progress);
        g.setColour (notchColor);
        g.fillRect (sliderPos - 1.0f, thumbY + 2.0f, 2.0f, thumbHeight - 4.0f);
    }
    else if (isVertical)
    {
        const float trackWidth = 6.0f;
        const float trackX = static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f;
        
        // Flat, borderless track background slot with zero inner 3D highlights [43]
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (trackX, static_cast<float>(y), trackWidth, static_cast<float>(height), 3.0f);

        // Draw fader cap handle thumb
        const float thumbHeight = 16.0f;
        const float thumbWidth = 24.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        const float thumbY = sliderPos - (thumbHeight * 0.5f);

        // Navy blue thumb cap color [43]
        g.setColour (t.faderCap);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Horizontal white indicator line across the middle of the Navy cap [43]
        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + 2.0f, thumbY + (thumbHeight * 0.5f) - 1.0f, thumbWidth - 4.0f, 2.0f);
    }
    else
    {
        const float trackHeight = 6.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;
        
        // Flat borderless horizontal track
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, static_cast<float>(width), trackHeight, 3.0f);

        // Horizontal thumb
        const float thumbWidth = 16.0f;
        const float thumbHeight = 24.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        g.setColour (t.faderCap);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Clean white vertical line on horizontal thumb
        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + (thumbWidth * 0.5f) - 1.0f, thumbY + 2.0f, 2.0f, thumbHeight - 4.0f);
    }
}