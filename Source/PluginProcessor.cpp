/*
  ==============================================================================
    SineSynth - PluginProcessor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    addParameter(masterVolume = new juce::AudioParameterFloat(
        "masterVolume", "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

    addParameter(paramAttack = new juce::AudioParameterFloat(
        "attack", "Attack",
        juce::NormalisableRange<float>(0.001f, 4.0f, 0.0f, 0.4f), 0.01f));

    addParameter(paramDecay = new juce::AudioParameterFloat(
        "decay", "Decay",
        juce::NormalisableRange<float>(0.001f, 4.0f, 0.0f, 0.4f), 0.1f));

    addParameter(paramSustain = new juce::AudioParameterFloat(
        "sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    addParameter(paramRelease = new juce::AudioParameterFloat(
        "release", "Release",
        juce::NormalisableRange<float>(0.001f, 8.0f, 0.0f, 0.4f), 0.3f));

    addParameter(paramWaveType = new juce::AudioParameterInt(
        "waveType", "Wave Type", 0, 3, 0));

    addParameter(paramUnison = new juce::AudioParameterInt(
        "unison", "Unison", 1, MAX_UNISON, 1));

    addParameter(paramDetune = new juce::AudioParameterFloat(
        "detune", "Detune",
        juce::NormalisableRange<float>(0.0f, 100.0f), 0.0f));
}

NewProjectAudioProcessor::~NewProjectAudioProcessor() {}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const { return JucePlugin_Name; }
bool NewProjectAudioProcessor::acceptsMidi()  const { return true; }
bool NewProjectAudioProcessor::producesMidi() const { return false; }
bool NewProjectAudioProcessor::isMidiEffect() const { return false; }
double NewProjectAudioProcessor::getTailLengthSeconds() const { return 8.5; }

int  NewProjectAudioProcessor::getNumPrograms() { return 1; }
int  NewProjectAudioProcessor::getCurrentProgram() { return 0; }
void NewProjectAudioProcessor::setCurrentProgram(int) {}
const juce::String NewProjectAudioProcessor::getProgramName(int) { return {}; }
void NewProjectAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& v : voices) { v.isActive = false; v.envelope.reset(); }
}

void NewProjectAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}
#endif

//==============================================================================
void NewProjectAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const float vol = masterVolume->get();
    const int   numSamps = buffer.getNumSamples();
    const int   numCh = buffer.getNumChannels();

    // Snapshot parameters
    const float aRate = timeToRate(paramAttack->get());
    const float dRate = timeToRate(paramDecay->get());
    const float sLvl = paramSustain->get();
    const float rRate = timeToRate(paramRelease->get());
    const auto  waveType = static_cast<WaveType> (paramWaveType->get());
    const int   unison = paramUnison->get();
    const float detune = paramDetune->get();

    // Keep envelopes AND unison updated live on all active voices
    for (auto& v : voices)
        if (v.isActive)
        {
            v.updateEnvelope(aRate, dRate, sLvl, rRate);
            v.updateUnison(unison, detune, currentSampleRate);
        }

    int samplePos = 0;

    auto renderSamples = [&](int from, int to)
        {
            for (int s = from; s < to; ++s)
            {
                float mono = 0.0f;
                for (auto& v : voices)
                    if (v.isActive || v.envelope.isActive())
                        mono += v.processSample(waveType);

                mono *= vol;
                for (int ch = 0; ch < numCh; ++ch)
                    buffer.getWritePointer(ch)[s] += mono;
            }
        };

    for (const auto midiMeta : midiMessages)
    {
        const auto msg = midiMeta.getMessage();
        const int  msgSample = midiMeta.samplePosition;

        renderSamples(samplePos, msgSample);
        samplePos = msgSample;

        if (msg.isNoteOn())
        {
            SynthVoice* v = findVoiceForNote(msg.getNoteNumber());
            if (v == nullptr) v = findFreeVoice();
            if (v != nullptr)
                v->noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                    currentSampleRate, unison, detune,
                    aRate, dRate, sLvl, rRate);
        }
        else if (msg.isNoteOff())
        {
            SynthVoice* v = findVoiceForNote(msg.getNoteNumber());
            if (v != nullptr) v->noteOff();
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            for (auto& v : voices) v.noteOff();
        }
    }

    renderSamples(samplePos, numSamps);

    int count = 0;
    for (auto& v : voices) if (v.isActive) ++count;
    activeVoiceCount.store(count);
}

//==============================================================================
SynthVoice* NewProjectAudioProcessor::findFreeVoice()
{
    for (auto& v : voices)
        if (!v.isActive && !v.envelope.isActive())
            return &v;

    // Voice steal: quietest releasing voice
    SynthVoice* quietest = nullptr;
    for (auto& v : voices)
        if (v.envelope.stage == ADSREnvelope::Stage::Release)
            if (quietest == nullptr || v.envelope.level < quietest->envelope.level)
                quietest = &v;

    return quietest ? quietest : &voices[0];
}

SynthVoice* NewProjectAudioProcessor::findVoiceForNote(int note)
{
    for (auto& v : voices)
        if (v.isActive && v.midiNote == note)
            return &v;
    return nullptr;
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor(*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream s(destData, true);
    s.writeFloat(masterVolume->get());
    s.writeFloat(paramAttack->get());
    s.writeFloat(paramDecay->get());
    s.writeFloat(paramSustain->get());
    s.writeFloat(paramRelease->get());
    s.writeInt(paramWaveType->get());
    s.writeInt(paramUnison->get());
    s.writeFloat(paramDetune->get());
}

void NewProjectAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream s(data, static_cast<size_t> (sizeInBytes), false);
    if (s.getDataSize() < 6 * sizeof(float) + 2 * sizeof(int)) return;
    *masterVolume = s.readFloat();
    *paramAttack = s.readFloat();
    *paramDecay = s.readFloat();
    *paramSustain = s.readFloat();
    *paramRelease = s.readFloat();
    *paramWaveType = s.readInt();
    *paramUnison = s.readInt();
    *paramDetune = s.readFloat();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}