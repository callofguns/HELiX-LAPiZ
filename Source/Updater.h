#pragma once

#include <juce_events/juce_events.h>

// Keeps LAPiZ up to date on Windows.
//
// When a LAPiZ window opens, this asks GitHub (at most every few hours) whether a newer release
// exists. If one does, it downloads the update zip, checks its SHA-256 against update.json, and
// swaps the new files in over the installed plugin (and the standalone app, if installed).
// Windows lets a loaded plugin file be renamed but not overwritten, so each old file is renamed to
// "<name>.old-<time>" and the new one is copied into its place. FL Studio keeps using the old copy
// until it restarts; the leftovers are deleted the next time LAPiZ checks for updates.
//
// The installer gives normal users write access to the LAPiZ folders, so updating never needs an
// administrator prompt. If those folders can't be written (for example an install from before
// v1.2), LAPiZ says a new version is available and links to the download instead.
//
// Only release builds made with -DLAPIZ_AUTO_UPDATE=ON (the GitHub Actions release workflow) update
// themselves, so a build on your own computer never replaces itself with the published version.
//
// One Updater is shared by every LAPiZ in the same process (use juce::SharedResourcePointer).
class Updater : public juce::ChangeBroadcaster, private juce::Thread
{
public:
    enum class State
    {
        idle,      // nothing to say
        checking,  // asking GitHub / downloading / installing
        upToDate,  // this is the newest version
        installed, // a new version has been put in place; restart FL Studio to use it
        manual,    // a new version exists but has to be installed by hand (see releasesPage)
        failed     // something went wrong; message says what
    };

    struct Status
    {
        State state = State::idle;
        juce::String version;  // the new version, for installed and manual
        juce::String message;  // for failed
        bool userAsked = false; // true if this came from "Check for updates now" rather than the automatic check
    };

    Updater();
    ~Updater() override;

    // False in builds that don't update themselves (local builds, and anything but Windows).
    static bool isSupported();

    // Called when a LAPiZ window opens: checks if updates are on and the last check was a while ago.
    void checkAutomatically();

    // "Check for updates now" in the settings menu: checks straight away, whatever the setting.
    void checkNow();

    Status getStatus() const;

    // The version this copy of LAPiZ is, from CMakeLists.txt (e.g. "1.2.0").
    static juce::String currentVersion();

private:
    void startCheck (bool userAsked);
    void run() override;
    void setStatus (State state, const juce::String& version = {}, const juce::String& message = {});

    // Returns the status to show when finished.
    Status checkAndInstall();

    mutable juce::CriticalSection lock;
    Status status;
    bool userAskedForThisCheck = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Updater)
};
