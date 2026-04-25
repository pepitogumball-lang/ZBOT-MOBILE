# Changelog

All notable changes to ZBOT-MOBILE are documented here. Versions follow
the `vX.Y.Z` tag in `mod.json`; each entry corresponds to a GitHub
Release published by `.github/workflows/build.yml`.

## v1.5.7 — Replay accuracy fix + Frame Advance (TAS stepper)

The big one. Two changes that directly target *"the macro replay
doesn't work"*:

### 1. Playback dispatch reordering (replay desync fix)

`src/playbackmanager.cpp` — `GJBaseGameLayer::processCommands`:

- Pre-v1.5.7, the parent `processCommands` was called FIRST, then we
  fired the recorded inputs via `this->handleButton`. Because GD
  consumes the input buffer inside `processCommands`, every replayed
  input was landing in the buffer for the **next** tick — a constant
  1-frame lag that compounds into visible desync on long macros and
  on fast levels.
- v1.5.7 reorders so that recorded inputs are dispatched BEFORE the
  parent `processCommands` call, exactly matching the canonical
  matcool/ReplayBot + FigmentBoy/zBot placement. The frame counter is
  also sampled before the parent call so record-time and playback-time
  frame numbers line up bit-for-bit.
- The recorded-frame snapshot in `recordmanager.cpp` was already
  correct — `handleButton` runs inside `processCommands` before the
  physics step, so it samples the same logical frame the new playback
  ordering targets.

### 2. Frame Advance — TAS-style frame stepper

`src/playbackmanager.cpp` + `src/gui.cpp` — Settings tab:

- New "Frame Advance" toggle in the Settings tab → "TAS / debug"
  section. While ON, the level is fully frozen — inputs are not
  consumed, physics does not step.
- A new "Advance 1 frame" button steps physics forward exactly one
  tick per tap. This is the standard TAS stepper behaviour seen in
  Eclipse, xdBot and GDH and is the right tool to diagnose where a
  macro desyncs (advance frame-by-frame and watch where playback
  diverges from expected behaviour).
- The existing `frameAdvance` / `doAdvance` flags on `zBot` (declared
  but previously unused) are now wired up.
- Off by default so casual users never accidentally lock the game.

### 3. Live playback diagnostic in the GUI

`src/gui.cpp` — `renderReplayInfo()`:

- During playback, the Home tab's status block now shows
  `Live: frame N  next input @ M  (delta +K)`.
- If the delta stays small and shrinks toward 0 each tick, playback is
  in sync. If the delta grows monotonically, the macro is desyncing
  and you have an exact frame number to investigate.

## v1.5.6 — Attribution fix: credit FigmentBoy + zilko correctly

Authorship correction. Previous releases credited "zilko" as the
author of "zBot", which conflated two separate community projects:

