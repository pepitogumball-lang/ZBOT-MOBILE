#include "gui.hpp"
#include "GMacro.hpp"
#include "replay.hpp"
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include "logger.hpp"

using namespace geode::prelude;

//    fromFile("my.macro"))
static int blockBadNameChars(ImGuiInputTextCallbackData* data) {
  auto c = data->EventChar;
  if (c == '.' || c == '/' || c == '\\') return 1; // reject
  return 0;
}

static std::string sanitizeName(const char* raw) {
  std::string out(raw);
  out.erase(std::remove_if(out.begin(), out.end(), [](char c){
    return c == '.' || c == '/' || c == '\\';
  }), out.end());
  return out;
}

//
// EclipseMenu-inspired theme
//
//
// Alberto se paso con el morado pero queda bien
void GUI::applyTheme() {
  ImGuiStyle& s = ImGui::GetStyle();

  s.WindowRounding  = 8.0f;
  s.ChildRounding  = 6.0f;
  s.FrameRounding  = 6.0f;
  s.PopupRounding  = 6.0f;
  s.GrabRounding   = 6.0f;
  s.TabRounding   = 6.0f;
  s.ScrollbarRounding= 8.0f;
  s.ScrollbarSize  = 14.0f;
  s.WindowBorderSize = 0.0f;
  s.FrameBorderSize = 0.0f;
  s.WindowPadding  = ImVec2(12.f, 12.f);
  s.FramePadding   = ImVec2(10.f, 7.f);
  s.ItemSpacing   = ImVec2(8.f, 7.f);
  s.ItemInnerSpacing = ImVec2(6.f, 6.f);
  s.IndentSpacing  = 18.f;

  ImVec4* c = s.Colors;
  auto rgba = [](int r, int g, int b, float a){
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, a);
  };

  c[ImGuiCol_Text]         = rgba(232, 232, 240, 1.0f);
  c[ImGuiCol_TextDisabled]     = rgba(140, 140, 156, 1.0f);
  c[ImGuiCol_WindowBg]       = rgba( 18, 18, 26, 0.96f);
  c[ImGuiCol_ChildBg]        = rgba( 24, 24, 34, 1.00f);
  c[ImGuiCol_PopupBg]        = rgba( 22, 22, 32, 0.98f);
  c[ImGuiCol_Border]        = rgba( 60, 60, 90, 0.50f);

  c[ImGuiCol_FrameBg]        = rgba( 30, 30, 44, 1.00f);
  c[ImGuiCol_FrameBgHovered]    = rgba( 50, 45, 85, 1.00f);
  c[ImGuiCol_FrameBgActive]     = rgba( 70, 60, 120, 1.00f);

  c[ImGuiCol_TitleBg]        = rgba( 22, 22, 34, 1.00f);
  c[ImGuiCol_TitleBgActive]     = rgba( 60, 45, 110, 1.00f);
  c[ImGuiCol_TitleBgCollapsed]   = rgba( 18, 18, 28, 1.00f);

  c[ImGuiCol_MenuBarBg]       = rgba( 22, 22, 32, 1.00f);

  c[ImGuiCol_ScrollbarBg]      = rgba( 18, 18, 28, 0.60f);
  c[ImGuiCol_ScrollbarGrab]     = rgba( 70, 60, 110, 1.00f);
  c[ImGuiCol_ScrollbarGrabHovered] = rgba( 95, 80, 150, 1.00f);
  c[ImGuiCol_ScrollbarGrabActive]  = rgba(115, 100, 180, 1.00f);

  c[ImGuiCol_CheckMark]       = rgba(180, 150, 255, 1.00f);
  c[ImGuiCol_SliderGrab]      = rgba(140, 110, 230, 1.00f);
  c[ImGuiCol_SliderGrabActive]   = rgba(170, 140, 255, 1.00f);

  c[ImGuiCol_Button]        = rgba( 70, 55, 130, 1.00f);
  c[ImGuiCol_ButtonHovered]     = rgba( 95, 75, 170, 1.00f);
  c[ImGuiCol_ButtonActive]     = rgba(120, 100, 210, 1.00f);

  c[ImGuiCol_Header]        = rgba( 55, 45, 100, 1.00f);
  c[ImGuiCol_HeaderHovered]     = rgba( 80, 65, 145, 1.00f);
  c[ImGuiCol_HeaderActive]     = rgba(105, 85, 185, 1.00f);

  c[ImGuiCol_Separator]       = rgba( 60, 55, 95, 1.00f);
  c[ImGuiCol_SeparatorHovered]   = rgba( 95, 80, 150, 1.00f);
  c[ImGuiCol_SeparatorActive]    = rgba(120, 100, 195, 1.00f);

  c[ImGuiCol_ResizeGrip]      = rgba( 70, 60, 120, 0.50f);
  c[ImGuiCol_ResizeGripHovered]   = rgba( 95, 80, 150, 0.80f);
  c[ImGuiCol_ResizeGripActive]   = rgba(120, 100, 200, 1.00f);

  c[ImGuiCol_Tab]          = rgba( 30, 28, 48, 1.00f);
  c[ImGuiCol_TabHovered]      = rgba( 95, 80, 160, 1.00f);
  c[ImGuiCol_TabActive]       = rgba( 70, 55, 130, 1.00f);
  c[ImGuiCol_TabUnfocused]     = rgba( 25, 25, 40, 1.00f);
  c[ImGuiCol_TabUnfocusedActive]  = rgba( 50, 45, 90, 1.00f);
}

