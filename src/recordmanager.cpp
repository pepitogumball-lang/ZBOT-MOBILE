#include "zBot.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

// Drop inputs that don't change the held state of (player, button).
// Returns true if the caller should keep the input.
static bool shouldRecord(zBot* mgr, bool down, int button, bool p2) {
    if (!mgr->dedupeInputs) return true;
    if (button < 1 || button > 7) return true; // exotic, just record it
    bool& held = p2 ? mgr->p2ButtonHeld[button] : mgr->p1ButtonHeld[button];
    if (held == down) return false;
    held = down;
    return true;
}

class $modify(zRecGJBGL, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool p1) {
        GJBaseGameLayer::handleButton(down, button, p1);

        zBot* mgr = zBot::get();
        if (mgr->state != RECORD || !mgr->currentReplay) return;

        // Skip recording inputs that the built-in spammer just emitted,
        // unless the user explicitly wants spam events captured into
        // the macro. Default behaviour (spamRecordToMacro = false) keeps
        // saved macros free of clickbot pollution so they remain
        // useful as standalone replays.
        if (mgr->spamSuppressRecord && !mgr->spamRecordToMacro) return;

        bool p2 = !p1 && m_levelSettings->m_twoPlayerMode && m_gameState.m_isDualMode;
        if (!shouldRecord(mgr, down, button, p2)) return;

        // Record the raw current frame so playback can fire each input
        // exactly when m_currentProgress matches at replay time. The
        // playback hook uses `frame <= currentProgress` so the input
        // lands on the same tick it was originally pressed on. This is
        // the matcool/ReplayBot + FigmentBoy/zBot canonical convention.
        mgr->currentReplay->addInput(
            static_cast<int>(m_gameState.m_currentProgress),
            button, p2, down
        );
    }
};

class $modify(zRecPL, PlayLayer) {
    bool init(GJGameLevel* lvl, bool useReplay, bool dontCreateObjects) {
        zBot* mgr = zBot::get();

        if (mgr->state == RECORD) {
            mgr->createNewReplay(lvl);
        }

        // Re-entering a level with PLAYBACK armed: rewind the cursor so
        // the macro plays from the very first input. The playback hook
        // watches the playbackEpoch counter and resyncs on change.
        if (mgr->state == PLAYBACK) {
            mgr->requestPlaybackRestart();
        }

        if (!PlayLayer::init(lvl, useReplay, dontCreateObjects)) return false;

        // Auto-Safe Mode: force practice mode at level init so any
        // checkpoint we create (or the level being completed) is treated
        // as practice and is NOT submitted as a normal-mode completion.
        // This is the "no-ban" safety net for showcases and macro work.
        if (mgr->autoSafeMode) {
            m_isPracticeMode = true;
        }

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        zBot* mgr = zBot::get();
        if (mgr->state != RECORD || !mgr->currentReplay) return;

        // After resetLevel(), m_currentProgress reflects either the
        // start of the level (full restart) or the last activated
        // checkpoint frame (death + respawn). Wiping inputs after
        // that point gives us xdBot/zBot-style checkpoint recording:
        // play -> checkpoint -> die -> respawn -> overwrite the bad
        // attempt and keep recording over it.
        //
        // This is the core of how a "perfect macro" gets built: every
        // failed attempt is replaced by the next one, so the final
        // saved file only contains the successful run.
        int frame = static_cast<int>(m_gameState.m_currentProgress);
        mgr->currentReplay->purgeAfter(frame);

        // Re-derive what's actually held at the respawn point from the
        // surviving inputs so dedupe sees the truth.
        mgr->resetButtonStateAfterFrame(frame);

        // The player object is reset to a clean state on respawn — it
        // is NOT holding any button, regardless of what was being held
        // mid-attempt. To keep the macro consistent with that physical
        // reality, synthesize a release event at the respawn frame for
        // every button the surviving inputs say is still held. Without
        // this, a button held across the death (e.g. the right-arrow
        // on a ship part) would never get released in the macro and
        // dedupe would silently drop the next press.
        bool dual = m_gameState.m_isDualMode &&
                    m_levelSettings &&
                    m_levelSettings->m_twoPlayerMode;

        auto releaseAllHeld = [&](bool* held, bool p2) {
            for (int b = 1; b <= 7; ++b) {
                if (!held[b]) continue;
                mgr->currentReplay->addInput(frame, b, p2, false);
                held[b] = false;
            }
        };
        releaseAllHeld(mgr->p1ButtonHeld, false);
        if (dual) releaseAllHeld(mgr->p2ButtonHeld, true);

        if (m_player1) m_player1->m_isDashing = false;
        if (dual && m_player2) m_player2->m_isDashing = false;
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        zBot* mgr = zBot::get();
        // Auto-Safe also blocks the actual death event during macro work,
        // so the level keeps playing back / recording without the player
        // exploding mid-attempt. The level is visually unmodified.
        if (mgr->autoSafeMode && (mgr->state == RECORD || mgr->state == PLAYBACK)) {
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }

    void levelComplete() {
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            mgr->levelCompleted = true;
            if (mgr->autoSave) {
                mgr->currentReplay->save(mgr->minHoldFrames, mgr->minGapFrames);
                // Mark that the completion path already wrote the file
                // so onExit doesn't redundantly save the exact same
                // macro a second time when the level scene is torn down.
                mgr->autoSavedThisRun = true;
                Notification::create("Perfect macro saved",
                    NotificationIcon::Success, 1.5f)->show();
            }
        }

        // Auto-Safe Mode at the finish line: swallow the real
        // levelComplete call so GD never counts the run, never grants
        // stars/orbs/coins/diamonds and never submits to leaderboards.
        // We just pop a tiny notification so the player knows the macro
        // reached the end and the safety net kicked in.
        if (mgr->autoSafeMode) {
            Notification::create("safe mode :3", NotificationIcon::Success, 1.5f)->show();
            return;
        }

        PlayLayer::levelComplete();
    }

    void onExit() {
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay && mgr->autoSave
            && !mgr->autoSavedThisRun) {
            // Perfect-run gate: only auto-save if the player legitimately
            // finished the level. Quitting mid-attempt leaves the saved
            // macro on disk untouched so a half-baked retry session
            // can't clobber a previously saved masterpiece.
            //
            // The autoSavedThisRun guard avoids the double-write that
            // used to happen when levelComplete() already saved the
            // macro and then onExit() saved the identical file again
            // a moment later as the scene tore down.
            //
            // Users can still hit "Save" manually from the GUI to write
            // an in-progress recording — that path bypasses this gate.
            if (!mgr->perfectRunOnly || mgr->levelCompleted) {
                mgr->currentReplay->save(mgr->minHoldFrames, mgr->minGapFrames);
            }
        }
        // Clear the dedupe flag so the next level's auto-save isn't
        // accidentally suppressed.
        mgr->autoSavedThisRun = false;
        PlayLayer::onExit();
    }
};
