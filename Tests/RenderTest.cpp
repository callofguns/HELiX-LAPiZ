// Renders notes through the synth with no host and no speakers, and checks the numbers.
// Run it with: build\LapizTests_artefacts\Release\LapizTests.exe  (exit code 0 means all passed)

#include <algorithm>
#include <cmath>
#include <iostream>
#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "FactoryTables.h"
#include "WavetableFile.h"
#include "UI/LapizLookAndFeel.h"
#include "UI/WavesPage.h"

namespace
{
    constexpr double testSampleRate = 48000.0;
    constexpr int blockSize = 512;
    constexpr int noteA3 = 57;  // 220 Hz
    constexpr int noteHigh = 100; // about 2637 Hz: only 9 harmonics fit below half the sample rate

    struct Measurement
    {
        float peak = 0.0f;
        double rms = 0.0;
        double brightness = 0.0; // RMS of the sample-to-sample change / RMS: higher means more treble
        double tailRms = 0.0;    // RMS of the last 100 ms
        bool allFinite = true;
    };

    using Setting = std::pair<const char*, float>; // parameter id, real (not 0..1) value

    void setParam (LapizAudioProcessor& p, const char* id, float realValue)
    {
        auto* param = p.getState().getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    }

    // Plays one note (held for holdSeconds, then released) and returns the left channel.
    std::vector<float> renderSamples (std::initializer_list<Setting> settings, int note,
                                      float holdSeconds, float tailSeconds)
    {
        LapizAudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, testSampleRate, blockSize);
        processor.prepareToPlay (testSampleRate, blockSize);

        setParam (processor, "position", 2.0f / 3.0f); // saw wave: lots of harmonics, good for testing a filter
        for (auto& s : settings)
            setParam (processor, s.first, s.second);

        const int totalSamples = (int) ((holdSeconds + tailSeconds) * (float) testSampleRate);
        const int releaseAt = (int) (holdSeconds * (float) testSampleRate);
        std::vector<float> left;
        left.reserve ((size_t) totalSamples);

        juce::AudioBuffer<float> buffer (2, blockSize);

        for (int start = 0; start < totalSamples; start += blockSize)
        {
            juce::MidiBuffer midi;
            if (start == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
            if (releaseAt >= start && releaseAt < start + blockSize)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), releaseAt - start);

            buffer.clear();
            processor.processBlock (buffer, midi);

