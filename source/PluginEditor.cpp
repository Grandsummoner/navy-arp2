#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p)
{
    addAndMakeVisible (oledDisplay);

    processor.apvts.addParameterListener ("panelTheme", this);

    // Bottom faders
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    juce::String scaleNotes[] = { "C", "D", "Eb", "F", "G", "Ab", "Bb", "C" };

    for (int i = 0; i < 8; ++i)
    {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical);
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setColour (juce::Slider::thumbColourId, juce::Colour (0xFF00D2FF));
        faders[i]->setColour (juce::Slider::trackColourId, juce::Colour (0xFF181C24));
        faders[i]->setLookAndFeel (&chromaLookAndFeel); 
        addAndMakeVisible (faders[i]);

        faderLabels[i]->setText (scaleNotes[i], juce::dontSendNotification);
        faderLabels[i]->setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold"))); 
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (faderLabels[i]);
    }

    // Left sidebar knobs and Titles
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::String leftNames[] = { "Morph", "Rest", "Legato", "Rate" }; 
    juce::String leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };

    for (int i = 0; i < 4; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF)); 
        leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        leftKnobs[i]->setComponentID (leftPrefixes[i]); 
        leftKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (leftKnobs[i]);

        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification);
        leftTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); 
        leftTitles[i]->setJustificationType (juce::Justification::centred);
        leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
        addAndMakeVisible (leftTitles[i]);
    }

    // Right sidebar knobs and Titles
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::String rightNames[] = { "Entropy", "Harmony", "Chaos", "Octaves" }; 
    juce::String rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };

    for (int i = 0; i < 4; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); 
        rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        rightKnobs[i]->setComponentID (rightPrefixes[i]); 
        rightKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (rightKnobs[i]);

        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification);
        rightTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); 
        rightTitles[i]->setJustificationType (juce::Justification::centred);
        rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
        addAndMakeVisible (rightTitles[i]);
    }

    // Crossfader
    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal);
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF));
    morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF222222));
    morphCrossfader.setLookAndFeel (&chromaLookAndFeel); 
    addAndMakeVisible (morphCrossfader);

    // Performance Deck Buttons (Symmetrical 4 buttons with Sentence Case)
    addAndMakeVisible (latchButton);
    latchButton.setButtonText ("Latch");
    latchButton.setClickingTogglesState (true);

    addAndMakeVisible (arpSeqButton);
    arpSeqButton.setButtonText ("SEQ"); // Swapped dynamically in timerCallback
    arpSeqButton.setClickingTogglesState (true);

    addAndMakeVisible (polyButton);
    polyButton.setButtonText ("Poly");
    polyButton.setClickingTogglesState (true);

    addAndMakeVisible (freezeButton);
    freezeButton.setButtonText ("Freeze");
    freezeButton.setClickingTogglesState (true);

    // Symmetrical Octatrack Scene Buttons
    addAndMakeVisible (sceneAButton);
    sceneAButton.setButtonText ("A");
    sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF151515));
    sceneAButton.addMouseListener (this, false); 

    addAndMakeVisible (sceneBButton);
    sceneBButton.setButtonText ("B");
    sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF151515));
    sceneBButton.addMouseListener (this, false); 

    // Left-Hand 2x2 Utility Grid (Sentence Case, no shouting)
    addAndMakeVisible (saveButton);
    saveButton.setButtonText ("Save");
    saveButton.setClickingTogglesState (true);
    saveButton.addMouseListener (this, false); 
    saveButton.onClick = [this] { if (saveButton.getToggleState()) recallButton.setToggleState (false, juce::dontSendNotification); };

    addAndMakeVisible (recallButton);
    recallButton.setButtonText ("Recall");
    recallButton.setClickingTogglesState (true);
    recallButton.setToggleState (false, juce::dontSendNotification); 
    recallButton.addMouseListener (this, false);
    recallButton.onClick = [this] { if (recallButton.getToggleState()) saveButton.setToggleState (false, juce::dontSendNotification); };

    addAndMakeVisible (copyButton);
    copyButton.setButtonText ("Copy");
    copyButton.setClickingTogglesState (true);
    copyButton.addMouseListener (this, false);

    addAndMakeVisible (initButton);
    initButton.setButtonText ("Init");
    initButton.setClickingTogglesState (true);
    initButton.addMouseListener (this, false);

    // Right-Hand 2x2 Dice Buttons (Melo, Arti, Time, Navy)
    addAndMakeVisible (diceMeloButton);
    diceMeloButton.setComponentID ("dice_melody");
    diceMeloButton.setButtonText ("Melo");
    diceMeloButton.setLookAndFeel (&chromaLookAndFeel);
    diceMeloButton.onClick = [this] { processor.diceMelody(); };

    addAndMakeVisible (diceArtiButton);
    diceArtiButton.setComponentID ("dice_articulation");
    diceArtiButton.setButtonText ("Arti");
    diceArtiButton.setLookAndFeel (&chromaLookAndFeel);
    diceArtiButton.onClick = [this] { processor.diceArticulation(); };

    addAndMakeVisible (diceTimeButton);
    diceTimeButton.setComponentID ("dice_time");
    diceTimeButton.setButtonText ("Time");
    diceTimeButton.setLookAndFeel (&chromaLookAndFeel);
    diceTimeButton.onClick = [this] { processor.diceTime(); };

    addAndMakeVisible (diceNavyButton);
    diceNavyButton.setComponentID ("dice_navy");
    diceNavyButton.setButtonText ("Navy");
    diceNavyButton.setLookAndFeel (&chromaLookAndFeel);
    diceNavyButton.onClick = [this] { processor.diceNavy(); };

    // 8 Preset Slots with Custom Mouse Listener bindings (No instant onClick actions)
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].addMouseListener (this, false);
    }

    // Configure Key & Scale Dropdowns
    addAndMakeVisible (rootKeyBox);
    addAndMakeVisible (scaleTypeBox);
    addAndMakeVisible (cycleLengthBox);

    rootKeyBox.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 1);
    scaleTypeBox.addItemList (juce::StringArray { "Major", "Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1);
    cycleLengthBox.addItemList (juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);

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
    arpSeqAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::arpSeq.getParamID(), arpSeqButton);
    polyAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::poly.getParamID(), polyButton);
    freezeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::freeze.getParamID(), freezeButton);

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

    processor.apvts.removeParameterListener ("panelTheme", this);
    
    rhythmMorphKnob.setLookAndFeel (nullptr);
    restKnob.setLookAndFeel (nullptr);
    legatoKnob.setLookAndFeel (nullptr);
    rateKnob.setLookAndFeel (nullptr);
    
    entropyKnob.setLookAndFeel (nullptr);
    harmonyKnob.setLookAndFeel (nullptr);
    chaosKnob.setLookAndFeel (nullptr);
    octavesKnob.setLookAndFeel (nullptr);

    fader1.setLookAndFeel (nullptr);
    fader2.setLookAndFeel (nullptr);
    fader3.setLookAndFeel (nullptr);
    fader4.setLookAndFeel (nullptr);
    fader5.setLookAndFeel (nullptr);
    fader6.setLookAndFeel (nullptr);
    fader7.setLookAndFeel (nullptr);
    fader8.setLookAndFeel (nullptr);
    morphCrossfader.setLookAndFeel (nullptr);

    diceMeloButton.setLookAndFeel (nullptr);
    diceArtiButton.setLookAndFeel (nullptr);
    diceTimeButton.setLookAndFeel (nullptr);
    diceNavyButton.setLookAndFeel (nullptr);

    diceMeloButton.onClick = nullptr;
    diceArtiButton.onClick = nullptr;
    diceTimeButton.onClick = nullptr;
    diceNavyButton.onClick = nullptr;

    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].onClick = nullptr;
        presetButtons[i].onStateChange = nullptr;
        presetButtons[i].removeMouseListener(this); 
    }

    sceneAButton.removeMouseListener (this);
    sceneBButton.removeMouseListener (this);
}

void PluginEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    
    if (parameterID == "panelTheme")
    {
        juce::MessageManager::callAsync ([this]() {
            repaint();
            oledDisplay.repaint();
            
            fader1.repaint(); fader2.repaint(); fader3.repaint(); fader4.repaint();
            fader5.repaint(); fader6.repaint(); fader7.repaint(); fader8.repaint();
            
            rhythmMorphKnob.repaint(); restKnob.repaint(); legatoKnob.repaint(); rateKnob.repaint();
            entropyKnob.repaint(); harmonyKnob.repaint(); chaosKnob.repaint(); octavesKnob.repaint();
            morphCrossfader.repaint();

            sceneAButton.repaint();
            sceneBButton.repaint();
            saveButton.repaint();
            recallButton.repaint();
        });
    }
}

void PluginEditor::mouseDown (const juce::MouseEvent& event)
{
    // 1. Preset Slot Interactions (Recall is instant-tap with auto-reset, Save is hold-to-save)
    for (int i = 0; i < 8; ++i)
    {
        if (event.eventComponent == &presetButtons[i])
        {
            if (saveButton.getToggleState()) // SAVE mode armed -> hold to write
            {
                presetPressStartTime[i] = juce::Time::getMillisecondCounter();
                presetAlreadySaved[i] = false;
            }
            else if (recallButton.getToggleState()) // RECALL mode armed -> instant tap load
            {
                processor.loadPreset (i);
                presetFlashTimer[i] = 24; 
                presetFlashType[i] = 2; // Cyan flash
                
                recallButton.setToggleState (false, juce::dontSendNotification);
            }
            else // Neither is armed -> fallback to right-click safety save
            {
                if (event.mods.isRightButtonDown())
                {
                    processor.savePreset (i);
                    presetFlashTimer[i] = 24;
                    presetFlashType[i] = 1; // Amber
                }
            }
        }
    }

    // 2. Left-hand 2x2 Utility long-press armings (Latching Modifier Engine)
    if (event.eventComponent == &saveButton)
    {
        savePressStartTime = juce::Time::getMillisecondCounter();
        saveAlreadySaved = false;
    }
    else if (event.eventComponent == &recallButton)
    {
        recallPressStartTime = juce::Time::getMillisecondCounter();
        recallAlreadySaved = false;
    }
    else if (event.eventComponent == &copyButton)
    {
        copyPressStartTime = juce::Time::getMillisecondCounter();
        copyAlreadySaved = false;
    }
    else if (event.eventComponent == &initButton)
    {
        initPressStartTime = juce::Time::getMillisecondCounter();
        initAlreadySaved = false;
    }

    // Symmetrical hold trackers for scene buttons
    if (event.eventComponent == &sceneAButton)
    {
        sceneAPressStartTime = juce::Time::getMillisecondCounter();
        sceneAAlreadySaved = false;
    }
    else if (event.eventComponent == &sceneBButton)
    {
        sceneBPressStartTime = juce::Time::getMillisecondCounter();
        sceneBAlreadySaved = false;
    }

    // 3. Sequential Latching Modifier Actions
    if (initButton.getToggleState())
    {
        bool actionTriggered = false;
        if (event.eventComponent == &diceMeloButton)
        {
            processor.resetRhythm(); // Clears faders to flat 100%
            actionTriggered = true;
        }
        else if (event.eventComponent == &diceArtiButton)
        {
            processor.apvts.getParameter(IDs::rest.getParamID())->setValueNotifyingHost(0.0f);
            processor.apvts.getParameter(IDs::legato.getParamID())->setValueNotifyingHost(0.5f);
            actionTriggered = true;
        }
        else if (event.eventComponent == &diceTimeButton)
        {
            processor.apvts.getParameter(IDs::rate.getParamID())->setValueNotifyingHost(2.0f / 3.0f); // 1/16
            processor.apvts.getParameter(IDs::octaves.getParamID())->setValueNotifyingHost(3.0f / 6.0f); // +0 Octaves
            processor.apvts.getParameter(IDs::cycleLength.getParamID())->setValueNotifyingHost(2.0f / 3.0f); // 4 Bars
            actionTriggered = true;
        }
        else if (event.eventComponent == &diceNavyButton)
        {
            processor.apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f);
            processor.apvts.getParameter(IDs::entropy.getParamID())->setValueNotifyingHost(0.0f);
            processor.apvts.getParameter(IDs::harmony.getParamID())->setValueNotifyingHost(0.0f);
            processor.apvts.getParameter(IDs::chaos.getParamID())->setValueNotifyingHost(0.0f);
            actionTriggered = true;
        }
        else if (event.eventComponent == &sceneAButton)
        {
            processor.clearSceneA();
            sceneAFlashTimer = 24; // Visual confirmation flash
            actionTriggered = true;
        }
        else if (event.eventComponent == &sceneBButton)
        {
            processor.clearSceneB();
            sceneBFlashTimer = 24; // Visual confirmation flash
            actionTriggered = true;
        }

        if (actionTriggered)
        {
            initButton.setToggleState (false, juce::dontSendNotification);
            initButton.repaint();
            return;
        }
    }

    if (copyButton.getToggleState())
    {
        bool actionTriggered = false;
        if (event.eventComponent == &sceneAButton)
        {
            processor.saveSceneA();
            sceneAFlashTimer = 24;
            actionTriggered = true;
        }
        else if (event.eventComponent == &sceneBButton)
        {
            processor.saveSceneB();
            sceneBFlashTimer = 24;
            actionTriggered = true;
        }

        if (actionTriggered)
        {
            copyButton.setToggleState (false, juce::dontSendNotification);
            copyButton.repaint();
            return;
        }
    }

    // 4. Right-Click LFO Modulation Menu Popup
    if (event.mods.isRightButtonDown() && event.eventComponent != this)
    {
        juce::Slider* clickedSlider = nullptr;
        juce::String paramPrefix = "";
        
        juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
        juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
        juce::String leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };
        juce::String rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };
        
        for (int i = 0; i < 4; ++i)
        {
            if (event.eventComponent == leftKnobs[i])  { clickedSlider = leftKnobs[i];  paramPrefix = leftPrefixes[i];  break; }
            if (event.eventComponent == rightKnobs[i]) { clickedSlider = rightKnobs[i]; paramPrefix = rightPrefixes[i]; break; }
        }
        
        if (clickedSlider != nullptr)
        {
            juce::PopupMenu menu;
            auto* rateParam = processor.apvts.getParameter (paramPrefix + "LfoRate");
            auto* depthParam = processor.apvts.getParameter (paramPrefix + "LfoDepth");
            
            if (rateParam != nullptr && depthParam != nullptr)
            {
                int currentRateChoice = static_cast<int> (rateParam->getValue() * 4.0f);
                float currentDepth = depthParam->getValue();
                
                menu.addSectionHeader ("LFO MODULATION");
                menu.addItem (1, "Disable LFO", true, (currentRateChoice == 0)); 
                
                menu.addSeparator();
                juce::PopupMenu rateMenu;
                rateMenu.addItem (10, "1/4 Note", true, (currentRateChoice == 1));
                rateMenu.addItem (11, "1/8 Note", true, (currentRateChoice == 2));
                rateMenu.addItem (12, "1/16 Note", true, (currentRateChoice == 3));
                rateMenu.addItem (13, "1/32 Note", true, (currentRateChoice == 4));
                menu.addSubMenu ("LFO Speed / Rate", rateMenu);
                
                juce::PopupMenu depthMenu;
                depthMenu.addItem (20, "Off (0%)", true, (currentDepth == 0.0f));
                depthMenu.addItem (21, "Slight (10%)", true, (currentDepth > 0.05f && currentDepth <= 0.15f));
                depthMenu.addItem (22, "Medium (25%)", true, (currentDepth > 0.2f && currentDepth <= 0.3f));
                depthMenu.addItem (23, "Heavy (50%)", true, (currentDepth > 0.45f && currentDepth <= 0.55f));
                depthMenu.addItem (24, "Full (100%)", true, (currentDepth > 0.95f));
                menu.addSubMenu ("LFO Depth / Range", depthMenu);
                
                int result = menu.showAt (clickedSlider);
                if (result == 1) // Disable LFO
                {
                    rateParam->setValueNotifyingHost (0.0f);
                    depthParam->setValueNotifyingHost (0.0f);
                }
                else if (result >= 10 && result <= 13) // Speed Choices
                {
                    float rateVal = static_cast<float> (result - 9) / 4.0f;
                    rateParam->setValueNotifyingHost (rateVal);
                    if (depthParam->getValue() == 0.0f) {
                        depthParam->setValueNotifyingHost (0.25f); // Default to 25% depth
                    }
                }
                else if (result >= 20 && result <= 24) // Depth Choices
                {
                    float depthVal = 0.0f;
                    if (result == 21)      depthVal = 0.1f;
                    else if (result == 22) depthVal = 0.25f;
                    else if (result == 23) depthVal = 0.5f;
                    else if (result == 24) depthVal = 1.0f;
                    depthParam->setValueNotifyingHost (depthVal);
                    
                    if (rateParam->getValue() == 0.0f) {
                        rateParam->setValueNotifyingHost (0.5f); // Default speed
                    }
                }
            }
        }
    }
}

