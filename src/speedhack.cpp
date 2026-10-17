#include "GMacro.hpp"
#include <Geode/modify/CCScheduler.hpp>
#include <algorithm>

using namespace geode::prelude;

// Custom scheduler for speedhack implementation
class $modify(zSpeedSched, cocos2d::CCScheduler) {
  void update(float dt) {
    GMacro* mgr = GMacro::get();
    float s = 1.f;
    if (mgr->speedHackEnabled && mgr->speed > GMACRO_SPEED_MIN) {
      s = static_cast<float>(std::clamp(mgr->speed, GMACRO_SPEED_MIN, GMACRO_SPEED_MAX));
    }
    CCScheduler::update(dt * s);
  }
};
