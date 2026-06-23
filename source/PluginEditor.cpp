#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p)
{
    addAndMakeVisible (oledDisplay);

    // Bottom faders
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    juce::String scaleNotes[] = { "C", "D", "Eb", "F", "G", "Ab", "Bb", "C" };

    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical);
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00D2FF));
        faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF112233));
        addAndMakeVisible (faders[i]);

        faderLabels[i]->setText (scaleNotes[i], juce::dontSendNotification);
        faderLabels[i]->setFont (juce::FontOptions (14.0f).withStyle ("bold"));
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (faderLabels[i]);
    }

    // Left sidebar knobs and Titles
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle };
    juce::String leftNames[] = { "MORPH", "REST", "LEGATO" };

    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF));
        leftKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        leftKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (leftKnobs[i]);

        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification);
        leftTitles[i]->setFont (juce::FontOptions (10.0f).withStyle ("bold"));
        leftTitles[i]->setJustificationType (juce::Justification::centred);
        leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555c));
        addAndMakeVisible (leftTitles[i]);
    }

    // Right sidebar knobs and Titles
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle };
    juce::String rightNames[] = { "ENTROPY", "HARMONY", "CHAOS" };

    for (int i = 0; i < 3; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300));
        rightKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        rightKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (rightKnobs[i]);

        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification);
        rightTitles[i]->setFont (juce::FontOptions (10.0f).withStyle ("bold"));
        rightTitles[i]->setJustificationType (juce::Justification::centred);
        rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555c));
        addAndMakeVisible (rightTitles[i]);
    }

    // Crossfader
    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal);
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF));
    morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF222222));
    addAndMakeVisible (morphCrossfader);

    // Latch toggle
    addAndMakeVisible (latchButton);
    latchButton.setButtonText ("LATCH");
    latchButton.setClickingTogglesState (true);
    latchButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF112233));
    latchButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF00D2FF));
    latchButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF00D2FF));
    latchButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));

    // DICE Buttons
    addAndMakeVisible (diceMelodyButton);
    diceMelodyButton.setButtonText ("DICE M");
    diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceMelodyButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));
    diceMelodyButton.onClick = [this] { processor.diceMelody(); };

    addAndMakeVisible (diceRhythmButton);
    diceRhythmButton.setButtonText ("DICE R");
    diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceRhythmButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));
    diceRhythmButton.onClick = [this] { processor.diceRhythm(); };

    // Toggle-to-Capture / Toggle-to-Clear Scenes
    addAndMakeVisible (sceneAButton);
    sceneAButton.setButtonText ("A");
    sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneAButton.onClick = [this] {
        if (processor.hasSceneA)
        {
            processor.hasSceneA = false;
            sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
        }
        else
        {
            processor.captureSceneA();
            sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF00D2FF));
        }
    };

    addAndMakeVisible (sceneBButton);
    sceneBButton.setButtonText ("B");
    sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneBButton.onClick = [this] {
        if (processor.hasSceneB)
        {
            processor.hasSceneB = false;
            sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
        }
        else
        {
            processor.captureSceneB();
            sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFB300));
        }
    };

    // 8 Preset Slots (Left-Click to Load / Right-Click to Save)
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF050505));
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF444444));

        presetButtons[i].onClick = [this, i] {
            processor.loadPreset(i);
        };

        presetButtons[i].addMouseListener (this, false);
    }

    // Bindings
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

    setSize (750, 480);
    startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); 
    // Clean up our button mouse listeners (prevents dangling pointer segfault 139)
    for (int i = 0; i < 8; ++i)
        presetButtons[i].removeMouseListener(this);
}

void PluginEditor::timerCallback()
{
    float morphValue = morphCrossfader.getValue();
    
    // Smoothly morph parameters and update underlying variables in real-time
    if (processor.hasSceneA && processor.hasSceneB && morphCrossfader.isMouseButtonDown())
    {
        for (int i = 0; i < 8; ++i)
        {
            float targetValue = (processor.sceneA.faders[i] * (1.0f - morphValue)) + (processor.sceneB.faders[i] * morphValue);
            processor.apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetValue);
        }

        processor.apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost ((processor.sceneA.rhythmMorph * (1.0f - morphValue)) + (processor.sceneB.rhythmMorph * morphValue));
        processor.apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost ((processor.sceneA.rest * (1.0f - morphValue)) + (processor.sceneB.rest * morphValue));
        processor.apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost ((processor.sceneA.legato * (1.0f - morphValue)) + (processor.sceneB.legato * morphValue));
        processor.apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost ((processor.sceneA.entropy * (1.0f - morphValue)) + (processor.sceneB.entropy * morphValue));
        processor.apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost ((processor.sceneA.harmony * (1.0f - morphValue)) + (processor.sceneB.harmony * morphValue));
        processor.apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost ((processor.sceneA.chaos * (1.0f - morphValue)) + (processor.sceneB.chaos * morphValue));
    }

    // Keep the Preset glow updated
    for (int i = 0; i < 8; ++i)
    {
        if (processor.isPresetSaved (i))
            presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF003344));
    }
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xFF141416));
    g.setColour (juce::Colour (0xFF232326));
    g.drawRect (getLocalBounds().toFloat(), 3.0f);

    g.setFont (juce::FontOptions (12.0f).withStyle ("bold"));
    g.setColour (juce::Colour (0xFF55555c));
    g.drawText ("RHYTHM", 15, 12, 100, 20, juce::Justification::left);
    g.drawText ("GENERATOR", getWidth() - 115, 12, 100, 20, juce::Justification::right);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (15);

    // 1. Bottom Section: 8 Scale-Degree Faders
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

    area.removeFromBottom (10);

    // 2. Scene Morph Crossfader
    auto morphArea = area.removeFromBottom (35);
    sceneAButton.setBounds (morphArea.removeFromLeft (35).reduced (0, 3));
    sceneBButton.setBounds (morphArea.removeFromRight (35).reduced (0, 3));
    morphCrossfader.setBounds (morphArea.reduced (10, 5));

    area.removeFromBottom (10);

    // 3. Sidebars
    auto leftSidebar = area.removeFromLeft (95);
    auto rightSidebar = area.removeFromRight (95);
    
    int leftRowHeight = leftSidebar.getHeight() / 4;
    rhythmMorphKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    restKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    legatoKnob.setBounds (leftSidebar.removeFromTop (leftRowHeight).reduced (2));
    latchButton.setBounds (leftSidebar.reduced (10, 8));

    int rightRowHeight = rightSidebar.getHeight() / 4;
    entropyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    harmonyKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    chaosKnob.setBounds (rightSidebar.removeFromTop (rightRowHeight).reduced (2));
    
    auto diceArea = rightSidebar;
    diceMelodyButton.setBounds (diceArea.removeFromLeft (diceArea.getWidth() / 2).reduced (2, 8));
    diceRhythmButton.setBounds (diceArea.reduced (2, 8));

    // 4. Center Section: OLED Display & 8 Preset Buttons
    auto presetArea = area.removeFromBottom (32);
    oledDisplay.setBounds (area.reduced (5, 5));

    int presetWidth = presetArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].setBounds (presetArea.removeFromLeft (presetWidth).reduced (4, 3));
    }
}