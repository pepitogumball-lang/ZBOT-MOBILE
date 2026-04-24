#ifndef _zbot_hpp
#define _zbot_hpp

#include <Geode/Geode.hpp>
#include "replay.hpp"

using namespace geode::prelude;

enum zState {
    NONE, RECORD, PLAYBACK
};

class zBot {
public:
    zState state = NONE;
    bool fmodified = false;
    float extraTPS = 0.f;

    bool disableRender = false;
    bool ignoreBypass = false;
    bool justLoaded = false;
    bool ignoreInput = false;
    bool frameAdvance = false;
    bool doAdvance = false;

    bool speedHackEnabled = false;
    bool speedHackAudio = true;
    bool clickbotEnabled = false;
    bool autoSafeMode = false;

    double speed = 1.0;
    double tps = 240.0;
    char loadName[128] = "";
    zReplay* currentReplay = nullptr;

    void createNewReplay(GJGameLevel* level) {
        if (currentReplay) delete currentReplay;
        currentReplay = new zReplay();
        currentReplay->levelInfo.id = level->m_levelID;
        currentReplay->levelInfo.name = level->m_levelName;
        currentReplay->name = level->m_levelName;
        // Macros are saved at the canonical 240 TPS so playback always
        // produces a normal-speed run, even if speedhack was on while
        // recording. Inputs are stored by in-game frame number, which is
        // not affected by the real-time speedhack.
        currentReplay->framerate = 240.0;
    }

    static zBot* get() {
        static zBot* instance = new zBot();
        return instance;
    }

    void playSound(bool p2, int button, bool down);
};

#endif
