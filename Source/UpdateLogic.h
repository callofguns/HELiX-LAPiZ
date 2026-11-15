#pragma once

#include <optional>
#include <juce_core/juce_core.h>

// The parts of updating that don't touch the network or the disk, so the tests can check them.
namespace UpdateLogic
{
    // Every release publishes update.json next to its files, and this link always points at the newest one.
    inline const juce::String manifestUrl = "https://github.com/callofguns/HELiX-LAPiZ/releases/latest/download/update.json";

    // Downloads are only accepted from this repository's releases.
    inline const juce::String allowedDownloadPrefix = "https://github.com/callofguns/HELiX-LAPiZ/releases/download/";

    inline const juce::String releasesPage = "https://github.com/callofguns/HELiX-LAPiZ/releases/latest";

    // "1.2.0", "v1.2" or "1.2" -> {1, 2, 0}. Empty if it isn't a version number.
    inline std::vector<int> parseVersion (juce::String text)
    {
        text = text.trim();
        if (text.startsWithIgnoreCase ("v"))
            text = text.substring (1);

        const auto parts = juce::StringArray::fromTokens (text, ".", "");
        if (parts.isEmpty() || parts.size() > 4)
            return {};

        std::vector<int> numbers;
        for (auto& part : parts)
        {
            if (part.isEmpty() || part.length() > 6 || ! part.containsOnly ("0123456789"))
                return {};
            numbers.push_back (part.getIntValue());
        }
        while (numbers.size() < 3)
            numbers.push_back (0);
        return numbers;
    }

    // True if candidate is a later version than current. Anything unreadable counts as not newer.
    inline bool isNewer (const juce::String& candidate, const juce::String& current)
    {
        const auto a = parseVersion (candidate);
        const auto b = parseVersion (current);
        if (a.empty() || b.empty())
            return false;
        return std::lexicographical_compare (b.begin(), b.end(), a.begin(), a.end());
    }

    // The later of two versions (an unreadable one loses).
    inline juce::String later (const juce::String& a, const juce::String& b)
    {
        if (parseVersion (a).empty()) return b;
        return isNewer (b, a) ? b : a;
    }

    struct Manifest
    {
        juce::String version; // e.g. "1.2.0"
        juce::String url;     // the update zip
        juce::String sha256;  // of the update zip, 64 lowercase hex digits
    };

    // Reads update.json: {"version": "1.2.0", "url": "https://github.com/...zip", "sha256": "..."}.
    // Returns nothing if anything is missing or looks wrong, so a bad file never leads to a download.
    inline std::optional<Manifest> parseManifest (const juce::String& json)
    {
        const auto parsed = juce::JSON::parse (json);
        if (! parsed.isObject())
            return std::nullopt;

        Manifest m;
        m.version = parsed.getProperty ("version", {}).toString().trim();
        m.url = parsed.getProperty ("url", {}).toString().trim();
        m.sha256 = parsed.getProperty ("sha256", {}).toString().trim().toLowerCase();

        if (parseVersion (m.version).empty())
            return std::nullopt;
        if (! m.url.startsWithIgnoreCase (allowedDownloadPrefix) || ! m.url.endsWithIgnoreCase (".zip")
            || m.url.contains ("..") || m.url.containsAnyOf (" \"'<>\\"))
            return std::nullopt;
        if (m.sha256.length() != 64 || ! m.sha256.containsOnly ("0123456789abcdef"))
            return std::nullopt;

        return m;
    }
}
