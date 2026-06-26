#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p)
{
    addAndMakeVisible (oledDisplay);

    // Bottom faders (Mapped to Custom LookAndFeel for fader slot and fader knob modifications) [5]
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    juce::String scaleNotes[] = { "C", "D", "Eb", "F", "G", "Ab", "Bb", "C" };

    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical);
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00D2FF));
        faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF181C24));
        faders[i]->setLookAndFeel (&chromaLookAndFeel); // Attach LookAndFeel to Linear sliders [5]
        addAndMakeVisible (faders[i]);

        faderLabels[i]->setText (scaleNotes[i], juce::dontSendNotification);
        faderLabels[i]->setFont (juce::Font (14.0f, juce::Font::bold));
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (faderLabels[i]);
    }

    // Left sidebar knobs and Titles
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::String leftNames[] = { "MORPH", "REST", "LEGATO", "RATE" };

    for (int i = 0; i < 4; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF)); 
        leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        addAndMakeVisible (leftKnobs[i]);

        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification);
        leftTitles[i]->setFont (juce::Font (10.0f, juce::Font::bold));
        leftTitles[i]->setJustificationType (juce::Justification::centred);
        leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
        addAndMakeVisible (leftTitles[i]);
    }

    // Right sidebar knobs and Titles
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::String rightNames[] = { "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };

    for (int i = 0; i < 4; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); 
        rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        addAndMakeVisible (rightKnobs[i]);

        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification);
        rightTitles[i]->setFont (juce::Font (10.0f, juce::Font::bold));
        rightTitles[i]->setJustificationType (juce::Justification::centred);
        rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
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

    // Chords toggle
    addAndMakeVisible (chordModeButton);
    chordModeButton.setButtonText ("CHORDS");
    chordModeButton.setClickingTogglesState (true);
    chordModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF141416));
    chordModeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF666666));
    chordModeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFFFFB300));
    chordModeButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));

    // DICE Buttons
    addAndMakeVisible (diceMelodyButton);
    diceMelodyButton.setButtonText ("DICE MELODY");
    diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceMelodyButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));
    diceMelodyButton.onClick = [this] { processor.diceMelody(); };

    addAndMakeVisible (diceRhythmButton);
    diceRhythmButton.setButtonText ("DICE RHYTHM");
    diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceRhythmButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFFFB300));
    diceRhythmButton.onClick = [this] { processor.diceRhythm(); };

    // Scenes
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

    // 8 Preset Slots
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF050505));
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF444444));

        presetButtons[i].onClick = [this, i] {
            if (chordModeButton.getToggleState()) processor.triggerDiatonicChordPad(i); 
            else processor.loadPreset(i); 
        };

        presetButtons[i].addMouseListener (this, false);
    }

    // Configure Key & Scale Dropdowns
    addAndMakeVisible (rootKeyBox);
    rootKeyBox.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 1);
    rootKeyBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF111111));
    rootKeyBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF222222));
    rootKeyBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFF00D2FF));

    addAndMakeVisible (scaleTypeBox);
    scaleTypeBox.addItemList (juce::StringArray { "Major", "Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1);
    scaleTypeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF111111));
    scaleTypeBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF222222));
    scaleTypeBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFFFFB300));

    addAndMakeVisible (cycleLengthBox);
    cycleLengthBox.addItemList (juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);
    cycleLengthBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF111111));
    cycleLengthBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF222222));
    cycleLengthBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xFFFFB300));

    // Parameter Bindings
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
    rateAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rate.getParamID(), rateKnob);

    entropyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::entropy.getParamID(), entropyKnob);
    harmonyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::harmony.getParamID(), harmonyKnob);
    chaosAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::chaos.getParamID(), chaosKnob);
    octavesAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::octaves.getParamID(), octavesKnob);

    morphAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::morph.getParamID(), morphCrossfader);
    latchAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::latch.getParamID(), latchButton);
    chordModeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::chordMode.getParamID(), chordModeButton);

    rootKeyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::rootKey.getParamID(), rootKeyBox);
    scaleTypeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::scaleType.getParamID(), scaleTypeBox);
    cycleLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::cycleLength.getParamID(), cycleLengthBox);

    setResizable (true, true);
    setResizeLimits (700, 460, 1400, 920);
    setSize (850, 560); 
    
    startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); 
    
    // Safety unregister custom LookAndFeel references on all rotary knobs [5]
    rhythmMorphKnob.setLookAndFeel (nullptr);
    restKnob.setLookAndFeel (nullptr);
    legatoKnob.setLookAndFeel (nullptr);
    rateKnob.setLookAndFeel (nullptr);
    
    entropyKnob.setLookAndFeel (nullptr);
    harmonyKnob.setLookAndFeel (nullptr);
    chaosKnob.setLookAndFeel (nullptr);
    octavesKnob.setLookAndFeel (nullptr);

    // Safety unregister custom LookAndFeel references on all bottom linear faders [5]
    fader1.setLookAndFeel (nullptr);
    fader2.setLookAndFeel (nullptr);
    fader3.setLookAndFeel (nullptr);
    fader4.setLookAndFeel (nullptr);
    fader5.setLookAndFeel (nullptr);
    fader6.setLookAndFeel (nullptr);
    fader7.setLookAndFeel (nullptr);
    fader8.setLookAndFeel (nullptr);

    diceMelodyButton.onClick = nullptr;
    diceRhythmButton.onClick = nullptr;
    sceneAButton.onClick = nullptr;
    sceneBButton.onClick = nullptr;

    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].onClick = nullptr;
        presetButtons[i].onStateChange = nullptr;
        presetButtons[i].removeMouseListener(this); 
    }
}

