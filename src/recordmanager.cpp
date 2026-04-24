#include "zBot.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

class $modify(zRecGJBGL, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool p1) {
        GJBaseGameLayer::handleButton(down, button, p1);

        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            bool p2 = !p1 && m_levelSettings->m_twoPlayerMode && m_gameState.m_isDualMode;
            mgr->currentReplay->addInput(
                static_cast<int>(m_gameState.m_currentProgress) - 1,
                button, p2, down
            );
        }
    }
};

class $modify(zRecPL, PlayLayer) {
    bool init(GJGameLevel* lvl, bool useReplay, bool dontCreateObjects) {
        zBot* mgr = zBot::get();

        if (mgr->state == RECORD) {
            mgr->createNewReplay(lvl);
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
        if (mgr->state == RECORD && mgr->currentReplay) {
            // After resetLevel(), m_currentProgress reflects either the
            // start of the level (full restart) or the last activated
            // checkpoint frame (death + respawn). Wiping inputs after
            // that point gives us xdBot/zBot-style checkpoint recording:
            // play -> checkpoint -> die -> respawn -> overwrite the bad
            // attempt and keep recording over it.
            int frame = static_cast<int>(m_gameState.m_currentProgress);
            mgr->currentReplay->purgeAfter(frame);
            mgr->currentReplay->addInput(frame, static_cast<int>(PlayerButton::Jump), false, false);
            if (m_player1) m_player1->m_isDashing = false;

            if (m_gameState.m_isDualMode && m_levelSettings->m_twoPlayerMode) {
                mgr->currentReplay->addInput(frame, static_cast<int>(PlayerButton::Jump), true, false);
                if (m_player2) m_player2->m_isDashing = false;
            }
        }
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
            mgr->currentReplay->save();
        }
        PlayLayer::levelComplete();
    }

    void onExit() {
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            mgr->currentReplay->save();
        }
        PlayLayer::onExit();
    }
};
