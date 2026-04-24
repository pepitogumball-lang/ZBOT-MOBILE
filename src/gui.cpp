#include "gui.hpp"
#include "zBot.hpp"
#include "replay.hpp"
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/ui/Notification.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

using namespace geode::prelude;

// Block '.' from macro names. The .gdr extension is appended automatically
// at save time, so a name with a dot in it would create files like
// "my.macro.gdr" which break the load lookup.
static int blockDots(ImGuiInputTextCallbackData* data) {
    if (data->EventChar == '.') return 1; // reject
    return 0;
}

// Strip any stray '.' that may have made it into the buffer (e.g. via
// paste, since the input filter only catches typed chars).
static std::string sanitizeName(const char* raw) {
    std::string out(raw);
    out.erase(std::remove(out.begin(), out.end(), '.'), out.end());
    return out;
}

// ---------------------------------------------------------------------------
// EclipseMenu-inspired theme
// ---------------------------------------------------------------------------
//
// Dark base with a violet accent. Buttons and tabs share the same accent
// family so the whole panel reads as one design system. Touch-friendly
// padding/rounding everywhere so the menu is comfortable on a phone.
void GUI::applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding   = 8.0f;
    s.ChildRounding    = 6.0f;
    s.FrameRounding    = 6.0f;
    s.PopupRounding    = 6.0f;
    s.GrabRounding     = 6.0f;
    s.TabRounding      = 6.0f;
    s.ScrollbarRounding= 8.0f;
    s.ScrollbarSize    = 14.0f;
    s.WindowBorderSize = 0.0f;
    s.FrameBorderSize  = 0.0f;
    s.WindowPadding    = ImVec2(12.f, 12.f);
    s.FramePadding     = ImVec2(10.f, 7.f);
    s.ItemSpacing      = ImVec2(8.f, 7.f);
    s.ItemInnerSpacing = ImVec2(6.f, 6.f);
    s.IndentSpacing    = 18.f;

    ImVec4* c = s.Colors;
    auto rgba = [](int r, int g, int b, float a){
        return ImVec4(r / 255.f, g / 255.f, b / 255.f, a);
    };

    c[ImGuiCol_Text]                  = rgba(232, 232, 240, 1.0f);
    c[ImGuiCol_TextDisabled]          = rgba(140, 140, 156, 1.0f);
    c[ImGuiCol_WindowBg]              = rgba( 18,  18,  26, 0.96f);
    c[ImGuiCol_ChildBg]               = rgba( 24,  24,  34, 1.00f);
    c[ImGuiCol_PopupBg]               = rgba( 22,  22,  32, 0.98f);
    c[ImGuiCol_Border]                = rgba( 60,  60,  90, 0.50f);

    c[ImGuiCol_FrameBg]               = rgba( 30,  30,  44, 1.00f);
    c[ImGuiCol_FrameBgHovered]        = rgba( 50,  45,  85, 1.00f);
    c[ImGuiCol_FrameBgActive]         = rgba( 70,  60, 120, 1.00f);

    c[ImGuiCol_TitleBg]               = rgba( 22,  22,  34, 1.00f);
    c[ImGuiCol_TitleBgActive]         = rgba( 60,  45, 110, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]      = rgba( 18,  18,  28, 1.00f);

    c[ImGuiCol_MenuBarBg]             = rgba( 22,  22,  32, 1.00f);

    c[ImGuiCol_ScrollbarBg]           = rgba( 18,  18,  28, 0.60f);
    c[ImGuiCol_ScrollbarGrab]         = rgba( 70,  60, 110, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered]  = rgba( 95,  80, 150, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]   = rgba(115, 100, 180, 1.00f);

    c[ImGuiCol_CheckMark]             = rgba(180, 150, 255, 1.00f);
    c[ImGuiCol_SliderGrab]            = rgba(140, 110, 230, 1.00f);
    c[ImGuiCol_SliderGrabActive]      = rgba(170, 140, 255, 1.00f);

    c[ImGuiCol_Button]                = rgba( 70,  55, 130, 1.00f);
    c[ImGuiCol_ButtonHovered]         = rgba( 95,  75, 170, 1.00f);
    c[ImGuiCol_ButtonActive]          = rgba(120, 100, 210, 1.00f);

    c[ImGuiCol_Header]                = rgba( 55,  45, 100, 1.00f);
    c[ImGuiCol_HeaderHovered]         = rgba( 80,  65, 145, 1.00f);
    c[ImGuiCol_HeaderActive]          = rgba(105,  85, 185, 1.00f);

    c[ImGuiCol_Separator]             = rgba( 60,  55,  95, 1.00f);
    c[ImGuiCol_SeparatorHovered]      = rgba( 95,  80, 150, 1.00f);
    c[ImGuiCol_SeparatorActive]       = rgba(120, 100, 195, 1.00f);

    c[ImGuiCol_ResizeGrip]            = rgba( 70,  60, 120, 0.50f);
    c[ImGuiCol_ResizeGripHovered]     = rgba( 95,  80, 150, 0.80f);
    c[ImGuiCol_ResizeGripActive]      = rgba(120, 100, 200, 1.00f);

    c[ImGuiCol_Tab]                   = rgba( 30,  28,  48, 1.00f);
    c[ImGuiCol_TabHovered]            = rgba( 95,  80, 160, 1.00f);
    c[ImGuiCol_TabActive]             = rgba( 70,  55, 130, 1.00f);
    c[ImGuiCol_TabUnfocused]          = rgba( 25,  25,  40, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]    = rgba( 50,  45,  90, 1.00f);
}

