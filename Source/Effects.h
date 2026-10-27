#pragma once

#include <atomic>
#include <cmath>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Lfo.h"

// The effects after the synth, always in this order:
//   Distortion -> Chorus -> Delay -> Reverb
// Each one has an on/off switch and is skipped completely when off.
namespace Effects
{
    // Pointers to the knob values, read once per audio block.
    struct Params
    {
        std::atomic<float>* distOn = nullptr;
        std::atomic<float>* distDrive = nullptr;
        std::atomic<float>* distTone = nullptr;
        std::atomic<float>* distMix = nullptr;

        std::atomic<float>* chorusOn = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;

        std::atomic<float>* delayOn = nullptr;
        std::atomic<float>* delayDivision = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;

        std::atomic<float>* reverbOn = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamping = nullptr;
        std::atomic<float>* reverbMix = nullptr;
    };

    inline constexpr float maxDriveDb = 36.0f;
    inline constexpr double maxDelaySeconds = 4.0;
    inline constexpr int defaultDelayDivision = 9; // 1/8 D

    // Distortion's Tone knob (0..1) as a low-pass cutoff in Hz: 800 Hz to 20 kHz.
    inline float toneToHz (float tone) { return 800.0f * std::pow (25.0f, tone); }

    class Chain
    {
    public:
        void prepare (double newSampleRate, int maxBlockSize)
        {
            sampleRate = newSampleRate;
            const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 2 };

            oversampling.initProcessing ((size_t) maxBlockSize);
            toneFilter.prepare (spec);
            toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

            const int chorusSize = (int) ((chorusCentreMs + chorusDepthMs) * 0.001 * sampleRate) + 4;
            for (auto& line : chorusLines)
                line.assign ((size_t) chorusSize, 0.0f);
            chorusWrite = 0;

            const int delaySize = (int) (maxDelaySeconds * sampleRate) + 4;
            for (auto& line : delayLines)
                line.assign ((size_t) delaySize, 0.0f);
            delayWrite = 0;
            delayTime.reset (sampleRate, 0.08); // glides to a new time instead of clicking

            reverb.prepare (spec);
            dryCopy.setSize (2, maxBlockSize);
            reset();
        }

        void reset()
        {
            oversampling.reset();
            toneFilter.reset();
            for (auto& line : chorusLines)
                std::fill (line.begin(), line.end(), 0.0f);
            chorusPhase = 0.0;
            for (auto& line : delayLines)
                std::fill (line.begin(), line.end(), 0.0f);
            reverb.reset();
            delayTimeSet = false;
        }

        // Latency added by the distortion's oversampling (it runs at 4x the sample rate so it doesn't alias).
        int getLatencySamples() const { return juce::roundToInt (oversampling.getLatencyInSamples()); }

        void process (juce::AudioBuffer<float>& buffer, const Params& p, double bpm)
        {
            if (buffer.getNumChannels() < 2)
                return; // the effects are stereo; a mono output plays the dry synth

            const int numSamples = buffer.getNumSamples();
            if (p.distOn->load() > 0.5f)   distort (buffer, p);
            if (p.chorusOn->load() > 0.5f) chorusBlock (buffer, p);
            if (p.delayOn->load() > 0.5f)  delayBlock (buffer, p, bpm, numSamples);
            else                           delayTimeSet = false;
            if (p.reverbOn->load() > 0.5f) reverbBlock (buffer, p);
        }

