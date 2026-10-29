#include "SimplePage.h"

namespace
{
    constexpr int panelTitleHeight = Panel::titleHeight;
    constexpr int knobRowHeight = 112;
    constexpr int panelGap = 26;   // wide enough for the arrow between panels
    constexpr int pagePadding = 16;

    struct PanelInfo { const char* number; const char* title; const char* subtitle; Theme::Module module; };

    const PanelInfo panelInfo[3] = {
        { "1", "Sound", "Pick the raw tone",             Theme::Module::sound },
        { "2", "Tone",  "Darker, brighter or warmer",    Theme::Module::tone },
        { "3", "Shape", "How each note starts and ends", Theme::Module::shape }
    };
}

SimplePage::SimplePage (LapizAudioProcessor& processor, HelpRegistrar registerHelp)
    : picker (processor),
      waveView (processor),
      position (processor.getState(), "position", "Wave Shape", "Wave Position", Theme::Module::sound, Format::percent),
      wobble (processor.getState(), "lfoAmount", "Wobble", "LFO Amount", Theme::Module::motion, Format::percent),
      volume (processor.getState(), "volume", "Volume", "Output level", Theme::Module::sound, Format::percent),
      filterView (processor.getState(), [&processor] { return processor.getSampleRate(); }, processor.getLive().cutoff),
      cutoff (processor.getState(), "cutoff", "Brightness", "Filter Cutoff", Theme::Module::tone, Format::hertz),
      resonance (processor.getState(), "resonance", "Emphasis", "Resonance", Theme::Module::tone, Format::percent),
      analog (processor.getState(), "analog", "Analog", "Drift & warmth", Theme::Module::tone, Format::percent),
      envelopeView (processor.getState(), {}, Theme::Module::shape),
      attack (processor.getState(), "attack", "Fade In", "Attack", Theme::Module::shape, Format::seconds),
      decay (processor.getState(), "decay", "Drop", "Decay", Theme::Module::shape, Format::seconds),
      sustain (processor.getState(), "sustain", "Hold Level", "Sustain", Theme::Module::shape, Format::percent),
      release (processor.getState(), "release", "Fade Out", "Release", Theme::Module::shape, Format::seconds)
{
    for (juce::Component* c : { (juce::Component*) &picker, (juce::Component*) &waveView, (juce::Component*) &position, (juce::Component*) &wobble,
                                (juce::Component*) &volume,
                                (juce::Component*) &filterView, (juce::Component*) &cutoff, (juce::Component*) &resonance, (juce::Component*) &analog,
                                (juce::Component*) &envelopeView, (juce::Component*) &attack, (juce::Component*) &decay,
                                (juce::Component*) &sustain, (juce::Component*) &release })
        addAndMakeVisible (c);

    // Filter type buttons
    const char* typeNames[3] = { "Low-pass", "High-pass", "Band-pass" };
    const char* typeHelp[3] = {
        "Low-pass: lets the low sounds through and cuts the highs. The classic synth sound - warm and dark when turned down.",
        "High-pass: lets the high sounds through and cuts the lows. Thin and airy when turned up.",
        "Band-pass: keeps only a band of sound around the Brightness setting and cuts both the lows and the highs. Nasal, like a small radio."
    };

    typeAttachment = std::make_unique<juce::ParameterAttachment> (
        *processor.getState().getParameter ("filterType"),
        [this] (float newValue) { updateTypeButtons ((int) newValue); });

    for (int i = 0; i < 3; ++i)
    {
        auto& button = typeButtons[i];
        button.setButtonText (typeNames[i]);
        button.setClickingTogglesState (true);
        button.setRadioGroupId (1002);
        Theme::setModule (button, Theme::Module::tone);
        button.onClick = [this, i] { typeAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (button);
        registerHelp (button, typeHelp[i]);
    }
    typeAttachment->sendInitialUpdate();

    // The Wobble knob says what it is moving, since that is chosen on the Advanced tab.
    lfoTargetAttachment = std::make_unique<juce::ParameterAttachment> (
        *processor.getState().getParameter ("lfoTarget"),
        [this] (float newValue)
        {
            const int target = juce::jlimit (0, (int) Lfo::targetNames.size() - 1, (int) newValue);
            wobble.setTechnicalText (juce::String ("LFO: ") + Lfo::targetNames[(size_t) target]);
        });
    lfoTargetAttachment->sendInitialUpdate();

    // Hover help text
    auto helpForKnob = [&] (LabeledKnob& knob, const juce::String& text)
    {
        registerHelp (knob, text);
        registerHelp (knob.getSlider(), text);
    };

    registerHelp (picker.getNameButton(), "Wavetable: the set of waves LAPiZ plays. Click for the full list: built-in tables, "
                                          "your saved ones and your latest drawing. Draw your own on the Waves tab.");
    registerHelp (picker.getPreviousButton(), "Previous wavetable.");
    registerHelp (picker.getNextButton(), "Next wavetable.");
    registerHelp (waveView, "Every wave in the wavetable, stacked from front to back. The bright one is the wave you hear right now. Turn Wave Shape to move through them.");
    helpForKnob (position, "Wave Shape (Wavetable Position): moves through the waves in the wavetable, from the front one to the back one. "
                           "Try turning it while holding a note. Double-click to reset.");
    helpForKnob (wobble, "Wobble (LFO Amount): adds a repeating movement to the sound. The small text shows what it moves. "
                         "Pick what it moves, its shape and its speed on the Advanced tab. Double-click to reset.");
    helpForKnob (volume, "Volume: the overall loudness of the synth. Double-click to reset.");

    registerHelp (filterView, "This curve shows which pitches the filter lets through. Left is bass, right is treble. Higher on the curve means more of that sound gets through.");
    helpForKnob (cutoff, "Brightness (Filter Cutoff): where the filter starts to cut. With Low-pass, turn it down for a darker, muffled sound and up for a bright, open one. Double-click to reset.");
    helpForKnob (resonance, "Emphasis (Resonance): boosts the sound right at the Brightness point. A little adds character; a lot makes it whistle or sound vocal. Double-click to reset.");
    helpForKnob (analog, "Analog: makes LAPiZ sound like an old hardware synth. Notes drift slightly out of tune, every note is a little different, "
                         "and the filter and output get warmer and rounder. 0 is clean and digital. Double-click to reset.");

    registerHelp (envelopeView, "This shows how the loudness of a note changes over time: from the moment you press a key, to the moment you let go.");
    helpForKnob (attack, "Fade In (Attack): how long a note takes to reach full volume after you press a key. Short is punchy, long is a slow swell. Double-click to reset.");
    helpForKnob (decay, "Drop (Decay): after reaching full volume, how long the note takes to fall down to the Hold Level. Double-click to reset.");
    helpForKnob (sustain, "Hold Level (Sustain): the volume the note stays at while you keep the key held down. Double-click to reset.");
    helpForKnob (release, "Fade Out (Release): how long the sound takes to die away after you let go of the key. Double-click to reset.");
}

void SimplePage::updateTypeButtons (int selectedIndex)
{
    for (int i = 0; i < 3; ++i)
        typeButtons[i].setToggleState (i == selectedIndex, juce::dontSendNotification);
}

void SimplePage::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();

    for (int i = 0; i < 3; ++i)
    {
        const auto rect = panelRects[i];
        Panel::draw (g, rect, panelInfo[i].module, panelInfo[i].number, panelInfo[i].title, panelInfo[i].subtitle);

        // Arrow to the next step: this is where the sound goes next.
        if (i < 2)
        {
            const float x = rect.getRight() + (float) panelGap * 0.5f;
            const float y = rect.getY() + 26.0f;
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

void SimplePage::resized()
{
    auto area = getLocalBounds().reduced (pagePadding, 8);
    const int panelWidth = (area.getWidth() - 2 * panelGap) / 3;

    juce::Rectangle<int> panels[3];
    for (int i = 0; i < 3; ++i)
    {
        panels[i] = area.removeFromLeft (panelWidth);
        area.removeFromLeft (panelGap);
        panelRects[i] = panels[i].toFloat();
    }

    // Splits a row into n equal slots.
    auto knobRow = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> knobs)
    {
        const int slot = row.getWidth() / (int) knobs.size();
        for (auto* knob : knobs)
            knob->setBounds (row.removeFromLeft (slot));
    };

    // Panel 1: Sound
    {
        auto inner = panels[0].reduced (14, 0);
        inner.removeFromTop (panelTitleHeight);
        inner.removeFromBottom (10);
        knobRow (inner.removeFromBottom (knobRowHeight), { &position, &wobble, &volume });
        inner.removeFromBottom (10);
        picker.setBounds (inner.removeFromTop (30));
        inner.removeFromTop (8);
        waveView.setBounds (inner);
    }

    // Panel 2: Tone
    {
        auto inner = panels[1].reduced (14, 0);
        inner.removeFromTop (panelTitleHeight);
        inner.removeFromBottom (10);
        knobRow (inner.removeFromBottom (knobRowHeight), { &cutoff, &resonance, &analog });
        inner.removeFromBottom (10);

        auto buttons = inner.removeFromBottom (30);
        const int buttonWidth = (buttons.getWidth() - 2 * 6) / 3;
        for (auto& button : typeButtons)
        {
            button.setBounds (buttons.removeFromLeft (buttonWidth));
            buttons.removeFromLeft (6);
        }
        inner.removeFromBottom (10);
        filterView.setBounds (inner);
    }

    // Panel 3: Shape
    {
        auto inner = panels[2].reduced (14, 0);
        inner.removeFromTop (panelTitleHeight);
        inner.removeFromBottom (10);
        knobRow (inner.removeFromBottom (knobRowHeight), { &sustain, &release });
        knobRow (inner.removeFromBottom (knobRowHeight), { &attack, &decay });
        inner.removeFromBottom (10);
        envelopeView.setBounds (inner);
    }
}