// ---------------------------------------------------------------------------
// Floating ball
// ---------------------------------------------------------------------------
//
// Draws a small draggable circle on the screen. Tapping it toggles the
// main panel; dragging it relocates the ball without toggling. We
// intentionally do NOT use ImGui's built-in window dragging because we
// want the whole circle area to act as the drag handle (no title bar
// to grab on a phone).
void GUI::renderFloatingBall() {
    constexpr float kBallRadius = 26.f;
    constexpr float kBallSize   = kBallRadius * 2.f;
    constexpr float kDragSlop   = 6.f; // pixels before tap becomes drag

    ImGuiIO& io = ImGui::GetIO();

    // Clamp ball into the visible viewport so it can't be lost off-screen
    // after a window resize.
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
    bool active  = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();

    // Manual drag-vs-tap detection: track movement during the press.
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
            // Persist the new ball position so it survives a relaunch.
            // Save only on drag-release so we don't hammer the save
            // file with one write per mouse-move event.
            auto m = Mod::get();
            m->setSavedValue<double>("ballPosX", static_cast<double>(ballPos.x));
            m->setSavedValue<double>("ballPosY", static_cast<double>(ballPos.y));
        }
        ballDragging = false;
    }

    // Visual: violet when the menu is open, dark slate otherwise.
    // EclipseMenu-style accent colour.
    ImU32 fill = visible ? IM_COL32(140, 110, 230, 235)
                         : IM_COL32(35,  35,  55,  220);
    if (hovered || active) {
        fill = visible ? IM_COL32(170, 140, 255, 245)
                       : IM_COL32(70,  60, 110,  240);
    }
    ImU32 ring = IM_COL32(232, 232, 240, 220);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, kBallRadius, fill, 32);
    dl->AddCircle(center, kBallRadius, ring, 32, 2.f);

    // Centered "Z" so the user knows which mod the ball belongs to.
    const char* label = "Z";
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
                IM_COL32(255, 255, 255, 255), label);

    // Tiny coloured dot in the corner reflecting state: red while
    // recording, green during playback, violet while the spammer is
    // active — visible even with the panel closed.
    zBot* mgr = zBot::get();
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

