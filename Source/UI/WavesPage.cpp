#include "WavesPage.h"
#include "../WavetableFile.h"

namespace
{
    constexpr int pagePadding = 16;
    constexpr int panelGap = 16;
    constexpr int rowHeight = 30;
    constexpr int keyPanelWidth = 270;
    constexpr int maxUndoSteps = 100;

    // A keyframe filled with one of the four basic shapes (0 sine, 1 triangle, 2 saw, 3 square).
    Wavetable::Frame basicShape (int shape)
    {
        Wavetable::Frame frame ((size_t) Wavetable::tableSize);
        for (int i = 0; i < Wavetable::tableSize; ++i)
        {
            const float p = (float) i / (float) Wavetable::tableSize;
            switch (shape)
            {
                case 1:  frame[(size_t) i] = p < 0.25f ? 4.0f * p : p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f; break;
                case 2:  frame[(size_t) i] = 2.0f * p - 1.0f; break;
                case 3:  frame[(size_t) i] = p < 0.5f ? 1.0f : -1.0f; break;
                default: frame[(size_t) i] = std::sin (juce::MathConstants<float>::twoPi * p); break;
            }
        }
        return frame;
    }
}

WavesPage::WavesPage (HelixAudioProcessor& p, HelpRegistrar registerHelp)
    : processor (p),
      tag ((juce::uint32) juce::Random::getSystemRandom().nextInt() | 1u),
      picker (p),
      position (p.getState(), "position", "Wave Shape", "Wavetable Position", Theme::Module::sound, Format::percent)
{
    setWantsKeyboardFocus (true);

    for (juce::Component* c : { (juce::Component*) &picker, (juce::Component*) &newButton, (juce::Component*) &copyButton,
                                (juce::Component*) &saveButton, (juce::Component*) &canvas, (juce::Component*) &drawTool,
                                (juce::Component*) &lineTool, (juce::Component*) &undoButton, (juce::Component*) &redoButton,
                                (juce::Component*) &keyframeList, (juce::Component*) &addButton, (juce::Component*) &removeButton,
                                (juce::Component*) &position })
        addAndMakeVisible (c);

    // Top row
    newButton.onClick  = [this] { newDrawing(); };
    copyButton.onClick = [this] { editCopy(); };
    saveButton.onClick = [this] { saveAsFile(); };

    // Drawing tools
    for (auto* b : { &drawTool, &lineTool })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (3001);
        Theme::setModule (*b, Theme::Module::sound);
    }
    drawTool.setToggleState (true, juce::dontSendNotification);
    drawTool.onClick = [this] { canvas.setTool (WaveCanvas::Tool::draw); };
    lineTool.onClick = [this] { canvas.setTool (WaveCanvas::Tool::line); };

    const int gridSizes[4] = { 0, 8, 16, 32 };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = gridButtons[i];
        b.setClickingTogglesState (true);
        b.setRadioGroupId (3002);
        Theme::setModule (b, Theme::Module::sound);
        const int cells = gridSizes[i];
        b.onClick = [this, cells] { canvas.setGrid (cells); };
        addAndMakeVisible (b);
        registerHelp (b, cells == 0 ? juce::String ("Grid off: draw freely.")
                                    : "Grid " + juce::String (cells) + ": snaps your drawing to " + juce::String (cells)
                                      + " flat steps across and " + juce::String (cells) + " levels up and down. Good for chiptune-style waves.");
    }
    gridButtons[0].setToggleState (true, juce::dontSendNotification);

    undoButton.onClick = [this] { undo(); };
    redoButton.onClick = [this] { redo(); };

    for (int i = 0; i < 4; ++i)
    {
        startButtons[i].onClick = [this, i] { fillWith (i); };
        addAndMakeVisible (startButtons[i]);
        registerHelp (startButtons[i], "Replaces the selected keyframe with a " + startButtons[i].getButtonText().toLowerCase()
                                       + " wave, as a starting point to draw on.");
    }

    canvas.onStrokeStart = [this] { saveUndoStep(); grabKeyboardFocus(); };
    canvas.onChange      = [this] { needsPush = true; keyframeList.repaint(); };
    canvas.onStrokeEnd   = [this] { pushToProcessor(); };

    // Keyframes
    keyframeList.onSelect = [this] (int i) { selectKeyframe (i); };
    addButton.onClick     = [this] { addKeyframe(); };
    removeButton.onClick  = [this] { removeKeyframe(); };

    // Hover help
    registerHelp (picker.getNameButton(), "Wavetable: the set of waves Helix plays. Click for the full list.");
    registerHelp (picker.getPreviousButton(), "Previous wavetable.");
    registerHelp (picker.getNextButton(), "Next wavetable.");
    registerHelp (newButton, "New drawing: start your own wavetable with two keyframes (a sine and a saw) to draw on.");
    registerHelp (copyButton, "Edit a copy: turns the current wavetable into keyframes you can draw on. "
                              "Tables with more than 8 waves are reduced to 8 evenly spaced ones. The original isn't changed.");
    registerHelp (saveButton, "Save as file: saves your drawing as a .wav wavetable in Documents\\Helix\\Wavetables. "
                              "It then appears in the wavetable list in every Helix, and other synths like Serum can load it too.");
    registerHelp (canvas, "Draw on the selected keyframe with the mouse. You hear the change straight away.");
    registerHelp (drawTool, "Draw: drag to paint the wave freehand.");
    registerHelp (lineTool, "Line: press, drag and let go to draw a straight line. Good for clean, sharp waves.");
    registerHelp (undoButton, "Undo the last change (Ctrl+Z).");
    registerHelp (redoButton, "Redo what you undid (Ctrl+Y).");
    registerHelp (keyframeList, "Your keyframes. Click one to draw on it. Wave Shape blends smoothly from keyframe 1 to the last one.");
    registerHelp (addButton, "Add: copies the selected keyframe and puts the copy right after it (up to 8).");
    registerHelp (removeButton, "Remove: deletes the selected keyframe (at least one is always kept).");
    registerHelp (position, "Wave Shape (Wavetable Position): blends through your keyframes. Turn it while holding a note to hear the morph. Double-click to reset.");
    registerHelp (position.getSlider(), "Wave Shape (Wavetable Position): blends through your keyframes. Turn it while holding a note to hear the morph. Double-click to reset.");

    processor.addChangeListener (this);
    syncFromProcessor();
    startTimerHz (30);
}

