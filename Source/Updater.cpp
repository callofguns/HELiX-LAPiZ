#include "Updater.h"
#include <juce_cryptography/juce_cryptography.h>
#include "UpdateLogic.h"
#include "UI/Settings.h"

#if JUCE_WINDOWS && LAPIZ_AUTO_UPDATE
 #define LAPIZ_UPDATER_ACTIVE 1
#else
 #define LAPIZ_UPDATER_ACTIVE 0
#endif

namespace
{
    // How long between automatic checks.
    constexpr juce::int64 checkIntervalMs = 6 * 60 * 60 * 1000;

    constexpr int timeoutMs = 15000;

   #if LAPIZ_UPDATER_ACTIVE
    constexpr juce::int64 maxDownloadBytes = 200 * 1024 * 1024;

    // Written into each installed folder by the release workflow, so a process still running an older
    // copy can tell that a newer one is already installed and doesn't download it again.
    const juce::String versionFileName = "lapiz-version.txt";

    // Where the installer puts things (see installer/LAPiZ.iss).
    struct Targets
    {
        juce::File plugin;     // ...\Common Files\VST3\LAPiZ.vst3, if installed
        juce::File standalone; // the folder with LAPiZ.exe, if installed
    };

    Targets findTargets()
    {
        Targets t;

        auto commonFiles = juce::SystemStats::getEnvironmentVariable ("CommonProgramW6432", {});
        if (commonFiles.isEmpty())
            commonFiles = juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files");
        const auto plugin = juce::File (commonFiles).getChildFile ("VST3").getChildFile ("LAPiZ.vst3");
        if (plugin.isDirectory())
            t.plugin = plugin;

        // Inno Setup records where it installed the app under the AppId from LAPiZ.iss.
        const auto location = juce::WindowsRegistry::getValue (
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
            "{8AC7DA59-70F4-4FBF-BF06-E7911CD5DCFE}_is1\\InstallLocation", {},
            juce::WindowsRegistry::WoW64_64bit);
        if (location.isNotEmpty() && juce::File::isAbsolutePath (location)
            && juce::File (location).getChildFile ("LAPiZ.exe").existsAsFile())
            t.standalone = juce::File (location);

        return t;
    }

    juce::String readVersionFile (const juce::File& file)
    {
        const auto text = file.loadFileAsString().trim();
        return UpdateLogic::parseVersion (text).empty() ? juce::String() : text;
    }

    // The newest version known to be installed: this copy, or one an earlier update already put in place.
    juce::String installedVersion (const Targets& t)
    {
        auto version = Updater::currentVersion();
        if (t.plugin != juce::File())
            version = UpdateLogic::later (version, readVersionFile (t.plugin.getChildFile ("Contents/Resources").getChildFile (versionFileName)));
        if (t.standalone != juce::File())
            version = UpdateLogic::later (version, readVersionFile (t.standalone.getChildFile (versionFileName)));
        return version;
    }

    bool canWriteTo (const juce::File& folder)
    {
        const auto probe = folder.getNonexistentChildFile (".lapiz-write-test", ".tmp", false);
        const bool ok = probe.create().wasOk();
        probe.deleteFile();
        return ok;
    }

    // Files renamed out of the way by an earlier update. Deleting fails (harmlessly) while they're still loaded.
    void deleteLeftovers (const juce::File& folder)
    {
        if (folder == juce::File())
            return;
        for (auto& f : folder.findChildFiles (juce::File::findFiles, true, "*.old-*"))
            f.deleteFile();
    }

    // Puts every file from 'from' (recursively) into 'to'. Each existing file is renamed to
    // "<name>.old-<stamp>" first, which Windows allows even when the file is loaded.
    // If anything fails, everything done so far is undone.
    struct FileSwap
    {
        juce::File target, previous; // previous is empty if there was no file before
    };