// ---------------------------------------------------------------------------
// Replay info / state switcher (shared widgets)
// ---------------------------------------------------------------------------
void GUI::renderReplayInfo() {
    zBot* mgr = zBot::get();
    if (mgr->currentReplay) {
        ImGui::Text("Replay: %s", mgr->currentReplay->name.c_str());
        ImGui::Text("Inputs: %zu  TPS: %.0f  Duration: %.2fs",
                    mgr->currentReplay->inputs.size(),
                    mgr->currentReplay->framerate,
                    mgr->currentReplay->inputs.empty()
                        ? 0.f
                        : static_cast<float>(mgr->currentReplay->inputs.back().frame)
                            / static_cast<float>(mgr->currentReplay->framerate));
    } else {
        ImGui::TextDisabled("No replay loaded");
    }
}

void GUI::renderStateSwitcher() {
    zBot* mgr = zBot::get();
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
        if (mgr->currentReplay) mgr->state = PLAYBACK;
        else mgr->state = NONE;
    }
}

// ---------------------------------------------------------------------------
// Home tab: state, replay info, big mode buttons
// ---------------------------------------------------------------------------
void GUI::renderHomeTab() {
    zBot* mgr = zBot::get();

    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Status");
    ImGui::Separator();
    renderReplayInfo();
    ImGui::Spacing();
    renderStateSwitcher();

    ImGui::Spacing();
    ImGui::TextDisabled(
        mgr->state == RECORD   ? "Recording... your run is being captured." :
        mgr->state == PLAYBACK ? "Playback armed. Restart the level to play." :
                                 "Pick Record to capture, or Playback to replay.");

    if (mgr->state == RECORD && mgr->currentReplay) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.f, 0.55f, 0.55f, 1.f),
            mgr->levelCompleted
                ? "Level completed - macro is a perfect run."
                : "In progress - finish the level for a perfect-run save.");
    }
}

// ---------------------------------------------------------------------------
// Macro tab: file IO, saved list, recording quality settings
// ---------------------------------------------------------------------------
void GUI::refreshMacros() {
    macros = zReplay::listSavedDetailed();
    if (selectedMacro >= static_cast<int>(macros.size())) {
        selectedMacro = -1;
    }
    macrosDirty = false;
}

// Format a byte count as a short, human-friendly string ("12 B",
// "3.4 KB", "1.2 MB"). Macros are tiny so MB is the practical ceiling.
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

// Format a unix timestamp as a short local-time date, e.g. "Apr 24 16:42".
// Falls back to "—" if the mtime couldn't be read.
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

