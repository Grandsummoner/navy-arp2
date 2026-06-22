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
    lastChordPitches = { 60, 64, 67 }; // Default C major root cache
}

PluginProcessor::~PluginProcessor() {}

const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }
bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }
bool PluginProcessor::isMidiEffect() const { return false; } // VST3 Instrument format for multi-track Ableton routing
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }
int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String PluginProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void PluginProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
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

// ==============================================================================
// EUCLID RHYTHM MATH GENERATOR
// ==============================================================================
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

// ==============================================================================
// INVISIBLE CURATOR LFO ENGINE & ACCUMULATOR
// ==============================================================================
void PluginProcessor::updateLfoModulations (int numSamples, double bpm)
{
    double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
    double samplesPerBar = samplesPerBeat * 4.0;
    double sampleDelta = numSamples;

    // Correctly map dropdown index (0,1,2,3) to actual bar counts (1,2,4,8) to prevent division by zero
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

    // Bipolar Entropy Accumulator (Evolving Melody)
    float baseEntropy = *apvts.getRawParameterValue (IDs::entropy.getParamID()); // Bipolar -1.0 to +1.0

    // Diatonic interval step selection based on Entropy depth
    float absEntropy = std::abs(baseEntropy);
    int stepInterval = 1; // Scalar steps
    if (absEntropy > 0.33f && absEntropy <= 0.66f) stepInterval = 2; // Diatonic 3rds
    else if (absEntropy > 0.66f) stepInterval = 4; // Diatonic 5ths (Dominant)

    if (currentStep == 0 && mLastStep == 7) // Loop turnaround completed
    {
        if (baseEntropy > 0.05f) accumulatedPitchOffset += stepInterval;
        else if (baseEntropy < -0.05f) accumulatedPitchOffset -= stepInterval;
        
        // Auto-reset on measure cycle boundary
        if (currentBarInCycle == 1) accumulatedPitchOffset = 0.0f;
    }

    // Curated LFOs
    lfoPhaseLegato += (sampleDelta / (samplesPerBar * 2.0)); // 2-bar sine
    if (lfoPhaseLegato >= 1.0) lfoPhaseLegato -= 1.0;
    modLegato = *apvts.getRawParameterValue (IDs::legato.getParamID()) + 0.2f * static_cast<float>(std::sin(lfoPhaseLegato * juce::MathConstants<double>::twoPi));
    modLegato = juce::jlimit(0.1f, 1.0f, modLegato);

    modRest = *apvts.getRawParameterValue (IDs::rest.getParamID());
    modHarmony = *apvts.getRawParameterValue (IDs::harmony.getParamID());
    modChaos = *apvts.getRawParameterValue (IDs::chaos.getParamID());

    // Trance Extension Visual Label calculation
    if (modHarmony < 0.34f) activeChordExtensionText = "TRIAD";
    else if (modHarmony >= 0.34f && modHarmony < 0.67f) activeChordExtensionText = "SUS";
    else activeChordExtensionText = "7th/9th";
}

