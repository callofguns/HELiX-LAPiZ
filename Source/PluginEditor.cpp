#include "PluginEditor.h"
#include "UI/Settings.h"
#include "Version.h"

namespace
{
    const juce::String defaultHelp = "Hover over any knob or button to see what it does.";
}

HelixAudioProcessorEditor::HelixAudioProcessorEditor (HelixAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      helpText (defaultHelp),
      simplePage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      wavesPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      advancedPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); })
{
    Theme::currentLook() = Settings::loadLook();
    lookAndFeel.refreshColours();
    setLookAndFeel (&lookAndFeel);

    for (auto* tab : { &simpleTab, &wavesTab, &advancedTab })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (1001);
        addAndMakeVisible (tab);
    }
    simpleTab.onClick   = [this] { showPage (Page::simple); };
    wavesTab.onClick    = [this] { showPage (Page::waves); };
    advancedTab.onClick = [this] { showPage (Page::advanced); };

    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (settingsButton);
    registerHelp (settingsButton, "Settings: switch between the Dark and Daylight look. Your choice is remembered for every Helix you open.");

    addChildComponent (simplePage);
    addChildComponent (wavesPage);
    addChildComponent (advancedPage);

    const auto lastPage = Settings::loadPage();
    showPage (lastPage == "waves" ? Page::waves : lastPage == "advanced" ? Page::advanced : Page::simple);

    setResizable (false, false);
    setSize (900, 580);
}

HelixAudioProcessorEditor::~HelixAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HelixAudioProcessorEditor::showSettingsMenu()
{
    const bool dark = Theme::currentLook() == Theme::Look::dark;

    juce::PopupMenu menu;
    menu.addSectionHeader ("Look");
    menu.addItem (1, "Dark", true, dark);
    menu.addItem (2, "Daylight", true, ! dark);

    juce::Component::SafePointer<HelixAudioProcessorEditor> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || result == 0)
                                return;
                            safeThis->setLook (result == 2 ? Theme::Look::daylight : Theme::Look::dark);
                        });
}

void HelixAudioProcessorEditor::setLook (Theme::Look look)
{
    Theme::currentLook() = look;
    Settings::saveLook (look);
    lookAndFeel.refreshColours();
    repaint();
}

void HelixAudioProcessorEditor::registerHelp (juce::Component& component, const juce::String& text)
{
    helpTexts[&component] = text;
    component.addMouseListener (this, false);
}

void HelixAudioProcessorEditor::showPage (Page page)
{
    simplePage.setVisible (page == Page::simple);
    wavesPage.setVisible (page == Page::waves);
    advancedPage.setVisible (page == Page::advanced);

    simpleTab.setToggleState (page == Page::simple, juce::dontSendNotification);
    wavesTab.setToggleState (page == Page::waves, juce::dontSendNotification);
    advancedTab.setToggleState (page == Page::advanced, juce::dontSendNotification);

    Settings::savePage (page == Page::waves ? "waves" : page == Page::advanced ? "advanced" : "simple");
}

void HelixAudioProcessorEditor::setHelpText (const juce::String& text)
{
    if (helpText != text)
    {
        helpText = text;
        repaint (0, getHeight() - footerHeight, getWidth(), footerHeight);
    }
}

void HelixAudioProcessorEditor::mouseEnter (const juce::MouseEvent& e)
{
    auto found = helpTexts.find (e.eventComponent);
    if (found != helpTexts.end())
    {
        helpOwner = found->first;
        setHelpText (found->second);
    }
}

void HelixAudioProcessorEditor::mouseExit (const juce::MouseEvent& e)
{
    // Only clear the text if it belongs to the control the mouse just left.
    if (e.eventComponent == helpOwner)
    {
        helpOwner = nullptr;
        setHelpText (defaultHelp);
    }
}

void HelixAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto& p = Theme::get();
    g.fillAll (p.ground);

    // Header
    auto header = getLocalBounds().removeFromTop (headerHeight).reduced (20, 0);
    g.setColour (p.ink);
    g.setFont (Theme::bold (26.0f));
    g.drawText (processor.getName(), header.removeFromLeft (90), juce::Justification::centredLeft);

    g.setColour (p.soft);
    g.setFont (Theme::regular (14.0f));
    g.drawText ("Wavetable synth", header.removeFromLeft (200), juce::Justification::centredLeft);

    // Footer: help text on the left, version on the right
    auto footer = getLocalBounds().removeFromBottom (footerHeight).reduced (20, 0);

    g.setColour (p.soft);
    g.setFont (Theme::regular (12.0f));
    g.drawText (versionTag, footer.removeFromRight (90), juce::Justification::centredRight);

    g.setColour (p.ink);
    g.setFont (Theme::regular (13.5f));
    g.drawFittedText (helpText, footer, juce::Justification::centredLeft, 2);
}

void HelixAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (headerHeight).reduced (20, 12);
    settingsButton.setBounds (header.removeFromRight (36));
    header.removeFromRight (12);
    advancedTab.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    wavesTab.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    simpleTab.setBounds (header.removeFromRight (100));

    area.removeFromBottom (footerHeight);
    simplePage.setBounds (area);
    wavesPage.setBounds (area);
    advancedPage.setBounds (area);
}
