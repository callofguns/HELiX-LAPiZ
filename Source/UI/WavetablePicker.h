#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../FactoryTables.h"
#include "../PluginProcessor.h"
#include "../WavetableFile.h"

// Shows which wavetable is loaded. Click the name for the full list, or use the arrows
// to step through the built-in tables, your saved files and your latest drawing.
class WavetablePicker : public juce::Component, private juce::ChangeListener
{
public:
    explicit WavetablePicker (LapizAudioProcessor& p) : processor (p)
    {
        for (auto* b : { &previous, &next, &nameButton })
            addAndMakeVisible (b);

        previous.onClick   = [this] { step (-1); };
        next.onClick       = [this] { step (1); };
        nameButton.onClick = [this] { showList(); };

        processor.addChangeListener (this);
        refresh();
    }

    ~WavetablePicker() override { processor.removeChangeListener (this); }

    // The individual parts, so the page can give each its own hover help.
    juce::Button& getNameButton()     { return nameButton; }
    juce::Button& getPreviousButton() { return previous; }
    juce::Button& getNextButton()     { return next; }

    void resized() override
    {
        auto area = getLocalBounds();
        previous.setBounds (area.removeFromLeft (30));
        next.setBounds (area.removeFromRight (30));
        area.reduce (6, 0);
        nameButton.setBounds (area);
    }

private:
    struct Entry
    {
        TableChoice::Kind kind;
        int factoryIndex = 0;
        juce::File file;
    };

    // Everything the arrows step through, in the same order as the list.
    std::vector<Entry> entries() const
    {
        std::vector<Entry> all;
        for (int i = 0; i < (int) FactoryTables::list.size(); ++i)
            all.push_back ({ TableChoice::Kind::factory, i, {} });
        for (auto& f : WavetableFile::listUserFiles())
            all.push_back ({ TableChoice::Kind::file, 0, f });
        if (! processor.getLastDrawing().keyframes.empty())
            all.push_back ({ TableChoice::Kind::drawn, 0, {} });
        return all;
    }

    int currentIndex (const std::vector<Entry>& all) const
    {
        const auto choice = processor.getTableChoice();
        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& e = all[i];
            if (e.kind != choice.kind) continue;
            if (e.kind == TableChoice::Kind::drawn
                || (e.kind == TableChoice::Kind::factory && e.factoryIndex == choice.factoryIndex)
                || (e.kind == TableChoice::Kind::file && e.file == choice.file))
                return (int) i;
        }
        return -1;
    }

    void choose (const Entry& e)
    {
        switch (e.kind)
        {
            case TableChoice::Kind::factory: processor.selectFactoryTable (e.factoryIndex); break;
            case TableChoice::Kind::file:    processor.selectTableFile (e.file); break;
            case TableChoice::Kind::drawn:
            {
                auto drawing = processor.getLastDrawing();
                processor.setDrawnTable (std::move (drawing.keyframes), drawing.name, drawing.editorTag);
                break;
            }
        }
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
        juce::String category;
        bool userHeaderShown = false;

        for (size_t i = 0; i < all.size(); ++i)
        {
            const auto& e = all[i];
            const int id = (int) i + 1;
            const bool ticked = (int) i == now;

            if (e.kind == TableChoice::Kind::factory)
            {
                const auto& info = FactoryTables::list[(size_t) e.factoryIndex];
                if (category != info.category)
                {
                    category = info.category;
                    menu.addSectionHeader (category);
                }
                menu.addItem (id, info.name, true, ticked);
            }
            else
            {
                if (! userHeaderShown)
                {
                    menu.addSectionHeader ("My wavetables");
                    userHeaderShown = true;
                }
                if (e.kind == TableChoice::Kind::file)
                    menu.addItem (id, e.file.getFileNameWithoutExtension(), true, ticked);
                else
                    menu.addItem (id, processor.getLastDrawing().name + " (drawing)", true, ticked);
            }
        }

        if (! userHeaderShown)
        {
            menu.addSectionHeader ("My wavetables");
            menu.addItem (-1, "Draw one on the Waves tab", false, false);
        }

        juce::Component::SafePointer<WavetablePicker> safeThis (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameButton),
                            [safeThis, all] (int result)
                            {
                                if (safeThis != nullptr && result > 0 && result <= (int) all.size())
                                    safeThis->choose (all[(size_t) result - 1]);
                            });
    }

    void refresh()
    {
        nameButton.setButtonText (processor.getTableChoice().name);
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    LapizAudioProcessor& processor;
    juce::TextButton previous { "<" }, next { ">" }, nameButton;
};
