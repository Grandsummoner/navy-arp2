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

// Symmetrical Standalone MIDI Learn bounds helper [3]
static juce::Rectangle<int> getCcPillBounds (int index, int xOffset)
{
    int cx = 0, cy = 0;
    if (index < 4) // Left sidebar knobs (0-3)
    {
        cx = 44 + 48; // Positioned immediately to the right of the knob
        cy = 121 + (index * 61) + 16;
    }
    else if (index >= 4 && index < 8) // Right sidebar knobs (4-7)
    {
        cx = 902 - 38; // Positioned immediately to the left of the knob
        cy = 121 + ((index - 4) * 61) + 16;
    }
    else if (index >= 8 && index < 16) // Step faders (8-15)
    {
        float faderX[8] = { 66.0f, 192.0f, 314.0f, 436.0f, 556.0f, 678.0f, 802.0f, 923.0f };
        cx = static_cast<int> (faderX[index - 8]) - 11;
        cy = 583 - 18; // Hovering right above the fader track
    }
    else if (index == 16) // Master Note Density (DEN)
    {
        cx = 26 + 84; 
        cy = 396 + 32;
    }
    else if (index == 17) // Master Swing (SWG)
    {
        cx = 884 - 38;
        cy = 396 + 32;
    }

    return { cx + xOffset, cy, 34, 15 };
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processor (p), oledDisplay (p), chromaLookAndFeel (p, this)
{
    // Protect active scene state from being corrupted by initial attachment-triggered value-changes [1.2.3]
    getProperties().set ("isUpdatingProgrammatically", true);

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

    // =====================================================================
    // SYMMETRICAL LEFT WING CONTROL INSTANTIATIONS [3]
    // =====================================================================
    addAndMakeVisible (soundButton);
    // Display the music note icon symmetrically stays with the main panel [3]
    soundButton.setButtonText (juce::String::fromUTF8 ("\xe2\x99\xab"));
    soundButton.setClickingTogglesState (true);
    soundButton.onClick = [this] { toggleLeftPanel(); };
    soundButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF181C20));
    soundButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFA0A5B0));
    soundButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    // ComboBox Setup
    juce::ComboBox* boxes[] = { &midiInBox, &midiOutBox, &audioRoutingBox };
    for (auto* b : boxes)
    {
        addAndMakeVisible (b);
        b->setLookAndFeel (&chromaLookAndFeel);
    }
    midiInBox.addItemList (juce::StringArray { "Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
    midiOutBox.addItemList (juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
    audioRoutingBox.addItemList (juce::StringArray { "Split A->1 / B->2", "Layered (Voice 1)", "External Out Only" }, 1);

    // Symmetrical Tab selection setup for Voice 1 (ANALOG, FM, STRING, PULSE) [3]
    auto setupSynthTab1 = [&](juce::TextButton& btn, juce::String text) {
        addAndMakeVisible (btn);
        btn.setButtonText (text);
        btn.setClickingTogglesState (true);
        btn.setRadioGroupId (1001);
        btn.setLookAndFeel (&chromaLookAndFeel);
    };
    setupSynthTab1 (v1AnalogBtn, "ANALOG");
    setupSynthTab1 (v1FmBtn, "FM");
    setupSynthTab1 (v1StringBtn, "STRING");
    setupSynthTab1 (v1PulseBtn, "PULSE");

    v1AnalogBtn.onClick = [this] { if (v1AnalogBtn.getToggleState()) processor.apvts.getParameter (IDs::voice1Synth.getParamID())->setValueNotifyingHost (0.0f); };
    v1FmBtn.onClick     = [this] { if (v1FmBtn.getToggleState())     processor.apvts.getParameter (IDs::voice1Synth.getParamID())->setValueNotifyingHost (1.0f / 3.0f); };
    v1StringBtn.onClick = [this] { if (v1StringBtn.getToggleState()) processor.apvts.getParameter (IDs::voice1Synth.getParamID())->setValueNotifyingHost (2.0f / 3.0f); };
    v1PulseBtn.onClick  = [this] { if (v1PulseBtn.getToggleState())  processor.apvts.getParameter (IDs::voice1Synth.getParamID())->setValueNotifyingHost (1.0f); };

    // Symmetrical Tab selection setup for Voice 2 (ANALOG, FM, STRING, PULSE) [3]
    auto setupSynthTab2 = [&](juce::TextButton& btn, juce::String text) {
        addAndMakeVisible (btn);
        btn.setButtonText (text);
        btn.setClickingTogglesState (true);
        btn.setRadioGroupId (1002);
        btn.setLookAndFeel (&chromaLookAndFeel);
    };
    setupSynthTab2 (v2AnalogBtn, "ANALOG");
    setupSynthTab2 (v2FmBtn, "FM");
    setupSynthTab2 (v2StringBtn, "STRING");
    setupSynthTab2 (v2PulseBtn, "PULSE");

    v2AnalogBtn.onClick = [this] { if (v2AnalogBtn.getToggleState()) processor.apvts.getParameter (IDs::voice2Synth.getParamID())->setValueNotifyingHost (0.0f); };
    v2FmBtn.onClick     = [this] { if (v2FmBtn.getToggleState())     processor.apvts.getParameter (IDs::voice2Synth.getParamID())->setValueNotifyingHost (1.0f / 3.0f); };
    v2StringBtn.onClick = [this] { if (v2StringBtn.getToggleState()) processor.apvts.getParameter (IDs::voice2Synth.getParamID())->setValueNotifyingHost (2.0f / 3.0f); };
    v2PulseBtn.onClick  = [this] { if (v2PulseBtn.getToggleState())  processor.apvts.getParameter (IDs::voice2Synth.getParamID())->setValueNotifyingHost (1.0f); };

    // Setup tactile ADSR envelope stage selector toggles [3]
    auto setupEnvBtn = [&](juce::TextButton& btn, juce::String text, int radioId) {
        addAndMakeVisible (btn);
        btn.setButtonText (text);
        btn.setClickingTogglesState (true);
        btn.setRadioGroupId (radioId);
        btn.setLookAndFeel (&chromaLookAndFeel);
    };
    setupEnvBtn (v1EnvA, "A", 2001);
    setupEnvBtn (v1EnvD, "D", 2001);
    setupEnvBtn (v1EnvS, "S", 2001);
    setupEnvBtn (v1EnvR, "R", 2001);

    v1EnvD.setToggleState (true, juce::dontSendNotification); // Default decay focus [3]
    v1EnvA.onClick = [this] { v1ActiveEnvStage = 0; };
    v1EnvD.onClick = [this] { v1ActiveEnvStage = 1; };
    v1EnvS.onClick = [this] { v1ActiveEnvStage = 2; };
    v1EnvR.onClick = [this] { v1ActiveEnvStage = 3; };

    setupEnvBtn (v2EnvA, "A", 2002);
    setupEnvBtn (v2EnvD, "D", 2002);
    setupEnvBtn (v2EnvS, "S", 2002);
    setupEnvBtn (v2EnvR, "R", 2002);

    v2EnvD.setToggleState (true, juce::dontSendNotification); // Default decay focus [3]
    v2EnvA.onClick = [this] { v2ActiveEnvStage = 0; };
    v2EnvD.onClick = [this] { v2ActiveEnvStage = 1; };
    v2EnvS.onClick = [this] { v2ActiveEnvStage = 2; };
    v2EnvR.onClick = [this] { v2ActiveEnvStage = 3; };

    // Slider Setup for Voice Envelopes and Volume gains [3]
    juce::Slider* sls[] = { &v1DecaySlider, &v1TimbreSlider, &v1ReverbSlider, &v2DecaySlider, &v2TimbreSlider, &v2ReverbSlider, &v1GainSlider, &v2GainSlider };
    for (auto* s : sls)
    {
        addAndMakeVisible (s);
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setLookAndFeel (&chromaLookAndFeel);
    }
    v1DecaySlider.setRange (0.005f, 3.0f);
    v2DecaySlider.setRange (0.005f, 3.0f);

    v1DecaySlider.onValueChange = [this] {
        float val = static_cast<float> (v1DecaySlider.getValue());
        if (v1ActiveEnvStage == 0)      processor.voice1.attack = val;
        else if (v1ActiveEnvStage == 1) processor.voice1.decay = val;
        else if (v1ActiveEnvStage == 2) processor.voice1.sustain = val;
        else if (v1ActiveEnvStage == 3) processor.voice1.release = val;
    };

    v2DecaySlider.onValueChange = [this] {
        float val = static_cast<float> (v2DecaySlider.getValue());
        if (v2ActiveEnvStage == 0)      processor.voice2.attack = val;
        else if (v2ActiveEnvStage == 1) processor.voice2.decay = val;
        else if (v2ActiveEnvStage == 2) processor.voice2.sustain = val;
        else if (v2ActiveEnvStage == 3) processor.voice2.release = val;
    };

    // Attachments Setup
    midiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::midiInChannel.getParamID(), midiInBox);
    midiOutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::midiOutChannel.getParamID(), midiOutBox);
    audioRoutingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::audioRouting.getParamID(), audioRoutingBox);

    v1TimbreAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Timbre.getParamID(), v1TimbreSlider);
    v1ReverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Reverb.getParamID(), v1ReverbSlider);
    v2DecayAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Decay.getParamID(), v2DecaySlider);
    v2TimbreAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Timbre.getParamID(), v2TimbreSlider);
    v2ReverbAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Reverb.getParamID(), v2ReverbSlider);
    v1GainAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Gain.getParamID(), v1GainSlider);
    v2GainAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Gain.getParamID(), v2GainSlider);

    updateSliderTextBoxThemeColors();

    // Setup collapsible Help Sidebar toggle button [3]
    addAndMakeVisible (helpButton);
    helpButton.setButtonText ("?");
    helpButton.setClickingTogglesState (true);
    helpButton.onClick = [this] { toggleHelpPanel(); };
    helpButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF181C20));
    helpButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFA0A5B0));
    helpButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    // Explicitly enable double-click to reset parameters to default values [1.2.1]
    rhythmMorphKnob.setDoubleClickReturnValue (true, 0.0f);
    restKnob.setDoubleClickReturnValue (true, 0.1f);
    legatoKnob.setDoubleClickReturnValue (true, 0.5f);
    rateKnob.setDoubleClickReturnValue (true, 0.5f); 
    entropyKnob.setDoubleClickReturnValue (true, 0.0f);
    harmonyKnob.setDoubleClickReturnValue (true, 0.0f);
    chaosKnob.setDoubleClickReturnValue (true, 0.0f);
    octavesKnob.setDoubleClickReturnValue (true, 0.0f);
    masterVelocityKnob.setDoubleClickReturnValue (true, 0.5f); 
    masterSwingKnob.setDoubleClickReturnValue (true, 0.0f); 

    for (int i = 0; i < 8; ++i) {
        faders[i]->setDoubleClickReturnValue (true, 0.5f); 
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
    setSize (1000, 681); 

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

    // Release programmatic update protection flag now that initialization and attachments are loaded
    getProperties().set ("isUpdatingProgrammatically", false);

    startTimerHz (30);
}

