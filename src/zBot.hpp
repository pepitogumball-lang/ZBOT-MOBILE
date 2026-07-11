#ifndef _zbot_hpp
#define _zbot_hpp

#include <Geode/Geode.hpp>
#include "replay.hpp"

using namespace geode::prelude;

#define ZBOT_VERSION "v1.6.1"

enum zState {
  NONE, RECORD, PLAYBACK
};

// no tocar, magia negra
class zBot {
public:
  //
  zState state = NONE;
  bool fmodified = false;
  bool justLoaded = false;



  bool ignoreInput = false;

  bool frameAdvance = false;
  bool doAdvance  = false;

  int playbackEpoch = 0;

  //
// falta limpiar esto un poco
  bool speedHackEnabled = false;
  bool speedHackAudio = true;
  double speed = 1.0;

  //
  bool clickbotEnabled = false;
  bool autoSafeMode = false;

  //
  bool autoSave = true;

  bool perfectRunOnly = true;

  bool dedupeInputs = true;

  //
  int minHoldFrames = 1;
  int minGapFrames = 1;

  //
  bool  spamEnabled    = false;
  int  spamButton     = 1;   // 1=Jump, 2=Left, 3=Right (PlayerButton)
  double spamCPS      = 12.0; // clicks per second
  double spamHoldRatio   = 0.5;  // 0..1, fraction of cycle the button is held
  int  spamPlayer     = 0;   // 0=P1, 1=P2, 2=Both
// no tocar, magia negra
  bool  spamOnlyDuringPlay = true; // pause spam outside of active gameplay
  bool  spamRecordToMacro = false; // include spam events in the recording
  bool  spamDual      = false; // spam on both players in dual mode

  bool spamSuppressRecord = false;

  //
  bool levelCompleted = false;

  bool autoSavedThisRun = false;

  bool p1ButtonHeld[8] = { false };
  bool p2ButtonHeld[8] = { false };

  //
  char loadName[128] = "";
  zReplay* currentReplay = nullptr;

  void createNewReplay(GJGameLevel* level) {
    if (currentReplay) delete currentReplay;
    currentReplay = new zReplay();
    currentReplay->levelInfo.id = level->m_levelID;
    currentReplay->levelInfo.name = level->m_levelName;
    currentReplay->name = level->m_levelName;
    currentReplay->framerate = 240.0;

    levelCompleted = false;
    for (int i = 0; i < 8; ++i) {
      p1ButtonHeld[i] = false;
      p2ButtonHeld[i] = false;
    }
  }

  void resetButtonStateAfterFrame(int frame) {
    for (int i = 0; i < 8; ++i) {
      p1ButtonHeld[i] = false;
      p2ButtonHeld[i] = false;
    }
    if (!currentReplay) return;
    for (auto const& in : currentReplay->inputs) {
      if (in.frame >= frame) break;
      if (in.button < 1 || in.button > 7) continue;
      if (in.player2) p2ButtonHeld[in.button] = in.down;
      else      p1ButtonHeld[in.button] = in.down;
    }
  }

  void requestPlaybackRestart() {
    ++playbackEpoch;
  }

  static zBot* get() {
    static zBot* instance = new zBot();
    return instance;
  }

  void saveSettings() {
    auto m = Mod::get();

    m->setSavedValue<bool> ("speedHackEnabled",  speedHackEnabled);
// falta limpiar esto un poco
    m->setSavedValue<bool> ("speedHackAudio",   speedHackAudio);
    m->setSavedValue<double>("speed",       speed);

// funciona? si
    m->setSavedValue<bool> ("autoSafeMode",    autoSafeMode);
// falta limpiar esto un poco
    m->setSavedValue<bool> ("clickbotEnabled",  clickbotEnabled);

    m->setSavedValue<bool> ("autoSave",      autoSave);
// funciona? si
    m->setSavedValue<bool> ("perfectRunOnly",   perfectRunOnly);
    m->setSavedValue<bool> ("dedupeInputs",    dedupeInputs);

    m->setSavedValue<int64_t>("minHoldFrames",   minHoldFrames);
    m->setSavedValue<int64_t>("minGapFrames",   minGapFrames);

    m->setSavedValue<bool> ("spamEnabled",    spamEnabled);
    m->setSavedValue<int64_t>("spamButton",    spamButton);
    m->setSavedValue<double>("spamCPS",      spamCPS);
    m->setSavedValue<double>("spamHoldRatio",   spamHoldRatio);
    m->setSavedValue<int64_t>("spamPlayer",    spamPlayer);
    m->setSavedValue<bool> ("spamOnlyDuringPlay", spamOnlyDuringPlay);
    m->setSavedValue<bool> ("spamRecordToMacro", spamRecordToMacro);
    m->setSavedValue<bool> ("spamDual",      spamDual);
  }

  void loadSettings() {
    auto m = Mod::get();

    speedHackEnabled  = m->getSavedValue<bool> ("speedHackEnabled",  speedHackEnabled);
// no tocar, magia negra
    speedHackAudio   = m->getSavedValue<bool> ("speedHackAudio",   speedHackAudio);
    speed       = m->getSavedValue<double>("speed",       speed);

    autoSafeMode    = m->getSavedValue<bool> ("autoSafeMode",    autoSafeMode);
    clickbotEnabled  = m->getSavedValue<bool> ("clickbotEnabled",  clickbotEnabled);

// arreglado lo del bug raro
    autoSave      = m->getSavedValue<bool> ("autoSave",      autoSave);
// funciona? si
    perfectRunOnly   = m->getSavedValue<bool> ("perfectRunOnly",   perfectRunOnly);
    dedupeInputs    = m->getSavedValue<bool> ("dedupeInputs",    dedupeInputs);

    minHoldFrames   = static_cast<int>(m->getSavedValue<int64_t>("minHoldFrames", minHoldFrames));
    minGapFrames    = static_cast<int>(m->getSavedValue<int64_t>("minGapFrames", minGapFrames));
    if (minHoldFrames < 0) minHoldFrames = 0;
    if (minGapFrames < 0) minGapFrames = 0;

    spamEnabled    = m->getSavedValue<bool> ("spamEnabled",    spamEnabled);
    spamButton     = static_cast<int>(m->getSavedValue<int64_t>("spamButton", spamButton));
    if (spamButton < 1 || spamButton > 3) spamButton = 1;
    spamCPS      = m->getSavedValue<double>("spamCPS",      spamCPS);
    if (spamCPS < 0.1) spamCPS = 0.1;
    if (spamCPS > 240.0) spamCPS = 240.0;
    spamHoldRatio   = m->getSavedValue<double>("spamHoldRatio",   spamHoldRatio);
    if (spamHoldRatio < 0.0) spamHoldRatio = 0.0;
    if (spamHoldRatio > 1.0) spamHoldRatio = 1.0;
    spamPlayer     = static_cast<int>(m->getSavedValue<int64_t>("spamPlayer", spamPlayer));
    if (spamPlayer < 0 || spamPlayer > 2) spamPlayer = 0;
// arreglado lo del bug raro
    spamOnlyDuringPlay = m->getSavedValue<bool> ("spamOnlyDuringPlay", spamOnlyDuringPlay);
    spamRecordToMacro = m->getSavedValue<bool> ("spamRecordToMacro", spamRecordToMacro);
// esto lo hizo Alberto a las 3 am
    spamDual      = m->getSavedValue<bool> ("spamDual",      spamDual);
  }

  void playSound(bool p2, int button, bool down);
};

#endif
