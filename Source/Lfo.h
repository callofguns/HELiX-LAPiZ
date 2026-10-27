#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <juce_core/juce_core.h>

// The LFO ("low frequency oscillator") is a slow, repeating movement that turns
// another control up and down for you. Shared by the audio code and the on-screen drawing.
namespace Lfo
{
    // Same order as the "LFO Shape" choices in the processor.
    enum Shape { sine = 0, triangle, sawDown, sawUp, square, random };
    static constexpr std::array<const char*, 6> shapeNames { "Sine", "Triangle", "Saw Down", "Saw Up", "Square", "Random" };

    // Same order as the "LFO Target" choices. The names match the knobs they move.
    enum Target { cutoff = 0, position, volume, pitch };
    static constexpr std::array<const char*, 4> targetNames { "Brightness", "Wave Shape", "Volume", "Pitch" };

    // How far a full Wobble moves each target.
    inline constexpr float cutoffOctaves = 10.0f;   // also used by the filter envelope's Sweep
    inline constexpr float pitchSemitones = 24.0f;

    // Tempo-synced speeds, slowest first. D = dotted (1.5x as long), T = triplet (2/3 as long).
    struct Division { const char* name; double beats; };
    static constexpr std::array<Division, 15> divisions { {
        { "1/1 D", 6.0 },  { "1/1", 4.0 },  { "1/1 T", 8.0 / 3.0 },
        { "1/2 D", 3.0 },  { "1/2", 2.0 },  { "1/2 T", 4.0 / 3.0 },
        { "1/4 D", 1.5 },  { "1/4", 1.0 },  { "1/4 T", 2.0 / 3.0 },
        { "1/8 D", 0.75 }, { "1/8", 0.5 },  { "1/8 T", 1.0 / 3.0 },
        { "1/16 D", 0.375 }, { "1/16", 0.25 }, { "1/16 T", 1.0 / 6.0 }
    } };
    inline constexpr int defaultDivision = 7; // 1/4

    // A repeatable "random" number from -1 to 1 for each cycle, so every voice that
    // shares the same cycle count hears the same random step.
    inline float randomStep (std::int64_t cycle, std::uint32_t seed) noexcept
    {
        auto x = ((std::uint64_t) cycle + 1u) * 0x9E3779B97F4A7C15ull + (std::uint64_t) seed * 0xBF58476D1CE4E5B9ull;
        x ^= x >> 31; x *= 0x94D049BB133111EBull; x ^= x >> 29;
        return (float) (x >> 40) / (float) (1ull << 23) - 1.0f;
    }

    // The LFO's value (-1 to 1) after "cycles" full cycles have passed.
    inline float valueAt (int shape, double cycles, std::uint32_t seed = 0) noexcept
    {
        const double whole = std::floor (cycles);
        const float p = (float) (cycles - whole); // 0..1 inside the current cycle

        switch (shape)
        {
            case triangle: return p < 0.25f ? 4.0f * p : p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f;
            case sawDown:  return 1.0f - 2.0f * p;
            case sawUp:    return 2.0f * p - 1.0f;
            case square:   return p < 0.5f ? 1.0f : -1.0f;
            case random:   return randomStep ((std::int64_t) whole, seed);
            default:       return std::sin (juce::MathConstants<float>::twoPi * p);
        }
    }

    // Where every voice's LFO is when it is not restarted on each note.
    // The processor fills this in at the start of each audio block.
    struct Clock
    {
        double cyclesAtBlockStart = 0.0;
        double cyclesPerSample = 0.0;
    };
}
