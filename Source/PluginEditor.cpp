#include "PluginEditor.h"
#include "UI/Settings.h"
#include "UpdateLogic.h"
#include "Version.h"

namespace
{
    const juce::String defaultHelp = "Hover over any knob or button to see what it does.";
}

LapizView::LapizView (LapizAudioProcessor& p)
    : processor (p),
      helpText (defaultHelp),
      presetBar (p),
      simplePage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      wavesPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      advancedPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      effectsPage (p, [this] (juce::Component& c, const juce::String& text) { registerHelp (c, text); }),
      keyboard (p.getKeyboardState())
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
    registerHelp (presetBar.getFavoriteButton(), "Favorite: star the current sound to put it at the top of the preset list. "
                                                 "Click again to take it off. (A surprise sound can be starred once you've saved it.)");
    registerHelp (presetBar.getSurpriseButton(), "Surprise me: makes a brand-new sound at random, always a musical one. "
                                                 "Keep clicking until you hear one you like, then Save it.");

    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (settingsButton);
    registerHelp (settingsButton, "Settings: switch between the Dark and Daylight look, pick a window size, and choose whether LAPiZ updates itself. "
                                  "Your choices are remembered for every LAPiZ you open.");

    addChildComponent (simplePage);
    addChildComponent (wavesPage);
    addChildComponent (advancedPage);
    addChildComponent (effectsPage);

    addAndMakeVisible (keyboard);
    addChildComponent (focusRing);
    registerHelp (keyboard, "Piano: click a key to play a note, or drag across the keys. Click higher up a key to play it louder. "
                            "Notes from FL Studio light up here too. Middle C is C5, as in FL Studio.");

    const auto lastPage = Settings::loadPage();
    showPage (lastPage == "waves" ? Page::waves : lastPage == "advanced" ? Page::advanced
            : lastPage == "effects" ? Page::effects : Page::simple);

    setSize (designWidth, designHeight);

    updater->addChangeListener (this);
    updater->checkAutomatically();
}

LapizView::~LapizView()
{
    updater->removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void LapizView::showSettingsMenu()
{
    const bool dark = Theme::currentLook() == Theme::Look::dark;
    const bool updates = Updater::isSupported();
    const auto status = updater->getStatus();

    juce::PopupMenu menu;
    menu.addSectionHeader ("Look");
    menu.addItem (1, "Dark", true, dark);
    menu.addItem (2, "Daylight", true, ! dark);

    // Window size. You can also drag the bottom-right corner.
    static constexpr int sizes[] = { 75, 100, 125, 150, 200 };
    menu.addSectionHeader ("Size");
    for (int i = 0; i < (int) std::size (sizes); ++i)
        menu.addItem (10 + i, juce::String (sizes[i]) + " %", true,
                      std::abs (currentScale * 100.0f - (float) sizes[i]) < 1.0f);

    menu.addSectionHeader (updates ? "Updates" : "Updates (off in this build)");
    menu.addItem (3, "Update automatically", updates, updates && Settings::loadAutoUpdate());
    menu.addItem (4, status.state == Updater::State::checking ? "Checking for updates..." : "Check for updates now",
                  updates && status.state != Updater::State::checking);
    if (status.state == Updater::State::manual)
        menu.addItem (5, "Download LAPiZ " + status.version + " from GitHub");

    juce::Component::SafePointer<LapizView> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || result == 0)
                                return;

                            if (result == 1 || result == 2)
                                safeThis->setLook (result == 2 ? Theme::Look::daylight : Theme::Look::dark);
                            else if (result == 3)
                                Settings::saveAutoUpdate (! Settings::loadAutoUpdate());
                            else if (result == 4)
                                safeThis->updater->checkNow();
                            else if (result == 5)
                                juce::URL (UpdateLogic::releasesPage).launchInDefaultBrowser();
                            else if (result >= 10 && result < 10 + (int) std::size (sizes) && safeThis->onScaleChosen)
                                safeThis->onScaleChosen ((float) sizes[result - 10] / 100.0f);
                        });
}

void LapizView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint (0, getHeight() - footerHeight, getWidth(), footerHeight);
}

juce::String LapizView::updateText() const
{
    const auto s = updater->getStatus();
    switch (s.state)
    {
        case Updater::State::installed:
            return "Updated to v" + s.version + (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone
                                                     ? ". Restart LAPiZ to use it." : ". Restart FL Studio to use it.");
        case Updater::State::manual:    return "LAPiZ v" + s.version + " is out: get it from the settings menu.";
        case Updater::State::checking:  return s.userAsked ? "Checking for updates..." : juce::String();
        case Updater::State::upToDate:  return s.userAsked ? "LAPiZ " + juce::String (versionTag) + " is up to date." : juce::String();
        case Updater::State::failed:    return s.userAsked ? "Update failed: " + s.message + "." : juce::String();
        case Updater::State::idle:
        default:                        return {};
    }
}

