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
  port 5000. No mod source code was modified.