//
//
//
// La bolita de marras
void GUI::renderFloatingBall() {
  constexpr float kBallRadius = 26.f;
  constexpr float kBallSize  = kBallRadius * 2.f;
  constexpr float kDragSlop  = 6.f; // pixels before tap becomes drag

  ImGuiIO& io = ImGui::GetIO();

  ballPos.x = std::max(0.f, std::min(ballPos.x, io.DisplaySize.x - kBallSize));
  ballPos.y = std::max(0.f, std::min(ballPos.y, io.DisplaySize.y - kBallSize));

  ImGui::SetNextWindowPos(ballPos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(kBallSize, kBallSize), ImGuiCond_Always);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));

  ImGui::Begin("##ZBALL", nullptr,
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImVec2 winPos = ImGui::GetWindowPos();
  ImVec2 center = ImVec2(winPos.x + kBallRadius, winPos.y + kBallRadius);

  ImGui::InvisibleButton("##ballhit", ImVec2(kBallSize, kBallSize));
  bool active = ImGui::IsItemActive();
  bool hovered = ImGui::IsItemHovered();

  if (active) {
    ImVec2 delta = io.MouseDelta;
    if (ballDragging || std::fabs(delta.x) > 0.f || std::fabs(delta.y) > 0.f) {
      ballPos.x += delta.x;
      ballPos.y += delta.y;
    }
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, kDragSlop)) {
      ballDragging = true;
    }
  }

  if (ImGui::IsItemDeactivated()) {
    if (!ballDragging) {
      visible = !visible;
    } else {
      auto m = Mod::get();
      m->setSavedValue<double>("ballPosX", static_cast<double>(ballPos.x));
      m->setSavedValue<double>("ballPosY", static_cast<double>(ballPos.y));
    }
    ballDragging = false;
  }

  ImU32 fill = visible ? IM_COL32(140, 110, 230, 235)
             : IM_COL32(35, 35, 55, 220);
  if (hovered || active) {
    fill = visible ? IM_COL32(170, 140, 255, 245)
            : IM_COL32(70, 60, 110, 240);
  }
  ImU32 ring = IM_COL32(232, 232, 240, 220);

  auto* dl = ImGui::GetWindowDrawList();
  dl->AddCircleFilled(center, kBallRadius, fill, 32);
  dl->AddCircle(center, kBallRadius, ring, 32, 2.f);

  const char* label = "Z";
  ImVec2 ts = ImGui::CalcTextSize(label);
  dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
        IM_COL32(255, 255, 255, 255), label);

  GMacro* mgr = GMacro::get();
  if (mgr->state == RECORD) {
    dl->AddCircleFilled(ImVec2(center.x + kBallRadius * 0.65f,
                  center.y - kBallRadius * 0.65f),
              5.f, IM_COL32(230, 60, 60, 255), 16);
  } else if (mgr->state == PLAYBACK) {
    dl->AddCircleFilled(ImVec2(center.x + kBallRadius * 0.65f,
                  center.y - kBallRadius * 0.65f),
              5.f, IM_COL32(80, 220, 120, 255), 16);
  }
  if (mgr->spamEnabled) {
    dl->AddCircleFilled(ImVec2(center.x - kBallRadius * 0.65f,
                  center.y - kBallRadius * 0.65f),
              4.f, IM_COL32(180, 150, 255, 255), 16);
  }

  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar(2);
}

//
//
// UI helper
void GUI::renderReplayInfo() {
  GMacro* mgr = GMacro::get();
  if (mgr->currentReplay) {
    ImGui::Text("Replay: %s", mgr->currentReplay->name.c_str());
    ImGui::Text("Inputs: %zu TPS: %.0f Duration: %.2fs",
          mgr->currentReplay->inputs.size(),
          mgr->currentReplay->framerate,
          mgr->currentReplay->inputs.empty()
            ? 0.f
            : static_cast<float>(mgr->currentReplay->inputs.back().frame)
              / static_cast<float>(mgr->currentReplay->framerate));
  } else {
    ImGui::TextDisabled("Sin replay cargado");
  }
}