            for (int i = 0; i < blockSize; ++i)
                left.push_back (buffer.getSample (0, i));
        }

        return left;
    }

    Measurement measure (const std::vector<float>& left, float holdSeconds)
    {
        Measurement m;
        for (float v : left)
        {
            if (! std::isfinite (v)) m.allFinite = false;
            m.peak = std::max (m.peak, std::abs (v));
        }

        // Skip the first 50 ms so the start of the note doesn't skew the measurement.
        const size_t skip = (size_t) (0.05 * testSampleRate);
        const size_t holdEnd = (size_t) (holdSeconds * (float) testSampleRate);
        double sumSq = 0.0, diffSq = 0.0;
        for (size_t i = skip + 1; i < holdEnd; ++i)
        {
            sumSq += (double) left[i] * left[i];
            const double d = (double) left[i] - left[i - 1];
            diffSq += d * d;
        }
        const double n = (double) (holdEnd - skip - 1);
        m.rms = std::sqrt (sumSq / n);
        m.brightness = m.rms > 0.0 ? std::sqrt (diffSq / n) / m.rms : 0.0;

        const size_t tailStart = left.size() - (size_t) (0.1 * testSampleRate);
        double tailSq = 0.0;
        for (size_t i = tailStart; i < left.size(); ++i)
            tailSq += (double) left[i] * left[i];
        m.tailRms = std::sqrt (tailSq / (double) (left.size() - tailStart));

        return m;
    }

    Measurement render (std::initializer_list<Setting> settings, float holdSeconds = 0.5f, float tailSeconds = 1.0f)
    {
        return measure (renderSamples (settings, noteA3, holdSeconds, tailSeconds), holdSeconds);
    }

    // The fraction of a signal's energy that is NOT at a multiple of frequencyHz.
    // For a clean note this is close to zero; aliasing puts energy between the harmonics.
    double nonHarmonicFraction (const std::vector<float>& signal, size_t start, double frequencyHz)
    {
        constexpr int order = 15;
        constexpr int n = 1 << order;

        juce::dsp::FFT fft (order);
        juce::dsp::WindowingFunction<float> window ((size_t) n, juce::dsp::WindowingFunction<float>::hann);

        std::vector<float> data ((size_t) n * 2, 0.0f);
        std::copy (signal.begin() + (std::ptrdiff_t) start, signal.begin() + (std::ptrdiff_t) start + n, data.begin());
        window.multiplyWithWindowingTable (data.data(), (size_t) n);
        fft.performRealOnlyForwardTransform (data.data());

        // Mark the bins around DC and around every harmonic as "expected".
        std::vector<char> expected ((size_t) n / 2 + 1, 0);
        for (int b = 0; b <= 4; ++b)
            expected[(size_t) b] = 1;
        for (int h = 1; (double) h * frequencyHz < testSampleRate / 2.0; ++h)
        {
            const int centre = (int) std::lround ((double) h * frequencyHz * n / testSampleRate);
            for (int b = centre - 4; b <= centre + 4; ++b)
                if (b >= 0 && b <= n / 2)
                    expected[(size_t) b] = 1;
        }

        double total = 0.0, unexpected = 0.0;
        for (int b = 0; b <= n / 2; ++b)
        {
            const double re = data[(size_t) b * 2], im = data[(size_t) b * 2 + 1];
            const double energy = re * re + im * im;
            total += energy;
            if (! expected[(size_t) b])
                unexpected += energy;
        }
        return total > 0.0 ? unexpected / total : 0.0;
    }

    // Brightness (see Measurement) of a stretch of the signal, from startSec to endSec.
    double brightnessBetween (const std::vector<float>& s, double startSec, double endSec)
    {
        const auto a = (size_t) (startSec * testSampleRate) + 1, b = (size_t) (endSec * testSampleRate);
        double sumSq = 0.0, diffSq = 0.0;
        for (size_t i = a; i < b; ++i)
        {
            sumSq += (double) s[i] * s[i];
            const double d = (double) s[i] - s[i - 1];
            diffSq += d * d;
        }
        return sumSq > 0.0 ? std::sqrt (diffSq / sumSq) : 0.0;
    }

    double rmsBetween (const std::vector<float>& s, double startSec, double endSec)
    {
        const auto a = (size_t) (startSec * testSampleRate), b = (size_t) (endSec * testSampleRate);
        double sumSq = 0.0;
        for (size_t i = a; i < b; ++i)
            sumSq += (double) s[i] * s[i];
        return std::sqrt (sumSq / (double) (b - a));
    }

    bool allFinite (const std::vector<float>& s)
    {
        return std::all_of (s.begin(), s.end(), [] (float v) { return std::isfinite (v); });
    }

    float peakOf (const std::vector<float>& s)
    {
        float peak = 0.0f;
        for (float v : s) peak = std::max (peak, std::abs (v));
        return peak;
    }

    // Lowest and highest brightness across 25 ms slices from 0.1 s to endSec.
    std::pair<double, double> brightnessRange (const std::vector<float>& s, double endSec)
    {
        double lo = 1.0e9, hi = 0.0;
        for (double t = 0.1; t + 0.025 <= endSec; t += 0.025)
        {
            const double b = brightnessBetween (s, t, t + 0.025);
            lo = std::min (lo, b);
            hi = std::max (hi, b);
        }
        return { lo, hi };
    }

    double rmsOfWave (const Wavetable& table, float position, int level)
    {
        double sum = 0.0;
        constexpr int n = 4096;
        for (int i = 0; i < n; ++i)
        {
            const float v = table.getSample (0.9999f * (float) i / (float) n, position, level);
            sum += (double) v * v;
        }
        return std::sqrt (sum / n);
    }

    // Plays one held note for `seconds` and returns both channels.
    std::pair<std::vector<float>, std::vector<float>> renderStereo (std::initializer_list<Setting> settings, float seconds)
    {
        LapizAudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, testSampleRate, blockSize);
        processor.prepareToPlay (testSampleRate, blockSize);
        setParam (processor, "position", 2.0f / 3.0f);
        for (auto& s : settings)
            setParam (processor, s.first, s.second);

        std::vector<float> left, right;
        juce::AudioBuffer<float> buffer (2, blockSize);
        for (int start = 0; start < (int) (seconds * (float) testSampleRate); start += blockSize)
        {
            juce::MidiBuffer midi;
            if (start == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, noteA3, 0.8f), 0);
            buffer.clear();
            processor.processBlock (buffer, midi);
            for (int i = 0; i < blockSize; ++i)
            {
                left.push_back (buffer.getSample (0, i));
                right.push_back (buffer.getSample (1, i));
            }
        }
        return { left, right };
    }

    // RMS of (left - right) relative to the RMS of left: 0 means mono, higher means wider.
    double sideAmount (const std::pair<std::vector<float>, std::vector<float>>& s)
    {
        double side = 0.0, mid = 0.0;
        for (size_t i = (size_t) (0.05 * testSampleRate); i < s.first.size(); ++i)
        {
            const double d = (double) s.first[i] - s.second[i];
            side += d * d;
            mid += (double) s.first[i] * s.first[i];
        }
        return mid > 0.0 ? std::sqrt (side / mid) : 0.0;
    }

    // Loads a built-in preset and plays a three-note chord for 1.5 seconds.
    // "loudest" is the level of the loudest 0.3 seconds, closer to how loud a short note sounds than the overall rms.
    struct PresetLevel { bool finite = true; float peak = 0.0f; double rms = 0.0, loudest = 0.0; juce::String loadedName; };

    PresetLevel playPresetChord (int index)
    {
        LapizAudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, testSampleRate, blockSize);
        processor.prepareToPlay (testSampleRate, blockSize);
        processor.loadFactoryPreset (index);

        PresetLevel level;
        juce::AudioBuffer<float> buffer (2, blockSize);
        double sumSq = 0.0;
        const int blocks = (int) (1.5 * testSampleRate) / blockSize;
        std::vector<double> blockSumSq;
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0)
                for (int note : { 48, 55, 60 }) // a chord, so pads and leads get a fair test
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
            buffer.clear();
            processor.processBlock (buffer, midi);
            double thisBlock = 0.0;
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < blockSize; ++s)
                {
                    const float v = buffer.getSample (c, s);
                    level.finite = level.finite && std::isfinite (v);
                    level.peak = std::max (level.peak, std::abs (v));
                    thisBlock += (double) v * v;
                }
            sumSq += thisBlock;
            blockSumSq.push_back (thisBlock);
        }
        level.rms = std::sqrt (sumSq / (2.0 * blocks * blockSize));

        const int window = juce::jmax (1, (int) (0.3 * testSampleRate) / blockSize);
        for (int start = 0; start + window <= blocks; ++start)
        {
            double windowSumSq = 0.0;
            for (int b = start; b < start + window; ++b)
                windowSumSq += blockSumSq[(size_t) b];
            level.loudest = std::max (level.loudest, std::sqrt (windowSumSq / (2.0 * window * blockSize)));
        }
        level.loadedName = processor.getPresetName();
        return level;
    }

    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "PASS  " : "FAIL  ") << what << std::endl;
        if (! ok) ++failures;
    }
}

