#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p)
{
    addAndMakeVisible (oledDisplay);

    // Register active parameter listener for the Theme switcher [3] 
    processor.apvts.addParameterListener ("panelTheme", this);

    // Bottom faders (Linear Chroma Customization) [5]
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
        faderLabels[i]->setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold"))); // Corrected for JUCE 8 [NEW]
        faderLabels[i]->setJustificationType (juce::Justification::centred);
        faderLabels[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (faderLabels[i]);
    }

    // Left sidebar knobs and Titles
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::String leftNames[] = { "MORPH", "REST", "LEGATO", "RATE" };
    juce::String leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };

    for (int i = 0; i < 4; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF)); 
        leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        leftKnobs[i]->setComponentID (leftPrefixes[i]); 
        leftKnobs[i]->addMouseListener (this, false); // Listen for right clicks [5]
        addAndMakeVisible (leftKnobs[i]);

        leftTitles[i]->setText (leftNames[i], juce::dontSendNotification);
        leftTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); // Corrected for JUCE 8 [NEW]
        leftTitles[i]->setJustificationType (juce::Justification::centred);
        leftTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
        addAndMakeVisible (leftTitles[i]);
    }

    // Right sidebar knobs and Titles
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::String rightNames[] = { "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };
    juce::String rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };

    for (int i = 0; i < 4; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300)); 
        rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        rightKnobs[i]->setComponentID (rightPrefixes[i]); 
        rightKnobs[i]->addMouseListener (this, false); // Listen for right clicks [5]
        addAndMakeVisible (rightKnobs[i]);

        rightTitles[i]->setText (rightNames[i], juce::dontSendNotification);
        rightTitles[i]->setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold"))); // Corrected for JUCE 8 [NEW]
        rightTitles[i]->setJustificationType (juce::Justification::centred);
        rightTitles[i]->setColour (juce::Label::textColourId, juce::Colour (0xFF55555C));
        addAndMakeVisible (rightTitles[i]);
    }

    // Crossfader
    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal);
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setColour (juce::Slider::thumbColourId, juce::Colour (0xFFFFFFFF));
    morphCrossfader.setColour (juce::Slider::trackColourId, juce::Colour (0xFF222222));
    morphCrossfader.setLookAndFeel (&chromaLookAndFeel); // Attach crossfader style [5]
    addAndMakeVisible (morphCrossfader);

    // Latch toggle
    addAndMakeVisible (latchButton);
    latchButton.setButtonText ("LATCH");
    latchButton.setClickingTogglesState (true);

    // Chords toggle
    addAndMakeVisible (chordModeButton);
    chordModeButton.setButtonText ("CHORDS");
    chordModeButton.setClickingTogglesState (true);

    // DICE Buttons with LookAndFeel Vector Drawings [NEW]
    addAndMakeVisible (diceMelodyButton);
    diceMelodyButton.setComponentID ("dice_melody");
    diceMelodyButton.setButtonText ("MELODY");
    diceMelodyButton.setLookAndFeel (&chromaLookAndFeel);
    diceMelodyButton.onClick = [this] { processor.diceMelody(); };

    addAndMakeVisible (diceRhythmButton);
    diceRhythmButton.setComponentID ("dice_rhythm");
    diceRhythmButton.setButtonText ("RHYTHM");
    diceRhythmButton.setLookAndFeel (&chromaLookAndFeel);
    diceRhythmButton.onClick = [this] { processor.diceRhythm(); };

    // Symmetrical Scene Buttons initialization (Adjusted to 3 slots) [NEW]
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible (sceneAButtons[i]);
        sceneAButtons[i].setButtonText ("A" + juce::String (i + 1));
        sceneAButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF151515));
        sceneAButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF66666A));
        sceneAButtons[i].addMouseListener (this, false); 

        addAndMakeVisible (sceneBButtons[i]);
        sceneBButtons[i].setButtonText ("B" + juce::String (i + 1));
        sceneBButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF151515));
        sceneBButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF66666A));
        sceneBButtons[i].addMouseListener (this, false); 
    }

    addAndMakeVisible (diceSceneAButton);
    diceSceneAButton.setComponentID ("dice_scene_a");
    diceSceneAButton.setButtonText (""); // Vector icon drawn in LookAndFeel
    diceSceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceSceneAButton.setLookAndFeel (&chromaLookAndFeel);
    diceSceneAButton.onClick = [this] { processor.diceActiveSceneA(); }; 

    addAndMakeVisible (diceSceneBButton);
    diceSceneBButton.setComponentID ("dice_scene_b");
    diceSceneBButton.setButtonText (""); // Vector icon drawn in LookAndFeel
    diceSceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
    diceSceneBButton.setLookAndFeel (&chromaLookAndFeel);
    diceSceneBButton.onClick = [this] { processor.diceActiveSceneB(); }; // Direct B-routing [NEW]

    // Save & Recall utility toggles (Left-sidebar bottom placement) [5]
    addAndMakeVisible (saveButton);
    saveButton.setButtonText ("SAVE");
    saveButton.setClickingTogglesState (true);
    saveButton.addMouseListener (this, false); // Listen for master long-press
    saveButton.onClick = [this] {
        if (saveButton.getToggleState()) {
            recallButton.setToggleState (false, juce::dontSendNotification);
        }
    };

    addAndMakeVisible (recallButton);
    recallButton.setButtonText ("RECALL");
    recallButton.setClickingTogglesState (true);
    recallButton.setToggleState (false, juce::dontSendNotification); // OFF by default [NEW]
    recallButton.onClick = [this] {
        if (recallButton.getToggleState()) {
            saveButton.setToggleState (false, juce::dontSendNotification);
        }
    };

    // 8 Preset Slots with Custom Mouse Listener bindings (No instant onClick actions) [NEW]
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF050505));
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF444444));
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

    // Unregister the Parameter Listener [3]
    processor.apvts.removeParameterListener ("panelTheme", this);
    
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
    morphCrossfader.setLookAndFeel (nullptr);

    diceMelodyButton.setLookAndFeel (nullptr);
    diceRhythmButton.setLookAndFeel (nullptr);
    diceSceneAButton.setLookAndFeel (nullptr);
    diceSceneBButton.setLookAndFeel (nullptr);

    diceMelodyButton.onClick = nullptr;
    diceRhythmButton.onClick = nullptr;
    diceSceneAButton.onClick = nullptr;
    diceSceneBButton.onClick = nullptr;

    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].onClick = nullptr;
        presetButtons[i].onStateChange = nullptr;
        presetButtons[i].removeMouseListener(this); 
    }

    for (int i = 0; i < 3; ++i)
    {
        sceneAButtons[i].removeMouseListener (this);
        sceneBButtons[i].removeMouseListener (this);
    }
}

