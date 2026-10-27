#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../FactoryPresets.h"
#include "../PluginProcessor.h"

// Shows which preset (whole sound) is loaded. Click the name for the full list, use the arrows
// to step through the built-in presets and your saved ones, or Save to keep the current sound.
class PresetBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBar (LapizAudioProcessor& p) : processor (p)
    {
        for (auto* b : { &previous, &next, &nameButton, &saveButton })
            addAndMakeVisible (b);

        previous.onClick   = [this] { step (-1); };
        next.onClick       = [this] { step (1); };
        nameButton.onClick = [this] { showList(); };
        saveButton.onClick = [this] { save(); };

        processor.addChangeListener (this);
        refresh();
    }

    ~PresetBar() override { processor.removeChangeListener (this); }

    // The individual parts, so the editor can give each its own hover help.
    juce::Button& getNameButton()     { return nameButton; }
    juce::Button& getPreviousButton() { return previous; }
    juce::Button& getNextButton()     { return next; }
    juce::Button& getSaveButton()     { return saveButton; }

    void resized() override
    {
        auto area = getLocalBounds();
        saveButton.setBounds (area.removeFromRight (58));
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

        juce::PopupMenu menu;
        juce::String category = "-";
        bool userHeaderShown = false;

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
                    category = preset.category;
                    if (category.isNotEmpty())
                        menu.addSectionHeader (category);
                }
                menu.addItem (id, preset.name, true, ticked);
            }
            else
            {
                if (! userHeaderShown)
                {
                    menu.addSectionHeader ("My presets");
                    userHeaderShown = true;
                }
                menu.addItem (id, e.file.getFileNameWithoutExtension(), true, ticked);
            }
        }

        if (! userHeaderShown)
        {
            menu.addSectionHeader ("My presets");
            menu.addItem (-1, "Click Save to keep your own sounds", false, false);
        }

        menu.addSeparator();
        menu.addItem (-2, "Open presets folder");

        juce::Component::SafePointer<PresetBar> safeThis (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton),
                            [safeThis, all] (int result)
                            {
                                if (safeThis == nullptr)
                                    return;
                                if (result == -2)
                                {
                                    LapizAudioProcessor::presetFolder().createDirectory();
                                    LapizAudioProcessor::presetFolder().startAsProcess();
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
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    LapizAudioProcessor& processor;
    juce::TextButton previous { "<" }, next { ">" }, nameButton, saveButton { "Save" };
    std::unique_ptr<juce::FileChooser> chooser;
};