    private:
        void distort (juce::AudioBuffer<float>& buffer, const Params& p)
        {
            const int numSamples = buffer.getNumSamples();
            const float mix = p.distMix->load();
            const float drive = juce::Decibels::decibelsToGain (p.distDrive->load() * maxDriveDb);
            const float makeUp = 0.3f / std::tanh (drive * 0.3f); // a typical note level comes out about as loud as it went in

            for (int c = 0; c < 2; ++c)
                dryCopy.copyFrom (c, 0, buffer, c, 0, numSamples);

            juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), 2, (size_t) numSamples);
            auto up = oversampling.processSamplesUp (block);
            for (size_t c = 0; c < up.getNumChannels(); ++c)
            {
                auto* data = up.getChannelPointer (c);
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                    data[i] = std::tanh (data[i] * drive) * makeUp;
            }
            oversampling.processSamplesDown (block);

            toneFilter.setCutoffFrequency (juce::jmin (toneToHz (p.distTone->load()), (float) (sampleRate * 0.45)));
            juce::dsp::ProcessContextReplacing<float> context (block);
            toneFilter.process (context);

            // The dry copy is a few samples early (the oversampling delays the wet one), which is fine for a blend.
            for (int c = 0; c < 2; ++c)
            {
                buffer.applyGain (c, 0, numSamples, mix);
                buffer.addFrom (c, 0, dryCopy, c, 0, numSamples, 1.0f - mix);
            }
        }

        // A short delay whose length sways up and down, blended with the dry sound. The left and right
        // sides sway a quarter-cycle apart, which is what makes the chorus sound wide.
        void chorusBlock (juce::AudioBuffer<float>& buffer, const Params& p)
        {
            const double increment = p.chorusRate->load() / sampleRate;
            const float depthSamples = p.chorusDepth->load() * (float) (chorusDepthMs * 0.001 * sampleRate);
            const float centreSamples = (float) (chorusCentreMs * 0.001 * sampleRate);
            const float mix = p.chorusMix->load();
            const int size = (int) chorusLines[0].size();

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                for (int c = 0; c < 2; ++c)
                {
                    auto* data = buffer.getWritePointer (c);
                    auto& line = chorusLines[c];
                    line[(size_t) chorusWrite] = data[i];

                    const double lfoPhase = chorusPhase + (c == 1 ? 0.25 : 0.0);
                    const float sway = (float) std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
                    float readPos = (float) chorusWrite - (centreSamples + depthSamples * sway);
                    if (readPos < 0.0f)
                        readPos += (float) size;

                    const int i0 = (int) readPos;
                    const int i1 = (i0 + 1) % size;
                    const float frac = readPos - (float) i0;
                    const float wet = line[(size_t) i0] + frac * (line[(size_t) i1] - line[(size_t) i0]);
                    data[i] = data[i] * (1.0f - 0.5f * mix) + wet * 0.5f * mix; // 100 % is the classic half-and-half blend
                }

                chorusWrite = (chorusWrite + 1) % size;
                chorusPhase += increment;
                if (chorusPhase >= 1.0)
                    chorusPhase -= 1.0;
            }
        }

        // A ping-pong delay: the echoes bounce between the left and right speakers.
        void delayBlock (juce::AudioBuffer<float>& buffer, const Params& p, double bpm, int numSamples)
        {
            const int division = juce::jlimit (0, (int) Lfo::divisions.size() - 1, juce::roundToInt (p.delayDivision->load()));
            const double seconds = juce::jmin (maxDelaySeconds, Lfo::divisions[(size_t) division].beats * 60.0 / bpm);
            const float targetSamples = (float) (seconds * sampleRate);

            if (! delayTimeSet)
            {
                delayTime.setCurrentAndTargetValue (targetSamples);
                delayTimeSet = true;
            }
            delayTime.setTargetValue (targetSamples);

            const float feedback = p.delayFeedback->load();
            const float mix = p.delayMix->load();
            const int size = (int) delayLines[0].size();
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);

            for (int i = 0; i < numSamples; ++i)
            {
                const float delay = delayTime.getNextValue();
                float readPos = (float) delayWrite - delay;
                if (readPos < 0.0f)
                    readPos += (float) size;

                const int i0 = (int) readPos;
                const int i1 = (i0 + 1) % size;
                const float frac = readPos - (float) i0;
                const float echoL = delayLines[0][(size_t) i0] + frac * (delayLines[0][(size_t) i1] - delayLines[0][(size_t) i0]);
                const float echoR = delayLines[1][(size_t) i0] + frac * (delayLines[1][(size_t) i1] - delayLines[1][(size_t) i0]);

                // The input goes into the left line; each echo crosses to the other side.
                delayLines[0][(size_t) delayWrite] = 0.5f * (left[i] + right[i]) + feedback * echoR;
                delayLines[1][(size_t) delayWrite] = feedback * echoL;
                delayWrite = (delayWrite + 1) % size;

                left[i] += mix * echoL;
                right[i] += mix * echoR;
            }
        }

        void reverbBlock (juce::AudioBuffer<float>& buffer, const Params& p)
        {
            juce::dsp::Reverb::Parameters settings;
            settings.roomSize = p.reverbSize->load();
            settings.damping = p.reverbDamping->load();
            settings.wetLevel = p.reverbMix->load() * 0.5f;
            settings.dryLevel = 1.0f;
            settings.width = 1.0f;
            reverb.setParameters (settings);

            juce::dsp::AudioBlock<float> block (buffer);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
        }

        double sampleRate = 44100.0;
        juce::AudioBuffer<float> dryCopy;

        juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
        juce::dsp::StateVariableTPTFilter<float> toneFilter;

        static constexpr double chorusCentreMs = 7.0, chorusDepthMs = 5.0;
        std::vector<float> chorusLines[2];
        int chorusWrite = 0;
        double chorusPhase = 0.0;

        std::vector<float> delayLines[2];
        int delayWrite = 0;
        juce::SmoothedValue<float> delayTime;
        bool delayTimeSet = false;

        juce::dsp::Reverb reverb;
    };
}
