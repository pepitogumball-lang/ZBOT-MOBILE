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

class $modify(zPlayGJBGL, GJBaseGameLayer) {
    struct Fields {
        // Playback cursor + clickbot lookahead cursor.
        int currIndex      = 0;
        int clickBotIndex  = 0;

        // Resync triggers: a new replay was loaded, or the in-game
        // frame went backwards (death without our resetLevel hook
        // firing first, e.g. some practice-mode rewinds).
        zReplay* lastReplay = nullptr;
        int      lastFrame  = -1;

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
        if (!zBot::get()->ignoreInput) {
            GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }

        zBot* mgr = zBot::get();
        int frame = static_cast<int>(m_gameState.m_currentProgress);

        // ---- Playback ------------------------------------------------------
        if (mgr->state == PLAYBACK && mgr->currentReplay) {
            // Re-sync indexes when the replay pointer changes (user
            // loaded a different macro mid-level) or when the frame
            // went backwards (death without a resetLevel hook firing
            // first). Without this, the playback would replay every
            // input from index 0 in a single frame, instantly bricking
            // the level.
            if (m_fields->lastReplay != mgr->currentReplay ||
                frame < m_fields->lastFrame) {
                m_fields->currIndex     = 0;
                m_fields->clickBotIndex = 0;
                m_fields->lastReplay    = mgr->currentReplay;
                while (m_fields->currIndex < (int)mgr->currentReplay->inputs.size() &&
                       mgr->currentReplay->inputs[m_fields->currIndex].frame < frame) {
                    m_fields->currIndex++;
                }
                m_fields->clickBotIndex = m_fields->currIndex;
            }
            m_fields->lastFrame = frame;

            while (m_fields->currIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[m_fields->currIndex].frame < frame) {
                auto input = mgr->currentReplay->inputs[m_fields->currIndex++];
                GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
            }

            // Tiny lookahead so the click sound effect feels in sync
            // when the clickbot is on. Pure cosmetic.
            int offset = static_cast<int>(mgr->currentReplay->framerate * 0.1);
            while (m_fields->clickBotIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[m_fields->clickBotIndex].frame < frame + offset) {
                auto click = mgr->currentReplay->inputs[m_fields->clickBotIndex++];
                if (mgr->clickbotEnabled) {
                    mgr->playSound(click.player2, click.button, click.down);
                }
            }
        } else {
            // Idle / record: keep lastFrame pegged so a future arming
            // of playback resyncs cleanly.
            m_fields->lastFrame = frame;
            m_fields->lastReplay = mgr->currentReplay;
        }

        // ---- Built-in spammer / autoclicker --------------------------------
        // Drives a configurable on/off cycle on a chosen PlayerButton.
        // Anchored to the frame the spammer was armed on so the phase
        // doesn't lurch when toggled mid-level. Bypasses recording by
        // default (see the spamSuppressRecord flag) so saved macros
        // stay clean.
        if (mgr->spamEnabled) {
            // Optional gate: only spam while the level is actively
            // being played (not paused, not in a transition).
            bool gateOk = true;
            if (mgr->spamOnlyDuringPlay) {
                gateOk = !cocos2d::CCDirector::sharedDirector()->isPaused();
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
                    mgr->spamSuppressRecord = true;
                    GJBaseGameLayer::handleButton(wantDown, button, !isP2);
                    mgr->spamSuppressRecord = false;
                };

                bool wantP1 = (mgr->spamPlayer == 0 || mgr->spamPlayer == 2);
                bool wantP2 = (mgr->spamPlayer == 1 || mgr->spamPlayer == 2) && dual;

                if (wantP1) driveButton(0, false);
                if (wantP2) driveButton(1, true);
            } else {
                // Gate closed: release any held spam button so the
                // player isn't stuck holding jump while paused.
                if (m_fields->spamHeld[0] || m_fields->spamHeld[1]) {
                    int button = mgr->spamButton;
                    if (button < 1 || button > 3) button = 1;
                    mgr->spamSuppressRecord = true;
                    if (m_fields->spamHeld[0]) {
                        GJBaseGameLayer::handleButton(false, button, true);
                        m_fields->spamHeld[0] = false;
                    }
                    if (m_fields->spamHeld[1]) {
                        GJBaseGameLayer::handleButton(false, button, false);
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
                GJBaseGameLayer::handleButton(false, button, true);
                m_fields->spamHeld[0] = false;
            }
            if (m_fields->spamHeld[1]) {
                GJBaseGameLayer::handleButton(false, button, false);
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

        // Forcibly invalidate the playback / spam cursors held on the
        // GJBaseGameLayer Fields. The processCommands hook will resync
        // on the next frame because lastFrame > current frame now.
        // We intentionally don't poke into the Fields directly here;
        // the mid-update resync handles the transition.
    }
};
