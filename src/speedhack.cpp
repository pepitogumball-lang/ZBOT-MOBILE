#include "GMacro.hpp"
#include <Geode/modify/CCScheduler.hpp>
#include <algorithm>

using namespace geode::prelude;

// Custom scheduler for speedhack implementation
class $modify(zSpeedSched, cocos2d::CCScheduler) {
  void update(float dt) {
    GMacro* mgr = GMacro::get();
    float s = 1.f;
    if (mgr->speedHackEnabled && mgr->speed > 0.05) {
      s = static_cast<float>(std::clamp(mgr->speed, 0.05, 8.0));
    }
    CCScheduler::update(dt * s);
  }
};
