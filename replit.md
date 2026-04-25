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
- 2026-04-24: v1.4.0 spam-safe checkpoints, built-in spammer, menu visibility.
  - `src/zBot.hpp`: added `ZBOT_VERSION` constant, spam-safety spacing
    (`minHoldFrames`, `minGapFrames`), built-in spammer config
    (`spamEnabled`, `spamButton`, `spamCPS`, `spamHoldRatio`,
    `spamPlayer`, `spamOnlyDuringPlay`, `spamRecordToMacro`,
    `spamSuppressRecord`), and visibility toggles (`hideWhilePlaying`,
    `hideAfterFinish`, `onlyShowInMenu`, `hudHiddenAfterFinish`). All
    new fields are persisted via `saveSettings()` / `loadSettings()`.
  - `src/replay.hpp`: `cleanInputs()` and `save()` now take optional
    `minHoldFrames` / `minGapFrames` and run a third spacing pass that
    pushes events forward in time so a press is held for at least
    `minHold` frames and consecutive presses of the same button sit
    at least `minGap` frames apart. Stops GD from eating same-frame
    press+release tuples produced by extreme spam taps.
  - `src/recordmanager.cpp`: checkpoint reset now synthesises a
    release event for **every** still-held button per player (not just
    Jump), so a button held across a death no longer poisons the
    dedupe state on the next attempt. The handleButton hook honours
    the spam-suppress flag so spammer events stay out of macros by
    default. `levelComplete` flips `hudHiddenAfterFinish`; `init`
    clears it so re-entering a level brings the menu back.
  - `src/playbackmanager.cpp`: moved playback cursors (`currIndex`,
    `clickBotIndex`) onto Geode `$modify` Fields, with
    auto-resync when the replay pointer changes or the in-game frame
    goes backwards (death without `resetLevel` firing first). Added
    the built-in spammer in the same `processCommands` hook: it
    drives the chosen `PlayerButton` on a configurable press/release
    cycle anchored to the frame it was armed on, releases the button
    cleanly when disabled or paused, and bypasses recording unless
    the user explicitly opts in.
  - `src/gui.hpp` / `src/gui.cpp`: new "Spam" tab with button/player
    pickers, CPS slider + presets (1/5/10/15/20/30), hold-ratio
    slider, and "Only while playing" / "Record spam into macro"
    toggles. New "Menu visibility" group in Settings with "Hide while
    playing", "Hide after finishing a level" and "Only show in main
    menu". `GUI::shouldRenderHud()` checks `PlayLayer::get()` and
    `CCDirector::isPaused()` to gate the renderer on those flags.
    Added a violet dot on the floating ball when the spammer is armed.
    Added `MenuLayer::init` hook that clears `hudHiddenAfterFinish`
    on return to the main menu. Macro tab now exposes the
    `minHoldFrames` / `minGapFrames` spacing inputs. Header bumped to
    v1.4.0 (sourced from `ZBOT_VERSION`).
  - `mod.json`: bumped version to `v1.4.0`.
- 2026-04-24: v1.4.1 visibility/playback bugfixes inspired by EclipseMenu.
  - `src/playbackmanager.cpp`: added `lastState` to the `$modify`
    Fields so PLAYBACK that's armed mid-level (without dying or a
    scene change) forces a cursor resync. Without this, a previously
    idle PlayLayer with a loaded macro would replay every input from
    index 0 in a single frame the moment PLAYBACK was toggled on,
    instantly bricking the level. Spam loop now also requires
    `PlayLayer::get() != nullptr` (no spam in the editor's edit
    mode) and uses `PlayLayer::m_isPaused` instead of
    `CCDirector::isPaused()` for the "only while playing" gate
    (the director flag is unreliable on Android; GD's pause menu
    overlays the scene without always pausing the director).
  - `src/gui.cpp`: `shouldRenderHud()` switched to
    `PlayLayer::m_isPaused` for the same reason. New "Hide while
    editing a level" toggle distinguishes the editor from in-level
    so users can keep the menu in test play but hide it while
    placing blocks. Added a `LevelEditorLayer::init` hook that
    clears `hudHiddenAfterFinish` on entering the editor (so a
    finished level followed by a jump into the editor doesn't leave
    the menu permanently hidden).
  - `src/zBot.hpp`: added `hideInEditor` field with persistence,
    bumped `ZBOT_VERSION` to `v1.4.1`.
  - `mod.json`: bumped version to `v1.4.1`.
