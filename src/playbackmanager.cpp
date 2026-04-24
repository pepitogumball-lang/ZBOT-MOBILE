#include "zBot.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
using namespace geode::prelude;

static int currIndex = 0;
static int clickBotIndex = 0;

class $modify(zPlayGJBGL, GJBaseGameLayer) {
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (!zBot::get()->ignoreInput) {
            GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }

        zBot* mgr = zBot::get();

        if (mgr->state == PLAYBACK && mgr->currentReplay) {
            int frame = static_cast<int>(m_gameState.m_currentProgress);

            while (currIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[currIndex].frame < frame) {
                auto input = mgr->currentReplay->inputs[currIndex++];
                GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
            }

            // Tiny lookahead so the click sound effect feels in sync
            // when the clickbot is on. Pure cosmetic.
            int offset = static_cast<int>(mgr->currentReplay->framerate * 0.1);
            while (clickBotIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[clickBotIndex].frame < frame + offset) {
                auto click = mgr->currentReplay->inputs[clickBotIndex++];
                if (mgr->clickbotEnabled) {
                    mgr->playSound(click.player2, click.button, click.down);
                }
            }
        }
    }
};

class $modify(zPlayPL, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();

        zBot* mgr = zBot::get();
        if (mgr->state == PLAYBACK && mgr->currentReplay) {
            currIndex = 0;
            clickBotIndex = 0;

            int frame = static_cast<int>(m_gameState.m_currentProgress);
            while (currIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[currIndex].frame < frame) {
                currIndex++;
            }
            while (clickBotIndex < (int)mgr->currentReplay->inputs.size() &&
                   mgr->currentReplay->inputs[clickBotIndex].frame < frame) {
                clickBotIndex++;
            }
        }
    }
};
