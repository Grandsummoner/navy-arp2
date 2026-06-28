#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "OledDisplay.h"
#include "ChromaCapsLookAndFeel.h"
#include "AppTheme.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p, this)
{
    addAndMakeVisible (oledDisplay);
    processor.apvts.addParameterListener ("panelTheme", this);

    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    juce::String scaleNotes[] = { "C", "D", "Eb", "F", "G", "Ab", "Bb", "C" };
    for (int i = 0; i < 8; ++i) {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical); faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00D2FF)); faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF181C24));
        faders[i]->setLookAndFeel (&chromaLookAndFeel); addAndMakeVisible (faders[i]);
        faderLabels[i]->setText (scaleNotes[i], juce::dontSendNotification); faderLabels[i]->setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold"))); 
        faderLabels[i]->setJustificationType (juce::Justification::centred); faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888)); addAndMakeVisible (faderLabels[i]);
    }

    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::String leftNames[] = { "Morph", "Rest", "Legato", "Rate" }, leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };
    for (int i = 0; i < 4; ++i) {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF)); leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        leftKnobs[i]->setComponentID (leftPrefixes[i]); leftKnobs[i]->addMouseListener (this, false); addAndMakeVisible (leftKnobs[i]);
        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification); leftTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); 
        leftTitles[i]->setJustificationType (juce::Justification::centred); leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C)); addAndMakeVisible (leftTitles[i]);
    }

    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::String rightNames[] = { "Entropy", "Harmony", "Chaos", "Octaves" }, rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };
    for (int i = 0; i < 4; ++i) {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        rightKnobs[i]->setComponentID (rightPrefixes[i]); rightKnobs[i]->addMouseListener (this, false); addAndMakeVisible (rightKnobs[i]);
        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification); rightTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); 
        rightTitles[i]->setJustificationType (juce::Justification::centred); rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C)); addAndMakeVisible (rightTitles[i]);
    }

    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal); morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF)); morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF222222));
    morphCrossfader.setLookAndFeel (&chromaLookAndFeel); addAndMakeVisible (morphCrossfader);

    juce::TextButton* deckBtns[] = { &latchButton, &arpSeqButton, &polyButton, &freezeButton }; juce::String deckTxt[] = { "Latch", "SEQ", "Poly", "Freeze" };
    for (int i = 0; i < 4; ++i) { addAndMakeVisible (deckBtns[i]); deckBtns[i]->setButtonText (deckTxt[i]); deckBtns[i]->setClickingTogglesState (true); deckBtns[i]->setLookAndFeel (&chromaLookAndFeel); }

    juce::TextButton* sceneBtns[] = { &sceneAButton, &sceneBButton }; juce::String sceneTxt[] = { "A", "B" };
    for (int i = 0; i < 2; ++i) { addAndMakeVisible (sceneBtns[i]); sceneBtns[i]->setButtonText (sceneTxt[i]); sceneBtns[i]->addMouseListener (this, false); sceneBtns[i]->setLookAndFeel (&chromaLookAndFeel); }

    juce::TextButton* utilBtns[] = { &saveButton, &recallButton, &copyButton, &initButton }; juce::String utilTxt[] = { "Save", "Recall", "Copy", "Init" };
    for (int i = 0; i < 4; ++i) { addAndMakeVisible (utilBtns[i]); utilBtns[i]->setButtonText (utilTxt[i]); utilBtns[i]->setClickingTogglesState (true); utilBtns[i]->addMouseListener (this, false); utilBtns[i]->setLookAndFeel (&chromaLookAndFeel); }
    saveButton.onClick   = [this] { if (saveButton.getToggleState()) recallButton.setToggleState (false, juce::dontSendNotification); };
    recallButton.onClick = [this] { if (recallButton.getToggleState()) saveButton.setToggleState (false, juce::dontSendNotification); };

    addAndMakeVisible (diceMeloButton); diceMeloButton.setComponentID ("dice_melody"); diceMeloButton.setButtonText ("Melo"); diceMeloButton.setLookAndFeel (&chromaLookAndFeel); 
    diceMeloButton.onClick = [this] { if (initButton.getToggleState()) { processor.resetRhythm(); initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceMelody(); } };
    
    addAndMakeVisible (diceArtiButton); diceArtiButton.setComponentID ("dice_articulation"); diceArtiButton.setButtonText ("Arti"); diceArtiButton.setLookAndFeel (&chromaLookAndFeel); 
    diceArtiButton.onClick = [this] { if (initButton.getToggleState()) { processor.apvts.getParameter(IDs::rest.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::legato.getParamID())->setValueNotifyingHost(0.5f); initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceArticulation(); } };
    
    addAndMakeVisible (diceTimeButton); diceTimeButton.setComponentID ("dice_time"); diceTimeButton.setButtonText ("Time"); diceTimeButton.setLookAndFeel (&chromaLookAndFeel); 
    diceTimeButton.onClick = [this] { if (initButton.getToggleState()) { processor.apvts.getParameter(IDs::rate.getParamID())->setValueNotifyingHost(2.0f / 3.0f); processor.apvts.getParameter(IDs::octaves.getParamID())->setValueNotifyingHost(3.0f / 6.0f); processor.apvts.getParameter(IDs::cycleLength.getParamID())->setValueNotifyingHost(2.0f / 3.0f); initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceTime(); } };
    
    addAndMakeVisible (diceNavyButton); diceNavyButton.setComponentID ("dice_navy"); diceNavyButton.setButtonText ("Navy"); diceNavyButton.setLookAndFeel (&chromaLookAndFeel); 
    diceNavyButton.onClick = [this] { if (initButton.getToggleState()) { processor.apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::entropy.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::harmony.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::chaos.getParamID())->setValueNotifyingHost(0.0f); initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceNavy(); } };

    sceneAButton.onClick = [this] {
        if (initButton.getToggleState()) { processor.clearSceneA(); sceneAFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); }
        else if (copyButton.getToggleState()) { processor.saveSceneA(); sceneAFlashTimer = 24; copyButton.setToggleState (false, juce::dontSendNotification); copyButton.repaint(); }
    };
    sceneBButton.onClick = [this] {
        if (initButton.getToggleState()) { processor.clearSceneB(); sceneBFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); }
        else if (copyButton.getToggleState()) { processor.saveSceneB(); sceneBFlashTimer = 24; copyButton.setToggleState (false, juce::dontSendNotification); copyButton.repaint(); }
    };

    for (int i = 0; i < 8; ++i) { addAndMakeVisible (presetButtons[i]); presetButtons[i].setButtonText (juce::String (i + 1)); presetButtons[i].addMouseListener (this, false); presetButtons[i].setLookAndFeel (&chromaLookAndFeel); }

    addAndMakeVisible (rootKeyBox); addAndMakeVisible (scaleTypeBox); addAndMakeVisible (cycleLengthBox);
    rootKeyBox.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 1);
    scaleTypeBox.addItemList (juce::StringArray { "Major", "Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1);
    cycleLengthBox.addItemList (juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);

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
    arpSeqAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::arpSeq.getParamID(), arpSeqButton);
    polyAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::poly.getParamID(), polyButton);
    freezeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::freeze.getParamID(), freezeButton);

    rootKeyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::rootKey.getParamID(), rootKeyBox);
    scaleTypeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::scaleType.getParamID(), scaleTypeBox);
    cycleLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::cycleLength.getParamID(), cycleLengthBox);

    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);
    juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    for (auto* k : knobs) k->setColour (juce::Slider::textBoxTextColourId, t.textDim);

    setResizable (true, true); setResizeLimits (700, 460, 1400, 920); setSize (850, 560); startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); processor.apvts.removeParameterListener ("panelTheme", this);
    juce::Slider* sliders[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8, &morphCrossfader };
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
    juce::TextButton* btns[] = { &diceMeloButton, &diceArtiButton, &diceTimeButton, &diceNavyButton, &latchButton, &arpSeqButton, &polyButton, &freezeButton, &sceneAButton, &sceneBButton, &saveButton, &recallButton, &copyButton, &initButton };
    for (auto* b : btns) { b->setLookAndFeel (nullptr); b->onClick = nullptr; }
    for (int i = 0; i < 8; ++i) { presetButtons[i].setLookAndFeel (nullptr); presetButtons[i].onClick = nullptr; presetButtons[i].onStateChange = nullptr; presetButtons[i].removeMouseListener(this); }
    sceneAButton.removeMouseListener (this); sceneBButton.removeMouseListener (this);
}

void PluginEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    if (parameterID == "panelTheme") {
        juce::MessageManager::callAsync ([this]() {
            int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
            auto t = AppTheme::get (themeIdx);
            juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
            for (auto* k : knobs) k->setColour (juce::Slider::textBoxTextColourId, t.textDim);

            repaint(); oledDisplay.repaint();
            fader1.repaint(); fader2.repaint(); fader3.repaint(); fader4.repaint(); fader5.repaint(); fader6.repaint(); fader7.repaint(); fader8.repaint();
            rhythmMorphKnob.repaint(); restKnob.repaint(); legatoKnob.repaint(); rateKnob.repaint();
            entropyKnob.repaint(); harmonyKnob.repaint(); chaosKnob.repaint(); octavesKnob.repaint();
            morphCrossfader.repaint(); sceneAButton.repaint(); sceneBButton.repaint(); saveButton.repaint(); recallButton.repaint();
        });
    }
}

void PluginEditor::mouseDown (const juce::MouseEvent& event)
{
    for (int i = 0; i < 8; ++i) {
        if (event.eventComponent == &presetButtons[i]) {
            if (saveButton.getToggleState()) { presetPressStartTime[i] = juce::Time::getMillisecondCounter(); presetAlreadySaved[i] = false; }
            else if (recallButton.getToggleState()) {
                processor.loadPreset (i); presetFlashTimer[i] = 24; presetFlashType[i] = 2;
                recallButton.setToggleState (false, juce::dontSendNotification); recallButton.repaint();
            }
            else if (event.mods.isRightButtonDown()) { processor.savePreset (i); presetFlashTimer[i] = 24; presetFlashType[i] = 1; }
        }
    }

    if (event.eventComponent == &saveButton) { savePressStartTime = juce::Time::getMillisecondCounter(); saveAlreadySaved = false; }
    else if (event.eventComponent == &recallButton) { recallPressStartTime = juce::Time::getMillisecondCounter(); recallAlreadySaved = false; }
    else if (event.eventComponent == &copyButton) { copyPressStartTime = juce::Time::getMillisecondCounter(); copyAlreadySaved = false; }
    else if (event.eventComponent == &initButton) { initPressStartTime = juce::Time::getMillisecondCounter(); initAlreadySaved = false; }
    else if (event.eventComponent == &sceneAButton) { sceneAPressStartTime = juce::Time::getMillisecondCounter(); sceneAAlreadySaved = false; }
    else if (event.eventComponent == &sceneBButton) { sceneBPressStartTime = juce::Time::getMillisecondCounter(); sceneBAlreadySaved = false; }
}