// Thread-safe async Parameter Listener callback to trigger immediate UI redraw [3]
void PluginEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);
    
    if (parameterID == "panelTheme")
    {
        // DAW automation can trigger parameter updates on the audio thread.
        // We must push the repaint commands safely onto the Main GUI Message Thread. [3]
        juce::MessageManager::callAsync ([this]() {
            repaint();
            oledDisplay.repaint();
            
            fader1.repaint(); fader2.repaint(); fader3.repaint(); fader4.repaint();
            fader5.repaint(); fader6.repaint(); fader7.repaint(); fader8.repaint();
            
            rhythmMorphKnob.repaint(); restKnob.repaint(); legatoKnob.repaint(); rateKnob.repaint();
            entropyKnob.repaint(); harmonyKnob.repaint(); chaosKnob.repaint(); octavesKnob.repaint();
            morphCrossfader.repaint();

            for (int i = 0; i < 3; ++i) {
                sceneAButtons[i].repaint();
                sceneBButtons[i].repaint();
            }
            saveButton.repaint();
            recallButton.repaint();
        });
    }
}

void PluginEditor::mouseDown (const juce::MouseEvent& event)
{
    // 1. Preset Slot Interactions [NEW]
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
                
                // Auto-toggle RECALL to OFF immediately after load
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

    // 2. Master SAVE Button long-press arming [NEW]
    if (event.eventComponent == &saveButton)
    {
        savePressStartTime = juce::Time::getMillisecondCounter();
        saveAlreadySaved = false;
    }

    // 3. Right-Click LFO Modulation Menu Popup [NEW]
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
                depthMenu.addItem (24, "Full (100%)", true, (currentDepth > 0.90f));
                menu.addSubMenu ("LFO Depth", depthMenu);

                menu.showMenuAsync (juce::PopupMenu::Options(), [rateParam, depthParam](int result) {
                    if (result == 1) {
                        rateParam->setValueNotifyingHost (0.0f); // Off
                    }
                    else if (result >= 10 && result <= 13) {
                        float val = static_cast<float> (result - 9) / 4.0f; // Map menu index back onto choices
                        rateParam->setValueNotifyingHost (val);
                    }
                    else if (result >= 20 && result <= 24) {
                        float depthVal = 0.0f;
                        if (result == 21) depthVal = 0.1f;
                        else if (result == 22) depthVal = 0.25f;
                        else if (result == 23) depthVal = 0.5f;
                        else if (result == 24) depthVal = 1.0f;
                        depthParam->setValueNotifyingHost (depthVal);
                    }
                });
            }
        }
    }

    // 4. Right-Click Main Panel Background Context Menu Theme Switcher [NEW]
    if (event.mods.isRightButtonDown() && event.eventComponent == this)
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("SELECT PANEL THEME");
        
        int currentTheme = static_cast<int> (*processor.apvts.getRawParameterValue ("panelTheme"));
        menu.addItem (100, "Navy Cyber (Dark Default)", true, (currentTheme == 0));
        menu.addItem (101, "Skyline (Beige Eurorack)",   true, (currentTheme == 1));
        menu.addItem (102, "Monochrome (Minimal Black)", true, (currentTheme == 2));
        menu.addItem (103, "Matrix Terminal (Neon Green)", true, (currentTheme == 3));
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this](int result) {
            if (result >= 100 && result <= 103) {
                // Instantly update the state saved parameter inside the APVTS
                processor.apvts.getParameter ("panelTheme")->setValueNotifyingHost (static_cast<float> (result - 100) / 3.0f);
            }
        });
    }

    // 5. Capture press start times for real-time hold-to-save scene logic
    for (int i = 0; i < 3; ++i)
    {
        if (event.eventComponent == &sceneAButtons[i])
        {
            sceneAPressStartTime[i] = juce::Time::getMillisecondCounter();
            sceneAAlreadySaved[i] = false;
        }
        else if (event.eventComponent == &sceneBButtons[i])
        {
            sceneBPressStartTime[i] = juce::Time::getMillisecondCounter();
            sceneBAlreadySaved[i] = false;
        }
    }
}

