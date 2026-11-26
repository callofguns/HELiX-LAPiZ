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

    // Whether LAPiZ looks for and installs new versions by itself (on unless you turn it off).
    inline bool loadAutoUpdate()
    {
        juce::PropertiesFile file (fileOptions());
        return file.getBoolValue ("autoUpdate", true);
    }

    inline void saveAutoUpdate (bool on)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("autoUpdate", on);
        file.saveIfNeeded();
    }

    // When LAPiZ last asked GitHub for a new version (milliseconds since 1970), so it only asks a few times a day.
    inline juce::int64 loadLastUpdateCheck()
    {
        juce::PropertiesFile file (fileOptions());
        return file.getValue ("lastUpdateCheck", "0").getLargeIntValue();
    }

    inline void saveLastUpdateCheck (juce::int64 when)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("lastUpdateCheck", juce::String (when));
        file.saveIfNeeded();
    }

    // How big the window is: 1.0 = 100 % (900 x 644). Shared by every LAPiZ you open.
    inline float loadWindowScale()
    {
        juce::PropertiesFile file (fileOptions());
        return juce::jlimit (0.75f, 2.0f, (float) file.getDoubleValue ("windowScale", 1.0));
    }

    inline void saveWindowScale (float scale)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("windowScale", (double) scale);
        file.saveIfNeeded();
    }

    // Starred presets, so they're easy to find. A built-in preset is stored as "factory:<name>",
    // one of yours as "file:<full path>".
    inline juce::StringArray loadFavorites()
    {
        juce::PropertiesFile file (fileOptions());
        auto favorites = juce::StringArray::fromLines (file.getValue ("favorites"));
        favorites.removeEmptyStrings();
        return favorites;
    }

    inline void saveFavorites (const juce::StringArray& favorites)
    {
        juce::PropertiesFile file (fileOptions());
        file.setValue ("favorites", favorites.joinIntoString ("\n"));
        file.saveIfNeeded();
    }
}
