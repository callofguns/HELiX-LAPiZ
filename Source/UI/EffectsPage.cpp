#include "EffectsPage.h"

namespace
{
    constexpr int knobRowHeight = 112;
    constexpr int panelGap = 26;   // wide enough for the arrow between panels
    constexpr int pagePadding = 16;

    juce::String divisionText (double index)
    {
        const int i = juce::jlimit (0, (int) Lfo::divisions.size() - 1, juce::roundToInt (index));
        return Lfo::divisions[(size_t) i].name;
    }

    juce::String rateText (double hz) { return juce::String (hz, 2) + " Hz"; }

    juce::String driveText (double v) { return "+" + juce::String (juce::roundToInt (v * Effects::maxDriveDb)) + " dB"; }

    juce::String toneText (double v) { return Format::hertz (Effects::toneToHz ((float) v)); }

    struct KnobInfo
    {
        const char* paramID;
        const char* title;
        const char* technicalName;
        std::function<juce::String (double)> format;
        const char* help;
    };

    struct EffectInfo
    {
        const char* onParamID;
        const char* title;
        const char* subtitle;
        const char* blurb;
        const char* onHelp;
        KnobInfo knobs[3];
    };

    const EffectInfo effectInfo[4] = {
        { "distOn", "Grit", "Distortion",
          "Adds crunch and warmth by pushing the sound until it breaks up. Great on basses and leads.",
          "Turns the distortion on or off.",
          { { "distDrive", "Drive", "Distortion Drive", driveText,
              "Drive: how hard the sound is pushed. A little adds warmth; a lot gives a fuzzy, aggressive sound. Double-click to reset." },
            { "distTone", "Tone", "Distortion Tone", toneText,
              "Tone: smooths off the harsh top of the distortion. Turn it down for a darker, rounder crunch. Double-click to reset." },
            { "distMix", "Mix", "Dry/Wet", Format::percent,
              "Mix: how much of the distorted sound you hear. Below 100 % blends in the clean sound, which keeps the punch. Double-click to reset." } } },

        { "chorusOn", "Chorus", "Chorus",
          "Makes one sound feel like several playing together, so it gets wider and shimmery. Great on pads and keys.",
          "Turns the chorus on or off.",
          { { "chorusRate", "Speed", "Chorus Rate", rateText,
              "Speed (Chorus Rate): how fast the shimmer moves. Slow is lush; fast gets wobbly. Double-click to reset." },
            { "chorusDepth", "Depth", "Chorus Depth", Format::percent,
              "Depth: how far the shimmer moves the pitch. More depth is more obvious and seasick. Double-click to reset." },
            { "chorusMix", "Mix", "Dry/Wet", Format::percent,
              "Mix: how much of the chorus you hear, blended with the clean sound. Double-click to reset." } } },

        { "delayOn", "Echo", "Delay",
          "Repeats each note in time with the song, bouncing between your left and right speakers.",
          "Turns the echo (delay) on or off.",
          { { "delayDivision", "Time", "Delay Time", divisionText,
              "Time (Delay Time): the gap between echoes, in note lengths, locked to FL Studio's tempo. 1/4 is one beat. "
              "D = dotted (1.5x longer), T = triplet (shorter). Double-click to reset." },
            { "delayFeedback", "Repeats", "Feedback", Format::percent,
              "Repeats (Feedback): how long the echoes keep going. Low gives one or two echoes; high gives a long trail. Double-click to reset." },
            { "delayMix", "Amount", "Delay Mix", Format::percent,
              "Amount (Delay Mix): how loud the echoes are. The clean sound always stays at full level. Double-click to reset." } } },

        { "reverbOn", "Space", "Reverb",
          "Puts the sound in a room or hall, so it rings on after each note. Great on pads, leads and plucks.",
          "Turns the reverb on or off.",
          { { "reverbSize", "Size", "Room Size", Format::percent,
              "Size (Room Size): how big the room is. Small is a tight room; large is a long, washy hall. Double-click to reset." },
            { "reverbDamping", "Darkness", "Damping", Format::percent,
              "Darkness (Damping): how quickly the bright part of the reverb fades. Higher sounds warmer and softer. Double-click to reset." },
            { "reverbMix", "Amount", "Reverb Mix", Format::percent,
              "Amount (Reverb Mix): how loud the reverb is. The clean sound always stays at full level. Double-click to reset." } } }
    };
}