// Status check
void GUI::renderStateSwitcher() {
  GMacro* mgr = GMacro::get();
  int currentState = (int)mgr->state;

  if (ImGui::RadioButton("Idle", &currentState, NONE)) mgr->state = NONE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Record", &currentState, RECORD)) {
    mgr->state = RECORD;
    if (PlayLayer::get()) {
      mgr->createNewReplay(PlayLayer::get()->m_level);
    }
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Playback", &currentState, PLAYBACK)) {
    if (mgr->currentReplay) {
      mgr->state = PLAYBACK;
      mgr->requestPlaybackRestart();
    } else {
      mgr->state = NONE;
    }
  }
}

//
//
// UI helper
// Aqui es donde se ve todo el tinglado
void GUI::renderHomeTab() {
  GMacro* mgr = GMacro::get();

  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Status");
  ImGui::Separator();
  renderReplayInfo();
  ImGui::Spacing();
  renderStateSwitcher();

  ImGui::Spacing();
  ImGui::TextDisabled(
    mgr->state == RECORD  ? "Grabando... tu run esta siendo capturado." :
    mgr->state == PLAYBACK ? "Playback listo. Reinicia el nivel para reproducir." :
                 "Elige Record para grabar o Playback para reproducir.");

  if (mgr->state == RECORD && mgr->currentReplay) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.f, 0.55f, 0.55f, 1.f),
      mgr->levelCompleted
        ? "Nivel completado - el macro es un run perfecto."
        : "Termina el nivel para guardar como run perfecto.");
  }

  if (mgr->frameAdvance) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.55f, 1.f),
              "Frame Advance: ON (nivel pausado)");
    auto* pl = PlayLayer::get();
    if (pl) {
      int curFrame = static_cast<int>(pl->m_gameState.m_currentProgress) / 2;
      ImGui::Text("Frame actual: %d", curFrame);
    } else {
      ImGui::TextDisabled("Entra a un nivel para empezar.");
    }
    if (ImGui::Button("Avanzar 1 frame", ImVec2(-1.f, 44.f))) {
      mgr->doAdvance = true;
    }
    ImGui::TextDisabled("manten pulsado para ir rapido.");
  }
}

//
//
void GUI::refreshMacros() {
  macros = zReplay::listSavedDetailed();
  if (selectedMacro >= static_cast<int>(macros.size())) {
    selectedMacro = -1;
  }
  macrosDirty = false;
}

static std::string fmtSize(std::uintmax_t bytes) {
  char buf[32];
  if (bytes < 1024) {
    std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
  } else if (bytes < 1024ull * 1024ull) {
    std::snprintf(buf, sizeof(buf), "%.1f KB", bytes / 1024.0);
  } else {
    std::snprintf(buf, sizeof(buf), "%.1f MB", bytes / (1024.0 * 1024.0));
  }
  return std::string(buf);
}

static std::string fmtDate(std::time_t t) {
  if (t <= 0) return std::string("-");
  std::tm tmv{};
#if defined(_WIN32)
  localtime_s(&tmv, &t);
#else
  localtime_r(&t, &tmv);
#endif
  char buf[32];
  if (std::strftime(buf, sizeof(buf), "%b %d %H:%M", &tmv) == 0) {
    return std::string("-");
  }
  return std::string(buf);
}