// LapizTests.exe --snapshot <file.png>: draws the Waves tab with a drawing loaded and saves a picture of it.
static int saveWavesSnapshot (const juce::File& file)
{
    LapizAudioProcessor processor;
    std::vector<Wavetable::Frame> frames;
    for (int f = 0; f < 3; ++f)
    {
        Wavetable::Frame frame ((size_t) Wavetable::tableSize);
        for (int i = 0; i < Wavetable::tableSize; ++i)
        {
            const float p = (float) i / (float) Wavetable::tableSize;
            frame[(size_t) i] = f == 0 ? std::sin (juce::MathConstants<float>::twoPi * p)
                              : f == 1 ? (p < 0.3f ? 0.9f : p < 0.6f ? -0.4f : -0.8f + p * 0.5f)
                                       : std::round (std::sin (juce::MathConstants<float>::twoPi * p) * 4.0f) / 4.0f;
        }
        frames.push_back (std::move (frame));
    }
    processor.setDrawnTable (frames, "My drawing");

    juce::SharedResourcePointer<Theme::Fonts> fonts;
    LapizLookAndFeel lookAndFeel;
    WavesPage page (processor, [] (juce::Component&, const juce::String&) {});
    page.setLookAndFeel (&lookAndFeel);
    page.setBounds (0, 0, 900, 490);

    juce::Image image (juce::Image::ARGB, 900, 490, true);
    {
        juce::Graphics g (image);
        g.fillAll (Theme::get().ground);
        page.paintEntireComponent (g, true);
    }
    page.setLookAndFeel (nullptr);

    file.deleteFile();
    juce::FileOutputStream out (file);
    return juce::PNGImageFormat().writeImageToStream (image, out) ? 0 : 1;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc == 3 && juce::String (argv[1]) == "--snapshot")
        return saveWavesSnapshot (juce::File (juce::String (argv[2])));

    // LapizTests.exe --presets: prints how loud each built-in preset plays a chord, for balancing them.
    if (argc == 2 && juce::String (argv[1]) == "--presets")
    {
        for (int i = 0; i < (int) FactoryPresets::list().size(); ++i)
        {
            const auto level = playPresetChord (i);
            std::cout << FactoryPresets::list()[(size_t) i].name << "\t" << level.peak << "\t" << level.rms << "\t" << level.loudest << std::endl;
        }
        return 0;
    }

    // ---- Basic sound and filter ----
    const auto open = render ({});
    std::cout << "open filter:  peak " << open.peak << "  rms " << open.rms << "  brightness " << open.brightness << std::endl;

    check (open.allFinite, "output is finite (no NaN or infinity)");
    check (open.rms > 0.01, "a held note makes sound");
    check (open.peak < 1.0f, "output stays below full scale at default settings");
    check (open.tailRms < 1.0e-4, "note is silent after release finishes");

    const auto dark = render ({ { "cutoff", 200.0f } });
    std::cout << "low-pass 200 Hz:  rms " << dark.rms << "  brightness " << dark.brightness << std::endl;
    check (dark.allFinite && dark.rms > 0.0, "low-pass at 200 Hz still passes the note");
    check (dark.brightness < 0.5 * open.brightness, "low-pass at 200 Hz removes most treble");

    const auto thin = render ({ { "filterType", 1.0f }, { "cutoff", 5000.0f } });
    std::cout << "high-pass 5 kHz:  rms " << thin.rms << "  brightness " << thin.brightness << std::endl;
    check (thin.allFinite, "high-pass output is finite");
    check (thin.rms < 0.3 * open.rms, "high-pass at 5 kHz removes most of the energy of a low note");

    const auto band = render ({ { "filterType", 2.0f }, { "cutoff", 660.0f } });
    std::cout << "band-pass 660 Hz:  peak " << band.peak << "  rms " << band.rms << std::endl;
    check (band.allFinite && band.rms > 0.0, "band-pass output is finite and not silent");
    check (band.peak < open.peak * 2.0f, "band-pass is not much louder than the unfiltered note");

    const auto edgy = render ({ { "cutoff", 440.0f }, { "resonance", 1.0f } });
    std::cout << "low-pass 440 Hz, max emphasis:  peak " << edgy.peak << std::endl;
    check (edgy.allFinite, "maximum emphasis stays finite (filter is stable)");
    check (edgy.peak < 2.0f, "maximum emphasis does not blow up in level");

    // ---- Band-limited wavetables ----
    const Wavetable table = FactoryTables::make (0);

    double sineError = 0.0;
    for (int level = 1; level < Wavetable::numLevels; ++level)
        for (int i = 0; i < 64; ++i)
        {
            const float phase = 0.9999f * (float) i / 64.0f;
            sineError = std::max (sineError, (double) std::abs (table.getSample (phase, 0.0f, level)
                                                                - std::sin (juce::MathConstants<float>::twoPi * phase)));
        }
    std::cout << "sine frame, worst error across all levels: " << sineError << std::endl;
    check (sineError < 0.01, "the sine frame is unchanged by band-limiting at every level");

    const double sawRms0 = rmsOfWave (table, 2.0f / 3.0f, 0);
    const double sawRms3 = rmsOfWave (table, 2.0f / 3.0f, 4); // 128 harmonics
    std::cout << "saw rms, as drawn " << sawRms0 << "  with 128 harmonics " << sawRms3 << std::endl;
    check (sawRms3 > 0.98 * sawRms0 && sawRms3 < 1.001 * sawRms0, "band-limiting keeps the saw's level (rebuild is correctly scaled)");

    check (table.levelForFrequency (20.0, testSampleRate) == 1, "a 20 Hz note uses the full-detail level");
    bool allFit = true;
    for (double f = 20.0; f < 12000.0; f *= 1.05)
    {
        const int level = table.levelForFrequency (f, testSampleRate);
        if ((double) Wavetable::maxHarmonics (level) * f > testSampleRate / 2.0)
            allFit = false;
    }
    check (allFit, "for every note, the chosen level keeps all harmonics below half the sample rate");

    // ---- Aliasing, measured on a high note ----
    const double highFreq = 440.0 * std::pow (2.0, (noteHigh - 69) / 12.0);
    const auto lapizHigh = renderSamples ({}, noteHigh, 1.5f, 0.1f);

    std::vector<float> naive (lapizHigh.size());
    for (size_t i = 0; i < naive.size(); ++i)
    {
        const double phase = std::fmod (highFreq * (double) i / testSampleRate, 1.0);
        naive[i] = (float) (2.0 * phase - 1.0);
    }

    const size_t analysisStart = (size_t) (0.5 * testSampleRate);
    const double naiveFraction = nonHarmonicFraction (naive, analysisStart, highFreq);
    const double lapizFraction = nonHarmonicFraction (lapizHigh, analysisStart, highFreq);
    std::cout << "note " << noteHigh << " (" << highFreq << " Hz): energy between the harmonics -- "
              << "naive saw " << 10.0 * std::log10 (naiveFraction) << " dB, LAPiZ "
              << 10.0 * std::log10 (std::max (lapizFraction, 1.0e-12)) << " dB" << std::endl;
    check (naiveFraction > 1.0e-3, "the measurement detects aliasing in a naive saw (above -30 dB)");
    check (lapizFraction < 1.0e-5, "LAPiZ's high note has almost no energy between the harmonics (below -50 dB)");

    // ---- Filter envelope ----
    {
        const auto opening = renderSamples ({ { "cutoff", 200.0f }, { "fenvAmount", 1.0f }, { "fenvAttack", 0.001f },
                                              { "fenvDecay", 0.3f }, { "fenvSustain", 0.0f } }, noteA3, 1.0f, 0.5f);
        const double early = brightnessBetween (opening, 0.01, 0.06), late = brightnessBetween (opening, 0.8, 0.95);
        std::cout << "filter envelope, Sweep +100 %:  brightness early " << early << "  late " << late << std::endl;
        check (allFinite (opening), "filter envelope output is finite");
        check (early > 2.0 * late, "a positive Sweep makes the start of the note brighter than the held part");

        const auto closing = renderSamples ({ { "cutoff", 5000.0f }, { "fenvAmount", -1.0f }, { "fenvAttack", 0.001f },
                                              { "fenvDecay", 0.3f }, { "fenvSustain", 0.0f } }, noteA3, 1.0f, 0.5f);
        const double earlyDark = brightnessBetween (closing, 0.01, 0.06), lateDark = brightnessBetween (closing, 0.8, 0.95);
        std::cout << "filter envelope, Sweep -100 %:  brightness early " << earlyDark << "  late " << lateDark << std::endl;
        check (earlyDark < 0.5 * lateDark, "a negative Sweep makes the start of the note darker than the held part");
    }

    // ---- LFO ----
    {
        const auto steady = renderSamples ({ { "cutoff", 400.0f } }, noteA3, 1.0f, 0.2f);
        const auto still = brightnessRange (steady, 1.0);
        const auto wobbling = renderSamples ({ { "cutoff", 400.0f }, { "lfoAmount", 0.4f }, { "lfoSync", 0.0f },
                                               { "lfoRate", 2.0f } }, noteA3, 1.0f, 0.2f);
        const auto moving = brightnessRange (wobbling, 1.0);
        std::cout << "LFO on Brightness:  brightness range off " << still.first << " - " << still.second
                  << "   on " << moving.first << " - " << moving.second << std::endl;
        check (still.second < 1.1 * still.first, "with Wobble at 0 the brightness stays still");
        check (moving.second > 2.0 * moving.first, "with Wobble up, the LFO moves the brightness");

        // Square LFO on volume, synced to 1/4 notes. With no host the tempo is 120 BPM, so 2 cycles per second.
        const auto chopped = renderSamples ({ { "lfoTarget", (float) Lfo::volume }, { "lfoShape", (float) Lfo::square },
                                              { "lfoAmount", 1.0f }, { "lfoSync", 1.0f },
                                              { "lfoDivision", (float) Lfo::defaultDivision } }, noteA3, 2.05f, 0.2f);
        int rises = 0;
        bool wasLoud = true;
        double loudest = 0.0;
        for (double t = 0.1; t < 2.0; t += 0.01)
            loudest = std::max (loudest, rmsBetween (chopped, t, t + 0.01));
        for (double t = 0.1; t < 2.0; t += 0.01)
        {
            const bool loud = rmsBetween (chopped, t, t + 0.01) > 0.5 * loudest;
            if (loud && ! wasLoud) ++rises;
            wasLoud = loud;
        }
        std::cout << "LFO square on Volume at 1/4, 120 BPM:  " << rises << " restarts of sound in 1.9 s" << std::endl;
        // Loud from 0, quiet from 0.25 s, loud again at 0.5, 1.0 and 1.5 s.
        check (rises == 3, "tempo-synced LFO runs at the right speed (2 cycles per second at 120 BPM, 1/4)");

        const auto siren = renderSamples ({ { "lfoTarget", (float) Lfo::pitch }, { "lfoAmount", 1.0f },
                                            { "lfoSync", 0.0f }, { "lfoRate", 5.0f }, { "lfoRetrigger", 1.0f } },
                                          noteHigh, 1.0f, 0.5f);
        std::cout << "LFO on Pitch, full amount, high note:  peak " << peakOf (siren) << std::endl;
        check (allFinite (siren) && peakOf (siren) < 1.0f, "a full pitch LFO on a high note stays finite and below full scale");

        const auto shaped = renderSamples ({ { "lfoTarget", (float) Lfo::position }, { "lfoAmount", 1.0f },
                                             { "lfoShape", (float) Lfo::random } }, noteA3, 1.0f, 0.5f);
        check (allFinite (shaped) && peakOf (shaped) < 1.0f, "a random LFO on Wave Shape stays finite and below full scale");
    }

    // ---- Built-in wavetables ----
    {
        bool allGood = true;
        for (int t = 0; t < (int) FactoryTables::list.size(); ++t)
        {
            const auto factory = FactoryTables::make (t);
            float peak = 0.0f;
            bool finite = true;
            for (int level = 0; level < Wavetable::numLevels; ++level)
                for (int i = 0; i < 512; ++i)
                {
                    const float v = factory.getSample (0.9999f * (float) i / 512.0f, (float) (i % 7) / 6.0f, level);
                    finite = finite && std::isfinite (v);
                    peak = std::max (peak, std::abs (v));
                }
            std::cout << "table " << FactoryTables::list[(size_t) t].name << ": " << factory.getNumFrames()
                      << " waves, peak " << peak << std::endl;
            if (! finite || peak > 1.4f || peak < 0.5f)
                allGood = false;
        }
        check (allGood, "every built-in wavetable builds, is finite and has a sensible level");

        // Render a note through each built-in table (picked the way the interface does).
        bool allPlay = true;
        for (int t = 0; t < (int) FactoryTables::list.size(); ++t)
        {
            LapizAudioProcessor processor;
            processor.setPlayConfigDetails (0, 2, testSampleRate, blockSize);
            processor.prepareToPlay (testSampleRate, blockSize);
            processor.selectFactoryTable (t);
            setParam (processor, "position", 0.6f);

            juce::AudioBuffer<float> buffer (2, blockSize);
            float peak = 0.0f;
            for (int b = 0; b < 40; ++b)
            {
                juce::MidiBuffer midi;
                if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, noteA3, 0.8f), 0);
                buffer.clear();
                processor.processBlock (buffer, midi);
                peak = std::max (peak, buffer.getMagnitude (0, 0, blockSize));
                if (buffer.getMagnitude (0, 0, blockSize) > 0.0f && ! std::isfinite (buffer.getSample (0, 0)))
                    allPlay = false;
            }
            if (! std::isfinite (peak) || peak <= 0.01f || peak >= 1.0f)
                allPlay = false;
        }
        check (allPlay, "every built-in wavetable plays a note that is audible and below full scale");
    }

    // ---- Drawn wavetables ----
    {
        // A wave drawn entirely above the middle line has a DC offset, which must not reach the speakers.
        const Wavetable lifted (std::vector<Wavetable::Frame> { Wavetable::Frame ((size_t) Wavetable::tableSize, 0.5f) });
        check (std::abs (lifted.getSample (0.3f, 0.0f, 1)) < 1.0e-4f, "a drawn wave's DC offset is removed from what you hear");
        check (std::abs (lifted.getSample (0.3f, 0.0f, 0) - 0.5f) < 1.0e-6f, "the display still shows the wave exactly as drawn");

        // Three keyframes to save and load in different ways.
        std::vector<Wavetable::Frame> drawn;
        for (int f = 0; f < 3; ++f)
        {
            Wavetable::Frame frame ((size_t) Wavetable::tableSize);
            for (int i = 0; i < Wavetable::tableSize; ++i)
                frame[(size_t) i] = std::sin ((float) (f + 1) * juce::MathConstants<float>::twoPi * (float) i / Wavetable::tableSize) * 0.8f;
            drawn.push_back (std::move (frame));
        }

        auto sameFrames = [] (const std::vector<Wavetable::Frame>& a, const std::vector<Wavetable::Frame>& b)
        {
            if (a.size() != b.size()) return false;
            for (size_t f = 0; f < a.size(); ++f)
                for (size_t i = 0; i < a[f].size(); ++i)
                    if (std::abs (a[f][i] - b[f][i]) > 1.0e-6f) return false;
            return true;
        };

        // .wav file
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("LapizTestTable.wav");
        check (WavetableFile::save (file, drawn), "a drawing saves as a .wav file");
        const auto loaded = WavetableFile::load (file);
        check (sameFrames (loaded, drawn), "the .wav file loads back with the same 3 waves");

        // Project save and load
        juce::MemoryBlock project;
        {
            LapizAudioProcessor a;
            a.setDrawnTable (drawn, "Test drawing");
            a.getStateInformation (project);
        }
        {
            LapizAudioProcessor b;
            b.setStateInformation (project.getData(), (int) project.getSize());
            const auto choice = b.getTableChoice();
            check (choice.kind == TableChoice::Kind::drawn && choice.name == "Test drawing" && sameFrames (choice.keyframes, drawn),
                   "a drawing is saved in the project and comes back exactly");
        }

        // A file table whose file has gone missing still loads from the copy stored in the project
        {
            LapizAudioProcessor a;
            a.selectTableFile (file);
            a.getStateInformation (project);
        }
        file.deleteFile();
        {
            LapizAudioProcessor b;
            b.setStateInformation (project.getData(), (int) project.getSize());
            const auto restored = b.getTable();
            bool same = restored->getNumFrames() == 3;
            for (int f = 0; same && f < 3; ++f)
                for (int i = 0; i < Wavetable::tableSize; ++i)
                    if (std::abs (restored->getFrame (f)[i] - drawn[(size_t) f][(size_t) i]) > 1.0e-6f) { same = false; break; }
            check (b.getTableChoice().kind == TableChoice::Kind::file && same,
                   "a project using a deleted wavetable file still sounds the same");
        }

        // Switching tables while a note is playing
        {
            LapizAudioProcessor processor;
            processor.setPlayConfigDetails (0, 2, testSampleRate, blockSize);
            processor.prepareToPlay (testSampleRate, blockSize);
            juce::AudioBuffer<float> buffer (2, blockSize);
            bool finite = true;
            for (int b = 0; b < 60; ++b)
            {
                juce::MidiBuffer midi;
                if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, noteA3, 0.8f), 0);
                if (b % 10 == 5) processor.setDrawnTable (drawn, "Swap");
                if (b % 10 == 0) processor.selectFactoryTable ((b / 10) % (int) FactoryTables::list.size());
                buffer.clear();
                processor.processBlock (buffer, midi);
                for (int i = 0; i < blockSize; ++i)
                    finite = finite && std::isfinite (buffer.getSample (0, i));
            }
            check (finite, "changing wavetables while a note plays stays finite");
        }
    }

    // Unison
    {
        const auto single = renderStereo ({}, 0.6f);
        const auto stack = renderStereo ({ { "unisonVoices", 7.0f }, { "unisonDetune", 0.5f }, { "unisonWidth", 1.0f } }, 0.6f);
        const auto narrow = renderStereo ({ { "unisonVoices", 7.0f }, { "unisonDetune", 0.5f }, { "unisonWidth", 0.0f } }, 0.6f);

        check (sideAmount (single) < 1.0e-6, "one copy plays the same in both speakers");
        check (sideAmount (narrow) < 1.0e-3, "unison with Width at 0 stays in the middle");
        check (sideAmount (stack) > 0.3, "unison with full Width spreads the copies left and right");
        check (allFinite (stack.first) && allFinite (stack.second) && peakOf (stack.first) < 1.0f && peakOf (stack.second) < 1.0f,
               "7 detuned copies stay finite and below full scale");

        const double singleRms = rmsBetween (single.first, 0.1, 0.6);
        const double stackRms = rmsBetween (stack.first, 0.1, 0.6);
        check (stackRms > 0.5 * singleRms && stackRms < 2.0 * singleRms, "unison keeps roughly the same loudness as one copy");

        // Detuned copies drift in and out of phase, so the level ripples: a single copy's level doesn't.
        auto ripple = [] (const std::vector<float>& s)
        {
            double lo = 1.0e9, hi = 0.0;
            const double window = 5.0 / 220.0; // exactly 5 cycles of the note, so a steady note measures the same everywhere
            for (double t = 0.3; t + window <= 0.6; t += 0.01) // after the volume envelope has settled
            {
                const double r = rmsBetween (s, t, t + window);
                lo = std::min (lo, r);
                hi = std::max (hi, r);
            }
            return hi / lo;
        };
        const auto stackMono = renderStereo ({ { "unisonVoices", 2.0f }, { "unisonDetune", 1.0f }, { "unisonWidth", 0.0f } }, 0.6f);
        std::cout << "      level ripple: one copy " << ripple (single.first) << ", two detuned copies " << ripple (stackMono.first) << std::endl;
        check (ripple (single.first) < 1.05 && ripple (stackMono.first) > 1.3, "Detune makes the copies beat against each other");
    }

    // Effects
    {
        const auto dry = render ({}, 0.3f, 1.0f);
        const auto echo = render ({ { "delayOn", 1.0f }, { "delayFeedback", 0.6f }, { "delayMix", 0.5f } }, 0.3f, 1.0f);
        const auto room = render ({ { "reverbOn", 1.0f }, { "reverbSize", 0.9f }, { "reverbMix", 1.0f } }, 0.3f, 1.0f);
        check (dry.tailRms < 1.0e-4 && echo.tailRms > 1.0e-3, "Echo keeps repeating after the note has ended");
        check (room.tailRms > 1.0e-3, "Space (reverb) rings on after the note has ended");

        const auto clean = render ({ { "position", 0.0f } });
        const auto gritty = render ({ { "position", 0.0f }, { "distOn", 1.0f }, { "distDrive", 1.0f }, { "distTone", 1.0f } });
        check (gritty.allFinite && gritty.brightness > 1.5 * clean.brightness, "Grit (distortion) adds harmonics to a sine wave");
        check (gritty.rms > 0.5 * clean.rms && gritty.peak < 1.0f, "Grit keeps roughly the same loudness, below full scale");

        const auto chorus = renderStereo ({ { "chorusOn", 1.0f }, { "chorusDepth", 1.0f }, { "chorusMix", 1.0f } }, 0.6f);
        check (sideAmount (chorus) > 0.05, "Chorus makes a single copy wider (left and right differ)");

        const auto everything = renderStereo ({ { "unisonVoices", 8.0f }, { "distOn", 1.0f }, { "distDrive", 1.0f }, { "chorusOn", 1.0f },
                                                { "delayOn", 1.0f }, { "delayFeedback", 0.9f }, { "delayMix", 1.0f },
                                                { "reverbOn", 1.0f }, { "reverbMix", 1.0f } }, 2.0f);
        check (allFinite (everything.first) && allFinite (everything.second) && peakOf (everything.first) < 2.0f,
               "all effects at once, turned up, stay finite and don't blow up");
    }

    // Presets
    {
        bool allPlay = true;
        for (int i = 0; i < (int) FactoryPresets::list().size(); ++i)
        {
            const auto level = playPresetChord (i);
            const auto& preset = FactoryPresets::list()[(size_t) i];
            const bool ok = level.finite && level.rms > 0.005 && level.peak < 1.0f && level.loadedName == preset.name;
            if (! ok)
                std::cout << "      preset " << preset.name << ": peak " << level.peak << ", rms " << level.rms << std::endl;
            allPlay = allPlay && ok;
        }
        check (allPlay, "every built-in preset loads and plays a chord that is audible and below full scale");

        juce::StringArray names;
        for (auto& preset : FactoryPresets::list())
            names.addIfNotAlreadyThere (preset.name);
        check (names.size() == (int) FactoryPresets::list().size(), "every built-in preset has its own name");

        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("LapizTestPreset.lapiz");
        {
            LapizAudioProcessor a;
            a.loadFactoryPreset (5);
            setParam (a, "cutoff", 1234.0f);
            setParam (a, "reverbOn", 1.0f);
            check (a.savePresetFile (file), "a preset saves as a file");

            LapizAudioProcessor b;
            check (b.loadPresetFile (file), "the preset file loads back");
            const float cutoff = b.getState().getParameter ("cutoff")->convertFrom0to1 (b.getState().getParameter ("cutoff")->getValue());
            check (std::abs (cutoff - 1234.0f) < 1.0f && b.getState().getRawParameterValue ("reverbOn")->load() > 0.5f
                       && b.getPresetName() == "LapizTestPreset" && b.getTableChoice().factoryIndex == FactoryPresets::list()[5].table,
                   "the loaded preset has the same controls, wavetable and name");

            // Loading a preset resets controls it doesn't mention.
            setParam (b, "unisonVoices", 6.0f);
            b.loadFactoryPreset (0);
            check (b.getState().getRawParameterValue ("unisonVoices")->load() < 1.5f && b.getPresetName() == "Init",
                   "Init puts every control back to its default");
        }
        file.deleteFile();
    }

    std::cout << (failures == 0 ? "\nAll checks passed." : "\nSome checks FAILED.") << std::endl;
    return failures;
}