void LapizView::setLook (Theme::Look look)
{
    Theme::currentLook() = look;
    Settings::saveLook (look);
    lookAndFeel.refreshColours();
    repaint();
}

void LapizView::registerHelp (juce::Component& component, const juce::String& text)
{
    helpTexts[&component] = text;
    component.addMouseListener (this, false);

    // The same words for screen readers, on the control itself (a knob's slider, not its labels).
    component.setHelpText (text);
    if (auto* knob = dynamic_cast<LabeledKnob*> (&component))
        knob->getSlider().setHelpText (text);
}

void LapizView::showPage (Page page)
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

void LapizView::setHelpText (const juce::String& text)
{
    if (helpText != text)
    {
        helpText = text;
        repaint (0, getHeight() - footerHeight, getWidth(), footerHeight);
    }
}

void LapizView::mouseEnter (const juce::MouseEvent& e)
{
    auto found = helpTexts.find (e.eventComponent);
    if (found != helpTexts.end())
    {
        helpOwner = found->first;
        setHelpText (found->second);
    }
}

void LapizView::mouseExit (const juce::MouseEvent& e)
{
    // Only clear the text if it belongs to the control the mouse just left.
    if (e.eventComponent == helpOwner)
    {
        helpOwner = nullptr;
        setHelpText (defaultHelp);
    }
}

void LapizView::paint (juce::Graphics& g)
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

    // News about updates replaces the version number while there is some.
    const auto news = updateText();
    if (news.isNotEmpty())
    {
        g.setColour (p.sound);
        g.setFont (Theme::bold (12.5f));
        g.drawFittedText (news, footer.removeFromRight (330), juce::Justification::centredRight, 2);
        footer.removeFromRight (16);
    }
    else
    {
        g.setColour (p.soft);
        g.setFont (Theme::regular (12.0f));
        g.drawText (versionTag, footer.removeFromRight (90), juce::Justification::centredRight);
    }

    g.setColour (p.ink);
    g.setFont (Theme::regular (13.5f));
    g.drawFittedText (helpText, footer, juce::Justification::centredLeft, 2);
}

void LapizView::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop (headerHeight).reduced (20, 12);
    settingsButton.setBounds (header.removeFromRight (36));
    header.removeFromRight (12);
    effectsTab.setBounds (header.removeFromRight (86));
    header.removeFromRight (6);
    advancedTab.setBounds (header.removeFromRight (86));
    header.removeFromRight (6);
    wavesTab.setBounds (header.removeFromRight (86));
    header.removeFromRight (6);
    simpleTab.setBounds (header.removeFromRight (86));

    // The preset bar sits between the LAPiZ name and the tabs.
    header.removeFromLeft (110);
    header.removeFromRight (16);
    presetBar.setBounds (header);

    area.removeFromBottom (footerHeight);
    keyboard.setBounds (area.removeFromBottom (keyboardHeight).reduced (20, 0).withTrimmedBottom (4));
    simplePage.setBounds (area);
    wavesPage.setBounds (area);
    advancedPage.setBounds (area);
    effectsPage.setBounds (area);
}

//==============================================================================
LapizAudioProcessorEditor::LapizAudioProcessorEditor (LapizAudioProcessor& p)
    : AudioProcessorEditor (&p), view (p)
{
    addAndMakeVisible (view);
    view.onScaleChosen = [this] (float scale) { setScale (scale); };

    setResizable (true, true);
    setResizeLimits (juce::roundToInt (LapizView::designWidth * minScale), juce::roundToInt (LapizView::designHeight * minScale),
                     juce::roundToInt (LapizView::designWidth * maxScale), juce::roundToInt (LapizView::designHeight * maxScale));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) LapizView::designWidth / (double) LapizView::designHeight);

    setScale (Settings::loadWindowScale());
}

LapizAudioProcessorEditor::~LapizAudioProcessorEditor()
{
    Settings::saveWindowScale (getScale());
}

float LapizAudioProcessorEditor::getScale() const
{
    return (float) getWidth() / (float) LapizView::designWidth;
}

void LapizAudioProcessorEditor::setScale (float scale)
{
    scale = juce::jlimit (minScale, maxScale, scale);
    setSize (juce::roundToInt (LapizView::designWidth * scale), juce::roundToInt (LapizView::designHeight * scale));
}

void LapizAudioProcessorEditor::resized()
{
    const auto scale = getScale();
    view.setBounds (0, 0, LapizView::designWidth, LapizView::designHeight);
    view.setTransform (juce::AffineTransform::scale (scale));
    view.currentScale = scale;
}
