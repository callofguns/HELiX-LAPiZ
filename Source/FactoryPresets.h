#pragma once

#include <array>
#include <initializer_list>
#include <utility>
#include <vector>

// The sounds built into LAPiZ. Each one starts from the default settings and changes only
// the controls listed. Values are real values, as shown under the knobs: seconds, Hz, 0..1,
// or the position in a list (filter type, LFO shape and target, note lengths).
namespace FactoryPresets
{
    using Setting = std::pair<const char*, float>;

    struct Preset
    {
        const char* category;
        const char* name;
        int table;                     // index into FactoryTables::list
        std::vector<Setting> settings;
    };

    // Positions in the Basic Shapes table.
    inline constexpr float sine = 0.0f, triangle = 1.0f / 3.0f, saw = 2.0f / 3.0f, square = 1.0f;

    // Positions in the lists, as named in Lfo.h and the processor.
    inline constexpr float lfoSine = 0, lfoTriangle = 1, lfoSawDown = 2, lfoSawUp = 3;
    inline constexpr float toBrightness = 0, toWaveShape = 1, toVolume = 2, toPitch = 3;
    inline constexpr float div1_4 = 7, div1_8D = 9, div1_8 = 10, div1_16 = 13;
    enum { basicShapes = 0, pulseWidth = 1, syncSaw = 2, softSaw = 3, wavefold = 4, fmMetal = 5, bitcrush = 6 };

