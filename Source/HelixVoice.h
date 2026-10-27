#pragma once

#include <atomic>
#include <cstdint>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "FilterMath.h"
#include "Lfo.h"
#include "Wavetable.h"

// Every synth needs a "sound" object; ours says it can play any note on any channel.
struct HelixSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Pointers to the knob values. They are read once per audio block.
struct VoiceParams
{
    std::atomic<float>* position   = nullptr;
    std::atomic<float>* attack     = nullptr;
    std::atomic<float>* decay      = nullptr;
    std::atomic<float>* sustain    = nullptr;
    std::atomic<float>* release    = nullptr;
    std::atomic<float>* filterType = nullptr;
    std::atomic<float>* cutoff     = nullptr;
    std::atomic<float>* resonance  = nullptr;

    // Filter envelope
    std::atomic<float>* fenvAttack  = nullptr;
    std::atomic<float>* fenvDecay   = nullptr;
    std::atomic<float>* fenvSustain = nullptr;
    std::atomic<float>* fenvRelease = nullptr;
    std::atomic<float>* fenvAmount  = nullptr;

    // LFO
    std::atomic<float>* lfoShape     = nullptr;
    std::atomic<float>* lfoTarget    = nullptr;
    std::atomic<float>* lfoAmount    = nullptr;
    std::atomic<float>* lfoRetrigger = nullptr;
    const Lfo::Clock* lfoClock = nullptr;

    // The wavetable to play during the current block (set by the processor before each block).
    const Wavetable* const* table = nullptr;
};

// One voice plays one note. The synth owns several, so chords can play.
// Signal path: wavetable oscillator -> filter -> volume envelope.
// The filter envelope and the LFO move the controls while the note plays.
class HelixVoice : public juce::SynthesiserVoice
{
public:
    explicit HelixVoice (const VoiceParams& p) : params (p) {}

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<HelixSound*> (sound) != nullptr;
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);

        if (newRate > 0.0)
        {
            filter.prepare ({ newRate, 512u, 1u });
            cutoffSmoother.reset (newRate, 0.02); // 20 ms glide so moving the knob doesn't crackle
        }
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        noteFrequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        phaseIncrement = (float) (noteFrequency / getSampleRate());
        noteTableLevel = Wavetable::levelForFrequency (noteFrequency, getSampleRate()); // keeps harmonics below Nyquist
        phase = 0.0f;
        level = velocity;

        // A restarted LFO begins at the start of its cycle, with its own random steps.
        noteLfoCycles = 0.0;
        randomSeed = ++noteCounter;
        startOrder = ++startCounter;

        adsr.setSampleRate (getSampleRate());
        filterEnvelope.setSampleRate (getSampleRate());
        updateEnvelopes();
        adsr.noteOn();
        filterEnvelope.noteOn();

        // Start the filter at its current setting (no sweep from an old value).
        filter.reset();
        updateFilter (true);
        currentCutoffHz = -1.0f;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff(); // let the release stage fade the note out
            filterEnvelope.noteOff();
        }
        else
        {
            adsr.reset();
            filterEnvelope.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        updateEnvelopes();
        updateFilter (false);
        const Wavetable& wavetable = **params.table;
        const float knobPosition = params.position->load();
        const float envAmount = params.fenvAmount->load();
        const int lfoShape = (int) params.lfoShape->load();
        const int lfoTarget = (int) params.lfoTarget->load();
        const float lfoAmount = params.lfoAmount->load();
        const bool restartLfo = params.lfoRetrigger->load() > 0.5f;
        const auto& clock = *params.lfoClock;

        for (int i = 0; i < numSamples; ++i)
        {
            // The LFO either runs from this note's start, or follows the shared clock.
            lfoCycles = restartLfo ? noteLfoCycles
                                   : clock.cyclesAtBlockStart + (double) (startSample + i) * clock.cyclesPerSample;
            noteLfoCycles += clock.cyclesPerSample;

            const float lfo = lfoAmount > 0.0f ? Lfo::valueAt (lfoShape, lfoCycles, restartLfo ? randomSeed : 0u) * lfoAmount
                                               : 0.0f;

            // Filter: the knob setting, moved by the filter envelope and (if chosen) the LFO, in octaves.
            float octaves = filterEnvelope.getNextSample() * envAmount * Lfo::cutoffOctaves;
            if (lfoTarget == Lfo::cutoff)
                octaves += lfo * Lfo::cutoffOctaves;

            const float knobCutoff = cutoffSmoother.getNextValue();
            const float cutoffHz = FilterMath::clampCutoff (octaves != 0.0f ? knobCutoff * std::exp2 (octaves) : knobCutoff,
                                                            getSampleRate());
            if (cutoffHz != currentCutoffHz)
            {
                filter.setCutoffFrequency (cutoffHz);
                currentCutoffHz = cutoffHz;
            }

            // Oscillator
            currentPosition = lfoTarget == Lfo::position ? juce::jlimit (0.0f, 1.0f, knobPosition + lfo) : knobPosition;

            float increment = phaseIncrement;
            int tableLevel = noteTableLevel;
            if (lfoTarget == Lfo::pitch && lfo != 0.0f)
            {
                // A higher pitch needs fewer harmonics, so the band-limited level is chosen again.
                const double frequency = noteFrequency * std::exp2 ((double) (lfo * Lfo::pitchSemitones) / 12.0);
                increment = (float) (frequency / getSampleRate());
                tableLevel = Wavetable::levelForFrequency (frequency, getSampleRate());
            }

            float gain = level * 0.3f;
            if (lfoTarget == Lfo::volume)
                gain *= 1.0f - 0.5f * (lfoAmount - lfo); // dips down from full volume, never above it

            const float raw = wavetable.getSample (phase, currentPosition, tableLevel) * gain;
            const float sample = filter.processSample (0, raw) * filterGain * adsr.getNextSample();

            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, startSample + i, sample);

            phase += increment;
            if (phase >= 1.0f)
                phase -= 1.0f;

            if (! adsr.isActive())
            {
                filterEnvelope.reset();
                clearCurrentNote(); // the note has fully faded out, so free this voice
                break;
            }
        }
    }

    // What this voice is doing right now, for the live displays.
    std::uint64_t getStartOrder() const noexcept  { return startOrder; }
    float getCurrentCutoff() const noexcept       { return currentCutoffHz; }
    float getCurrentPosition() const noexcept     { return currentPosition; }
    double getLfoCycles() const noexcept          { return lfoCycles; }