void PluginEditor::mouseUp (const juce::MouseEvent& event)
{
    // 1. Preset Slot Reset [NEW]
    for (int i = 0; i < 8; ++i)
    {
        if (event.eventComponent == &presetButtons[i])
        {
            presetPressStartTime[i] = 0;
            presetAlreadySaved[i] = false;
        }
    }

    // 2. Master SAVE Button Reset [NEW]
    if (event.eventComponent == &saveButton)
    {
        savePressStartTime = 0;
        saveAlreadySaved = false;
    }

    // 3. Real-Time Release Latching Logic for Scenes
    for (int i = 0; i < 3; ++i)
    {
        if (event.eventComponent == &sceneAButtons[i])
        {
            bool wasSaved = sceneAAlreadySaved[i];
            sceneAPressStartTime[i] = 0; 
            sceneAAlreadySaved[i] = false; 
            
            if (! wasSaved) 
            {
                if (processor.activeSceneAIndex.load() == i && processor.isSceneASaved(i))
                {
                    processor.loadSceneA (i);
                }
                else
                {
                    processor.activeSceneAIndex.store (i);
                    processor.loadSceneA (i);
                }
            }
        }
        else if (event.eventComponent == &sceneBButtons[i])
        {
            bool wasSaved = sceneBAlreadySaved[i];
            sceneBPressStartTime[i] = 0; 
            sceneBAlreadySaved[i] = false; 
            
            if (! wasSaved) 
            {
                if (processor.activeSceneBIndex.load() == i && processor.isSceneBSaved(i))
                {
                    processor.loadSceneB (i);
                }
                else
                {
                    processor.activeSceneBIndex.store (i);
                    processor.loadSceneB (i);
                }
            }
        }
    }
}

