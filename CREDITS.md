# Credits

ZBOT-MOBILE is **not an original work**. It is a mobile-focused port and
extension of an existing Geode mod, and the bulk of the design and many
core ideas come from other community projects. Full attribution below.

## Original mod

- **zBot** by **zilko** — the original macro / replay tool that this
  project is derived from. The recording and playback model, the
  `zReplay` data layout in `src/replay.hpp`, the `zState` enum, the
  recordmanager / playbackmanager split and the speedhack approach all
  come from zilko's zBot.
  Upstream: https://github.com/zilko-x/zBot

ZBOT-MOBILE would not exist without zilko's work. zilko is listed as a
co-developer in `mod.json` and credited here.

## Inspiration / reference implementations

These mods were used as references for specific behaviours and are
credited because their mechanics directly informed equivalent code in
this fork:

- **matcool / ReplayBot** — canonical playback semantics (record raw
  `m_currentProgress`, fire on `frame <= currentProgress`, restart cursor
  on every `resetLevel`).
- **FigmentBoy / zBot** — same playback semantics, used as a
  cross-reference.
- **EclipseMenu** — the dark/violet ImGui theme, tabbed layout and
  saved-macros search filter style.
- **xdBot** / **EclipseMenu** — clock-style speedhack approach
  (delta-time scaling + matching FMOD pitch hook).

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
- Android32 / Android64 CI build pipeline in
  `.github/workflows/build.yml`.

## Maintainer

- **pepitogumball** — mobile port and ongoing maintenance.

## License / reuse

This mod reuses code from zilko's zBot. If zilko or any of the authors
listed above object to this fork or the way their work is credited,
please open an issue on the upstream repository
(https://github.com/pepitogumball-lang/ZBOT-MOBILE) and the mod will be
updated or taken down accordingly.
