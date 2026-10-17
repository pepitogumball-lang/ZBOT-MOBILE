#include "GMacro.hpp"
#include <Geode/modify/FMODAudioEngine.hpp>
#include <algorithm>

using namespace geode::prelude;

// FMOD audio pitch adjustment for speedhack support
class $modify(zSpeedAudio, FMODAudioEngine) {
  struct Fields {
    float pitch = 1.f;
  };

  void update(float delta) {
    FMODAudioEngine::update(delta);

    GMacro* mgr = GMacro::get();
    float pitch = 1.f;
    if (mgr->speedHackEnabled && mgr->speedHackAudio && mgr->speed > GMACRO_SPEED_MIN) {
      pitch = std::clamp(static_cast<float>(mgr->speed), 0.5f, 2.0f);
    }

    if (pitch == m_fields->pitch) return;
    m_fields->pitch = pitch;

    FMOD::ChannelGroup* group = nullptr;
    if (m_system && m_system->getMasterChannelGroup(&group) == FMOD_OK && group) {
      group->setPitch(pitch);
    }
  }
};
