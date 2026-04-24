# ZBOT-MOBILE

A Geode SDK mod for Geometry Dash (Android & Windows) — work in progress.

## What this project is

This is a **C++ Geode SDK mod** for the rhythm game *Geometry Dash*. It compiles
into a `.geode` file (a shared library plus assets) that the game loads at
runtime. It is **not** a web application — there is no server, API, or browser
UI in the project itself.

- Manifest: `mod.json` (id `pepitogumball.zbot_mobile`)
- Build system: CMake (>= 3.25), C++23
- Dependencies fetched via CPM:
  - [Geode SDK](https://github.com/geode-sdk/geode) (via `GEODE_SDK` env var)
  - [GDReplayFormat](https://github.com/maxnut/GDReplayFormat)
  - [gd-imgui-cocos](https://github.com/matcool/gd-imgui-cocos)
- CI: `.github/workflows/build.yml` builds Android32/Android64 `.geode` artifacts
  on push to `main` using the `geode-sdk/build-geode-mod` action.

## Project layout

```
src/                       C++ sources (main, gui, recording, playback, speedhack)
resources/                 packaged assets (logo.png)
CMakeLists.txt             CMake build configuration
mod.json                   Geode mod manifest
.github/workflows/         GitHub Actions CI for Android builds
web/index.html             Replit info page (see "Replit setup" below)
serve.py                   Tiny static HTTP server for the info page
```

## Replit setup

Because the actual mod cannot be compiled or executed inside Replit (it
requires the Geode SDK, Geode CLI, and the Android NDK — multi-gigabyte
toolchains — and ultimately needs Geometry Dash itself to load it), the
Replit workflow serves a static **project info page** on port 5000 that
explains what the project is and how to build/install it.

- Workflow: `Start application` → `python3 serve.py`
- Port: `5000` (webview), bound to `0.0.0.0`
- Caching disabled in dev so edits are visible immediately

## Building the mod (outside Replit)

Locally, with the Geode toolchain installed:

```bash
geode sdk install
geode build               # host platform
geode build -p android64  # Android target
```

Or rely on the GitHub Actions workflow, which builds and publishes a release
on every push to `main`. Drop the produced `.geode` file into:

- Android: `/storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/`
- Windows: `%LocalAppData%\GeometryDash\geode\mods\`

## Recent changes

- 2026-04-24: Imported repo into Replit. Added `web/index.html` and `serve.py`
  to satisfy the Replit workflow requirement with a static info page on
  port 5000.
- 2026-04-24: v1.1.0 mod menu overhaul.
  - `src/gui.hpp` / `src/gui.cpp`: replaced the "Show ZBOT" rectangle with a
    draggable circular floating ball (tap to toggle, drag to move, with a
    state dot showing record/playback). Added a big X close button. Added a
    saved-macros browser (list, load, play, delete with confirm) and a
    clock-based speedhack section with preset buttons (0, 0.1, 0.25, 0.5,
    1x, 2x, 3x, 4x) plus free-form numeric input. Speed `0` means "no
    speedhack effect" (matches xdBot semantics).
  - `src/replay.hpp`: added `zReplay::listSaved()` and
    `zReplay::deleteByName()` so the GUI can enumerate / remove `.gdr`
    macros on disk.
  - `mod.json`: bumped version to `v1.1.0`.
- 2026-04-24: v1.2.0 perfect-macro pass + EclipseMenu-style theme.
  - `src/zBot.hpp`: added `perfectRunOnly`, `autoSave`, `dedupeInputs`,
    `levelCompleted` flags and per-button held-state tracking
    (`p1ButtonHeld`, `p2ButtonHeld`) plus `resetButtonStateAfterFrame()`
    so dedupe stays consistent after a checkpoint rewind.
  - `src/replay.hpp`: added `zReplay::cleanInputs()` (stable-sort by
    frame, then drop no-op state toggles per (player, button)). `save()`
    now calls it so on-disk macros are always tidy.
  - `src/recordmanager.cpp`: record-time dedupe via `shouldRecord()`;
    cleaner respawn synth events; `levelComplete()` flips
    `levelCompleted` and auto-saves with a "Perfect macro saved"
    notification; `onExit()` honours the perfect-run gate so
    incomplete attempts no longer overwrite saved masterpieces.
  - `src/gui.hpp` / `src/gui.cpp`: rewrote the panel as a tabbed layout
    (Home / Macro / Speed / Settings), added an EclipseMenu-inspired
    dark+violet theme via `applyTheme()`, added a live status bar
    (state, input count, current speed), and added the new "Perfect
    run only", "Auto save" and "Dedupe inputs" toggles in the Macro
    tab.
  - `mod.json`: bumped version to `v1.2.0`.
- 2026-04-24: v1.3.0 persistent settings + macro metadata.
  - `src/zBot.hpp`: added `loadSettings()` / `saveSettings()` that
    round-trip every user-visible flag (`speedHackEnabled`,
    `speedHackAudio`, `speed`, `autoSafeMode`, `clickbotEnabled`,
    `autoSave`, `perfectRunOnly`, `dedupeInputs`) through Geode's
    per-mod save store (`Mod::get()->{get,set}SavedValue`). Defaults
    fall back to the in-class initialisers so first-time users still
    get the safe perfect-run-only setup.
  - `src/replay.hpp`: added `MacroFileInfo` struct + `listSavedDetailed()`
    so the saved-macros list can show file size and last-modified date
    without parsing each `.gdr`.
  - `src/gui.hpp` / `src/gui.cpp`: macro list now shows each macro's
    size + date next to the name. Settings tab, Speed tab, and Macro
    quality toggles all persist on change (batched via per-tab dirty
    flags so only one save call fires per frame). The floating ball's
    position is also persisted (saved on drag-release, restored at
    setup via `Mod::get()->getSavedValue<double>`). Header bumped to
    v1.3.0.
  - `mod.json`, `web/index.html`: bumped to v1.3.0 + updated copy.