- 2026-04-24: v1.4.2 macro replay fix + always-visible summon ball
  (EclipseMenu / xdBot inspired).
  - `src/playbackmanager.cpp`: the v1.4.1 resync fast-forwarded the
    cursor past every input with `frame < currentFrame`
    unconditionally. On a fresh level entry at frame=1 that meant
    the frame-0 input got skipped forever and macros looked broken
    ("loaded but nothing fires"). Now the cursor only fast-forwards
    when joining mid-level (`frame > 1`); fresh entries and restarts
    leave the cursor at 0 so frame-0 inputs fire on the next tick.
    The mid-level brick-bug from v1.4.0 is still fixed.
  - `src/gui.cpp` / `src/gui.hpp`: split `shouldRenderHud()` into
    `shouldRenderBall()` and `shouldRenderPanel()`. The floating ball
    is now an always-visible "summon" handle (only `onlyShowInMenu`
    hides it) so the user is never stranded without a way to reopen
    the panel during a pause. The heavy panel still respects every
    visibility toggle independently. Matches EclipseMenu's behaviour
    (their indicator stays visible while the big panel is hidden).
  - `src/gui.cpp`: added an EclipseMenu-style filter input above the
    macro list. Empty filter shows everything; otherwise
    case-insensitive substring match on the macro name. Shows a
    placeholder when the filter excludes every macro on disk.
  - `src/gui.hpp`: new `macroFilter[64]` buffer for the filter text.
  - `src/recordmanager.cpp`: gated `hudHiddenAfterFinish = true` on
    the `hideAfterFinish` toggle so flipping the toggle on AFTER
    finishing a level no longer instantly hides the menu.
  - `src/zBot.hpp`: bumped `ZBOT_VERSION` to `v1.4.2`.
  - `mod.json`: bumped version to `v1.4.2`.
