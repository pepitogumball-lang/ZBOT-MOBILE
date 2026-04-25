# Credits

ZBOT-MOBILE is **not an original work**. It is a mobile-focused port and
extension that combines code, design, and ideas from several existing
Geometry Dash community projects. The two upstream macro mods this fork
draws from are credited below in the most concrete terms possible so
that authorship is unambiguous.

## Upstream macro mods

### FigmentBoy's **zBot** — recording / playback core
- Upstream: https://github.com/FigmentBoy/zBot
- The recording and playback model, the `zReplay` data layout in
  `src/replay.hpp`, the `zState` enum, the recordmanager / playbackmanager
  split, the `requestPlaybackRestart()` lifecycle, and the
  *record raw `m_currentProgress` / fire on `frame <= currentProgress` /
  restart cursor on every `resetLevel`* playback semantics are all
  derived from FigmentBoy's zBot.
- The project name **ZBOT-MOBILE** is a direct reference to this
  upstream and is used here as a respectful "mobile port of" label, not
  to imply endorsement.

### zilko's **xdBot** — speedhack + additional behaviour
- Upstream: https://github.com/ZiLko/xdBot
- The clock-style speedhack approach (delta-time scaling + matching
  FMOD pitch hook) in `src/speedhack.cpp` and `src/speedhackaudio.cpp`,
  and several smaller UX behaviours, were referenced from zilko's
  xdBot.

Both **FigmentBoy** and **zilko** are listed as co-developers in
`mod.json`. ZBOT-MOBILE would not exist without their work.

## Inspiration / reference implementations

These mods were used as references for specific behaviours and are
credited because their mechanics directly informed equivalent code in
this fork:

- **matcool / ReplayBot** — the original canonical playback semantics
  (record raw `m_currentProgress`, fire on `frame <= currentProgress`,
  restart cursor on every `resetLevel`) that FigmentBoy's zBot also
  implements and that this fork inherits.
- **EclipseMenu** — the dark/violet ImGui theme, tabbed layout and
  saved-macros search filter style.

## Libraries

Pulled in via CPM in `CMakeLists.txt`:

- **Geode SDK** — https://github.com/geode-sdk/geode
- **GDReplayFormat** by **maxnut** —
  https://github.com/maxnut/GDReplayFormat
- **gd-imgui-cocos** by **matcool** —
  https://github.com/matcool/gd-imgui-cocos

## What is original to ZBOT-MOBILE

- The mobile-friendly always-visible draggable floating ball UI that
  replaces the keyboard-driven panel toggle.
- Touch-first tab layout sized for small screens.
- The "perfect run only" auto-save gate with checkpoint-based purge.
- Spam-safe macro normalization (`minHoldFrames`, `minGapFrames`).
- Built-in autoclicker / spammer with CPS + hold-ratio sliders, with
  an opt-in record-to-macro flag.
- Persistent settings via Geode's per-mod save store.
- The in-game GitHub Releases update check in `src/updatecheck.cpp`.
- Android32 / Android64 CI build pipeline in
  `.github/workflows/build.yml`.

## Maintainer

- **pepitogumball** — mobile port and ongoing maintenance.

## License / reuse

This mod reuses code derived from FigmentBoy's zBot and references
behaviour from zilko's xdBot. **The upstream license for FigmentBoy's
zBot is the Aseprite EULA**, which is a restrictive non-FOSS license —
anyone redistributing a build of ZBOT-MOBILE should review that
license themselves before doing so, and any objection from the
upstream authors will be honoured.

If FigmentBoy, zilko, or any of the authors listed above object to
this fork or to the way their work is credited, please open an issue
at https://github.com/pepitogumball-lang/ZBOT-MOBILE/issues and the
mod will be updated, re-licensed, or taken down accordingly.