private:
    void updateEnvelopes()
    {
        adsr.setParameters ({ params.attack->load(), params.decay->load(),
                              params.sustain->load(), params.release->load() });
        filterEnvelope.setParameters ({ params.fenvAttack->load(), params.fenvDecay->load(),
                                        params.fenvSustain->load(), params.fenvRelease->load() });
    }

    // Reads the filter knobs. jumpToTarget skips the glide (used when a note starts).
    void updateFilter (bool jumpToTarget)
    {
        const int type = (int) params.filterType->load();
        const float q = FilterMath::resonanceToQ (params.resonance->load());
        const float cutoff = params.cutoff->load();

        filter.setType (type == FilterMath::highPass ? juce::dsp::StateVariableTPTFilterType::highpass
                      : type == FilterMath::bandPass ? juce::dsp::StateVariableTPTFilterType::bandpass
                                                     : juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setResonance (q);

        // JUCE's band-pass gets louder as Q rises; scaling by 1/Q keeps its peak at 0 dB.
        filterGain = type == FilterMath::bandPass ? 1.0f / q : 1.0f;

        if (jumpToTarget)
            cutoffSmoother.setCurrentAndTargetValue (cutoff);
        else
            cutoffSmoother.setTargetValue (cutoff);
    }

    // Shared by all voices: counts notes, so the newest one can be found and each gets its own random steps.
    static inline std::atomic<std::uint64_t> startCounter { 0 };
    static inline std::atomic<std::uint32_t> noteCounter { 0 };

    VoiceParams params;
    juce::ADSR adsr, filterEnvelope;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmoother { 20000.0f };

    double noteFrequency = 440.0;
    int noteTableLevel = 1;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float level = 0.0f;
    float filterGain = 1.0f;

    double noteLfoCycles = 0.0, lfoCycles = 0.0;
    std::uint32_t randomSeed = 0;
    std::uint64_t startOrder = 0;
    float currentCutoffHz = -1.0f;
    float currentPosition = 0.0f;
};
