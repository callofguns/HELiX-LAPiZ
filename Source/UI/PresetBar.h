#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Icons.h"
#include "Settings.h"
#include "Theme.h"
#include "../FactoryPresets.h"
#include "../PluginProcessor.h"

// Shows which preset (whole sound) is loaded. Click the name for the full list, use the arrows
// to step through the built-in presets and your saved ones, star a preset to add it to your
// favorites, roll the die for a surprise sound, or Save to keep the current sound.
class PresetBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBar (LapizAudioProcessor& p) : processor (p)
    {
        for (juce::Component* b : { (juce::Component*) &previous, (juce::Component*) &next, (juce::Component*) &nameButton,
                                    (juce::Component*) &favoriteButton, (juce::Component*) &surpriseButton, (juce::Component*) &saveButton })
            addAndMakeVisible (b);

        previous.onClick       = [this] { step (-1); };
        next.onClick           = [this] { step (1); };
        nameButton.onClick     = [this] { showList(); };
        saveButton.onClick     = [this] { save(); };
        favoriteButton.onClick = [this] { toggleFavorite(); };
        surpriseButton.onClick = [this] { processor.loadSurprise(); };

        processor.addChangeListener (this);
        refresh();
    }

    ~PresetBar() override { processor.removeChangeListener (this); }

    // The individual parts, so the editor can give each its own hover help.
    juce::Button& getNameButton()     { return nameButton; }
    juce::Button& getPreviousButton() { return previous; }
    juce::Button& getNextButton()     { return next; }
    juce::Button& getSaveButton()     { return saveButton; }
    juce::Button& getFavoriteButton() { return favoriteButton; }
    juce::Button& getSurpriseButton() { return surpriseButton; }

    void resized() override
    {
        auto area = getLocalBounds();
        saveButton.setBounds (area.removeFromRight (52));
        area.removeFromRight (6);
        surpriseButton.setBounds (area.removeFromRight (getHeight()));
        area.removeFromRight (4);
        favoriteButton.setBounds (area.removeFromRight (getHeight()));
        area.removeFromRight (8);
        previous.setBounds (area.removeFromLeft (30));
        next.setBounds (area.removeFromRight (30));
        area.reduce (4, 0);
        nameButton.setBounds (area);
    }

