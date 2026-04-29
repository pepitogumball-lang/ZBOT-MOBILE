#include "zBot.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <cocos2d.h>
#include <cmath>
#include <algorithm>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Playback + built-in spammer
// ---------------------------------------------------------------------------
//
// Both pieces of logic live on the same GJBaseGameLayer hook because
// they share the per-frame fast path and the same notion of "current
// in-game frame" (m_gameState.m_currentProgress). Per-instance state
// lives on Geode's $modify Fields so two PlayLayer entries in a row
// (or arming Playback mid-level) can't bleed indexes between sessions.
//
// Playback model (matcool/ReplayBot, FigmentBoy/zBot semantics):
//   - Record: store the raw m_currentProgress at the moment handleButton
//     fires.
//   - Playback: every tick, fire all inputs whose frame is <= the
//     current m_currentProgress. The cursor advances monotonically;
//     it is rewound to 0 only when the playback "epoch" bumps (set by
//     the GUI when a macro is loaded/armed, and by PlayLayer::resetLevel
//     when the player restarts the level).
//
// The epoch trigger is critical because Geode $modify Fields are
// per-modify-class, not shared between zPlayPL (PlayLayer) and
// zPlayGJBGL (GJBaseGameLayer). zBot::playbackEpoch is the single
// source of truth — both sides read/write the same int.

class $modify(zPlayGJBGL, GJBaseGameLayer) {
    struct Fields {
        // Playback cursor + clickbot lookahead cursor.
        int currIndex      = 0;
        int clickBotIndex  = 0;

        // Cached epoch + replay pointer so we can detect "the user
        // armed a different macro" or "the level was reset" without
        // having to peek into other modify classes' Fields.
        int      lastEpoch  = -1;
        zReplay* lastReplay = nullptr;

        // Spam state: tracked button state per player so we only emit
        // events on transitions, and the frame the spammer was first
        // armed on so phase math is anchored to that frame instead of
        // the absolute progress (which gives a cleaner cycle when the
        // user toggles spam mid-level).
        bool spamHeld[2]   = { false, false };
        int  spamAnchor    = -1;
        bool spamWasOn     = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        zBot* mgr = zBot::get();

        // ---------------------------------------------------------------
        // Sample the frame number BEFORE the parent call.
        //
        // m_gameState.m_currentProgress increments inside processCommands.
        // Sampling here gives us the frame number the inputs will be
        // consumed on. This is the same value handleButton sees during
        // recording (handleButton runs inside processCommands before
        // physics steps), so record-time frames and playback-time frames
        // line up exactly.
        //
        // Pre-v1.5.7 we sampled this AFTER calling parent processCommands,
        // which meant every replayed input arrived in GD's input buffer
        // one tick late. That cumulative 1-frame lag is what made saved
        // macros desync on playback, especially on fast levels.
        //
        // /2 because GD 2.208/2.2081 changed m_currentProgress to count
        // half-ticks instead of visual frames -- it now ticks twice as
        // fast as on older versions. The recorder applies the same /2,
        // so the comparison is apples-to-apples and saved durations,
        // spam phase math, and the clickbot SFX lookahead are all
        // expressed in real visual frames. EclipseMenu's bot module
        // (hacks/Bot/Bot.cpp::processBot) does the same compensation.
        // ---------------------------------------------------------------
        int frame = static_cast<int>(m_gameState.m_currentProgress) / 2;
        auto* pl = PlayLayer::get();
        bool inLevel = pl != nullptr;

