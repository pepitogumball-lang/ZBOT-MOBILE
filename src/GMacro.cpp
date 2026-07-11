#include "GMacro.hpp"
#include <Geode/binding/FMODAudioEngine.hpp>

//
// emission. We:
//
void GMacro::playSound(bool /*p2*/, int /*button*/, bool down) {
  if (!down) return;

  auto* engine = FMODAudioEngine::sharedEngine();
  if (!engine) return;

  engine->playEffect("playSound_01.ogg");
}