    bool swapIn (const juce::File& from, const juce::File& to, bool recursive, const juce::String& stamp,
                 std::vector<FileSwap>& done)
    {
        for (auto& source : from.findChildFiles (juce::File::findFiles, recursive))
        {
            const auto target = to.getChildFile (source.getRelativePathFrom (from));
            if (! target.isAChildOf (to) || ! target.getParentDirectory().createDirectory())
                return false;

            FileSwap swap { target, {} };
            if (target.exists())
            {
                swap.previous = target.getSiblingFile (target.getFileName() + ".old-" + stamp);
                if (! target.moveFileTo (swap.previous))
                    return false;
            }
            done.push_back (swap);

            if (! source.copyFileTo (target))
                return false;
        }
        return true;
    }

    void undo (std::vector<FileSwap>& done)
    {
        for (auto it = done.rbegin(); it != done.rend(); ++it)
        {
            it->target.deleteFile();
            if (it->previous != juce::File())
                it->previous.moveFileTo (it->target);
        }
        done.clear();
    }
   #endif
}

Updater::Updater() : juce::Thread ("LAPiZ updater") {}

Updater::~Updater()
{
    // Downloads stop within one chunk; installing (a few file renames and copies) is never interrupted.
    stopThread (timeoutMs + 5000);
}

bool Updater::isSupported()
{
    return LAPIZ_UPDATER_ACTIVE != 0;
}

juce::String Updater::currentVersion()
{
   #ifdef JucePlugin_VersionString
    return JucePlugin_VersionString;
   #else
    return "0.0.0";
   #endif
}

void Updater::checkAutomatically()
{
    if (! isSupported() || ! Settings::loadAutoUpdate())
        return;

    const auto now = juce::Time::currentTimeMillis();
    const auto last = Settings::loadLastUpdateCheck();
    if (last > 0 && last <= now && now - last < checkIntervalMs)
        return;

    startCheck (false);
}

void Updater::checkNow()
{
    if (isSupported())
        startCheck (true);
}

void Updater::startCheck (bool userAsked)
{
    if (isThreadRunning())
        return;

    {
        const juce::ScopedLock sl (lock);
        if (status.state == State::installed)
        {
            // Already done in this session; just show it again.
            status.userAsked = userAsked;
            sendChangeMessage();
            return;
        }
        userAskedForThisCheck = userAsked;
    }

    setStatus (State::checking);
    startThread (juce::Thread::Priority::low);
}

Updater::Status Updater::getStatus() const
{
    const juce::ScopedLock sl (lock);
    return status;
}

void Updater::setStatus (State state, const juce::String& version, const juce::String& message)
{
    {
        const juce::ScopedLock sl (lock);
        status.state = state;
        status.version = version;
        status.message = message;
        status.userAsked = userAskedForThisCheck;
    }
    sendChangeMessage();
}

void Updater::run()
{
    const auto result = checkAndInstall();

    if (threadShouldExit())
        return;

    if (result.state != State::failed)
        Settings::saveLastUpdateCheck (juce::Time::currentTimeMillis());

    setStatus (result.state, result.version, result.message);
}

