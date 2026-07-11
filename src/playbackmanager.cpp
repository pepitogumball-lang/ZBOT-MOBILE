#include "GMacro.hpp"
#include "replay.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <cocos2d.h>
#include <cmath>
#include <algorithm>
#include "logger.hpp"

using namespace geode::prelude;

class $modify(zPlayGJBGL, GJBaseGameLayer) {
  struct Fields {
    int currIndex   = 0;
    int clickBotIndex = 0;
    int   lastEpoch = -1;
    zReplay* lastReplay = nullptr;
    bool spamHeld[2]  = { false, false };
    int spamAnchor  = -1;
    bool spamWasOn   = false;
  };

// no tocar, magia negra
  void processCommands(float dt, bool isHalfTick, bool isLastTick) {
    GMacro* mgr = GMacro::get();
    auto* pl = PlayLayer::get();
    bool inLevel = pl != nullptr;

    //
    if (mgr->frameAdvance && inLevel && !mgr->doAdvance) {
      mgr->doAdvance = false;
      return; 
    }
    mgr->doAdvance = false;

    int frame = static_cast<int>(m_gameState.m_currentProgress) / 2;

    //
    if (mgr->state == PLAYBACK && mgr->currentReplay) {
      bool epochChanged  = (m_fields->lastEpoch != mgr->playbackEpoch);
// arreglado lo del bug raro
      bool replayChanged = (m_fields->lastReplay != mgr->currentReplay);

      if (epochChanged || replayChanged) {
        m_fields->lastEpoch   = mgr->playbackEpoch;
        m_fields->lastReplay  = mgr->currentReplay;
        m_fields->currIndex   = 0;
        m_fields->clickBotIndex = 0; // Removed: ghost flag

        ZLOG_INFO("EPOCH", "cursor reset"
           " | frame=" << frame
           << " | inLevel=" << inLevel
           << " | inputs=" << mgr->currentReplay->inputs.size()
           << " | hasCheckpoint=" << (inLevel && pl->m_currentCheckpoint != nullptr)
           << " | epoch=" << mgr->playbackEpoch);

// funciona? si
        bool isMidLevelJoin = inLevel
          && pl->m_currentCheckpoint != nullptr
          && frame > 1;
        if (isMidLevelJoin) {
          auto& inputs = mgr->currentReplay->inputs;
          while (m_fields->currIndex < (int)inputs.size() &&
              inputs[m_fields->currIndex].frame < frame) {
            m_fields->currIndex++;
          }
          m_fields->clickBotIndex = m_fields->currIndex;
        }
      }

      auto& inputs = mgr->currentReplay->inputs;

      while (m_fields->currIndex < (int)inputs.size() &&
          inputs[m_fields->currIndex].frame <= frame) {
        auto& input = inputs[m_fields->currIndex++];
        
        ZLOG_INFO("PLAYBACK", "FIRE"
           " frame=" << input.frame
           << " btn=" << input.button
           << " down=" << input.down
           << " p2=" << input.player2
           << " | curFrame=" << frame
           << " | idx=" << (m_fields->currIndex-1));
        #ifdef GEODE_IS_MOBILE
        m_allowedButtons.clear();
        #endif
        GJBaseGameLayer::handleButton(input.down, input.button, !input.player2);
      }

      if (frame % 60 == 0) {
        int remaining = (int)inputs.size() - m_fields->currIndex;
        auto lvl = (remaining == 0 && !inputs.empty())
              ? ZLogLevel::Warn : ZLogLevel::Debug;
        ZBotLogger::get().log(lvl, "PLAYBACK",
          std::string("tick frame=") + std::to_string(frame)
          + " cursor=" + std::to_string(m_fields->currIndex)
          + "/" + std::to_string((int)inputs.size())
          + " remaining=" + std::to_string(remaining));
      }

      int offset = static_cast<int>(mgr->currentReplay->framerate * 0.1);
      while (m_fields->clickBotIndex < (int)inputs.size() &&
          inputs[m_fields->clickBotIndex].frame <= frame + offset) {
        auto click = inputs[m_fields->clickBotIndex++];
        if (mgr->clickbotEnabled) {
          mgr->playSound(click.player2, click.button, click.down);
        }
      }
    } else {
      m_fields->lastEpoch = mgr->playbackEpoch;
      m_fields->lastReplay = mgr->currentReplay;
    }

    bool runParent = !mgr->ignoreInput;
    if (runParent) {
      GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }

    //
    if (mgr->spamEnabled && inLevel) {
      bool gateOk = true;
      if (mgr->spamOnlyDuringPlay) {
        gateOk = !pl->m_isPaused;
      }

      if (gateOk) {
        if (!m_fields->spamWasOn || frame < m_fields->spamAnchor) {
          m_fields->spamAnchor = frame;
          m_fields->spamHeld[0] = false;
          m_fields->spamHeld[1] = false;
        }
        m_fields->spamWasOn = true;

        double tps = (mgr->currentReplay && mgr->currentReplay->framerate > 0.0)
          ? mgr->currentReplay->framerate
          : 240.0;
        double cps = std::clamp(mgr->spamCPS, 0.1, 240.0);
        double cycle = tps / cps;
        if (cycle < 1.0) cycle = 1.0;
        double hold = cycle * std::clamp(mgr->spamHoldRatio, 0.0, 1.0);
        if (hold < 1.0 && mgr->spamHoldRatio > 0.0) hold = 1.0;
        if (hold >= cycle) hold = cycle - 0.001;

        double rel = static_cast<double>(frame - m_fields->spamAnchor);
        double phase = std::fmod(rel, cycle);
        if (phase < 0.0) phase += cycle;
// arreglado lo del bug raro
        bool wantDown = (phase < hold);

        bool dual = m_gameState.m_isDualMode &&
              m_levelSettings &&
              m_levelSettings->m_twoPlayerMode;

        int button = mgr->spamButton;
        if (button < 1 || button > 3) button = 1;

// esto lo hizo Alberto a las 3 am
        auto driveButton = [&](int playerIdx, bool isP2) {
          bool& cur = m_fields->spamHeld[playerIdx];
          if (cur == wantDown) return;
          cur = wantDown;

          mgr->spamSuppressRecord = true;
          #ifdef GEODE_IS_MOBILE
          m_allowedButtons.clear();
          #endif
          GJBaseGameLayer::handleButton(wantDown, button, !isP2);
          mgr->spamSuppressRecord = false;
        };

        // spamPlayer: 0=P1, 1=P2, 2=Both
        // spamDual solo aplica si el nivel esta en modo dual
        bool doP1 = (mgr->spamPlayer == 0 || mgr->spamPlayer == 2);
        bool doP2 = (mgr->spamPlayer == 1 || mgr->spamPlayer == 2) && dual;
        if (doP1) driveButton(0, false);
        if (doP2) driveButton(1, true);
      }
    } else {
      m_fields->spamWasOn = false;
    }
  }
};
