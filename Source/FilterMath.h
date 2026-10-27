#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

// Shared by the audio code and the on-screen filter curve, so the curve you see
// is calculated the same way as the filter you hear.
namespace FilterMath
{
    // Same order as the "Filter Type" choices in the processor.
    enum Type { lowPass = 0, highPass = 1, bandPass = 2 };

    // The Emphasis knob (0..1) is turned into the filter's Q value here.
    // 0 gives 0.707 (a flat, standard 12 dB/octave filter); 1 gives roughly 9.9 (a strong peak).
    inline float resonanceToQ (float resonance)
    {
        return 0.7071f * std::pow (14.0f, resonance);
    }

    // The filter must stay below half the sample rate, so keep the cutoff safely under it.
    inline float clampCutoff (float cutoffHz, double sampleRate)
    {
        return juce::jlimit (20.0f, (float) (sampleRate * 0.45), cutoffHz);
    }

    // Linear gain of the filter at freqHz. This is the transfer function of the
    // state-variable filter, with the frequency warping JUCE's filter uses.
    // The band-pass is the 0 dB version (peak gain of 1 at the cutoff).
    inline float magnitudeAt (int type, float freqHz, float cutoffHz, float q, double sampleRate)
    {
        const double pi = juce::MathConstants<double>::pi;
        const double w = std::tan (pi * freqHz / sampleRate) / std::tan (pi * cutoffHz / sampleRate);

        const double re = 1.0 - w * w;
        const double im = w / q;
        const double denominator = std::sqrt (re * re + im * im);

        switch (type)
        {
            case highPass: return (float) ((w * w) / denominator);
            case bandPass: return (float) ((w / q) / denominator);
            default:       return (float) (1.0 / denominator);
        }
    }
}
