#include "PluginEditor.h"
#include "UI/Settings.h"
#include "Version.h"

namespace
{
    const juce::String defaultHelp = "Hover over any knob or button to see what it does.";
}

LapizAudioProcessorEditor::LapizAudioProcessorEditor (LapizAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      helpText (defaultHelp),
      presetBar (p),
      simplePage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      wavesPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      advancedPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      effectsPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); })
{
    Theme::currentLook() = Settings::loadLook();
    lookAndFeel.refreshColours();
    setLookAndFeel (&lookAndFeel);

    for (auto* tab : { &simpleTab, &wavesTab, &advancedTab, &effectsTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (1001);
        addAndMakeVisible (tab);
    }
    simpleTab.onClick   = [this] { showPage (Page::simple); };
    wavesTab.onClick    = [this] { showPage (Page::waves); };
    advancedTab.onClick = [this] { showPage (Page::advanced); };
    effectsTab.onClick  = [this] { showPage (Page::effects); };

    addAndMakeVisible (presetBar);
    registerHelp (presetBar.getNameButton(), "Preset: a whole sound, ready to play. Click for the full list of built-in sounds and the ones you saved.");
    registerHelp (presetBar.getPreviousButton(), "Previous preset.");
    registerHelp (presetBar.getNextButton(), "Next preset.");
    registerHelp (presetBar.getSaveButton(), "Save: keeps the current sound as a preset in Documents\\LAPiZ\\Presets, so you can load it in any project.");

    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (settingsButton);
    registerHelp (settingsButton, "Settings: switch between the Dark and Daylight look. Your choice is remembered for every LAPiZ you open.");

    addChildComponent (simplePage);
    addChildComponent (wavesPage);
    addChildComponent (advancedPage);
    addChildComponent (effectsPage);

    const auto lastPage = Settings::loadPage();
    showPage (lastPage == "waves" ? Page::waves : lastPage == "advanced" ? Page::advanced
            : lastPage == "effects" ? Page::effects : Page::simple);

    setResizable (false, false);
    setSize (900, 580);
}

LapizAudioProcessorEditor::~LapizAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LapizAudioProcessorEditor::showSettingsMenu()
{
    const bool dark = Theme::currentLook() == Theme::Look::dark;

    juce::PopupMenu menu;
    menu.addSectionHeader ("Look");
    menu.addItem (1, "Dark", true, dark);
    menu.addItem (2, "Daylight", true, ! dark);

    juce::Component::SafePointer<LapizAudioProcessorEditor> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || result == 0)
                                return;
                            safeThis->setLook (result == 2 ? Theme::Look::daylight : Theme::Look::dark);
                        });
}

void LapizAudioProcessorEditor::setLook (Theme::Look look)
{
    Theme::currentLook() = look;
    Settings::saveLook (look);
    lookAndFeel.refreshColours();
    repaint();
}

void LapizAudioProcessorEditor::registerHelp (juce::Component& component, const juce::String& text)
{
    helpTexts[&component] = text;
    component.addMouseListener (this, false);
}

void LapizAudioProcessorEditor::showPage (Page page)
{
    simplePage.setVisible (page == Page::simple);
    wavesPage.setVisible (page == Page::waves);
    advancedPage.setVisible (page == Page::advanced);
    effectsPage.setVisible (page == Page::effects);

    simpleTab.setToggleState (page == Page::simple, juce::dontSendNotification);
    wavesTab.setToggleState (page == Page::waves, juce::dontSendNotification);
    advancedTab.setToggleState (page == Page::advanced, juce::dontSendNotification);
    effectsTab.setToggleState (page == Page::effects, juce::dontSendNotification);

    Settings::savePage (page == Page::waves ? "waves" : page == Page::advanced ? "advanced"
                      : page == Page::effects ? "effects" : "simple");
}

void LapizAudioProcessorEditor::setHelpText (const juce::String& text)
{
    if (helpText != text)
    {
        helpText = text;
        repaint (0, getHeight() - footerHeight, getWidth(), footerHeight);
    }
}

void LapizAudioProcessorEditor::mouseEnter (const juce::MouseEvent& e)
{
    auto found = helpTexts.find (e.eventComponent);
    if (found != helpTexts.end())
    {
        helpOwner = found->first;
        setHelpText (found->second);
    }
}

void LapizAudioProcessorEditor::mouseExit (const juce::MouseEvent& e)
{
    // Only clear the text if it belongs to the control the mouse just left.
    if (e.eventComponent == helpOwner)
    {
        helpOwner = nullptr;
        setHelpText (defaultHelp);
    }
}

void LapizAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();
    g.fillAll (p.ground);

    // Header
    auto header = getLocalBounds().removeFromTop (headerHeight).reduced (20, 0);
    g.setColour (p.ink);
    g.setFont (Theme::bold (26.0f));
    g.drawText (processor.getName(), header.removeFromLeft (110), juce::Justification::centredLeft);

    // Footer: help text on the left, version on the right
    auto footer = getLocalBounds().removeFromBottom (footerHeight).reduced (20, 0);

    g.setColour (p.soft);
    g.setFont (Theme::regular (12.0f));
    g.drawText (versionTag, footer.removeFromRight (90), juce::Justification::centredRight);

    g.setColour (p.ink);
    g.setFont (Theme::regular (13.5f));
    g.drawFittedText (helpText, footer, juce::Justification::centredLeft, 2);
}

void LapizAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (headerHeight).reduced (20, 12);
    settingsButton.setBounds (header.removeFromRight (36));
    header.removeFromRight (12);
    effectsTab.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    advancedTab.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    wavesTab.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    simpleTab.setBounds (header.removeFromRight (100));

    // The preset bar sits between the LAPiZ name and the tabs.
    header.removeFromLeft (110);
    header.removeFromRight (16);
    presetBar.setBounds (header);

    area.removeFromBottom (footerHeight);
    simplePage.setBounds (area);
    wavesPage.setBounds (area);
    advancedPage.setBounds (area);
    effectsPage.setBounds (area);
}
