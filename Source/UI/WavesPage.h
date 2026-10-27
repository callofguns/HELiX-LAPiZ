#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../PluginProcessor.h"
#include "KeyframeList.h"
#include "LabeledKnob.h"
#include "Panel.h"
#include "Theme.h"
#include "WaveCanvas.h"
#include "WavetablePicker.h"

// The Waves tab: pick a wavetable, or draw your own.
// You draw a few keyframes; Wave Shape blends smoothly from the first to the last.
// The drawing is played as you draw, saved with your project, and can be saved as a .wav file.
class WavesPage : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    using HelpRegistrar = std::function<void (juce::Component&, const juce::String&)>;

    WavesPage (HelixAudioProcessor& processor, HelpRegistrar registerHelp);
    ~WavesPage() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    struct Snapshot
    {
        std::vector<Wavetable::Frame> keyframes;
        int selected = 0;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void syncFromProcessor();
    void refreshView();
    void showReadOnlyFrame();
    void pushToProcessor();

    void saveUndoStep();
    void undo();
    void redo();

    void newDrawing();
    void editCopy();
    void saveAsFile();
    void addKeyframe();
    void removeKeyframe();
    void fillWith (int shape);
    void selectKeyframe (int index);

    bool isDrawing() const { return processor.getTableChoice().kind == TableChoice::Kind::drawn; }

    HelixAudioProcessor& processor;
    const juce::uint32 tag;

    // The drawing being edited. The processor plays a copy, updated as you draw.
    std::vector<Wavetable::Frame> keyframes;
    int selected = 0;
    juce::String drawingName { "My drawing" };
    bool needsPush = false;

    std::vector<Snapshot> undoSteps, redoSteps;

    std::shared_ptr<const Wavetable> shownTable; // keeps the table alive while the canvas shows it
    int shownFrame = -1;

    WavetablePicker picker;
    juce::TextButton newButton { "New drawing" }, copyButton { "Edit a copy" }, saveButton { "Save as file..." };

    WaveCanvas canvas;
    juce::TextButton drawTool { "Draw" }, lineTool { "Line" };
    juce::TextButton gridButtons[4] { juce::TextButton ("Off"), juce::TextButton ("8"), juce::TextButton ("16"), juce::TextButton ("32") };
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" };
    juce::TextButton startButtons[4] { juce::TextButton ("Sine"), juce::TextButton ("Triangle"),
                                       juce::TextButton ("Saw"), juce::TextButton ("Square") };

    KeyframeList keyframeList;
    juce::TextButton addButton { "Add" }, removeButton { "Remove" };
    LabeledKnob position;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<float> drawPanel, keyPanel;
    juce::Rectangle<int> gridLabel, startLabel;
};