PluginEditor::~PluginEditor() 
{ 
    stopTimer(); processor.apvts.removeParameterListener ("panelTheme", this);
    juce::Slider* sliders[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &masterVelocityKnob, &masterSwingKnob, &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8, &morphCrossfader, &v1GainSlider, &v2GainSlider };
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
    
    // Explicitly sized array definition to bypass MSVC range-based template confusion [1.2.3]
    juce::TextButton* btns[14] = { &diceMeloButton, &diceArtiButton, &diceTimeButton, &diceNavyButton, &latchButton, &arpSeqButton, &polyButton, &freezeButton, &sceneAButton, &sceneBButton, &saveButton, &recallButton, &copyButton, &initButton };
    for (int i = 0; i < 14; ++i) { btns[i]->setLookAndFeel (nullptr); btns[i]->onClick = nullptr; }
    
    for (int i = 0; i < 8; ++i) { presetButtons[i].setLookAndFeel (nullptr); presetButtons[i].onClick = nullptr; presetButtons[i].onStateChange = nullptr; presetButtons[i].removeMouseListener(this); }
    sceneAButton.removeMouseListener (this); sceneBButton.removeMouseListener (this);

    // Left Panel component de-registrations [3]
    juce::ComboBox* boxes[] = { &midiInBox, &midiOutBox, &audioRoutingBox };
    for (auto* b : boxes) b->setLookAndFeel (nullptr);
    juce::Slider* sls[] = { &v1DecaySlider, &v1TimbreSlider, &v1ReverbSlider, &v2DecaySlider, &v2TimbreSlider, &v2ReverbSlider };
    for (auto* s : sls) s->setLookAndFeel (nullptr);

    // Dynamic clean up of property-wrapped syncButton [1.2.3]
    getProperties().remove ("syncWrapper");
}

