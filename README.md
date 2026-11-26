# LAPiZ

A wavetable synth by HELiX for FL Studio (VST3, Windows), built to be powerful but easy for beginners:
plain-English controls, hover help on everything and live visual feedback.

- **Simple** tab: pick a wavetable, shape the tone with a filter, add analog drift and warmth with one knob, and set how notes start and end.
- **Waves** tab: draw your own wavetables with the mouse, blend between keyframes, save them as .wav.
- **Advanced** tab: filter envelope, an LFO ("Wobble") and unison ("Thickness": up to 8 detuned copies per note).
- **Effects** tab: distortion, chorus, ping-pong delay and reverb.
- **Piano**: a keyboard along the bottom of the window. Click or drag across the keys to play without a MIDI keyboard; notes from FL Studio light up on it. Middle C is C5, as in FL Studio.
- **Presets**: 102 built-in sounds, soft and magical, mostly pads and arps; save or load your own (Documents\LAPiZ\Presets).

## Installing

Run `LAPiZ-<version>-setup.exe`. It puts the plugin in `C:\Program Files\Common Files\VST3`
(and, if you choose, the standalone app in `Program Files\HELiX\LAPiZ`).
Then in FL Studio open **Options > Manage plugins**, click **Find installed plugins**, and star LAPiZ.

### Updates

From v1.2 on, LAPiZ keeps itself up to date. When you open its window, it checks GitHub for a new
release (at most every 6 hours). If one exists, it downloads it, checks the file's SHA-256, and
installs it in the background. The footer then says "Updated to vX. Restart FL Studio to use it."
You can turn this off, or check straight away, in the settings (gear) menu.

Updating never needs an administrator prompt: the installer lets normal users change the two
LAPiZ folders (the plugin in `Common Files\VST3\LAPiZ.vst3` and the app folder).
Installs of v1.1 or older have to install v1.2 by hand once.

## Building

Needs Visual Studio Build Tools and CMake. JUCE is a git submodule, so clone with:

    git clone --recurse-submodules <repo url>

Then:

    cmake -S . -B build
    cmake --build build --config Release

The plugin is copied to `Documents\VST3` after every build (FL Studio must be closed while building).
Run the tests with `build\LapizTests_artefacts\Release\LapizTests.exe` (exit code 0 means all passed).

To make the installer, install [Inno Setup 6](https://jrsoftware.org/isinfo.php), build in Release, then run:

    "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\LAPiZ.iss

The installer is written to `dist\`.

Builds on your own computer don't update themselves. Only release builds do, which are made with
`-DLAPIZ_AUTO_UPDATE=ON`. Add `-DLAPIZ_COPY_AFTER_BUILD=OFF` to skip copying to `Documents\VST3`.

## Releasing a new version

1. Bump the version: the third number for small changes and fixes (`1.2.0` → `1.2.1`), the
   second for new features (`1.2.1` → `1.3.0`). It goes in `CMakeLists.txt`, `Source/Version.h`,
   `installer/LAPiZ.iss` and the website; [CLAUDE.md](CLAUDE.md) lists exactly what to change.
2. Commit and push, then either publish a release on GitHub with a matching tag, such as `v1.2.1`
   or `v1.3` (Releases > Draft a new release), or push the tag yourself:

       git tag v1.2.1
       git push origin v1.2.1

GitHub Actions (`.github/workflows/release.yml`) then builds and tests LAPiZ, makes the installer
and the update zip, and adds them to the GitHub Release along with an `update.json`. Every
installed copy of LAPiZ picks it up from there. If a run fails, fix the problem on master, then
rebuild the same tag with **Actions > Release > Run workflow**.

## License

Copyright (C) 2026 Dhruv

This program is free software: you can redistribute it and/or modify it under the terms of the
GNU Affero General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program
(see [LICENSE](LICENSE)). If not, see <https://www.gnu.org/licenses/>.

### Third-party code and fonts

LAPiZ is built with [JUCE](https://juce.com) 9.0.4, used under the AGPLv3 (JUCE is also available
under a commercial licence), and includes the Atkinson Hyperlegible font (SIL Open Font License 1.1)
and several permissively licensed libraries that come with JUCE (zlib, HarfBuzz, libpng, FLAC,
the VST3 SDK and others). Their licences are reproduced in full in
[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt), which the installer also copies next to the app.

VST is a registered trademark of Steinberg Media Technologies GmbH.