void GUI::renderMacroTab() {
    zBot* mgr = zBot::get();

    if (macrosDirty) refreshMacros();

    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Macro file");
    ImGui::Separator();
    ImGui::TextDisabled("No dots; the .gdr extension is added automatically.");
    ImGui::InputText("Name", mgr->loadName, IM_ARRAYSIZE(mgr->loadName),
        ImGuiInputTextFlags_CallbackCharFilter, blockDots);

    if (ImGui::Button("Save")) {
        if (mgr->currentReplay) {
            std::string clean = sanitizeName(mgr->loadName);
            if (!clean.empty()) mgr->currentReplay->name = clean;
            mgr->currentReplay->save(mgr->minHoldFrames, mgr->minGapFrames);
            macrosDirty = true;
            Notification::create("Macro saved", NotificationIcon::Success, 1.0f)->show();
        } else {
            Notification::create("Nothing to save", NotificationIcon::Warning, 1.5f)->show();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        std::string clean = sanitizeName(mgr->loadName);
        if (!clean.empty()) {
            zReplay* r = zReplay::fromFile(clean);
            if (r) {
                if (mgr->currentReplay) delete mgr->currentReplay;
                mgr->currentReplay = r;
                Notification::create("Macro loaded", NotificationIcon::Success, 1.0f)->show();
            } else {
                Notification::create("Macro not found", NotificationIcon::Error, 1.5f)->show();
            }
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
    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Recording quality");
    ImGui::Separator();
    bool qDirty = false;
    qDirty |= ImGui::Checkbox("Perfect run only", &mgr->perfectRunOnly);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Only auto-save when the level is completed.\n"
                          "Quitting mid-attempt won't overwrite your\n"
                          "previously saved masterpiece.");
    }
    ImGui::SameLine();
    qDirty |= ImGui::Checkbox("Auto save", &mgr->autoSave);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Save the macro automatically on level\n"
                          "complete (and exit, if 'Perfect run only'\n"
                          "is off).");
    }
    qDirty |= ImGui::Checkbox("Dedupe inputs", &mgr->dedupeInputs);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Drop input events that don't change the\n"
                          "held state of a button. Eliminates glitchy\n"
                          "double-taps and bouncing taps.");
    }

    ImGui::TextDisabled("Spam-safe spacing (frames):");
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::InputInt("Min hold##sp", &mgr->minHoldFrames, 1, 4)) {
        if (mgr->minHoldFrames < 0) mgr->minHoldFrames = 0;
        if (mgr->minHoldFrames > 60) mgr->minHoldFrames = 60;
        qDirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Force every press to be held for at least\n"
                          "this many frames before its release lands.\n"
                          "Stops GD from eating same-frame press+release\n"
                          "tuples produced by extreme spam taps.\n"
                          "Default: 1");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::InputInt("Min gap##sp", &mgr->minGapFrames, 1, 4)) {
        if (mgr->minGapFrames < 0) mgr->minGapFrames = 0;
        if (mgr->minGapFrames > 60) mgr->minGapFrames = 60;
        qDirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Force at least this many frames between\n"
                          "consecutive presses of the same button.\n"
                          "Default: 1");
    }

    if (qDirty) mgr->saveSettings();

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Saved macros");
    ImGui::Separator();
    ImGui::Text("%zu macro%s on disk",
                macros.size(), macros.size() == 1 ? "" : "s");
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh")) {
        macrosDirty = true;
    }

    if (macros.empty()) {
        ImGui::TextDisabled("Save a recording to see it here.");
        return;
    }

    // EclipseMenu-style filter: case-insensitive substring match on
    // the macro display name. Empty filter shows everything.
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##macrofilter", "Filter by name...",
                             macroFilter, IM_ARRAYSIZE(macroFilter));
    auto matchesFilter = [this](const std::string& n) {
        if (macroFilter[0] == '\0') return true;
        std::string needle = macroFilter;
        std::string hay    = n;
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

            // Row label: "name   |   size   |   date". The ##rowN suffix
            // keeps each Selectable's ImGui ID unique even if two macros
            // somehow shared a display name.
            char rowLabel[256];
            std::snprintf(rowLabel, sizeof(rowLabel),
                "%-24s   %8s   %s##row%d",
                n.c_str(),
                fmtSize(info.size).c_str(),
                fmtDate(info.mtime).c_str(),
                i);

            if (ImGui::Selectable(rowLabel, selected,
                    ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedMacro = i;
                std::strncpy(mgr->loadName, n.c_str(),
                             IM_ARRAYSIZE(mgr->loadName) - 1);
                mgr->loadName[IM_ARRAYSIZE(mgr->loadName) - 1] = '\0';

                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    zReplay* r = zReplay::fromFile(n);
                    if (r) {
                        if (mgr->currentReplay) delete mgr->currentReplay;
                        mgr->currentReplay = r;
                        mgr->state = PLAYBACK;
                        Notification::create("Playing macro",
                            NotificationIcon::Success, 1.0f)->show();
                    }
                }
            }
        }
        if (shown == 0) {
            ImGui::TextDisabled("  No macros match \"%s\".", macroFilter);
        }
        ImGui::EndListBox();
    }
    ImGui::PopItemWidth();

    bool hasSel = (selectedMacro >= 0 &&
                   selectedMacro < static_cast<int>(macros.size()));

    if (!hasSel) ImGui::BeginDisabled();

    if (ImGui::Button("Load##sel")) {
        const std::string& n = macros[selectedMacro].name;
        zReplay* r = zReplay::fromFile(n);
        if (r) {
            if (mgr->currentReplay) delete mgr->currentReplay;
            mgr->currentReplay = r;
            Notification::create("Macro loaded",
                NotificationIcon::Success, 1.0f)->show();
        } else {
            Notification::create("Failed to load",
                NotificationIcon::Error, 1.5f)->show();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Play##sel")) {
        const std::string& n = macros[selectedMacro].name;
        zReplay* r = zReplay::fromFile(n);
        if (r) {
            if (mgr->currentReplay) delete mgr->currentReplay;
            mgr->currentReplay = r;
            mgr->state = PLAYBACK;
            Notification::create("Playback armed",
                NotificationIcon::Success, 1.0f)->show();
        } else {
            Notification::create("Failed to load",
                NotificationIcon::Error, 1.5f)->show();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete##sel")) {
        ImGui::OpenPopup("Delete macro?");
    }

    if (!hasSel) ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("Delete macro?", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        if (hasSel) {
            ImGui::Text("Permanently delete \"%s\"?",
                        macros[selectedMacro].name.c_str());
        } else {
            ImGui::TextDisabled("No macro selected.");
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
                Notification::create("Macro deleted",
                    NotificationIcon::Success, 1.0f)->show();
            } else {
                Notification::create("Delete failed",
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

// ---------------------------------------------------------------------------
// Speed tab: clock-style speedhack with presets
// ---------------------------------------------------------------------------
void GUI::renderSpeedTab() {
    zBot* mgr = zBot::get();

    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Speedhack (clock)");
    ImGui::Separator();
    ImGui::TextDisabled("Scales the cocos scheduler delta time.\n"
                        "Same approach as xdBot's clock speedhack.");

    bool spDirty = false;
    spDirty |= ImGui::Checkbox("Enabled##sh", &mgr->speedHackEnabled);
    ImGui::SameLine();
    spDirty |= ImGui::Checkbox("Audio pitch", &mgr->speedHackAudio);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pitch the music to match the game speed.\n"
                          "Recommended; otherwise the song desyncs.");
    }

    float speed = static_cast<float>(mgr->speed);
    if (ImGui::InputFloat("Speed (x)", &speed, 0.05f, 0.25f, "%.3f")) {
        if (speed < 0.f) speed = 0.f;
        mgr->speed = static_cast<double>(speed);
        spDirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("0    = off\n"
                          "0.1  = ~frame by frame\n"
                          "0.25 = slow practice\n"
                          "1.0  = normal\n"
                          ">1   = faster (no limit)");
    }

    static const struct {
        const char* label;
        double value;
    } kPresets[] = {
        { "0",    0.0  }, // off
        { "0.1",  0.1  }, // ~frame by frame
        { "0.25", 0.25 },
        { "0.5",  0.5  },
        { "1x",   1.0  },
        { "2x",   2.0  },
        { "3x",   3.0  },
        { "4x",   4.0  },
    };

    for (size_t i = 0; i < sizeof(kPresets) / sizeof(kPresets[0]); ++i) {
        if (i > 0) ImGui::SameLine();
        bool matches = std::fabs(mgr->speed - kPresets[i].value) < 1e-4;
        if (matches) {
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(170, 140, 255, 230));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 160, 255, 245));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(210, 180, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(20, 20, 30, 255));
        }
        if (ImGui::SmallButton(kPresets[i].label)) {
            mgr->speed = kPresets[i].value;
            mgr->speedHackEnabled = (kPresets[i].value > 0.0);
            spDirty = true;
        }
        if (matches) ImGui::PopStyleColor(4);
    }

    if (mgr->speed <= 0.0) {
        ImGui::TextDisabled("(speed = 0 -> speedhack inactive)");
    }

    if (spDirty) mgr->saveSettings();
}