        // ---- Playback (BEFORE parent dispatch) -----------------------------
        // Inputs are pushed into GD's input buffer on this tick so that
        // the parent processCommands call below picks them up and applies
        // them in the same physics step they were originally recorded on.
        // This is the matcool/ReplayBot + FigmentBoy/zBot canonical
        // ordering and is the v1.5.7 fix for replay desync.
        if (mgr->state == PLAYBACK && mgr->currentReplay) {
            // Detect a fresh start: the GUI bumped the epoch (loaded a
            // new macro / hit Replay) or the player swapped the active
            // replay pointer underneath us. PlayLayer::resetLevel also
            // bumps the epoch so a level restart rewinds the cursor.
            bool epochChanged   = (m_fields->lastEpoch  != mgr->playbackEpoch);
            bool replayChanged  = (m_fields->lastReplay != mgr->currentReplay);

            if (epochChanged || replayChanged) {
                m_fields->lastEpoch     = mgr->playbackEpoch;
                m_fields->lastReplay    = mgr->currentReplay;
                m_fields->currIndex     = 0;
                m_fields->clickBotIndex = 0;

                // The GUI sets `justLoaded = true` when arming a fresh
                // replay so any hook can detect "this is the first
                // playback tick after a load". Consume it here so the
                // flag has a defined lifecycle (true on arm -> false
                // on first playback tick) and can never silently stay
                // latched on across later runs.
                mgr->justLoaded = false;

                // Mid-level join (e.g. user armed Playback during a
                // checkpoint attempt): fast-forward past inputs that
                // already "should have fired" so we don't replay 500
                // events in a single tick and brick the level. Fresh
                // starts (frame 0/1) leave the cursor at 0 so frame-0
                // inputs still play normally.
                if (frame > 1) {
                    auto& inputs = mgr->currentReplay->inputs;
                    while (m_fields->currIndex < (int)inputs.size() &&
                           inputs[m_fields->currIndex].frame < frame) {
                        m_fields->currIndex++;
                    }
                    m_fields->clickBotIndex = m_fields->currIndex;
                }
            }

            auto& inputs = mgr->currentReplay->inputs;

            // Fire every input whose frame has now arrived. Using <= is
            // the canonical matcool/FigmentBoy comparison: an input
            // recorded at frame N fires the moment m_currentProgress
            // reaches N, on the same tick.
            while (m_fields->currIndex < (int)inputs.size() &&
                   inputs[m_fields->currIndex].frame <= frame) {
                auto& input = inputs[m_fields->currIndex++];
                // EclipseMenu / xdBot / FigmentBoy zBot canonical
                // playback injection. Two cooperating pieces are
                // needed to make this work on Android:
                //
                // 1) Clear `m_allowedButtons` on mobile before the
                //    call. GD's mobile build maintains a per-tick
                //    set of buttons that are "allowed" to be
                //    processed this tick (populated by the on-screen
                //    touch controls). `handleButton` consults this
                //    set FIRST and silently drops any event whose
                //    button isn't in it -- which is exactly what
                //    eats every synthetic playback input on Android
                //    ("macro carga pero el personaje no hace nada").
                //    EclipseMenu does the same clear in its
                //    `processBot` (hacks/Bot/Bot.cpp:484-486).
                //
                // 2) Call the qualified parent
                //    `GJBaseGameLayer::handleButton` so the event
                //    goes STRAIGHT to the engine, bypassing the
                //    modify chain. Routing through `this->` would
                //    let every other handleButton hook see (and
                //    potentially drop) the synthetic input. The
                //    recording hook doesn't need to see playback
                //    events anyway -- it short-circuits on
                //    `state != RECORD` -- and the clickbot SFX is
                //    driven by the separate `clickBotIndex`
                //    lookahead below, not by handleButton.
                #ifdef GEODE_IS_MOBILE
                m_allowedButtons.clear();
                #endif
                GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
            }

            // Tiny lookahead so the click sound effect feels in sync
            // when the clickbot SFX is on. Pure cosmetic — does not
            // emit any input events.
            int offset = static_cast<int>(mgr->currentReplay->framerate * 0.1);
            while (m_fields->clickBotIndex < (int)inputs.size() &&
                   inputs[m_fields->clickBotIndex].frame <= frame + offset) {
                auto click = inputs[m_fields->clickBotIndex++];
                if (mgr->clickbotEnabled) {
                    mgr->playSound(click.player2, click.button, click.down);
                }
            }
        } else {
            // Idle / record: keep the epoch + replay pointer cache
            // current so the next PLAYBACK arming triggers a clean
            // restart on the next tick.
            m_fields->lastEpoch  = mgr->playbackEpoch;
            m_fields->lastReplay = mgr->currentReplay;
        }

        // ---- Frame-advance gate (TAS stepper) ------------------------------
        //
        // When the user toggles `Frame Advance` in the Settings tab,
        // physics only steps when they explicitly tap the `Advance` button
        // (which sets `doAdvance` for one tick). Skipping the parent
        // processCommands freezes the level: inputs are not consumed,
        // physics does not step. Standard TAS stepper behaviour, lifted
        // conceptually from Eclipse / xdBot / GDH.
        //
        // The pre-existing `ignoreInput` flag is preserved as the global
        // mute switch.
        bool runParent = !mgr->ignoreInput;
        if (mgr->frameAdvance && inLevel) {
            if (!mgr->doAdvance) runParent = false;
            mgr->doAdvance = false;
        }
        if (runParent) {
            GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }

