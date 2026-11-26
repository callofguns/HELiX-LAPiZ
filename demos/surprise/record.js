// Records one loop of the stage as timestamped frames (CDP screencast), then cuts it with ffmpeg.
const { chromium } = require('playwright');
const fs = require('fs'); const path = require('path'); const { execFileSync } = require('child_process');
const [,, name, width, outDir, zoomArg] = process.argv; const Z = +(zoomArg || 2);
(async () => {
  const b = await chromium.launch();
  const p = await b.newPage({ viewport: { width: Math.round(+width * Z), height: Math.round(900 * Z) }, deviceScaleFactor: 1 });
  await p.addInitScript((z) => { window.__Z = z; }, Z);
  await p.addInitScript(() => { document.addEventListener('DOMContentLoaded', () => { document.documentElement.style.zoom = String(window.__Z); }); });
  const marks = [];
  await p.exposeFunction('__mark', (m, t) => marks.push([m, t]));
  await p.addInitScript(() => {
    new MutationObserver(() => { const s = document.querySelector('.ss-stage'); if (s && s.dataset.mark && s.dataset.mark !== window.__last) { window.__last = s.dataset.mark; window.__mark(s.dataset.mark, Date.now()); } })
      .observe(document, { subtree: true, attributes: true, attributeFilter: ['data-mark'], childList: true });
  });
  await p.goto(`http://127.0.0.1:5179/?capture=${name}`);
  await p.waitForSelector('.ss-stage'); await p.waitForTimeout(500);
  const r = await p.evaluate(() => { const e = document.querySelector('.ss-stage').getBoundingClientRect(); return { x: e.x, y: e.y, w: e.width, h: e.height, zoom: getComputedStyle(document.documentElement).zoom }; });
  const cdp = await p.context().newCDPSession(p);
  const frames = []; const dir = path.join(outDir, 'frames'); fs.rmSync(dir, { recursive: true, force: true }); fs.mkdirSync(dir, { recursive: true });
  cdp.on('Page.screencastFrame', async (f) => { const file = path.join(dir, String(frames.length).padStart(5, '0') + '.jpg'); fs.writeFileSync(file, Buffer.from(f.data, 'base64')); frames.push([file, f.metadata.timestamp * 1000]); cdp.send('Page.screencastFrameAck', { sessionId: f.sessionId }).catch(() => {}); });
  await cdp.send('Page.startScreencast', { format: 'jpeg', quality: 95, everyNthFrame: 1 });
  await p.waitForTimeout(31000);
  await cdp.send('Page.stopScreencast'); await b.close();
  const dies = marks.filter(m => m[0] === 'die').map(m => m[1]);
  const start = dies[1] - 900, end = dies[2] - 900;
  console.log('marks', marks.length, 'loop ms', end - start, 'frames', frames.length, 'stage', JSON.stringify(r));
  // concat list with each frame's duration, inside [start, end]
  frames.sort((x, y) => x[1] - y[1]);
  // Frames only arrive when something changes, so start from the last frame before the loop
  let first = 0; frames.forEach((f, i) => { if (f[1] <= start) first = i; });
  const inLoop = frames.slice(first).filter((f) => f[1] <= end);
  let list = '';
  for (let i = 0; i < inLoop.length; i++) {
    const t0 = Math.max(inLoop[i][1], start), t1 = i + 1 < inLoop.length ? inLoop[i + 1][1] : end;
    if (t1 <= t0) continue;
    list += `file '${inLoop[i][0]}'\nduration ${((t1 - t0) / 1000).toFixed(4)}\n`;
  }
  list += `file '${inLoop[inLoop.length - 1][0]}'\n`;
  fs.writeFileSync(path.join(outDir, 'list.txt'), list);
  const crop = `crop=${Math.round(r.w)}:${Math.round(r.h)}:${Math.round(r.x)}:${Math.round(r.y)}`;
  fs.writeFileSync(path.join(outDir, 'crop.txt'), crop);
})();