// ---------------------------------------------------------------------------
// Spam tab: built-in autoclicker / spammer
// ---------------------------------------------------------------------------
void GUI::renderSpamTab() {
    zBot* mgr = zBot::get();

    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Spam / autoclicker");
    ImGui::Separator();
    ImGui::TextDisabled("Drives the chosen button on a configurable\n"
                        "press/release cycle while you're in a level.");

    bool dirty = false;

    dirty |= ImGui::Checkbox("Enabled##spam", &mgr->spamEnabled);
    ImGui::SameLine();
    dirty |= ImGui::Checkbox("Only while playing", &mgr->spamOnlyDuringPlay);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pause the spammer while the game is paused\n"
                          "or in a transition. Recommended.");
    }

    // Button picker (Jump / Left / Right -> PlayerButton 1/2/3).
    static const char* kButtonLabels[] = { "Jump", "Left", "Right" };
    int btnIdx = std::clamp(mgr->spamButton - 1, 0, 2);
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::Combo("Button", &btnIdx, kButtonLabels, IM_ARRAYSIZE(kButtonLabels))) {
        mgr->spamButton = btnIdx + 1;
        dirty = true;
    }

    // Player picker.
    static const char* kPlayerLabels[] = { "Player 1", "Player 2", "Both" };
    int plIdx = std::clamp(mgr->spamPlayer, 0, 2);
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::Combo("Player", &plIdx, kPlayerLabels, IM_ARRAYSIZE(kPlayerLabels))) {
        mgr->spamPlayer = plIdx;
        dirty = true;
    }

    // CPS slider with finger-friendly extents.
    float cps = static_cast<float>(mgr->spamCPS);
    if (ImGui::SliderFloat("CPS", &cps, 0.5f, 60.f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
        if (cps < 0.1f) cps = 0.1f;
        mgr->spamCPS = static_cast<double>(cps);
        dirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clicks per second. Internally converted to\n"
                          "frames using the macro framerate (240 TPS).");
    }

    // Hold ratio slider.
    float ratio = static_cast<float>(mgr->spamHoldRatio);
    if (ImGui::SliderFloat("Hold ratio", &ratio, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
        mgr->spamHoldRatio = static_cast<double>(ratio);
        dirty = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Fraction of the cycle the button is held.\n"
                          "0.5 = even press/release\n"
                          "0.1 = brief tap\n"
                          "0.9 = long hold with brief release");
    }

    // CPS quick-presets row.
    static const struct { const char* label; double cps; } kCpsPresets[] = {
        { "1",   1.0  },
        { "5",   5.0  },
        { "10",  10.0 },
        { "15",  15.0 },
        { "20",  20.0 },
        { "30",  30.0 },
    };
    ImGui::TextDisabled("Quick CPS:");
    for (size_t i = 0; i < sizeof(kCpsPresets) / sizeof(kCpsPresets[0]); ++i) {
        if (i > 0) ImGui::SameLine();
        bool matches = std::fabs(mgr->spamCPS - kCpsPresets[i].cps) < 1e-3;
        if (matches) {
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(170, 140, 255, 230));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(190, 160, 255, 245));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(210, 180, 255, 255));
            ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(20, 20, 30, 255));
        }
        if (ImGui::SmallButton(kCpsPresets[i].label)) {
            mgr->spamCPS = kCpsPresets[i].cps;
            dirty = true;
        }
        if (matches) ImGui::PopStyleColor(4);
    }

    ImGui::Spacing();
    dirty |= ImGui::Checkbox("Record spam into macro", &mgr->spamRecordToMacro);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("By default, spam events are NOT written into\n"
                          "the macro so saved files stay clean. Turn this\n"
                          "on if you want a recorded macro that contains\n"
                          "the spam pattern itself.");
    }

    ImGui::Spacing();
    ImGui::TextColored(
        mgr->spamEnabled ? ImVec4(0.6f, 1.0f, 0.6f, 1.0f) : ImVec4(0.6f, 0.6f, 0.7f, 1.f),
        "Status: %s", mgr->spamEnabled ? "ARMED" : "off");

    if (dirty) mgr->saveSettings();
}

