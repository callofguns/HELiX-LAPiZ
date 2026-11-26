#pragma once

#include <juce_core/juce_core.h>
#include "FactoryPresets.h"

// "Surprise me": makes a new sound at random, but always a musical one in LAPiZ's style (soft,
// magical, mid-range). It picks a kind of sound first (pad, pluck, keys, bell or lead), then picks
// every setting within ranges that suit that kind, the same way the built-in presets are made.
// Settings it doesn't mention stay at their defaults, like a built-in preset.
namespace Surprise
{
    struct Sound
    {
        juce::String name;   // e.g. "Velvet Bloom"
        int table = 0;       // index into FactoryTables::list
        std::vector<std::pair<juce::String, float>> settings; // parameter id, real value
    };

    inline Sound make (juce::Random& random)
    {
        using namespace FactoryPresets;

        auto between = [&random] (float low, float high) { return low + random.nextFloat() * (high - low); };
        // For times and frequencies: even spread on a log scale, so 100 ms and 1 s are equally likely.
        auto logBetween = [&random] (float low, float high) { return low * std::pow (high / low, random.nextFloat()); };
        auto chance = [&random] (float probability) { return random.nextFloat() < probability; };
        auto pick = [&random] (std::initializer_list<int> options)
        {
            return *(options.begin() + random.nextInt ((int) options.size()));
        };

        Sound sound;
        auto set = [&sound] (const char* id, float value) { sound.settings.emplace_back (id, value); };

        enum Kind { pad, pluck, keys, bell, lead };
        const float roll = random.nextFloat();
        const Kind kind = roll < 0.35f ? pad : roll < 0.65f ? pluck : roll < 0.82f ? keys : roll < 0.92f ? bell : lead;

        // The wavetable. Bitcrush and Sync Saw are the edgiest, so they turn up least.
        switch (kind)
        {
            case pad:   sound.table = pick ({ softSaw, softSaw, wavefold, pulseWidth, basicShapes, syncSaw }); break;
            case pluck: sound.table = pick ({ basicShapes, basicShapes, wavefold, fmMetal, pulseWidth, bitcrush }); break;
            case keys:  sound.table = pick ({ pulseWidth, softSaw, basicShapes, fmMetal }); break;
            case bell:  sound.table = pick ({ fmMetal, fmMetal, wavefold }); break;
            case lead:  sound.table = pick ({ softSaw, basicShapes, syncSaw, pulseWidth }); break;
        }
        set ("position", between (0.0f, 1.0f));
        set ("analog", between (0.15f, 0.7f));

        // How each note starts and ends, and the filter.
        int voices = 1;
        switch (kind)
        {
            case pad:
                set ("attack", logBetween (0.4f, 1.5f));  set ("decay", logBetween (0.8f, 2.0f));
                set ("sustain", between (0.6f, 0.9f));     set ("release", logBetween (1.5f, 3.0f));
                set ("cutoff", logBetween (1200.0f, 5000.0f)); set ("resonance", between (0.0f, 0.2f));
                voices = 3 + random.nextInt (4);           // 3 to 6 copies
                set ("lfoTarget", toWaveShape); set ("lfoAmount", between (0.1f, 0.3f));
                set ("lfoSync", 0.0f); set ("lfoRate", logBetween (0.08f, 0.3f));
                break;
            case pluck:
                set ("attack", logBetween (0.001f, 0.005f)); set ("decay", logBetween (0.15f, 0.6f));
                set ("sustain", between (0.0f, 0.15f));       set ("release", logBetween (0.2f, 0.6f));
                set ("cutoff", logBetween (700.0f, 3500.0f)); set ("resonance", between (0.0f, 0.4f));
                set ("fenvAmount", between (0.2f, 0.5f)); set ("fenvAttack", 0.001f);
                set ("fenvDecay", logBetween (0.1f, 0.3f)); set ("fenvSustain", 0.0f);
                voices = 1 + random.nextInt (2);
                break;
            case keys:
                set ("attack", logBetween (0.002f, 0.02f)); set ("decay", logBetween (0.5f, 1.5f));
                set ("sustain", between (0.3f, 0.6f));      set ("release", logBetween (0.3f, 0.8f));
                set ("cutoff", logBetween (1500.0f, 5000.0f)); set ("resonance", between (0.0f, 0.2f));
                voices = 1 + random.nextInt (3);
                break;
            case bell:
                set ("attack", 0.001f);                      set ("decay", logBetween (1.0f, 2.5f));
                set ("sustain", 0.0f);                       set ("release", logBetween (1.0f, 2.0f));
                set ("cutoff", logBetween (4000.0f, 8000.0f));
                break;
            case lead:
                set ("attack", logBetween (0.01f, 0.05f)); set ("decay", 0.3f);
                set ("sustain", between (0.6f, 0.8f));     set ("release", logBetween (0.2f, 0.4f));
                set ("cutoff", logBetween (1500.0f, 4000.0f)); set ("resonance", between (0.0f, 0.3f));
                set ("lfoTarget", toPitch); set ("lfoAmount", between (0.04f, 0.1f)); // a gentle vibrato
                set ("lfoSync", 0.0f); set ("lfoRate", between (4.5f, 6.0f));
                voices = 1 + random.nextInt (2);
                break;
        }

        if (voices > 1)
        {
            set ("unisonVoices", (float) voices);
            set ("unisonDetune", between (0.15f, 0.35f));
            set ("unisonWidth", between (0.7f, 1.0f));
        }

        // Effects: mostly space and shimmer, a little grit now and then.
        if (chance (kind == pad || kind == keys ? 0.6f : 0.25f))
        {
            set ("chorusOn", 1.0f); set ("chorusDepth", between (0.3f, 0.6f)); set ("chorusMix", between (0.35f, 0.6f));
        }
        if (chance (kind == pluck || kind == lead ? 0.6f : 0.25f))
        {
            set ("delayOn", 1.0f); set ("delayDivision", (float) pick ({ (int) div1_8, (int) div1_8D, (int) div1_16, (int) div1_4 }));
            set ("delayFeedback", between (0.25f, 0.5f)); set ("delayMix", between (0.15f, 0.3f));
        }
        if (chance (kind == pad || kind == bell ? 1.0f : 0.8f))
        {
            set ("reverbOn", 1.0f); set ("reverbSize", between (0.5f, 0.9f));
            set ("reverbDamping", between (0.3f, 0.6f)); set ("reverbMix", between (0.2f, kind == pad ? 0.5f : 0.35f));
        }
        if (chance (0.1f))
        {
            set ("distOn", 1.0f); set ("distDrive", between (0.15f, 0.35f)); set ("distMix", between (0.3f, 0.6f));
        }

        // Volume: about where the built-in presets on this wavetable sit, a little lower to be safe.
        // (The tests play hundreds of these and check none is too loud or too quiet.)
        float volume = 0.6f;
        switch (sound.table)
        {
            case wavefold: case fmMetal: volume = 0.42f; break;
            case pulseWidth:             volume = 0.52f; break;
            case bitcrush:               volume = 0.6f;  break;
            default:                     volume = 0.6f;  break;
        }
        if (kind == pad)
            volume *= 0.8f; // many copies plus chorus stack up, so pads need more room
        set ("volume", volume);

        // A name in the spirit of the built-in ones.
        static const char* const adjectives[] { "Velvet", "Crystal", "Silver", "Moonlit", "Hazy", "Golden", "Drifting",
                                                "Glass", "Starlit", "Quiet", "Lunar", "Amber", "Misty", "Twilight", "Feather",
                                                "Dreamy", "Gentle", "Hidden", "Sleepy", "Shimmering" };
        static const char* const padNouns[]   { "Bloom", "Cloud", "Tide", "Haze", "Glow", "Horizon", "Aurora", "Garden" };
        static const char* const pluckNouns[] { "Drops", "Sparks", "Steps", "Ripple", "Fireflies", "Pebbles", "Dew" };
        static const char* const keysNouns[]  { "Keys", "Tines", "Organ", "Piano", "Lullaby" };
        static const char* const bellNouns[]  { "Bells", "Chimes", "Bowl", "Lantern" };
        static const char* const leadNouns[]  { "Song", "Whistle", "Flute", "Voice", "Comet" };

        auto from = [&random] (const auto& words) { return juce::String (words[random.nextInt ((int) std::size (words))]); };
        const juce::String noun = kind == pad ? from (padNouns) : kind == pluck ? from (pluckNouns)
                                : kind == keys ? from (keysNouns) : kind == bell ? from (bellNouns) : from (leadNouns);
        sound.name = from (adjectives) + " " + noun;

        // Never reuse a built-in preset's name (like "Velvet Glow"), or LAPiZ would mistake it for that preset.
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            bool taken = false;
            for (auto& preset : list())
                taken = taken || sound.name == preset.name;
            if (! taken)
                break;
            sound.name = from (adjectives) + " " + noun;
        }
        return sound;
    }
}
