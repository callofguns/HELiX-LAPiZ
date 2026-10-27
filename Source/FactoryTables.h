#pragma once

#include <array>
#include <cmath>
#include <functional>
#include "Wavetable.h"

// The wavetables built into LAPiZ. They are calculated from formulas when chosen,
// so they take no space in the plugin file.
namespace FactoryTables
{
    struct Info { const char* category; const char* name; };

    static constexpr std::array<Info, 7> list { {
        { "Basic",   "Basic Shapes" },
        { "Analog",  "Pulse Width" },
        { "Analog",  "Sync Saw" },
        { "Analog",  "Soft Saw" },
        { "Digital", "Wavefold" },
        { "Digital", "FM Metal" },
        { "Digital", "Bitcrush" }
    } };

    namespace detail
    {
        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        constexpr int framesPerTable = 64;

        // Makes `count` frames; shape(p, t) gives the value at p (0..1 in the cycle) for t (0..1 across the table).
        inline std::vector<Wavetable::Frame> build (int count, const std::function<float (float, float)>& shape,
                                                    bool normalise = true)
        {
            std::vector<Wavetable::Frame> frames ((size_t) count, Wavetable::Frame ((size_t) Wavetable::tableSize));
            for (int f = 0; f < count; ++f)
            {
                const float t = count > 1 ? (float) f / (float) (count - 1) : 0.0f;
                auto& frame = frames[(size_t) f];
                for (int i = 0; i < Wavetable::tableSize; ++i)
                    frame[(size_t) i] = shape ((float) i / (float) Wavetable::tableSize, t);

                if (! normalise)
                    continue;

                // Centre the wave on the middle line (a thin pulse sits mostly below it), then make
                // every frame peak at full size, so scanning doesn't change the loudness much.
                float mean = 0.0f;
                for (auto v : frame) mean += v;
                mean /= (float) Wavetable::tableSize;

                float peak = 0.0f;
                for (auto& v : frame)
                {
                    v -= mean;
                    peak = std::max (peak, std::abs (v));
                }

                if (peak > 0.0f)
                    for (auto& v : frame)
                        v /= peak;
            }
            return frames;
        }

        inline float saw (float p)    { return 2.0f * p - 1.0f; }
        inline float square (float p) { return p < 0.5f ? 1.0f : -1.0f; }
        inline float triangle (float p) { return p < 0.25f ? 4.0f * p : p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f; }
    }

    // Builds the table at `index` in `list`.
    inline Wavetable make (int index)
    {
        using namespace detail;

        switch (index)
        {
            case 1: // Pulse Width: a square that narrows to a thin pulse
                return Wavetable (build (framesPerTable, [] (float p, float t) { return p < 0.5f - 0.46f * t ? 1.0f : -1.0f; }));

            case 2: // Sync Saw: a saw restarting inside each cycle, at 1x up to 8x the note's speed (hard sync)
                return Wavetable (build (framesPerTable, [] (float p, float t)
                {
                    const float ratio = 1.0f + 7.0f * t;
                    const float x = p * ratio;
                    return saw (x - std::floor (x));
                }));

            case 3: // Soft Saw: from a single harmonic up to a full, bright saw
                return Wavetable (build (framesPerTable, [] (float p, float t)
                {
                    const int harmonics = 1 + juce::roundToInt (t * t * 127.0f);
                    float sum = 0.0f;
                    for (int k = 1; k <= harmonics; ++k)
                        sum += std::sin (twoPi * (float) k * p) / (float) k;
                    return -sum; // same direction as LAPiZ's saw (rising)
                }));

            case 4: // Wavefold: a sine driven harder and harder, so it folds back on itself
                return Wavetable (build (framesPerTable, [] (float p, float t)
                {
                    const float drive = 1.0f + 7.0f * t;
                    return std::sin (juce::MathConstants<float>::halfPi * drive * std::sin (twoPi * p));
                }));

            case 5: // FM Metal: a sine bent by another sine at 3x its speed (frequency modulation)
                return Wavetable (build (framesPerTable, [] (float p, float t)
                {
                    return std::sin (twoPi * p + 6.0f * t * std::sin (twoPi * 3.0f * p));
                }));

            case 6: // Bitcrush: a saw losing detail, fewer steps across and fewer levels up and down
                return Wavetable (build (framesPerTable, [] (float p, float t)
                {
                    const float steps = std::exp2 (10.0f - 7.0f * t);  // 1024 down to 8 steps across
                    const float levels = std::exp2 (6.0f - 4.0f * t);  // 64 down to 4 levels
                    const float held = std::floor (p * steps) / steps;
                    return std::round (saw (held) * levels) / levels;
                }));

            default: // Basic Shapes: the four classic waves
                return Wavetable (build (4, [] (float p, float t)
                {
                    const int which = juce::roundToInt (t * 3.0f);
                    return which == 0 ? std::sin (twoPi * p) : which == 1 ? triangle (p) : which == 2 ? saw (p) : square (p);
                }, false), juce::StringArray { "Sine", "Triangle", "Saw", "Square" });
        }
    }
}
