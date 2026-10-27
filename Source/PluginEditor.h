#pragma once

#include <map>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/AdvancedPage.h"
#include "UI/LapizLookAndFeel.h"
#include "UI/SimplePage.h"
#include "UI/WavesPage.h"

// The plugin window: header with tabs and the settings button, the page below it,
// and a help bar along the bottom.
class LapizAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LapizAudioProcessorEditor (LapizAudioProcessor&);
    ~LapizAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void registerHelp (juce::Component& component, const juce::String& text);
    enum class Page { simple, waves, advanced };
    void showPage (Page page);
    void setHelpText (const juce::String& text);
    void showSettingsMenu();
    void setLook (Theme::Look look);

    static constexpr int headerHeight = 56;
    static constexpr int footerHeight = 34;

    LapizAudioProcessor& processor;

    // Declared first so they outlive every component that uses them.
    juce::SharedResourcePointer<Theme::Fonts> fonts;
    LapizLookAndFeel lookAndFeel;

    // These must come before the pages: they register their help texts while being built.
    std::map<juce::Component*, juce::String> helpTexts;
    juce::Component* helpOwner = nullptr;
    juce::String helpText;

    juce::TextButton simpleTab { "Simple" }, wavesTab { "Waves" }, advancedTab { "Advanced" };
    SettingsButton settingsButton;
    SimplePage simplePage;
    WavesPage wavesPage;
    AdvancedPage advancedPage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LapizAudioProcessorEditor)
};