void PluginEditor::toggleLeftPanel()
{
    isLeftPanelOpen = !isLeftPanelOpen;
    soundButton.setToggleState (isLeftPanelOpen, juce::dontSendNotification);
    
    // Set appropriate color theme highlights when active
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    juce::Colour activeColor = juce::Colour (0xFF00E5FF); 
    if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1); 
    else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66); 
    
    soundButton.setColour (juce::TextButton::buttonColourId, isLeftPanelOpen ? activeColor : juce::Colour (0xFF181C20));
    soundButton.setColour (juce::TextButton::textColourOffId, isLeftPanelOpen ? juce::Colours::black : juce::Colour (0xFFA0A5B0));
    soundButton.setColour (juce::TextButton::textColourOnId, isLeftPanelOpen ? juce::Colours::black : juce::Colours::white);

    // Dynamic symmetrical window width sizing [3]
    int targetW = 1000 + (isLeftPanelOpen ? 300 : 0) + (isHelpPanelOpen ? 300 : 0);
    setSize (targetW, 681);
}

void PluginEditor::toggleHelpPanel()
{
    isHelpPanelOpen = !isHelpPanelOpen;
    helpButton.setToggleState (isHelpPanelOpen, juce::dontSendNotification);
    
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    juce::Colour activeColor = juce::Colour (0xFF00E5FF); 
    if (themeIdx == 1)      activeColor = juce::Colour (0xFFECEFF1); 
    else if (themeIdx == 2) activeColor = juce::Colour (0xFF00FF66); 
    
    helpButton.setColour (juce::TextButton::buttonColourId, isHelpPanelOpen ? activeColor : juce::Colour (0xFF181C20));
    helpButton.setColour (juce::TextButton::textColourOffId, isHelpPanelOpen ? juce::Colours::black : juce::Colour (0xFFA0A5B0));
    helpButton.setColour (juce::TextButton::textColourOnId, isHelpPanelOpen ? juce::Colours::black : juce::Colours::white);

    // Dynamic symmetrical window width sizing [3]
    int targetW = 1000 + (isLeftPanelOpen ? 300 : 0) + (isHelpPanelOpen ? 300 : 0);
    setSize (targetW, 681);
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
    // Standalone MIDI CC Mapping & Learn Pill Clicks Interception [3]
    if (isHelpPanelOpen)
    {
        int xOffset = isLeftPanelOpen ? 300 : 0;
        auto mousePos = getMouseXYRelative();
        for (int i = 0; i < 18; ++i)
        {
            auto pillBounds = getCcPillBounds (i, xOffset);
            if (pillBounds.contains (mousePos))
            {
                if (processor.activeMidiLearnIndex.load() == i)
                    processor.activeMidiLearnIndex.store (-1);
                else
                    processor.activeMidiLearnIndex.store (i);
                
                repaint();
                return; // Intercept click entirely to bypass knob adjustments
            }
        }
    }

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
    int xOffset = isLeftPanelOpen ? 300 : 0;

    if (backgroundImage.isValid())
    {
        // Paint main background panel inside the left 1000px boundary offset [3]
        g.drawImage (backgroundImage, static_cast<float> (xOffset), 0.0f, 1000.0f, 681.0f, 
                     0.0f, 0.0f, static_cast<float> (backgroundImage.getWidth()), static_cast<float> (backgroundImage.getHeight()));
    }
    else
    {
        g.fillAll (juce::Colour (0xFF0D1E36));
    }

    // Select color routing matching active panel theme
    int themeIdx = static_cast<int> (processor.apvts.getRawParameterValue ("panelTheme")->load());
    juce::Colour themeColor = juce::Colour (0xFF00E5FF); 
    if (themeIdx == 1)      themeColor = juce::Colour (0xFFECEFF1); 
    else if (themeIdx == 2) themeColor = juce::Colour (0xFF00FF66); 

    // =====================================================================
    // SYMMETRICAL LEFT SIDEBAR DRAWING (SOUND CONTROL PANEL) [3]
    // =====================================================================
    if (isLeftPanelOpen)
    {
        int sidebarW = 300;
        g.setColour (juce::Colour (0xFF05070A)); // Match OLED black
        g.fillRect (0, 0, sidebarW, getHeight());

        g.setColour (themeColor.withAlpha (0.4f));
        g.drawVerticalLine (sidebarW, 0.0f, static_cast<float> (getHeight()));

        // Subtle blueprint guidelines behind control rows to enhance visual aesthetic
        g.setColour (themeColor.withAlpha (0.015f));
        for (int gx = 25; gx < sidebarW; gx += 25)
            g.drawVerticalLine (gx, 45.0f, static_cast<float> (getHeight() - 15));
        for (int gy = 55; gy < getHeight(); gy += 25)
            g.drawHorizontalLine (gy, 15.0f, static_cast<float> (sidebarW - 15));

        // Horizontal blueprint separator lines between sections
        g.setColour (juce::Colour (0xFF13171F));
        g.drawHorizontalLine (180, 15.0f, static_cast<float> (sidebarW - 15));
        g.drawHorizontalLine (380, 15.0f, static_cast<float> (sidebarW - 15));
        g.drawHorizontalLine (580, 15.0f, static_cast<float> (sidebarW - 15));

        // Document Title
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions ("Courier New", 14.0f, juce::Font::bold));
        g.drawText ("[ AUDIO & MIDI ROUTING ]", 15, 25, sidebarW - 30, 20, juce::Justification::centredLeft);

        // Define structural section details for the 4 island pills
        struct HelpSection {
            juce::String title;
            juce::Colour pillColor;
            int yPos;
        };

        std::vector<HelpSection> sections = {
            { "01 // SYSTEM MIDI ROUTING", juce::Colour (0xFF00E5FF), 65 },  
            { "02 // VOICE 1 (SCENE A TARGET)", juce::Colour (0xFFFF3366), 185 },  
            { "03 // VOICE 2 (SCENE B TARGET)", juce::Colour (0xFFD500F9), 385 },  
            { "04 // SYNTH AUDIO ROUTING", juce::Colour (0xFFFFB300), 585 }   
        };

        for (const auto& sec : sections)
        {
            int rx = 15;
            int ry = sec.yPos;
            int rw = sidebarW - 30; // 270px
            int rh = 22;

            // Render Rounded "Island Pill" Header container
            g.setColour (sec.pillColor.withAlpha (0.12f));
            g.fillRoundedRectangle (static_cast<float> (rx), static_cast<float> (ry), static_cast<float> (rw), static_cast<float> (rh), 4.0f);
            
            g.setColour (sec.pillColor.withAlpha (0.80f));
            g.drawRoundedRectangle (static_cast<float> (rx), static_cast<float> (ry), static_cast<float> (rw), static_cast<float> (rh), 4.0f, 1.0f);

            // Print monospace text inside pill (High contrast white)
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
            g.drawText (sec.title, rx + 10, ry, rw - 20, rh, juce::Justification::centredLeft);
        }

        // Render micro sliders parameter text tags on Eurorack single-line blueprint positions
        g.setColour (juce::Colour (0xFFA0A5B0));
        g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
        
        g.drawText ("MIDI IN:", 15, 95, 80, 16, juce::Justification::centredLeft);
        g.drawText ("MIDI OUT:", 15, 145, 80, 16, juce::Justification::centredLeft);

        g.drawText ("DECAY", 15, 272, 65, 16, juce::Justification::centredLeft);
        g.drawText ("TIMBRE", 15, 302, 65, 16, juce::Justification::centredLeft);
        g.drawText ("REVERB", 15, 332, 65, 16, juce::Justification::centredLeft);
        g.drawText ("VOL", 15, 362, 65, 16, juce::Justification::centredLeft);

        g.drawText ("DECAY", 15, 472, 65, 16, juce::Justification::centredLeft);
        g.drawText ("TIMBRE", 15, 502, 65, 16, juce::Justification::centredLeft);
        g.drawText ("REVERB", 15, 532, 65, 16, juce::Justification::centredLeft);
        g.drawText ("VOL", 15, 562, 65, 16, juce::Justification::centredLeft);

        g.drawText ("SIGNAL FLOW:", 15, 615, 150, 16, juce::Justification::centredLeft);

        // Fetch voice parameters to draw real-time interactive blueprint graphs
        float v1DecayVal = static_cast<float> (v1DecaySlider.getValue());
        float v1TimbreVal = static_cast<float> (v1TimbreSlider.getValue());
        float v1ReverbVal = static_cast<float> (v1ReverbSlider.getValue());
        
        float v2DecayVal = static_cast<float> (v2DecaySlider.getValue());
        float v2TimbreVal = static_cast<float> (v2TimbreSlider.getValue());
        float v2ReverbVal = static_cast<float> (v2ReverbSlider.getValue());

        // Contained Dark Glossy Screens behind each dynamic graph column [3]
        auto drawDisplayScreen = [&](int sx, int sy, int sw, int sh, juce::Colour col) {
            g.setColour (juce::Colour (0xFF020406));
            g.fillRoundedRectangle (static_cast<float> (sx), static_cast<float> (sy), static_cast<float> (sw), static_cast<float> (sh), 3.0f);
            g.setColour (col.withAlpha (0.15f));
            g.drawRoundedRectangle (static_cast<float> (sx), static_cast<float> (sy), static_cast<float> (sw), static_cast<float> (sh), 3.0f, 1.0f);
        };

        drawDisplayScreen (233, 270, 54, 20, juce::Colour (0xFFFF3366));
        drawDisplayScreen (233, 300, 54, 20, juce::Colour (0xFFFF3366));
        drawDisplayScreen (233, 330, 54, 20, juce::Colour (0xFFFF3366));

        drawDisplayScreen (233, 470, 54, 20, juce::Colour (0xFFD500F9));
        drawDisplayScreen (233, 500, 54, 20, juce::Colour (0xFFD500F9));
        drawDisplayScreen (233, 530, 54, 20, juce::Colour (0xFFD500F9));

        // Dynamic vector drawing lambdas to create glowing oscilloscopes
        auto drawDecayGraph = [&](int gx, int gy, float val, juce::Colour col) {
            juce::Path p;
            p.startNewSubPath (gx, gy + 13);
            p.lineTo (gx + 5, gy + 3);
            float decayX = gx + 5 + val * 35.0f;
            p.lineTo (decayX, gy + 3);
            p.lineTo (gx + 45, gy + 13);
            g.setColour (col.withAlpha (0.10f));
            g.fillPath (p);
            g.setColour (col.withAlpha (0.75f));
            g.strokePath (p, juce::PathStrokeType (1.25f));
        };

        auto drawTimbreGraph = [&](int gx, int gy, float val, juce::Colour col) {
            juce::Path p;
            p.startNewSubPath (gx, gy + 13);
            float cutoffX = gx + 5 + val * 30.0f;
            p.lineTo (cutoffX - 4, gy + 13);
            p.quadraticTo (cutoffX, gy + 2, cutoffX + 4, gy + 13); // resonance bump
            p.lineTo (gx + 45, gy + 13);
            g.setColour (col.withAlpha (0.10f));
            g.fillPath (p);
            g.setColour (col.withAlpha (0.75f));
            g.strokePath (p, juce::PathStrokeType (1.25f));
        };

        auto drawReverbGraph = [&](int gx, int gy, float val, juce::Colour col) {
            g.setColour (col.withAlpha (0.75f));
            int numTaps = 4 + static_cast<int> (val * 8.0f);
            for (int tap = 0; tap < numTaps; ++tap)
            {
                float progress = static_cast<float> (tap) / static_cast<float> (numTaps);
                float tx = gx + 2.0f + progress * 40.0f;
                float th = (10.0f - progress * 7.0f) * (0.3f + val * 0.7f);
                g.drawVerticalLine (static_cast<int> (tx), gy + 13.0f - th, gy + 13.0f);
            }
        };

        // Draw Interactive Blueprint Graphs (Symmetrically matched to voice header colors)
        int graphX = 235;
        drawDecayGraph  (graphX, 272, v1DecayVal, juce::Colour (0xFFFF3366)); // Voice 1 Pink
        drawTimbreGraph (graphX, 302, v1TimbreVal, juce::Colour (0xFFFF3366));
        drawReverbGraph (graphX, 332, v1ReverbVal, juce::Colour (0xFFFF3366));

        drawDecayGraph  (graphX, 472, v2DecayVal, juce::Colour (0xFFD500F9)); // Voice 2 Purple
        drawTimbreGraph (graphX, 502, v2TimbreVal, juce::Colour (0xFFD500F9));
        drawReverbGraph (graphX, 532, v2ReverbVal, juce::Colour (0xFFD500F9));
    }

    // =====================================================================
    // SYMMETRICAL RIGHT SIDEBAR DRAWING (QUICK MANUAL GUIDE) [3]
    // =====================================================================
    if (isHelpPanelOpen)
    {
        int sidebarX = xOffset + 1000;
        int sidebarW = 300;

        // Fill background matching high-tech OLED theme
        g.setColour (juce::Colour (0xFF05070A));
        g.fillRect (sidebarX, 0, sidebarW, getHeight());

        g.setColour (themeColor.withAlpha (0.4f));
        g.drawVerticalLine (sidebarX, 0.0f, static_cast<float> (getHeight()));

        // Document Title
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions ("Courier New", 14.0f, juce::Font::bold));
        g.drawText ("[ QUICK-START CHEAT SHEET ]", sidebarX + 15, 25, sidebarW - 30, 20, juce::Justification::centredLeft);

        // Section descriptors for the 4 island pills
        struct HelpSection {
            juce::String title;
            juce::String body;
            juce::Colour pillColor;
            int yPos;
        };

        std::vector<HelpSection> sections = {
            { 
                "01 // SNAPSHOT MORPHING", 
                "- Select snap [A] or [B] deck focus.\n- Tweaks/dice write only to active deck.\n- Drag CROSSFADER to morph the DSP.", 
                juce::Colour (0xFFFF3366), 
                60 
            },
            { 
                "02 // SATELLITE LFO ROUTING", 
                "- Right-click small dials for LFO setup.\n- Rate & depth show on backlit Corona.\n- Pulse = Speed; Brightness = LFO Depth.", 
                juce::Colour (0xFFD500F9), 
                205 
            },
            { 
                "03 // NOTE DENSITY (DEN)", 
                "- Global modifier on bottom-left.\n- Below 50%: reduces note probabilities.\n- Above 50%: fills empty steps.", 
                juce::Colour (0xFF00E5FF), 
                350 
            },
            { 
                "04 // PERFORMANCE PRESETS", 
                "- Click SAVE + slots 1-8 to record.\n- Click RECALL to latch preset surf.\n- Double-click dials to reset to center.", 
                juce::Colour (0xFFFFB300), 
                495 
            }
        };

        for (const auto& sec : sections)
        {
            int rx = sidebarX + 15;
            int ry = sec.yPos;
            int rw = sidebarW - 30; // 270px
            int rh = 22;

            // Render Rounded "Island Pill" Header container
            g.setColour (sec.pillColor.withAlpha (0.12f));
            g.fillRoundedRectangle (static_cast<float> (rx), static_cast<float> (ry), static_cast<float> (rw), static_cast<float> (rh), 4.0f);
            
            g.setColour (sec.pillColor.withAlpha (0.80f));
            g.drawRoundedRectangle (static_cast<float> (rx), static_cast<float> (ry), static_cast<float> (rw), static_cast<float> (rh), 4.0f, 1.0f);

            // Print monospace text inside pill (High contrast white)
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
            g.drawText (sec.title, rx + 10, ry, rw - 20, rh, juce::Justification::centredLeft);

            // Section body bulleted texts below pill
            g.setColour (juce::Colour (0xFFA0A5B0));
            g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::plain));
            g.drawFittedText (sec.body, rx + 5, ry + rh + 8, rw - 10, 100, juce::Justification::topLeft, 4);
        }
    }
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    int xOffset = isLeftPanelOpen ? 300 : 0;
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

    // =====================================================================
    // STANDALONE REALTIME FLOATING CC MAPPING LABELS OVERLAY [3]
    // =====================================================================
    if (isHelpPanelOpen)
    {
        for (int i = 0; i < 18; ++i)
        {
            auto pillBounds = getCcPillBounds (i, xOffset);
            int mappedCc = processor.midiCcMappings[i].load();
            bool isLearning = (processor.activeMidiLearnIndex.load() == i);

            // Black glossy backdrop capsule
            g.setColour (juce::Colour (0xFF05070A));
            g.fillRoundedRectangle (pillBounds.toFloat(), 3.0f);

            // Soft glow outline (Red on active MIDI Learn, Theme Accent on mapped)
            juce::Colour pillOutline = isLearning ? juce::Colours::red : themeColor.withAlpha (0.75f);
            g.setColour (pillOutline);
            g.drawRoundedRectangle (pillBounds.toFloat().reduced (0.5f), 3.0f, 1.0f);

            juce::String text = isLearning ? "LRN" : ((mappedCc >= 0) ? "C" + juce::String (mappedCc) : "MAP");
            g.setColour (isLearning ? juce::Colours::red : juce::Colours::white);
            g.setFont (juce::FontOptions ("Courier New", 9.0f, juce::Font::bold));
            g.drawText (text, pillBounds, juce::Justification::centred, false);
        }
    }

    // Custom Small HUD display boxes under the 8 small knobs [1.2.0]
    juce::Slider* smallKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::String smallLabels[] = { "MORPH", "REST", "LEGATO", "BPM", "ENTROPY", "HARMONY", "CHAOS", "OCTAVES" };
    int smallKnobsX[] = { 44, 44, 44, 44, 902, 902, 902, 902 };
    int smallKnobsY[] = { 121, 182, 244, 306, 121, 182, 244, 306 };

    for (int i = 0; i < 8; ++i)
    {
        int boxX = smallKnobsX[i] + xOffset;
        int boxY = smallKnobsY[i] + 48; 
        int boxW = 48;
        int boxH = 12;

        g.setColour (juce::Colour (0xFF05070A)); 
        g.fillRect (boxX, boxY, boxW, boxH);

        g.setColour (themeColor.withAlpha (0.75f));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawFittedText (smallLabels[i], boxX, boxY, boxW, boxH, juce::Justification::centred, 1);
    }

    // Custom Master HUD display boxes centered directly under the big knobs [1.2.0]
    // 1. Bottom-Left: Master Note Density (VEL) HUD Box
    juce::Rectangle<int> denBox (xOffset + 33, 484, 70, 15);
    g.setColour (juce::Colour (0xFF05070A));
    g.fillRoundedRectangle (denBox.toFloat(), 2.0f);
    g.setColour (themeColor.withAlpha (0.4f));
    g.drawRoundedRectangle (denBox.toFloat(), 2.0f, 1.0f);

    g.setColour (themeColor); 
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    juce::String denText = "DEN: " + juce::String (static_cast<int> (std::round (masterVelocityKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (denText, denBox, juce::Justification::centred, 1);

    // 2. Bottom-Right: Master Swing (SWG) HUD Box
    juce::Rectangle<int> swgBox (xOffset + 891, 484, 70, 15);
    g.setColour (juce::Colour (0xFF05070A));
    g.fillRoundedRectangle (swgBox.toFloat(), 2.0f);
    g.setColour (themeColor.withAlpha (0.4f));
    g.drawRoundedRectangle (swgBox.toFloat(), 2.0f, 1.0f);

    g.setColour (themeColor); 
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    juce::String swgText = "SWG: " + juce::String (static_cast<int> (std::round (masterSwingKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (swgText, swgBox, juce::Justification::centred, 1);

    // 3. Draw static, high-contrast white "MEMORY SLOTS" label spaced out under buttons 3 to 6 [1.2.3]
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
    g.drawText ("M  E  M  O  R  Y     S  L  O  T  S", xOffset + 326, 552, 344, 16, juce::Justification::centred);

    // 4. Draw static "NAVY-ARP MONITOR" stamped onto physical panel with enhanced presence [1.2.3]
    g.setColour (themeColor.withAlpha (0.85f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("NAVY-ARP MONITOR", xOffset + 157, 381, 680, 15, juce::Justification::centred, true);
}

void PluginEditor::mouseMove (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (DRAW_DIAGNOSTIC_GRID)
        repaint(); 
}

void PluginEditor::resized()
{
    int xOffset = isLeftPanelOpen ? 300 : 0;

    // Screen bounds (OLED)
    oledDisplay.setBounds (xOffset + 157, 57, 680, 320);

    // Left 2x2 Utility Grid (Save, Recall, Copy, Init)
    saveButton.setBounds (xOffset + 31, 49, 33, 28); 
    recallButton.setBounds (xOffset + 73, 49, 33, 28); 
    copyButton.setBounds (xOffset + 31, 83, 33, 28); 
    initButton.setBounds (xOffset + 73, 83, 33, 28);

    // Left Column small Knobs
    rhythmMorphKnob.setBounds (xOffset + 44, 121, 48, 48);
    restKnob.setBounds (xOffset + 44, 182, 48, 48);
    legatoKnob.setBounds (xOffset + 44, 244, 48, 48);
    rateKnob.setBounds (xOffset + 44, 306, 48, 48);
    
    // Left Master Velocity Knob
    masterVelocityKnob.setBounds (xOffset + 26, 396, 84, 86);

    // Right 2x2 Utility Grid (Melo, Arti, Time, Navy)
    diceMeloButton.setBounds (xOffset + 888, 48, 33, 28); 
    diceArtiButton.setBounds (xOffset + 930, 48, 33, 28); 
    diceTimeButton.setBounds (xOffset + 888, 84, 33, 28); 
    diceNavyButton.setBounds (xOffset + 930, 84, 33, 28);

    // Right Column small Knobs
    entropyKnob.setBounds (xOffset + 902, 121, 48, 48);
    harmonyKnob.setBounds (xOffset + 902, 182, 48, 48);
    chaosKnob.setBounds (xOffset + 902, 244, 48, 48);
    octavesKnob.setBounds (xOffset + 902, 306, 48, 48);
    
    // Right Master Swing Knob
    masterSwingKnob.setBounds (xOffset + 884, 396, 84, 86);

    // Top Dropdowns (Left) - Expanded width by 12px to prevent clipping / truncation [3]
    rootKeyBox.setBounds (xOffset + 163, 17, 62, 17); 
    scaleTypeBox.setBounds (xOffset + 233, 17, 65, 17); 
    cycleLengthBox.setBounds (xOffset + 304, 17, 65, 17);
    panelThemeBox.setBounds (xOffset + 374, 17, 72, 17); 
    
    // Top Row Performance Buttons (Right) [1.2.3]
    latchButton.setBounds (xOffset + 533, 15, 58, 17); 
    arpSeqButton.setBounds (xOffset + 602, 15, 58, 17); 
    polyButton.setBounds (xOffset + 669, 15, 58, 17); 
    freezeButton.setBounds (xOffset + 738, 15, 58, 17);
    
    // Symmetrical "?" and "SND" toggler icons stay fixed and shift with main panel [3]
    helpButton.setBounds (xOffset + 965, 15, 18, 18);
    soundButton.setBounds (xOffset + 15, 15, 30, 18); // Symmetrically locked to main panel origin [3]

    // Safety boundaries set dynamically using property fetches for wrapped syncButton [1.2.3]
    auto syncWrapper = SyncButtonWrapper::Ptr (dynamic_cast<SyncButtonWrapper*> (getProperties()["syncWrapper"].getObject()));
    if (syncWrapper != nullptr)
    {
        syncWrapper->button.setBounds (xOffset + 807, 15, 17, 17); // Sited nicely after freeze [1.2.3]
    }

    // Crossfader Row
    sceneAButton.setBounds (xOffset + 214, 396, 67, 58);
    morphCrossfader.setBounds (xOffset + 336, 415, 321, 28);
    sceneBButton.setBounds (xOffset + 714, 396, 67, 58);

    // Invisible Preset Memory Slots (1-8)
    const float presetX[8] = { 145.0f, 236.0f, 326.0f, 415.0f, 504.0f, 594.0f, 682.0f, 772.0f };
    for (int i = 0; i < 8; ++i) 
    {
        presetButtons[i].setBounds (static_cast<int>(presetX[i] + xOffset), 477, 76, 65);
    }

    // Vertical Upfaders (1-8)
    const float faderX[8] = { 66.0f, 192.0f, 314.0f, 436.0f, 556.0f, 678.0f, 802.0f, 923.0f };
    juce::Slider* faderPtrs[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    for (int i = 0; i < 8; ++i) 
    {
        faderPtrs[i]->setBounds (static_cast<int>(faderX[i] + xOffset) - 6, 583, 24, 60);
    }

    // =====================================================================
    // SYMMETRICAL LEFT WING CONTROL BOUNDS POSITIONING [3]
    // =====================================================================
    if (isLeftPanelOpen)
    {
        midiInBox.setBounds (85, 95, 200, 20);
        midiOutBox.setBounds (85, 135, 200, 20);

        // Voice 1 Symmetrical tactile instrument tabs [3]
        v1AnalogBtn.setBounds (15, 215, 63, 18);
        v1FmBtn.setBounds (81, 215, 63, 18);
        v1StringBtn.setBounds (147, 215, 63, 18);
        v1PulseBtn.setBounds (213, 215, 63, 18);

        // Voice 1 single-line compact Euro-rack layout [3]
        v1EnvA.setBounds (45, 272, 16, 16);
        v1EnvD.setBounds (63, 272, 16, 16);
        v1EnvS.setBounds (81, 272, 16, 16);
        v1EnvR.setBounds (99, 272, 16, 16);
        v1DecaySlider.setBounds (120, 272, 105, 16);

        v1TimbreSlider.setBounds (85, 302, 140, 16);
        v1ReverbSlider.setBounds (85, 332, 140, 16);
        v1GainSlider.setBounds (85, 362, 140, 16);

        // Voice 2 Symmetrical tactile instrument tabs [3]
        v2AnalogBtn.setBounds (15, 415, 63, 18);
        v2FmBtn.setBounds (81, 415, 63, 18);
        v2StringBtn.setBounds (147, 415, 63, 18);
        v2PulseBtn.setBounds (213, 415, 63, 18);

        // Voice 2 single-line compact Euro-rack layout [3]
        v2EnvA.setBounds (45, 472, 16, 16);
        v2EnvD.setBounds (63, 472, 16, 16);
        v2EnvS.setBounds (81, 472, 16, 16);
        v2EnvR.setBounds (99, 472, 16, 16);
        v2DecaySlider.setBounds (120, 472, 105, 16);

        v2TimbreSlider.setBounds (85, 502, 140, 16);
        v2ReverbSlider.setBounds (85, 532, 140, 16);
        v2GainSlider.setBounds (85, 562, 140, 16);

        audioRoutingBox.setBounds (85, 615, 200, 20);
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
        
        juce::Colour borderCol = currentFreeze ? juce::Colour (0xFF00E5FF) : t.slotOutline;
        juce::Colour textCol = currentFreeze ? juce::Colour (0xFF00E5FF) : t.textDim;
        
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

    // Motorized Console Visual Fader Physics: Update knobs only if not actively grabbed by mouse pointer [1.2.0]
    if (rhythmMorphKnob.getThumbBeingDragged() < 0)
        rhythmMorphKnob.setValue (interpolate (processor.sceneA.rhythmMorph, processor.sceneB.rhythmMorph), juce::dontSendNotification);
        
    if (restKnob.getThumbBeingDragged() < 0)
        restKnob.setValue (interpolate (processor.sceneA.rest, processor.sceneB.rest), juce::dontSendNotification);
        
    if (legatoKnob.getThumbBeingDragged() < 0)
        legatoKnob.setValue (interpolate (processor.sceneA.legato, processor.sceneB.legato), juce::dontSendNotification);
        
    if (rateKnob.getThumbBeingDragged() < 0)
        rateKnob.setValue (interpolate (processor.sceneA.rate, processor.sceneB.rate), juce::dontSendNotification);
        
    if (entropyKnob.getThumbBeingDragged() < 0)
        entropyKnob.setValue (interpolate (processor.sceneA.entropy, processor.sceneB.entropy), juce::dontSendNotification);
        
    if (harmonyKnob.getThumbBeingDragged() < 0)
        harmonyKnob.setValue (interpolate (processor.sceneA.harmony, processor.sceneB.harmony), juce::dontSendNotification);
        
    if (chaosKnob.getThumbBeingDragged() < 0)
        chaosKnob.setValue (interpolate (processor.sceneA.chaos, processor.sceneB.chaos), juce::dontSendNotification);
        
    if (octavesKnob.getThumbBeingDragged() < 0)
        octavesKnob.setValue (interpolate (processor.sceneA.octaves, processor.sceneB.octaves), juce::dontSendNotification);

    // Motorized Console Visual Fader Physics: Update upfaders only if not actively grabbed by mouse pointer [1.2.0]
    juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
    for (int i = 0; i < 8; ++i) {
        if (faders[i]->getThumbBeingDragged() < 0)
        {
            faders[i]->setValue (interpolate (processor.sceneA.faders[i], processor.sceneB.faders[i]), juce::dontSendNotification);
        }
    }

    // Synchronize current Synthesis Button Tab Highlight states [3]
    if (isLeftPanelOpen)
    {
        int v1SynthVal = static_cast<int> (std::round (processor.voice1SynthPtr->load()));
        v1AnalogBtn.setToggleState (v1SynthVal == 0, juce::dontSendNotification);
        v1FmBtn.setToggleState (v1SynthVal == 1, juce::dontSendNotification);
        v1StringBtn.setToggleState (v1SynthVal == 2, juce::dontSendNotification);
        v1PulseBtn.setToggleState (v1SynthVal == 3, juce::dontSendNotification);

        int v2SynthVal = static_cast<int> (std::round (processor.voice2SynthPtr->load()));
        v2AnalogBtn.setToggleState (v2SynthVal == 0, juce::dontSendNotification);
        v2FmBtn.setToggleState (v2SynthVal == 1, juce::dontSendNotification);
        v2StringBtn.setToggleState (v2SynthVal == 2, juce::dontSendNotification);
        v2PulseBtn.setToggleState (v2SynthVal == 3, juce::dontSendNotification);

        // Symmetrical ADSR Stage Select Highlight Synchronization [3]
        v1EnvA.setToggleState (v1ActiveEnvStage == 0, juce::dontSendNotification);
        v1EnvD.setToggleState (v1ActiveEnvStage == 1, juce::dontSendNotification);
        v1EnvS.setToggleState (v1ActiveEnvStage == 2, juce::dontSendNotification);
        v1EnvR.setToggleState (v1ActiveEnvStage == 3, juce::dontSendNotification);

        v2EnvA.setToggleState (v2ActiveEnvStage == 0, juce::dontSendNotification);
        v2EnvD.setToggleState (v2ActiveEnvStage == 1, juce::dontSendNotification);
        v2EnvS.setToggleState (v2ActiveEnvStage == 2, juce::dontSendNotification);
        v2EnvR.setToggleState (v2ActiveEnvStage == 3, juce::dontSendNotification);

        // Dynamically adjust slider ranges based on active sub-stage focus
        if (v1ActiveEnvStage == 2) {
            if (v1DecaySlider.getMaximum() != 1.0f) v1DecaySlider.setRange (0.0f, 1.0f);
        } else {
            if (v1DecaySlider.getMaximum() != 3.0f) v1DecaySlider.setRange (0.005f, 3.0f);
        }

        if (v2ActiveEnvStage == 2) {
            if (v2DecaySlider.getMaximum() != 1.0f) v2DecaySlider.setRange (0.0f, 1.0f);
        } else {
            if (v2DecaySlider.getMaximum() != 3.0f) v2DecaySlider.setRange (0.005f, 3.0f);
        }

        // Read active ADSR variables when sliders are not being grabbed
        if (v1DecaySlider.getThumbBeingDragged() < 0)
        {
            float targetVal = 0.0f;
            if (v1ActiveEnvStage == 0)      targetVal = processor.voice1.attack;
            else if (v1ActiveEnvStage == 1) targetVal = processor.voice1.decay;
            else if (v1ActiveEnvStage == 2) targetVal = processor.voice1.sustain;
            else if (v1ActiveEnvStage == 3) targetVal = processor.voice1.release;
            v1DecaySlider.setValue (targetVal, juce::dontSendNotification);
        }

        if (v2DecaySlider.getThumbBeingDragged() < 0)
        {
            float targetVal = 0.0f;
            if (v2ActiveEnvStage == 0)      targetVal = processor.voice2.attack;
            else if (v2ActiveEnvStage == 1) targetVal = processor.voice2.decay;
            else if (v2ActiveEnvStage == 2) targetVal = processor.voice2.sustain;
            else if (v2ActiveEnvStage == 3) targetVal = processor.voice2.release;
            v2DecaySlider.setValue (targetVal, juce::dontSendNotification);
        }
    }

    getProperties().set ("isUpdatingProgrammatically", false);

    // OLED Parameter HUD Overlay Triggering with Standard 3-Argument Signature [1.2.3]
    juce::Slider* smallKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob };
    juce::String smallNames[] = { "Rhythm Morph", "Rest", "Legato", "BPM", "Entropy", "Harmony", "Chaos", "Octaves" };
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

    if (masterVelocityKnob.getThumbBeingDragged() >= 0)
    {
        oledDisplay.showParameterOverlay ("Note Density", static_cast<float> (masterVelocityKnob.getValue()), "Off");
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

    // Force parent repaint of visual layers
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