WavesPage::~WavesPage()
{
    processor.removeChangeListener (this);
    if (needsPush)
        pushToProcessor();
}

//==============================================================================

void WavesPage::changeListenerCallback (juce::ChangeBroadcaster*)
{
    syncFromProcessor();
}

// Takes a drawing from the processor if it didn't come from this page
// (for example, when a project is opened or this window has just been opened).
void WavesPage::syncFromProcessor()
{
    const auto choice = processor.getTableChoice();
    if (choice.kind == TableChoice::Kind::drawn && choice.editorTag != tag)
    {
        keyframes = choice.keyframes;
        drawingName = choice.name;
        selected = juce::jlimit (0, (int) keyframes.size() - 1, selected);
        undoSteps.clear();
        redoSteps.clear();
    }
    refreshView();
}

void WavesPage::refreshView()
{
    const bool drawing = isDrawing() && ! keyframes.empty();

    if (drawing)
    {
        shownTable.reset();
        canvas.setFrame (&keyframes[(size_t) selected]);
        keyframeList.setKeyframes (&keyframes, selected);
    }
    else
    {
        shownTable = processor.getTable();
        shownFrame = -1;
        showReadOnlyFrame();
        keyframeList.setKeyframes (nullptr, 0);
    }

    for (juce::Component* c : { (juce::Component*) &drawTool, (juce::Component*) &lineTool, (juce::Component*) &saveButton })
        c->setEnabled (drawing);
    for (auto& b : gridButtons)  b.setEnabled (drawing);
    for (auto& b : startButtons) b.setEnabled (drawing);

    copyButton.setEnabled (! drawing);
    addButton.setEnabled (drawing && (int) keyframes.size() < KeyframeList::maxKeyframes);
    removeButton.setEnabled (drawing && keyframes.size() > 1);
    undoButton.setEnabled (drawing && ! undoSteps.empty());
    redoButton.setEnabled (drawing && ! redoSteps.empty());
}

void WavesPage::timerCallback()
{
    // While drawing, the sound is updated up to 30 times a second.
    if (needsPush)
        pushToProcessor();

    // When showing a table you can't draw on, follow the Wave Shape knob.
    if (shownTable != nullptr)
        showReadOnlyFrame();
}

