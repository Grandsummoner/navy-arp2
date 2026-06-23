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
    lastChordPitches = { 60, 64, 67 }; 
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
    const juce::ScopedLock sl (noteLock); // Acquire lock during init
    mSampleRate = sampleRate;
    mLastStep = -1;
    mLastNotePlayed = -1;
    mNoteOffTime = 0;
    mTimeInSamples = 0;
    activeHeldNotes.clear();
    latchedNotes.clear();
    isFirstNoteOfNewChord = true;
    juce::ignoreUnused (samplesPerBlock);
}

void PluginProcessor::releaseResources() {}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

std::vector<int> PluginProcessor::generateEuclideanPattern (int steps, int pulses)
{
    std::vector<int> pattern(steps, 0);
    if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), 1); return pattern; }

    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps) { bucket -= steps; pattern[i] = 1; }
    }
    return pattern;
}

void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
    double samplesPerBar = samplesPerBeat * 4.0;
    double sampleDelta = numSamples;

    int cycleIndex = juce::jlimit (0, 3, static_cast<int> (*apvts.getRawParameterValue (IDs::cycleLength.getParamID())));
    int cycleBars = 4;
    if (cycleIndex == 0)      cycleBars = 1;
    else if (cycleIndex == 1) cycleBars = 2;
    else if (cycleIndex == 2) cycleBars = 4;
    else if (cycleIndex == 3) cycleBars = 8;

    if (cycleBars > 0)
        currentBarInCycle = (static_cast<int>(std::floor(mSongPositionPPQ / 4.0)) % cycleBars) + 1;
    else
        currentBarInCycle = 1;

    float baseEntropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());

    float absEntropy = std::abs(baseEntropy);
    int stepInterval = 1; 
    if (absEntropy > 0.33f && absEntropy <= 0.66f) stepInterval = 2; 
    else if (absEntropy > 0.66f) stepInterval = 4; 

    if (currentStep == 0 && mLastStep == 7) 
    {
        if (baseEntropy > 0.05f) accumulatedPitchOffset += stepInterval;
        else if (baseEntropy < -0.05f) accumulatedPitchOffset -= stepInterval;
        if (currentBarInCycle == 1) accumulatedPitchOffset = 0.0f;
    }

    lfoPhaseLegato += (sampleDelta / (samplesPerBar * 2.0)); 
    if (lfoPhaseLegato >= 1.0) lfoPhaseLegato -= 1.0;
    modLegato = *apvts.getRawParameterValue (IDs::legato.getParamID()) + 0.2f * static_cast<float>(std::sin(lfoPhaseLegato * juce::MathConstants<double>::twoPi));
    modLegato = juce::jlimit(0.1f, 1.0f, modLegato);

    modRest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    modHarmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    modChaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());

    if (modHarmony < 0.34f) activeChordExtensionText = "TRIAD";
    else if (modHarmony >= 0.34f && modHarmony < 0.67f) activeChordExtensionText = "SUS";
    else activeChordExtensionText = "7th/9th";
}

void PluginProcessor::scheduleNoteOff (juce::MidiBuffer& midi, int pitch, int delaySamples)
{
    if (delaySamples <= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (1, pitch), 0);
    }
    else
    {
        scheduledNoteOffs.push_back ({ pitch, delaySamples });
    }
}

