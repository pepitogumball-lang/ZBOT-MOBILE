# ZBOT-MOBILE

Mobile-friendly port of **zBot** (originally by **zilko**) for Geometry
Dash, built as a Geode SDK mod.

> ⚠️ This project is a **fork / port**, not an original mod. The
> recording / playback core comes from zilko's zBot. See
> [CREDITS.md](./CREDITS.md) for full attribution to zilko and to every
> other project whose work is reused or referenced here.

## Status

Work in progress. Current version: **v1.5.2** (see `mod.json`).

## Building

This is a C++ Geode mod, built with the Geode CLI + Geode SDK. The
repository ships an Android CI pipeline at
`.github/workflows/build.yml`. Each push to `main` builds Android32 +
Android64 `.geode` files and publishes a GitHub Release tagged with the
version from `mod.json` (e.g. `v1.5.2`).

## Credits

Please read [CREDITS.md](./CREDITS.md). The short version:

- Original **zBot** by [zilko](https://github.com/zilko-x/zBot).
- Playback semantics referenced from **matcool / ReplayBot** and
  **FigmentBoy / zBot**.
- Theme / layout inspired by **EclipseMenu**.
- Libraries: Geode SDK, **GDReplayFormat** by maxnut, **gd-imgui-cocos**
  by matcool.

Mobile port and maintenance: **pepitogumball**.