// ==============================================================================
// DECREMENTING NOTE-OFF SCHEDULER (PREVENTS SYNTH VOICE CHOKING)
// ==============================================================================
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
// MAIN REAL-TIME DSP & MIDI CLOCK PROCESSOR
// ==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear audio buffer outputs to prevent NaNs/Subnormals (passes pluginval cleanly)
    buffer.clear();

    // 1. Query DAW Transport State
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

    // 2. Process Decrementing Note-Off Queue
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

    // 3. Monitor physical keyboard pressed MIDI keys
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

    // BYPASS RULE: If stopped, unlatched, and no keys held, bypass cleanly
    if (! isPlaying && ! isLatchActive && activeHeldNotes.empty())
    {
        currentStep = 0; mLastStep = -1;
        return; 
    }

    midiMessages.clear();

    const auto& notesToPlay = isLatchActive ? latchedNotes : activeHeldNotes;
    
    // 4. Dual-Clock Step Generation
    if (! notesToPlay.empty())
    {
        bool stepTriggered = false;
        double samplesPerBeat = mSampleRate * (60.0 / (bpm > 0 ? bpm : 120.0));
        double stepSamples = samplesPerBeat * 0.25;

        if (isPlaying)
        {
            // DAW Sync PPQ Clock
            int stepIndex = static_cast<int> (std::floor (mSongPositionPPQ / 0.25)) % 8;
            if (stepIndex != mLastStep)
            {
                mLastStep = stepIndex;
                currentStep = stepIndex;
                stepTriggered = true;
            }
        }
        else
        {
            // Standalone Sample Clock
            mTimeInSamples += numSamples;
            if (mTimeInSamples >= stepSamples)
            {
                mTimeInSamples = 0;
                currentStep = (currentStep + 1) % 8;
                mLastStep = currentStep;
                stepTriggered = true;
            }
        }

        // 5. Arpeggiator Step & Euclidean Execution
        if (stepTriggered)
        {
            float faderProb = *apvts.getRawParameterValue (juce::String ("fader" + juce::String (currentStep + 1)));
            float baseRhyMorph = *apvts.getRawParameterValue (IDs::rhythmMorph.getParamID());
            
            // Calculate Euclidean Ratchets
            int ratchetPulses = static_cast<int>(std::round(baseRhyMorph * 8.0f));
            std::vector<int> euclidRatchets = generateEuclideanPattern (8, ratchetPulses);
            bool isRatchetStep = euclidRatchets[currentStep] == 1;

            bool shouldPlay = (juce::Random::getSystemRandom().nextFloat() <= faderProb);
            bool isRest = (juce::Random::getSystemRandom().nextFloat() <= activeRest);

            if (shouldPlay && ! isRest)
            {
                // If another note is currently playing, turn it off immediately
                if (mLastNotePlayed != -1)
                {
                    processedMidi.addEvent (juce::MidiMessage::noteOff (1, mLastNotePlayed), 0);
                    mLastNotePlayed = -1;
                }

                // Clamped root and scale selectors to prevent out-of-bounds array access under automation
                int rootKeyIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
                int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

                // 10 Native Scales
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

                // Chaos Octave Leaps (Sample & Hold Quantized)
                if (modChaos > 0.2f && juce::Random::getSystemRandom().nextFloat() <= modChaos)
                    targetPitch += (juce::Random::getSystemRandom().nextBool() ? 12 : -12);

                targetPitch = juce::jlimit(0, 127, targetPitch);
                int durationSamples = static_cast<int>(stepSamples * modLegato);

                // Trigger Main Note
                processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(100)), 0);
                scheduleNoteOff (processedMidi, targetPitch, durationSamples);

                // Euclidean Ratchet execution (Double trigger)
                if (isRatchetStep)
                {
                    int ratchetDelay = static_cast<int>(stepSamples * 0.5);
                    processedMidi.addEvent (juce::MidiMessage::noteOn (1, targetPitch, static_cast<juce::uint8>(90)), ratchetDelay);
                    scheduleNoteOff (processedMidi, targetPitch, ratchetDelay + (durationSamples / 2));
                }

                // Trance Extension Chords (Pillar 3)
                if (modHarmony >= 0.34f)
                {
                    int extOffset = (modHarmony < 0.67f) ? 5 : 14; // Sus4 (+5 semitones) or 9th (+14)
                    int extPitch = juce::jlimit(0, 127, targetPitch + extOffset);
                    processedMidi.addEvent (juce::MidiMessage::noteOn (1, extPitch, static_cast<juce::uint8>(85)), 0);
                    scheduleNoteOff (processedMidi, extPitch, durationSamples);
                }
            }
        }
    }
    else { mLastStep = -1; currentStep = 0; }

    midiMessages.swapWith (processedMidi);
}

// ==============================================================================
// SMART DIATONIC CHORD PADS WITH VOICE LEADING (PILLAR 1 & 2)
// ==============================================================================
void PluginProcessor::triggerDiatonicChordPad (int padIndex)
{
    int rootIdx = juce::jlimit (0, 11, static_cast<int> (*apvts.getRawParameterValue (IDs::rootKey.getParamID())));
    int scaleIdx = juce::jlimit (0, 9, static_cast<int> (*apvts.getRawParameterValue (IDs::scaleType.getParamID())));

    // Base Diatonic Triad roots for degree I through VIII
    std::vector<int> degrees = { 0, 2, 4, 5, 7, 9, 11, 12 };
    if (scaleIdx == 1) degrees = { 0, 2, 3, 5, 7, 8, 10, 12 }; // Minor

    int baseRoot = 48 + rootIdx + degrees[padIndex % 8];
    std::vector<int> newChord = { baseRoot, baseRoot + 4, baseRoot + 7 }; // Simplified triad generator
    if (scaleIdx == 1) newChord = { baseRoot, baseRoot + 3, baseRoot + 7 };

    // Apply Smooth Voice Leading (Inversions)
    if (! lastChordPitches.empty() && newChord.size() == lastChordPitches.size())
    {
        int pitchDiff = newChord[2] - lastChordPitches[2];
        if (pitchDiff > 5) newChord[2] -= 12; // Invert top note down
        else if (pitchDiff < -5) newChord[0] += 12; // Invert bottom note up
    }
    
    std::sort(newChord.begin(), newChord.end());
    lastChordPitches = newChord;

    // Inject into Latch buffer
    latchedNotes = newChord;
    apvts.getParameter(IDs::latch.getParamID())->setValueNotifyingHost(1.0f); // Auto-latch the chord
}

// ==============================================================================
// SCENE & PRESET MANAGEMENT
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

void PluginProcessor::resetAccumulator() { accumulatedPitchOffset = 0.0f; }
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
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::entropy, "Entropy", -1.0f, 1.0f, 0.0f)); // Bipolar -1.0 to +1.0
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::harmony, "Harmony", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::chaos, "Chaos", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (IDs::morph, "Morph Crossfader", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::latch, "Latch Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool>  (IDs::chordMode, "Chord Mode", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::rootKey, "Root Key", 
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::scaleType, "Scale", 
        juce::StringArray { "Major", "Natural Minor", "Pentatonic Minor", "Pentatonic Major", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor", "Melodic Minor" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (IDs::cycleLength, "Cycle Length", 
        juce::StringArray { "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2)); // Default 4 Bars

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }