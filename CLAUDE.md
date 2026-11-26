# LAPiZ

A wavetable synth for FL Studio (VST3, Windows) built with JUCE. See README.md for building,
testing and releasing.

## Version numbers

Every change that reaches users bumps the version. Pick the part to bump by the size of the change:

- **Small changes bump the third number** (1.2.0 → 1.2.1 → 1.2.2): bug fixes, colour or contrast
  tweaks, wording, small UI polish, preset volume fixes.
- **New features bump the second number and reset the third** (1.2.3 → 1.3.0): a new control,
  effect, tab, setting or batch of presets.
- The first number only changes if the owner asks.

If a pull request already bumps the version and hasn't been released yet, don't bump it again:
several small changes can share one patch version.

Bump it in all of these places, in the same pull request as the change:

| File | 1.2.1 looks like | 1.3.0 looks like |
| --- | --- | --- |
| `CMakeLists.txt`: `project(Lapiz VERSION ...)` | `1.2.1` | `1.3.0` |
| `Source/Version.h`: `versionTag` | `v1.2.1` | `v1.3` |
| `installer/LAPiZ.iss`: default `AppVersion` | `1.2.1` | `1.3.0` |
| `docs/index.html`: the version in the hero and the setup file name in the install steps | `1.2.1`, `LAPiZ-1.2.1-setup.exe` | `1.3`, `LAPiZ-1.3.0-setup.exe` |

`versionTag` drops a trailing `.0` (`v1.3`, not `v1.3.0`); the release workflow checks this.

The release tag follows the same rule: `v1.2.1` for a small change, `v1.3` for a feature release.
The workflow (`.github/workflows/release.yml`) refuses to build a tag that doesn't match these files.