- 2026-04-24: v1.5.0 macro replay rebuild + visibility toggles removed.
  - **Macro playback rebuild (matcool/ReplayBot + FigmentBoy/zBot canonical
    semantics).** The previous `(currentProgress - 1)` recording offset
    paired with `frame < currentProgress` playback comparison had subtle
    off-by-one paths (frame=-1 records when a player jumped on the very
    first tick; resync logic that relied on stale `lastFrame` to detect a
    level reset) that intermittently swallowed the first input of a
    macro. Rebuilt to the canonical pattern:
    - `src/recordmanager.cpp`: record the raw `m_currentProgress` (no
      `-1` offset) so the on-disk frame number is the actual game frame
      the player pressed on. `PlayLayer::init` now bumps the playback
      epoch when re-entering a level with PLAYBACK armed so the cursor
      always rewinds to input 0.
    - `src/playbackmanager.cpp`: fire any input whose `frame <=
      currentProgress` on this tick (matches matcool/FigmentBoy). Cursor
      rewind is no longer driven by the brittle `frame < lastFrame`
      heuristic — it watches `zBot::playbackEpoch`, an explicit counter
      bumped from the GUI Replay button, from
      `PlayLayer::resetLevel` (level restart) and from `PlayLayer::init`
      (re-entering with PLAYBACK armed). The epoch is the single source
      of truth shared between the GJBaseGameLayer and PlayLayer modify
      classes (whose Geode `$modify` Fields don't share storage).
      Switched the playback fire path to `this->handleButton(...)` so it
      flows through the modify chain like real input.
    - `src/zBot.hpp`: added `playbackEpoch` + `requestPlaybackRestart()`.
  - **Settings tab: every "hide menu while X" toggle removed.** The
    "Hide while playing", "Hide after finishing", "Only show in main
    menu" and "Hide while editing a level" toggles caused more
    confusion than they solved (users would lose the menu and not
    know how to bring it back). Dropped:
    - `src/zBot.hpp`: removed `hideWhilePlaying`, `hideAfterFinish`,
      `onlyShowInMenu`, `hideInEditor`, `hudHiddenAfterFinish` fields
      and their save/load entries.
    - `src/gui.hpp` / `src/gui.cpp`: removed `shouldRenderBall()` /
      `shouldRenderPanel()` (renderer now unconditionally draws the
      ball and only hides the panel via the user's own open/close
      toggle). Removed the "Menu visibility" section in
      `renderSettingsTab`. Removed the `MenuLayer::init` and
      `LevelEditorLayer::init` `$modify` hooks (only existed to clear
      the now-deleted `hudHiddenAfterFinish` flag).
    - `src/recordmanager.cpp`: dropped every `hudHiddenAfterFinish` /
      `hideAfterFinish` reference from `init` and `levelComplete`.
  - **Saved-macros UX: single Replay button.** The old "Load##sel" and
    "Play##sel" two-button workflow was confusing. `src/gui.cpp` now
    shows a single big `Replay` button below the saved-macros list:
    one tap loads the selected macro from disk, sets `state = PLAYBACK`
    regardless of the Home tab radio, copies the macro name into the
    `Name` input field at the top, and bumps `playbackEpoch` so the
    cursor restarts from input 0 the next time the level (re)starts.
    Workflow: tap macro -> tap Replay -> restart level -> macro plays.
    Added a "Selected: <name>" line above the action buttons so the
    user always knows which macro Replay/Delete will act on (no need
    to scroll back to the Name field). Removed the double-click play
    handler — the explicit Replay button is the only play path now.
  - `src/zBot.hpp`: bumped `ZBOT_VERSION` to `v1.5.0`.
  - `mod.json`: bumped version to `v1.5.0`.
  - `web/index.html`: bumped version badge + summary copy to v1.5.0.
- 2026-04-24: v1.5.1 macro tab two-button layout + correctness sweep.
  - **Macro tab: Delete + Replay as two side-by-side buttons.** The
    Replay/Delete pair under the saved-macros list is now laid out
    horizontally on a single row, with Delete on the left (fixed
    110 px secondary action) and Replay on the right (primary, takes
    the rest of the row). Both at 38 px height for comfortable touch.
    `src/gui.cpp::renderMacroTab` rewrite of the action-button block.
  - **Spam emissions go through the modify chain.** `src/playbackmanager.cpp`
    used `GJBaseGameLayer::handleButton(...)` (qualified) to emit the
    autoclicker's synthetic presses. That call bypasses the modify
    chain entirely, which silently broke the `spamRecordToMacro`
    setting (the recording hook never saw the events). Switched all
    six spam emission sites to `this->handleButton(...)`. The
    `spamSuppressRecord` flag still gates whether the record hook
    actually persists the event, so `spamRecordToMacro = false` keeps
    its existing "macros stay clean" guarantee.
  - **No more double auto-save on level complete.** `src/recordmanager.cpp::levelComplete`
    saved the macro, then `onExit` saved the same file again as the
    scene tore down. Added `zBot::autoSavedThisRun` to dedupe — set
    by `levelComplete`, checked and cleared by `onExit`.
  - **Macro filename sanitization tightened.** `src/gui.cpp` previously
    blocked `.` from typed input but allowed `/` and `\\` through, and
    a paste could still smuggle a dot in. Renamed
    `blockDots` -> `blockBadNameChars` (rejects `.`, `/`, `\\` at the
    InputText callback) and updated `sanitizeName` to strip all three
    on a paste/load path.
  - **Save now auto-selects the just-saved macro.** `src/gui.cpp` Save
    button refreshes the macro list, finds the saved name, sets
    `selectedMacro` accordingly, and clears `macroFilter` so a stale
    search term can't hide the new entry. One tap on Replay
    immediately after Save just works.
  - `src/zBot.hpp`: added `autoSavedThisRun` field; bumped
    `ZBOT_VERSION` to `v1.5.1`.
  - `mod.json`: bumped version to `v1.5.1`.
  - `web/index.html`: bumped version badge + summary copy to v1.5.1.
- 2026-04-25: v1.5.10 macro playback fix on Android ("el macro no funciona").
  - **The bug:** on Android (the only platform we ship to) GD's
    `GJBaseGameLayer` keeps a per-frame `m_allowedButtons` set that
    every `handleButton` call is filtered against. The set only
    contains buttons the on-screen touch controls just emitted, so a
    replay-injected event was silently dropped and the player never
    moved. Internally the playback cursor was advancing fine, which
    is why the macro looked alive in the menu but produced zero input
    in-game -- the "el macro no funciona" symptom users reported.
  - **The fix:** in `src/playbackmanager.cpp::processCommands` clear
    `m_allowedButtons` immediately before each replay-driven
    `handleButton` call, gated on `#ifdef GEODE_IS_MOBILE` so desktop
    behaviour is untouched. Same one-line guard added to the
    autoclicker/spammer's `driveButton` path so synthetic spam clicks
    aren't filtered out either. This is the workaround EclipseMenu
    uses in `hacks/Bot/Bot.cpp::processBot`, confirmed against the
    extracted reference archive in `attached_assets/`.
  - `src/zBot.hpp`: bumped `ZBOT_VERSION` to `v1.5.10`.
  - `mod.json`: bumped version to `v1.5.10`.
  - `web/index.html`: bumped version badge + summary copy to v1.5.10.
