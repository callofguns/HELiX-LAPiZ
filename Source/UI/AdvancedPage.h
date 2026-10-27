#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "EnvelopeView.h"
#include "LabeledKnob.h"
#include "LfoView.h"
#include "Panel.h"
#include "Theme.h"

// The Advanced tab: the controls that make a sound move over time, and thicken it.
//   Brightness over time - the filter envelope, which opens or closes the filter on every note
//   Thickness            - unison: stacked, detuned copies of each note, spread left and right
//   Wobble               - the LFO, a repeating movement for one control you choose
class AdvancedPage : public juce::Component
{
public:
    using HelpRegistrar = std::function<void (juce::Component&, const juce::String&)>;

    AdvancedPage (LapizAudioProcessor& processor, HelpRegistrar registerHelp);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void showSpeedKnob (bool tempoSync);

    // Filter envelope
    EnvelopeView filterEnvView;
    LabeledKnob fenvAttack, fenvDecay, fenvSustain, fenvRelease, fenvAmount;

    // Unison
    LabeledKnob unisonVoices, unisonDetune, unisonWidth;

    // LFO
    LfoView lfoView;
    LfoShapeButton shapeButtons[6] { LfoShapeButton (0), LfoShapeButton (1), LfoShapeButton (2),
                                     LfoShapeButton (3), LfoShapeButton (4), LfoShapeButton (5) };
    juce::TextButton targetButtons[4];
    juce::TextButton freeButton { "Free" }, tempoButton { "Tempo" };
    juce::TextButton restartButton { "Restart each note" };
    LabeledKnob rate, division, amount;

    std::unique_ptr<juce::ParameterAttachment> shapeAttachment, targetAttachment, syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> restartAttachment;

    juce::Rectangle<float> envPanel, unisonPanel, lfoPanel;
    juce::Rectangle<int> shapeLabel, targetLabel, timingLabel;
};
