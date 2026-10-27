# Helix

A wavetable synth for FL Studio (VST3, Windows), built to be powerful but easy for beginners:
plain-English controls, hover help on everything and live visual feedback.

## Building

Needs Visual Studio Build Tools and CMake. JUCE is a git submodule, so clone with:

    git clone --recurse-submodules <repo url>

Then:

    cmake -S . -B build
    cmake --build build --config Release

The plugin is copied to `Documents\VST3` after every build.
Run the tests with `build\HelixTests_artefacts\Release\HelixTests.exe` (exit code 0 means all passed).

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

- [JUCE](https://juce.com) 9.0.4, used under the AGPLv3 (JUCE is also available under a commercial licence).
- Atkinson Hyperlegible font (Braille Institute), under the SIL Open Font License 1.1
  (see `Resources/Fonts/OFL.txt`).