void PluginEditor::mouseUp (const juce::MouseEvent& event)
{
    // Reset press start times on release to prevent repeat triggers
    for (int i = 0; i < 8; ++i)
    {
        if (event.eventComponent == &presetButtons[i])
        {
            presetPressStartTime[i] = 0;
            presetAlreadySaved[i] = false;
        }
    }

    if (event.eventComponent == &sceneAButton)   { sceneAPressStartTime = 0; sceneAAlreadySaved = false; }
    if (event.eventComponent == &sceneBButton)   { sceneBPressStartTime = 0; sceneBAlreadySaved = false; }
    if (event.eventComponent == &saveButton)     { savePressStartTime = 0; saveAlreadySaved = false; }
    if (event.eventComponent == &recallButton)   { recallPressStartTime = 0; recallAlreadySaved = false; }
    if (event.eventComponent == &copyButton)     { copyPressStartTime = 0; copyAlreadySaved = false; }
    if (event.eventComponent == &initButton)     { initPressStartTime = 0; initAlreadySaved = false; }
}

void PluginEditor::paint (juce::Graphics& g)
{
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    g.fillAll (t.background);

    // Subtle outline
    g.setColour (t.border);
    g.drawRect (getLocalBounds().toFloat(), 3.0f);

    // Section outlines
    auto bounds = getLocalBounds().toFloat();
    
    // Left Sidebar panel border
    g.drawRoundedRectangle (10.0f, 10.0f, 170.0f, bounds.getHeight() - 210.0f, 4.0f, 1.5f);
    
    // Right Sidebar panel border
    g.drawRoundedRectangle (bounds.getWidth() - 180.0f, 10.0f, 170.0f, bounds.getHeight() - 210.0f, 4.0f, 1.5f);

    // Bottom faders panel border
    g.drawRoundedRectangle (10.0f, bounds.getHeight() - 190.0f, bounds.getWidth() - 20.0f, 180.0f, 4.0f, 1.5f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    int bottomY = getHeight() - 180;
    int leftPanelHeight = getHeight() - 210;

    // Bottom Faders layout
    int faderWidth = (getWidth() - 40) / 8;
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    
    for (int i = 0; i < 8; ++i)
    {
        int faderX = 20 + i * faderWidth;
        faders[i]->setBounds (faderX + 10, bottomY + 10, faderWidth - 20, 130);
        faderLabels[i]->setBounds (faderX, bottomY + 145, faderWidth, 20);
    }

    // Left sidebar layout
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    int knobHeight = (leftPanelHeight - 20) / 4;
    for (int i = 0; i < 4; ++i)
    {
        int knobY = 15 + i * knobHeight;
        leftKnobs[i]->setBounds (20, knobY + 12, 150, knobHeight - 16);
        leftTitles[i]->setBounds (20, knobY, 150, 14);
    }

    // Right sidebar layout
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    int rightX = getWidth() - 180;
    for (int i = 0; i < 4; ++i)
    {
        int knobY = 15 + i * knobHeight;
        rightKnobs[i]->setBounds (rightX + 10, knobY + 12, 150, knobHeight - 16);
        rightTitles[i]->setBounds (rightX + 10, knobY, 150, 14);
    }

    // Center layout calculations
    int centerWidth = getWidth() - 370;
    
    // Dropdowns
    int dropWidth = (centerWidth - 20) / 3;
    rootKeyBox.setBounds (190, 15, dropWidth, 24);
    scaleTypeBox.setBounds (190 + dropWidth + 10, 15, dropWidth, 24);
    cycleLengthBox.setBounds (190 + (dropWidth + 10) * 2, 15, dropWidth, 24);

    // OLED Display
    int oledY = 50;
    int oledHeight = 150;
    oledDisplay.setBounds (190, oledY, centerWidth, oledHeight);

    // Crossfader and Scene Anchors
    int crossfaderY = oledY + oledHeight + 15;
    sceneAButton.setBounds (190, crossfaderY, 40, 24);
    morphCrossfader.setBounds (235, crossfaderY, centerWidth - 90, 24);
    sceneBButton.setBounds (190 + centerWidth - 40, crossfaderY, 40, 24);

    // Utility & Performance Grid Section
    int deckY = crossfaderY + 35;
    int deckHeight = leftPanelHeight - deckY;
    
    // Left-hand 2x2 Utility Grid
    int utilWidth = 90;
    int btnW = (utilWidth - 5) / 2;
    int btnH = (deckHeight - 5) / 2;
    saveButton.setBounds (190, deckY, btnW, btnH);
    recallButton.setBounds (190 + btnW + 5, deckY, btnW, btnH);
    copyButton.setBounds (190, deckY + btnH + 5, btnW, btnH);
    initButton.setBounds (190 + btnW + 5, deckY + btnH + 5, btnW, btnH);

    // Right-hand 2x2 Dice Grid
    int diceX = 190 + centerWidth - utilWidth;
    diceMeloButton.setBounds (diceX, deckY, btnW, btnH);
    diceArtiButton.setBounds (diceX + btnW + 5, deckY, btnW, btnH);
    diceTimeButton.setBounds (diceX, deckY + btnH + 5, btnW, btnH);
    diceNavyButton.setBounds (diceX + btnW + 5, deckY + btnH + 5, btnW, btnH);

    // Performance Deck (Center 4 Buttons)
    int perfX = 190 + utilWidth + 10;
    int perfWidth = centerWidth - (utilWidth * 2) - 20;
    int perfBtnW = (perfWidth - 15) / 4;
    
    latchButton.setBounds (perfX, deckY, perfBtnW, btnH);
    arpSeqButton.setBounds (perfX + perfBtnW + 5, deckY, perfBtnW, btnH);
    polyButton.setBounds (perfX + (perfBtnW + 5) * 2, deckY, perfBtnW, btnH);
    freezeButton.setBounds (perfX + (perfBtnW + 5) * 3, deckY, perfBtnW, btnH);

    // 8 Preset Buttons below center buttons
    int presetY = deckY + btnH + 5;
    int presetBtnW = (perfWidth - 35) / 8;
    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].setBounds (perfX + i * (presetBtnW + 5), presetY, presetBtnW, btnH);
    }
}

