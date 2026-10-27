#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

// How knob values are shown under each knob.
namespace Format
{
    inline juce::String percent (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; }

    // For knobs that go both ways: "+40 %" opens, "-40 %" closes.
    inline juce::String signedPercent (double v)
    {
        const int rounded = juce::roundToInt (v * 100.0);
        return (rounded > 0 ? "+" : "") + juce::String (rounded) + " %";
    }

    inline juce::String seconds (double v)
    {
        return v < 1.0 ? juce::String (juce::roundToInt (v * 1000.0)) + " ms"
                       : juce::String (v, 2) + " s";
    }

    inline juce::String hertz (double v)
    {
        return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz"
                           : juce::String (juce::roundToInt (v)) + " Hz";
    }
}

// A knob with a plain-English name on top and the real synth term in small text under it,
// so beginners can read it and slowly learn the proper names. The value is shown underneath.
class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                 const juce::String& title, const juce::String& technicalName,
                 Theme::Module module, std::function<juce::String (double)> valueToText)
        : titleText (title), technicalText (technicalName), formatValue (std::move (valueToText))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        Theme::setModule (slider, module);
        slider.onValueChange = [this] { repaint(); };

        addAndMakeVisible (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);
    }

    juce::Slider& getSlider() { return slider; }

    void setTechnicalText (const juce::String& text)
    {
        technicalText = text;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& p = Theme::get();
        auto area = getLocalBounds();

        g.setColour (p.ink);
        g.setFont (Theme::bold (14.0f));
        g.drawText (titleText, area.removeFromTop (18), juce::Justification::centred);

        g.setColour (p.soft);
        g.setFont (Theme::regular (11.0f));
        g.drawText (technicalText, area.removeFromTop (14), juce::Justification::centred);

        g.setColour (p.ink);
        g.setFont (Theme::regular (13.0f));
        g.drawText (formatValue (slider.getValue()), area.removeFromBottom (18), juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (32);
        area.removeFromBottom (18);
        slider.setBounds (area);
    }

private:
    juce::String titleText, technicalText;
    std::function<juce::String (double)> formatValue;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