// Shows the wave of the current (built-in or file) table nearest to the Wave Shape knob.
void WavesPage::showReadOnlyFrame()
{
    const float pos = processor.getState().getRawParameterValue ("position")->load();
    const int frame = juce::roundToInt (pos * (float) (shownTable->getNumFrames() - 1));
    if (frame == shownFrame)
        return;

    shownFrame = frame;
    canvas.setReadOnly (shownTable->getFrame (frame),
                        "This is " + processor.getTableChoice().name + ". Click 'Edit a copy' to draw on it, "
                        "or 'New drawing' to start from scratch.");
}

void WavesPage::pushToProcessor()
{
    needsPush = false;
    if (! keyframes.empty())
        processor.setDrawnTable (keyframes, drawingName, tag);
}

//==============================================================================
// Undo

void WavesPage::saveUndoStep()
{
    undoSteps.push_back ({ keyframes, selected });
    if ((int) undoSteps.size() > maxUndoSteps)
        undoSteps.erase (undoSteps.begin());
    redoSteps.clear();
    undoButton.setEnabled (true);
    redoButton.setEnabled (false);
}

void WavesPage::undo()
{
    if (undoSteps.empty() || ! isDrawing())
        return;

    redoSteps.push_back ({ keyframes, selected });
    keyframes = std::move (undoSteps.back().keyframes);
    selected = undoSteps.back().selected;
    undoSteps.pop_back();
    pushToProcessor();
    refreshView();
}

void WavesPage::redo()
{
    if (redoSteps.empty() || ! isDrawing())
        return;

    undoSteps.push_back ({ keyframes, selected });
    keyframes = std::move (redoSteps.back().keyframes);
    selected = redoSteps.back().selected;
    redoSteps.pop_back();
    pushToProcessor();
    refreshView();
}

bool WavesPage::keyPressed (const juce::KeyPress& key)
{
    const auto ctrl = juce::ModifierKeys::ctrlModifier;
    if (key == juce::KeyPress ('z', ctrl, 0))
    {
        undo();
        return true;
    }
    if (key == juce::KeyPress ('y', ctrl, 0) || key == juce::KeyPress ('z', ctrl | juce::ModifierKeys::shiftModifier, 0))
    {
        redo();
        return true;
    }
    return false;
}

//==============================================================================
// Actions

void WavesPage::newDrawing()
{
    if (isDrawing())
    {
        saveUndoStep();
    }
    else
    {
        undoSteps.clear();
        redoSteps.clear();
    }

    keyframes = { basicShape (0), basicShape (2) };
    selected = 0;
    drawingName = "My drawing";
    pushToProcessor();
    refreshView();
}

void WavesPage::editCopy()
{
    const auto table = processor.getTable();
    const int count = table->getNumFrames();
    const int keep = juce::jmin (count, KeyframeList::maxKeyframes);

    keyframes.clear();
    for (int k = 0; k < keep; ++k)
    {
        const int frame = keep > 1 ? juce::roundToInt ((float) k * (float) (count - 1) / (float) (keep - 1)) : 0;
        const float* data = table->getFrame (frame);
        keyframes.emplace_back (data, data + Wavetable::tableSize);
    }

    selected = 0;
    drawingName = processor.getTableChoice().name + " copy";
    undoSteps.clear();
    redoSteps.clear();
    pushToProcessor();
    refreshView();
}

void WavesPage::saveAsFile()
{
    if (keyframes.empty())
        return;

    const auto folder = WavetableFile::folder();
    folder.createDirectory();

    chooser = std::make_unique<juce::FileChooser> ("Save wavetable", folder.getChildFile (drawingName + ".wav"), "*.wav");

    juce::Component::SafePointer<WavesPage> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safeThis] (const juce::FileChooser& fc)
                          {
                              if (safeThis == nullptr || fc.getResult() == juce::File())
                                  return;

                              const auto file = fc.getResult().withFileExtension ("wav");
                              if (WavetableFile::save (file, safeThis->keyframes))
                              {
                                  safeThis->drawingName = file.getFileNameWithoutExtension();
                                  safeThis->processor.renameDrawnTable (safeThis->drawingName);
                              }
                              else
                              {
                                  juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't save",
                                      "Helix couldn't write " + file.getFullPathName()
                                      + ". Check that the folder exists and isn't read-only, then try again.");
                              }
                          });
}