- **zBot** is by **FigmentBoy** (`fig.zbot`,
  https://github.com/FigmentBoy/zBot). The recording / playback core,
  `zReplay` layout, recordmanager / playbackmanager split, and the
  `m_currentProgress` playback semantics in this fork are derived
  from FigmentBoy's zBot.
- **xdBot** is by **zilko** (`zilko.xdbot`,
  https://github.com/ZiLko/xdBot). The clock-style speedhack and
  several smaller behaviours in this fork were referenced from
  zilko's xdBot.

Changes:

- `mod.json` — `developers` is now
  `["pepitogumball", "FigmentBoy", "zilko"]`. Description rewritten
  to attribute the core to FigmentBoy and the speedhack/extras to
  zilko.
- `CREDITS.md` — fully rewritten so that each upstream is credited
  for the specific files / behaviours derived from it. Also adds an
  explicit note that FigmentBoy's zBot ships under the Aseprite EULA
  (a restrictive non-FOSS license) and that anyone redistributing a
  build should review it.
- `README.md` — short-form credits updated to match.
- `web/index.html` — info card and notice rewritten with the correct
  upstream links.

This is the attribution change that should unblock the Geode index
review that previously rejected the mod for incorrect attribution.

## v1.5.5 — In-game update check via GitHub Releases API

New feature:

- `src/updatecheck.cpp` (new) — on mod load, ZBOT-MOBILE makes a single
  HTTPS request to
  `https://api.github.com/repos/pepitogumball-lang/ZBOT-MOBILE/releases/latest`,
  parses the `tag_name` field, and compares it to `ZBOT_VERSION`. If
  the remote tag is a higher `vX.Y.Z`, the user gets one in-game
  notification: *"ZBOT-MOBILE vX.Y.Z available - check the GitHub
  releases page"*.
- `mod.json` — new `disable-update-check` boolean setting (default
  `false`). Geode wires this into the standard per-mod settings panel,
  so the user can opt out without editing files.
- All failure paths (no network, GitHub rate-limit, malformed JSON,
  unparseable tag) silently no-op. The check never blocks gameplay,
  never opens a browser on its own, and never auto-installs anything.
- `CMakeLists.txt` — `src/updatecheck.cpp` added to `SOURCES`.

CI / repo hygiene:

- The local `post-commit` git hook now prints a one-line confirmation
  after a successful auto-push, e.g.
  `[post-commit] pushed <sha> to origin/main -> workflow build.yml
  triggered (<repo>/actions)`. On failure it prints the push output
  with each line prefixed so it's obvious in the log.

## v1.5.4 — Save reliability + Clickbot SFX + version sync

Bug fixes:

- `src/replay.hpp` — `zReplay::save` now returns `bool` and reports
  failure when the macros directory can't be created, the output file
  can't be opened, the write fails, or the final flush/close fails.
  Previously the GUI showed "Macro saved" unconditionally — even if
  the disk write silently failed (permission denied, storage full,
  bad path), the user got a green success toast for nothing.
- `src/gui.cpp` — the Save button now reads that boolean. On failure
  it shows a red "Save failed - check storage permission" toast
  instead of lying with the success notification.
- `src/replay.hpp` — `zReplay::fromFile` now validates the result of
  `tellg()`. A failed seek used to return -1, which got cast to a
  huge unsigned `std::vector<uint8_t>` size and either OOM'd the
  process or triggered undefined behaviour. Also clamps macro size
  to 64 MiB so a corrupt / hostile file can't trigger a giant
  allocation.

Functional fix:

- `src/zBot.cpp` — `zBot::playSound` was previously an empty stub, so
  the in-GUI "Clickbot SFX" toggle did nothing. Now plays a short
  click effect via `FMODAudioEngine::playEffect` on every press
  event (release events are skipped so each click makes one sound,
  not two). Falls back to a silent no-op if FMOD isn't ready or the
  asset is missing — never crashes a frame.

Version sync:

- `src/zBot.hpp` — `ZBOT_VERSION` was stuck on `v1.5.1`, three
  releases behind `mod.json`. Now bumped to `v1.5.4` and a comment
  enforces the "must match mod.json" rule.
- `src/replay.hpp` — the `"1.0.0"` Replay constructor argument is the
  on-disk gdr metadata format version, not the mod version. Added a
  comment so it's not mistaken for drift in the future.

Carried forward from v1.5.3 (re-stated for the release notes since
v1.5.3's auto-release bundled them already):

- Replay button clears `ignoreInput` and sets `justLoaded = true`
  before arming playback.
- `processCommands` now consumes `justLoaded` on the first playback
  tick after a fresh arm so the flag has a defined lifecycle.

## v1.5.3 — Replay input-lock fix

Fixed: tapping **Replay** on a saved macro could arm playback without
firing any inputs, especially noticeable on tablet / mid-attempt joins.

Root cause: a previous PLAYBACK could leave `zBot::ignoreInput`
latched on. The next arm bumped the playback epoch and reset the
cursor correctly, but `processCommands` then refused to forward inputs
because `ignoreInput` was still `true`. The macro looked armed but
nothing happened.

Changes:

- `src/gui.cpp` — Replay button now also clears `ignoreInput` and
  sets `justLoaded = true` immediately after `requestPlaybackRestart()`,
  so the next playback tick starts from a clean state.
- `src/playbackmanager.cpp` — `processCommands` now consumes
  `justLoaded` (clears it back to `false`) on the first playback tick
  after a fresh arm. This gives the flag a defined lifecycle (true on
  arm → false on first tick) so it can never silently stay latched on
  across later runs.

How to verify: open a level, pick a macro, tap **Replay**, restart the
level. Inputs should now fire even if you'd been in PLAYBACK earlier
in the same session.

## v1.5.2 — Attribution fix

This release exists to address the Geode index rejection of v1.5.1 for
missing credit to the original mod and library authors.

- `mod.json` — switched `developer` to a `developers` array crediting
  **zilko** (original zBot author) alongside `pepitogumball`. Added
  `links.source` and rewrote the description to explicitly say this
  is a port of zBot.
- New **`CREDITS.md`** — full attribution to zilko, plus matcool
  (ReplayBot, gd-imgui-cocos), FigmentBoy, EclipseMenu, maxnut
  (GDReplayFormat). Lists what is original to ZBOT-MOBILE vs reused.
- `README.md` — rewritten to flag the project as a fork up front.
- `web/index.html` — credit notice + link to `CREDITS.md`.

CI: `.github/workflows/build.yml` now reads the version from `mod.json`
and tags the GitHub Release as `vX.Y.Z` (e.g. `v1.5.3`) instead of
always `latest`. Each release uploads only the `.geode` files.

## v1.5.1 and earlier

Pre-attribution-fix releases. See the git log for details.
