#pragma once

#include <atomic>
#include <cmath>
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "../FilterMath.h"

// Draws the filter's frequency response: bass on the left, treble on the right,
// and the curve shows how much of each pitch range gets through.
// While a note plays, the curve follows the filter envelope and LFO; the thin line marks the knob setting.
class FilterView : public juce::Component, private juce::Timer
{
public:
    FilterView (juce::AudioProcessorValueTreeState& state, std::function<double()> sampleRateGetter,
                const std::atomic<float>& liveCutoffHz)
        : getSampleRate (std::move (sampleRateGetter)),
          type (state.getRawParameterValue ("filterType")),
          cutoff (state.getRawParameterValue ("cutoff")),
          resonance (state.getRawParameterValue ("resonance")),
          liveCutoff (liveCutoffHz)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (p.well);
        g.fillRoundedRectangle (bounds, 6.0f);

        auto labelArea = bounds.reduced (6.0f, 0.0f).removeFromBottom (20.0f);
        auto area = bounds.reduced (12.0f, 12.0f);
        area.removeFromBottom (20.0f);

        auto xForFreq = [&] (float f) { return area.getX() + area.getWidth() * std::log (f / minFreq) / std::log (maxFreq / minFreq); };
        auto yForDb   = [&] (float db) { return area.getBottom() - area.getHeight() * (db - minDb) / (maxDb - minDb); };

        // Grid and frequency labels
        g.setFont (Theme::regular (11.0f));
        for (const auto& mark : { std::pair<float, const char*> { 100.0f, "100 Hz" },
                                  std::pair<float, const char*> { 1000.0f, "1 kHz" },
                                  std::pair<float, const char*> { 10000.0f, "10 kHz" } })
        {
            const float x = xForFreq (mark.first);
            g.setColour (p.edge);
            g.drawVerticalLine ((int) x, area.getY(), area.getBottom());
            g.setColour (p.soft);
            g.drawText (mark.second, juce::Rectangle<float> (x - 30.0f, labelArea.getY(), 60.0f, labelArea.getHeight()),
                        juce::Justification::centred);
        }
        g.setColour (p.edge);
        g.drawHorizontalLine ((int) yForDb (0.0f), area.getX(), area.getRight());

        // The response curve
        const double sampleRate = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        const int filterType = (int) type->load();
        const float knobCutoffHz = FilterMath::clampCutoff (cutoff->load(), sampleRate);
        const float live = liveCutoff.load();
        const float cutoffHz = live > 0.0f ? live : knobCutoffHz;
        const float q = FilterMath::resonanceToQ (resonance->load());

        juce::Path curve;
        const int steps = juce::jmax (2, (int) area.getWidth() / 2);
        for (int i = 0; i <= steps; ++i)
        {
            const float f = minFreq * std::pow (maxFreq / minFreq, (float) i / (float) steps);
            const float mag = FilterMath::magnitudeAt (filterType, f, cutoffHz, q, sampleRate);
            const float db = juce::jlimit (minDb, maxDb, 20.0f * std::log10 (juce::jmax (mag, 1.0e-6f)));
            const float x = area.getX() + area.getWidth() * (float) i / (float) steps;

            if (i == 0) curve.startNewSubPath (x, yForDb (db));
            else        curve.lineTo (x, yForDb (db));
        }

        auto fill = curve;
        fill.lineTo (area.getRight(), area.getBottom());
        fill.lineTo (area.getX(), area.getBottom());
        fill.closeSubPath();
        g.setColour (p.tone.withAlpha (0.18f));
        g.fillPath (fill);

        g.setColour (p.tone);
        g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        // Marker at the knob's cutoff setting
        g.setColour (p.ink.withAlpha (0.4f));
        g.drawVerticalLine ((int) xForFreq (knobCutoffHz), area.getY(), area.getBottom());
    }

private:
    void timerCallback() override
    {
        const float t = type->load(), c = cutoff->load(), r = resonance->load(), l = liveCutoff.load();
        if (t != lastType || c != lastCutoff || r != lastResonance || l != lastLive)
        {
            lastType = t;
            lastCutoff = c;
            lastResonance = r;
            lastLive = l;
            repaint();
        }
    }

    static constexpr float minFreq = 20.0f, maxFreq = 20000.0f;
    static constexpr float minDb = -36.0f, maxDb = 24.0f;

    std::function<double()> getSampleRate;
    std::atomic<float>* type;
    std::atomic<float>* cutoff;
    std::atomic<float>* resonance;
    const std::atomic<float>& liveCutoff;
    float lastType = -1.0f, lastCutoff = -1.0f, lastResonance = -1.0f, lastLive = -1.0f;
};