        // ---- Built-in spammer / autoclicker --------------------------------
        // Drives a configurable on/off cycle on a chosen PlayerButton.
        // Anchored to the frame the spammer was armed on so the phase
        // doesn't lurch when toggled mid-level. Bypasses recording by
        // default (see the spamSuppressRecord flag) so saved macros
        // stay clean.
        if (mgr->spamEnabled && inLevel) {
            // Optional gate: only spam while the level is actively
            // being played (not paused, not in a transition).
            // PlayLayer::m_isPaused is the canonical pause flag --
            // CCDirector::isPaused() is unreliable on Android because
            // GD's pause menu overlays the scene without pausing the
            // director on every code path.
            bool gateOk = true;
            if (mgr->spamOnlyDuringPlay) {
                gateOk = !pl->m_isPaused;
            }

            if (gateOk) {
                // Reset anchor whenever spam is toggled on, or whenever
                // the frame jumps backwards (respawn) — keeps the
                // cadence aligned with the current attempt instead of
                // an old one.
                if (!m_fields->spamWasOn || frame < m_fields->spamAnchor) {
                    m_fields->spamAnchor = frame;
                    m_fields->spamHeld[0] = false;
                    m_fields->spamHeld[1] = false;
                }
                m_fields->spamWasOn = true;

                double tps = (mgr->currentReplay && mgr->currentReplay->framerate > 0.0)
                    ? mgr->currentReplay->framerate
                    : 240.0;
                double cps = std::clamp(mgr->spamCPS, 0.1, 240.0);
                double cycle = tps / cps;
                if (cycle < 1.0) cycle = 1.0;
                double hold = cycle * std::clamp(mgr->spamHoldRatio, 0.0, 1.0);
                if (hold < 1.0 && mgr->spamHoldRatio > 0.0) hold = 1.0;
                if (hold >= cycle) hold = cycle - 0.001;

                double rel  = static_cast<double>(frame - m_fields->spamAnchor);
                double phase = std::fmod(rel, cycle);
                if (phase < 0.0) phase += cycle;
                bool wantDown = (phase < hold);

                bool dual = m_gameState.m_isDualMode &&
                            m_levelSettings &&
                            m_levelSettings->m_twoPlayerMode;

                int button = mgr->spamButton;
                if (button < 1 || button > 3) button = 1;

                auto driveButton = [&](int playerIdx, bool isP2) {
                    bool& cur = m_fields->spamHeld[playerIdx];
                    if (cur == wantDown) return;
                    cur = wantDown;

                    // Mark the upcoming handleButton as a spam-emitted
                    // event so the record hook can choose to skip it.
                    // Route through this->handleButton so the modify
                    // chain (record hook included) actually sees it --
                    // a qualified GJBaseGameLayer::handleButton call
                    // would skip every other hook and silently break
                    // the spamRecordToMacro flag.
                    mgr->spamSuppressRecord = true;
                    // Spammer routes through `this->handleButton` on
                    // purpose: the recording hook needs to see spam
                    // events when `spamRecordToMacro` is on. Playback
                    // bypasses the chain (qualified parent call above)
                    // because the recorder doesn't need to see those.
                    //
                    // Same Android allowed-buttons workaround as the
                    // playback path: clear the per-tick set so the
                    // engine's handleButton doesn't drop our synthetic
                    // event silently. EclipseMenu uses the same guard
                    // before its `simulateClick`.
                    #ifdef GEODE_IS_MOBILE
                    m_allowedButtons.clear();
                    #endif
                    this->handleButton(wantDown, button, !isP2);
                    mgr->spamSuppressRecord = false;
                };

                bool wantP1 = (mgr->spamPlayer == 0 || mgr->spamPlayer == 2);
                bool wantP2 = (mgr->spamPlayer == 1 || mgr->spamPlayer == 2) && dual;

                if (wantP1) driveButton(0, false);
                if (wantP2) driveButton(1, true);
            } else {
                // Gate closed: release any held spam button so the
                // player isn't stuck holding jump while paused. Same
                // routing rule as the press path -- this->handleButton
                // so the modify chain stays consistent.
                if (m_fields->spamHeld[0] || m_fields->spamHeld[1]) {
                    int button = mgr->spamButton;
                    if (button < 1 || button > 3) button = 1;
                    mgr->spamSuppressRecord = true;
                    if (m_fields->spamHeld[0]) {
                        #ifdef GEODE_IS_MOBILE
                        m_allowedButtons.clear();
                        #endif
                        this->handleButton(false, button, true);
                        m_fields->spamHeld[0] = false;
                    }
                    if (m_fields->spamHeld[1]) {
                        #ifdef GEODE_IS_MOBILE
                        m_allowedButtons.clear();
                        #endif
                        this->handleButton(false, button, false);
                        m_fields->spamHeld[1] = false;
                    }
                    mgr->spamSuppressRecord = false;
                }
                m_fields->spamWasOn = false;
            }
        } else if (m_fields->spamWasOn) {
            // Spammer was just turned off: release any still-held button
            // so we don't strand the player in a held state.
            int button = mgr->spamButton;
            if (button < 1 || button > 3) button = 1;
            mgr->spamSuppressRecord = true;
            if (m_fields->spamHeld[0]) {
                #ifdef GEODE_IS_MOBILE
                m_allowedButtons.clear();
                #endif
                this->handleButton(false, button, true);
                m_fields->spamHeld[0] = false;
            }
            if (m_fields->spamHeld[1]) {
                #ifdef GEODE_IS_MOBILE
                m_allowedButtons.clear();
                #endif
                this->handleButton(false, button, false);
                m_fields->spamHeld[1] = false;
            }
            mgr->spamSuppressRecord = false;
            m_fields->spamWasOn = false;
        }
    }
};

class $modify(zPlayPL, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();

        // A level restart should rewind the playback cursor to 0 so the
        // macro plays from the beginning of the run. We can't poke into
        // the GJBaseGameLayer modify Fields from here (different modify
        // class -> different Fields struct), so we bump the shared
        // playback epoch on the zBot singleton and the playback hook
        // picks up the change on its next tick.
        zBot* mgr = zBot::get();
        if (mgr->state == PLAYBACK) {
            mgr->requestPlaybackRestart();
        }
    }
};