EffectsPage::EffectsPage (LapizAudioProcessor& processor, HelpRegistrar registerHelp)
{
    auto& state = processor.getState();

    for (size_t s = 0; s < slots.size(); ++s)
    {
        const auto& info = effectInfo[s];
        auto& slot = slots[s];
        slot.title = info.title;
        slot.subtitle = info.subtitle;
        slot.blurb = info.blurb;

        for (size_t k = 0; k < 3; ++k)
        {
            const auto& knobInfo = info.knobs[k];
            slot.knobs[k] = std::make_unique<LabeledKnob> (state, knobInfo.paramID, knobInfo.title, knobInfo.technicalName,
                                                           Theme::Module::effects, knobInfo.format);
            addAndMakeVisible (*slot.knobs[k]);
            registerHelp (*slot.knobs[k], knobInfo.help);
            registerHelp (slot.knobs[k]->getSlider(), knobInfo.help);
        }

        slot.onButton.setClickingTogglesState (true);
        Theme::setModule (slot.onButton, Theme::Module::effects);
        slot.onButton.onStateChange = [this, &slot] { showOnState (slot); };
        addAndMakeVisible (slot.onButton);
        registerHelp (slot.onButton, info.onHelp);
        slot.onAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, info.onParamID, slot.onButton);
        showOnState (slot);
    }
}

void EffectsPage::showOnState (Slot& slot)
{
    const bool on = slot.onButton.getToggleState();
    const auto text = on ? "On" : "Off";
    if (slot.onButton.getButtonText() != text)
        slot.onButton.setButtonText (text);

    for (auto& knob : slot.knobs)
        knob->setAlpha (on ? 1.0f : 0.4f);
}

void EffectsPage::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();

    for (size_t s = 0; s < slots.size(); ++s)
    {
        const auto& slot = slots[s];
        Panel::draw (g, slot.panel, Theme::Module::effects, juce::String ((int) s + 1), slot.title, slot.subtitle);

        g.setColour (p.soft);
        g.setFont (Theme::regular (13.0f));
        g.drawFittedText (slot.blurb, slot.blurbArea, juce::Justification::topLeft, 5);

        // Arrow to the next effect: this is where the sound goes next.
        if (s + 1 < slots.size())
        {
            const float x = slot.panel.getRight() + (float) panelGap * 0.5f;
            const float y = slot.panel.getY() + 26.0f;
            juce::Path arrow;
            arrow.startNewSubPath (x - 7.0f, y);
            arrow.lineTo (x + 6.0f, y);
            arrow.startNewSubPath (x + 1.0f, y - 5.0f);
            arrow.lineTo (x + 6.0f, y);
            arrow.lineTo (x + 1.0f, y + 5.0f);
            g.setColour (p.soft);
            g.strokePath (arrow, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

void EffectsPage::resized()
{
    auto area = getLocalBounds().reduced (pagePadding, 8);
    const int panelWidth = (area.getWidth() - 3 * panelGap) / 4;

    for (auto& slot : slots)
    {
        auto panel = area.removeFromLeft (panelWidth);
        area.removeFromLeft (panelGap);
        slot.panel = panel.toFloat();

        auto inner = panel.reduced (14, 0);
        inner.removeFromTop (Panel::titleHeight);
        inner.removeFromBottom (10);

        // Bottom up: the third knob centred, the first two side by side, then the on/off switch and the description.
        auto lowerRow = inner.removeFromBottom (knobRowHeight);
        slot.knobs[2]->setBounds (lowerRow.withSizeKeepingCentre (lowerRow.getWidth() / 2, lowerRow.getHeight()));
        inner.removeFromBottom (6);
        auto upperRow = inner.removeFromBottom (knobRowHeight);
        slot.knobs[0]->setBounds (upperRow.removeFromLeft (upperRow.getWidth() / 2));
        slot.knobs[1]->setBounds (upperRow);

        inner.removeFromTop (6);
        slot.onButton.setBounds (inner.removeFromTop (30));
        inner.removeFromTop (12);
        slot.blurbArea = inner;
    }
}
