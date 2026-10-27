#include "AdvancedPage.h"

namespace
{
    constexpr int knobRowHeight = 112;
    constexpr int buttonRowHeight = 30;
    constexpr int rowLabelWidth = 64;
    constexpr int panelGap = 16;
    constexpr int pagePadding = 16;

    juce::String lfoRateText (double hz)
    {
        return hz < 10.0 ? juce::String (hz, 2) + " Hz" : juce::String (hz, 1) + " Hz";
    }

    juce::String divisionText (double index)
    {
        const int i = juce::jlimit (0, (int) Lfo::divisions.size() - 1, juce::roundToInt (index));
        return Lfo::divisions[(size_t) i].name;
    }
}

AdvancedPage::AdvancedPage (LapizAudioProcessor& processor, HelpRegistrar registerHelp)
    : filterEnvView (processor.getState(), "fenv", Theme::Module::tone, "fenvAmount"),
      fenvAttack (processor.getState(), "fenvAttack", "Rise", "Attack", Theme::Module::tone, Format::seconds),
      fenvDecay (processor.getState(), "fenvDecay", "Drop", "Decay", Theme::Module::tone, Format::seconds),
      fenvSustain (processor.getState(), "fenvSustain", "Hold Level", "Sustain", Theme::Module::tone, Format::percent),
      fenvRelease (processor.getState(), "fenvRelease", "Return", "Release", Theme::Module::tone, Format::seconds),
      fenvAmount (processor.getState(), "fenvAmount", "Sweep", "Env Amount", Theme::Module::tone, Format::signedPercent),
      lfoView (processor.getState(), processor.getLive().lfoCycles),
      rate (processor.getState(), "lfoRate", "Speed", "LFO Rate", Theme::Module::motion, lfoRateText),
      division (processor.getState(), "lfoDivision", "Speed", "LFO Sync Rate", Theme::Module::motion, divisionText),
      amount (processor.getState(), "lfoAmount", "Wobble", "LFO Amount", Theme::Module::motion, Format::percent)
{
    auto& state = processor.getState();

    for (juce::Component* c : { (juce::Component*) &filterEnvView, (juce::Component*) &fenvAttack, (juce::Component*) &fenvDecay,
                                (juce::Component*) &fenvSustain, (juce::Component*) &fenvRelease, (juce::Component*) &fenvAmount,
                                (juce::Component*) &lfoView, (juce::Component*) &rate, (juce::Component*) &division,
                                (juce::Component*) &amount })
        addAndMakeVisible (c);

    auto helpForKnob = [&] (LabeledKnob& knob, const juce::String& text)
    {
        registerHelp (knob, text);
        registerHelp (knob.getSlider(), text);
    };

    // ---- Filter envelope ----
    registerHelp (filterEnvView, "This shows how the Brightness moves on every note: up when Sweep is above zero, "
                                 "down (drawn upside down) when it is below zero.");
    helpForKnob (fenvAttack, "Rise (Attack): how long the filter takes to sweep to its furthest point after you press a key. "
                             "Short gives a snappy pluck, long a slow opening. Double-click to reset.");
    helpForKnob (fenvDecay, "Drop (Decay): how long it takes to come back from the furthest point to the Hold Level. Double-click to reset.");
    helpForKnob (fenvSustain, "Hold Level (Sustain): how far along the sweep the filter stays while you hold the key. "
                              "0 % returns all the way to the Brightness knob. Double-click to reset.");
    helpForKnob (fenvRelease, "Return (Release): how long the filter takes to go back to the Brightness knob after you let go. Double-click to reset.");
    helpForKnob (fenvAmount, "Sweep (Filter Envelope Amount): how far each note moves the Brightness. Turn right to open it "
                             "(brighter), left to close it (darker). In the middle it does nothing. Double-click to reset.");

    // ---- LFO shape ----
    const char* shapeHelp[6] = {
        "Sine: a smooth, even up-and-down. Good for gentle vibrato and slow sweeps.",
        "Triangle: up and down in straight lines. Similar to sine, slightly sharper at the turns.",
        "Saw Down: jumps up, then falls slowly. Great for rhythmic 'wub' filter movements.",
        "Saw Up: rises slowly, then drops suddenly. Like a swell that keeps restarting.",
        "Square: flips between two settings. Good for trills and choppy on/off effects.",
        "Random: jumps to a new random setting each cycle (also called sample and hold). Good for bleepy, computer-like sounds."
    };

    shapeAttachment = std::make_unique<juce::ParameterAttachment> (
        *state.getParameter ("lfoShape"),
        [this] (float v) { for (int i = 0; i < 6; ++i) shapeButtons[i].setToggleState (i == (int) v, juce::dontSendNotification); });

    for (int i = 0; i < 6; ++i)
    {
        shapeButtons[i].setClickingTogglesState (true);
        shapeButtons[i].setRadioGroupId (2001);
        shapeButtons[i].onClick = [this, i] { shapeAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (shapeButtons[i]);
        registerHelp (shapeButtons[i], shapeHelp[i]);
    }
    shapeAttachment->sendInitialUpdate();

    // ---- LFO target ----
    const char* targetHelp[4] = {
        "Moves Brightness (the filter cutoff). The classic wobble bass and filter sweep.",
        "Moves Wave Shape (the wavetable position), so the tone itself morphs back and forth.",
        "Moves Volume up and down (tremolo). The LFO only turns it down from the normal level, never louder.",
        "Moves Pitch up and down. A little gives vibrato; a lot (up to 2 octaves each way) gives sirens and dive sweeps."
    };

    targetAttachment = std::make_unique<juce::ParameterAttachment> (
        *state.getParameter ("lfoTarget"),
        [this] (float v) { for (int i = 0; i < 4; ++i) targetButtons[i].setToggleState (i == (int) v, juce::dontSendNotification); });

    for (int i = 0; i < 4; ++i)
    {
        auto& button = targetButtons[i];
        button.setButtonText (Lfo::targetNames[(size_t) i]);
        button.setClickingTogglesState (true);
        button.setRadioGroupId (2002);
        Theme::setModule (button, Theme::Module::motion);
        button.onClick = [this, i] { targetAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (button);
        registerHelp (button, targetHelp[i]);
    }
    targetAttachment->sendInitialUpdate();

    // ---- LFO timing ----
    syncAttachment = std::make_unique<juce::ParameterAttachment> (
        *state.getParameter ("lfoSync"),
        [this] (float v)
        {
            tempoButton.setToggleState (v > 0.5f, juce::dontSendNotification);
            freeButton.setToggleState (v <= 0.5f, juce::dontSendNotification);
            showSpeedKnob (v > 0.5f);
        });

    for (auto* button : { &freeButton, &tempoButton })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (2003);
        Theme::setModule (*button, Theme::Module::motion);
        addAndMakeVisible (button);
    }
    freeButton.onClick  = [this] { syncAttachment->setValueAsCompleteGesture (0.0f); };
    tempoButton.onClick = [this] { syncAttachment->setValueAsCompleteGesture (1.0f); };
    syncAttachment->sendInitialUpdate();

    registerHelp (freeButton, "Free: Speed is set in cycles per second (Hz) and ignores the song's tempo.");
    registerHelp (tempoButton, "Tempo: Speed is locked to FL Studio's tempo, in note lengths (1/4 = once per beat). "
                               "While the song plays, the wobble lines up with the beat. D = dotted, T = triplet.");

    restartButton.setClickingTogglesState (true);
    Theme::setModule (restartButton, Theme::Module::motion);
    addAndMakeVisible (restartButton);
    restartAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, "lfoRetrigger", restartButton);
    registerHelp (restartButton, "Restart each note: when on, the LFO starts from the beginning of its shape every time you press a key, "
                                 "so every note moves the same way. When off, it keeps running and notes join in wherever it is.");

    registerHelp (lfoView, "The LFO's shape. The dot shows where it is right now. The faint line is the full shape; "
                           "the bright one is how much of it you hear, set by Wobble.");
    helpForKnob (rate, "Speed (LFO Rate): how many times per second the movement repeats. Double-click to reset.");
    helpForKnob (division, "Speed (LFO Sync Rate): how long one movement lasts, in note lengths. 1/1 is a whole bar, "
                           "1/4 is one beat. D = dotted (1.5x longer), T = triplet (shorter). Double-click to reset.");
    helpForKnob (amount, "Wobble (LFO Amount): how strongly the LFO moves the control picked under 'Moves'. "
                         "0 % turns it off. This is the same knob as Wobble on the Simple page. Double-click to reset.");
}

