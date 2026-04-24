#ifndef _zbot_hpp
#define _zbot_hpp

#include <Geode/Geode.hpp>
#include "replay.hpp"

using namespace geode::prelude;

#define ZBOT_VERSION "v1.4.2"

enum zState {
    NONE, RECORD, PLAYBACK
};

class zBot {
public:
    // ----- Recording / playback state ---------------------------------------
    zState state = NONE;
    bool fmodified = false;
    float extraTPS = 0.f;

    bool disableRender = false;
    bool ignoreBypass = false;
    bool justLoaded = false;
    bool ignoreInput = false;
    bool frameAdvance = false;
    bool doAdvance = false;

    // ----- Speedhack --------------------------------------------------------
    // Clock-style speedhack (xdBot/EclipseMenu semantics): we scale the
    // delta time fed into the cocos scheduler. Music pitch follows via a
    // separate FMOD hook.
    bool speedHackEnabled = false;
    bool speedHackAudio = true;
    double speed = 1.0;
    double tps = 240.0;

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

    // ----- Visibility / HUD behaviour ---------------------------------------
    bool hideWhilePlaying = false; // hide menu unless game is paused
    bool hideAfterFinish  = false; // hide menu after a level is completed
    bool onlyShowInMenu   = false; // hide whenever PlayLayer is loaded
    bool hideInEditor     = false; // hide menu while editing a level

    // Runtime-only flag flipped on by levelComplete and flipped off on
    // MenuLayer::init (i.e. once the player returns to the main menu).
    bool hudHiddenAfterFinish = false;

    // ----- Per-attempt state ------------------------------------------------
    bool levelCompleted = false;

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

        m->setSavedValue<bool>  ("hideWhilePlaying",   hideWhilePlaying);
        m->setSavedValue<bool>  ("hideAfterFinish",    hideAfterFinish);
        m->setSavedValue<bool>  ("onlyShowInMenu",     onlyShowInMenu);
        m->setSavedValue<bool>  ("hideInEditor",       hideInEditor);
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

        hideWhilePlaying   = m->getSavedValue<bool>  ("hideWhilePlaying",   hideWhilePlaying);
        hideAfterFinish    = m->getSavedValue<bool>  ("hideAfterFinish",    hideAfterFinish);
        onlyShowInMenu     = m->getSavedValue<bool>  ("onlyShowInMenu",     onlyShowInMenu);
        hideInEditor       = m->getSavedValue<bool>  ("hideInEditor",       hideInEditor);
    }

    void playSound(bool p2, int button, bool down);
};

#endif
