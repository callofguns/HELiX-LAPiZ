#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Theme.h"

// The piano along the bottom of the window. Click (or drag across) the keys to play LAPiZ without
// a MIDI keyboard; notes coming from FL Studio light up too, so you can see what's playing.
// Notes are named the way FL Studio names them: middle C (MIDI note 60) is C5.
// It reads the current look every time it paints, so switching looks only needs a repaint.
class PianoKeyboard : public juce::MidiKeyboardComponent
{
public:
    static constexpr int lowestNote = 24;  // C2 in FL Studio's naming
    static constexpr int highestNote = 108; // C9

    explicit PianoKeyboard (juce::MidiKeyboardState& state)
        : juce::MidiKeyboardComponent (state, juce::KeyboardComponentBase::horizontalKeyboard)
    {
        setAvailableRange (lowestNote, highestNote);
        setOctaveForMiddleC (5);
        setScrollButtonsVisible (false);
        setVelocity (0.8f, true); // clicking higher up a key plays it louder, like the synth's own velocity
        setMidiChannel (1);
    }

    void resized() override
    {
        // Fit every white key in the range across the full width.
        int whiteKeys = 0;
        for (int note = lowestNote; note <= highestNote; ++note)
            if (! juce::MidiMessage::isMidiNoteBlack (note))
                ++whiteKeys;
        setKeyWidth ((float) getWidth() / (float) whiteKeys);
        juce::MidiKeyboardComponent::resized();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::get().ground);
        juce::MidiKeyboardComponent::paint (g);
    }

    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour, juce::Colour) override
    {
        const auto& p = Theme::get();
        const bool dark = Theme::currentLook() == Theme::Look::dark;
        const auto key = area.reduced (0.5f, 0.0f);

        auto fill = dark ? p.ink : juce::Colours::white;
        if (isDown)      fill = p.sound;
        else if (isOver) fill = fill.interpolatedWith (p.sound, 0.18f);

        g.setColour (fill);
        g.fillRoundedRectangle (key.withTrimmedTop (-4.0f), 4.0f); // round only the bottom corners
        g.setColour (p.edge);
        g.drawRoundedRectangle (key.withTrimmedTop (-4.0f), 4.0f, 1.0f);

        // Label each C, so you can find your way around: C2 ... C9.
        if (note % 12 == 0)
        {
            g.setColour (isDown ? p.onAccent : (dark ? p.ground : p.soft));
            g.setFont (Theme::regular (juce::jmin (11.0f, area.getWidth() * 0.62f)));
            g.drawText (getWhiteNoteText (note), key.withTop (key.getBottom() - 16.0f), juce::Justification::centred, false);
        }
    }

    void drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour) override
    {
        const auto& p = Theme::get();
        const bool dark = Theme::currentLook() == Theme::Look::dark;

        auto fill = dark ? p.well : p.ink;
        if (isDown)      fill = p.sound;
        else if (isOver) fill = fill.interpolatedWith (p.sound, 0.3f);

        g.setColour (fill);
        g.fillRoundedRectangle (area.withTrimmedTop (-3.0f).reduced (0.5f, 0.0f), 3.0f);
    }

    juce::String getWhiteNoteText (int note) override
    {
        return note % 12 == 0 ? juce::MidiMessage::getMidiNoteName (note, true, true, 5) : juce::String();
    }
};