// ---------------------------------------------------------------------------
// Settings tab: safety, audio, visibility, misc
// ---------------------------------------------------------------------------
void GUI::renderSettingsTab() {
    zBot* mgr = zBot::get();

    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Safety");
    ImGui::Separator();
    bool stDirty = false;
    stDirty |= ImGui::Checkbox("Auto-Safe Mode", &mgr->autoSafeMode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Forces practice mode on level start so any new\n"
                          "checkpoint you set doesn't count as a real run\n"
                          "(no submission, no ban risk). Also blocks death\n"
                          "events while a macro is recording or playing.\n"
                          "The level itself stays visually unmodified.");
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Playback");
    ImGui::Separator();
    stDirty |= ImGui::Checkbox("Clickbot SFX", &mgr->clickbotEnabled);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Play a click sound for every input during playback.");
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "Menu visibility");
    ImGui::Separator();
    ImGui::TextDisabled("Hide the floating Z ball + panel based on\n"
                        "where you are in the game.");

    stDirty |= ImGui::Checkbox("Hide while playing", &mgr->hideWhilePlaying);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hide the menu while a level is being actively\n"
                          "played. It reappears as soon as the game is\n"
                          "paused.");
    }
    stDirty |= ImGui::Checkbox("Hide after finishing a level", &mgr->hideAfterFinish);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hide the menu once the level is completed.\n"
                          "It reappears when you return to the main menu.");
    }
    stDirty |= ImGui::Checkbox("Hide while editing a level", &mgr->hideInEditor);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hide the menu while you're inside the level\n"
                          "editor. Doesn't affect editor test play.");
    }
    stDirty |= ImGui::Checkbox("Only show in main menu", &mgr->onlyShowInMenu);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Master switch: keep the menu hidden whenever\n"
                          "you're inside a level (paused or otherwise)\n"
                          "or inside the editor. Only appears on the\n"
                          "main menu / level select.");
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.6f, 1.0f, 1.0f), "About");
    ImGui::Separator();
    ImGui::TextWrapped(
        "ZBOT-MOBILE %s - macro / clock speedhack mod for Geometry Dash.\n"
        "Inspired by FigmentBoy/zBot, Zilko/xdBot, and EclipseMenu.\n"
        "Settings are remembered between sessions.",
        ZBOT_VERSION);
    ImGui::TextDisabled("Tip: drag the floating Z ball to move it; tap to toggle.");

    if (stDirty) mgr->saveSettings();
}