void PluginEditor::timerCallback()
{
    float morphValue = morphCrossfader.getValue();
    
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

    // Dynamic Fader Labels updating to show current scale notes based on selected Key/Scale
    int activeKey = rootKeyBox.getSelectedItemIndex();
    int activeScale = scaleTypeBox.getSelectedItemIndex();
    
    std::vector<int> offsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; // Major
    if (activeScale == 1)      offsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; // Minor
    else if (activeScale == 2) offsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; // Pentatonic Minor
    else if (activeScale == 3) offsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  // Pentatonic Major
    else if (activeScale == 4) offsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  // Dorian
    else if (activeScale == 5) offsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  // Phrygian
    else if (activeScale == 6) offsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  // Lydian
    else if (activeScale == 7) offsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  // Mixolydian
    else if (activeScale == 8) offsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  // Harmonic Minor
    else if (activeScale == 9) offsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  // Melodic Minor

    juce::String chromaticNotes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };

    for (int i = 0; i < 8; ++i)
    {
        int noteIndex = (activeKey + offsets[i]) % 12;
        faderLabels[i]->setText (chromaticNotes[noteIndex], juce::dontSendNotification);
    }

    // Keep Scene Active Button Colors Synced beautifully
    if (processor.hasSceneA)
    {
        sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF00D2FF));
        sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
    }
    else
    {
        sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF18181C));
        sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF88888A));
    }

    if (processor.hasSceneB)
    {
        sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFAA00));
        sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
    }
    else
    {
        sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF18181C));
        sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF88888A));
    }

    // Dynamic preset saved status glow
    for (int i = 0; i < 8; ++i)
    {
        if (processor.isPresetSaved (i))
        {
            presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A3A4A));
            presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF00D2FF));
        }
        else
        {
            presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF141416));
            presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF66666A));
        }
    }
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Dark industrial metallic background
    g.fillAll (juce::Colour (0xFF16181F));
    
    // Symmetrical steel panel divider lines
    g.setColour (juce::Colour (0xFF2A2E3D));
    g.drawRect (getLocalBounds().toFloat(), 3.0f);
    
    // Horizontal groove line separating upper controls plate from bottom faders plate
    g.drawHorizontalLine (getHeight() - static_cast<int>(getHeight() * 0.22f), 15.0f, getWidth() - 15.0f);

    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xFF55555C));
    g.drawText ("RHYTHM", 15, 12, 100, 20, juce::Justification::left);
    g.drawText ("GENERATOR", getWidth() - 115, 12, 100, 20, juce::Justification::right);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (15);

    // Get current proportional dimensions to compute responsive coordinates [4]
    int totalWidth = getWidth();
    int totalHeight = getHeight();

    // 1. Bottom Section: 8 Scale-Degree Faders (Faders space reduced [1])
    int bottomHeight = static_cast<int>(totalHeight * 0.17f); 
    auto bottomArea = area.removeFromBottom (bottomHeight);
    auto faderLabelArea = bottomArea.removeFromBottom (16);
    int faderWidth = bottomArea.getWidth() / 8;
    
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    
    for (int i = 0; i < 8; ++i)
    {
        auto column = bottomArea.removeFromLeft (faderWidth);
        faders[i]->setBounds (column.reduced (juce::jmax (2, faderWidth / 8), 0));
        
        auto labelColumn = faderLabelArea.removeFromLeft (faderWidth);
        faderLabels[i]->setBounds (labelColumn);
    }

    area.removeFromBottom (10);

    // 2. Scene Morph Crossfader
    int morphHeight = static_cast<int>(totalHeight * 0.07f);
    auto morphArea = area.removeFromBottom (juce::jlimit (25, 45, morphHeight));
    sceneAButton.setBounds (morphArea.removeFromLeft (45).reduced (0, 3));
    sceneBButton.setBounds (morphArea.removeFromRight (45).reduced (0, 3));
    morphCrossfader.setBounds (morphArea.reduced (15, 5));

    area.removeFromBottom (15);

    // 3. Sidebars (Width scaled proportionally at 15% of total editor layout width) [4]
    int sidebarWidth = static_cast<int>(totalWidth * 0.15f); 
    auto leftSidebar = area.removeFromLeft (sidebarWidth);
    auto rightSidebar = area.removeFromRight (sidebarWidth);
    
    // Left Sidebar: 4 Knobs (Morph, Rest, Legato, Rate) + Vertical Buttons [4]
    int leftRowHeight = leftSidebar.getHeight() / 5;
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    
    for (int i = 0; i < 4; ++i)
    {
        auto row = leftSidebar.removeFromTop (leftRowHeight);
        leftTitles[i]->setBounds (row.removeFromTop (16));
        leftKnobs[i]->setBounds (row.reduced (4, 2));
    }
    
    auto leftBtnArea = leftSidebar.reduced (5, 2);
    int leftBtnHeight = leftBtnArea.getHeight() / 2;
    latchButton.setBounds (leftBtnArea.removeFromTop (leftBtnHeight).reduced (2));
    chordModeButton.setBounds (leftBtnArea.reduced (2));

    // Right Sidebar: 4 Knobs (Entropy, Harmony, Chaos, Octaves) + Vertical Buttons [4]
    int rightRowHeight = rightSidebar.getHeight() / 5;
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    
    for (int i = 0; i < 4; ++i)
    {
        auto row = rightSidebar.removeFromTop (rightRowHeight);
        rightTitles[i]->setBounds (row.removeFromTop (16));
        rightKnobs[i]->setBounds (row.reduced (4, 2));
    }
    
    auto diceArea = rightSidebar.reduced (5, 2);
    int diceBtnHeight = diceArea.getHeight() / 2;
    diceMelodyButton.setBounds (diceArea.removeFromTop (diceBtnHeight).reduced (2));
    diceRhythmButton.setBounds (diceArea.reduced (2));

    // 4. Center Section: Dropdown Bar, OLED Display, and 8 Preset Buttons [4]
    int presetHeight = static_cast<int>(totalHeight * 0.06f);
    auto presetArea = area.removeFromBottom (juce::jlimit (24, 36, presetHeight));
    auto oledArea = area.reduced (5, 5);
    
    // Proportional Dropdown Bar positioned safely above the graphic display
    int dropdownBarHeight = static_cast<int>(totalHeight * 0.07f);
    auto dropdownBarArea = oledArea.removeFromTop (juce::jlimit (28, 40, dropdownBarHeight));
    int dropdownWidth = dropdownBarArea.getWidth() / 3;
    
    rootKeyBox.setBounds (dropdownBarArea.removeFromLeft (dropdownWidth).reduced (6, 2));
    cycleLengthBox.setBounds (dropdownBarArea.removeFromLeft (dropdownWidth).reduced (6, 2));
    scaleTypeBox.setBounds (dropdownBarArea.reduced (6, 2));
    
    oledDisplay.setBounds (oledArea.reduced (0, 5));

    int presetWidth = presetArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].setBounds (presetArea.removeFromLeft (presetWidth).reduced (4, 3));
    }
}