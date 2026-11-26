# Rehearsal: LAPiZ "Surprise me" and favorites

The looping demo in the website's presets section (`docs/images/surprise-demo.mp4`, the phone
version `surprise-demo-phone.mp4`, and a `.jpg` poster for each). Built with the
[stageshow](https://github.com/benjitaylor/skills/tree/main/stageshow) skill (MIT) from the
plugin's own colors, font and icons.

Gold: `close-up` and `hero-demo` (kept: corner bleed, the large cursor, a popover anchored to its
trigger, beats with named targets; changed: the plugin's own header stands in for window chrome,
since LAPiZ is a Windows plugin with no macOS title bar).
Beats: die → "Crystal Fireflies" (wave and wavetable change, star greys out) … die → "Velvet
Bloom" … preset list opens, Favorites on top … "Moonlit Haze" picked, star filled (payoff);
loop 8.8 s, ends where it began.
Captured: 1280 (wide, 720 × 420) and 390 (compact, 358 × 340); dark only (the plugin's look);
reduced motion shows the poster (menu open, favorite highlighted).
Gate: `rehearsal passed` at both widths.
Film: `layout: div.lz-panel leaves -152px below its last child` stands: the panels run off the
window's foot on purpose (corner bleed).
Assumptions: the surprise names and their wavetables are illustrative (real ones are random);
the menu shows two category rows as bars.
Not checked: Safari playback of the MP4 (H.264, should be fine).

## Remaking it

1. Copy `index.tsx` and `lapiz.css` into the stageshow skill as `examples/lapiz-surprise/`, with
   `Resources/Fonts/Inter-Regular.ttf` and `Inter-SemiBold.ttf` beside them.
2. In the skill: `npm install`, then `npx vite playground --config playground/vite.config.ts --port 5179`.
3. Check it: `node scripts/film.mjs "http://127.0.0.1:5179/?capture=lapiz-surprise" --select '#example' --every 500 --count 28 --gate --out film.png`
   (and again with `--width 390`).
4. Record one loop: `node record.js lapiz-surprise 1280 wide 1.5` and `node record.js lapiz-surprise 390 compact 2`
   (Playwright), then `ffmpeg -f concat -safe 0 -i <dir>/list.txt -vf "$(cat <dir>/crop.txt),fps=30,format=yuv420p" -c:v libx264 -crf 20 -movflags +faststart -an out.mp4`.
