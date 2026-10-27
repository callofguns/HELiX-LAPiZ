#pragma once

#include <atomic>
#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

// Draws an envelope: how something (the volume, or the filter) changes from key press to key release.
// The segment widths are scaled (square root) so short times stay visible, and the
// whole shape is stretched to fill the box, so it is a guide, not a time ruler.
class EnvelopeView : public juce::Component, private juce::Timer
{
public:
    // idPrefix "" uses attack/decay/sustain/release; "fenv" uses fenvAttack/fenvDecay/...
    // If amountID is given and that knob is below zero, the shape is drawn upside down,
    // because the envelope then pushes downwards.
    EnvelopeView (juce::AudioProcessorValueTreeState& state, const juce::String& idPrefix, Theme::Module moduleToUse,
                  const juce::String& amountID = {})
        : attack (state.getRawParameterValue (paramID (idPrefix, "attack"))),
          decay (state.getRawParameterValue (paramID (idPrefix, "decay"))),
          sustain (state.getRawParameterValue (paramID (idPrefix, "sustain"))),
          release (state.getRawParameterValue (paramID (idPrefix, "release"))),
          amount (amountID.isEmpty() ? nullptr : state.getRawParameterValue (amountID)),
          module (moduleToUse)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (p.well);
        g.fillRoundedRectangle (bounds, 6.0f);

        auto area = bounds.reduced (12.0f, 12.0f);

        const float a = width (attack->load());
        const float d = width (decay->load());
        const float r = width (release->load());
        const float s = 0.3f; // the "key held down" section has no fixed length, so it gets a fixed width
        const float total = a + d + s + r;
        const float level = sustain->load();

        auto xAt = [&] (float units) { return area.getX() + area.getWidth() * units / total; };
        const bool upsideDown = isUpsideDown();
        auto yAt = [&] (float v) { return upsideDown ? area.getY() + area.getHeight() * v
                                                     : area.getBottom() - area.getHeight() * v; };

        juce::Path shape;
        shape.startNewSubPath (xAt (0.0f), yAt (0.0f));
        shape.lineTo (xAt (a), yAt (1.0f));
        shape.lineTo (xAt (a + d), yAt (level));
        shape.lineTo (xAt (a + d + s), yAt (level));
        shape.lineTo (xAt (total), yAt (0.0f));

        const auto colour = Theme::moduleColour (module);
        auto fill = shape;
        fill.closeSubPath();
        g.setColour (colour.withAlpha (0.18f));
        g.fillPath (fill);

        g.setColour (colour);
        g.strokePath (shape, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        // Dividers showing where the key is pressed and released
        g.setColour (p.edge);
        g.drawVerticalLine ((int) xAt (a + d + s), area.getY(), area.getBottom());
        g.setColour (p.soft);
        g.setFont (Theme::regular (11.0f));
        g.drawText ("key down", juce::Rectangle<float> (area.getX(), bounds.getY() + 2.0f, 70.0f, 14.0f), juce::Justification::left);
        g.drawText ("key up", juce::Rectangle<float> (xAt (a + d + s) + 4.0f, bounds.getY() + 2.0f, 60.0f, 14.0f), juce::Justification::left);
    }

private:
    static float width (float seconds) { return std::sqrt (seconds / 5.0f); }

    static juce::String paramID (const juce::String& prefix, const juce::String& name)
    {
        return prefix.isEmpty() ? name : prefix + name.substring (0, 1).toUpperCase() + name.substring (1);
    }

    void timerCallback() override
    {
        const float a = attack->load(), d = decay->load(), s = sustain->load(), r = release->load();
        const bool flipped = isUpsideDown();
        if (a != lastA || d != lastD || s != lastS || r != lastR || flipped != lastFlipped)
        {
            lastA = a; lastD = d; lastS = s; lastR = r; lastFlipped = flipped;
            repaint();
        }
    }

    bool isUpsideDown() const { return amount != nullptr && amount->load() < 0.0f; }

    std::atomic<float>* attack;
    std::atomic<float>* decay;
    std::atomic<float>* sustain;
    std::atomic<float>* release;
    std::atomic<float>* amount;
    Theme::Module module;
    float lastA = -1.0f, lastD = -1.0f, lastS = -1.0f, lastR = -1.0f;
    bool lastFlipped = false;
};
