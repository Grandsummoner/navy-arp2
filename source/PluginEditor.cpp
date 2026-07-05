#define DRAW_DIAGNOSTIC_GRID 0  // Set to 1 to show the overlay and coordinate bubble, 0 to hide it

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "OledDisplay.h"
#include "ChromaCapsLookAndFeel.h"
#include "AppTheme.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p, this)
{
    // Load the compiled background PNG panel asset
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::panel_png, 
                                                       BinaryData::panel_pngSize);

    addAndMakeVisible (oledDisplay);
    processor.apvts.addParameterListener ("panelTheme", this);

    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    juce::Label* faderLabels[] = { &faderLabel1, &faderLabel2, &faderLabel3, &faderLabel4, &faderLabel5, &faderLabel6, &faderLabel7, &faderLabel8 };
    for (int i = 0; i < 8; ++i) {
        faders[i]->setSliderStyle (juce::Slider::LinearVertical); 
        faders[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        faders[i]->setLookAndFeel (&chromaLookAndFeel); 
        faders[i]->setComponentID ("fader" + juce::String (i + 1)); 
        addAndMakeVisible (faders[i]);
        faderLabels[i]->setText ("", juce::dontSendNotification); 
    }

    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob };
    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::String leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };
    for (int i = 0; i < 4; ++i) {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); 
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
        leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        leftKnobs[i]->setComponentID (leftPrefixes[i]); 
        leftKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (leftKnobs[i]);
        leftTitles[i]->setText ("", juce::dontSendNotification); 
    }

    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    juce::String rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };
    for (int i = 0; i < 4; ++i) {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); 
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
        rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        rightKnobs[i]->setComponentID (rightPrefixes[i]); 
        rightKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (rightKnobs[i]);
        rightTitles[i]->setText ("", juce::dontSendNotification); 
    }

    // Initialize Left Master Knob (mast)
    masterVelocityKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterVelocityKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterVelocityKnob.setLookAndFeel (&chromaLookAndFeel);
    masterVelocityKnob.setComponentID ("masterVelocity");
    masterVelocityKnob.addMouseListener (this, false);
    addAndMakeVisible (masterVelocityKnob);
    masterVelocityTitle.setText ("", juce::dontSendNotification);

    // Initialize Right Master Knob (mlart)
    masterSwingKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterSwingKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterSwingKnob.setLookAndFeel (&chromaLookAndFeel);
    masterSwingKnob.setComponentID ("masterSwing");
    masterSwingKnob.addMouseListener (this, false);
    addAndMakeVisible (masterSwingKnob);
    masterSwingTitle.setText ("", juce::dontSendNotification);

    morphCrossfader.setSliderStyle (juce::Slider::LinearHorizontal); 
    morphCrossfader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    morphCrossfader.setComponentID ("morph"); 
    morphCrossfader.setLookAndFeel (&chromaLookAndFeel); 
    addAndMakeVisible (morphCrossfader);

    juce::TextButton* deckBtns[] = { &latchButton, &arpSeqButton, &polyButton, &freezeButton }; 
    juce::String deckTxt[] = { "Latch", "SEQ", "Poly", "Freeze" };
    for (int i = 0; i < 4; ++i) { 
        addAndMakeVisible (deckBtns[i]); 
        deckBtns[i]->setButtonText (deckTxt[i]); 
        deckBtns[i]->setClickingTogglesState (true); 
        deckBtns[i]->setLookAndFeel (&chromaLookAndFeel); 
    }

    juce::TextButton* sceneBtns[] = { &sceneAButton, &sceneBButton }; 
    juce::String sceneTxt[] = { "A", "B" };
    for (int i = 0; i < 2; ++i) { 
        addAndMakeVisible (sceneBtns[i]); 
        sceneBtns[i]->setButtonText (sceneTxt[i]); 
        sceneBtns[i]->addMouseListener (this, false); 
        sceneBtns[i]->setLookAndFeel (&chromaLookAndFeel); 
    }

    juce::TextButton* utilBtns[] = { &saveButton, &recallButton, &copyButton, &initButton }; 
    juce::String utilTxt[] = { "Save", "Recall", "Copy", "Init" };
    for (int i = 0; i < 4; ++i) { 
        utilBtns[i]->setLookAndFeel (&chromaLookAndFeel); 
        addAndMakeVisible (utilBtns[i]); 
        utilBtns[i]->setButtonText (utilTxt[i]); 
        utilBtns[i]->setClickingTogglesState (true); 
        utilBtns[i]->addMouseListener (this, false); 
    }
    saveButton.onClick   = [this] { if (saveButton.getToggleState()) recallButton.setToggleState (false, juce::dontSendNotification); };
    recallButton.onClick = [this] { if (recallButton.getToggleState()) saveButton.setToggleState (false, juce::dontSendNotification); };

    addAndMakeVisible (diceMeloButton); 
    diceMeloButton.setComponentID ("dice_melody"); 
    diceMeloButton.setButtonText ("Melo"); 
    diceMeloButton.setLookAndFeel (&chromaLookAndFeel); 
    diceMeloButton.onClick = [this] { if (initButton.getToggleState()) { processor.resetRhythm(); initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceMelody(); } };
    
    addAndMakeVisible (diceArtiButton); 
    diceArtiButton.setComponentID ("dice_articulation"); 
    diceArtiButton.setButtonText ("Arti"); 
    diceArtiButton.setLookAndFeel (&chromaLookAndFeel); 
    diceArtiButton.onClick = [this] { if (initButton.getToggleState()) { processor.apvts.getParameter(IDs::rest.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::legato.getParamID())->setValueNotifyingHost(0.5f); initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceArticulation(); } };
    
    addAndMakeVisible (diceTimeButton); 
    diceTimeButton.setComponentID ("dice_time"); 
    diceTimeButton.setButtonText ("Time"); 
    diceTimeButton.setLookAndFeel (&chromaLookAndFeel); 
    diceTimeButton.onClick = [this] { 
        if (initButton.getToggleState()) { 
            processor.apvts.getParameter (IDs::rate.getParamID())->setValueNotifyingHost (2.0f / 3.0f); 
            processor.apvts.getParameter (IDs::octaves.getParamID())->setValueNotifyingHost (3.0f / 6.0f); 
            processor.apvts.getParameter (IDs::cycleLength.getParamID())->setValueNotifyingHost (2.0f / 3.0f); 
            initFlashTimer = 24; 
            initButton.setToggleState (false, juce::dontSendNotification); 
            initButton.repaint(); 
        } else { 
            processor.diceTime(); 
        } 
    };
    
    addAndMakeVisible (diceNavyButton); 
    diceNavyButton.setComponentID ("dice_navy"); 
    diceNavyButton.setButtonText ("Navy"); 
    diceNavyButton.setLookAndFeel (&chromaLookAndFeel); 
    diceNavyButton.onClick = [this] { if (initButton.getToggleState()) { processor.apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::entropy.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::harmony.getParamID())->setValueNotifyingHost(0.0f); processor.apvts.getParameter(IDs::chaos.getParamID())->setValueNotifyingHost(0.0f); initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); } else { processor.diceNavy(); } };

    sceneAButton.onClick = [this] {
        if (initButton.getToggleState()) { processor.clearSceneA(); sceneAFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); }
        else if (copyButton.getToggleState()) { 
            processor.sceneA = processor.sceneB;
            processor.hasSceneA = processor.hasSceneB;
            sceneAFlashTimer = 24; 
            sceneBFlashTimer = 24;
            copyButton.setToggleState (false, juce::dontSendNotification); 
            copyButton.repaint(); 
        }
    };
    sceneBButton.onClick = [this] {
        if (initButton.getToggleState()) { processor.clearSceneB(); sceneBFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint(); }
        else if (copyButton.getToggleState()) { 
            processor.sceneB = processor.sceneA;
            processor.hasSceneB = processor.hasSceneA;
            sceneAFlashTimer = 24; 
            sceneBFlashTimer = 24;
            copyButton.setToggleState (false, juce::dontSendNotification); 
            copyButton.repaint(); 
        }
    };

    for (int i = 0; i < 8; ++i) { 
        addAndMakeVisible (presetButtons[i]); 
        presetButtons[i].setButtonText (juce::String (i + 1)); 
        presetButtons[i].addMouseListener (this, false); 
        presetButtons[i].setLookAndFeel (&chromaLookAndFeel); 
    }

    addAndMakeVisible (rootKeyBox); 
    addAndMakeVisible (scaleTypeBox); 
    addAndMakeVisible (cycleLengthBox);
    rootKeyBox.addItemList (juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 1);
    scaleTypeBox.addItemList (juce::StringArray { "Major", "Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1);
    cycleLengthBox.addItemList (juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 1);

    addAndMakeVisible (panelThemeBox);
    panelThemeBox.addItemList (juce::StringArray { "Navy Cyber", "Skyline", "Monochrome", "Matrix" }, 1);
    panelThemeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::panelTheme.getParamID(), panelThemeBox);

    fader1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader1.getParamID(), fader1);
    fader2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader2.getParamID(), fader2);
    fader3Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader3.getParamID(), fader3);
    fader4Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader4.getParamID(), fader4);
    fader5Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader5.getParamID(), fader5);
    fader6Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader6.getParamID(), fader6);
    fader7Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader7.getParamID(), fader7);
    fader8Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::fader8.getParamID(), fader8);

    rhythmMorphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rhythmMorph.getParamID(), rhythmMorphKnob);
    restAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rest.getParamID(), restKnob);
    legatoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::legato.getParamID(), legatoKnob);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::rate.getParamID(), rateKnob);

    entropyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::entropy.getParamID(), entropyKnob);
    harmonyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::harmony.getParamID(), harmonyKnob);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::chaos.getParamID(), chaosKnob);
    octavesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::octaves.getParamID(), octavesKnob);

    masterVelocityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::masterVelocity.getParamID(), masterVelocityKnob);
    masterSwingAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::masterSwing.getParamID(), masterSwingKnob);

    morphAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::morph.getParamID(), morphCrossfader);
    latchAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::latch.getParamID(), latchButton);
    arpSeqAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::arpSeq.getParamID(), arpSeqButton);
    polyButtonAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::poly.getParamID(), polyButton);
    freezeButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::freeze.getParamID(), freezeButton);

    rootKeyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::rootKey.getParamID(), rootKeyBox);
    scaleTypeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::scaleType.getParamID(), scaleTypeBox);
    cycleLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::cycleLength.getParamID(), cycleLengthBox);

    updateSliderTextBoxThemeColors();

    setResizable (false, false); 
    setSize (1521, 1034); 

    if (DRAW_DIAGNOSTIC_GRID)
        setMouseClickGrabsKeyboardFocus (true);

    startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); processor.apvts.removeParameterListener ("panelTheme", this);
    juce::Slider* sliders[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &masterVelocityKnob, &masterSwingKnob, &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8, &morphCrossfader };
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
        juce::Component::SafePointer<PluginEditor> safeThis (this);
        juce::MessageManager::callAsync ([safeThis]() {
            if (safeThis != nullptr) {
                safeThis->updateSliderTextBoxThemeColors();
                safeThis->repaint(); safeThis->oledDisplay.repaint();
                safeThis->fader1.repaint(); safeThis->fader2.repaint(); safeThis->fader3.repaint(); safeThis->fader4.repaint(); safeThis->fader5.repaint(); safeThis->fader6.repaint(); safeThis->fader7.repaint(); safeThis->fader8.repaint();
                safeThis->rhythmMorphKnob.repaint(); safeThis->restKnob.repaint(); safeThis->legatoKnob.repaint(); safeThis->rateKnob.repaint();
                safeThis->entropyKnob.repaint(); safeThis->harmonyKnob.repaint(); safeThis->chaosKnob.repaint(); safeThis->octavesKnob.repaint();
                safeThis->masterVelocityKnob.repaint(); safeThis->masterSwingKnob.repaint();
                safeThis->morphCrossfader.repaint(); safeThis->sceneAButton.repaint(); safeThis->sceneBButton.repaint(); safeThis->saveButton.repaint(); safeThis->recallButton.repaint();
            }
        });
    }
}

