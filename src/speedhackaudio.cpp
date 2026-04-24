#include "zBot.hpp"
#include <Geode/modify/FMODAudioEngine.hpp>

using namespace geode::prelude;

// When the speedhack is on, music and SFX automatically follow by
// applying a pitch multiplier to FMOD's master channel group. This is
// what keeps a 0.25x or 0.1x slow-mo run from sounding broken.
class $modify(zSpeedAudio, FMODAudioEngine) {
    struct Fields {
        float pitch = 1.f;
    };

    void update(float delta) {
        FMODAudioEngine::update(delta);

        zBot* mgr = zBot::get();
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
