#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// The rounded panels both pages are built from: a band of the module's colour along the top,
// a title (with an optional step number) and a short line saying what the panel is for.
namespace Panel
{
    inline constexpr int titleHeight = 50;

    inline void draw (juce::Graphics& g, juce::Rectangle<float> rect, Theme::Module module,
                      const juce::String& number, const juce::String& title, const juce::String& subtitle)
    {
        const auto& p = Theme::get();
        const auto colour = Theme::moduleColour (module);

        g.setColour (colour);
        g.fillRoundedRectangle (rect, 10.0f);
        g.setColour (p.panel);
        g.fillRoundedRectangle (rect.withTrimmedTop (4.0f), 10.0f);

        auto header = rect.reduced (14.0f, 0.0f).withTrimmedTop (12.0f).removeFromTop ((float) titleHeight - 12.0f);
        auto titleRow = header.removeFromTop (22.0f);

        g.setFont (Theme::bold (17.0f));
        if (number.isNotEmpty())
        {
            g.setColour (colour);
            g.drawText (number, titleRow.removeFromLeft (16.0f), juce::Justification::centredLeft);
        }

        g.setColour (p.ink);
        g.drawText (title, titleRow, juce::Justification::centredLeft);

        g.setColour (p.soft);
        g.setFont (Theme::regular (12.5f));
        g.drawText (subtitle, header.removeFromTop (16.0f), juce::Justification::centredLeft);
    }
}