void PluginEditor::mouseUp (const juce::MouseEvent& event)
{
    for (int i = 0; i < 8; ++i) { if (event.eventComponent == &presetButtons[i]) { presetPressStartTime[i] = 0; presetAlreadySaved[i] = false; } }
    if (event.eventComponent == &sceneAButton) { sceneAPressStartTime = 0; sceneAAlreadySaved = false; }
    if (event.eventComponent == &sceneBButton) { sceneBPressStartTime = 0; sceneBAlreadySaved = false; }
    if (event.eventComponent == &saveButton) { savePressStartTime = 0; saveAlreadySaved = false; }
    if (event.eventComponent == &recallButton) { recallPressStartTime = 0; recallAlreadySaved = false; }
    if (event.eventComponent == &copyButton) { copyPressStartTime = 0; copyAlreadySaved = false; }
    if (event.eventComponent == &initButton) { initPressStartTime = 0; initAlreadySaved = false; }
}

void PluginEditor::paint (juce::Graphics& g)
{
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load()); auto t = AppTheme::get (themeIdx);
    g.fillAll (t.background); g.setColour (t.border); g.drawRect (getLocalBounds().toFloat(), 3.0f);
    auto bounds = getLocalBounds().toFloat();
    g.drawRoundedRectangle (10.0f, 10.0f, 170.0f, bounds.getHeight() - 210.0f, 4.0f, 1.5f);
    g.drawRoundedRectangle (bounds.getWidth() - 180.0f, 10.0f, 170.0f, bounds.getHeight() - 210.0f, 4.0f, 1.5f);
    g.drawRoundedRectangle (10.0f, bounds.getHeight() - 190.0f, bounds.getWidth() - 20.0f, 180.0f, 4.0f, 1.5f);
    g.setColour (t.textDim); g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
    g.drawText ("Rhythm", 20, 15, 150, 20, juce::Justification::left); g.drawText ("Generator", getWidth() - 170, 15, 150, 20, juce::Justification::right);

    // Subtle 3D inset line around the display screen (Improvement 6)
    auto displayBounds = oledDisplay.getBounds().toFloat();
    g.setColour (juce::Colour (themeIdx == 1 ? 0xFFFFFFFF : 0x18FFFFFF));
    g.drawRoundedRectangle (displayBounds.expanded(1.0f), 2.0f, 1.0f);
    g.setColour (juce::Colour (themeIdx == 1 ? 0x25000000 : 0x75000000));
    g.drawRoundedRectangle (displayBounds, 2.0f, 1.0f);
}