private:
    struct Entry
    {
        int factoryIndex = -1; // -1 for a saved file
        juce::File file;
    };

    // Everything the arrows step through, in the same order as the list.
    static std::vector<Entry> entries()
    {
        std::vector<Entry> all;
        for (int i = 0; i < (int) FactoryPresets::list().size(); ++i)
            all.push_back ({ i, {} });
        for (auto& f : LapizAudioProcessor::listUserPresets())
            all.push_back ({ -1, f });
        return all;
    }

    int currentIndex (const std::vector<Entry>& all) const
    {
        const auto file = processor.getPresetFile();
        const auto name = processor.getPresetName();
        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& e = all[i];
            if (file != juce::File() ? e.file == file
                                     : (e.factoryIndex >= 0 && name == FactoryPresets::list()[(size_t) e.factoryIndex].name))
                return (int) i;
        }
        return -1;
    }

    void choose (const Entry& e)
    {
        if (e.factoryIndex >= 0)
            processor.loadFactoryPreset (e.factoryIndex);
        else if (! processor.loadPresetFile (e.file))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't open preset",
                                                    "LAPiZ couldn't read " + e.file.getFullPathName());
    }

    // How the current preset is remembered among your favorites; empty if it can't be starred
    // (a surprise sound that hasn't been saved yet).
    juce::String currentKey() const
    {
        const auto file = processor.getPresetFile();
        if (file != juce::File())
            return "file:" + file.getFullPathName();
        const auto name = processor.getPresetName();
        for (auto& preset : FactoryPresets::list())
            if (name == preset.name)
                return juce::String ("factory:") + preset.name;
        return {};
    }

    // Your favorites that still exist, in the order you starred them.
    static std::vector<std::pair<juce::String, Entry>> favoriteEntries()
    {
        std::vector<std::pair<juce::String, Entry>> found;
        for (auto& key : Settings::loadFavorites())
        {
            if (key.startsWith ("factory:"))
            {
                const auto name = key.fromFirstOccurrenceOf ("factory:", false, false);
                for (int i = 0; i < (int) FactoryPresets::list().size(); ++i)
                    if (name == FactoryPresets::list()[(size_t) i].name)
                        found.push_back ({ name, Entry { i, {} } });
            }
            else if (key.startsWith ("file:"))
            {
                const juce::File file (key.fromFirstOccurrenceOf ("file:", false, false));
                if (file.existsAsFile())
                    found.push_back ({ file.getFileNameWithoutExtension(), Entry { -1, file } });
            }
        }
        return found;
    }

    void toggleFavorite()
    {
        const auto key = currentKey();
        if (key.isEmpty())
            return;
        auto favorites = Settings::loadFavorites();
        if (favorites.contains (key))
            favorites.removeString (key);
        else
            favorites.add (key);
        Settings::saveFavorites (favorites);
        refresh();
    }

    void step (int delta)
    {
        const auto all = entries();
        const int count = (int) all.size();
        const int now = currentIndex (all);
        const int target = now < 0 ? (delta > 0 ? 0 : count - 1) : (now + delta + count) % count;
        choose (all[(size_t) target]);
    }

    void showList()
    {
        const auto all = entries();
        const int now = currentIndex (all);

        // Built-in presets go in one submenu per category (they're listed category by category),
        // except those with no category, like Init, which sit at the top.
        juce::PopupMenu menu, categoryMenu;

        // Your favorites come first. Their menu ids start at favoriteIdBase.
        const auto favorites = favoriteEntries();
        const auto nowKey = currentKey();
        menu.addSectionHeader ("Favorites");
        if (favorites.empty())
            menu.addItem (-3, "Click the star to add the current sound here", false, false);
        for (size_t i = 0; i < favorites.size(); ++i)
        {
            const auto& e = favorites[i].second;
            const auto key = e.factoryIndex >= 0 ? juce::String ("factory:") + FactoryPresets::list()[(size_t) e.factoryIndex].name
                                                 : "file:" + e.file.getFullPathName();
            menu.addItem (favoriteIdBase + (int) i, favorites[i].first, true, key == nowKey);
        }
        menu.addSectionHeader ("All sounds");
        juce::String category;
        bool categoryTicked = false;
        bool userHeaderShown = false;

        auto finishCategory = [&]
        {
            if (category.isNotEmpty())
                menu.addSubMenu (category, categoryMenu, true, nullptr, categoryTicked);
            categoryMenu = {};
            categoryTicked = false;
        };

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& e = all[i];
            const int id = (int) i + 1;
            const bool ticked = (int) i == now;

            if (e.factoryIndex >= 0)
            {
                const auto& preset = FactoryPresets::list()[(size_t) e.factoryIndex];
                if (category != preset.category)
                {
                    finishCategory();
                    category = preset.category;
                }
                if (category.isEmpty())
                    menu.addItem (id, preset.name, true, ticked);
                else
                    categoryMenu.addItem (id, preset.name, true, ticked);
                categoryTicked = categoryTicked || ticked;
            }
            else
            {
                if (! userHeaderShown)
                {
                    finishCategory();
                    category = {};
                    menu.addSectionHeader ("My presets");
                    userHeaderShown = true;
                }
                menu.addItem (id, e.file.getFileNameWithoutExtension(), true, ticked);
            }
        }

        if (! userHeaderShown)
        {
            finishCategory();
            menu.addSectionHeader ("My presets");
            menu.addItem (-1, "Click Save to keep your own sounds", false, false);
        }

        menu.addSeparator();
        menu.addItem (-2, "Open presets folder");

        juce::Component::SafePointer<PresetBar> safeThis (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton),
                            [safeThis, all, favorites] (int result)
                            {
                                if (safeThis == nullptr)
                                    return;
                                if (result == -2)
                                {
                                    LapizAudioProcessor::presetFolder().createDirectory();
                                    LapizAudioProcessor::presetFolder().startAsProcess();
                                }
                                else if (result >= favoriteIdBase && result < favoriteIdBase + (int) favorites.size())
                                {
                                    safeThis->choose (favorites[(size_t) (result - favoriteIdBase)].second);
                                }
                                else if (result > 0 && result <= (int) all.size())
                                {
                                    safeThis->choose (all[(size_t) result - 1]);
                                }
                            });
    }

    // Asks for a file name in the presets folder, suggesting the current preset's name.
    void save()
    {
        const auto folder = LapizAudioProcessor::presetFolder();
        folder.createDirectory();

        const auto suggestion = folder.getChildFile (juce::File::createLegalFileName (processor.getPresetName()))
                                      .withFileExtension (LapizAudioProcessor::presetExtension);

        chooser = std::make_unique<juce::FileChooser> ("Save preset", suggestion,
                                                       juce::String ("*") + LapizAudioProcessor::presetExtension);
        juce::Component::SafePointer<PresetBar> safeThis (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safeThis] (const juce::FileChooser& fc)
                              {
                                  if (safeThis == nullptr || fc.getResult() == juce::File())
                                      return;

                                  const auto file = fc.getResult().withFileExtension (LapizAudioProcessor::presetExtension);
                                  if (! safeThis->processor.savePresetFile (file))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't save preset",
                                                                              "LAPiZ couldn't write " + file.getFullPathName());
                              });
    }

    void refresh()
    {
        nameButton.setButtonText (processor.getPresetName());
        const auto key = currentKey();
        favoriteButton.setEnabled (key.isNotEmpty());
        favoriteButton.setToggleState (key.isNotEmpty() && Settings::loadFavorites().contains (key), juce::dontSendNotification);
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    LapizAudioProcessor& processor;
    static constexpr int favoriteIdBase = 100000;

    IconButton previous { "Previous preset", IconButton::Icon::previous }, next { "Next preset", IconButton::Icon::next };
    juce::TextButton nameButton, saveButton { "Save" };
    IconButton favoriteButton { "Favorite", IconButton::Icon::star }, surpriseButton { "Surprise me", IconButton::Icon::die };
    std::unique_ptr<juce::FileChooser> chooser;
};
