#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// LAPiZ's icons, all drawn the same way: one outline stroke in the text color, on a small square
// button with a rounded hairline border. Chevrons step through lists, the star marks a favorite
// (filled when on), the die makes a surprise sound and the gear opens the settings.
class IconButton : public juce::Button
{
public:
    enum class Icon { previous, next, star, die, gear };

    IconButton (const juce::String& name, Icon iconToDraw) : juce::Button (name), icon (iconToDraw)
    {
        setTitle (name); // what a screen reader says
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        const auto& p = Theme::get();
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        const float alpha = isEnabled() ? 1.0f : 0.35f;

        if ((highlighted || down) && isEnabled())
        {
            g.setColour (p.well);
            g.fillRoundedRectangle (r, 6.0f);
        }
        g.setColour (p.edge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);

        const auto c = r.getCentre();
        const float size = juce::jmin (r.getWidth(), r.getHeight()) * 0.3f; // half the icon's height
        const auto ink = (highlighted || down ? p.ink : p.soft).withMultipliedAlpha (alpha);
        const juce::PathStrokeType stroke (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.setColour (ink);

        switch (icon)
        {
            case Icon::previous:
            case Icon::next:
            {
                const float dir = icon == Icon::next ? 1.0f : -1.0f;
                juce::Path chevron;
                chevron.startNewSubPath (c.x - dir * size * 0.35f, c.y - size * 0.7f);
                chevron.lineTo (c.x + dir * size * 0.35f, c.y);
                chevron.lineTo (c.x - dir * size * 0.35f, c.y + size * 0.7f);
                g.strokePath (chevron, stroke);
                break;
            }

            case Icon::star:
            {
                juce::Path star;
                star.addStar (c, 5, size * 0.42f, size, 0.0f);
                if (getToggleState())
                {
                    g.setColour (p.ink.withMultipliedAlpha (alpha));
                    g.fillPath (star);
                }
                g.strokePath (star, stroke);
                break;
            }

            case Icon::die:
            {
                // A die showing five.
                const auto face = juce::Rectangle<float> (size * 1.9f, size * 1.9f).withCentre (c);
                g.drawRoundedRectangle (face, 3.0f, 1.5f);
                const float pip = size * 0.3f, offset = face.getWidth() * 0.26f;
                for (auto [dx, dy] : { std::pair { -1, -1 }, { 1, -1 }, { 0, 0 }, { -1, 1 }, { 1, 1 } })
                    g.fillEllipse (juce::Rectangle<float> (pip, pip).withCentre (c.translated ((float) dx * offset, (float) dy * offset)));
                break;
            }

            case Icon::gear:
            {
                // Eight teeth around a ring, drawn as one outline, with a hole in the middle.
                const float outer = size * 1.05f, inner = size * 0.78f;
                juce::Path gear;
                for (int i = 0; i < 8; ++i)
                {
                    const float mid = (float) i * juce::MathConstants<float>::twoPi / 8.0f;
                    const float base = 0.24f, top = 0.15f; // half-widths in radians
                    auto at = [&c] (float radius, float angle)
                    {
                        return juce::Point<float> (c.x + radius * std::sin (angle), c.y - radius * std::cos (angle));
                    };
                    const auto p0 = at (inner, mid - base), p1 = at (outer, mid - top), p2 = at (outer, mid + top), p3 = at (inner, mid + base);
                    if (i == 0) gear.startNewSubPath (p0); else gear.lineTo (p0);
                    gear.lineTo (p1); gear.lineTo (p2); gear.lineTo (p3);
                }
                gear.closeSubPath();
                g.strokePath (gear, stroke);
                g.drawEllipse (juce::Rectangle<float> (size * 0.7f, size * 0.7f).withCentre (c), 1.5f);
                break;
            }
        }
    }

private:
    Icon icon;
};
