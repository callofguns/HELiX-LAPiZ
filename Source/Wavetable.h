#pragma once

#include <cmath>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

// A wavetable is a list of single-cycle waveforms ("frames").
// Morphing smoothly between frames is what "scanning" a wavetable means.
//
// To avoid harsh aliasing on high notes, every frame is stored several times ("levels"),
// each with fewer high harmonics than the one before. A note picks the richest level
// whose harmonics all stay below half the sample rate (the Nyquist limit).
//   level 0       : the shape exactly as drawn (only used for drawing it on screen)
//   levels 1 .. 11: band-limited copies, from 1023 harmonics down to 1, with any DC offset removed
//
// A Wavetable never changes once built. To change the sound, the processor builds a new one and swaps it in.
class Wavetable
{
public:
    static constexpr int tableSize = 2048;   // samples per frame (the same as Serum, so its files load)
    static constexpr int maxFrames = 256;
    static constexpr int numBandLimitedLevels = 11;
    static constexpr int numLevels = numBandLimitedLevels + 1;

    using Frame = std::vector<float>; // tableSize samples, from -1 to 1

    // frames: 1 to maxFrames frames of tableSize samples each (extra frames are ignored).
    // frameNames: optional names to show under the display, one per frame.
    explicit Wavetable (const std::vector<Frame>& frames, juce::StringArray frameNames = {})
        : numFrames (juce::jlimit (1, maxFrames, (int) frames.size())),
          names (std::move (frameNames)),
          data ((size_t) numFrames * numLevels * (tableSize + 1), 0.0f)
    {
        juce::dsp::FFT fft (order);
        std::vector<float> spectrum ((size_t) tableSize * 2), work ((size_t) tableSize * 2);

        for (int frame = 0; frame < numFrames; ++frame)
        {
            const Frame* source = frame < (int) frames.size() ? &frames[(size_t) frame] : nullptr;

            // Level 0: the shape as drawn.
            float* original = levelData (frame, 0);
            for (int i = 0; i < tableSize; ++i)
                original[i] = source != nullptr && i < (int) source->size() ? (*source)[(size_t) i] : 0.0f;
            original[tableSize] = original[0];

            // Work out how much of each harmonic the shape contains...
            std::fill (spectrum.begin(), spectrum.end(), 0.0f);
            std::copy (original, original + tableSize, spectrum.begin());
            fft.performRealOnlyForwardTransform (spectrum.data());

            // ...then rebuild it from only the harmonics each level allows.
            for (int level = 1; level < numLevels; ++level)
            {
                const int keep = maxHarmonics (level);
                work = spectrum;
                for (int bin = 0; bin < tableSize; ++bin)
                    if (bin == 0 || (bin > keep && bin < tableSize - keep))
                        work[(size_t) bin * 2] = work[(size_t) bin * 2 + 1] = 0.0f;

                fft.performRealOnlyInverseTransform (work.data());

                float* out = levelData (frame, level);
                std::copy (work.begin(), work.begin() + tableSize, out);
                out[tableSize] = out[0]; // extra point so interpolation never reads past the end
            }
        }
    }

    int getNumFrames() const noexcept { return numFrames; }
    const juce::StringArray& getFrameNames() const noexcept { return names; }

    // One frame exactly as drawn (tableSize samples).
    const float* getFrame (int frame) const noexcept { return levelData (juce::jlimit (0, numFrames - 1, frame), 0); }

    // How many harmonics a band-limited level keeps (level 1 and up).
    static int maxHarmonics (int level) noexcept
    {
        return juce::jmin (tableSize / 2 - 1, (tableSize / 2) >> (level - 1));
    }

    // The richest level that has no harmonic above half the sample rate for a note at frequencyHz.
    static int levelForFrequency (double frequencyHz, double sampleRate) noexcept
    {
        const double nyquist = 0.5 * sampleRate;

        for (int level = 1; level < numLevels; ++level)
            if ((double) maxHarmonics (level) * frequencyHz <= nyquist)
                return level;

        return numLevels - 1;
    }

    // phase: 0..1 position inside the cycle (must be below 1). position: 0..1 across the frames.
    float getSample (float phase, float position, int level = 0) const noexcept
    {
        const float framePos = juce::jlimit (0.0f, 1.0f, position) * (float) (numFrames - 1);
        const int frameA = (int) framePos;
        const int frameB = juce::jmin (frameA + 1, numFrames - 1);
        const float frameFrac = framePos - (float) frameA;

        const float tablePos = phase * (float) tableSize;
        const int index = (int) tablePos;
        const float indexFrac = tablePos - (float) index;

        const float a = readFrame (frameA, level, index, indexFrac);
        const float b = readFrame (frameB, level, index, indexFrac);
        return a + frameFrac * (b - a);
    }

private:
    static constexpr int order = 11; // 2^11 = tableSize
    static_assert ((1 << order) == tableSize);

    float* levelData (int frame, int level) noexcept
    {
        return data.data() + ((size_t) frame * numLevels + (size_t) level) * (tableSize + 1);
    }

    const float* levelData (int frame, int level) const noexcept
    {
        return data.data() + ((size_t) frame * numLevels + (size_t) level) * (tableSize + 1);
    }

    float readFrame (int frame, int level, int index, float frac) const noexcept
    {
        const float* table = levelData (frame, level);
        return table[index] + frac * (table[index + 1] - table[index]);
    }

    int numFrames;
    juce::StringArray names;
    std::vector<float> data;
};
