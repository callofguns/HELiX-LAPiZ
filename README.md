# LAPiZ

A wavetable synth by HELiX for FL Studio (VST3, Windows), built to be powerful but easy for beginners:
plain-English controls, hover help on everything and live visual feedback.

- **Simple** tab: pick a wavetable, shape the tone with a filter, add analog drift and warmth with one knob, and set how notes start and end.
- **Waves** tab: draw your own wavetables with the mouse, blend between keyframes, save them as .wav.
- **Advanced** tab: filter envelope, an LFO ("Wobble") and unison ("Thickness": up to 8 detuned copies per note).
- **Effects** tab: distortion, chorus, ping-pong delay and reverb.
- **Presets**: 102 built-in sounds, soft and magical, mostly pads and arps; save or load your own (Documents\LAPiZ\Presets).

## Installing

Run `LAPiZ-<version>-setup.exe`. It puts the plugin in `C:\Program Files\Common Files\VST3`
(and, if you choose, the standalone app in `Program Files\HELiX\LAPiZ`).
Then in FL Studio open **Options > Manage plugins**, click **Find installed plugins**, and star LAPiZ.

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