void PluginEditor::timerCallback()
{
    uint32_t now = juce::Time::getMillisecondCounter();
    
    // Arp/Seq dynamic label updates
    bool isArp = *processor.apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f;
    arpSeqButton.setButtonText (isArp ? "Arp" : "Seq");

    // Flash timer decays
    if (sceneAFlashTimer > 0) { sceneAFlashTimer--; if (sceneAFlashTimer == 0) sceneAButton.repaint(); }
    if (sceneBFlashTimer > 0) { sceneBFlashTimer--; if (sceneBFlashTimer == 0) sceneBButton.repaint(); }
    if (saveFlashTimer > 0) { saveFlashTimer--; if (saveFlashTimer == 0) saveButton.repaint(); }
    if (recallFlashTimer > 0) { recallFlashTimer--; if (recallFlashTimer == 0) recallButton.repaint(); }
    if (copyFlashTimer > 0) { copyFlashTimer--; if (copyFlashTimer == 0) copyButton.repaint(); }
    if (initFlashTimer > 0) { initFlashTimer--; if (initFlashTimer == 0) initButton.repaint(); }

    // Preset Slot hold-to-save checks and decays
    for (int i = 0; i < 8; ++i)
    {
        if (presetButtons[i].isMouseButtonDown() && presetPressStartTime[i] != 0 && !presetAlreadySaved[i])
        {
            if (now - presetPressStartTime[i] >= 1000)
            {
                processor.savePreset (i);
                presetAlreadySaved[i] = true;
                presetFlashTimer[i] = 24;
                presetFlashType[i] = 1; // Amber confirmation flash
                
                if (saveButton.getToggleState())
                    saveButton.setToggleState (false, juce::dontSendNotification);
            }
        }
        
        if (presetFlashTimer[i] > 0)
        {
            presetFlashTimer[i]--;
            if (presetFlashTimer[i] == 0)
                presetButtons[i].repaint();
        }
    }

    // Scene long-press check
    if (sceneAButton.isMouseButtonDown() && sceneAPressStartTime != 0 && !sceneAAlreadySaved)
    {
        if (now - sceneAPressStartTime >= 1000)
        {
            processor.saveSceneA();
            sceneAAlreadySaved = true;
            sceneAFlashTimer = 24;
        }
    }
    if (sceneBButton.isMouseButtonDown() && sceneBPressStartTime != 0 && !sceneBAlreadySaved)
    {
        if (now - sceneBPressStartTime >= 1000)
        {
            processor.saveSceneB();
            sceneBAlreadySaved = true;
            sceneBFlashTimer = 24;
        }
    }

    // Utility 2x2 long press checks
    if (saveButton.isMouseButtonDown() && savePressStartTime != 0 && !saveAlreadySaved)
    {
        if (now - savePressStartTime >= 1000)
        {
            int activeSlot = processor.activePresetIndex.load();
            processor.savePreset (activeSlot);
            saveAlreadySaved = true;
            saveFlashTimer = 24;
            saveButton.setToggleState (false, juce::dontSendNotification);
        }
    }
    if (recallButton.isMouseButtonDown() && recallPressStartTime != 0 && !recallAlreadySaved)
    {
        if (now - recallPressStartTime >= 1000)
        {
            int activeSlot = processor.activePresetIndex.load();
            processor.loadPreset (activeSlot);
            recallAlreadySaved = true;
            recallFlashTimer = 24;
            recallButton.setToggleState (false, juce::dontSendNotification);
        }
    }
    if (copyButton.isMouseButtonDown() && copyPressStartTime != 0 && !copyAlreadySaved)
    {
        if (now - copyPressStartTime >= 1000)
        {
            processor.sceneB = processor.sceneA;
            processor.hasSceneB = processor.hasSceneA;
            copyAlreadySaved = true;
            copyFlashTimer = 24;
            copyButton.setToggleState (false, juce::dontSendNotification);
        }
    }
    if (initButton.isMouseButtonDown() && initPressStartTime != 0 && !initAlreadySaved)
    {
        if (now - initPressStartTime >= 1000)
        {
            auto& apvts = processor.apvts;
            for (auto& param : apvts.getProcessor().getParameters())
                param->setValueNotifyingHost (param->getDefaultValue());
            initAlreadySaved = true;
            initFlashTimer = 24;
            initButton.setToggleState (false, juce::dontSendNotification);
        }
    }
}