// Status check
void GUI::renderMacroTab() {
  GMacro* mgr = GMacro::get();

  if (macrosDirty) refreshMacros();

  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Archivo de macro");
  ImGui::Separator();
  ImGui::TextDisabled("Sin puntos; la extension .gdr se anade automaticamente.");
  ImGui::InputText("Name", mgr->loadName, IM_ARRAYSIZE(mgr->loadName),
    ImGuiInputTextFlags_CallbackCharFilter, blockBadNameChars);

  if (ImGui::Button("Save")) {
    if (mgr->currentReplay) {
      std::string clean = sanitizeName(mgr->loadName);
      if (!clean.empty()) mgr->currentReplay->name = clean;
      bool ok = mgr->currentReplay->save(
        mgr->minHoldFrames, mgr->minGapFrames);

      if (ok) {
        std::string savedName = mgr->currentReplay->name;
        refreshMacros();
        selectedMacro = -1;
        for (int i = 0; i < (int)macros.size(); ++i) {
          if (macros[i].name == savedName) { selectedMacro = i; break; }
        }
        macroFilter[0] = '\0';

        Notification::create("Macro guardado",
          NotificationIcon::Success, 1.0f)->show();
      } else {
        Notification::create("Error al guardar - revisa permisos de almacenamiento",
          NotificationIcon::Error, 2.0f)->show();
      }
    } else {
      Notification::create("Nada que guardar", NotificationIcon::Warning, 1.5f)->show();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    if (mgr->currentReplay) {
      delete mgr->currentReplay;
      mgr->currentReplay = nullptr;
      mgr->state = NONE;
    }
  }

  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Calidad de grabacion");
  ImGui::Separator();
  bool qDirty = false;
  qDirty |= ImGui::Checkbox("Solo run perfecto", &mgr->perfectRunOnly);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Solo auto-guarda cuando completas el nivel.\n"
             "Salir a mitad no sobreescribe tu macro guardado.");
  }
  ImGui::SameLine();
  qDirty |= ImGui::Checkbox("Auto guardar", &mgr->autoSave);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Guarda el macro automaticamente al completar\n"
             "el nivel (y al salir si 'Solo run perfecto' esta off).");
  }
  qDirty |= ImGui::Checkbox("Deduplicar inputs", &mgr->dedupeInputs);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Elimina eventos de input que no cambian el estado\n"
             "del boton. Evita doble-taps y rebotes raros.");
  }

  ImGui::TextDisabled("Espaciado anti-spam (frames):");
  ImGui::SetNextItemWidth(120.f);
  if (ImGui::InputInt("Min hold##sp", &mgr->minHoldFrames, 1, 4)) {
    if (mgr->minHoldFrames < 0) mgr->minHoldFrames = 0;
    if (mgr->minHoldFrames > 60) mgr->minHoldFrames = 60;
    qDirty = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Fuerza que cada pulsacion se mantenga al menos\n"
             "este numero de frames antes de soltarse.\n"
             "Evita que GD ignore pulsaciones muy rapidas.\n"
             "Por defecto: 1");
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(120.f);
  if (ImGui::InputInt("Min gap##sp", &mgr->minGapFrames, 1, 4)) {
    if (mgr->minGapFrames < 0) mgr->minGapFrames = 0;
    if (mgr->minGapFrames > 60) mgr->minGapFrames = 60;
    qDirty = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Fuerza al menos este numero de frames entre\n"
             "pulsaciones consecutivas del mismo boton.\n"
             "Por defecto: 1");
  }

  if (qDirty) mgr->saveSettings();

  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Macros guardados");
  ImGui::Separator();
  ImGui::Text("%zu macro%s guardado%s",
        macros.size(), macros.size() == 1 ? "" : "s",
        macros.size() == 1 ? "" : "s");
  ImGui::SameLine();
  if (ImGui::SmallButton("Refresh")) {
    macrosDirty = true;
  }

  if (macros.empty()) {
    ImGui::TextDisabled("Graba algo para verlo aqui.");
    return;
  }

  ImGui::SetNextItemWidth(-1);
  ImGui::InputTextWithHint("##macrofilter", "Filtrar por nombre...",
               macroFilter, IM_ARRAYSIZE(macroFilter));
  auto matchesFilter = [this](const std::string& n) {
    if (macroFilter[0] == '\0') return true;
    std::string needle = macroFilter;
    std::string hay  = n;
    std::transform(needle.begin(), needle.end(), needle.begin(),
      [](unsigned char c){ return std::tolower(c); });
    std::transform(hay.begin(), hay.end(), hay.begin(),
      [](unsigned char c){ return std::tolower(c); });
    return hay.find(needle) != std::string::npos;
  };

  ImGui::PushItemWidth(-1);
  if (ImGui::BeginListBox("##macrolist",
      ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 7.f))) {
    int shown = 0;
    for (int i = 0; i < static_cast<int>(macros.size()); ++i) {
      const auto& info = macros[i];
      const std::string& n = info.name;
      if (!matchesFilter(n)) continue;
      ++shown;
      bool selected = (i == selectedMacro);

      char rowLabel[256];
      std::snprintf(rowLabel, sizeof(rowLabel),
        "%-24s  %8s  %s##row%d",
        n.c_str(),
        fmtSize(info.size).c_str(),
        fmtDate(info.mtime).c_str(),
        i);

      if (ImGui::Selectable(rowLabel, selected)) {
        selectedMacro = i;
        std::strncpy(mgr->loadName, n.c_str(),
               IM_ARRAYSIZE(mgr->loadName) - 1);
        mgr->loadName[IM_ARRAYSIZE(mgr->loadName) - 1] = '\0';
      }
    }
    if (shown == 0) {
      ImGui::TextDisabled(" No macros match \"%s\".", macroFilter);
    }
    ImGui::EndListBox();
  }
  ImGui::PopItemWidth();

  bool hasSel = (selectedMacro >= 0 &&
          selectedMacro < static_cast<int>(macros.size()));

  if (hasSel) {
    ImGui::TextColored(ImVec4(0.85f, 0.78f, 1.0f, 1.f),
              "Selected: %s", macros[selectedMacro].name.c_str());
  } else {
    ImGui::TextDisabled("Seleccionado: (toca un macro arriba)");
  }

  if (!hasSel) ImGui::BeginDisabled();

  constexpr float kActionHeight = 38.f;
  constexpr float kDeleteWidth  = 110.f;
  constexpr float kInnerSpacing = 8.f;

  if (ImGui::Button("Delete", ImVec2(kDeleteWidth, kActionHeight))) {
    ImGui::OpenPopup("Delete macro?");
  }
  ImGui::SameLine(0.f, kInnerSpacing);

  if (ImGui::Button("Replay", ImVec2(-1.f, kActionHeight))) {
    const std::string& n = macros[selectedMacro].name;
    zReplay* r = zReplay::fromFile(n);
    if (r) {
      if (mgr->currentReplay) delete mgr->currentReplay;
      mgr->currentReplay = r;
      mgr->state = PLAYBACK;
      mgr->requestPlaybackRestart();
      mgr->ignoreInput = false;
      mgr->justLoaded = true;
      std::strncpy(mgr->loadName, n.c_str(),
             IM_ARRAYSIZE(mgr->loadName) - 1);
      mgr->loadName[IM_ARRAYSIZE(mgr->loadName) - 1] = '\0';
      Notification::create("Replay listo - reinicia el nivel",
        NotificationIcon::Success, 1.5f)->show();
    } else {
      Notification::create("Error al cargar el macro",
        NotificationIcon::Error, 1.5f)->show();
    }
  }

  if (!hasSel) ImGui::EndDisabled();

  if (ImGui::BeginPopupModal("Delete macro?", nullptr,
      ImGuiWindowFlags_AlwaysAutoResize)) {
    if (hasSel) {
      ImGui::Text("Borrar permanentemente \"%s\"?",
            macros[selectedMacro].name.c_str());
    } else {
      ImGui::TextDisabled("Ningun macro seleccionado.");
    }
    ImGui::Separator();
    if (ImGui::Button("Delete", ImVec2(120, 0)) && hasSel) {
      const std::string n = macros[selectedMacro].name;
      if (zReplay::deleteByName(n)) {
        if (mgr->currentReplay && mgr->currentReplay->name == n) {
          delete mgr->currentReplay;
          mgr->currentReplay = nullptr;
          mgr->state = NONE;
        }
        macrosDirty = true;
        selectedMacro = -1;
        Notification::create("Macro borrado",
          NotificationIcon::Success, 1.0f)->show();
      } else {
        Notification::create("Error al borrar",
          NotificationIcon::Error, 1.5f)->show();
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

//
//
void GUI::renderSpeedTab() {
  GMacro* mgr = GMacro::get();

  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Speedhack (clock)");
  ImGui::Separator();
  ImGui::TextDisabled("Escala el delta time del scheduler de cocos.\n"
            "Mismo metodo que el clock speedhack de xdBot.");

  bool spDirty = false;
  spDirty |= ImGui::Checkbox("Enabled##sh", &mgr->speedHackEnabled);
  ImGui::SameLine();
  spDirty |= ImGui::Checkbox("Pitch de audio", &mgr->speedHackAudio);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Ajusta el pitch de la musica a la velocidad del juego.\n"
             "Recomendado; si no, la cancion se desincroniza.");
  }

  float speed = static_cast<float>(mgr->speed);
  if (ImGui::InputFloat("Speed (x)", &speed, 0.05f, 0.25f, "%.3f")) {
    if (speed < 0.f) speed = 0.f;
    mgr->speed = static_cast<double>(speed);
    spDirty = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("0  = apagado\n"
             "0.1 = casi frame a frame\n"
             "0.25 = practica lenta\n"
             "1.0 = normal\n"
             ">1  = mas rapido (sin limite)");
  }

  static const struct {
    const char* label;
    double value;
  } kPresets[] = {
    { "0",  0.0 }, // off
    { "0.1", 0.1 }, // ~frame by frame
    { "0.25", 0.25 },
    { "0.5", 0.5 },
    { "1x",  1.0 },
    { "2x",  2.0 },
    { "3x",  3.0 },
    { "4x",  4.0 },
  };

  for (size_t i = 0; i < sizeof(kPresets) / sizeof(kPresets[0]); ++i) {
    if (i > 0) ImGui::SameLine();
    bool matches = std::fabs(mgr->speed - kPresets[i].value) < 1e-4;
    if (matches) {
      ImGui::PushStyleColor(ImGuiCol_Button,    IM_COL32(170, 140, 255, 230));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 160, 255, 245));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 180, 255, 255));
      ImGui::PushStyleColor(ImGuiCol_Text,     IM_COL32(20, 20, 30, 255));
    }
    if (ImGui::SmallButton(kPresets[i].label)) {
      mgr->speed = kPresets[i].value;
      mgr->speedHackEnabled = (kPresets[i].value > 0.0);
      spDirty = true;
    }
    if (matches) ImGui::PopStyleColor(4);
  }

  if (mgr->speed <= 0.0) {
    ImGui::TextDisabled("(speed = 0 -> speedhack inactivo)");
  }

  if (spDirty) mgr->saveSettings();
}

