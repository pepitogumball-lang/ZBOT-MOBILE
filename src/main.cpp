#include "zBot.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CheckpointObject.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

struct PracticeInfo {
    std::vector<int> checkpointFrames;
};
static PracticeInfo practice;

class $modify(zBGL, GJBaseGameLayer) {
    struct Fields {
        bool p1J = false, p1L = false, p1R = false;
        bool p2J = false, p2L = false, p2R = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            int frame = static_cast<int>(m_gameState.m_currentProgress * mgr->tps);
            auto recordInput = [&](PlayerObject* p, bool p2, int btn, bool& lastState) {
                if (!p) return;
                bool currentState = p->m_holdingButtons[btn];
                if (currentState != lastState) {
                    mgr->currentReplay->addInput(frame, btn, p2, currentState);
                    lastState = currentState;
                }
            };
            recordInput(m_player1, false, 1, m_fields->p1J);
            if (m_levelSettings->m_platformerMode) {
                recordInput(m_player1, false, 2, m_fields->p1L);
                recordInput(m_player1, false, 3, m_fields->p1R);
            }
        }
    }
};

class $modify(zPL, PlayLayer) {
    void createCheckpoint() {
        PlayLayer::createCheckpoint();
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD) {
            practice.checkpointFrames.push_back(static_cast<int>(m_gameState.m_currentProgress * mgr->tps));
        }
    }
    void removeLastCheckpoint() {
        PlayLayer::removeLastCheckpoint();
        if (!practice.checkpointFrames.empty()) practice.checkpointFrames.pop_back();
    }
    void resetLevel() {
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay && m_isPracticeMode) {
            int resumeFrame = 0;
            if (m_checkpoints->count() > 0 && !practice.checkpointFrames.empty()) {
                resumeFrame = practice.checkpointFrames.back();
            }
            mgr->currentReplay->purgeAfter(resumeFrame);
        }
        PlayLayer::resetLevel();
    }

    // Auto-Safe Mode: prevent the player from dying while recording or
    // playing back a macro. Useful for clean showcases. The level itself
    // stays visually unmodified — we only skip the death event.
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        zBot* mgr = zBot::get();
        if (mgr->autoSafeMode && (mgr->state == RECORD || mgr->state == PLAYBACK)) {
            return;
        }
        PlayLayer::destroyPlayer(player, obj);
    }
};