void PluginEditor::mouseDown (const juce::MouseEvent& event)
{
    juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::ParameterID rates[] = { IDs::rhythmMorphLfoRate, IDs::restLfoRate, IDs::legatoLfoRate, IDs::rateLfoRate, IDs::entropyLfoRate, IDs::harmonyLfoRate, IDs::chaosLfoRate, IDs::octavesLfoRate };
    juce::ParameterID depths[] = { IDs::rhythmMorphLfoDepth, IDs::restLfoDepth, IDs::legatoLfoDepth, IDs::rateLfoDepth, IDs::entropyLfoDepth, IDs::harmonyLfoDepth, IDs::chaosLfoDepth, IDs::octavesLfoDepth };

    for (int i = 0; i < 8; ++i)
    {
        if (event.eventComponent == knobs[i] && event.mods.isRightButtonDown())
        {
            auto* rateParam = processor.apvts.getParameter (rates[i].getParamID());
            auto* depthParam = processor.apvts.getParameter (depths[i].getParamID());
            
            if (rateParam != nullptr && depthParam != nullptr)
            {
                juce::PopupMenu menu;
                menu.addSectionHeader (juce::String(knobs[i]->getComponentID()).toUpperCase() + " LFO MODULATION");
                
                juce::PopupMenu speedMenu;
                int currentRateIdx = static_cast<int> (std::round (rateParam->getValue() * 4.0f)); 
                speedMenu.addItem (1, "Off", true, currentRateIdx == 0);
                speedMenu.addItem (2, "1/4 Beat", true, currentRateIdx == 1);
                speedMenu.addItem (3, "1/8 Beat", true, currentRateIdx == 2);
                speedMenu.addItem (4, "1/16 Beat", true, currentRateIdx == 3);
                speedMenu.addItem (5, "1/32 Beat", true, currentRateIdx == 4);
                menu.addSubMenu ("Speed", speedMenu);
                
                juce::PopupMenu depthMenu;
                float currentDepth = depthParam->getValue(); 
                depthMenu.addItem (10, "0% (Off)", true, currentDepth == 0.0f);
                depthMenu.addItem (11, "10%", true, std::abs(currentDepth - 0.1f) < 0.05f);
                depthMenu.addItem (12, "25%", true, std::abs(currentDepth - 0.25f) < 0.05f);
                depthMenu.addItem (13, "50%", true, std::abs(currentDepth - 0.5f) < 0.05f);
                depthMenu.addItem (14, "75%", true, std::abs(currentDepth - 0.75f) < 0.05f);
                depthMenu.addItem (15, "100%", true, std::abs(currentDepth - 1.0f) < 0.05f);
                menu.addSubMenu ("Depth", depthMenu);
                
                menu.showMenuAsync (juce::PopupMenu::Options(), [rateParam, depthParam](int result) {
                    if (result >= 1 && result <= 5)
                    {
                        rateParam->setValueNotifyingHost (static_cast<float>(result - 1) / 4.0f);
                    }
                    else if (result >= 10 && result <= 15)
                    {
                        float depthsList[] = { 0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f };
                        depthParam->setValueNotifyingHost (depthsList[result - 10]);
                    }
                });
                return; 
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        if (event.eventComponent == &presetButtons[i]) {
            if (initButton.getToggleState()) {
                processor.presetSlotsSaved[i] = false;
                processor.presets[i] = SceneState();
                presetFlashTimer[i] = 24;
                presetFlashType[i] = 1; 
                initButton.setToggleState (false, juce::dontSendNotification);
                initButton.repaint();
            }
            else if (saveButton.getToggleState()) { 
                processor.savePreset (i);
                presetFlashTimer[i] = 24;
                presetFlashType[i] = 1; 
                saveButton.setToggleState (false, juce::dontSendNotification);
                saveButton.repaint();
            }
            else if (copyButton.getToggleState()) {
                if (copySourcePresetIndex == -1) {
                    copySourcePresetIndex = i;
                    presetFlashTimer[i] = 24;
                    presetFlashType[i] = 2; 
                }
                else {
                    if (copySourcePresetIndex != i) {
                        processor.presets[i] = processor.presets[copySourcePresetIndex];
                        processor.presetSlotsSaved[i] = processor.presetSlotsSaved[copySourcePresetIndex];
                        presetFlashTimer[i] = 24;
                        presetFlashType[i] = 1; 
                    }
                    copySourcePresetIndex = -1; 
                    copyButton.setToggleState (false, juce::dontSendNotification);
                    copyButton.repaint();
                }
            }
            else if (recallButton.getToggleState()) {
                processor.loadPreset (i); presetFlashTimer[i] = 24; presetFlashType[i] = 2;
                recallButton.setToggleState (false, juce::dontSendNotification); recallButton.repaint();
            }
            else if (event.mods.isRightButtonDown()) { 
                processor.savePreset (i); presetFlashTimer[i] = 24; presetFlashType[i] = 1; 
            }
        }
    }

    if (event.eventComponent == &saveButton) { savePressStartTime = juce::Time::getMillisecondCounter(); saveAlreadySaved = false; }
    else if (event.eventComponent == &recallButton) { recallPressStartTime = juce::Time::getMillisecondCounter(); recallAlreadySaved = false; }
    else if (event.eventComponent == &copyButton) { copyPressStartTime = juce::Time::getMillisecondCounter(); copyAlreadySaved = false; }
    else if (event.eventComponent == &initButton) { initPressStartTime = juce::Time::getMillisecondCounter(); initAlreadySaved = false; }
    else if (event.eventComponent == &sceneAButton) { sceneAPressStartTime = juce::Time::getMillisecondCounter(); sceneAAlreadySaved = false; }
    else if (event.eventComponent == &sceneBButton) { sceneBPressStartTime = juce::Time::getMillisecondCounter(); sceneBActiveState = false; }
}

void PluginEditor::mouseUp (const juce::MouseEvent& event)
{
    for (int i = 0; i < 8; ++i) { if (event.eventComponent == &presetButtons[i]) { presetPressStartTime[i] = 0; presetAlreadySaved[i] = false; } }
    if (event.eventComponent == &sceneAButton) { sceneAPressStartTime = 0; sceneAAlreadySaved = false; }
    if (event.eventComponent == &sceneBButton) { sceneBPressStartTime = 0; sceneBActiveState = false; }
    if (event.eventComponent == &saveButton) { savePressStartTime = 0; saveAlreadySaved = false; }
    if (event.eventComponent == &recallButton) { recallPressStartTime = 0; recallAlreadySaved = false; }
    if (event.eventComponent == &copyButton) { copyPressStartTime = 0; copyAlreadySaved = false; }
    if (event.eventComponent == &initButton) { initPressStartTime = 0; initAlreadySaved = false; }
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Draw static faceplate background
    if (backgroundImage.isValid())
    {
        g.drawImage (backgroundImage, getLocalBounds().toFloat(), 
                     juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll (juce::Colour (0xFF0D1E36));
    }
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    // Render visual diagnostic grid on top of all UI components [1.1.8]
    if (DRAW_DIAGNOSTIC_GRID)
    {
        const int width = getWidth();
        const int height = getHeight();

        // Vertical Grid lines (Cyan/Blue tint)
        for (int x = 50; x < width; x += 50)
        {
            g.setColour (juce::Colour (0x4400E1FF));
            g.drawVerticalLine (x, 0.0f, static_cast<float> (height));
            
            g.setColour (juce::Colour (0xBB00E1FF));
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (juce::String (x), x - 12, height - 12, 24, 10, juce::Justification::centred);
        }

        // Horizontal Grid lines (Pink/Red tint)
        for (int y = 50; y < height; y += 50)
        {
            g.setColour (juce::Colour (0x44FF3366));
            g.drawHorizontalLine (y, 0.0f, static_cast<float> (width));
            
            g.setColour (juce::Colour (0xBBFF3366));
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (juce::String (y), 4, y - 5, 24, 10, juce::Justification::left);
        }

        // Track and draw current mouse cursor coordinate bubble
        auto mousePos = getMouseXYRelative();
        if (mousePos.x >= 0 && mousePos.x <= width && mousePos.y >= 0 && mousePos.y <= height)
        {
            g.setColour (juce::Colours::yellow.withAlpha (0.5f));
            g.drawVerticalLine (mousePos.x, 0.0f, static_cast<float> (height));
            g.drawHorizontalLine (mousePos.y, 0.0f, static_cast<float> (width));

            g.setColour (juce::Colours::black.withAlpha (0.85f));
            g.fillRoundedRectangle (static_cast<float> (mousePos.x) + 12.0f, static_cast<float> (mousePos.y) + 12.0f, 65.0f, 20.0f, 3.0f);
            
            g.setColour (juce::Colours::yellow);
            g.drawRoundedRectangle (static_cast<float> (mousePos.x) + 12.0f, static_cast<float> (mousePos.y) + 12.0f, 65.0f, 20.0f, 3.0f, 1.0f);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            
            juce::String coordStr = juce::String (mousePos.x) + ", " + juce::String (mousePos.y);
            g.drawText (coordStr, mousePos.x + 12, mousePos.y + 12, 65, 20, juce::Justification::centred);
        }
    }
}

void PluginEditor::mouseMove (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (DRAW_DIAGNOSTIC_GRID)
        repaint(); // Force paintOverChildren to update as cursor tracks
}

void PluginEditor::resized()
{
    // Integrated exact pinpoint measurements from your $1521x1034 diagnostic screenshots
    
    // 1. OLED Display screen bezel (exact bounding box)
    oledDisplay.setBounds (243, 91, 1035, 487);

    // 2. Left sidebar 2x2 grid centered exactly on button caps (Centers: X=54.5, 97.5, Y=61.5, 94.0)
    // Sized to 45x30
    saveButton.setBounds (32, 46, 45, 30); 
    recallButton.setBounds (75, 46, 45, 30); 
    copyButton.setBounds (32, 79, 45, 30); 
    initButton.setBounds (75, 79, 45, 30);

    // 3. Left sidebar small knobs centered exactly on dials (Centers: X=73.5, Y=145.5, 208.0, 270.5, 333.0)
    // Sized to 111x70, giving them 7px TextBoxBelow height offset alignment
    rhythmMorphKnob.setBounds (18, 110, 111, 70);
    restKnob.setBounds (18, 173, 111, 70);
    legatoKnob.setBounds (18, 235, 111, 70);
    rateKnob.setBounds (18, 298, 111, 70);

    // 4. Left Master Knob sitting exactly on its dial center (Center: X=73.0, Y=438.0)
    masterVelocityKnob.setBounds (13, 378, 121, 121);

    // 5. Right sidebar 2x2 grid centered exactly on button caps (Centers: X=902.5, 945.5, Y=61.5, 94.0)
    // Sized to 45x30
    diceMeloButton.setBounds (880, 46, 45, 30); 
    diceArtiButton.setBounds (923, 46, 45, 30); 
    diceTimeButton.setBounds (880, 79, 45, 30); 
    diceNavyButton.setBounds (923, 79, 45, 30);

    // 6. Right sidebar small knobs centered exactly on dials (Centers: X=926.5, Y=145.5, 208, 270.5, 333)
    entropyKnob.setBounds (871, 110, 111, 70);
    harmonyKnob.setBounds (871, 173, 111, 70);
    chaosKnob.setBounds (871, 235, 111, 70);
    octavesKnob.setBounds (871, 298, 111, 70);

    // 7. Right Master Knob sitting exactly on its dial center (Center: X=927, Y=440)
    masterSwingKnob.setBounds (866, 378, 121, 121);

    // 8. Top Row Dropdowns (Aligned inside header slots, centered vertically on Y = 26)
    rootKeyBox.setBounds (162, 15, 80, 22); 
    scaleTypeBox.setBounds (247, 15, 80, 22); 
    cycleLengthBox.setBounds (332, 15, 80, 22);
    panelThemeBox.setBounds (417, 15, 80, 22); 
    
    // 9. Top Row Performance buttons (Aligned inside header slots, centered vertically on Y = 26)
    latchButton.setBounds (503, 15, 80, 22); 
    arpSeqButton.setBounds (588, 15, 80, 22); 
    polyButton.setBounds (673, 15, 80, 22); 
    freezeButton.setBounds (758, 15, 80, 22);

    // 10. Horizontal Crossfader Row restricted between Button A and B (Centers: X=255 and 745, Axis Y = 424)
    // Crossfader track fits perfectly inside the printed 350-650 slot range (width 300)
    sceneAButton.setBounds (233, 402, 44, 44);
    morphCrossfader.setBounds (350, 404, 300, 40);
    sceneBButton.setBounds (723, 402, 44, 44);

    // 11. Symmetrical Column Alignment (Y = 897 to 980 vertical upfader tracks)
    // Centered horizontally on the 8 columns starting at 117 with exact 184px spacing
    const float trackCenters[8] = { 117.0f, 301.0f, 485.0f, 669.0f, 852.0f, 1036.0f, 1220.0f, 1404.0f };
    for (int i = 0; i < 8; ++i) 
    {
        // Preset Matrix Switches centered over their tracks (Y = 669 to 775, center Y = 722)
        presetButtons[i].setBounds (static_cast<int> (trackCenters[i]) - 35, 687, 70, 70);

        // Upfaders aligned inside their slots inside the black bottom strip (Y = 897 to 980 fader slot tracks)
        juce::Slider* faderPtrs[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
        faderPtrs[i]->setBounds (static_cast<int> (trackCenters[i]) - 15, 885, 30, 110);
    }
}

void PluginEditor::timerCallback()
{
    uint32_t now = juce::Time::getMillisecondCounter();
    bool isArp = *processor.apvts.getRawParameterValue (IDs::arpSeq.getParamID()) > 0.5f; 
    arpSeqButton.setButtonText (isArp ? "Arp" : "Seq");

    static bool lastAnchorB = false; bool currentAnchorB = processor.isSceneBActiveAnchor.load();
    if (currentAnchorB != lastAnchorB) { 
        lastAnchorB = currentAnchorB; 
        sceneAFlashTimer = 24; 
        sceneBFlashTimer = 24;
        sceneAButton.repaint(); 
        sceneBButton.repaint(); 
    }

    static bool lastFreezeState = false;
    bool currentFreeze = *processor.apvts.getRawParameterValue (IDs::freeze.getParamID()) > 0.5f;
    if (currentFreeze != lastFreezeState) {
        lastFreezeState = currentFreeze;
        int activeThemeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
        auto t = AppTheme::get (activeThemeIdx);
        
        juce::Colour borderCol = currentFreeze ? juce::Colour (0xFF80D8FF) : t.slotOutline;
        juce::Colour textCol = currentFreeze ? juce::Colour (0xFF80D8FF) : t.textDim;
        
        juce::Slider* knobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &masterVelocityKnob, &masterSwingKnob };
        for (auto* k : knobs) {
            k->setColour (juce::Slider::textBoxOutlineColourId, borderCol);
            k->setColour (juce::Slider::textBoxTextColourId, textCol);
            k->repaint();
        }
        freezeButton.repaint();
    }

    if (sceneAFlashTimer > 0) { sceneAFlashTimer--; if (sceneAFlashTimer == 0) sceneAButton.repaint(); }
    if (sceneBFlashTimer > 0) { sceneBFlashTimer--; if (sceneBFlashTimer == 0) sceneBButton.repaint(); }
    if (saveFlashTimer > 0) { saveFlashTimer--; if (saveFlashTimer == 0) saveButton.repaint(); }
    if (recallFlashTimer > 0) { recallFlashTimer--; if (recallFlashTimer == 0) recallButton.repaint(); }
    if (copyFlashTimer > 0) { copyFlashTimer--; if (copyFlashTimer == 0) copyButton.repaint(); }
    if (initFlashTimer > 0) { initFlashTimer--; if (initFlashTimer == 0) initButton.repaint(); }

    float morphVal = static_cast<float> (morphCrossfader.getValue());
    auto interpolate = [morphVal](float valA, float valB) -> float {
        return (valA * (1.0f - morphVal)) + (valB * morphVal);
    };

    if (!rhythmMorphKnob.isMouseButtonDown()) rhythmMorphKnob.setValue (interpolate (processor.sceneA.rhythmMorph, processor.sceneB.rhythmMorph), juce::dontSendNotification);
    if (!restKnob.isMouseButtonDown())        restKnob.setValue (interpolate (processor.sceneA.rest,        processor.sceneB.rest),        juce::dontSendNotification);
    if (!legatoKnob.isMouseButtonDown())      legatoKnob.setValue (interpolate (processor.sceneA.legato,    processor.sceneB.legato),      juce::dontSendNotification);
    if (!rateKnob.isMouseButtonDown())        rateKnob.setValue (interpolate (processor.sceneA.rate,        processor.sceneB.rate),        juce::dontSendNotification);
    if (!entropyKnob.isMouseButtonDown())     entropyKnob.setValue (interpolate (processor.sceneA.entropy,   processor.sceneB.entropy),   juce::dontSendNotification);
    if (!harmonyKnob.isMouseButtonDown())     harmonyKnob.setValue (interpolate (processor.sceneA.harmony,   processor.sceneB.harmony),   juce::dontSendNotification);
    if (!chaosKnob.isMouseButtonDown())       chaosKnob.setValue (interpolate (processor.sceneA.chaos,     processor.sceneB.chaos),     juce::dontSendNotification);
    if (!octavesKnob.isMouseButtonDown())     octavesKnob.setValue (interpolate (processor.sceneA.octaves,   processor.sceneB.octaves),   juce::dontSendNotification);

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
    if (sceneBButton.isMouseButtonDown() && sceneBPressStartTime != 0 && !sceneBActiveState) { if (now - sceneBPressStartTime >= 1000) { processor.setActiveAnchor (true); sceneBActiveState = true; sceneBFlashTimer = 24; } }

    if (saveButton.isMouseButtonDown() && savePressStartTime != 0 && !saveAlreadySaved) { if (now - savePressStartTime >= 1000) { processor.savePreset (processor.activePresetIndex.load()); saveAlreadySaved = true; saveFlashTimer = 24; saveButton.setToggleState (false, juce::dontSendNotification); saveButton.repaint(); } }
    if (recallButton.isMouseButtonDown() && recallPressStartTime != 0 && !recallAlreadySaved) { if (now - recallPressStartTime >= 1000) { processor.loadPreset (processor.activePresetIndex.load()); recallAlreadySaved = true; recallFlashTimer = 24; recallButton.setToggleState (false, juce::dontSendNotification); recallButton.repaint(); } }
    if (copyButton.getToggleState() && copySourcePresetIndex != -1) {
        presetFlashTimer[copySourcePresetIndex] = 24;
        presetFlashType[copySourcePresetIndex] = 2; 
    }
    if (copyButton.isMouseButtonDown() && copyPressStartTime != 0 && !copyAlreadySaved) { if (now - copyPressStartTime >= 1000) { processor.sceneB = processor.sceneA; processor.hasSceneB = processor.hasSceneA; copyAlreadySaved = true; copyFlashTimer = 24; copyButton.setToggleState (false, juce::dontSendNotification); copyButton.repaint(); } }
    if (initButton.isMouseButtonDown() && initPressStartTime != 0 && !initAlreadySaved) {
        if (now - initPressStartTime >= 1000) {
            for (auto* param : processor.getParameters()) { if (param != nullptr) param->setValueNotifyingHost (param->getDefaultValue()); }
            initAlreadySaved = true; initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint();
        }
    }

    oledDisplay.repaint();
}

void PluginEditor::updateSliderTextBoxThemeColors()
{
    // Override standard theme configurations to lock the textboxes to high-contrast dark panels
    juce::Slider* allKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    for (auto* k : allKnobs)
    {
        // Force textbox colors to lock onto dark-midnight theme styling, removing outlines to blend into graphics
        k->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF00D2FF));      // Glowing Electric Cyan Text
        k->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xFF0F1116)); // Matte Dark Backing
        k->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); // Translucent/None Border
    }

    // Explicitly hide the textboxes on master dials so they render as clean pure physical knobs
    masterVelocityKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterSwingKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    juce::Label* leftTitles[] = { &rhythmMorphTitle, &restTitle, &legatoTitle, &rateTitle };
    juce::Label* rightTitles[] = { &entropyTitle, &harmonyTitle, &chaosTitle, &octavesTitle };
    for (auto* title : leftTitles)
        title->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    for (auto* title : rightTitles)
        title->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
}