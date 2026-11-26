#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Icons.h"
#include "Theme.h"

// Draws JUCE's built-in controls (knobs, buttons, menus) in LAPiZ's style.
// Everything reads the current palette when it paints, so switching looks only needs a repaint.
class LapizLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LapizLookAndFeel() { refreshColours(); }

    // Menus keep their own colour settings, so they are updated when the look changes.
    void refreshColours()
    {
        const auto& p = Theme::get();
        setColour (juce::PopupMenu::backgroundColourId, p.panel);
        setColour (juce::PopupMenu::textColourId, p.ink);
        setColour (juce::PopupMenu::headerTextColourId, p.soft);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, p.well);
        setColour (juce::PopupMenu::highlightedTextColourId, p.ink);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float position, float startAngle, float endAngle, juce::Slider& slider) override
    {
        const auto& p = Theme::get();
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float radius = (juce::jmin (bounds.getWidth(), bounds.getHeight()) - 10.0f) * 0.5f;
        if (radius <= 4.0f)
            return;

        const auto centre = bounds.getCentre();
        const float thickness = juce::jmax (3.0f, radius * 0.2f);
        const float angle = startAngle + position * (endAngle - startAngle);
        const juce::PathStrokeType stroke (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (p.edge);
        g.strokePath (track, stroke);

        // Knobs that go below and above zero (like Sweep) fill from the top centre instead of the left.
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const float fromAngle = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

        if (angle != fromAngle)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                 juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
            g.setColour (Theme::moduleColour (Theme::moduleOf (slider)));
            g.strokePath (value, stroke);
        }

        const float dot = thickness * 1.7f;
        g.setColour (p.ink);
        g.fillEllipse (juce::Rectangle<float> (dot, dot).withCentre (centre.getPointOnCircumference (radius, angle)));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        const auto& p = Theme::get();
        const auto r = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto module = Theme::moduleOf (button);

        if (! button.isEnabled())
            g.setOpacity (0.4f);

        if (button.getToggleState())
        {
            g.setColour (module == Theme::Module::none ? p.ink : Theme::moduleColour (module));
            g.fillRoundedRectangle (r, 6.0f);
            return;
        }

        if (highlighted || down)
        {
            g.setColour (p.well);
            g.fillRoundedRectangle (r, 6.0f);
        }
        g.setColour (p.edge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        const auto& p = Theme::get();
        const bool on = button.getToggleState();
        const auto module = Theme::moduleOf (button);

        g.setColour ((on ? (module == Theme::Module::none ? p.ground : p.onAccent) : p.ink)
                         .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));
        g.setFont (on ? Theme::bold (14.0f) : Theme::regular (14.0f));
        // Long names (like "Haunted Music Box" in the preset bar) squeeze a little rather than getting cut off.
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (3, 0), juce::Justification::centred, 1, 0.85f);
    }

    juce::Font getPopupMenuFont() override { return Theme::regular (15.0f); }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        juce::SharedResourcePointer<Theme::Fonts> fonts;
        return font.isBold() ? fonts->bold : fonts->regular;
    }
};

// The gear button that opens the settings menu.
class SettingsButton : public IconButton
{
public:
    SettingsButton() : IconButton ("Settings", IconButton::Icon::gear) {}
};
