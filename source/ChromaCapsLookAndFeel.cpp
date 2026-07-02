#include "ChromaCapsLookAndFeel.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

ChromaCapsLookAndFeel::ChromaCapsLookAndFeel (PluginProcessor& p, juce::AudioProcessorEditor* editor)
    : processor (p), parentEditor (editor)
{
}

ChromaCapsLookAndFeel::~ChromaCapsLookAndFeel()
{
}

void ChromaCapsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    
    auto& lf = button.getLookAndFeel();
    g.setFont (lf.getTextButtonFont (button, button.getHeight()));

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    
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

    // 3. Draw thin outline (Orange-Red on active toggles for Skyline theme)
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
    }
    else
    {
        g.setColour (button.findColour (juce::ComboBox::outlineColourId, true).withAlpha (0.15f));
    }

    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
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

    const bool isVertical = (style == juce::Slider::LinearVertical);
    
    if (isVertical)
    {
        const float trackWidth = 6.0f;
        const float trackX = static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f;
        
        // Base track background slot (Clean dark track with no level overlay, as meters are in the OLED)
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (trackX, static_cast<float>(y), trackWidth, static_cast<float>(height), 3.0f);
        
        // Inner 3D bevel highlighting
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawVerticalLine (static_cast<int>(trackX), static_cast<float>(y), static_cast<float>(y + height));
        
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawVerticalLine (static_cast<int>(trackX + trackWidth), static_cast<float>(y), static_cast<float>(y + height));

        // Draw fader cap handle thumb
        const float thumbHeight = 16.0f;
        const float thumbWidth = 24.0f;
        const float thumbX = static_cast<float>(x) + (static_cast<float>(width) - thumbWidth) * 0.5f;
        const float thumbY = sliderPos - (thumbHeight * 0.5f);

        const juce::Colour capColour = slider.findColour (juce::Slider::thumbColourId);
        g.setColour (capColour);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Neon Cyan indicator line inside fader cap
        g.setColour (juce::Colour (0xFF00D2FF));
        g.fillRect (thumbX + 2.0f, thumbY + (thumbHeight * 0.5f) - 1.0f, thumbWidth - 4.0f, 2.0f);
    }
    else
    {
        const float trackHeight = 6.0f;
        const float trackY = static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f;
        
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (static_cast<float>(x), trackY, static_cast<float>(width), trackHeight, 3.0f);
        
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawHorizontalLine (static_cast<int>(trackY), static_cast<float>(x), static_cast<float>(x + width));
        
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawHorizontalLine (static_cast<int>(trackY + trackHeight), static_cast<float>(x), static_cast<float>(x + width));

        // Horizontal thumb
        const float thumbWidth = 16.0f;
        const float thumbHeight = 24.0f;
        const float thumbX = sliderPos - (thumbWidth * 0.5f);
        const float thumbY = static_cast<float>(y) + (static_cast<float>(height) - thumbHeight) * 0.5f;

        const juce::Colour capColour = slider.findColour (juce::Slider::thumbColourId);
        g.setColour (capColour);
        g.fillRoundedRectangle (thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        g.setColour (juce::Colours::white);
        g.fillRect (thumbX + (thumbWidth * 0.5f) - 1.0f, thumbY + 2.0f, 2.0f, thumbHeight - 4.0f);
    }
}