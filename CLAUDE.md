# LAPiZ

A wavetable synth for FL Studio (VST3, Windows) built with JUCE. See README.md for building,
testing and releasing.

## Fonts

The owner wants Apple's SF Pro, with Inter wherever SF Pro can't be used. Apple's license only
allows SF Pro for apps on Apple platforms, so:

- **Plugin:** Inter (embedded, `Resources/Fonts/`), since LAPiZ is Windows-only.
- **Website:** body text uses the visitor's system font first (`-apple-system`, which is SF Pro on
  Apple devices), then Inter. Never serve SF Pro files from the site. The LAPiZ wordmark and
  headings keep Bricolage Grotesque.

## Spelling

The owner is in the US: use US spelling in everything people read, including the plugin's text,
the website, the README, release notes and pull requests ("color", "center", "favorite",
"behavior", "gray", "analog"). Code that calls JUCE keeps JUCE's names (`juce::Colour`,
`Colours::`), since those can't change.

## Version numbers

Every change that reaches users bumps the version. Pick the part to bump by the size of the change:

- **Small changes bump the third number** (1.2.0 → 1.2.1 → 1.2.2): bug fixes, colour or contrast
  tweaks, wording, small UI polish, preset volume fixes.
- **New features bump the second number and reset the third** (1.2.3 → 1.3.0): a new control,
  effect, tab, setting or batch of presets.
- The first number only changes if the owner asks.

If a pull request already bumps the version and hasn't been released yet, don't bump it again:
several small changes can share one patch version.

Bump it in these places, in the same pull request as the change:

| File | 1.2.1 looks like | 1.3.0 looks like |
| --- | --- | --- |
| `CMakeLists.txt`: `project(Lapiz VERSION ...)` | `1.2.1` | `1.3.0` |
| `Source/Version.h`: `versionTag` | `v1.2.1` | `v1.3` |
| `installer/LAPiZ.iss`: default `AppVersion` | `1.2.1` | `1.3.0` |

The website is the exception: see "The website waits for the release" below.

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

## The website waits for the release

The website (`docs/`) goes live as soon as a change reaches master, so it must always describe the
latest **published** release, never an unreleased version. Feature and fix pull requests don't
touch `docs/`: no new version number, no setup file name, no mention of an unreleased feature.

When the owner decides to release, make one small **release prep** pull request first, with all
the website changes for that release:

- the version in the hero (`1.3`) and the setup file name in the install steps (`LAPiZ-1.3.0-setup.exe`);
- any text, screenshots or sections about what the release adds or changes.
- the tab screenshots in `docs/images/`, redrawn from the release's code so they show what people
  will get. Build `LapizTests` and run, for each tab:
  `LapizTests --window docs/images/<tab>.png <tab> 2x` (tabs: simple, waves, advanced, effects;
  add `daylight` for the light look). They're 2x pictures of the 900x644 window, so the `<img>`
  in `docs/index.html` keeps `width="900" height="644"`.

The owner merges it and then creates the GitHub release. Website-only changes that aren't about an
unreleased version (a typo, a layout fix, accessibility) can go out any time.

## Releasing: ask first

After every change that bumps the version, ask the owner whether to publish it as a release now or
wait and collect more updates first. Don't assume either way, and ask again for the next change.

- **Release now:** make the release prep pull request (above). Once it's merged, the owner
  creates the GitHub release with the tag (Claude can't push tags from cloud sessions) and pastes the release notes. Offer to check the
  build afterwards.
- **Wait:** the next change keeps adding to the same unreleased version where the rules above allow
  (small changes share it; a new feature moves it to the next second number), and the release notes
  grow to cover everything since the last release.
