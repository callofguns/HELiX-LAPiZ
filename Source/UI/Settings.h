#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "Theme.h"

// Preferences that apply to every LAPiZ, saved in the user's app-data folder
// (on Windows: %APPDATA%\LAPiZ\LAPiZ.settings).
namespace Settings
{
    inline juce::PropertiesFile::Options fileOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "LAPiZ";
        options.folderName = "LAPiZ";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        return options;
    }

    inline Theme::Look loadLook()
    {
        juce::PropertiesFile file (fileOptions());
        return file.getValue ("look", "dark") == "daylight" ? Theme::Look::daylight : Theme::Look::dark;
    }

    inline void saveLook (Theme::Look look)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("look", look == Theme::Look::daylight ? "daylight" : "dark");
        file.saveIfNeeded();
    }

    // The tab that was open last ("simple", "waves", "advanced" or "effects"), so LAPiZ reopens where you left off.
    inline juce::String loadPage()
    {
        juce::PropertiesFile file (fileOptions());
        return file.getValue ("page", "simple");
    }

    inline void savePage (const juce::String& page)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("page", page);
        file.saveIfNeeded();
    }
}