//
//
// Internal logic
void GUI::renderSpamTab() {
  GMacro* mgr = GMacro::get();

  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Spam / autoclicker");
  ImGui::Separator();
  ImGui::TextDisabled("Drives the chosen button on a configurable\n"
            "press/release cycle while you're in a level.");

  bool dirty = false;

  dirty |= ImGui::Checkbox("Enabled##spam", &mgr->spamEnabled);
  ImGui::SameLine();
  dirty |= ImGui::Checkbox("Only while playing", &mgr->spamOnlyDuringPlay);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Pausa el spam mientras el juego esta pausado\n"
             "o en transicion. Recomendado.");
  }

  static const char* kButtonLabels[] = { "Saltar", "Izquierda", "Derecha" };
  int btnIdx = std::clamp(mgr->spamButton - 1, 0, 2);
  ImGui::SetNextItemWidth(160.f);
  if (ImGui::Combo("Button", &btnIdx, kButtonLabels, IM_ARRAYSIZE(kButtonLabels))) {
    mgr->spamButton = btnIdx + 1;
    dirty = true;
  }

  // Player picker
  static const char* kPlayerLabels[] = { "Jugador 1", "Jugador 2", "Ambos" };
  int plIdx = std::clamp(mgr->spamPlayer, 0, 2);
  ImGui::SetNextItemWidth(160.f);
  if (ImGui::Combo("Player", &plIdx, kPlayerLabels, IM_ARRAYSIZE(kPlayerLabels))) {
    mgr->spamPlayer = plIdx;
    dirty = true;
  }

  float cps = static_cast<float>(mgr->spamCPS);
  if (ImGui::SliderFloat("CPS", &cps, 0.5f, 60.f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
    if (cps < 0.1f) cps = 0.1f;
    mgr->spamCPS = static_cast<double>(cps);
    dirty = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Clicks por segundo. Se convierte internamente a\n"
             "frames usando el framerate del macro (240 TPS).");
  }

  float ratio = static_cast<float>(mgr->spamHoldRatio);
  if (ImGui::SliderFloat("Hold ratio", &ratio, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
    mgr->spamHoldRatio = static_cast<double>(ratio);
    dirty = true;
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Fraccion del ciclo que el boton esta pulsado.\n"
             "0.5 = pulsacion/soltada iguales\n"
             "0.1 = tap rapido\n"
             "0.9 = pulsacion larga con soltada breve");
  }

  static const struct { const char* label; double cps; } kCpsPresets[] = {
    { "1",  1.0 },
    { "5",  5.0 },
    { "10", 10.0 },
    { "15", 15.0 },
    { "20", 20.0 },
    { "30", 30.0 },
  };
  ImGui::TextDisabled("Quick CPS:");
  for (size_t i = 0; i < sizeof(kCpsPresets) / sizeof(kCpsPresets[0]); ++i) {
    if (i > 0) ImGui::SameLine();
    bool matches = std::fabs(mgr->spamCPS - kCpsPresets[i].cps) < 1e-3;
    if (matches) {
      ImGui::PushStyleColor(ImGuiCol_Button,    IM_COL32(170, 140, 255, 230));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 160, 255, 245));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 180, 255, 255));
      ImGui::PushStyleColor(ImGuiCol_Text,     IM_COL32(20, 20, 30, 255));
    }
    if (ImGui::SmallButton(kCpsPresets[i].label)) {
      mgr->spamCPS = kCpsPresets[i].cps;
      dirty = true;
    }
    if (matches) ImGui::PopStyleColor(4);
  }

  ImGui::Spacing();
  dirty |= ImGui::Checkbox("Grabar spam en macro", &mgr->spamRecordToMacro);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Por defecto, los eventos de spam NO se escriben\n"
             "en el macro para que los archivos queden limpios.\n"
             "Activalo si quieres que el macro incluya el patron\n"
             "de spam en si mismo.");
  }

  ImGui::Spacing();
  ImGui::TextColored(
    mgr->spamEnabled ? ImVec4(0.6f, 1.0f, 0.6f, 1.0f) : ImVec4(0.6f, 0.6f, 0.7f, 1.f),
    "Status: %s", mgr->spamEnabled ? "ACTIVO" : "apagado");

  if (dirty) mgr->saveSettings();
}

