# ZBOT-MOBILE

Mobile-friendly Geometry Dash macro mod, built as a Geode SDK mod.

> ⚠️ This project is a **fork / port**, not an original mod. The
> recording / playback core is derived from
> [FigmentBoy's **zBot**](https://github.com/FigmentBoy/zBot), and the
> speedhack plus several smaller behaviours are referenced from
> [zilko's **xdBot**](https://github.com/ZiLko/xdBot). See
> [CREDITS.md](./CREDITS.md) for the full per-file attribution and a
> note on the upstream license.

## Status

Work in progress. Current version: **v1.5.7** (see `mod.json`).

See [CHANGELOG.md](./CHANGELOG.md) for the full release notes.

## Building

This is a C++ Geode mod, built with the Geode CLI + Geode SDK. The
repository ships an Android CI pipeline at
`.github/workflows/build.yml`. Each push to `main` builds Android32 +
Android64 `.geode` files and publishes a GitHub Release tagged with the
version from `mod.json` (e.g. `v1.5.6`).

## Credits

Please read [CREDITS.md](./CREDITS.md). The short version:

- Recording / playback core derived from **FigmentBoy's zBot**.
- Speedhack + extra behaviour referenced from **zilko's xdBot**.
- Original canonical playback semantics by **matcool / ReplayBot**.
- Theme / layout inspired by **EclipseMenu**.
- Libraries: Geode SDK, **GDReplayFormat** by maxnut, **gd-imgui-cocos**
  by matcool.

Mobile port and maintenance: **pepitogumball**.
