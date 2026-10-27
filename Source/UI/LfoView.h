#pragma once

#include <atomic>
#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "../Lfo.h"

namespace LfoDrawing
{
    // The path of the LFO shape across the given box. Random shows `cycles` steps starting at firstCycle.
    inline juce::Path shapePath (int shape, juce::Rectangle<float> box, double firstCycle = 0.0, int cycles = 1)
    {
        juce::Path path;
        constexpr int pointsPerCycle = 96;
        const int numPoints = pointsPerCycle * cycles;
        for (int i = 0; i <= numPoints; ++i)
        {
            const double c = firstCycle + (double) cycles * juce::jmin (0.99999, (double) i / numPoints);
            const float x = box.getX() + box.getWidth() * (float) i / (float) numPoints;
            const float y = box.getCentreY() - Lfo::valueAt (shape, c, 0u) * box.getHeight() * 0.5f;
            if (i == 0) path.startNewSubPath (x, y);
            else        path.lineTo (x, y);
        }
        return path;
    }
}

// Shows the LFO's shape, with a dot riding along it at the LFO's current position,
// so you can see the movement you are hearing.
class LfoView : public juce::Component, private juce::Timer
{
public:
    LfoView (juce::AudioProcessorValueTreeState& state, const std::atomic<double>& liveCycles)
        : shape (state.getRawParameterValue ("lfoShape")),
          amount (state.getRawParameterValue ("lfoAmount")),
          cycles (liveCycles)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (p.well);
        g.fillRoundedRectangle (bounds, 6.0f);

        auto area = bounds.reduced (14.0f, 16.0f);
        const int s = (int) shape->load();
        const double now = cycles.load();

        // Random shows a run of four steps so you can see it jumping; the others show one cycle.
        const int shown = s == Lfo::random ? 4 : 1;
        const double windowStart = std::floor (now / shown) * shown;
        const float progress = (float) ((now - windowStart) / shown);

        g.setColour (p.edge);
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

        // Full-size shape faintly, and the shape at the current Wobble amount on top.
        const float depth = amount->load();
        g.setColour (p.motion.withAlpha (0.25f));
        g.strokePath (LfoDrawing::shapePath (s, area, s == Lfo::random ? windowStart : 0.0, shown), juce::PathStrokeType (1.2f));

        const auto scaled = area.withSizeKeepingCentre (area.getWidth(), area.getHeight() * depth);
        g.setColour (p.motion);
        g.strokePath (LfoDrawing::shapePath (s, scaled, s == Lfo::random ? windowStart : 0.0, shown),
                      juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        const float x = area.getX() + area.getWidth() * progress;
        const float y = scaled.getCentreY() - Lfo::valueAt (s, s == Lfo::random ? now : now - std::floor (now), 0u)
                                              * scaled.getHeight() * 0.5f;
        g.setColour (p.ink);
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ x, y }));

        if (depth <= 0.0f)
        {
            g.setColour (p.soft);
            g.setFont (Theme::regular (12.0f));
            g.drawText ("Turn up Wobble to hear it", bounds.reduced (10.0f, 4.0f).removeFromBottom (16.0f),
                        juce::Justification::centredRight);
        }
    }

private:
    void timerCallback() override { repaint(); }

    std::atomic<float>* shape;
    std::atomic<float>* amount;
    const std::atomic<double>& cycles;
};

// A button showing a small picture of one LFO shape.
class LfoShapeButton : public juce::Button
{
public:
    explicit LfoShapeButton (int shapeToShow) : juce::Button (Lfo::shapeNames[(size_t) shapeToShow]), shape (shapeToShow) {}

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        const auto& p = Theme::get();
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();

        if (on)
        {
            g.setColour (p.motion);
            g.fillRoundedRectangle (r, 6.0f);
        }
        else
        {
            if (highlighted || down)
            {
                g.setColour (p.well);
                g.fillRoundedRectangle (r, 6.0f);
            }
            g.setColour (p.edge);
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
        }

        const auto icon = r.withSizeKeepingCentre (juce::jmin (30.0f, r.getWidth() - 12.0f), r.getHeight() - 14.0f);
        g.setColour (on ? p.onAccent : p.ink);
        g.strokePath (LfoDrawing::shapePath (shape, icon, 0.0, shape == Lfo::random ? 4 : 1),
                      juce::PathStrokeType (1.8f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }

private:
    int shape;
};
