#pragma once

#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../PluginProcessor.h"

// Helix's signature display: the whole wavetable drawn as a stack of waves going back
// in depth, with the wave you are hearing right now lit up in the Sound colour.
// Along the bottom: the waveform names (if the table has them), or which frame you are on.
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (HelixAudioProcessor& processorToShow)
        : processor (processorToShow),
          position (processorToShow.getState().getRawParameterValue ("position"))
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (p.well);
        g.fillRoundedRectangle (bounds, 6.0f);

        const auto table = processor.getTable();
        if (table == nullptr)
            return;

        auto namesArea = bounds.reduced (6.0f, 0.0f).removeFromBottom (22.0f);
        auto area = bounds.reduced (10.0f, 12.0f);
        area.removeFromBottom (20.0f);

        const int numFrames = table->getNumFrames();
        const int numStacked = numFrames > 1 ? 16 : 2; // blended layers between the waves, for the stacked look
        const float depthX = juce::jmin (4.0f, area.getWidth() / 70.0f);
        const float depthY = juce::jmin (3.2f, area.getHeight() / 55.0f);
        const float waveWidth = area.getWidth() - depthX * (float) (numStacked - 1);
        const float amplitude = (area.getHeight() - depthY * (float) (numStacked - 1)) * 0.5f * 0.85f;
        const float frontCentreY = area.getBottom() - amplitude / 0.85f;

        const float pos = currentPosition();
        const float liveDepth = pos * (float) (numStacked - 1);

        // Draws one wave of the table at the given depth (0 = front, numStacked - 1 = back).
        auto wavePath = [&] (float tablePosition, float depth)
        {
            const float left = area.getX() + depth * depthX;
            const float centreY = frontCentreY - depth * depthY;
            juce::Path path;
            constexpr int numPoints = 120;
            for (int i = 0; i <= numPoints; ++i)
            {
                const float phase = 0.9999f * (float) i / (float) numPoints;
                const float x = left + waveWidth * (float) i / (float) numPoints;
                const float y = centreY - juce::jlimit (-1.2f, 1.2f, table->getSample (phase, tablePosition)) * amplitude;
                if (i == 0) path.startNewSubPath (x, y);
                else        path.lineTo (x, y);
            }
            return path;
        };

        // The rest of the table, back to front, fading with distance from the live wave.
        for (int j = numStacked - 1; j >= 0; --j)
        {
            const float closeness = 1.0f - std::abs ((float) j - liveDepth) / (float) numStacked;
            g.setColour (p.soft.withAlpha (0.06f + 0.18f * closeness));
            g.strokePath (wavePath ((float) j / (float) (numStacked - 1), (float) j), juce::PathStrokeType (1.0f));
        }

        // The wave you hear.
        g.setColour (p.sound);
        g.strokePath (wavePath (pos, liveDepth), juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        const auto& names = table->getFrameNames();
        const int closest = juce::roundToInt (pos * (float) (numFrames - 1));

        if (names.size() == numFrames)
        {
            // Waveform names, with the closest one lit up
            const float slot = namesArea.getWidth() / (float) numFrames;
            for (int i = 0; i < numFrames; ++i)
            {
                g.setColour (i == closest ? p.sound : p.soft);
                g.setFont (i == closest ? Theme::bold (12.0f) : Theme::regular (12.0f));
                g.drawText (names[i], juce::Rectangle<float> (namesArea.getX() + slot * (float) i, namesArea.getY(),
                                                              slot, namesArea.getHeight()),
                            juce::Justification::centred);
            }
        }
        else
        {
            g.setColour (p.soft);
            g.setFont (Theme::regular (12.0f));
            g.drawText (numFrames == 1 ? juce::String ("1 wave (Wave Shape has nothing to scan)")
                                       : "Wave " + juce::String (closest + 1) + " of " + juce::String (numFrames),
                        namesArea, juce::Justification::centred);
        }
    }

private:
    float currentPosition() const
    {
        const float live = processor.getLive().position.load();
        return live >= 0.0f ? live : position->load();
    }

    void timerCallback() override
    {
        const float now = currentPosition();
        const auto* table = processor.getTable().get();
        if (now != lastPosition || table != lastTable)
        {
            lastPosition = now;
            lastTable = table;
            repaint();
        }
    }

    HelixAudioProcessor& processor;
    std::atomic<float>* position;
    float lastPosition = -1.0f;
    const Wavetable* lastTable = nullptr;
};
