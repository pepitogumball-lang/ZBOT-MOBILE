#include "zBot.hpp"
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class $modify(zBGL, GJBaseGameLayer) {
    struct Fields {
        bool p1J = false, p1L = false, p1R = false;
        bool p2J = false, p2L = false, p2R = false;
    };

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD) {
            int frame = (int)m_gameState.m_currentProgress; // Simplification for now
            
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

            if (m_levelSettings->m_twoPlayerMode && m_player2) {
                recordInput(m_player2, true, 1, m_fields->p2J);
                if (m_levelSettings->m_platformerMode) {
                    recordInput(m_player2, true, 2, m_fields->p2L);
                    recordInput(m_player2, true, 3, m_fields->p2R);
                }
            }
        }
    }
};

#include <Geode/modify/PauseLayer.hpp>

class $modify(zPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) menu = this->getChildByID("left-button-menu");

        if (menu) {
            auto btnSprite = CircleButtonSprite::createWithSpriteFrameName("geode.loader/geode-logo-outline.png", 0.7f, CircleBaseColor::Green, CircleBaseSize::Medium);
            auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(zPauseLayer::onZBotMenu));
            btn->setID("zbot-button");
            menu->addChild(btn);
            menu->updateLayout();
        }
    }

    void onZBotMenu(CCObject*) {
        GUI::get()->visible = !GUI::get()->visible;
    }
};

class $modify(zPL, PlayLayer) {

    bool init(GJGameLevel* lvl, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(lvl, useReplay, dontCreateObjects)) return false;
        
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD) {
            mgr->createNewReplay(lvl);
        }
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            int frame = (int)m_gameState.m_currentProgress;
            mgr->currentReplay->purgeAfter(frame);
        }
    }

    void onExit() {
        zBot* mgr = zBot::get();
        if (mgr->state == RECORD && mgr->currentReplay) {
            mgr->currentReplay->save();
        }
        PlayLayer::onExit();
    }
};
