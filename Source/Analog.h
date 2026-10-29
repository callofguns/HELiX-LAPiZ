#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "FilterMath.h"

// The Analog knob (0..1): the small imperfections that make old hardware synths sound alive.
//   - Drift: every unison copy wanders slowly and slightly out of tune.
//   - Note variation: each note starts a little differently (pitch, brightness, level, timing).
//   - Warm filter: the filter saturates when pushed, so strong Emphasis gets thick instead of whistly.
//   - Soft saturation: gentle warmth on the synth's output, like running through analog circuits.
// At 0 everything is exactly the clean digital sound.
namespace Analog
{
    inline constexpr float maxDriftCents = 7.0f;      // slow wandering of each copy, at full Analog
    inline constexpr float maxNoteCents = 4.0f;       // a fixed offset picked for each note
    inline constexpr float maxNoteOctaves = 0.15f;    // brightness offset picked for each note
    inline constexpr float maxNoteLevel = 0.12f;      // loudness offset for each note (about +-1 dB)
    inline constexpr float maxNoteTiming = 0.15f;     // envelope times stretched or shortened by up to 15 %

    // Furthest a copy's pitch can be from the note, in cents (for choosing how many harmonics are safe).
    inline float maxPitchOffsetCents (float amount) { return amount * (maxDriftCents + maxNoteCents); }

    // A slow random wander between -1 and 1: it glides towards a new random target every so often.
    class Drift
    {
    public:
        void reset (juce::Random& random)
        {
            value = random.nextFloat() * 2.0f - 1.0f;
            target = random.nextFloat() * 2.0f - 1.0f;
            secondsToNextTarget = 0.2f + 0.5f * random.nextFloat();
        }

        // Moves on by `seconds` and returns the new value.
        float advance (float seconds, juce::Random& random)
        {
            secondsToNextTarget -= seconds;
            if (secondsToNextTarget <= 0.0f)
            {
                target = random.nextFloat() * 2.0f - 1.0f;
                secondsToNextTarget = 0.2f + 0.5f * random.nextFloat();
            }
            value += (target - value) * (1.0f - std::exp (-seconds / glideSeconds));
            return value;
        }

    private:
        static constexpr float glideSeconds = 0.6f;
        float value = 0.0f, target = 0.0f, secondsToNextTarget = 0.0f;
    };

    // The same state-variable filter as JUCE's StateVariableTPTFilter (left and right), plus warmth:
    // the input is driven into soft saturation, and the filter's resonating part is softly limited,
    // like the circuits in analog filters. With warmth at 0 it gives exactly the same output as JUCE's.
    class WarmFilter
    {
    public:
        void prepare (double newSampleRate) { sampleRate = newSampleRate; update(); reset(); }
        void reset() { s1 = {}; s2 = {}; }

        void setType (int newType)              { type = newType; }
        void setResonance (float newQ)          { q = newQ; update(); }
        void setCutoffFrequency (float newHz)   { cutoff = newHz; update(); }
        void setWarmth (float newWarmth)        { warmth = newWarmth; }

        float processSample (int channel, float x)
        {
            auto& ls1 = s1[(size_t) channel];
            auto& ls2 = s2[(size_t) channel];

            if (warmth > 0.0f)
                x += warmth * (soft (x, inputDrive) - x);

            const float yHP = h * (x - ls1 * (g + R2) - ls2);
            const float yBP = yHP * g + ls1;
            ls1 = yHP * g + yBP;
            const float yLP = yBP * g + ls2;
            ls2 = yBP * g + yLP;

            if (warmth > 0.0f)
                ls1 += warmth * (soft (ls1, stateDrive) - ls1);

            return type == FilterMath::highPass ? yHP : type == FilterMath::bandPass ? yBP : yLP;
        }

    private:
        // tanh, scaled so small signals pass at the same level and only big ones are rounded off.
        static float soft (float x, float drive) { return std::tanh (x * drive) / drive; }

