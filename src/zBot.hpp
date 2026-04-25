#ifndef _zbot_hpp
#define _zbot_hpp

#include <Geode/Geode.hpp>
#include "replay.hpp"

using namespace geode::prelude;

// User-facing mod version. Kept in sync with the `version` field in
// mod.json — both must say the same vX.Y.Z. The on-disk replay format
// version is separate and lives in src/replay.hpp.
#define ZBOT_VERSION "v1.5.9"

enum zState {
    NONE, RECORD, PLAYBACK
};

class zBot {
public:
    // ----- Recording / playback state ---------------------------------------
    zState state = NONE;
    bool fmodified = false;

    // Set true by the GUI when arming a fresh replay so any hook can
    // detect "this is the first playback tick after a load". Consumed
    // by the playback hook on the first PLAYBACK tick after arming.
    bool justLoaded = false;

    // Global "swallow live input" toggle. Reserved for the legacy mute
    // path used while a transition is replaying; checked in the
    // playback hook before calling parent processCommands.
    bool ignoreInput = false;

    // Frame Advance (TAS stepper). When `frameAdvance` is true and the
    // user is in a level, processCommands skips the parent call —
    // freezing inputs + physics — until `doAdvance` is set (one tick
    // per Advance tap). doAdvance is auto-cleared after consumption.
    bool frameAdvance = false;
    bool doAdvance    = false;

    // Bumped every time playback should restart from the beginning of
    // the macro: when a fresh macro is loaded, when the user re-arms
    // PLAYBACK from the saved-macros list, or when the level is reset.
    // The processCommands hook compares its cached value against this
    // and forces a cursor rewind whenever they disagree, which is the
    // canonical matcool/FigmentBoy "restart-the-replay" trigger.
    int playbackEpoch = 0;

    // ----- Speedhack --------------------------------------------------------
    // Clock-style speedhack (xdBot/EclipseMenu semantics): we scale the
    // delta time fed into the cocos scheduler. Music pitch follows via a
    // separate FMOD hook.
    bool speedHackEnabled = false;
    bool speedHackAudio = true;
    double speed = 1.0;

    // ----- Misc -------------------------------------------------------------
    bool clickbotEnabled = false;
    bool autoSafeMode = false;

    // ----- Macro recording quality / safety ---------------------------------
    // Auto-save the macro when the level ends. Off by default if the user
    // wants strict "save only when I press Save" behaviour.
    bool autoSave = true;

    // Perfect-run gate: when true, the recording is ONLY saved when the
    // player legitimately completes the level. Quitting mid-attempt does
    // NOT touch the file on disk, so a half-finished retry session can't
    // overwrite a previously saved masterpiece.
    bool perfectRunOnly = true;

    // Dedupe state-changing inputs at record time: skip presses that don't
    // change the player's button state (e.g. a release of an already-up
    // button). Prevents glitchy double-taps from polluting the macro.
    bool dedupeInputs = true;

    // ----- Spam-safety normalization ----------------------------------------
    // Applied at save time by zReplay::cleanInputs. Guarantees that a
    // press is held for at least `minHoldFrames` and that consecutive
    // presses of the same button have at least `minGapFrames` between
    // them. Defaults to 1/1 — just enough to stop GD from eating
    // same-frame press+release tuples produced by extreme spam taps,
    // without slowing legitimate fast input.
    int minHoldFrames = 1;
    int minGapFrames  = 1;

    // ----- Built-in spammer / autoclicker -----------------------------------
    bool   spamEnabled        = false;
    int    spamButton         = 1;     // 1=Jump, 2=Left, 3=Right (PlayerButton)
    double spamCPS            = 12.0;  // clicks per second
    double spamHoldRatio      = 0.5;   // 0..1, fraction of cycle the button is held
    int    spamPlayer         = 0;     // 0=P1, 1=P2, 2=Both
    bool   spamOnlyDuringPlay = true;  // pause spam outside of active gameplay
    bool   spamRecordToMacro  = false; // include spam events in the recording

    // Runtime-only flag set by the spammer right before it emits a
    // synthetic input, then cleared. The record hook checks this and
    // refuses to record the input when `spamRecordToMacro` is off, so
    // macros stay free of clickbot pollution by default.
    bool spamSuppressRecord = false;

    // ----- Per-attempt state ------------------------------------------------
    bool levelCompleted = false;

    // Set by levelComplete() when it has already auto-saved the macro
    // for this run. The matching onExit() hook checks this and skips
    // its own save so we don't write the exact same .gdr file twice
    // every time a perfect run finishes (once on completion, once
    // again as the level scene tears down).
    bool autoSavedThisRun = false;

    // Tracked button state per player so dedupe works without scanning the
    // whole input list every frame.
    bool p1ButtonHeld[8] = { false };
    bool p2ButtonHeld[8] = { false };

    // ----- File IO ----------------------------------------------------------
    char loadName[128] = "";
    zReplay* currentReplay = nullptr;

    void createNewReplay(GJGameLevel* level) {
        if (currentReplay) delete currentReplay;
        currentReplay = new zReplay();
        currentReplay->levelInfo.id = level->m_levelID;
        currentReplay->levelInfo.name = level->m_levelName;
        currentReplay->name = level->m_levelName;
        // Macros are saved at the canonical 240 TPS so playback always
        // produces a normal-speed run, even if speedhack was on while
        // recording. Inputs are stored by in-game frame number, which is
        // not affected by the real-time speedhack.
        currentReplay->framerate = 240.0;

        levelCompleted = false;
        for (int i = 0; i < 8; ++i) {
            p1ButtonHeld[i] = false;
            p2ButtonHeld[i] = false;
        }
    }