// ---------------------------------------------------------------------------
// Main panel
// ---------------------------------------------------------------------------
void GUI::renderMainPanel() {
    ImGui::SetNextWindowSize(ImVec2(460, 600), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(360, 400), ImVec2(900, 1400));

    // Built-in close (X) is rendered by ImGui when we pass &visible.
    if (!ImGui::Begin("ZBOT-MOBILE", &visible)) {
        ImGui::End();
        return;
    }

    // Header row: title + finger-friendly X close button.
    ImGui::TextColored(ImVec4(0.85f, 0.78f, 1.0f, 1.f),
                       "ZBOT-MOBILE %s", ZBOT_VERSION);
    ImGui::SameLine(ImGui::GetWindowWidth() - 56.f);
    if (ImGui::Button("X", ImVec2(36.f, 26.f))) {
        visible = false;
    }
    ImGui::Separator();

    // Tab bar (EclipseMenu-inspired layout).
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
        ImGui::EndTabBar();
    }

    // Status bar at the bottom: live recording info + global state.
    ImGui::Separator();
    zBot* mgr = zBot::get();
    const char* stateLabel =
        mgr->state == RECORD   ? "RECORDING" :
        mgr->state == PLAYBACK ? "PLAYBACK"  :
                                 "IDLE";
    ImVec4 stateColor =
        mgr->state == RECORD   ? ImVec4(1.f, 0.4f, 0.4f, 1.f) :
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

