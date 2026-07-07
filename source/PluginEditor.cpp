#define DRAW_DIAGNOSTIC_GRID 0

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "OledDisplay.h"
#include "ChromaCapsLookAndFeel.h"
#include "AppTheme.h"

// =====================================================================
// PERSISTENT DYNAMIC COMPONENT WRAPPER (ZERO HEADER FOOTPRINT) [1.2.3]
// =====================================================================
class SyncButtonWrapper : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<SyncButtonWrapper>;

    SyncButtonWrapper (juce::AudioProcessorValueTreeState& apvts, juce::Component& parent, juce::LookAndFeel& lf)
        : button ("Sync")
    {
        parent.addAndMakeVisible (button);
        button.setClickingTogglesState (true);
        button.setLookAndFeel (&lf);
        button.setComponentID ("sync");
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "sync", button);
    }

    ~SyncButtonWrapper() override
    {
        button.setLookAndFeel (nullptr);
    }

    juce::TextButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p, this)
{
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
    juce::String leftPrefixes[] = { "rhythmMorph", "rest", "legato", "rate" };
    for (int i = 0; i < 4; ++i) {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); 
        leftKnobs[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0); 
        leftKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        leftKnobs[i]->setComponentID (leftPrefixes[i]); 
        leftKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (leftKnobs[i]);
    }

    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::String rightPrefixes[] = { "entropy", "harmony", "chaos", "octaves" };
    for (int i = 0; i < 4; ++i) {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); 
        rightKnobs[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0); 
        rightKnobs[i]->setLookAndFeel (&chromaLookAndFeel); 
        rightKnobs[i]->setComponentID (rightPrefixes[i]); 
        rightKnobs[i]->addMouseListener (this, false); 
        addAndMakeVisible (rightKnobs[i]);
    }

    masterVelocityKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterVelocityKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterVelocityKnob.setLookAndFeel (&chromaLookAndFeel);
    masterVelocityKnob.setComponentID ("masterVelocity");
    masterVelocityKnob.addMouseListener (this, false);
    addAndMakeVisible (masterVelocityKnob);

    masterSwingKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterSwingKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterSwingKnob.setLookAndFeel (&chromaLookAndFeel);
    masterSwingKnob.setComponentID ("masterSwing");
    masterSwingKnob.addMouseListener (this, false);
    addAndMakeVisible (masterSwingKnob);

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

    // Instantiates Sync button wrapper dynamically to keep header untouched [1.2.3]
    auto* syncWrapper = new SyncButtonWrapper (processor.apvts, *this, chromaLookAndFeel);
    getProperties().set ("syncWrapper", syncWrapper);

    juce::TextButton* sceneBtns[] = { &sceneAButton, &sceneBButton }; 
    juce::String sceneTxt[] = { "A", "B" };
    for (int i = 0; i < 2; ++i) { 
        addAndMakeVisible (sceneBtns[i]); 
        sceneBtns[i]->setButtonText (sceneTxt[i]); 
        sceneBtns[i]->addMouseListener (this, false); 
        sceneBtns[i]->setLookAndFeel (&chromaLookAndFeel); 
        sceneBtns[i]->setTriggeredOnMouseDown (true); // Instantly switches on mouse down
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

    // Cleaned 2x2 focused grid resets to directly update targeted snaps [1.2.1]
    diceMeloButton.setComponentID ("dice_melody"); 
    diceMeloButton.setButtonText ("Melo"); 
    diceMeloButton.setLookAndFeel (&chromaLookAndFeel); 
    diceMeloButton.onClick = [this] { 
        if (initButton.getToggleState()) { 
            processor.resetRhythm(); 
            initFlashTimer = 24; 
            initButton.setToggleState (false, juce::dontSendNotification); 
            initButton.repaint(); 
        } else { 
            processor.diceMelody(); 
        } 
    };
    addAndMakeVisible (diceMeloButton); 
    
    diceArtiButton.setComponentID ("dice_articulation"); 
    diceArtiButton.setButtonText ("Arti"); 
    diceArtiButton.setLookAndFeel (&chromaLookAndFeel); 
    diceArtiButton.onClick = [this] { 
        if (initButton.getToggleState()) { 
            bool isSceneB = processor.isSceneBActiveAnchor.load();
            SceneState& activeScene = isSceneB ? processor.sceneB : processor.sceneA;
            activeScene.rest = 0.1f;    // Default Rest
            activeScene.legato = 0.5f;  // Default Legato
            initFlashTimer = 24; 
            initButton.setToggleState (false, juce::dontSendNotification); 
            initButton.repaint(); 
        } else { 
            processor.diceArticulation(); 
        } 
    };
    addAndMakeVisible (diceArtiButton); 
    
    diceTimeButton.setComponentID ("dice_time"); 
    diceTimeButton.setButtonText ("Time"); 
    diceTimeButton.setLookAndFeel (&chromaLookAndFeel); 
    diceTimeButton.onClick = [this] { 
        if (initButton.getToggleState()) { 
            bool isSceneB = processor.isSceneBActiveAnchor.load();
            SceneState& activeScene = isSceneB ? processor.sceneB : processor.sceneA;
            activeScene.rate = 0.5f;     // Default rate center
            activeScene.octaves = 0.0f;  // Default 0 octaves
            processor.apvts.getParameter (IDs::cycleLength.getParamID())->setValueNotifyingHost (2.0f / 3.0f); 
            initFlashTimer = 24; 
            initButton.setToggleState (false, juce::dontSendNotification); 
            initButton.repaint(); 
        } else { 
            processor.diceTime(); 
        } 
    };
    addAndMakeVisible (diceTimeButton); 
    
    diceNavyButton.setComponentID ("dice_navy"); 
    diceNavyButton.setButtonText ("Navy"); 
    diceNavyButton.setLookAndFeel (&chromaLookAndFeel); 
    diceNavyButton.onClick = [this] { 
        if (initButton.getToggleState()) { 
            bool isSceneB = processor.isSceneBActiveAnchor.load();
            SceneState& activeScene = isSceneB ? processor.sceneB : processor.sceneA;
            activeScene.rhythmMorph = 0.0f;
            activeScene.entropy = 0.0f;
            activeScene.harmony = 0.0f;
            activeScene.chaos = 0.0f;
            initFlashTimer = 24; 
            initButton.setToggleState (false, juce::dontSendNotification); 
            initButton.repaint(); 
        } else { 
            processor.diceNavy(); 
        } 
    };
    addAndMakeVisible (diceNavyButton); 

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
        else {
            // Instant selection toggle (no long press needed)
            processor.setActiveAnchor (false); 
            sceneAFlashTimer = 24;
            sceneAButton.repaint();
            sceneBButton.repaint();
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
        else {
            // Instant selection toggle (no long press needed)
            processor.setActiveAnchor (true); 
            sceneBFlashTimer = 24;
            sceneAButton.repaint();
            sceneBButton.repaint();
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
    panelThemeBox.addItemList (juce::StringArray { "Navy", "Monochrome", "Matrix" }, 1);
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

    // Explicitly enable double-click to reset parameters to default values [1.2.1]
    rhythmMorphKnob.setDoubleClickReturnValue (true, 0.0f);
    restKnob.setDoubleClickReturnValue (true, 0.1f);
    legatoKnob.setDoubleClickReturnValue (true, 0.5f);
    rateKnob.setDoubleClickReturnValue (true, 0.5f); // Default center BPM
    entropyKnob.setDoubleClickReturnValue (true, 0.0f);
    harmonyKnob.setDoubleClickReturnValue (true, 0.0f);
    chaosKnob.setDoubleClickReturnValue (true, 0.0f);
    octavesKnob.setDoubleClickReturnValue (true, 0.0f);
    masterVelocityKnob.setDoubleClickReturnValue (true, 0.5f); // Note Density default (50%)
    masterSwingKnob.setDoubleClickReturnValue (true, 0.0f); // Master Swing default (0%)

    for (int i = 0; i < 8; ++i) {
        faders[i]->setDoubleClickReturnValue (true, 0.5f); // Default 50% probability
    }

    // Connect slider callbacks to automatically update active scene state [1.2.2] using self-contained getProperties flags [1.2.3]
    rhythmMorphKnob.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.rhythmMorph = static_cast<float>(rhythmMorphKnob.getValue()); else processor.sceneA.rhythmMorph = static_cast<float>(rhythmMorphKnob.getValue()); } };
    restKnob.onValueChange        = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.rest = static_cast<float>(restKnob.getValue()); else processor.sceneA.rest = static_cast<float>(restKnob.getValue()); } };
    legatoKnob.onValueChange      = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.legato = static_cast<float>(legatoKnob.getValue()); else processor.sceneA.legato = static_cast<float>(legatoKnob.getValue()); } };
    rateKnob.onValueChange        = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.rate = static_cast<float>(rateKnob.getValue()); else processor.sceneA.rate = static_cast<float>(rateKnob.getValue()); } };
    entropyKnob.onValueChange     = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.entropy = static_cast<float>(entropyKnob.getValue()); else processor.sceneA.entropy = static_cast<float>(entropyKnob.getValue()); } };
    harmonyKnob.onValueChange     = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.harmony = static_cast<float>(harmonyKnob.getValue()); else processor.sceneA.harmony = static_cast<float>(harmonyKnob.getValue()); } };
    chaosKnob.onValueChange       = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.chaos = static_cast<float>(chaosKnob.getValue()); else processor.sceneA.chaos = static_cast<float>(chaosKnob.getValue()); } };
    octavesKnob.onValueChange     = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.octaves = static_cast<float>(octavesKnob.getValue()); else processor.sceneA.octaves = static_cast<float>(octavesKnob.getValue()); } };

    fader1.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[0] = static_cast<float>(fader1.getValue()); else processor.sceneA.faders[0] = static_cast<float>(fader1.getValue()); } };
    fader2.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[1] = static_cast<float>(fader2.getValue()); else processor.sceneA.faders[1] = static_cast<float>(fader2.getValue()); } };
    fader3.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[2] = static_cast<float>(fader3.getValue()); else processor.sceneA.faders[2] = static_cast<float>(fader3.getValue()); } };
    fader4.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[3] = static_cast<float>(fader4.getValue()); else processor.sceneA.faders[3] = static_cast<float>(fader4.getValue()); } };
    fader5.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[4] = static_cast<float>(fader5.getValue()); else processor.sceneA.faders[4] = static_cast<float>(fader5.getValue()); } };
    fader6.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[5] = static_cast<float>(fader6.getValue()); else processor.sceneA.faders[5] = static_cast<float>(fader6.getValue()); } };
    fader7.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[6] = static_cast<float>(fader7.getValue()); else processor.sceneA.faders[6] = static_cast<float>(fader7.getValue()); } };
    fader8.onValueChange = [this] { if (! getProperties().getWithDefault ("isUpdatingProgrammatically", false)) { if (processor.isSceneBActiveAnchor.load()) processor.sceneB.faders[7] = static_cast<float>(fader8.getValue()); else processor.sceneA.faders[7] = static_cast<float>(fader8.getValue()); } };

    setResizable (false, false); 
    setSize (1000, 681); // Matches physical artwork dimension 

    if (DRAW_DIAGNOSTIC_GRID)
        setMouseClickGrabsKeyboardFocus (true);

    // Ensure all knobs and upfaders are on the absolute top visual layer [1.2.1]
    juce::Slider* allSliders[] = { 
        &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8,
        &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob,
        &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob,
        &masterVelocityKnob, &masterSwingKnob, &morphCrossfader
    };
    for (auto* s : allSliders) {
        s->toFront (false);
    }

    startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); processor.apvts.removeParameterListener ("panelTheme", this);
    juce::Slider* sliders[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &masterVelocityKnob, &masterSwingKnob, &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8, &morphCrossfader };
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
    
    // Explicitly sized array definition to bypass MSVC range-based template confusion [1.2.3]
    juce::TextButton* btns[14] = { &diceMeloButton, &diceArtiButton, &diceTimeButton, &diceNavyButton, &latchButton, &arpSeqButton, &polyButton, &freezeButton, &sceneAButton, &sceneBButton, &saveButton, &recallButton, &copyButton, &initButton };
    for (int i = 0; i < 14; ++i) { btns[i]->setLookAndFeel (nullptr); btns[i]->onClick = nullptr; }
    
    for (int i = 0; i < 8; ++i) { presetButtons[i].setLookAndFeel (nullptr); presetButtons[i].onClick = nullptr; presetButtons[i].onStateChange = nullptr; presetButtons[i].removeMouseListener(this); }
    sceneAButton.removeMouseListener (this); sceneBButton.removeMouseListener (this);

    // Dynamic clean up of property-wrapped syncButton [1.2.3]
    getProperties().remove ("syncWrapper");
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

    // Intercept mouse click before dragging starts to clear LFO parameter states if Init is toggled [1.2.3]
    if (initButton.getToggleState())
    {
        for (int i = 0; i < 8; ++i)
        {
            if (event.eventComponent == knobs[i])
            {
                auto* rateParam = processor.apvts.getParameter (rates[i].getParamID());
                auto* depthParam = processor.apvts.getParameter (depths[i].getParamID());
                if (rateParam != nullptr && depthParam != nullptr)
                {
                    rateParam->setValueNotifyingHost (0.0f);  // Reset LFO Speed to "Off" [1.2.3]
                    depthParam->setValueNotifyingHost (0.0f); // Reset LFO Depth to "0%" [1.2.3]
                }
                initButton.setToggleState (false, juce::dontSendNotification);
                initButton.repaint();
                return; // Intercept entirely to prevent LFO menus popping up
            }
        }
    }

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
                // REMOVED untoggling recallButton to allow instant preset slot surfing [1.2.0]
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
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    juce::Colour themeColor = juce::Colour (0xFF00E5FF); // Theme 0 (Navy): Teal
    if (themeIdx == 1)      themeColor = juce::Colour (0xFFECEFF1); // Theme 1 (Monochrome): White/Silver
    else if (themeIdx == 2) themeColor = juce::Colour (0xFF00FF66); // Theme 2 (Matrix): Neon Green

    if (DRAW_DIAGNOSTIC_GRID)
    {
        const int width = getWidth();
        const int height = getHeight();

        for (int x = 50; x < width; x += 50)
        {
            g.setColour (juce::Colour (0x4400E1FF));
            g.drawVerticalLine (x, 0.0f, static_cast<float> (height));
            
            g.setColour (juce::Colour (0xBB00E1FF));
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (juce::String (x), x - 12, height - 12, 24, 10, juce::Justification::centred);
        }

        for (int y = 50; y < height; y += 50)
        {
            g.setColour (juce::Colour (0x44FF3366));
            g.drawHorizontalLine (y, 0.0f, static_cast<float> (width));
            
            g.setColour (juce::Colour (0xBBFF3366));
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (juce::String (y), 4, y - 5, 24, 10, juce::Justification::left);
        }

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

    // Custom Small HUD display boxes under the 8 small knobs [1.2.0]
    juce::Slider* smallKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::String smallLabels[] = { "MORPH", "REST", "LEGATO", "BPM" /* Renamed [1.2.3] */, "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };
    int smallKnobsX[] = { 44, 44, 44, 44, 902, 902, 902, 902 };
    int smallKnobsY[] = { 121, 182, 244, 306, 121, 182, 244, 306 };

    for (int i = 0; i < 8; ++i)
    {
        int boxX = smallKnobsX[i];
        int boxY = smallKnobsY[i] + 48; // Located exactly 48px below knob origins
        int boxW = 48;
        int boxH = 12;

        g.setColour (juce::Colour (0xFF05070A)); // Solid dark fill to clear background faceplate area
        g.fillRect (boxX, boxY, boxW, boxH);

        // Safe JUCE getThumbBeingDragged check to monitor active dragging
        if (smallKnobs[i]->getThumbBeingDragged() >= 0)
        {
            // Display a sleek horizontal progress bar instead of text [1.2.0]
            float val = static_cast<float> (smallKnobs[i]->getValue());
            float progress = val;
            if (smallLabels[i] == "ENTROPY")  progress = (val + 1.0f) * 0.5f;
            else if (smallLabels[i] == "OCTAVES")  progress = (val + 3.0f) / 6.0f;
            progress = juce::jlimit (0.0f, 1.0f, progress);

            int fillW = static_cast<int> (std::round (progress * static_cast<float> (boxW - 4)));
            g.setColour (themeColor);
            g.fillRect (boxX + 2, boxY + 4, fillW, boxH - 8);
        }
        else
        {
            // Center the neat uppercase label text
            g.setColour (themeColor.withAlpha (0.75f));
            g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
            g.drawFittedText (smallLabels[i], boxX, boxY, boxW, boxH, juce::Justification::centred, 1);
        }
    }

    // Custom Master HUD display boxes centered directly under the big knobs [1.2.0]
    // 1. Bottom-Left: Master Note Density (VEL) HUD Box
    juce::Rectangle<int> denBox (33, 484, 70, 15);
    g.setColour (juce::Colour (0xFF05070A));
    g.fillRoundedRectangle (denBox.toFloat(), 2.0f);
    g.setColour (themeColor.withAlpha (0.4f));
    g.drawRoundedRectangle (denBox.toFloat(), 2.0f, 1.0f);

    g.setColour (themeColor); // RESET COLOR TO FULL THEME COLOR [1.2.0]
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    juce::String denText = "DEN: " + juce::String (static_cast<int> (std::round (masterVelocityKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (denText, denBox, juce::Justification::centred, 1);

    // 2. Bottom-Right: Master Swing (SWG) HUD Box
    juce::Rectangle<int> swgBox (891, 484, 70, 15);
    g.setColour (juce::Colour (0xFF05070A));
    g.fillRoundedRectangle (swgBox.toFloat(), 2.0f);
    g.setColour (themeColor.withAlpha (0.4f));
    g.drawRoundedRectangle (swgBox.toFloat(), 2.0f, 1.0f);

    g.setColour (themeColor); // RESET COLOR TO FULL THEME COLOR [1.2.0] (Fixes greyed-out text bug)
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    juce::String swgText = "SWG: " + juce::String (static_cast<int> (std::round (masterSwingKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (swgText, swgBox, juce::Justification::centred, 1);

    // 3. Draw static, high-contrast white "MEMORY SLOTS" label under the buttons [1.2.3]
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
    g.drawText ("MEMORY SLOTS", 0, 552, 1000, 16, juce::Justification::centred);

    // 4. Draw static "NAVY-ARP MONITOR" stamped onto physical panel [1.2.3]
    g.setColour (themeColor.withAlpha (0.5f));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("NAVY-ARP MONITOR", 157, 381, 680, 15, juce::Justification::centred, true);
}

void PluginEditor::mouseMove (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (DRAW_DIAGNOSTIC_GRID)
        repaint(); 
}

void PluginEditor::resized()
{
    // Screen bounds (OLED)
    oledDisplay.setBounds (157, 57, 680, 320);

    // Left 2x2 Utility Grid (Save, Recall, Copy, Init)
    saveButton.setBounds (31, 49, 33, 28); 
    recallButton.setBounds (73, 49, 33, 28); 
    copyButton.setBounds (31, 83, 33, 28); 
    initButton.setBounds (73, 83, 33, 28);

    // Left Column small Knobs
    rhythmMorphKnob.setBounds (44, 121, 48, 48);
    restKnob.setBounds (44, 182, 48, 48);
    legatoKnob.setBounds (44, 244, 48, 48);
    rateKnob.setBounds (44, 306, 48, 48);
    
    // Left Master Velocity Knob
    masterVelocityKnob.setBounds (26, 396, 84, 86);

    // Right 2x2 Utility Grid (Melo, Arti, Time, Navy)
    diceMeloButton.setBounds (888, 48, 33, 28); 
    diceArtiButton.setBounds (930, 48, 33, 28); 
    diceTimeButton.setBounds (888, 84, 33, 28); 
    diceNavyButton.setBounds (930, 84, 33, 28);

    // Right Column small Knobs
    entropyKnob.setBounds (902, 121, 48, 48);
    harmonyKnob.setBounds (902, 182, 48, 48);
    chaosKnob.setBounds (902, 244, 48, 48);
    octavesKnob.setBounds (902, 306, 48, 48);
    
    // Right Master Swing Knob
    masterSwingKnob.setBounds (884, 396, 84, 86);

    // Top Dropdowns (Left)
    rootKeyBox.setBounds (163, 17, 58, 17); 
    scaleTypeBox.setBounds (233, 17, 58, 17); 
    cycleLengthBox.setBounds (304, 17, 58, 17);
    panelThemeBox.setBounds (374, 17, 58, 17); 
    
    // Top Row Performance Buttons (Right) [1.2.3]
    latchButton.setBounds (533, 15, 58, 17); 
    arpSeqButton.setBounds (602, 15, 58, 17); 
    polyButton.setBounds (669, 15, 58, 17); 
    freezeButton.setBounds (738, 15, 58, 17);
    
    // Safety boundaries set dynamically using property fetches for wrapped syncButton [1.2.3]
    auto syncWrapper = SyncButtonWrapper::Ptr (dynamic_cast<SyncButtonWrapper*> (getProperties()["syncWrapper"].getObject()));
    if (syncWrapper != nullptr)
    {
        syncWrapper->button.setBounds (807, 15, 17, 17); // Sited nicely after freeze [1.2.3]
    }

    // Crossfader Row
    sceneAButton.setBounds (214, 396, 67, 58);
    morphCrossfader.setBounds (336, 415, 321, 28);
    sceneBButton.setBounds (714, 396, 67, 58);

    // Invisible Preset Memory Slots (1-8)
    const float presetX[8] = { 145.0f, 236.0f, 326.0f, 415.0f, 504.0f, 594.0f, 682.0f, 772.0f };
    for (int i = 0; i < 8; ++i) 
    {
        presetButtons[i].setBounds (static_cast<int>(presetX[i]), 477, 76, 65);
    }

    // Vertical Upfaders (1-8)
    const float faderX[8] = { 66.0f, 192.0f, 314.0f, 436.0f, 556.0f, 678.0f, 802.0f, 923.0f };
    juce::Slider* faderPtrs[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    for (int i = 0; i < 8; ++i) 
    {
        faderPtrs[i]->setBounds (static_cast<int>(faderX[i]) - 6, 583, 24, 60);
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

    // Independent Active Focus Editing Routing (Spring-Loaded Motorized Morph) [1.2.0]
    bool isSceneB = processor.isSceneBActiveAnchor.load();
    SceneState& activeScene = isSceneB ? processor.sceneB : processor.sceneA;

    // Direct programmatic updates are flagged to avoid value-change listener triggers [1.2.2] using self-contained getProperties flags [1.2.3]
    getProperties().set ("isUpdatingProgrammatically", true);

    rhythmMorphKnob.setValue (interpolate (processor.sceneA.rhythmMorph, processor.sceneB.rhythmMorph), juce::dontSendNotification);
    restKnob.setValue (interpolate (processor.sceneA.rest, processor.sceneB.rest), juce::dontSendNotification);
    legatoKnob.setValue (interpolate (processor.sceneA.legato, processor.sceneB.legato), juce::dontSendNotification);
    rateKnob.setValue (interpolate (processor.sceneA.rate, processor.sceneB.rate), juce::dontSendNotification);
    entropyKnob.setValue (interpolate (processor.sceneA.entropy, processor.sceneB.entropy), juce::dontSendNotification);
    harmonyKnob.setValue (interpolate (processor.sceneA.harmony, processor.sceneB.harmony), juce::dontSendNotification);
    chaosKnob.setValue (interpolate (processor.sceneA.chaos, processor.sceneB.chaos), juce::dontSendNotification);
    octavesKnob.setValue (interpolate (processor.sceneA.octaves, processor.sceneB.octaves), juce::dontSendNotification);

    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    for (int i = 0; i < 8; ++i) {
        faders[i]->setValue (interpolate (processor.sceneA.faders[i], processor.sceneB.faders[i]), juce::dontSendNotification);
    }

    getProperties().set ("isUpdatingProgrammatically", false);

    // OLED Parameter HUD Overlay Triggering with Standard 3-Argument Signature [1.2.3]
    juce::Slider* smallKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::String smallNames[] = { "Rhythm Morph", "Rest", "Legato", "BPM" /* Renamed to BPM overlay [1.2.3] */, "Entropy", "Harmony", "Chaos", "Octaves" };
    for (int i = 0; i < 8; ++i)
    {
        if (smallKnobs[i]->getThumbBeingDragged() >= 0)
        {
            float val = static_cast<float> (smallKnobs[i]->getValue());
            
            // Format LFO speed label details
            juce::String lfoText = "Off";
            if (processor.lfoRatePtrs[i] != nullptr && processor.lfoDepthPtrs[i] != nullptr)
            {
                int rChoice = static_cast<int> (processor.lfoRatePtrs[i]->load());
                float depth = processor.lfoDepthPtrs[i]->load();
                if (rChoice > 0 && depth > 0.0f)
                {
                    juce::StringArray speeds { "Off", "1/4", "1/8", "1/16", "1/32" };
                    lfoText = speeds[rChoice] + " (" + juce::String (static_cast<int> (depth * 100.0f)) + "%)";
                }
            }

            // Normalizes knob range proportionally for display bar [1.2.3]
            float progress = val;
            if (smallNames[i] == "Entropy")  progress = (val + 1.0f) * 0.5f;
            else if (smallNames[i] == "Octaves")  progress = (val + 3.0f) / 6.0f;
            progress = juce::jlimit (0.0f, 1.0f, progress);

            // Trigger multi-bar HUD overlay using the original 3-arg signature [1.2.3]
            oledDisplay.showParameterOverlay (smallNames[i], progress, lfoText);
        }
    }

    // REMOVED upfader showParameterOverlay calls to satisfy Point 1 [1.2.3]

    if (masterVelocityKnob.getThumbBeingDragged() >= 0)
    {
        oledDisplay.showParameterOverlay ("BPM", static_cast<float> (masterVelocityKnob.getValue()), "Off");
    }

    if (masterSwingKnob.getThumbBeingDragged() >= 0)
    {
        oledDisplay.showParameterOverlay ("Master Swing", static_cast<float> (masterSwingKnob.getValue()), "Off");
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

    if (saveButton.isMouseButtonDown() && savePressStartTime != 0 && !saveAlreadySaved) { if (now - savePressStartTime >= 1000) { processor.savePreset (processor.activePresetIndex.load()); saveAlreadySaved = true; saveFlashTimer = 24; saveButton.setToggleState (false, juce::dontSendNotification); saveButton.repaint(); } }
    if (recallButton.isMouseButtonDown() && recallPressStartTime != 0 && !recallAlreadySaved) { if (now - recallPressStartTime >= 1000) { processor.loadPreset (processor.activePresetIndex.load()); recallAlreadySaved = true; recallFlashTimer = 24; recallButton.setToggleState (false, juce::dontSendNotification); recallButton.repaint(); } }
    if (copyButton.getToggleState() && copySourcePresetIndex != -1) {
        presetFlashTimer[copySourcePresetIndex] = 24;
        presetFlashType[copySourcePresetIndex] = 2; 
    }
    if (copyButton.isMouseButtonDown() && copyPressStartTime != 0 && !copyAlreadySaved) { if (now - copyPressStartTime >= 1000) { processor.sceneB = processor.sceneA; processor.hasSceneB = processor.hasSceneA; copyAlreadySaved = true; copyFlashTimer = 24; copyButton.setToggleState (false, juce::dontSendNotification); copyButton.repaint(); } }
    if (initButton.getToggleState() && copySourcePresetIndex != -1) {
        presetFlashTimer[copySourcePresetIndex] = 24;
        presetFlashType[copySourcePresetIndex] = 2; 
    }
    if (initButton.isMouseButtonDown() && initPressStartTime != 0 && !initAlreadySaved) {
        if (now - initPressStartTime >= 1000) {
            for (auto* param : processor.getParameters()) { if (param != nullptr) param->setValueNotifyingHost (param->getDefaultValue()); }
            initAlreadySaved = true; initFlashTimer = 24; initButton.setToggleState (false, juce::dontSendNotification); initButton.repaint();
        }
    }

    // Force parent repaint to redraw all black HUD display boxes and active LFO knob rings smoothly at 30 fps [1.2.0]
    repaint();
    oledDisplay.repaint();
}

void PluginEditor::updateSliderTextBoxThemeColors()
{
    juce::Slider* allKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    for (auto* k : allKnobs)
    {
        k->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF00D2FF));      
        k->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xFF0F1116)); 
        k->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); 
    }
}