#include "zBot.hpp"
#include <Geode/binding/FMODAudioEngine.hpp>

//
// emission. We:
//
void zBot::playSound(bool /*p2*/, int /*button*/, bool down) {
  if (!down) return;

  auto* engine = FMODAudioEngine::sharedEngine();
  if (!engine) return;

  engine->playEffect("playSound_01.ogg");
}