Updater::Status Updater::checkAndInstall()
{
    Status result;

   #if LAPIZ_UPDATER_ACTIVE
    auto fail = [&result] (const juce::String& message) { result.state = State::failed; result.message = message; return result; };

    const auto targets = findTargets();
    deleteLeftovers (targets.plugin);
    deleteLeftovers (targets.standalone);

    // 1. Ask GitHub for the newest version.
    int statusCode = 0;
    auto manifestStream = juce::URL (UpdateLogic::manifestUrl)
                              .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                                      .withConnectionTimeoutMs (timeoutMs)
                                                      .withStatusCode (&statusCode));
    if (manifestStream == nullptr || statusCode != 200)
        return fail ("couldn't reach GitHub");

    const auto manifest = UpdateLogic::parseManifest (manifestStream->readEntireStreamAsString().substring (0, 4096));
    manifestStream.reset();
    if (! manifest)
        return fail ("the update information on GitHub couldn't be read");

    if (! UpdateLogic::isNewer (manifest->version, installedVersion (targets)))
    {
        result.state = State::upToDate;
        return result;
    }

    result.version = manifest->version;

    // 2. Make sure the new files can be put in place without an administrator prompt.
    const bool anyTarget = targets.plugin != juce::File() || targets.standalone != juce::File();
    if (! anyTarget
        || (targets.plugin != juce::File() && ! canWriteTo (targets.plugin))
        || (targets.standalone != juce::File() && ! canWriteTo (targets.standalone)))
    {
        result.state = State::manual;
        return result;
    }

    // 3. Download the update and check it's exactly the file the release published.
    const auto work = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("LAPiZ-update");
    work.deleteRecursively();
    if (! work.createDirectory())
        return fail ("couldn't create a temporary folder");

    const auto zipFile = work.getChildFile ("update.zip");
    {
        statusCode = 0;
        auto in = juce::URL (manifest->url)
                      .createInputStream (juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                              .withConnectionTimeoutMs (timeoutMs)
                                              .withStatusCode (&statusCode));
        if (in == nullptr || statusCode != 200)
            return fail ("the download didn't start");

        juce::FileOutputStream out (zipFile);
        if (out.failedToOpen())
            return fail ("couldn't save the download");

        juce::HeapBlock<char> buffer (65536);
        juce::int64 total = 0;
        while (! in->isExhausted())
        {
            if (threadShouldExit())
                return fail ("stopped");

            const auto n = in->read (buffer, 65536);
            if (n < 0)
                return fail ("the download was cut off");
            if (n == 0)
                break;

            total += n;
            if (total > maxDownloadBytes || ! out.write (buffer, (size_t) n))
                return fail ("the download was too large or couldn't be saved");
        }
        out.flush();
        if (out.getStatus().failed())
            return fail ("couldn't save the download");
    }

    if (juce::SHA256 (zipFile).toHexString().toLowerCase() != manifest->sha256)
    {
        work.deleteRecursively();
        return fail ("the download was damaged (its checksum didn't match)");
    }

    // 4. Unpack: LAPiZ.vst3\... goes over the plugin, everything else next to the standalone app.
    const auto staging = work.getChildFile ("files");
    {
        juce::ZipFile zip (zipFile);
        if (zip.getNumEntries() == 0 || zip.uncompressTo (staging).failed())
            return fail ("couldn't unpack the update");
    }

    const auto newPlugin = staging.getChildFile ("LAPiZ.vst3");
    if (targets.plugin != juce::File() && ! newPlugin.isDirectory())
        return fail ("the update has no plugin in it");
    if (targets.standalone != juce::File() && ! staging.getChildFile ("LAPiZ.exe").existsAsFile())
        return fail ("the update has no app in it");

    // 5. Swap the files in. The lock stops two copies of LAPiZ (say FL Studio and the app) doing it at once.
    {
        juce::InterProcessLock installLock ("LAPiZ-update");
        if (! installLock.enter (30000))
            return fail ("another copy of LAPiZ is updating");

        // The other copy may have just finished installing the same version.
        if (! UpdateLogic::isNewer (manifest->version, installedVersion (targets)))
        {
            work.deleteRecursively();
            result.state = State::installed;
            return result;
        }

        const auto stamp = juce::String (juce::Time::currentTimeMillis());
        std::vector<FileSwap> done;
        bool ok = true;

        if (targets.plugin != juce::File())
            ok = swapIn (newPlugin, targets.plugin, true, stamp, done);

        if (ok && targets.standalone != juce::File())
            ok = swapIn (staging, targets.standalone, false, stamp, done);

        if (! ok)
        {
            undo (done);
            work.deleteRecursively();
            return fail ("couldn't replace the installed files, so nothing was changed");
        }
    }

    work.deleteRecursively();
    result.state = State::installed;
   #else
    result.state = State::idle;
   #endif

    return result;
}