void WavesPage::addKeyframe()
{
    if ((int) keyframes.size() >= KeyframeList::maxKeyframes)
        return;

    saveUndoStep();
    keyframes.insert (keyframes.begin() + selected + 1, keyframes[(size_t) selected]);
    ++selected;
    pushToProcessor();
    refreshView();
}

void WavesPage::removeKeyframe()
{
    if (keyframes.size() <= 1)
        return;

    saveUndoStep();
    keyframes.erase (keyframes.begin() + selected);
    selected = juce::jmin (selected, (int) keyframes.size() - 1);
    pushToProcessor();
    refreshView();
}

void WavesPage::fillWith (int shape)
{
    saveUndoStep();
    keyframes[(size_t) selected] = basicShape (shape);
    pushToProcessor();
    refreshView();
}

void WavesPage::selectKeyframe (int index)
{
    selected = juce::jlimit (0, (int) keyframes.size() - 1, index);
    refreshView();
}

//==============================================================================

void WavesPage::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();

    Panel::draw (g, drawPanel, Theme::Module::sound, {}, "Draw", "Draw the selected keyframe with the mouse");
    Panel::draw (g, keyPanel, Theme::Module::sound, {}, "Keyframes", "Wave Shape blends from the first to the last");

    g.setColour (p.soft);
    g.setFont (Theme::regular (13.0f));
    g.drawText ("Grid", gridLabel, juce::Justification::centredLeft);
    g.drawText ("Start from", startLabel, juce::Justification::centredLeft);
}

void WavesPage::resized()
{
    auto area = getLocalBounds().reduced (pagePadding, 8);

    // Top row: picker on the left, actions on the right
    auto top = area.removeFromTop (32);
    picker.setBounds (top.removeFromLeft (320));
    saveButton.setBounds (top.removeFromRight (130));
    top.removeFromRight (8);
    copyButton.setBounds (top.removeFromRight (110));
    top.removeFromRight (8);
    newButton.setBounds (top.removeFromRight (110));
    area.removeFromTop (10);

    auto right = area.removeFromRight (keyPanelWidth);
    area.removeFromRight (panelGap);
    drawPanel = area.toFloat();
    keyPanel = right.toFloat();

    // Draw panel
    {
        auto inner = area.reduced (14, 0);
        inner.removeFromTop (Panel::titleHeight);
        inner.removeFromBottom (10);

        auto tools = inner.removeFromTop (rowHeight);
        drawTool.setBounds (tools.removeFromLeft (64));
        tools.removeFromLeft (6);
        lineTool.setBounds (tools.removeFromLeft (64));
        tools.removeFromLeft (18);
        gridLabel = tools.removeFromLeft (38);
        for (auto& b : gridButtons)
        {
            b.setBounds (tools.removeFromLeft (44));
            tools.removeFromLeft (4);
        }
        redoButton.setBounds (tools.removeFromRight (64));
        tools.removeFromRight (6);
        undoButton.setBounds (tools.removeFromRight (64));

        auto start = inner.removeFromBottom (rowHeight);
        startLabel = start.removeFromLeft (80);
        const int startWidth = (start.getWidth() - 3 * 6) / 4;
        for (auto& b : startButtons)
        {
            b.setBounds (start.removeFromLeft (startWidth));
            start.removeFromLeft (6);
        }

        inner.removeFromTop (8);
        inner.removeFromBottom (8);
        canvas.setBounds (inner);
    }

    // Keyframes panel
    {
        auto inner = right.reduced (14, 0);
        inner.removeFromTop (Panel::titleHeight);
        inner.removeFromBottom (10);

        position.setBounds (inner.removeFromBottom (112).withSizeKeepingCentre (110, 112));
        inner.removeFromBottom (8);

        auto buttons = inner.removeFromBottom (rowHeight);
        const int half = (buttons.getWidth() - 6) / 2;
        addButton.setBounds (buttons.removeFromLeft (half));
        buttons.removeFromLeft (6);
        removeButton.setBounds (buttons);
        inner.removeFromBottom (8);

        keyframeList.setBounds (inner);
    }
}
