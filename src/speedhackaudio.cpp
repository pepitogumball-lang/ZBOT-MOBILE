#include "GMacro.hpp"
#include <Geode/modify/FMODAudioEngine.hpp>

using namespace geode::prelude;

// esto lo hizo Alberto a las 3 am
class $modify(zSpeedAudio, FMODAudioEngine) {
  struct Fields {
    float pitch = 1.f;
  };

  void update(float delta) {
    FMODAudioEngine::update(delta);

    GMacro* mgr = GMacro::get();
    float pitch = (mgr->speedHackEnabled && mgr->speedHackAudio)
      ? static_cast<float>(mgr->speed)
      : 1.f;

    if (pitch == m_fields->pitch) return;
    m_fields->pitch = pitch;

    FMOD::ChannelGroup* group = nullptr;
    if (m_system && m_system->getMasterChannelGroup(&group) == FMOD_OK && group) {
      group->setPitch(pitch);
    }
  }
};