void AdvancedPage::showSpeedKnob (bool tempoSync)
{
    rate.setVisible (! tempoSync);
    division.setVisible (tempoSync);
}

void AdvancedPage::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();

    Panel::draw (g, envPanel, Theme::Module::tone, {}, "Brightness over time", "Filter envelope: sweeps the Brightness on every note");
    Panel::draw (g, lfoPanel, Theme::Module::motion, {}, "Wobble", "LFO: a repeating movement for one control");

    g.setColour (p.soft);
    g.setFont (Theme::regular (13.0f));
    g.drawText ("Shape", shapeLabel, juce::Justification::centredLeft);
    g.drawText ("Moves", targetLabel, juce::Justification::centredLeft);
    g.drawText ("Timing", timingLabel, juce::Justification::centredLeft);
}

void AdvancedPage::resized()
{
    auto area = getLocalBounds().reduced (pagePadding, 8);
    const int panelWidth = (area.getWidth() - panelGap) / 2;

    auto left = area.removeFromLeft (panelWidth);
    area.removeFromLeft (panelGap);
    auto right = area;
    envPanel = left.toFloat();
    lfoPanel = right.toFloat();

    auto knobRow = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> knobs)
    {
        const int slot = row.getWidth() / (int) knobs.size();
        for (auto* knob : knobs)
            knob->setBounds (row.removeFromLeft (slot));
    };

    // Brightness over time
    {
        auto inner = left.reduced (14, 0);
        inner.removeFromTop (Panel::titleHeight);
        inner.removeFromBottom (10);
        knobRow (inner.removeFromBottom (knobRowHeight), { &fenvAttack, &fenvDecay, &fenvSustain, &fenvRelease, &fenvAmount });
        inner.removeFromBottom (10);
        filterEnvView.setBounds (inner);
    }

    // Wobble
    {
        auto inner = right.reduced (14, 0);
        inner.removeFromTop (Panel::titleHeight);
        inner.removeFromBottom (10);

        auto knobs = inner.removeFromBottom (knobRowHeight);
        auto speedSlot = knobs.removeFromLeft (knobs.getWidth() / 2);
        rate.setBounds (speedSlot);
        division.setBounds (speedSlot);
        amount.setBounds (knobs);

        // Rows of buttons, bottom to top: Timing, Moves, Shape
        auto buttonRow = [&inner] (juce::Rectangle<int>& label)
        {
            inner.removeFromBottom (10);
            auto row = inner.removeFromBottom (buttonRowHeight);
            label = row.removeFromLeft (rowLabelWidth);
            return row;
        };

        auto timingRow = buttonRow (timingLabel);
        freeButton.setBounds (timingRow.removeFromLeft (70));
        timingRow.removeFromLeft (6);
        tempoButton.setBounds (timingRow.removeFromLeft (70));
        timingRow.removeFromLeft (12);
        restartButton.setBounds (timingRow);

        auto targetRow = buttonRow (targetLabel);
        const int targetWidth = (targetRow.getWidth() - 3 * 6) / 4;
        for (auto& button : targetButtons)
        {
            button.setBounds (targetRow.removeFromLeft (targetWidth));
            targetRow.removeFromLeft (6);
        }

        auto shapeRow = buttonRow (shapeLabel);
        const int shapeWidth = (shapeRow.getWidth() - 5 * 6) / 6;
        for (auto& button : shapeButtons)
        {
            button.setBounds (shapeRow.removeFromLeft (shapeWidth));
            shapeRow.removeFromLeft (6);
        }

        inner.removeFromBottom (10);
        lfoView.setBounds (inner);
    }
}