    // Re-derive cached "is button held" state from the surviving inputs
    // up to (but not including) `frame`. Called from resetLevel after
    // purgeAfter(frame) so dedupe stays consistent with what's actually
    // on disk after a checkpoint rewind.
    void resetButtonStateAfterFrame(int frame) {
        for (int i = 0; i < 8; ++i) {
            p1ButtonHeld[i] = false;
            p2ButtonHeld[i] = false;
        }
        if (!currentReplay) return;
        for (auto const& in : currentReplay->inputs) {
            if (in.frame >= frame) break;
            if (in.button < 1 || in.button > 7) continue;
            if (in.player2) p2ButtonHeld[in.button] = in.down;
            else            p1ButtonHeld[in.button] = in.down;
        }
    }

    // Bump the playback epoch so processCommands knows to reset its
    // cursor to 0 on the next tick. Called from PlayLayer::resetLevel
    // and from the GUI when loading / arming a macro.
    void requestPlaybackRestart() {
        ++playbackEpoch;
    }

    static zBot* get() {
        static zBot* instance = new zBot();
        return instance;
    }

    // Persist all user-tweakable settings via Geode's per-mod save store
    // so the menu remembers the player's choices between game launches.
    // Only fields the user sees in the GUI are saved — transient
    // recording/playback flags are always reset on launch.
    void saveSettings() {
        auto m = Mod::get();

        m->setSavedValue<bool>  ("speedHackEnabled",   speedHackEnabled);
        m->setSavedValue<bool>  ("speedHackAudio",     speedHackAudio);
        m->setSavedValue<double>("speed",              speed);

        m->setSavedValue<bool>  ("autoSafeMode",       autoSafeMode);
        m->setSavedValue<bool>  ("clickbotEnabled",    clickbotEnabled);

        m->setSavedValue<bool>  ("autoSave",           autoSave);
        m->setSavedValue<bool>  ("perfectRunOnly",     perfectRunOnly);
        m->setSavedValue<bool>  ("dedupeInputs",       dedupeInputs);

        m->setSavedValue<int64_t>("minHoldFrames",     minHoldFrames);
        m->setSavedValue<int64_t>("minGapFrames",      minGapFrames);

        m->setSavedValue<bool>  ("spamEnabled",        spamEnabled);
        m->setSavedValue<int64_t>("spamButton",        spamButton);
        m->setSavedValue<double>("spamCPS",            spamCPS);
        m->setSavedValue<double>("spamHoldRatio",      spamHoldRatio);
        m->setSavedValue<int64_t>("spamPlayer",        spamPlayer);
        m->setSavedValue<bool>  ("spamOnlyDuringPlay", spamOnlyDuringPlay);
        m->setSavedValue<bool>  ("spamRecordToMacro",  spamRecordToMacro);
    }

    void loadSettings() {
        auto m = Mod::get();

        speedHackEnabled   = m->getSavedValue<bool>  ("speedHackEnabled",   speedHackEnabled);
        speedHackAudio     = m->getSavedValue<bool>  ("speedHackAudio",     speedHackAudio);
        speed              = m->getSavedValue<double>("speed",              speed);

        autoSafeMode       = m->getSavedValue<bool>  ("autoSafeMode",       autoSafeMode);
        clickbotEnabled    = m->getSavedValue<bool>  ("clickbotEnabled",    clickbotEnabled);

        autoSave           = m->getSavedValue<bool>  ("autoSave",           autoSave);
        perfectRunOnly     = m->getSavedValue<bool>  ("perfectRunOnly",     perfectRunOnly);
        dedupeInputs       = m->getSavedValue<bool>  ("dedupeInputs",       dedupeInputs);

        minHoldFrames      = static_cast<int>(m->getSavedValue<int64_t>("minHoldFrames", minHoldFrames));
        minGapFrames       = static_cast<int>(m->getSavedValue<int64_t>("minGapFrames",  minGapFrames));
        if (minHoldFrames < 0) minHoldFrames = 0;
        if (minGapFrames  < 0) minGapFrames  = 0;

        spamEnabled        = m->getSavedValue<bool>  ("spamEnabled",        spamEnabled);
        spamButton         = static_cast<int>(m->getSavedValue<int64_t>("spamButton", spamButton));
        if (spamButton < 1 || spamButton > 3) spamButton = 1;
        spamCPS            = m->getSavedValue<double>("spamCPS",            spamCPS);
        if (spamCPS < 0.1) spamCPS = 0.1;
        if (spamCPS > 240.0) spamCPS = 240.0;
        spamHoldRatio      = m->getSavedValue<double>("spamHoldRatio",      spamHoldRatio);
        if (spamHoldRatio < 0.0) spamHoldRatio = 0.0;
        if (spamHoldRatio > 1.0) spamHoldRatio = 1.0;
        spamPlayer         = static_cast<int>(m->getSavedValue<int64_t>("spamPlayer", spamPlayer));
        if (spamPlayer < 0 || spamPlayer > 2) spamPlayer = 0;
        spamOnlyDuringPlay = m->getSavedValue<bool>  ("spamOnlyDuringPlay", spamOnlyDuringPlay);
        spamRecordToMacro  = m->getSavedValue<bool>  ("spamRecordToMacro",  spamRecordToMacro);
    }

    void playSound(bool p2, int button, bool down);
};

#endif