//
//
// Stability fix
void GUI::renderSettingsTab() {
  GMacro* mgr = GMacro::get();

  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Safety");
  ImGui::Separator();
// Status check
  bool stDirty = false;
  stDirty |= ImGui::Checkbox("Safe mode (anti-ban)", &mgr->autoSafeMode);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Activa el modo practica al entrar al nivel para que\n"
             "los checkpoints no cuenten como run real.\n"
             "Tambien bloquea muertes mientras graba o reproduce.\n"
             "El nivel visualmente no cambia.");
  }

  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Playback");
  ImGui::Separator();
  stDirty |= ImGui::Checkbox("Clickbot SFX", &mgr->clickbotEnabled);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Reproduce un sonido de click por cada input durante el playback.");
  }

  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "About");
  ImGui::Separator();
  ImGui::TextWrapped(
    "G-Macro Android %s\n"
    "Grabación, playback, speedhack y spam para Android.\n"
    "Desarrollado por pepitogumball, Alberto Cruz y María López.\n"
    "Utiliza libGDR para compatibilidad de macros.",
    GMACRO_VERSION);
  ImGui::TextDisabled("Tip: arrastra la bolita Z para moverla; toca para abrir/cerrar.");

  if (stDirty) mgr->saveSettings();
}


//
//
void GUI::renderConsoleTab() {
  //
  ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Console");
  ImGui::SameLine();

  const char* levels[] = { "ALL", "INFO+", "WARN+", "ERROR" };
  ImGui::SetNextItemWidth(90.f);
  ImGui::Combo("##lvl", &consoleMinLevel, levels, IM_ARRAYSIZE(levels));
  ImGui::SameLine();

  // Text filter
  ImGui::SetNextItemWidth(120.f);
  ImGui::InputText("##filter", consoleFilter, sizeof(consoleFilter));
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Filter by tag or text");
  ImGui::SameLine();

  if (ImGui::Button(consolePaused ? "Resume" : "Pause", ImVec2(60.f, 0.f)))
    consolePaused = !consolePaused;
  ImGui::SameLine();

  // Clear
  if (ImGui::Button("Clear", ImVec2(50.f, 0.f)))
    ZBotLogger::get().clear();
  ImGui::SameLine();

  // Auto-scroll checkbox
  ImGui::Checkbox("Auto", &consoleAutoScroll);
  ImGui::Separator();

  //
  auto entries = ZBotLogger::get().snapshot();
  std::string filterStr(consoleFilter);

  ImVec4 colDebug = ImVec4(0.55f, 0.55f, 0.65f, 1.f); // grey
  ImVec4 colInfo = ImVec4(0.85f, 0.85f, 0.90f, 1.f); // white
  ImVec4 colWarn = ImVec4(1.00f, 0.85f, 0.30f, 1.f); // yellow
  ImVec4 colError = ImVec4(1.00f, 0.35f, 0.35f, 1.f); // red
  // Tag-specific overrides
  auto tagColor = [&](const ZLogEntry& e) -> ImVec4 {
    if (e.tag == "RECORD")  return ImVec4(0.40f, 1.00f, 0.55f, 1.f); // green
    if (e.tag == "PLAYBACK") return ImVec4(0.35f, 0.90f, 1.00f, 1.f); // cyan
    if (e.tag == "EPOCH")  return ImVec4(0.85f, 0.60f, 1.00f, 1.f); // violet
    if (e.level == ZLogLevel::Warn) return colWarn;
    if (e.level == ZLogLevel::Error) return colError;
    if (e.level == ZLogLevel::Debug) return colDebug;
    return colInfo;
  };

  float footerH = 44.f;
  ImGui::BeginChild("##consolescroll",
           ImVec2(0.f, -footerH),
           false,
           ImGuiWindowFlags_HorizontalScrollbar);

  for (auto& e : entries) {
    // Level filter
    if ((int)e.level < consoleMinLevel) continue;
    if (!filterStr.empty()) {
      std::string hay = e.full;
      std::string needle = filterStr;
      // tolower both
      for (auto& c : hay)  c = (char)tolower((unsigned char)c);
      for (auto& c : needle) c = (char)tolower((unsigned char)c);
      if (hay.find(needle) == std::string::npos) continue;
    }
    ImGui::TextColored(tagColor(e), "%s", e.full.c_str());
  }

  if (consoleAutoScroll && !consolePaused)
    ImGui::SetScrollHereY(1.0f);

  ImGui::EndChild();

  //
  ImGui::Separator();

  if (consoleCopied) {
    consoleCopiedTimer -= ImGui::GetIO().DeltaTime;
    if (consoleCopiedTimer <= 0.f) consoleCopied = false;
  }

  ImVec2 btnSize = ImVec2(ImGui::GetContentRegionAvail().x, 36.f);
  if (consoleCopied) {
    ImGui::PushStyleColor(ImGuiCol_Button,
               ImVec4(0.2f, 0.75f, 0.35f, 1.f));
    if (ImGui::Button("Copied! Paste to Zapia :)", btnSize)) {}
    ImGui::PopStyleColor();
  } else {
    if (ImGui::Button("Copy all logs to clipboard", btnSize)) {
      ImGui::SetClipboardText(ZBotLogger::get().allText().c_str());
      consoleCopied   = true;
      consoleCopiedTimer = 1.5f;
    }
  }
}

