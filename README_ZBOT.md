# G-Macro Android (v1.8.0)

A macro utility for Geometry Dash on Android, providing frame-accurate recording and playback using the GDR format.

## Features

*   **Frame-Accurate Macros**: Precise input recording and playback synchronized with the game's physics engine.
*   **GDR Support**: Compatibility with the industry-standard GDR replay format (msgpack binary and JSON).
*   **Integrated Speedhack**: Adjust gameplay speed with audio pitch sync.
*   **Audio-Synced Clickbot**: Click sounds synchronized with the exact input frame during playback.
*   **Mobile Optimized UI**: Floating ball + panel interface designed for Android touch interaction.

## Requirements

*   **Geode**: v5.11.2 or later
*   **Geometry Dash**: 2.2081 (Android)

## Installation

1.  Ensure you have **Geode** v5.11.2+ installed on your Geometry Dash Android version.
2.  Download the latest `.geode` file from the [Releases](https://github.com/pepitogumball-lang/ZBOT-MOBILE/releases) section.
3.  Install via the Geode mod manager.

## Usage

*   **Record**: Activate Record mode, enter a level, and play. By default, only perfect runs (level completions) are saved. You can change this in the Macro tab.
*   **Playback**: Load a saved macro, select Replay, then restart the level to play it back.
*   **Speed**: Adjust game speed with the Speed tab. Speedhack also adjusts audio pitch.
*   **Spam**: Configurable autoclicker with player/button/CPS controls.
*   **Settings**: Customize safe mode, clickbot, and update check in the Settings tab.

## Macro Storage

Macros are saved as `.gdr` files (GDR format, msgpack-encoded) in:
`Mod::get()->getSaveDir() / "macros"`

## Building

The mod is built by the Geode SDK (CMake). Dependencies (`GDReplayFormat`,
`gd-imgui-cocos`) are pinned and fetched automatically by CPM:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

CI (`.github/workflows/build.yml`) builds for **Android32** and **Android64** on
every push and on manual dispatch (Actions → Build ZBOT-MOBILE → Run workflow).

## Tests

Pure macro logic (`src/macro_utils.hpp`) has no Geode dependency, so it is unit
tested in `tests/test_macro_utils.cpp`. The GDR serialization round-trip is
tested against the real GDReplayFormat library in `tests/test_gdr_roundtrip.cpp`:

```sh
# Pure logic
g++ -std=c++17 -Wall -Wextra -I src tests/test_macro_utils.cpp -o /tmp/test_macro
/tmp/test_macro

# GDR round-trip (needs GDReplayFormat headers)
git clone https://github.com/maxnut/GDReplayFormat /tmp/gdr
git -C /tmp/gdr checkout 9c32b5529067901418ea6e4dfbd36437dc8712bc
g++ -std=c++17 -Wall -Wextra -I /tmp/gdr/include tests/test_gdr_roundtrip.cpp -o /tmp/test_gdr
/tmp/test_gdr
```

CI runs both automatically in the **Unit tests** job.

## Validation status

* **Compilation**: verified in CI for Android32 + Android64.
* **Unit tests**: verified in CI (pure logic).
* **In-game behaviour**: **not yet verified on a physical Android device.** The
  frame semantics of recording vs. playback, checkpoints and restarts still need
  a manual pass — see `docs/MACRO_TEST_PLAN.md` for the checklist.

## Credits

This project is maintained by **pepitogumball-lang**. Special thanks to the Geode SDK team and the creators of the GDR format.

---

*Developed by pepitogumball-lang.*
