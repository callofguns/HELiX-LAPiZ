import { Bar, Menu, Popover, Roll, Stage, type Act } from '../../kit'
import './lapiz.css'

// LAPiZ, a wavetable synth plugin: "Surprise me" and favorites, for the presets section of
// its website. Corner bleed: the plugin window at 1.3x, inset top and left, running off the
// right and bottom, so the preset bar and the Sound panel's wave are the picture.
//
// Story (one loop, ~12 s): the die makes a surprise sound (the name rolls, the wave and the
// wavetable change, the star greys out because an unsaved sound can't be a favorite), the die
// again, then the preset list opens with Favorites on top and the starred Moonlit Haze comes
// back. The loop ends where it began, on Moonlit Haze with its star filled.
//
// Real strings: LAPiZ, the preset names, Save, Sound, the wavetable names, Favorites,
// All sounds, Init. Everything else is the product's own shapes or bars.

type Sound = { name: string; table: string; wave: number; knobs: [number, number, number] }

const moonlit: Sound = { name: 'Moonlit Haze', table: 'Soft Saw', wave: 0, knobs: [0.45, 0.2, 0.6] }
const fireflies: Sound = { name: 'Crystal Fireflies', table: 'Basic Shapes', wave: 1, knobs: [0.7, 0.05, 0.55] }
const bloom: Sound = { name: 'Velvet Bloom', table: 'Wavefold', wave: 2, knobs: [0.25, 0.35, 0.5] }

type S = Sound & { starred: boolean; canStar: boolean; menu: boolean; hi: number }

const initial: S = { ...moonlit, starred: true, canStar: true, menu: false, hi: -1 }
// The poster tells the story alone: a surprise on screen, the list open with the favorite on top.
const poster: S = { ...bloom, starred: false, canStar: false, menu: true, hi: 1 }

async function script(a: Act<S>) {
  await a.wait(900)
  a.mark('die')
  await a.click({ at: 'die', x: 0.5, y: 0.5 })
  a.put({ ...fireflies, starred: false, canStar: false })
  await a.wait(1700)
  await a.click({ at: 'die', x: 0.5, y: 0.5 })
  a.put({ ...bloom })
  await a.wait(1700)
  a.mark('menu')
  await a.click({ at: 'name', x: 0.92, y: 0.62 })
  a.set({ menu: true })
  await a.wait(500)
  await a.move({ at: 'fav', x: 0.35, y: 0.5 })
  a.set({ hi: 1 })
  await a.wait(350)
  await a.click({ at: 'fav', x: 0.35, y: 0.5 })
  a.set({ ...moonlit, starred: true, canStar: true })
  a.set({ menu: false, hi: -1 })
  a.mark('payoff')
  // Back to where it rested at the start, off the payoff, so the loop joins up
  await a.move(a.layout === 'compact' ? { x: 300, y: 300 } : { x: 600, y: 330 })
  await a.wait(900)
}

// One period of each sound's wave, as points across the display.
const shapes: ((p: number) => number)[] = [
  (p) => 0.75 * Math.sin(2 * Math.PI * p) + 0.18 * Math.sin(4 * Math.PI * p), // soft saw
  (p) => (p < 0.25 ? 4 * p : p < 0.75 ? 2 - 4 * p : 4 * p - 4) * 0.85, // triangle
  (p) => 0.8 * Math.sin(2 * Math.PI * p + 1.1 * Math.sin(2 * Math.PI * p)), // wavefold
]

function wavePath(i: number, w: number, h: number) {
  const pts: string[] = []
  for (let k = 0; k <= 48; k++) {
    const p = k / 48
    pts.push(`${k === 0 ? 'M' : 'L'}${(p * w).toFixed(1)},${(h / 2 - shapes[i](p) * h * 0.38).toFixed(1)}`)
  }
  return pts.join(' ')
}

function Knob({ v }: { v: number }) {
  // A 270-degree track and the value arc in the Sound teal, like the plugin's knobs.
  const r = 17, c = 2 * Math.PI * r, track = c * 0.75
  return (
    <svg className="lz-knob" width="44" height="44" viewBox="0 0 44 44" aria-hidden>
      <circle cx="22" cy="22" r={r} className="lz-knob__track" strokeDasharray={`${track} ${c}`} transform="rotate(135 22 22)" />
      <circle cx="22" cy="22" r={r} className="lz-knob__value" strokeDasharray={`${track * v} ${c}`} transform="rotate(135 22 22)" />
    </svg>
  )
}

