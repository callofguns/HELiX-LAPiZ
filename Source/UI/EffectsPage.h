#pragma once

#include <array>
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "LabeledKnob.h"
#include "Panel.h"
#include "Theme.h"

// The Effects tab: four effects after the synth, left to right in the order the sound passes through them.
//   Grit (distortion) -> Chorus -> Echo (delay) -> Space (reverb)
// Each has an on/off switch; its knobs are dimmed while it is off.
class EffectsPage : public juce::Component
{
public:
    using HelpRegistrar = std::function<void (juce::Component&, const juce::String&)>;

    EffectsPage (LapizAudioProcessor& processor, HelpRegistrar registerHelp);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct Slot
    {
        const char* title;
        const char* subtitle;
        const char* blurb; // what it's good for, shown in the panel
        juce::TextButton onButton;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment;
        std::array<std::unique_ptr<LabeledKnob>, 3> knobs;
        juce::Rectangle<float> panel;
        juce::Rectangle<int> blurbArea;
    };

    void showOnState (Slot& slot);

    std::array<Slot, 4> slots;
};
