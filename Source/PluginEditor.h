#pragma once

#include <map>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Updater.h"
#include "UI/AdvancedPage.h"
#include "UI/EffectsPage.h"
#include "UI/LapizLookAndFeel.h"
#include "UI/PianoKeyboard.h"
#include "UI/PresetBar.h"
#include "UI/SimplePage.h"
#include "UI/WavesPage.h"

// Everything in the plugin window: header with tabs and the settings button, the page below it,
// a piano keyboard, and a help bar along the bottom. It's always laid out at its design size
// (900 x 644); the editor below scales it to whatever size the window is.
class LapizView : public juce::Component, private juce::ChangeListener
{
public:
    static constexpr int designWidth = 900, designHeight = 644;

    explicit LapizView (LapizAudioProcessor&);
    ~LapizView() override;

    // Called with a window scale (1.0 = 100 %) when one is chosen from the settings menu.
    std::function<void (float)> onScaleChosen;
    float currentScale = 1.0f;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void registerHelp (juce::Component& component, const juce::String& text);
    enum class Page { simple, waves, advanced, effects };
    void showPage (Page page);
    void setHelpText (const juce::String& text);
    void showSettingsMenu();
    void setLook (Theme::Look look);
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    juce::String updateText() const; // shown in the footer instead of the version while there's news

    static constexpr int headerHeight = 56;
    static constexpr int footerHeight = 34;
    static constexpr int keyboardHeight = 64;

    LapizAudioProcessor& processor;

    // Declared first so they outlive every component that uses them.
    juce::SharedResourcePointer<Theme::Fonts> fonts;
    LapizLookAndFeel lookAndFeel;
    juce::SharedResourcePointer<Updater> updater;

    // These must come before the pages: they register their help texts while being built.
    std::map<juce::Component*, juce::String> helpTexts;
    juce::Component* helpOwner = nullptr;
    juce::String helpText;

    juce::TextButton simpleTab { "Simple" }, wavesTab { "Waves" }, advancedTab { "Advanced" }, effectsTab { "Effects" };
    PresetBar presetBar;
    SettingsButton settingsButton;
    SimplePage simplePage;
    WavesPage wavesPage;
    AdvancedPage advancedPage;
    EffectsPage effectsPage;
    PianoKeyboard keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LapizView)
};

// The plugin window. You can resize it by dragging the bottom-right corner (or with the host's own
// window controls), from 75 % to 200 %, or pick a size in the settings menu. It keeps its shape,
// and the size is remembered for every LAPiZ you open.
class LapizAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr float minScale = 0.75f, maxScale = 2.0f;

    explicit LapizAudioProcessorEditor (LapizAudioProcessor&);
    ~LapizAudioProcessorEditor() override;

    void resized() override;
    void setScale (float scale);
    float getScale() const;

private:
    LapizView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LapizAudioProcessorEditor)
};