// ==============================================================================
// REAL-TIME THREAD-SAFE PROCESS BLOCK
// ==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // Acquire lightweight thread lock for the remainder of this block (resolves data races)
    const juce::ScopedLock sl (noteLock);

    bool isPlaying = false;
    double bpm = 120.0;
    mSongPositionPPQ = 0.0;

    if (auto* playhead = getPlayHead())
    {
        if (auto pos = playhead->getPosition())
        {
            isPlaying = pos->getIsPlaying();
            auto bpmOpt = pos->getBpm();
            if (bpmOpt.hasValue()) bpm = *bpmOpt;
            auto ppqOpt = pos->getPpqPosition();
            if (ppqOpt.hasValue()) mSongPositionPPQ = *ppqOpt;
        }
    }

    int numSamples = buffer.getNumSamples();
    updateLfoModulations (numSamples, bpm);

    bool isLatchActive = *apvts.getRawParameterValue (IDs::latch.getParamID()) > 0.5f;

    juce::MidiBuffer processedMidi;
    for (auto it = scheduledNoteOffs.begin(); it != scheduledNoteOffs.end();)
    {
        it->second -= numSamples;
        if (it->second <= 0)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, it->first), 0);
            it = scheduledNoteOffs.erase(it);
        }
        else ++it;
    }

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

            if (isLatchActive)
            {
                if (isFirstNoteOfNewChord)
                {
                    for (int n : latchedNotes) scheduleNoteOff (processedMidi, n, 0);
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
            if (activeHeldNotes.empty()) isFirstNoteOfNewChord = true;
        }
    }

    if (! isPlaying && ! isLatchActive && activeHeldNotes.empty())
    {
        currentStep = 0; mLastStep = -1;
        return; 
    }

    midiMessages.clear();

    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    
    if (mLastNotePlayed != -1)
    {
        mNoteOffTime -= numSamples;
        if (mNoteOffTime <= 0)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
            mLastNotePlayed = -1;
        }
    }

    if (! notesToPlay.empty())
    {
        if (isPlaying)
        {
            double stepLengthPPQ = 0.25;
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / 0.25)) % 8;

            if (stepIndex != mLastStep)
            {
                mLastStep = stepIndex;
                currentStep = stepIndex;
                triggerArpStep (activeFaderProb[currentStep], notesToPlay, processedMidi, bpm);
            }
        }
        else
        {
            double samplesPerBeat = mSampleRate * (60.0 / bpm);
            double stepLengthInSamples = samplesPerBeat * 0.25;

            mTimeInSamples += numSamples;

            if (mTimeInSamples >= stepLengthInSamples)
            {
                mTimeInSamples = 0;
                currentStep = (currentStep + 1) % 8;
                mLastStep = currentStep;
                triggerArpStep (activeFaderProb[currentStep], notesToPlay, processedMidi, bpm);
            }
        }
    }
    else
    {
        mLastStep = -1;
        currentStep = 0;
    }

    midiMessages.swapWith (processedMidi);
}

void PluginProcessor::triggerArpStep (float stepProbability, const std::vector<int>& notesToPlay, juce::MidiBuffer& processedMidi, double bpm)
{
    bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= stepProbability);
    bool isRest = (juce::Random::getSystemRandom().nextFloat() <= modRest);

    if (shouldPlay && ! isRest && ! notesToPlay.empty())
    {
        if (mLastNotePlayed != -1)
        {
            processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
            mLastNotePlayed = -1;
        }

        int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
        int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

        std::vector<int> scaleOffsets = { 0, 2, 4, 5, 7, 9, 11, 12 }; // Major
        if (scaleIdx == 1)      scaleOffsets = { 0, 2, 3, 5, 7, 8, 10, 12 }; // Natural Minor (Aeolian)
        else if (scaleIdx == 2) scaleOffsets = { 0, 3, 5, 7, 10, 12, 15, 17 }; // Pentatonic Minor
        else if (scaleIdx == 3) scaleOffsets = { 0, 2, 4, 7, 9, 12, 14, 16 };  // Pentatonic Major
        else if (scaleIdx == 4) scaleOffsets = { 0, 2, 3, 5, 7, 9, 10, 12 };  // Dorian
        else if (scaleIdx == 5) scaleOffsets = { 0, 1, 3, 5, 7, 8, 10, 12 };  // Phrygian
        else if (scaleIdx == 6) scaleOffsets = { 0, 2, 4, 6, 7, 9, 11, 12 };  // Lydian
        else if (scaleIdx == 7) scaleOffsets = { 0, 2, 4, 5, 7, 9, 10, 12 };  // Mixolydian
        else if (scaleIdx == 8) scaleOffsets = { 0, 2, 3, 5, 7, 8, 11, 12 };  // Harmonic Minor
        else if (scaleIdx == 9) scaleOffsets = { 0, 2, 3, 5, 7, 9, 11, 12 };  // Melodic Minor

        int rawPitch = notesToPlay[currentStep % notesToPlay.size()];
        int octave = (rawPitch / 12) * 12;
        int targetPitch = octave + rootKeyIdx + scaleOffsets[currentStep] + static_cast<int>(accumulatedPitchOffset);

        if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos)
            targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);

        targetPitch = juce::jlimit(0, 127, targetPitch);
        int durationSamples = static_cast<int>(mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0)) * 0.25 * modLegato);

        processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
        mLastNotePlayed = targetPitch;
        mNoteOffTime = durationSamples;
    }
}

