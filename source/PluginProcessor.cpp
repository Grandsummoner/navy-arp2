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
                    processedMidi.addEvent (juce::MidiMessage::n