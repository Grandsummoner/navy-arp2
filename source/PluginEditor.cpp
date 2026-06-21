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

    // Left sidebar knobs
    juce::Slider* leftKnobs[] = { &rhythmMorphKnob, &restKnob, &legatoKnob };
    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        leftKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        leftKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFF00D2FF));
        leftKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        leftKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (leftKnobs[i]);
    }

    // Right sidebar knobs
    juce::Slider* rightKnobs[] = { &entropyKnob, &harmonyKnob, &chaosKnob };
    for (int i = 0; i < 3; ++i)
    {
        rightKnobs[i]->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rightKnobs[i]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 16);
        rightKnobs[i]->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFFFB300));
        rightKnobs[i]->setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
        rightKnobs[i]->setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xFF888888));
        addAndMakeVisible (rightKnobs[i]);
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
    latchButton.setClickingTogglesState (true); // Force toggle behavior
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

    // Scene Capture System (Octatrack Style)
    addAndMakeVisible (sceneAButton);
    sceneAButton.setButtonText ("A");
    sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneAButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneAButton.onClick = [this] {
        processor.captureSceneA();
        sceneAButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF00D2FF)); // Glow cyan when captured
    };

    addAndMakeVisible (sceneBButton);
    sceneBButton.setButtonText ("B");
    sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF111111));
    sceneBButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFFCCCCCC));
    sceneBButton.onClick = [this] {
        processor.captureSceneB();
        sceneBButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFFFFB300)); // Glow amber when captured
    };

    // 8 Preset Slots (Recall on click / Save on 2.0s hold)
    for (int i = 0; i < 8; ++i)
    {
        addAndMakeVisible (presetButtons[i]);
        presetButtons[i].setButtonText (juce::String (i + 1));
        presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF050505));
        presetButtons[i].setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF444444));

        presetButtons[i].onStateChange = [this, i] {
            if (presetButtons[i].isDown())
            {
                presetPressStartTime[i] = juce::Time::getMillisecondCounter();
            }
            else if (presetPressStartTime[i] != 0)
            {
                uint32_t elapsed = juce::Time::getMillisecondCounter() - presetPressStartTime[i];
                presetPressStartTime[i] = 0;

                if (elapsed >= 2000) // 2.0s Hold -> Save
                {
                    processor.savePreset(i);
                    presetButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF003344)); // Outer edge glow enabled
                }
                else // Tap -> Recall
                {
                    processor.loadPreset(i);
                }
            }
        };
    }

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

    entropyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::entropy.getParamID(), entropyKnob);
    harmonyAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::harmony.getParamID(), harmonyKnob);
    chaosAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::chaos.getParamID(), chaosKnob);

    morphAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, IDs::morph.getParamID(), morphCrossfader);
    latchAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, IDs::latch.getParamID(), latchButton);

    setSize (750, 480);
    startTimerHz (30); // Refreshes visual elements like faders gliding in morph mode
}

PluginEditor::~PluginEditor() { stopTimer(); }

void PluginEditor::timerCallback()
{
    // If the morph crossfader is moved and we are NOT in edit mode,
    // smoothly glide the on-screen sliders and knobs in real-time
    if (! processor.isEditingSceneA && ! processor.isEditingSceneB)
    {
        float morphValue = morphCrossfader.getValue();
        
        juce::Slider* faders[] = { &fader1, &fader2, &fader3, &fader4, &fader5, &fader6, &fader7, &fader8 };
        for (int i = 0; i < 8; ++i)
        {
            float targetValue = (processor.sceneA.faders[i] * (1.0f - morphValue)) + (processor.sceneB.faders[i] * morphValue);
            faders[i]->setValue (targetValue, juce::dontSendNotification);
        }

        rhythmMorphKnob.setValue ((processor.sceneA.rhythmMorph * (1.0f - morphValue)) + (processor.sceneB.r