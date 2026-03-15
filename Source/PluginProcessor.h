/*
  ==============================================================================
    SineSynth - PluginProcessor.h
    Full-featured: ADSR, waveform selection, unison+detune, polyphony
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
enum class WaveType { Sine = 0, Saw, Triangle, Square };

//==============================================================================
struct ADSREnvelope
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float sustainLvl = 0.8f;
    float releaseRate = 0.0f;

    void noteOn() { stage = Stage::Attack; }
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
    void reset() { stage = Stage::Idle; level = 0.0f; }

    float process()
    {
        switch (stage)
        {
        case Stage::Attack:
            level += attackRate;
            if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            level -= decayRate;
            if (level <= sustainLvl) { level = sustainLvl; stage = Stage::Sustain; }
            break;
        case Stage::Sustain:
            level = sustainLvl;
            break;
        case Stage::Release:
            level -= releaseRate;
            if (level <= 0.0f) { level = 0.0f; stage = Stage::Idle; }
            break;
        default:
            level = 0.0f;
            break;
        }
        return level;
    }

    bool isActive() const { return stage != Stage::Idle || level > 0.0f; }
};

//==============================================================================
static constexpr int MAX_UNISON = 8;

struct OscVoice
{
    float phase = 0.0f;
    float phaseInc = 0.0f;
    bool  active = false;
};

//==============================================================================
struct SynthVoice
{
    bool         isActive = false;
    int          midiNote = -1;
    float        velocity = 1.0f;
    ADSREnvelope envelope;
    OscVoice     oscs[MAX_UNISON];
    int          unisonCount = 1;

    void noteOn(int note, float vel, double sampleRate,
        int unison, float detuneCents,
        float aRate, float dRate, float sLvl, float rRate)
    {
        midiNote = note;
        velocity = vel;
        isActive = true;
        unisonCount = unison;

        envelope.reset();
        envelope.attackRate = aRate;
        envelope.decayRate = dRate;
        envelope.sustainLvl = sLvl;
        envelope.releaseRate = rRate;
        envelope.noteOn();

        double baseFreq = 440.0 * std::pow(2.0, (note - 69) / 12.0);

        for (int i = 0; i < MAX_UNISON; ++i)
        {
            oscs[i].active = (i < unison);
            oscs[i].phase = 0.0f;
            if (i < unison)
            {
                float spreadSemitones = 0.0f;
                if (unison > 1)
                {
                    float t = (float)i / (float)(unison - 1);
                    spreadSemitones = (t - 0.5f) * (detuneCents / 100.0f);
                }
                double freq = baseFreq * std::pow(2.0, spreadSemitones / 12.0);
                oscs[i].phaseInc = (float)(juce::MathConstants<double>::twoPi * freq / sampleRate);
            }
        }
    }

    void noteOff() { envelope.noteOff(); }

    void updateEnvelope(float aRate, float dRate, float sLvl, float rRate)
    {
        envelope.attackRate = aRate;
        envelope.decayRate = dRate;
        envelope.sustainLvl = sLvl;
        envelope.releaseRate = rRate;
    }

    // Called every block on held voices -- reconfigures osc count & detune live
    // without resetting the envelope or existing phases.
    void updateUnison(int unison, float detuneCents, double sampleRate)
    {
        unisonCount = unison;
        double baseFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);

        for (int i = 0; i < MAX_UNISON; ++i)
        {
            bool wasActive = oscs[i].active;
            oscs[i].active = (i < unison);
            if (i < unison)
            {
                float spreadSemitones = 0.0f;
                if (unison > 1)
                {
                    float t = (float)i / (float)(unison - 1);
                    spreadSemitones = (t - 0.5f) * (detuneCents / 100.0f);
                }
                double freq = baseFreq * std::pow(2.0, spreadSemitones / 12.0);
                oscs[i].phaseInc = (float)(juce::MathConstants<double>::twoPi * freq / sampleRate);
                // If newly activated, start phase at 0 to avoid a click
                if (!wasActive)
                    oscs[i].phase = 0.0f;
            }
        }
    }

    float processSample(WaveType waveType)
    {
        float envLevel = envelope.process();
        if (!envelope.isActive()) { isActive = false; return 0.0f; }

        float out = 0.0f;
        float scale = 1.0f / (float)unisonCount;  // divide by count so coherent unison (detune=0) stays same volume

        for (int i = 0; i < unisonCount; ++i)
        {
            auto& osc = oscs[i];
            float s = 0.0f;

            switch (waveType)
            {
            case WaveType::Sine:
                s = std::sin(osc.phase);
                break;
            case WaveType::Saw:
                s = 1.0f - (osc.phase / juce::MathConstants<float>::pi);
                break;
            case WaveType::Triangle:
            {
                float norm = osc.phase / juce::MathConstants<float>::twoPi;
                s = (norm < 0.5f) ? (4.0f * norm - 1.0f) : (3.0f - 4.0f * norm);
                break;
            }
            case WaveType::Square:
                s = (osc.phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
                break;
            }

            osc.phase += osc.phaseInc;
            if (osc.phase > juce::MathConstants<float>::twoPi)
                osc.phase -= juce::MathConstants<float>::twoPi;

            out += s * scale;
        }

        return out * envLevel * velocity;
    }
};

//==============================================================================
class NewProjectAudioProcessor : public juce::AudioProcessor
{
public:
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool   acceptsMidi()   const override;
    bool   producesMidi()  const override;
    bool   isMidiEffect()  const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ---- Parameters ----
    juce::AudioParameterFloat* masterVolume = nullptr;
    juce::AudioParameterFloat* paramAttack = nullptr;  // 0.001..4.0  s
    juce::AudioParameterFloat* paramDecay = nullptr;  // 0.001..4.0  s
    juce::AudioParameterFloat* paramSustain = nullptr;  // 0.0..1.0
    juce::AudioParameterFloat* paramRelease = nullptr;  // 0.001..8.0  s
    juce::AudioParameterInt* paramWaveType = nullptr;  // 0..3
    juce::AudioParameterInt* paramUnison = nullptr;  // 1..8
    juce::AudioParameterFloat* paramDetune = nullptr;  // 0..100 cents

    std::atomic<int> activeVoiceCount{ 0 };

private:
    static constexpr int MAX_POLY = 16;
    SynthVoice voices[MAX_POLY];
    double     currentSampleRate = 44100.0;

    float timeToRate(float seconds) const
    {
        return (seconds <= 0.0001f) ? 1.0f
            : (1.0f / (float)(seconds * currentSampleRate));
    }

    SynthVoice* findFreeVoice();
    SynthVoice* findVoiceForNote(int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectAudioProcessor)
};