void PluginEditor::resized()
{
    int bottomY = getHeight() - 180, centerWidth = getWidth() - 370, centerStartX = 185;
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    
    int rightX = getWidth() - 180, knobsAvailableHeight = bottomY - 120, knobHeight = juce::jlimit (50, 100, knobsAvailableHeight / 4), knobStartY = 38;
    for (int i = 0; i < 4; ++i) {
        int knobY = knobStartY + i * knobHeight;
        leftKnobs[i]->setBounds (15, knobY + 12, 150, knobHeight - 16); leftTitles[i]->setBounds (15, knobY, 150, 14);
        rightKnobs[i]->setBounds (rightX + 5, knobY + 12, 150, knobHeight - 16); rightTitles[i]->setBounds (rightX + 5, knobY, 150, 14);
    }

    int gridY = bottomY - 100;
    saveButton.setBounds (15, gridY, 72, 36); recallButton.setBounds (93, gridY, 72, 36); copyButton.setBounds (15, gridY + 42, 72, 36); initButton.setBounds (93, gridY + 42, 72, 36);
    int diceStartX = getWidth() - 165;
    diceMeloButton.setBounds (diceStartX, gridY, 72, 36); diceArtiButton.setBounds (diceStartX + 78, gridY, 72, 36); diceTimeButton.setBounds (diceStartX, gridY + 42, 72, 36); diceNavyButton.setBounds (diceStartX + 78, gridY + 42, 72, 36);

    int dropWidth = static_cast<int> ((centerWidth * 0.45f) / 3), perfWidth = static_cast<int> ((centerWidth * 0.55f) / 4);
    rootKeyBox.setBounds (centerStartX, 15, dropWidth - 5, 24); scaleTypeBox.setBounds (centerStartX + dropWidth, 15, dropWidth - 5, 24); cycleLengthBox.setBounds (centerStartX + dropWidth * 2, 15, dropWidth - 5, 24);
    int perfStartX = centerStartX + dropWidth * 3 + 10;
    latchButton.setBounds (perfStartX, 15, perfWidth - 5, 24); arpSeqButton.setBounds (perfStartX + perfWidth, 15, perfWidth - 5, 24); polyButton.setBounds (perfStartX + perfWidth * 2, 15, perfWidth - 5, 24); freezeButton.setBounds (perfStartX + perfWidth * 3, 15, perfWidth - 5, 24);

    int oledY = 50, presetsY = static_cast<int> (gridY + 6), crossfaderY = static_cast<int> (gridY + 48), oledHeight = presetsY - oledY - 10;
    oledDisplay.setBounds (centerStartX, oledY, centerWidth, oledHeight);

    // Proportional Visual Morphing Alignment for the centered Crossfader strip
    int presetBtnW = (centerWidth - 35) / 8;
    for (int i = 0; i < 8; ++i) presetButtons[i].setBounds (centerStartX + i * (presetBtnW + 5), presetsY, presetBtnW, 24);

    int rowWidth = 290;
    int rowStartX = centerStartX + (centerWidth - rowWidth) / 2;
    sceneAButton.setBounds (rowStartX, crossfaderY, 40, 24);
    morphCrossfader.setBounds (rowStartX + 45, crossfaderY, 200, 24);
    sceneBButton.setBounds (rowStartX + 250, crossfaderY, 40, 24);

    int faderWidth = (getWidth() - 40) / 8;
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    for (int i = 0; i < 8; ++i) {
        int faderX = 20 + i * faderWidth; faders[i]->setBounds (faderX + 10, bottomY + 10, faderWidth - 20, 130); faderLabels[i]->setBounds (faderX, bottomY + 145, faderWidth, 20);
    }
}

void PluginEditor::timerCallback()
{
    uint32_t now = juce::Time::getMillisecondCounter();
    bool isArp = *processor.apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f; arpSeqButton.setButtonText (isArp ? "Arp" : "Seq");

    static bool lastAnchorB = false; bool currentAnchorB = processor.isSceneBActiveAnchor.load();
    if (currentAnchorB != lastAnchorB) { lastAnchorB = currentAnchorB; sceneAButton.repaint(); sceneBButton.repaint(); }

    // Dynamic contrast & "Ice Blue" active indicators for freeze mode (Improvement 7)
    static bool lastFreezeState = false;
    bool currentFreeze = *processor.apvts.getRawParameterValue (IDs::freeze.getParamID()) > 0.5f;
    if (currentFreeze != lastFreezeState) {
        lastFreezeState = currentFreeze;
        int activeThemeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (activeThemeIdx);
        juce::Colour textCol = currentFreeze ? juce::Colour (0xFF80D8FF) : t.textDim;
        
        juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
        for (auto* k : knobs) { k->setColour (juce::Slider::textBoxTextColourId, textCol); k->repaint(); }
        freezeButton.repaint();
    }

    if (sceneAFlashTimer > 0) { sceneAFlashTimer--; if (sceneAFlashTimer == 0) sceneAButton.repaint(); }
    if (sceneBFlashTimer > 0) { sceneBFlashTimer--; if (sceneBFlashTimer == 0) sceneBButton.repaint(); }
    if (saveFlashTimer > 0) { saveFlashTimer--; if (saveFlashTimer == 0) saveButton.repaint(); }
    if (recallFlashTimer > 0) { recallFlashTimer--; if (recallFlashTimer == 0) recallButton.repaint(); }
    if (copyFlashTimer > 0) { copyFlashTimer--; if (copyFlashTimer == 0) copyButton.repaint(); }
    if (initFlashTimer > 0) { initFlashTimer--; if (initFlashTimer == 0) initButton.repaint(); }

    // Real-Time Visual Morphing / Interpolation for UI Dials & Faders
    float morphVal = static_cast<float> (morphCrossfader.getValue());
    auto interpolate = [morphVal](float valA, float valB) -> float {
        return (valA * (1.0f - morphVal)) + (valB * morphVal);
    };

    // Knobs visual updates
    if (!rhythmMorphKnob.isMouseButtonDown()) rhythmMorphKnob.setValue (interpolate (processor.sceneA.rhythmMorph, processor.sceneB.rhythmMorph), juce::dontSendNotification);
    if (!restKnob.isMouseButtonDown())        restKnob.setValue (interpolate (processor.sceneA.rest,        processor.sceneB.rest),        juce::dontSendNotification);
    if (!legatoKnob.isMouseButtonDown())      legatoKnob.setValue (interpolate (processor.sceneA.legato,    processor.sceneB.legato),      juce::dontSendNotification);
    if (!rateKnob.isMouseButtonDown())        rateKnob.setValue (interpolate (processor.sceneA.rate,        processor.sceneB.rate),        juce::dontSendNotification);
    if (!entropyKnob.isMouseButtonDown())     entropyKnob.setValue (interpolate (processor.sceneA.entropy,   processor.sceneB.entropy),   juce::dontSendNotification);
    if (!harmonyKnob.isMouseButtonDown())     harmonyKnob.setValue (interpolate (processor.sceneA.harmony,   processor.sceneB.harmony),   juce::dontSendNotification);
    if (!chaosKnob.isMouseButtonDown())       chaosKnob.setValue (interpolate (processor.sceneA.chaos,     processor.sceneB.chaos),     juce::dontSendNotification);
    if (!octavesKnob.isMouseButtonDown())     octavesKnob.setValue (interpolate (processor.sceneA.octaves,   processor.sceneB.octaves),   juce::dontSendNotification);

    // Upfaders visual updates
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    for (int i = 0; i < 8; ++i) {
        if (!faders[i]->isMouseButtonDown()) {
            faders[i]->setValue (interpolate (processor.sceneA.faders[i], processor.sceneB.faders[i]), juce::dontSendNotification);
        }
    }

    for (int i = 0; i < 8; ++i) {
        if (presetButtons[i].isMouseButtonDown() && presetPressStartTime[i] != 0 && !presetAlreadySaved[i]) {
            if (now - presetPressStartTime[i] >= 1000) {
                processor.savePreset (i); presetAlreadySaved[i] = true; presetFlashTimer[i] = 24; presetFlashType[i] = 1;
                if (saveButton.getToggleState()) { saveButton.setToggleState (false, juce::dontSendNotification); saveButton.repaint(); }
            }
        }
        if (presetFlashTimer[i] > 0) { presetFlashTimer[i]--; if (presetFlashTimer[i] == 0) presetButtons[i].repaint(); }
    }

    if (sceneAButton.isMouseButtonDown() && sceneAPressStartTime != 0 && !sceneAAlreadySaved) { if (now - sceneAPressStartTime >= 1000) { processor.setActiveAnchor (false); sceneAAlreadySaved = true; sceneAFlashTimer = 24; } }
    if (sceneBButton.isMouseButtonDown() && sceneBPressStartTime != 0 && !sceneBAlreadySaved) { if (now - sceneBPressStartTime >= 1000) { processor.setActiveAnchor (true); sceneBAlreadySaved = true; sceneBFlashTimer = 24; } }

    if (saveButton.isMouseButtonDown() && savePressStartTime != 0 && !saveAlreadySaved) { if (now - savePressStartTime >= 1000) { processor.savePreset (processor.activePresetIndex.load()); saveAlreadySaved = true; saveFlashTimer = 24; saveButton.setToggleState (false, juce::dontSendNotification); saveButton.repaint(); } }
    if (recallButton.isMouseButtonDown() && recallPressStartTime != 0 && !recallAlreadySaved) { if (now - recallPressStartTime >= 1000) { processor.loadPreset (processor.activePresetIndex.load()); recallAlreadySaved = true; recallFlashTimer = 24; recallButton.setToggleState (false, juce::dontSendNotification); recallButton.repaint(); } }
    if (copyButton.isMouseButtonDown() && copyPressStartTime != 0 && !copyAlreadySaved) { if (now - copyPressStartTime >= 1000) { processor.sceneB = processor.sceneA; processor.hasSceneB = processor.hasSceneA; copyAlreadySaved = true; copyFlashTimer = 24; copyButton.setToggleState (false, juce::dontSendNotification); copyButton.repaint(); } }
    if (initButton.isMouseButtonDown() && initPressStartTime != 0 && !initAlreadySaved) {
        if (now - initPressStartTime >= 1000) {
            for (auto* param : processor.getParameters()) { if (param != nullptr) param->setValueNotifyingHost (param->getDefaultValue()); }
            initAlreadySaved = true; initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint();
        }
    }
}