//
// Main panel
//
void GUI::renderMainPanel() {
  ImGui::SetNextWindowSize(ImVec2(460, 600), ImGuiCond_Once);
  ImGui::SetNextWindowSizeConstraints(ImVec2(360, 400), ImVec2(900, 1400));

  if (!ImGui::Begin("G-Macro Android", &visible)) {
    ImGui::End();
    return;
  }

  ImGui::TextColored(ImVec4(0.85f, 0.78f, 1.0f, 1.f),
            "G-Macro Android %s", GMACRO_VERSION);
  ImGui::SameLine(ImGui::GetWindowWidth() - 56.f);
  if (ImGui::Button("X", ImVec2(36.f, 26.f))) {
    visible = false;
  }
  ImGui::Separator();

  if (ImGui::BeginTabBar("##zbtabs", ImGuiTabBarFlags_None)) {
    if (ImGui::BeginTabItem("Home")) {
      renderHomeTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Macro")) {
      renderMacroTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Speed")) {
      renderSpeedTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Spam")) {
      renderSpamTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Settings")) {
      renderSettingsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Console")) {
      renderConsoleTab();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::Separator();
  GMacro* mgr = GMacro::get();
  const char* stateLabel =
    mgr->state == RECORD  ? "RECORDING" :
    mgr->state == PLAYBACK ? "PLAYBACK" :
                 "IDLE";
  ImVec4 stateColor =
    mgr->state == RECORD  ? ImVec4(1.f, 0.4f, 0.4f, 1.f) :
    mgr->state == PLAYBACK ? ImVec4(0.4f, 1.f, 0.5f, 1.f) :
                 ImVec4(0.6f, 0.6f, 0.7f, 1.f);
  ImGui::TextColored(stateColor, "%s", stateLabel);
  ImGui::SameLine();
  ImGui::TextDisabled(" | ");
  ImGui::SameLine();
  if (mgr->currentReplay) {
    ImGui::Text("%zu inputs", mgr->currentReplay->inputs.size());
  } else {
    ImGui::TextDisabled("no replay");
  }
  ImGui::SameLine();
  ImGui::TextDisabled(" | ");
  ImGui::SameLine();
  ImGui::Text("%.2fx", mgr->speed);
  if (mgr->spamEnabled) {
    ImGui::SameLine();
    ImGui::TextDisabled(" | ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.75f, 0.6f, 1.f, 1.f),
              "spam %.1f cps", mgr->spamCPS);
  }

  ImGui::End();
}

//
// Top-level renderer
//
//
// Stability fix
void GUI::renderer() {
  renderFloatingBall();
  if (visible) renderMainPanel();
}

void GUI::setup() {
  applyTheme();

  GMacro::get()->loadSettings();

  auto m = Mod::get();
  ballPos.x = static_cast<float>(m->getSavedValue<double>("ballPosX", ballPos.x));
  ballPos.y = static_cast<float>(m->getSavedValue<double>("ballPosY", ballPos.y));
}

// UI Implementation by Alberto Cruz
class $modify(zLoadingLayer, LoadingLayer) {
  bool init(bool fromReload) {
    if (!LoadingLayer::init(fromReload)) return false;

    ImGuiCocos::get().setup([] {
      GUI::get()->setup();
    }).draw([] {
      GUI::get()->renderer();
    });

    return true;
  }
};
