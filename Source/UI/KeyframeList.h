#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../Wavetable.h"

// Small pictures of each keyframe in your drawing, two per row. Click one to draw on it.
// Wave Shape blends from keyframe 1 (fully left) to the last keyframe (fully right).
class KeyframeList : public juce::Component
{
public:
    static constexpr int maxKeyframes = 8;

    std::function<void (int)> onSelect;

    // keyframes may be nullptr (nothing to show).
    void setKeyframes (const std::vector<Wavetable::Frame>* keyframes, int selectedIndex)
    {
        frames = keyframes;
        selected = selectedIndex;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();

        if (frames == nullptr || frames->empty())
        {
            g.setColour (p.soft);
            g.setFont (Theme::regular (13.0f));
            g.drawFittedText ("Your keyframes will show here when you draw.", getLocalBounds().reduced (8),
                              juce::Justification::centred, 3);
            return;
        }

        for (int i = 0; i < (int) frames->size(); ++i)
        {
            const auto box = cellBounds (i).toFloat().reduced (3.0f);
            const bool isSelected = i == selected;

            g.setColour (p.well);
            g.fillRoundedRectangle (box, 6.0f);
            g.setColour (isSelected ? p.sound : p.edge);
            g.drawRoundedRectangle (box, 6.0f, isSelected ? 2.0f : 1.0f);

            g.setColour (isSelected ? p.sound : p.soft);
            g.setFont (isSelected ? Theme::bold (12.0f) : Theme::regular (12.0f));
            g.drawText (juce::String (i + 1), box.reduced (6.0f, 3.0f).toNearestInt(), juce::Justification::topLeft);

            const auto waveArea = box.reduced (22.0f, 8.0f).withTrimmedRight (-12.0f);
            const auto& frame = (*frames)[(size_t) i];
            juce::Path wave;
            constexpr int points = 64;
            for (int k = 0; k <= points; ++k)
            {
                const int s = juce::jmin (Wavetable::tableSize - 1, k * Wavetable::tableSize / points);
                const float x = waveArea.getX() + waveArea.getWidth() * (float) k / (float) points;
                const float y = waveArea.getCentreY() - juce::jlimit (-1.0f, 1.0f, frame[(size_t) s]) * waveArea.getHeight() * 0.5f;
                if (k == 0) wave.startNewSubPath (x, y);
                else        wave.lineTo (x, y);
            }
            g.setColour (isSelected ? p.sound : p.ink.withAlpha (0.7f));
            g.strokePath (wave, juce::PathStrokeType (1.4f));
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (frames == nullptr)
            return;

        for (int i = 0; i < (int) frames->size(); ++i)
            if (cellBounds (i).contains (e.getPosition()))
            {
                if (onSelect) onSelect (i);
                return;
            }
    }

private:
    juce::Rectangle<int> cellBounds (int index) const
    {
        const int columns = 2, rows = maxKeyframes / columns;
        const int w = getWidth() / columns, h = getHeight() / rows;
        return { (index % columns) * w, (index / columns) * h, w, h };
    }

    const std::vector<Wavetable::Frame>* frames = nullptr;
    int selected = 0;
};
