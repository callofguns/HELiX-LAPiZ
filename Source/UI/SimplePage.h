#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "EnvelopeView.h"
#include "FilterView.h"
#include "LabeledKnob.h"
#include "Panel.h"
#include "Theme.h"
#include "WaveformView.h"
#include "WavetablePicker.h"

// The front page: everything a beginner needs, in three panels read left to right.
//   1. SOUND - which wave you start from, how much it wobbles (LFO), and how loud
//   2. TONE  - the filter (how bright or dark it is)
//   3. SHAPE - the volume envelope (how a note starts and ends)
class SimplePage : public juce::Component
{
public:
    // registerHelp tells the window which text to show in the help bar when the mouse is over a control.
    using HelpRegistrar = std::function<void (juce::Component&, const juce::String&)>;

    SimplePage (HelixAudioProcessor& processor, HelpRegistrar registerHelp);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void updateTypeButtons (int selectedIndex);

    // Panel 1
    WavetablePicker picker;
    WaveformView waveView;
    LabeledKnob position, wobble, volume;
    std::unique_ptr<juce::ParameterAttachment> lfoTargetAttachment;

    // Panel 2
    FilterView filterView;
    juce::TextButton typeButtons[3];
    LabeledKnob cutoff, resonance;
    std::unique_ptr<juce::ParameterAttachment> typeAttachment;

    // Panel 3
    EnvelopeView envelopeView;
    LabeledKnob attack, decay, sustain, release;

    juce::Rectangle<float> panelRects[3];
};