const Chevron = ({ dir }: { dir: 'left' | 'right' }) => (
  <svg width="14" height="14" viewBox="0 0 14 14" aria-hidden><path d={dir === 'left' ? 'M8.5 3 L4.5 7 L8.5 11' : 'M5.5 3 L9.5 7 L5.5 11'} /></svg>
)
const Star = ({ filled }: { filled: boolean }) => (
  <svg width="16" height="16" viewBox="0 0 16 16" aria-hidden data-filled={filled || undefined}>
    <path d="M8 1.8 L9.7 5.9 L14.1 6.2 L10.7 9 L11.8 13.3 L8 11 L4.2 13.3 L5.3 9 L1.9 6.2 L6.3 5.9 Z" />
  </svg>
)
const Die = () => (
  <svg width="16" height="16" viewBox="0 0 16 16" aria-hidden>
    <rect x="2.25" y="2.25" width="11.5" height="11.5" rx="2.5" />
    {[[5.3, 5.3], [10.7, 5.3], [8, 8], [5.3, 10.7], [10.7, 10.7]].map(([x, y], i) => <circle key={i} cx={x} cy={y} r="1.15" className="lz-pip" />)}
  </svg>
)

export default function LapizSurprise() {
  return (
    <Stage
      width={720}
      height={420}
      compact={{ below: 560, width: 358, height: 340 }}
      initial={initial}
      poster={poster}
      script={script}
      play="loop"
      theme="dark"
      cursor={{ at: { wide: { x: 600, y: 330 }, compact: { x: 300, y: 300 } }, style: 'large' }}
      className="lz"
    >
      {(s, _rect, layout) => (
        <div className="lz-scale" data-layout={layout}>
          <div className="lz-win">
            <div className="lz-header">
              <div className="lz-wordmark">LAPiZ</div>
              <div className="lz-presets">
                <div className="lz-icon"><Chevron dir="left" /></div>
                <div className="lz-name" data-target="name" data-cursor="pointer" data-open={s.menu || undefined}>
                  <Roll>{s.name}</Roll>
                </div>
                <div className="lz-icon"><Chevron dir="right" /></div>
                <div className="lz-icon lz-star" data-off={!s.canStar || undefined}><Star filled={s.starred} /></div>
                <div className="lz-icon" data-target="die" data-cursor="pointer"><Die /></div>
                <div className="lz-text-button">Save</div>
              </div>
            </div>

            <div className="lz-panels">
              <div className="lz-panel">
                <div className="lz-panel-title"><span className="lz-num">1</span>Sound</div>
                <div className="lz-picker">
                  <div className="lz-icon lz-icon--small"><Chevron dir="left" /></div>
                  <div className="lz-picker-name"><Roll variant="fade">{s.table}</Roll></div>
                  <div className="lz-icon lz-icon--small"><Chevron dir="right" /></div>
                </div>
                <div className="lz-wave">
                  <svg viewBox="0 0 240 118" preserveAspectRatio="none" aria-hidden>
                    {[0.25, 0.5, 0.75].map((y) => <line key={y} x1="0" x2="240" y1={118 * y} y2={118 * y} className="lz-grid" />)}
                    <path d={wavePath(s.wave, 240, 118)} className="lz-wave-line" />
                  </svg>
                </div>
              </div>

              <div className="lz-panel lz-panel--tone">
                <div className="lz-panel-title"><span className="lz-num lz-num--tone">2</span><Bar w={44} h={9} r={4.5} tone={2} /></div>
                <div className="lz-filter">
                  <svg viewBox="0 0 240 150" preserveAspectRatio="none" aria-hidden>
                    <path d="M0 60 H200 Q214 60 222 76 L240 120 V150 H0 Z" className="lz-filter-fill" />
                    <path d="M0 60 H200 Q214 60 222 76 L240 120" className="lz-filter-line" />
                  </svg>
                </div>
              </div>
            </div>
          </div>

          <Popover show={s.menu} origin="top left" className="lz-menu-pop">
            <Menu
              className="lz-menu"
              value={s.hi}
              targets={[undefined, 'fav', undefined, undefined, undefined, undefined]}
              items={[
                <span className="lz-menu-head">Favorites</span>,
                <span className="lz-menu-row"><span className="lz-tick"><Star filled /></span>Moonlit Haze</span>,
                <span className="lz-menu-head">All sounds</span>,
                <span className="lz-menu-row">Init</span>,
                <span className="lz-menu-row"><Bar w={34} h={7} r={3.5} tone={3} /><span className="lz-sub">›</span></span>,
                <span className="lz-menu-row"><Bar w={26} h={7} r={3.5} tone={3} /><span className="lz-sub">›</span></span>,
              ]}
            />
          </Popover>
        </div>
      )}
    </Stage>
  )
}
