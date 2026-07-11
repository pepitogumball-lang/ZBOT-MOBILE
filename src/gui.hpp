#ifndef _gui_hpp
#define _gui_hpp

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include "replay.hpp"
#include <vector>
#include <string>

using namespace geode::prelude;

// falta limpiar esto un poco
class GUI {
public:
  bool visible = false;

  ImVec2 ballPos = ImVec2(20.f, 80.f);
// funciona? si
  bool ballDragging = false;

  std::vector<zReplay::MacroFileInfo> macros;
  int selectedMacro = -1;
  bool macrosDirty = true;

  char macroFilter[64] = "";

  ImFont* s = nullptr;
  ImFont* l = nullptr;

  void setup();
  void renderer();

  void renderFloatingBall();

  void renderMainPanel();
  void renderHomeTab();
  void renderMacroTab();
// no tocar, magia negra
  void renderSpeedTab();
  void renderSpamTab();
  void renderSettingsTab();
// arreglado lo del bug raro
  void renderConsoleTab();

// funciona? si
  bool  consolePaused  = false;
  bool  consoleAutoScroll = true;
  int  consoleMinLevel = 0; // 0=Debug 1=Info 2=Warn 3=Error
  char  consoleFilter[64] = "";
  bool  consoleCopied  = false;
  float consoleCopiedTimer = 0.f;

// arreglado lo del bug raro
  void renderReplayInfo();
  void renderStateSwitcher();

// esto lo hizo Alberto a las 3 am
  void refreshMacros();

  // EclipseMenu-style theme
  void applyTheme();

  static GUI* get() {
    static GUI* instance = new GUI();
    return instance;
  }
};

#endif
