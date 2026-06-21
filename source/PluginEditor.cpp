#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // 1. Add and configure the Central OLED Display
    addAndMakeVisible (oledDisplay);

    // 2. Configure the 8 Scale-Degree Faders (Bottom row)
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    
    // Dynamic note reading labels based on our planned Scale Degree layout
    juce::String scaleNotes[] = { "C", "D", "Eb", "F", "G", "Ab", "Bb", "C" };

    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical);
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00D2FF)); // Neon Aqua
        faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF112233));
        addAndMakeVisible (faders[i]);

        faderLabels[i]->setText (scaleNotes[i], juce::dontSendNotification);
        faderLabels[i]->setFont (juce::Font ("Consolas", 14.0f, juce::Font::bold));
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (faderLabels[i]);
    }

    // 3. Configure the Left Sidebar Knobs (Rhythm)
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob };
    juce::String leftNames[] = { "Rhy-Morph", "Rest", "Legato" };
    
    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF));
        leftKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000)); // Transparent border
        leftKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (leftKnobs[i]);
    }

    // 4. Configure the Right Sidebar Knobs (Harmony & Chaos)
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob };
    juce::String rightNames[] = { "Entropy", "Harmony", "Chaos" };

    for (int i = 0; i < 3; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); // Warm Amber
        rightKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        rightKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (rightKnobs[i]);
    }

    // 5. Configure the Octatrack-style Scene Morph Crossfader
    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal);
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF)); // Ice White Slider
    morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF222222));
    addAndMakeVisible (morphCrossfader);

    // 6. Configure Buttons
    addAndMakeVisible (latchButton);
    latchButton.setButtonText ("LATCH");
    latchButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF112233));
    latchButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF00D2FF));

    addAndMakeVisible (diceMelodyButton);
    diceMelodyButton.setButtonText ("DICE M");
    diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceMelodyButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));

    addAndMakeVisible (diceRhythmButton);
    diceRhythmButton.setButtonText ("DICE R");
    diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceRhythmButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));

    // Scene Edit buttons
    addAndMakeVisible (sceneAButton);
    sceneAButton.setButtonText ("A");
    sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));

    addAndMakeVisible (sceneBButton);
    sceneBButton.setButtonText ("B");
    sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));

    // 7. Configure the 8 OLED-Style Preset Buttons
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF0a0a0a));
        // Soft outer-edge indicator glow (Muted blue/purple for blank slots initially)
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF444444));
    }

    // ==============================================================================
    // Bind all GUI sliders and buttons to our APVTS variables under-the-hood
    // ==============================================================================
    fader1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader1.getParamID(), fader1);
    fader2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader2.getParamID(), fader2);
    fader3Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader3.getParamID(), fader3);
    fader4Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader4.getParamID(), fader4);
    fader5Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader5.getParamID(), fader5);
    fader6Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader6.getParamID(), fader6);
    fader7Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader7.getParamID(), fader7);
    fader8Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader8.getParamID(), fader8);

    rhythmMorphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rhythmMorph.getParamID(), rhythmMorphKnob);
    restAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rest.getParamID(), restKnob);
    legatoAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::legato.getParamID(), legatoKnob);

    entropyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::entropy.getParamID(), entropyKnob);
    harmonyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::harmony.getParamID(), harmonyKnob);
    chaosAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::chaos.getParamID(), chaosKnob);

    morphAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::morph.getParamID(), morphCrossfader);
    latchAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::latch.getParamID(), latchButton);

    // Set the overall window scale (Width, Height)
    setSize (750, 480);
}

PluginEditor::~PluginEditor()
{
}

// ==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    // Dark Minimalist Chassis Background (Premium hardware texture)
    g.fillAll (juce::Colour (0xFF141416));

    // Outer Chassis Bevel Accent
    g.setColour (juce::Colour (0xFF232326));
    g.drawRect (getLocalBounds().toFloat(), 3.0f);

    // Section Titles
    g.setFont (juce::FontOptions (12.0f).withStyle ("bold"));
    g.setColour (juce::Colour (0xFF55555c));
    g.drawText ("RHYTHM", 15, 12, 100, 20, juce::Justification::left);
    g.drawText ("GENERATOR", getWidth() - 115, 12, 100, 20, juce::Justification::right);
}

// ==============================================================================
// This is our screen layout algorithm. It carves up the window area into stable 
// bounds so that our OLED display, side controls, and faders never overlap.
// ==============================================================================
void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (15);

    // 1. Bottom Section: 8 Scale-Degree Faders (Take up 115 pixels vertically)
    auto bottomArea = area.removeFromBottom (115);
    auto faderLabelArea = bottomArea.removeFromBottom (20);
    int faderWidth = bottomArea.getWidth() / 8;
    
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    
    for (int i = 0; i < 8; ++i)
    {
        auto column = bottomArea.removeFromLeft (faderWidth);
        faders[i]->setBounds (column.reduced (6, 0));
        
        auto labelColumn = faderLabelArea.removeFromLeft (faderWidth);
        faderLabels[i]->setBounds (labelColumn);
    }

    // Space Divider
    area.removeFromBottom (10);

    // 2. Scene Morph Crossfader (Take up 35 pixels)
    auto morphArea = area.removeFromBottom (35);
    sceneAButton.setBounds (morphArea.removeFromLeft (35).reduced (0, 3));
    sceneBButton.setBounds (morphArea.removeFromRight (35).reduced (0, 3));
    morphCrossfader.setBounds (morphArea.reduced (10, 5));

    // Space Divider
    area.removeFromBottom (10);

    // 3. Sidebars
    auto leftSidebar = area.removeFromLeft (95);
    auto rightSidebar = area.removeFromRight (95);
    
    // Left Sidebar: Knobs & Latch Button
    int leftRowHeight = leftSidebar.getHeight() / 4;
    rhythmMorphKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    restKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    legatoKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    latchButton.setBounds (leftSidebar.reduced (10, 8));

    // Right Sidebar: Knobs & DICE Buttons
    int rightRowHeight = rightSidebar.getHeight() / 4;
    entropyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    harmonyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    chaosKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    
    auto diceArea = rightSidebar;
    diceMelodyButton.setBounds (diceArea.removeFromLeft (diceArea.getWidth() / 2).reduced (2, 8));
    diceRhythmButton.setBounds (diceArea.reduced (2, 8));

    // 4. Center Section: OLED Display & 8 Preset Buttons
    auto presetArea = area.removeFromBottom (32);
    
    // OLED screen gets the remaining middle real estate
    oledDisplay.setBounds (area.reduced (5, 5));

    // Position the 8 Preset buttons directly underneath the OLED display
    int presetWidth = presetArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].setBounds (presetArea.removeFromLeft (presetWidth).reduced (4, 3));
    }
}