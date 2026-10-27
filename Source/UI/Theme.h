#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Colours and fonts for the interface.
// Each step of the sound has its own colour (Sound, Tone, Shape), used by that step's
// knobs and display, so you can follow the signal from left to right.
// Motion (the LFO) has a colour of its own, because it can move any of them, and so do the effects.
// There are two looks, Dark and Daylight, chosen in the settings menu.
namespace Theme
{
    enum class Look { dark, daylight };

    // Stored on components as the "module" property so the drawing code knows which colour to use.
    enum class Module { none = 0, sound, tone, shape, motion, effects };

    struct Palette
    {
        juce::Colour ground, panel, well, edge, ink, soft;
        juce::Colour sound, tone, shape, motion, effects;
        juce::Colour onAccent; // text drawn on top of a module colour
    };

    inline const Palette darkPalette {
        juce::Colour (0xff222936), juce::Colour (0xff2b3342), juce::Colour (0xff1b212b),
        juce::Colour (0xff3a4456), juce::Colour (0xffeef1f6), juce::Colour (0xff9aa4b5),
        juce::Colour (0xff43c6c0), juce::Colour (0xffa193ff), juce::Colour (0xffff8fa3), juce::Colour (0xfff2b84b), juce::Colour (0xff6bb8ff),
        juce::Colour (0xff12161d)
    };

    inline const Palette daylightPalette {
        juce::Colour (0xffdfe5ec), juce::Colour (0xfff8fafc), juce::Colour (0xffeef2f6),
        juce::Colour (0xffc9d2dc), juce::Colour (0xff1c2330), juce::Colour (0xff5d6878),
        juce::Colour (0xff0a8f8a), juce::Colour (0xff5b47d6), juce::Colour (0xffd23f63), juce::Colour (0xffa16207), juce::Colour (0xff1f6fd1),
        juce::Colour (0xffffffff)
    };

    inline Look& currentLook()
    {
        static Look look = Look::dark;
        return look;
    }

    inline const Palette& get()
    {
        return currentLook() == Look::daylight ? daylightPalette : darkPalette;
    }

    inline juce::Colour moduleColour (Module module)
    {
        const auto& p = get();
        switch (module)
        {
            case Module::sound: return p.sound;
            case Module::tone:  return p.tone;
            case Module::shape: return p.shape;
            case Module::motion: return p.motion;
            case Module::effects: return p.effects;
            default:            return p.ink;
        }
    }

    inline Module moduleOf (const juce::Component& c)
    {
        return (Module) (int) c.getProperties().getWithDefault ("module", 0);
    }

    inline void setModule (juce::Component& c, Module module)
    {
        c.getProperties().set ("module", (int) module);
    }

    // Atkinson Hyperlegible, embedded in the plugin (SIL Open Font License, see Resources/Fonts/OFL.txt).
    // Shared between all open LAPiZ windows and released when the last one closes.
    struct Fonts
    {
        juce::Typeface::Ptr regular = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AtkinsonHyperlegibleRegular_ttf, (size_t) BinaryData::AtkinsonHyperlegibleRegular_ttfSize);
        juce::Typeface::Ptr bold = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AtkinsonHyperlegibleBold_ttf, (size_t) BinaryData::AtkinsonHyperlegibleBold_ttfSize);
    };

    inline juce::Font regular (float size)
    {
        juce::SharedResourcePointer<Fonts> fonts;
        return juce::Font (juce::FontOptions().withTypeface (fonts->regular).withHeight (size));
    }

    inline juce::Font bold (float size)
    {
        juce::SharedResourcePointer<Fonts> fonts;
        return juce::Font (juce::FontOptions().withTypeface (fonts->bold).withHeight (size));
    }
}