        void update()
        {
            g  = (float) std::tan (juce::MathConstants<double>::pi * cutoff / sampleRate);
            R2 = (float) (1.0 / q);
            h  = (float) (1.0 / (1.0 + R2 * g + g * g));
        }

        static constexpr float inputDrive = 0.9f, stateDrive = 0.15f;
        double sampleRate = 44100.0;
        float cutoff = 1000.0f, q = 0.7071f, warmth = 0.0f;
        float g = 0.0f, R2 = 0.0f, h = 0.0f;
        int type = FilterMath::lowPass;
        std::array<float, 2> s1 {}, s2 {};
    };

    // Gentle saturation on the whole synth, before the effects. It is slightly lopsided, which adds
    // the even harmonics that make tube and tape circuits sound warm; a blocker removes the offset
    // that comes with that. Blended in by the Analog amount.
    // It runs at 4x the sample rate so the new harmonics don't alias. At 0 it is skipped entirely,
    // so the sound is untouched; turning it on or off crossfades over one block, so it doesn't click.
    class Saturator
    {
    public:
        void prepare (double sampleRate, int maxBlockSize)
        {
            oversampling.initProcessing ((size_t) maxBlockSize);
            dryCopy.setSize (2, maxBlockSize);
            const double oversampledRate = sampleRate * (double) oversampling.getOversamplingFactor();
            blockerCoefficient = (float) (1.0 - juce::MathConstants<double>::twoPi * 10.0 / oversampledRate); // 10 Hz
            reset();
        }

        void reset()
        {
            oversampling.reset();
            lastIn = {};
            lastOut = {};
            wasOn = false;
        }

        void process (juce::AudioBuffer<float>& buffer, float amount)
        {
            const bool on = amount > 0.0f;
            if (! on && ! wasOn)
                return;

            const float used = on ? amount : lastAmount; // while fading out, keep the last setting
            const int channels = juce::jmin (2, buffer.getNumChannels());
            const int numSamples = buffer.getNumSamples();
            if (numSamples > dryCopy.getNumSamples())
                dryCopy.setSize (2, numSamples, false, false, true);
            for (int c = 0; c < channels; ++c)
                dryCopy.copyFrom (c, 0, buffer, c, 0, numSamples);

            juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) channels, (size_t) numSamples);
            auto up = oversampling.processSamplesUp (block);
            const float offset = std::tanh (drive * bias);
            for (size_t c = 0; c < up.getNumChannels(); ++c)
            {
                auto* data = up.getChannelPointer (c);
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                {
                    const float x = data[i];
                    const float saturated = (std::tanh (drive * (x + bias)) - offset) / drive;

                    // Remove the slow offset (DC) the lopsided curve adds.
                    const float blocked = saturated - lastIn[c] + blockerCoefficient * lastOut[c];
                    lastIn[c] = saturated;
                    lastOut[c] = blocked;

                    data[i] = x + used * (blocked - x);
                }
            }
            oversampling.processSamplesDown (block);

            if (on != wasOn)
            {
                for (int c = 0; c < channels; ++c)
                {
                    auto* data = buffer.getWritePointer (c);
                    const auto* dry = dryCopy.getReadPointer (c);
                    for (int i = 0; i < numSamples; ++i)
                    {
                        const float t = (float) (i + 1) / (float) numSamples;
                        data[i] = dry[i] + (on ? t : 1.0f - t) * (data[i] - dry[i]);
                    }
                }
                if (! on)
                    reset();
            }

            wasOn = on;
            lastAmount = used;
        }

    private:
        static constexpr float drive = 1.6f, bias = 0.15f;
        juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
        juce::AudioBuffer<float> dryCopy;
        float blockerCoefficient = 0.999f, lastAmount = 0.0f;
        std::array<float, 2> lastIn {}, lastOut {};
        bool wasOn = false;
    };
}