// ==============================================================================
// THREAD-SAFE DIATONIC CHORD PADS
// ==============================================================================
void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    // Acquire the lock to prevent data race with Audio Thread (Resolves Segfault 139)
    const juce::ScopedLock sl (noteLock);

    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

    std::vector<int> degrees = { 0, 2, 4, 5, 7, 9, 11, 12 };
    if (scaleIdx == 1) degrees = { 0, 2, 3, 5, 7, 8, 10, 12 }; 

    int baseRoot = 48 + rootIdx + degrees[padIndex % 8];
    std::vector<int> newChord = { baseRoot, baseRoot + 4, baseRoot + 7 }; 
    if (scaleIdx == 1) newChord = { baseRoot, baseRoot + 3, baseRoot + 7 };

    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size())
    {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; 
        else if (pitchDiff < -5) newChord[0] += 12; 
    }
    
    std::sort(newChord.begin(), newChord.end());
    lastChordPitches = newChord;

    latchedNotes = newChord;
    apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); 
}

// ==============================================================================
void PluginProcessor::savePreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8) return;
    for (int i = 0; i < 8; ++i) presets[slotIndex].faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
    presets[slotIndex].rhythmMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
    presets[slotIndex].rest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    presets[slotIndex].legato = *apvts.getRawParameterValue (IDs::legato.getParamID());
    presets[slotIndex].entropy = *apvts.getRawParameterValue (IDs::entropy.getParamID());
    presets[slotIndex].harmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    presets[slotIndex].chaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());
    presetSlotsSaved[slotIndex] = true;
}

void PluginProcessor::loadPreset (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8 || ! presetSlotsSaved[slotIndex]) return;
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (presets[slotIndex].rhythmMorph);
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (presets[slotIndex].rest);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (presets[slotIndex].legato);
    apvts.getParameter (IDs::entropy.getParamID())->setValueNotifyingHost (presets[slotIndex].entropy);
    apvts.getParameter (IDs::harmony.getParamID())->setValueNotifyingHost (presets[slotIndex].harmony);
    apvts.getParameter (IDs::chaos.getParamID())->setValueNotifyingHost (presets[slotIndex].chaos);

    for (int i = 0; i < 8; ++i)
        apvts.getParameter (juce::String ("fader" + juce::String (i + 1)))->setValueNotifyingHost (presets[slotIndex].faders[i]);
}

void PluginProcessor::captureSceneA()
{
    for (int i = 0; i < 8; ++i) sceneA.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
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
    for (int i = 0; i < 8; ++i) sceneB.faders[i] = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (i + 1)));
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
    for (int i = 1; i <= 8; ++i) apvts.getParameter ("fader" + juce::String(i))->setValueNotifyingHost (random->nextFloat());
}

void PluginProcessor::diceRhythm()
{
    auto* random = &juce::Random::getSystemRandom();
    apvts.getParameter (IDs::rhythmMorph.getParamID())->setValueNotifyingHost (random->nextFloat());
    apvts.getParameter (IDs::rest.getParamID())->setValueNotifyingHost (random->nextFloat() * 0.5f);
    apvts.getParameter (IDs::legato.getParamID())->setValueNotifyingHost (0.2f + random->nextFloat() * 0.8f);
}

void PluginProcessor::resetAccumulator() { const juce::ScopedLock sl (noteLock); accumulatedPitchOffset = 0.0f; }
void PluginProcessor::resetRhythm() { apvts.getParameter(IDs::rhythmMorph.getParamID())->setValueNotifyingHost(0.0f); }

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

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (int i = 1; i <= 8; ++i)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("fader" + juce::String (i), 1), "Fader " + juce::String (i), 0.0f, 1.0f, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rhythmMorph, "Rhythm Morph", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::rest, "Rest", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::legato, "Legato", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", -1.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }