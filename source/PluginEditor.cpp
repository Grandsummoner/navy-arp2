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
    juce::ComboBox* bxs[] = { &midiInBox, &midiOutBox1, &midiOutBox2, &audioRoutingBox };
    for (auto* b : bxs)
    {
        addAndMakeVisible (b);
        b->setLookAndFeel (&chromaLookAndFeel);
    }
    midiInBox.addItemList (juce::StringArray { "Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
    midiOutBox1.addItemList (juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
    midiOutBox2.addItemList (juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" }, 1);
    audioRoutingBox.addItemList (juce::StringArray { "Split A->1 / B->2", "Layered (Voice 1)", "External Out Only" }, 1);

    // Symmetrical Tab selection setup for Voice 1 (Multi-select layout) [3]
    auto setupSynthTab1 = [&](juce::TextButton& btn, juce::String text) {
        addAndMakeVisible (btn);
        btn.setButtonText (text);
        btn.setClickingTogglesState (true);
        btn.setLookAndFeel (&chromaLookAndFeel);
    };
    setupSynthTab1 (v1AnalogBtn, "Analog");
    setupSynthTab1 (v1FmBtn, "FM");
    setupSynthTab1 (v1StringBtn, "String");
    setupSynthTab1 (v1PulseBtn, "Pulse");

    v1AnalogAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice1Analog.getParamID(), v1AnalogBtn);
    v1FmAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice1Fm.getParamID(), v1FmBtn);
    v1StringAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice1String.getParamID(), v1StringBtn);
    v1PulseAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice1Pulse.getParamID(), v1PulseBtn);

    // Symmetrical Tab selection setup for Voice 2 (Multi-select layout) [3]
    auto setupSynthTab2 = [&](juce::TextButton& btn, juce::String text) {
        addAndMakeVisible (btn);
        btn.setButtonText (text);
        btn.setClickingTogglesState (true);
        btn.setLookAndFeel (&chromaLookAndFeel);
    };
    setupSynthTab2 (v2AnalogBtn, "Analog");
    setupSynthTab2 (v2FmBtn, "FM");
    setupSynthTab2 (v2StringBtn, "String");
    setupSynthTab2 (v2PulseBtn, "Pulse");

    v2AnalogAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice2Analog.getParamID(), v2AnalogBtn);
    v2FmAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice2Fm.getParamID(), v2FmBtn);
    v2StringAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice2String.getParamID(), v2StringBtn);
    v2PulseAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::voice2Pulse.getParamID(), v2PulseBtn);

    // Symmetrical Rotary Knob Setup for Voice Envelopes, Timbre, Delay, Reverb and Volumes (No more faders) [3]
    juce::Slider* sls[] = {
        &v1AttackKnob, &v1DecayKnob, &v1SustainKnob, &v1ReleaseKnob, &v1TimbreKnob, &v1DelayKnob, &v1ReverbKnob, &v1VolumeKnob,
        &v2AttackKnob, &v2DecayKnob, &v2SustainKnob, &v2ReleaseKnob, &v2TimbreKnob, &v2DelayKnob, &v2ReverbKnob, &v2VolumeKnob
    };
    juce::String sliderIDs[] = {
        "v1Attack", "v1Decay", "v1Sustain", "v1Release", "v1Timbre", "v1Delay", "v1Reverb", "v1Volume",
        "v2Attack", "v2Decay", "v2Sustain", "v2Release", "v2Timbre", "v2Delay", "v2Reverb", "v2Volume"
    };

    for (int i = 0; i < 16; ++i)
    {
        addAndMakeVisible (sls[i]);
        sls[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        sls[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sls[i]->setLookAndFeel (&chromaLookAndFeel);
        sls[i]->setComponentID (sliderIDs[i]);
        sls[i]->addMouseListener (this, false);
    }

    // Attachments Setup for 16 individual Symmetrical Voice parameters [3]
    midiInAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::midiInChannel.getParamID(), midiInBox);
    midiOutAttachment1 = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::midiOutChannel1.getParamID(), midiOutBox1);
    midiOutAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::midiOutChannel2.getParamID(), midiOutBox2);
    audioRoutingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, IDs::audioRouting.getParamID(), audioRoutingBox);

    v1AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Attack.getParamID(), v1AttackKnob);
    v1DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Decay.getParamID(), v1DecayKnob);
    v1SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Sustain.getParamID(), v1SustainKnob);
    v1ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Release.getParamID(), v1ReleaseKnob);
    v1TimbreAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Timbre.getParamID(), v1TimbreKnob);
    v1DelayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Delay.getParamID(), v1DelayKnob);
    v1ReverbAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Reverb.getParamID(), v1ReverbKnob);
    v1VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice1Gain.getParamID(), v1VolumeKnob);

    v2AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Attack.getParamID(), v2AttackKnob);
    v2DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Decay.getParamID(), v2DecayKnob);
    v2SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Sustain.getParamID(), v2SustainKnob);
    v2ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Release.getParamID(), v2ReleaseKnob);
    v2TimbreAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Timbre.getParamID(), v2TimbreKnob);
    v2DelayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Delay.getParamID(), v2DelayKnob);
    v2ReverbAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Reverb.getParamID(), v2ReverbKnob);
    v2VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::voice2Gain.getParamID(), v2VolumeKnob);

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
        &masterVelocityKnob, &masterSwingKnob, &morphCrossfader,
        &v1AttackKnob, &v1DecayKnob, &v1SustainKnob, &v1ReleaseKnob, &v1TimbreKnob, &v1DelayKnob, &v1ReverbKnob, &v1VolumeKnob,
        &v2AttackKnob, &v2DecayKnob, &v2SustainKnob, &v2ReleaseKnob, &v2TimbreKnob, &v2DelayKnob, &v2ReverbKnob, &v2VolumeKnob
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
    juce::Slider* sliders[] = { 
        &rhythmMorphKnob, &restKnob, &legatoKnob, &rateKnob, &entropyKnob, &harmonyKnob, &chaosKnob, &octavesKnob, &masterVelocityKnob, &masterSwingKnob, &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8, &morphCrossfader,
        &v1AttackKnob, &v1DecayKnob, &v1SustainKnob, &v1ReleaseKnob, &v1TimbreKnob, &v1DelayKnob, &v1ReverbKnob, &v1VolumeKnob,
        &v2AttackKnob, &v2DecayKnob, &v2SustainKnob, &v2ReleaseKnob, &v2TimbreKnob, &v2DelayKnob, &v2ReverbKnob, &v2VolumeKnob
    };
    for (auto* s : sliders) s->setLookAndFeel (nullptr);
    
    // Explicitly sized array definition to bypass MSVC range-based template confusion [1.2.3]
    juce::TextButton* btns[14] = { &diceMeloButton, &diceArtiButton, &diceTimeButton, &diceNavyButton, &latchButton, &arpSeqButton, &polyButton, &freezeButton, &sceneAButton, &sceneBButton, &saveButton, &recallButton, &copyButton, &initButton };
    for (int i = 0; i < 14; ++i) { btns[i]->setLookAndFeel (nullptr); btns[i]->onClick = nullptr; }
    
    for (int i = 0; i < 8; ++i) { presetButtons[i].setLookAndFeel (nullptr); presetButtons[i].onClick = nullptr; presetButtons[i].onStateChange = nullptr; presetButtons[i].removeMouseListener(this); }
    sceneAButton.removeMouseListener (this); sceneBButton.removeMouseListener (this);

    // Left Panel component de-registrations [3]
    juce::ComboBox* boxes[] = { &midiInBox, &midiOutBox1, &midiOutBox2, &audioRoutingBox };
    for (auto* b : boxes) b->setLookAndFeel (nullptr);

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

    // Standardised proportional typeface name mapped to Ableton styles [3]
    juce::String fontName = juce::Font::getDefaultSansSerifFontName();

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

        // Document Title - Standardised clean sentence case sans-serif typography [3]
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
        g.drawText ("Audio & MIDI routing", 15, 25, sidebarW - 30, 20, juce::Justification::centredLeft);

        // Define structural section details for the 4 island pills
        struct HelpSection {
            juce::String title;
            juce::Colour pillColor;
            int yPos;
        };

        std::vector<HelpSection> sections = {
            { "01 // System MIDI routing", juce::Colour (0xFF00E5FF), 65 },  
            { "02 // Voice 1 (Scene A target)", juce::Colour (0xFFFF3366), 185 },  
            { "03 // Voice 2 (Scene B target)", juce::Colour (0xFFD500F9), 385 },  
            { "04 // Synth audio routing", juce::Colour (0xFFFFB300), 585 }   
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
            g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
            g.drawText (sec.title, rx + 10, ry, rw - 20, rh, juce::Justification::centredLeft);
        }

        // Render micro sliders parameter text tags on Eurorack single-line blueprint positions
        g.setColour (juce::Colour (0xFFA0A5B0));
        g.setFont (juce::Font (fontName, 10.0f, juce::Font::bold));
        
        g.drawText ("MIDI In:", 15, 95, 80, 16, juce::Justification::centredLeft);
        g.drawText ("MIDI Out 1:", 15, 125, 80, 16, juce::Justification::centredLeft);
        g.drawText ("MIDI Out 2:", 15, 155, 80, 16, juce::Justification::centredLeft);

        g.drawText ("Signal flow:", 15, 615, 150, 16, juce::Justification::centredLeft);

        // Fetch ADSR values dynamically [3]
        float v1A = processor.voice1AttackPtr->load();
        float v1D = processor.voice1DecayPtr->load();
        float v1S = processor.voice1SustainPtr->load();
        float v1R = processor.voice1ReleasePtr->load();

        float v2A = processor.voice2AttackPtr->load();
        float v2D = processor.voice2DecayPtr->load();
        float v2S = processor.voice2SustainPtr->load();
        float v2R = processor.voice2ReleasePtr->load();

        // Contained Dark Glossy Screens behind each dynamic ADSR visualizer [3]
        auto drawDisplayScreen = [&](int sx, int sy, int sw, int sh, juce::Colour col) {
            g.setColour (juce::Colour (0xFF020406));
            g.fillRoundedRectangle (static_cast<float> (sx), static_cast<float> (sy), static_cast<float> (sw), static_cast<float> (sh), 3.0f);
            g.setColour (col.withAlpha (0.15f));
            g.drawRoundedRectangle (static_cast<float> (sx), static_cast<float> (sy), static_cast<float> (sw), static_cast<float> (sh), 3.0f, 1.0f);
        };

        drawDisplayScreen (15, 240, 270, 42, juce::Colour (0xFFFF3366));
        drawDisplayScreen (15, 440, 270, 42, juce::Colour (0xFFD500F9));

        // Draw Interactive Blueprint ADSR Graphs contained inside the screen box [3]
        auto drawAdsrGraph = [&](int sx, int sy, int sw, int sh, float a, float d, float s, float r, juce::Colour col) {
            // Background subtle gridlines inside screen
            g.setColour (col.withAlpha (0.04f));
            for (int gx = sx + 10; gx < sx + sw; gx += 15)
                g.drawVerticalLine (gx, sy + 1, sy + sh - 1);
            for (int gy = sy + 5; gy < sy + sh; gy += 8)
                g.drawHorizontalLine (gy, sx + 1, sx + sw - 1);

            juce::Path p;
            float total = std::max (0.1f, a + d + r);
            
            float startX = sx + 8.0f;
            float endX = sx + sw - 8.0f;
            float graphW = endX - startX;
            
            // Linear scale ADSR parameters dynamically to look like real-time screen [3]
            float attackX = startX + (a / 2.0f) * (graphW * 0.22f);
            float decayX = attackX + (d / 3.0f) * (graphW * 0.22f);
            float sustainX = decayX + (graphW * 0.30f);
            float releaseX = sustainX + (r / 3.0f) * (graphW * 0.22f);

            float bottomY = sy + sh - 3.0f;
            float topY = sy + 3.0f;
            float sustainY = bottomY - s * (bottomY - topY);

            p.startNewSubPath (startX, bottomY);
            p.lineTo (attackX, topY);
            p.lineTo (decayX, sustainY);
            p.lineTo (sustainX, sustainY);
            p.lineTo (releaseX, bottomY);

            g.setColour (col.withAlpha (0.08f));
            g.fillPath (p);

            g.setColour (col.withAlpha (0.85f));
            g.strokePath (p, juce::PathStrokeType (1.50f));
        };

        drawAdsrGraph (15, 240, 270, 42, v1A, v1D, v1S, v1R, juce::Colour (0xFFFF3366)); // Voice 1 Pink
        drawAdsrGraph (15, 440, 270, 42, v2A, v2D, v2S, v2R, juce::Colour (0xFFD500F9)); // Voice 2 Purple

        // Render micro labels above rotary knobs (Attack, Decay, Sustain, Release, Timbre, Delay, Reverb, Volume)
        g.setColour (juce::Colour (0xFFA0A5B0));
        g.setFont (juce::Font (fontName, 10.0f, juce::Font::bold));

        // Voice 1 text labels centered above the pots
        g.drawText ("A",    15,  285, 42, 10, juce::Justification::centred);
        g.drawText ("D",    80,  285, 42, 10, juce::Justification::centred);
        g.drawText ("S",    145, 285, 42, 10, juce::Justification::centred);
        g.drawText ("R",    210, 285, 42, 10, juce::Justification::centred);
        g.drawText ("Timb", 15,  335, 42, 10, juce::Justification::centred);
        g.drawText ("Delay", 80,  335, 42, 10, juce::Justification::centred);
        g.drawText ("Revb", 145, 335, 42, 10, juce::Justification::centred);
        g.drawText ("Vol",  210, 335, 42, 10, juce::Justification::centred);

        // Voice 2 text labels centered above the pots
        g.drawText ("A",    15,  485, 42, 10, juce::Justification::centred);
        g.drawText ("D",    80,  485, 42, 10, juce::Justification::centred);
        g.drawText ("S",    145, 485, 42, 10, juce::Justification::centred);
        g.drawText ("R",    210, 485, 42, 10, juce::Justification::centred);
        g.drawText ("Timb", 15,  535, 42, 10, juce::Justification::centred);
        g.drawText ("Delay", 80,  535, 42, 10, juce::Justification::centred);
        g.drawText ("Revb", 145, 535, 42, 10, juce::Justification::centred);
        g.drawText ("Vol",  210, 535, 42, 10, juce::Justification::centred);
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
        g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
        g.drawText ("Quick-start cheat sheet", sidebarX + 15, 25, sidebarW - 30, 20, juce::Justification::centredLeft);

        // Section descriptors for the 4 island pills
        struct HelpSection {
            juce::String title;
            juce::String body;
            juce::Colour pillColor;
            int yPos;
        };

        std::vector<HelpSection> sections = {
            { 
                "01 // Snapshot morphing", 
                "- Select snap [A] or [B] deck focus.\n- Tweaks/dice write only to active deck.\n- Drag CROSSFADER to morph the DSP.", 
                juce::Colour (0xFFFF3366), 
                60 
            },
            { 
                "02 // Satellite LFO routing", 
                "- Right-click small dials for LFO setup.\n- Rate & depth show on backlit Corona.\n- Pulse = Speed; Brightness = LFO Depth.", 
                juce::Colour (0xFFD500F9), 
                205 
            },
            { 
                "03 // Note density (DEN)", 
                "- Global modifier on bottom-left.\n- Below 50%: reduces note probabilities.\n- Above 50%: fills empty steps.", 
                juce::Colour (0xFF00E5FF), 
                350 
            },
            { 
                "04 // Performance presets", 
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
            g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
            g.drawText (sec.title, rx + 10, ry, rw - 20, rh, juce::Justification::centredLeft);

            // Section body bulleted texts below pill
            g.setColour (juce::Colour (0xFFA0A5B0));
            g.setFont (juce::Font (fontName, 10.0f, juce::Font::plain));
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

    juce::String fontName = juce::Font::getDefaultSansSerifFontName();

    if (DRAW_DIAGNOSTIC_GRID)
    {
        const int width = getWidth();
        const int height = getHeight();

        for (int x = 50; x < width; x += 50)
        {
            g.setColour (juce::Colour (0x4400E1FF));
            g.drawVerticalLine (x, 0.0f, static_cast<float> (height));
            
            g.setColour (juce::Colour (0xBB00E1FF));
            g.setFont (juce::Font (fontName, 9.0f, juce::Font::plain));
            g.drawText (juce::String (x), x - 12, height - 12, 24, 10, juce::Justification::centred);
        }

        for (int y = 50; y < height; y += 50)
        {
            g.setColour (juce::Colour (0x44FF3366));
            g.drawHorizontalLine (y, 0.0f, static_cast<float> (width));
            
            g.setColour (juce::Colour (0xBBFF3366));
            g.setFont (juce::Font (fontName, 9.0f, juce::Font::plain));
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
            g.setFont (juce::Font (fontName, 10.0f, juce::Font::bold));
            
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
            g.setFont (juce::Font (fontName, 8.5f, juce::Font::bold)); // Symmetrical small readout [3.0.1]
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
        g.setFont (juce::Font (fontName, 8.5f, juce::Font::bold));
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
    g.setFont (juce::Font (fontName, 9.5f, juce::Font::bold));
    juce::String denText = "DEN: " + juce::String (static_cast<int> (std::round (masterVelocityKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (denText, denBox, juce::Justification::centred, 1);

    // 2. Bottom-Right: Master Swing (SWG) HUD Box
    juce::Rectangle<int> swgBox (xOffset + 891, 484, 70, 15);
    g.setColour (juce::Colour (0xFF05070A));
    g.fillRoundedRectangle (swgBox.toFloat(), 2.0f);
    g.setColour (themeColor.withAlpha (0.4f));
    g.drawRoundedRectangle (swgBox.toFloat(), 2.0f, 1.0f);

    g.setColour (themeColor); 
    g.setFont (juce::Font (fontName, 9.5f, juce::Font::bold));
    juce::String swgText = "SWG: " + juce::String (static_cast<int> (std::round (masterSwingKnob.getValue() * 100.0f))) + "%";
    g.drawFittedText (swgText, swgBox, juce::Justification::centred, 1);

    // 3. Draw static, high-contrast white "MEMORY SLOTS" label spaced out under buttons 3 to 6 [1.2.3]
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
    g.drawText ("Memory slots", xOffset + 326, 552, 344, 16, juce::Justification::centred);

    // 4. Draw static "NAVY-ARP MONITOR" stamped onto physical panel with enhanced presence [1.2.3]
    g.setColour (themeColor.withAlpha (0.85f));
    g.setFont (juce::Font (fontName, 12.0f, juce::Font::bold));
    g.drawText ("Navy-arp monitor", xOffset + 157, 381, 680, 15, juce::Justification::centred, true);
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
        midiOutBox1.setBounds (85, 125, 200, 20); // Voice 1 MIDI [3]
        midiOutBox2.setBounds (85, 155, 200, 20); // Voice 2 MIDI [3]

        // Voice 1 Symmetrical layerable tactile instrument tabs [3]
        v1AnalogBtn.setBounds (15, 215, 63, 18);
        v1FmBtn.setBounds (81, 215, 63, 18);
        v1StringBtn.setBounds (147, 215, 63, 18);
        v1PulseBtn.setBounds (213, 215, 63, 18);

        // Voice 1 tactile independent ADSR / Timbre / Delay / Reverb / Volume knobs (Rotaries!) [3]
        v1AttackKnob.setBounds  (15,  295, 42, 42);
        v1DecayKnob.setBounds   (80,  295, 42, 42);
        v1SustainKnob.setBounds (145, 295, 42, 42);
        v1ReleaseKnob.setBounds (210, 295, 42, 42);

        v1TimbreKnob.setBounds  (15,  345, 42, 42);
        v1DelayKnob.setBounds   (80,  345, 42, 42);
        v1ReverbKnob.setBounds  (145, 345, 42, 42);
        v1VolumeKnob.setBounds  (210, 345, 42, 42);

        // Voice 2 Symmetrical layerable tactile instrument tabs [3]
        v2AnalogBtn.setBounds (15, 415, 63, 18);
        v2FmBtn.setBounds (81, 415, 63, 18);
        v2StringBtn.setBounds (147, 415, 63, 18);
        v2PulseBtn.setBounds (213, 415, 63, 18);

        // Voice 2 tactile independent ADSR / Timbre / Delay / Reverb / Volume knobs (Rotaries!) [3]
        v2AttackKnob.setBounds  (15,  495, 42, 42);
        v2DecayKnob.setBounds   (80,  495, 42, 42);
        v2SustainKnob.setBounds (145, 495, 42, 42);
        v2ReleaseKnob.setBounds (210, 495, 42, 42);

        v2TimbreKnob.setBounds  (15,  545, 42, 42);
        v2DelayKnob.setBounds   (80,  545, 42, 42);
        v2ReverbKnob.setBounds  (145, 545, 42, 42);
        v2VolumeKnob.setBounds  (210, 545, 42, 42);

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
        bool v1Analog = processor.voice1AnalogPtr->load() > 0.5f;
        bool v1Fm     = processor.voice1FmPtr->load() > 0.5f;
        bool v1String = processor.voice1StringPtr->load() > 0.5f;
        bool v1Pulse  = processor.voice1PulsePtr->load() > 0.5f;

        v1AnalogBtn.setToggleState (v1Analog, juce::dontSendNotification);
        v1FmBtn.setToggleState (v1Fm, juce::dontSendNotification);
        v1StringBtn.setToggleState (v1String, juce::dontSendNotification);
        v1PulseBtn.setToggleState (v1Pulse, juce::dontSendNotification);

        bool v2Analog = processor.voice2AnalogPtr->load() > 0.5f;
        bool v2Fm     = processor.voice2FmPtr->load() > 0.5f;
        bool v2String = processor.voice2StringPtr->load() > 0.5f;
        bool v2Pulse  = processor.voice2PulsePtr->load() > 0.5f;

        v2AnalogBtn.setToggleState (v2Analog, juce::dontSendNotification);
        v2FmBtn.setToggleState (v2Fm, juce::dontSendNotification);
        v2StringBtn.setToggleState (v2String, juce::dontSendNotification);
        v2PulseBtn.setToggleState (v2Pulse, juce::dontSendNotification);
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