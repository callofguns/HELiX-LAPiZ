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

## Release notes

Whenever a change bumps the version, write release notes for it and give them to the owner in
chat, ready to paste into the GitHub release. Put the same notes in the pull request description.
If several pull requests share one version, the notes cover all of them.

The notes also cover the website (`docs/index.html`, live at
https://callofguns.github.io/HELiX-LAPiZ/): list what changed on it since the last release, even
in pull requests that didn't bump the version, and always end with a link to it. To find those
changes, compare `docs/` with the previous release tag (`git diff <last tag> -- docs`).

Write them for people who use LAPiZ, not for developers: what changed and why it matters to them,
in plain words, with no file names or code. Use this shape:

```markdown
## LAPiZ v1.2.1: <a few words on what it's about>

<One or two sentences: what this release is.>

### What's new        (or "What's fixed" for a release with only fixes)
- **<Change>.** <What it means for the user.>

### On the website
- **<Change>.** <What visitors will see.>

### Updating
If you have LAPiZ 1.2 or later, it updates itself: open LAPiZ and restart FL Studio when the bar
at the bottom says so. On 1.1 or older? Download **LAPiZ-<version>-setup.exe** below and run it.

More about LAPiZ: https://callofguns.github.io/HELiX-LAPiZ/
```

Leave out sections that would be empty, and keep a fix release short.
