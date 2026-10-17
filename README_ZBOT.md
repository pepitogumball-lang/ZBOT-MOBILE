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

## Credits

This project is maintained by **pepitogumball-lang**. Special thanks to the Geode SDK team and the creators of the GDR format.

---

*Developed by pepitogumball-lang.*