// ---------------------------------------------------------------------------
// Visibility decision
// ---------------------------------------------------------------------------
//
// Visibility decisions are split into two functions so the floating
// ball can stay visible as a "summon" handle even while the heavy
// main panel is hidden during gameplay. EclipseMenu and xdBot do the
// same — there's always *something* clickable so the user is never
// stranded without a way back to the menu.
//
// The flags compose as follows (most restrictive first):
//   onlyShowInMenu  -> hide everything while in a level / editor
//   hideAfterFinish -> hide the panel once a level is completed
//   hideWhilePlaying-> hide the panel while in a level, not paused
//   hideInEditor    -> hide the panel while inside the level editor
//
// `onlyShowInMenu` is the only toggle that hides the floating ball.
// All other toggles only hide the panel — the ball remains tappable
// so the user can reopen the panel during a pause / on the next menu.
//
// PlayLayer::get() being non-null means we're inside a level (paused
// or otherwise). PlayLayer::m_isPaused is the canonical pause flag;
// CCDirector::isPaused() is unreliable on Android because GD's pause
// menu overlays the scene without always pausing the director.
// LevelEditorLayer::get() being non-null means we're in the editor
// (edit mode; test play exposes a PlayLayer too).
bool GUI::shouldRenderBall() {
    zBot* mgr = zBot::get();
    auto* pl  = PlayLayer::get();
    auto* led = LevelEditorLayer::get();
    bool inLevel  = pl != nullptr;
    bool inEditor = led != nullptr && pl == nullptr;

    if (mgr->onlyShowInMenu && (inLevel || inEditor)) return false;
    return true;
}

bool GUI::shouldRenderPanel() {
    zBot* mgr = zBot::get();
    auto* pl  = PlayLayer::get();
    auto* led = LevelEditorLayer::get();
    bool inLevel  = pl != nullptr;
    bool inEditor = led != nullptr && pl == nullptr;
    bool paused   = inLevel && pl->m_isPaused;

    if (mgr->onlyShowInMenu && (inLevel || inEditor)) return false;
    if (mgr->hideAfterFinish && mgr->hudHiddenAfterFinish) return false;
    if (mgr->hideWhilePlaying && inLevel && !paused) return false;
    if (mgr->hideInEditor && inEditor) return false;

    return true;
}

// ---------------------------------------------------------------------------
// Top-level renderer
// ---------------------------------------------------------------------------
void GUI::renderer() {
    // Ball is always available as a summon handle (subject only to
    // `onlyShowInMenu`). The panel respects every visibility toggle
    // independently, so "hide while playing" only hides the heavy
    // panel while leaving the ball tappable for a quick re-open
    // during a pause or on the next menu.
    if (shouldRenderBall()) renderFloatingBall();
    if (visible && shouldRenderPanel()) renderMainPanel();
}

void GUI::setup() {
    applyTheme();

    // Restore persisted state from previous sessions: zBot owns the
    // toggles / numeric inputs, the GUI owns the floating ball
    // position. Defaults survive untouched if no save key exists yet.
    zBot::get()->loadSettings();

    auto m = Mod::get();
    ballPos.x = static_cast<float>(m->getSavedValue<double>("ballPosX", ballPos.x));
    ballPos.y = static_cast<float>(m->getSavedValue<double>("ballPosY", ballPos.y));
}

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

// Returning to the main menu clears the "level just finished" HUD-hide
// flag so the user can find the menu again after a successful run.
// We also clear it on entering the editor (different scene from the
// finished level) so the user isn't stuck without a menu while editing.
class $modify(zMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        zBot::get()->hudHiddenAfterFinish = false;
        return true;
    }
};

class $modify(zEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool unk) {
        if (!LevelEditorLayer::init(level, unk)) return false;
        zBot::get()->hudHiddenAfterFinish = false;
        return true;
    }
};