    inline const std::vector<Preset>& list()
    {
        static const std::vector<Preset> presets {
            { "", "Init", basicShapes, {} },

            // ---- Bass ----
            { "Bass", "Sub Bass", basicShapes,
              { { "position", sine }, { "attack", 0.005f }, { "decay", 0.3f }, { "sustain", 1.0f }, { "release", 0.12f },
                { "volume", 0.8f } } },
            { "Bass", "Reese", basicShapes,
              { { "position", saw }, { "sustain", 1.0f }, { "release", 0.15f }, { "cutoff", 900.0f }, { "resonance", 0.2f },
                { "unisonVoices", 4.0f }, { "unisonDetune", 0.35f }, { "unisonWidth", 0.3f },
                { "distOn", 1.0f }, { "distDrive", 0.3f }, { "distTone", 0.5f }, { "distMix", 0.6f } } },
            { "Bass", "Wobble", basicShapes,
              { { "position", saw }, { "sustain", 1.0f }, { "release", 0.1f }, { "cutoff", 300.0f }, { "resonance", 0.45f },
                { "lfoTarget", toBrightness }, { "lfoShape", lfoSine }, { "lfoAmount", 0.45f }, { "lfoDivision", div1_8 },
                { "lfoRetrigger", 1.0f }, { "unisonVoices", 2.0f }, { "unisonDetune", 0.15f }, { "unisonWidth", 0.2f },
                { "distOn", 1.0f }, { "distDrive", 0.5f } } },
            { "Bass", "Growl", fmMetal,
              { { "position", 0.3f }, { "sustain", 1.0f }, { "release", 0.1f }, { "cutoff", 2500.0f }, { "resonance", 0.3f },
                { "lfoTarget", toWaveShape }, { "lfoShape", lfoTriangle }, { "lfoAmount", 0.35f }, { "lfoDivision", div1_16 },
                { "lfoRetrigger", 1.0f }, { "distOn", 1.0f }, { "distDrive", 0.6f }, { "distTone", 0.6f } } },

            // ---- Lead ----
            { "Lead", "Supersaw", basicShapes,
              { { "position", saw }, { "decay", 0.3f }, { "sustain", 0.8f }, { "release", 0.35f }, { "cutoff", 9000.0f }, { "volume", 0.5f },
                { "unisonVoices", 7.0f }, { "unisonDetune", 0.45f }, { "unisonWidth", 1.0f },
                { "delayOn", 1.0f }, { "delayDivision", div1_8D }, { "delayFeedback", 0.3f }, { "delayMix", 0.2f },
                { "reverbOn", 1.0f }, { "reverbSize", 0.5f }, { "reverbMix", 0.2f } } },
            { "Lead", "Sync Lead", syncSaw,
              { { "position", 0.5f }, { "sustain", 0.9f }, { "release", 0.25f }, { "cutoff", 3000.0f },
                { "fenvAmount", 0.25f }, { "fenvDecay", 0.5f }, { "fenvSustain", 0.4f },
                { "unisonVoices", 2.0f }, { "unisonDetune", 0.1f },
                { "delayOn", 1.0f }, { "delayFeedback", 0.25f }, { "delayMix", 0.2f } } },
            { "Lead", "Square Lead", basicShapes,
              { { "position", square }, { "sustain", 0.9f }, { "release", 0.2f }, { "cutoff", 4000.0f }, { "resonance", 0.2f },
                { "lfoTarget", toPitch }, { "lfoShape", lfoSine }, { "lfoAmount", 0.02f }, { "lfoSync", 0.0f }, { "lfoRate", 5.5f },
                { "chorusOn", 1.0f } } },

            // ---- Pad ----
            { "Pad", "Warm Pad", softSaw,
              { { "position", 0.4f }, { "attack", 1.2f }, { "decay", 1.0f }, { "sustain", 0.8f }, { "release", 2.0f },
                { "cutoff", 2200.0f }, { "unisonVoices", 5.0f }, { "unisonDetune", 0.3f }, { "unisonWidth", 0.9f },
                { "lfoTarget", toWaveShape }, { "lfoAmount", 0.2f }, { "lfoSync", 0.0f }, { "lfoRate", 0.15f },
                { "chorusOn", 1.0f }, { "chorusMix", 0.6f },
                { "reverbOn", 1.0f }, { "reverbSize", 0.8f }, { "reverbMix", 0.45f } } },
            { "Pad", "Glass Pad", wavefold,
              { { "position", 0.2f }, { "attack", 0.8f }, { "decay", 1.0f }, { "sustain", 0.8f }, { "release", 2.5f }, { "volume", 0.55f },
                { "cutoff", 6000.0f }, { "lfoTarget", toWaveShape }, { "lfoAmount", 0.3f }, { "lfoSync", 0.0f }, { "lfoRate", 0.1f },
                { "chorusOn", 1.0f },
                { "reverbOn", 1.0f }, { "reverbSize", 0.9f }, { "reverbDamping", 0.3f }, { "reverbMix", 0.5f } } },

            // ---- Pluck ----
            { "Pluck", "Soft Pluck", basicShapes,
              { { "position", 0.5f }, { "attack", 0.002f }, { "decay", 0.35f }, { "sustain", 0.0f }, { "release", 0.3f },
                { "cutoff", 600.0f }, { "resonance", 0.25f },
                { "fenvAmount", 0.45f }, { "fenvAttack", 0.001f }, { "fenvDecay", 0.25f }, { "fenvSustain", 0.0f },
                { "delayOn", 1.0f }, { "delayDivision", div1_8D }, { "delayFeedback", 0.35f }, { "delayMix", 0.25f },
                { "reverbOn", 1.0f }, { "reverbMix", 0.25f } } },
            { "Pluck", "Digital Pluck", bitcrush,
              { { "position", 0.5f }, { "attack", 0.001f }, { "decay", 0.25f }, { "sustain", 0.0f }, { "release", 0.2f },
                { "cutoff", 1200.0f }, { "fenvAmount", 0.4f }, { "fenvDecay", 0.2f }, { "fenvSustain", 0.0f },
                { "delayOn", 1.0f }, { "delayDivision", div1_16 }, { "delayFeedback", 0.3f }, { "delayMix", 0.2f } } },

            // ---- Keys ----
            { "Keys", "PWM Keys", pulseWidth,
              { { "position", 0.3f }, { "attack", 0.005f }, { "decay", 0.8f }, { "sustain", 0.5f }, { "release", 0.5f },
                { "cutoff", 3500.0f }, { "lfoTarget", toWaveShape }, { "lfoShape", lfoTriangle }, { "lfoAmount", 0.25f },
                { "lfoSync", 0.0f }, { "lfoRate", 0.6f }, { "chorusOn", 1.0f } } },

            // ---- FX ----
            { "FX", "Siren", basicShapes,
              { { "position", 0.2f }, { "sustain", 1.0f }, { "release", 0.4f }, { "volume", 0.55f },
                { "lfoTarget", toPitch }, { "lfoShape", lfoTriangle }, { "lfoAmount", 0.5f }, { "lfoSync", 0.0f }, { "lfoRate", 0.5f },
                { "reverbOn", 1.0f }, { "reverbMix", 0.3f } } }
        };
        return presets;
    }
}
