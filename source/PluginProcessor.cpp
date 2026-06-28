#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                      #if ! JucePlugin_IsMidiEffect
                       #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                      #endif
                      ),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Configure Curated LFO Presets Map
    lfoPresets = {
        { "Off",    0.00f, 0.00f }, // 0: Standard static scene state
        { "Drift",  0.25f, 0.15f }, // 1: Slow (1/4 Note), Subtle (15%)
        { "Bounce", 0.50f, 0.35f }, // 2: Medium (1/8 Note), Moderate (35%)
        { "Vibe",   1.00f, 0.50f }, // 3: Fast (1/16 Note), Expressive (50%)
        { "Tremor", 2.00f, 0.75f }  // 4: Rapid (1/32 Note), Intense (75%)
    };

    // Initialize scenes with standard defaults
    for (int i = 0; i < 8; ++i)
    {
        sceneA.stepFaders[i] = 1.0f;
        sceneA.knobValues[i] = 0.5f;
        sceneA.lfoPresets[i] = 0;

        sceneB.stepFaders[i] = 1.0f;
        sceneB.knobValues[i] = 0.5f;
        sceneB.lfoPresets[i] = 0;
    }
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Morph Crossfader (Shortened to 200px horizontally in layout)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("morph", 1), "Morph", 0.0f, 1.0f, 0.5f));

    // Freeze Mode Toggle
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("freeze", 1), "Freeze", false));

    // MIDI Output Channel Selector (1 to 16)
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("midiChannel", 1), "MIDI Channel", 1, 16, 1));

    // Step parameters & generic knob layouts for parameter state binding
    for (int i = 0; i < 8; ++i)
    {
        juce::String idStr = juce::String (i);
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("step" + idStr, 1), "Step " + juce::String (i + 1), 0.0f, 1.0f, 1.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("knob" + idStr, 1), "Knob " + juce::String (i + 1), 0.0f, 1.0f, 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID ("lfoPreset" + idStr, 1), "LFO Preset " + juce::String (i + 1), 0, 4, 0));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    clockStep = 0;
    playheadPosition = 0.0;
    activeNotes.clear();
    
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear audio buffer if layout demands it (Arpeggiator operates primarily as a MIDI generator)
    for (int i = 0; i < buffer.getNumChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    // 1. Retrieve Central Controller & Latch Settings
    const float morphVal = apvts.getRawParameterValue ("morph")->load();
    const bool freezeActive = apvts.getRawParameterValue ("freeze")->load();
    const int targetMidiChannel = static_cast<int> (apvts.getRawParameterValue ("midiChannel")->load());

    // Cache handling upon switching Freeze state
    if (freezeActive && !isFrozen.load())
    {
        isFrozen.store (true);
        // Snapshot current states into frozen cache
        for (int i = 0; i < 8; ++i)
        {
            float interpFader = juce::jmap (morphVal, sceneA.stepFaders[i], sceneB.stepFaders[i]);
            float interpKnob = juce::jmap (morphVal, sceneA.knobValues[i], sceneB.knobValues[i]);
            
            frozenCache.stepFaders[i] = interpFader;
            frozenCache.knobValues[i] = interpKnob;
            frozenCache.lfoPresets[i] = (morphVal < 0.5f) ? sceneA.lfoPresets[i] : sceneB.lfoPresets[i];
        }
    }
    else if (!freezeActive && isFrozen.load())
    {
        isFrozen.store (false);
    }

    // 2. Perform Decoupled Scene Parameter Interpolation (governed by morph position)
    SceneSnapshot activeState;
    if (isFrozen.load())
    {
        activeState = frozenCache;
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            activeState.stepFaders[i] = juce::jmap (morphVal, sceneA.stepFaders[i], sceneB.stepFaders[i]);
            activeState.knobValues[i] = juce::jmap (morphVal, sceneA.knobValues[i], sceneB.knobValues[i]);
            activeState.lfoPresets[i] = (morphVal < 0.5f) ? sceneA.lfoPresets[i] : sceneB.lfoPresets[i];
        }
    }

    // 3. Precise Sample-Offset Note Off Execution (prevents chokes in stopped states)
    auto it = activeNotes.begin();
    while (it != activeNotes.end())
    {
        it->second -= numSamples;
        if (it->second <= 0)
        {
            // Insert Note-Off at precise expired sample offset rather than snapping strictly to sample 0
            int sampleOffset = juce::jlimit (0, numSamples - 1, it->second + numSamples);
            midiMessages.addEvent (juce::MidiMessage::noteOff (targetMidiChannel, it->first), sampleOffset);
            it = activeNotes.erase (it);
        }
        else
        {
            ++it;
        }
    }

    // 4. Temporary Internal Clock Tracking / Simple Step Sequencer Trigger
    if (auto* currentPlayHead = getPlayHead())
    {
        if (auto bpmOpt = currentPlayHead->getPosition()->getBpm())
        {
            double bpm = *bpmOpt;
            double samplesPerBeat = (currentSampleRate * 60.0) / bpm;
            double samplesPerStep = samplesPerBeat * 0.25; // 16th note steps

            // Simplified triggering loop inside the buffer blocks
            static double sampleCounter = 0.0;
            sampleCounter += numSamples;

            if (sampleCounter >= samplesPerStep)
            {
                sampleCounter = std::fmod (sampleCounter, samplesPerStep);
                clockStep = (clockStep + 1) % 8;

                // Process note sequence trigger
                float currentFaderVal = activeState.stepFaders[clockStep];
                if (currentFaderVal > 0.1f) 
                {
                    // Generate basic arp root transposition based on fader height
                    int noteNumber = 60 + static_cast<int> (currentFaderVal * 12.0f);
                    
                    // Trigger note on
                    midiMessages.addEvent (juce::MidiMessage::noteOn (targetMidiChannel, noteNumber, 0.8f), 0);
                    
                    // Track for precise sample-offset note offs later
                    activeNotes.push_back ({ noteNumber, static_cast<int> (samplesPerStep * 0.8) });
                }
            }
        }
    }
}

//==============================================================================
void PluginProcessor::captureScene (bool targetB)
{
    // Write standard UI control states down to static scene buffers
    SceneSnapshot& destScene = targetB ? sceneB : sceneA;
    for (int i = 0; i < 8; ++i)
    {
        destScene.stepFaders[i] = apvts.getRawParameterValue ("step" + juce::String (i))->load();
        destScene.knobValues[i] = apvts.getRawParameterValue ("knob" + juce::String (i))->load();
        destScene.lfoPresets[i] = static_cast<int> (apvts.getRawParameterValue ("lfoPreset" + juce::String (i))->load());
    }
}

void PluginProcessor::triggerSceneAnchorSwitch()
{
    // Swaps internal active editing lens target atomically
    bool wasSceneB = isSceneBActiveAnchor.load();
    isSceneBActiveAnchor.store (!wasSceneB);
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return "Navy Arp";
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::producesMidi() const
{
    return true;
}

bool PluginProcessor::isMidiEffect() const
{
    return true;
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

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    juce::ignoreUnused (layouts);
    return true;
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates the editor instance
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
// Go to CMake/JUCE entrypoint definitions
#ifndef JucePlugin_PreferredChannelConfigurations
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
#endif