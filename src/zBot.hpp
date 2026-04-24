#ifndef _zbot_hpp
#define _zbot_hpp

#include <Geode/Geode.hpp>
#include "replay.hpp"

using namespace geode::prelude;

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

    // Set to true by PlayLayer when the level was successfully completed
    // during the current recording session.
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

    // Reset the cached "is button held" state for a given player. Called
    // from resetLevel so that after a respawn we treat all buttons as
    // released regardless of what we recorded before the rewind.
    void resetButtonStateAfterFrame(int frame) {
        // Replay the surviving inputs to figure out which buttons should
        // currently be held. This keeps dedupe consistent with reality
        // after a checkpoint rewind.
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

    void playSound(bool p2, int button, bool down);
};

#endif
