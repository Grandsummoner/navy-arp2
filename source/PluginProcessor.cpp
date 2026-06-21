#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      ),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    sceneA = SceneState();
    sceneB = SceneState();
}

PluginProcessor::~PluginProcessor() {}

const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }
bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }

bool PluginProcessor::isMidiEffect() const
{
    return false; // Revert to standard Instrument so Ableton allows multi-track routing
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

// ==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mTimeInSamples = 0;
    mLastNotePlayed = -1;
    activeHeldNotes.clear();
    latchedNotes.clear();
    isFirstNoteOfNewChord = true;
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

// ==============================================================================
// REAL-TIME ARPEGGIATOR MIDI CLOCK ENGINE (WITH LATCH CHORD MEMORY)
// ==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer);

    // Read parameters
    float activeFaderProb[8];
    for (int i = 0; i < 8; ++i)
        activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    float activeRest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    float activeLegato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;

    // 3. Monitor physical keyboard pressed MIDI keys
    juce::MidiBuffer processedMidi;
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            
            // Add to active fingers-on-keys tracker
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end())
            {
                activeHeldNotes.push_back (note);
                std::sort (activeHeldNotes.begin(), activeHeldNotes.end());
            }

            // Latch Mode: If starting a brand new chord, wipe the latch memory
            if (isLatchActive)
            {
                if (isFirstNoteOfNewChord)
                {
                    latchedNotes.clear();
                    isFirstNoteOfNewChord = false;
                }
                if (std::find (latchedNotes.begin(), latchedNotes.end(), note) == latchedNotes.end())
                {
                    latchedNotes.push_back (note);
                    std::sort (latchedNotes.begin(), latchedNotes.end());
                }
            }
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
            
            // If all physical keys are released, flag that the next note starts a new chord
            if (activeHeldNotes.empty())
            {
                isFirstNoteOfNewChord = true;
            }
        }
    }
    midiMessages.clear();

    // Determine the active chord notes to play from (latch memory vs. active fingers)
    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    
    if (! notesToPlay.empty())
    {
        double bpm = 120.0;
        if (auto* playhead = getPlayHead())
        {
            if (auto pos = playhead->getPosition())
            {
                auto bpmOpt = pos->getBpm();
                if (bpmOpt.hasValue())
                    bpm = *bpmOpt;
            }
        }

        double samplesPerBeat = mSampleRate * (60.0 / bpm);
        double stepLengthInSamples = samplesPerBeat * 0.25;

        int numSamples = buffer.getNumSamples();
        mTimeInSamples += numSamples;

        if (mTimeInSamples >= stepLengthInSamples)
        {
            mTimeInSamples = 0;
            currentStep = (currentStep + 1) % 8;

            float stepProbability = activeFaderProb[currentStep];
            bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= stepProbability);
            bool isRest = (juce::Random::getSystemRandom().nextFloat() <= activeRest);

            if (shouldPlay && ! isRest)
            {
                if (mLastNotePlayed != -1)
                {
                    processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                    mLastNotePlayed = -1;
                }

                int pitchIndex = currentStep % notesToPlay.size();
                int targetNote = notesToPlay[pitchIndex];
                
                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetNote, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetNote;
            }
        }
    }
    else
    {
        if (mLastNotePlayed != -1)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
            mLastNotePlayed = -1;
        }
        currentStep = 0;
    }

    midiMessages.swapWith(processedMidi);
}

bool PluginProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void PluginProcessor::savePreset (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8)
    {
        for (int i = 0; i < 8; ++i)
            presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

        presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
        presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
        presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
        presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
        presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
        presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
        presetSlotsSaved[slotIndex] = true;
    }
}

void PluginProcessor::loadPreset (int slotIndex)
{
    if (slotIndex >= 0 && slotIndex < 8 && presetSlotsSaved[slotIndex])
    {
        apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph);
        apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest);
        apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato);
        apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy);
        apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony);
        apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos);

        for (int i = 0; i < 8; ++i)
            apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]);
    }
}

void PluginProcessor::captureSceneA()
{
    for (int i = 0; i < 8; ++i)
        sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    sceneA.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    sceneA.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneA.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneA.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneA.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneA.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneA = true;
}

void PluginProcessor::captureSceneB()
{
    for (int i = 0; i < 8; ++i)
        sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    sceneB.rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());