void PluginEditor::timerCallback()
{
    float morphValue = morphCrossfader.getValue();
    int currentActiveA = processor.activeSceneAIndex.load(); 
    int currentActiveB = processor.activeSceneBIndex.load(); 

    // Motorized Parameter Gliding: pull morphs from dynamic 3-slot presets [NEW]
    if (processor.isSceneASaved (currentActiveA) && processor.isSceneBSaved (currentActiveB) && morphCrossfader.isMouseButtonDown())
    {
        for (int i = 0; i < 8; ++i)
        {
            float targetValue = (processor.sceneAPresets[currentActiveA].faders[i] * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].faders[i] * morphValue);
            processor.apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (targetValue);
        }

        processor.apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost ((processor.sceneAPresets[currentActiveA].rhythmMorph * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].rhythmMorph * morphValue));
        processor.apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost ((processor.sceneAPresets[currentActiveA].rest * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].rest * morphValue));
        processor.apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost ((processor.sceneAPresets[currentActiveA].legato * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].legato * morphValue));
        
        processor.apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (((processor.sceneAPresets[currentActiveA].entropy + 1.0f) * 0.5f * (1.0f - morphValue)) + ((processor.sceneBPresets[currentActiveB].entropy + 1.0f) * 0.5f * morphValue));
        processor.apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost ((processor.sceneAPresets[currentActiveA].harmony * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].harmony * morphValue));
        processor.apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost ((processor.sceneAPresets[currentActiveA].chaos * (1.0f - morphValue)) + (processor.sceneBPresets[currentActiveB].chaos * morphValue));
        
        processor.apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (((processor.sceneAPresets[currentActiveA].rate / 3.0f) * (1.0f - morphValue)) + ((processor.sceneBPresets[currentActiveB].rate / 3.0f) * morphValue));
        processor.apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (((processor.sceneAPresets[currentActiveA].octaves + 2.0f) / 5.0f * (1.0f - morphValue)) + ((processor.sceneBPresets[currentActiveB].octaves + 2.0f) / 5.0f * morphValue));
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

    // Update dynamically themed colors in real-time across auxiliary labels and buttons
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    // Apply color-coded theme updates on labels
    rhythmMorphTitle.setColour (juce::Label::textColourId, t.textDim);
    restTitle.setColour (juce::Label::textColourId, t.textDim);
    legatoTitle.setColour (juce::Label::textColourId, t.textDim);
    rateTitle.setColour (juce::Label::textColourId, t.textDim);
    entropyTitle.setColour (juce::Label::textColourId, t.textDim);
    harmonyTitle.setColour (juce::Label::textColourId, t.textDim);
    chaosTitle.setColour (juce::Label::textColourId, t.textDim);
    octavesTitle.setColour (juce::Label::textColourId, t.textDim);

    for (int i = 0; i < 8; ++i)
        faderLabels[i]->setColour (juce::Label::textColourId, t.textDim);

    // Dynamic Knob value textbox contrast calibration
    juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    for (auto* knob : knobs)
    {
        knob->setColour (juce::Slider::textBoxTextColourId, (themeIdx == 1) ? juce::Colour (0xFF1A1A18) : juce::Colour (0xFFE0E0E0));
        knob->setColour (juce::Slider::textBoxBackgroundColourId, (themeIdx == 1) ? juce::Colour (0x20000000) : juce::Colour (0x20FFFFFF));
    }

    // Dynamic ComboBox Contrast Calibration
    rootKeyBox.setColour (juce::ComboBox::backgroundColourId, themeIdx == 1 ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF111111));
    rootKeyBox.setColour (juce::ComboBox::outlineColourId, themeIdx == 1 ? juce::Colour (0xFFB8B5AB) : juce::Colour (0xFF222222));
    rootKeyBox.setColour (juce::ComboBox::textColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.leftAccent);
    rootKeyBox.setColour (juce::ComboBox::arrowColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.leftAccent);

    scaleTypeBox.setColour (juce::ComboBox::backgroundColourId, themeIdx == 1 ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF111111));
    scaleTypeBox.setColour (juce::ComboBox::outlineColourId, themeIdx == 1 ? juce::Colour (0xFFB8B5AB) : juce::Colour (0xFF222222));
    scaleTypeBox.setColour (juce::ComboBox::textColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.rightAccent);
    scaleTypeBox.setColour (juce::ComboBox::arrowColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.rightAccent);

    cycleLengthBox.setColour (juce::ComboBox::backgroundColourId, themeIdx == 1 ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF111111));
    cycleLengthBox.setColour (juce::ComboBox::outlineColourId, themeIdx == 1 ? juce::Colour (0xFFB8B5AB) : juce::Colour (0xFF222222));
    cycleLengthBox.setColour (juce::ComboBox::textColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.rightAccent);
    cycleLengthBox.setColour (juce::ComboBox::arrowColourId, themeIdx == 1 ? juce::Colour (0xFF1A1A18) : t.rightAccent);

    // Dynamic LED and background colors for main buttons
    if (themeIdx == 1) // Skyline Eurorack
    {
        latchButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFD4D1C9));
        latchButton.setColour (juce::TextButton::buttonOnColourId, t.leftAccent);
        latchButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        latchButton.setColour (juce::TextButton::textColourOffId, t.leftAccent);

        chordModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFD4D1C9));
        chordModeButton.setColour (juce::TextButton::buttonOnColourId, t.rightAccent);
        chordModeButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        chordModeButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);

        diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFD4D1C9));
        diceMelodyButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);
        
        diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFD4D1C9));
        diceRhythmButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);
    }
    else // Dark Themes
    {
        latchButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF112233));
        latchButton.setColour (juce::TextButton::buttonOnColourId, t.leftAccent);
        latchButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));
        latchButton.setColour (juce::TextButton::textColourOffId, t.leftAccent);

        chordModeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF141416));
        chordModeButton.setColour (juce::TextButton::buttonOnColourId, t.rightAccent);
        chordModeButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));
        chordModeButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);

        diceMelodyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
        diceMelodyButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);

        diceRhythmButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF221100));
        diceRhythmButton.setColour (juce::TextButton::textColourOffId, t.rightAccent);
    }

    // Dynamic Save/Recall Button Contrast Calibration
    juce::Colour unselectedBtnCol = (themeIdx == 1) ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF151518);
    juce::Colour saveActiveCol = t.rightAccent; 
    juce::Colour recallActiveCol = t.leftAccent; 

    saveButton.setColour (juce::TextButton::buttonOnColourId, saveActiveCol);
    saveButton.setColour (juce::TextButton::textColourOnId, (themeIdx == 1) ? juce::Colours::white : juce::Colours::black);
    saveButton.setColour (juce::TextButton::textColourOffId, saveActiveCol);

    recallButton.setColour (juce::TextButton::buttonOnColourId, recallActiveCol);
    recallButton.setColour (juce::TextButton::textColourOnId, (themeIdx == 1) ? juce::Colours::white : juce::Colours::black);
    recallButton.setColour (juce::TextButton::textColourOffId, recallActiveCol);

    // Master Save Button Long-Press Detection & Flash Sequence [NEW]
    if (savePressStartTime > 0 && ! saveAlreadySaved)
    {
        auto elapsed = juce::Time::getMillisecondCounter() - savePressStartTime;
        if (elapsed >= 1000)
        {
            int activePreset = processor.activePresetIndex.load();
            if (activePreset >= 0 && activePreset < 8)
            {
                processor.savePreset (activePreset);
            }
            saveFlashTimer = 24; // 800ms triple flash
            saveAlreadySaved = true;
            saveButton.setToggleState (false, juce::dontSendNotification);
        }
    }

    if (saveFlashTimer > 0)
    {
        saveFlashTimer--;
        int pulseIndex = saveFlashTimer / 4;
        bool flashOn = (pulseIndex % 2 == 1);
        
        if (flashOn)
        {
            saveButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFAA00));
            saveButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
            saveButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));
        }
        else
        {
            saveButton.setColour (juce::TextButton::buttonColourId, unselectedBtnCol);
            saveButton.setColour (juce::TextButton::textColourOffId, saveActiveCol);
        }
    }
    else
    {
        if (saveButton.getToggleState())
            saveButton.setColour (juce::TextButton::buttonColourId, saveActiveCol);
        else
            saveButton.setColour (juce::TextButton::buttonColourId, unselectedBtnCol);
    }

    if (recallButton.getToggleState())
        recallButton.setColour (juce::TextButton::buttonColourId, recallActiveCol);
    else
        recallButton.setColour (juce::TextButton::buttonColourId, unselectedBtnCol);

    // Preset Slot Hold-to-Save polling loops [NEW]
    for (int i = 0; i < 8; ++i)
    {
        if (presetPressStartTime[i] > 0 && ! presetAlreadySaved[i])
        {
            auto elapsed = juce::Time::getMillisecondCounter() - presetPressStartTime[i];
            if (elapsed >= 1000)
            {
                processor.savePreset (i);
                presetFlashTimer[i] = 24; 
                presetFlashType[i] = 1; // Amber
                presetAlreadySaved[i] = true;
                
                // Auto-toggle SAVE to OFF
                saveButton.setToggleState (false, juce::dontSendNotification);
            }
        }
    }

    // Symmetrical Real-Time Save-while-held Scene Detection (Adjusted to 3 slots)
    for (int i = 0; i < 3; ++i)
    {
        if (sceneAPressStartTime[i] > 0 && ! sceneAAlreadySaved[i])
        {
            auto elapsed = juce::Time::getMillisecondCounter() - sceneAPressStartTime[i];
            if (elapsed >= 1000)
            {
                processor.saveSceneA (i);
                sceneAFlashTimer[i] = 24; // Start 24-frame triple flash (800ms)
                sceneAAlreadySaved[i] = true;
                saveButton.setToggleState (false, juce::dontSendNotification);
                recallButton.setToggleState (true, juce::dontSendNotification);
            }
        }
        if (sceneBPressStartTime[i] > 0 && ! sceneBAlreadySaved[i])
        {
            auto elapsed = juce::Time::getMillisecondCounter() - sceneBPressStartTime[i];
            if (elapsed >= 1000)
            {
                processor.saveSceneB (i);
                sceneBFlashTimer[i] = 24; // Start 24-frame triple flash (800ms)
                sceneBAlreadySaved[i] = true;
                saveButton.setToggleState (false, juce::dontSendNotification);
                recallButton.setToggleState (true, juce::dontSendNotification);
            }
        }
    }

    // Dynamic Solid Contrast text color calibration [NEW]
    juce::Colour textCol = (themeIdx == 1) ? juce::Colour (0xFF1A1A18) : juce::Colours::white;

    // Symmetrical Scene LEDs with Real-Time Crossfade Power Shifting (3 slots)
    for (int i = 0; i < 3; ++i)
    {
        // Scene A Colors (Pastel Green Saved, Dynamic LED brightness on Active)
        if (sceneAFlashTimer[i] > 0) // Amber Write Flash
        {
            sceneAFlashTimer[i]--;
            int pulseIndex = sceneAFlashTimer[i] / 4;
            bool flashOn = (pulseIndex % 2 == 1);
            
            if (flashOn)
            {
                sceneAButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFAA00));
                sceneAButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
            }
            else
            {
                sceneAButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF101012));
                sceneAButtons[i].setColour (juce::TextButton::textColourOffId, t.unlitLed);
            }
        }
        else if (currentActiveA == i) // Active (Intensity proportional to fader position)
        {
            float intensity = 1.0f - morphValue;
            float displayVal = juce::jlimit (0.15f, 1.0f, intensity); // Maintain base glow
            juce::Colour greenLED = juce::Colour (0xFF00FF66);
            
            sceneAButtons[i].setColour (juce::TextButton::buttonColourId, greenLED.withAlpha (displayVal * 0.8f));
            sceneAButtons[i].setColour (juce::TextButton::textColourOffId, textCol.withAlpha (themeIdx == 1 ? 1.0f : displayVal)); // Solid text in light theme
        }
        else if (processor.isSceneASaved (i)) // Saved Inactive (Pastel Green)
        {
            sceneAButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A2F25));
            sceneAButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF4CFF99));
        }
        else // Empty Unsaved
        {
            sceneAButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF101012));
            sceneAButtons[i].setColour (juce::TextButton::textColourOffId, t.unlitLed);
        }

        // Scene B Colors (Pastel Blue Saved, Dynamic LED brightness on Active)
        if (sceneBFlashTimer[i] > 0) // Amber Write Flash
        {
            sceneBFlashTimer[i]--;
            int pulseIndex = sceneBFlashTimer[i] / 4;
            bool flashOn = (pulseIndex % 2 == 1);
            
            if (flashOn)
            {
                sceneBButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFAA00));
                sceneBButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
            }
            else
            {
                sceneBButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF101012));
                sceneBButtons[i].setColour (juce::TextButton::textColourOffId, t.unlitLed);
            }
        }
        else if (currentActiveB == i) // Active (Intensity proportional to fader position)
        {
            float intensity = morphValue;
            float displayVal = juce::jlimit (0.15f, 1.0f, intensity); // Maintain base glow
            juce::Colour blueLED = juce::Colour (0xFF00D2FF);
            
            sceneBButtons[i].setColour (juce::TextButton::buttonColourId, blueLED.withAlpha (displayVal * 0.8f));
            sceneBButtons[i].setColour (juce::TextButton::textColourOffId, textCol.withAlpha (themeIdx == 1 ? 1.0f : displayVal)); // Solid text in light theme
        }
        else if (processor.isSceneBSaved (i)) // Saved Inactive (Pastel Blue)
        {
            sceneBButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF1A2B3D));
            sceneBButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF4CCFFF));
        }
        else // Empty Unsaved
        {
            sceneBButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF101012));
            sceneBButtons[i].setColour (juce::TextButton::textColourOffId, t.unlitLed);
        }
    }

    // Dynamic preset saved status and flash glow [NEW]
    for (int i = 0; i < 8; ++i)
    {
        if (presetFlashTimer[i] > 0)
        {
            presetFlashTimer[i]--;
            int pulseIndex = presetFlashTimer[i] / 4;
            bool flashOn = (pulseIndex % 2 == 1);
            
            if (flashOn)
            {
                juce::Colour flashCol = (presetFlashType[i] == 1) ? juce::Colour (0xFFFFAA00) : juce::Colour (0xFF00D2FF);
                presetButtons[i].setColour (juce::TextButton::buttonColourId, flashCol);
                presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
            }
            else
            {
                presetButtons[i].setColour (juce::TextButton::buttonColourId, themeIdx == 1 ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF141416));
                presetButtons[i].setColour (juce::TextButton::textColourOffId, t.textDim);
            }
        }
        else if (processor.isPresetSaved (i))
        {
            presetButtons[i].setColour (juce::TextButton::buttonColourId, t.leftAccent.withAlpha(0.22f));
            presetButtons[i].setColour (juce::TextButton::textColourOffId, t.leftAccent);
        }
        else
        {
            presetButtons[i].setColour (juce::TextButton::buttonColourId, themeIdx == 1 ? juce::Colour (0xFFD4D1C9) : juce::Colour (0xFF141416));
            presetButtons[i].setColour (juce::TextButton::textColourOffId, t.textDim);
        }
    }

    // Force-repaint the 8 rotary knobs at 30Hz so the LFO visual "breathing" is smooth [CRITICAL FIX]
    rhythmMorphKnob.repaint();
    restKnob.repaint();
    legatoKnob.repaint();
    rateKnob.repaint();
    entropyKnob.repaint();
    harmonyKnob.repaint();
    chaosKnob.repaint();
    octavesKnob.repaint();
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Retrieve dynamic panel theme background
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    auto t = AppTheme::get (themeIdx);

    g.fillAll (t.background);
    g.setColour (t.border);
    g.drawRect (getLocalBounds().toFloat(), 3.0f);
    g.drawHorizontalLine (getHeight() - static_cast<int>(getHeight() * 0.22f), 15.0f, getWidth() - 15.0f);

    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle (juce::Font::bold))); // Corrected FontOptions syntax [NEW]
    g.setColour (t.textDim.withAlpha (0.7f));
    g.drawText ("RHYTHM", 15, 12, 100, 20, juce::Justification::left);
    g.drawText ("GENERATOR", getWidth() - 115, 12, 100, 20, juce::Justification::right);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (15);
    int totalWidth = getWidth();
    int totalHeight = getHeight();

    // 0. Carve out a 25px top margin so panel headers don't overlap with the top knob row [CRITICAL FIX]
    area.removeFromTop (25);

    // 1. Bottom Section: 8 Scale-Degree Faders (Faders vertical scale reduced by 20%)
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

    // 2. Symmetrical Crossfader row with expanded button slots [NEW]
    int morphHeight = static_cast<int>(totalHeight * 0.07f);
    auto morphArea = area.removeFromBottom (juce::jlimit (25, 45, morphHeight));
    
    int buttonWidth = 42; // Expanded from 35px to 42px
    int diceWidth = 36;   // Expanded from 26px to 36px
    
    // Left-Side: Symmetrical descending layout (A3, A2, A1, Dice A)
    for (int i = 2; i >= 0; --i) {
        sceneAButtons[i].setBounds (morphArea.removeFromLeft (buttonWidth).reduced (2, 3));
    }
    diceSceneAButton.setBounds (morphArea.removeFromLeft (diceWidth).reduced (2, 3));

    // Right-Side: Symmetrical descending layout (B3, B2, B1, Dice B from right)
    for (int i = 2; i >= 0; --i) {
        sceneBButtons[i].setBounds (morphArea.removeFromRight (buttonWidth).reduced (2, 3));
    }
    diceSceneBButton.setBounds (morphArea.removeFromRight (diceWidth).reduced (2, 3));

    // Center Crossfader (Symmetrically restricted to remaining space)
    morphCrossfader.setBounds (morphArea.reduced (15, 5));

    area.removeFromBottom (15);

    // 3. Sidebars with vertical spacer configurations [NEW]
    int sidebarWidth = static_cast<int>(totalWidth * 0.15f); 
    auto leftSidebar = area.removeFromLeft (sidebarWidth);
    auto rightSidebar = area.removeFromRight (sidebarWidth);
    
    // Left Sidebar Layout
    auto leftBtnArea = leftSidebar.removeFromBottom (70).reduced (5, 2);
    leftSidebar.removeFromBottom (15); // Dedicated 15px layout gap [NEW]
    
    int leftRowHeight = leftSidebar.getHeight() / 4;
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    
    for (int i = 0; i < 4; ++i)
    {
        auto row = leftSidebar.removeFromTop (leftRowHeight);
        // Labels positioned directly below the knobs [NEW]
        leftKnobs[i]->setBounds (row.removeFromTop (row.getHeight() - 16).reduced (4, 2));
        leftTitles[i]->setBounds (row);
    }
    
    int leftBtnHeight = leftBtnArea.getHeight() / 2;
    saveButton.setBounds (leftBtnArea.removeFromTop (leftBtnHeight).reduced (2));
    recallButton.setBounds (leftBtnArea.reduced (2));

    // Right Sidebar Layout
    auto diceArea = rightSidebar.removeFromBottom (70).reduced (5, 2);
    rightSidebar.removeFromBottom (15); // Dedicated 15px layout gap [NEW]
    
    int rightRowHeight = rightSidebar.getHeight() / 4;
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    
    for (int i = 0; i < 4; ++i)
    {
        auto row = rightSidebar.removeFromTop (rightRowHeight);
        // Labels positioned directly below the knobs [NEW]
        rightKnobs[i]->setBounds (row.removeFromTop (row.getHeight() - 16).reduced (4, 2));
        rightTitles[i]->setBounds (row);
    }
    
    int diceBtnHeight = diceArea.getHeight() / 2;
    diceMelodyButton.setBounds (diceArea.removeFromTop (diceBtnHeight).reduced (2));
    diceRhythmButton.setBounds (diceArea.reduced (2));

    // 4. Center Section: Dropdown Bar, OLED Display, and 8 Preset Buttons
    int presetHeight = static_cast<int>(totalHeight * 0.06f);
    auto presetArea = area.removeFromBottom (juce::jlimit (24, 36, presetHeight));
    auto oledArea = area.reduced (5, 5);
    
    // Proportional Dropdown Bar positioned safely above the graphic display with Latch/Chords buttons
    int dropdownBarHeight = static_cast<int>(totalHeight * 0.07f);
    auto dropdownBarArea = oledArea.removeFromTop (juce::jlimit (28, 40, dropdownBarHeight));
    int totalCenterWidth = dropdownBarArea.getWidth();
    
    int systemButtonWidth = 70;
    int availableWidthForComboBoxes = totalCenterWidth - (systemButtonWidth * 2) - 20; 
    int selectWidth = availableWidthForComboBoxes / 3;

    rootKeyBox.setBounds (dropdownBarArea.removeFromLeft (selectWidth).reduced (2, 2));
    scaleTypeBox.setBounds (dropdownBarArea.removeFromLeft (selectWidth).reduced (2, 2));
    cycleLengthBox.setBounds (dropdownBarArea.removeFromLeft (selectWidth).reduced (2, 2));
    
    dropdownBarArea.removeFromLeft (20); // Spacing gap before system buttons
    latchButton.setBounds (dropdownBarArea.removeFromLeft (systemButtonWidth).reduced (2, 2));
    chordModeButton.setBounds (dropdownBarArea.removeFromLeft (systemButtonWidth).reduced (2, 2));
    
    oledDisplay.setBounds (oledArea.reduced (0, 5));

    int presetWidth = presetArea.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
    {
        presetButtons[i].setBounds (presetArea.removeFromLeft (presetWidth).reduced (4, 3));
    }
}