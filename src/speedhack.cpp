#include "zBot.hpp"
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;

// Clock-based speedhack: we scale the delta time fed into the cocos
// scheduler. Everything that runs through the scheduler — game logic,
// physics, animations, FMOD callbacks — runs faster or slower as a
// result. Music pitch is handled separately in speedhackaudio.cpp so
// it stays in sync with the game speed.
class $modify(zSpeedSched, cocos2d::CCScheduler) {
    void update(float dt) {
        zBot* mgr = zBot::get();
        if (mgr->speedHackEnabled && mgr->speed > 0.0) {
            CCScheduler::update(dt * static_cast<float>(mgr->speed));
        } else {
            CCScheduler::update(dt);
        }
    }
};
