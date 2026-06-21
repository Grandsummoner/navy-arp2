#include "PluginProcessor.h"
#include "PluginEditor.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
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
bool PluginProcessor::isMidiEffect() const { return false; }
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }
int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String PluginProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void PluginProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mTimeInSamples = 0;
    mLastNotePlayed = -1;
    activeHeldNotes.clear();
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
         && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        #if ! JucePlugin_IsSynth
            if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
                return false;
        #endif
        return true;
    #endif
}

// ==============================================================================
// REAL-TIME ARPEGGIATOR MIDI CLOCK ENGINE
// ==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // 1. Clear Audio Buffers (Navy-arp outputs only MIDI)
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 2. Read parameters
    float activeFaderProb[8];
    for (int i = 0; i < 8; ++i)
        activeFaderProb[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));

    float activeRest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    float activeLegato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    float activeHarmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    float activeChaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());

    // 3. Monitor physical keyboard pressed MIDI keys
    juce::MidiBuffer processedMidi;
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            int note = msg.getNoteNumber();
            if (std::find (activeHeldNotes.begin(), activeHeldNotes.end(), note) == activeHeldNotes.end())
            {
                activeHeldNotes.push_back (note);
                std::sort (activeHeldNotes.begin(), activeHeldNotes.end());
            }
        }
        else if (msg.isNoteOff())
        {
            int note = msg.getNoteNumber();
            activeHeldNotes.erase (std::remove (activeHeldNotes.begin(), activeHeldNotes.end(), note), activeHeldNotes.end());
        }
    }
    midiMessages.clear();

    // 4. Calculate tempo clock and generate steps
    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;
    
    // Only generate arpeggiated steps if keys are pressed, or latch is active
    if (! activeHeldNotes.empty() || isLatchActive)
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
        double stepLengthInSamples = samplesPerBeat * 0.25; // 1/16th notes

        int numSamples = buffer.getNumSamples();
        mTimeInSamples += numSamples;

        if (mTimeInSamples >= stepLengthInSamples)
        {
            mTimeInSamples = 0;
            currentStep = (currentStep + 1) % 8;

            // Probability Check using faders
            float stepProbability = activeFaderProb[currentStep];
            bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= stepProbability);
            bool isRest = (juce::Random::getSystemRandom().nextFloat() <= activeRest);

            if (shouldPlay && ! isRest && ! activeHeldNotes.empty())
            {
                // Kill previous note
                if (mLastNotePlayed != -1)
                {
                    processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                    mLastNotePlayed = -1;
                }

                // Map step index to one of the active held keys
                int pitchIndex = currentStep % activeHeldNotes.size();
                int targetNote = activeHeldNotes[pitchIndex];
                
                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetNote, static_cast<juce::uint8>(100)), 0);
                mLastNotePlayed = targetNote;
            }
        }
    }
    else
    {
        currentStep = 0;
    }

    midiMessages.swapWith(processedMidi);
}

// ==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

// ==============================================================================
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
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
    }
}

// ==============================================================================
// Preset Management
// ==============================================================================
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

// ==============================================================================
// Scene Capture Functions (Octatrack Style)
// ==============================================================================
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
    sceneB.rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    sceneB.legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    sceneB.entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    sceneB.harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    sceneB.chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    hasSceneB = true;
}

void PluginProcessor::diceMelody()
{
    auto* random = &juce::Random::getSystemRandom();
    for (int i = 0; i < 8; ++i)
    {
        float randomVal = random->nextFloat();
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (randomVal);
    }
}

void PluginProcessor::diceRhythm()
{
    auto* random = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (random->nextFloat());
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (random->nextFloat() * 0.7f);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.1f + random->nextFloat() * 0.8f);
}

// ==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int i = 1; i <= 8; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("fader" + juce::String (i), 1), "Fader " + juce::String (i), 0.0f, 1.0f, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rhythmMorph, "Rhythm Morph", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rest, "Rest", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::legato, "Legato", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (IDs::latch, "Latch Mode", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }