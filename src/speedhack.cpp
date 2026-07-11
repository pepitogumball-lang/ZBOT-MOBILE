#include "GMacro.hpp"
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;

// esto lo hizo Alberto a las 3 am
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
