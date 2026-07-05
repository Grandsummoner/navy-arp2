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

    float centerX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    float centerY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    
    // Scale the rotary sizes to match the 1241x848 layout
    float knobRadius = juce::jmin (static_cast<float> (width), static_cast<float> (height)) * 0.35f;
    float ledRadius = knobRadius + 7.0f;
    float ledDiameter = 3.5f;

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
            float outerRadius = 5.0f;
            juce::ColourGradient glow (activeColor.withAlpha (0.6f), ledX, ledY,
                                       activeColor.withAlpha (0.0f), ledX + outerRadius, ledY + outerRadius,
                                       true);
            g.setGradientFill (glow);
            g.fillEllipse (ledX - outerRadius, ledY - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

            // Bright inner core
            float innerRadius = 1.5f;
            g.setColour (juce::Colours::white.interpolatedWith (activeColor, 0.2f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
        else
        {
            // Dim inactive LED
            float innerRadius = 1.0f;
            g.setColour (juce::Colour (0xFF1F2229).withAlpha (0.25f));
            g.fillEllipse (ledX - innerRadius, ledY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        }
    }

    // Note: We skip drawing the solid background ellipse to let the
    // photorealistic brushed metallic knob asset from the PNG panel show through!

    // 4. Draw pointer needle centered over the asset knob
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path path;
    float pointerThickness = 2.0f;
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
    const bool isButtonA = (text == "A");
    const bool isButtonB = (text == "B");
    const bool isUtilButton = (text == "Save" || text == "Recall" || text == "Copy" || text == "Init");
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");
    const bool isPresetButton = (text == "1" || text == "2" || text == "3" || text == "4" || text == "5" || text == "6" || text == "7" || text == "8");
    
    // Add Latch, Poly, Freeze and default "Seq" / "SEQ" to the skip list so they don't double-render [1.1.8]
    const bool isStaticTopButton = (text == "Latch" || text == "Poly" || text == "Freeze" || text == "Seq" || text == "SEQ");

    if (isUtilButton || isDiceButton || isPresetButton || isButtonA || isButtonB || isStaticTopButton)
    {
        return; 
    }

    // If the sequencer toggles to "Arp", render it cleanly in C++
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    
    if (button.getToggleState())
        g.setColour (juce::Colours::white);
    else
        g.setColour (juce::Colour (0xFF888888));

    g.drawFittedText (text, button.getLocalBounds(), juce::Justification::centred, 1);
}

void ChromaCapsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    const float cornerSize = 4.0f;
    bool isFlashing = false;
    juce::Colour flashColour = backgroundColour;
    const juce::String text = button.getButtonText();
    const bool isDiceButton = (text == "Melo" || text == "Arti" || text == "Time" || text == "Navy");
    const bool isStaticTopButton = (text == "Latch" || text == "Poly" || text == "Freeze" || text == "Seq" || text == "SEQ");

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
                    int flashState = (editor->presetFlashTimer[i] / 4) % 2; 
                    if (flashState == 1)
                        flashColour = (editor->presetFlashType[i] == 1) ? juce::Colour (0xFFFF5533) : juce::Colour (0xFF00D2FF);
                }
                break;
            }
        }

        if (!isFlashing)
        {
            if (&button == &(editor->saveButton) && editor->saveFlashTimer > 0)
            {
                isFlashing = true;
                flashColour = ((editor->saveFlashTimer / 4) % 2 == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
            else if (&button == &(editor->recallButton) && editor->recallFlashTimer > 0)
            {
                isFlashing = true;
                flashColour = ((editor->recallFlashTimer / 4) % 2 == 1) ? juce::Colour (0xFF00D2FF) : backgroundColour;
            }
            else if (&button == &(editor->copyButton) && editor->copyFlashTimer > 0)
            {
                isFlashing = true;
                flashColour = ((editor->copyFlashTimer / 4) % 2 == 1) ? juce::Colour (0xFFFFFF33) : backgroundColour;
            }
            else if (&button == &(editor->initButton) && editor->initFlashTimer > 0)
            {
                isFlashing = true;
                flashColour = ((editor->initFlashTimer / 4) % 2 == 1) ? juce::Colour (0xFFFF5533) : backgroundColour;
            }
        }
    }

    // 2. Render background overlays depending on state
    if (isFlashing)
    {
        g.setColour (flashColour.withAlpha (0.6f));
        g.fillRoundedRectangle (bounds, cornerSize);
    }
    else if (text == "Arp")
    {
        // Smart Masking: Draw a dark-blue fill to mask the printed "Seq" text underneath, then draw active highlight [1.1.8]
        g.setColour (juce::Colour (0xFF1E222A)); 
        g.fillRoundedRectangle (bounds, cornerSize);
        
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.25f));
        g.fillRoundedRectangle (bounds, cornerSize);
        g.setColour (juce::Colour (0xFF00D2FF).withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.reduced(0.5f), cornerSize, 1.25f);
    }
    else if (isDiceButton)
    {
        // Watermark Masking: Draw solid dark-slate backing to completely hide the Gemini symbol [1.1.8]
        g.setColour (juce::Colour (0xFF1E222A));
        g.fillRoundedRectangle (bounds, cornerSize);
        
        if (shouldDrawButtonAsDown) {
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (bounds, cornerSize);
        } else if (shouldDrawButtonAsHighlighted) {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }
    }
    else if (isStaticTopButton)
    {
        // Fully transparent when inactive, or draw a subtle highlight when toggled active [1.1.8]
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
        // Active Toggle state (draw a beautiful cyan highlight box over the panel button asset)
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
    else
    {
        return; // Transparent to reveal asset buttons [1.1.8]
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

        // Custom Widescreen Fader Cap
        const float thumbWidth = 20.0f;
        const float thumbHeight = 15.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        g.setColour (juce::Colour (0xFF1E222A));
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);

        juce::Colour notchColor = t.crossfaderTrackA.interpolatedWith (t.crossfaderTrackB, progress);
        g.setColour (notchColor);
        g.fillRect (sliderPos - 1.0f, thumbY + 1.0f, 2.0f, thumbHeight - 2.0f);
    }
    else if (isVertical)
    {
        // Draw fader cap handle thumb
        const float thumbHeight = 12.0f;
        const float thumbWidth = 22.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        const float thumbY = sliderPos - (thumbHeight * 0.5f);

        g.setColour (t.faderCap);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 1.5f);

        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + 1.0f, thumbY + (thumbHeight * 0.5f) - 1.0f, thumbWidth - 2.0f, 2.0f);
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