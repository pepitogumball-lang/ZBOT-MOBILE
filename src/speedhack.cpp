#include "GMacro.hpp"
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;

// Custom scheduler for speedhack implementation
class $modify(zSpeedSched, cocos2d::CCScheduler) {
  void update(float dt) {
    GMacro* mgr = GMacro::get();
    if (mgr->speedHackEnabled && mgr->speed > 0.0) {
      CCScheduler::update(dt * static_cast<float>(mgr->speed));
    } else {
      CCScheduler::update(dt);
    }
  }
};
