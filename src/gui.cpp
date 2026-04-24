#include "gui.hpp"
#include "zBot.hpp"
#include "replay.hpp"
#include <Geode/modify/LoadingLayer.hpp>
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
        if (ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left, kDragSlop)) {
            ballDragging = true;
        }
    }

    if (ImGui::IsItemDeactivated()) {
        if (!ballDragging) {
            visible = !visible;
        }
        ballDragging = false;
    }

    // Visual: gold when the menu is open, dark blue otherwise. Hover
    // brightens the fill so the user gets feedback before they tap.
    ImU32 fill = visible ? IM_COL32(255, 200, 60, 230) : IM_COL32(40, 50, 70, 220);
    if (hovered || active) {
        fill = visible ? IM_COL32(255, 220, 100, 240)
                       : IM_COL32(70, 90, 130, 240);
    }
    ImU32 ring = IM_COL32(255, 255, 255, 220);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, kBallRadius, fill, 32);
    dl->AddCircle(center, kBallRadius, ring, 32, 2.f);

    // Centered "Z" so the user knows which mod the ball belongs to.
    const char* label = "Z";
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
                IM_COL32(255, 255, 255, 255), label);

    // Tiny coloured dot in the corner reflecting state: red while
    // recording, green while playing, hidden when idle. Helps the
    // player tell what mode they're in even with the panel closed.
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

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// ---------------------------------------------------------------------------
// Replay info / state switcher
// ---------------------------------------------------------------------------
void GUI::renderReplayInfo() {
    zBot* mgr = zBot::get();
    if (mgr->currentReplay) {
        ImGui::Text("Replay: %s", mgr->currentReplay->name.c_str());
        ImGui::Text("Inputs: %zu  TPS: %.0f",
                    mgr->currentReplay->inputs.size(),
                    mgr->currentReplay->framerate);
    } else {
        ImGui::TextDisabled("No replay loaded");
    }
}

void GUI::renderStateSwitcher() {
    zBot* mgr = zBot::get();
    int currentState = (int)mgr->state;

    if (ImGui::RadioButton("None", &currentState, NONE)) mgr->state = NONE;
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
// Speedhack section
// ---------------------------------------------------------------------------
//
// xdBot/zBot-style "clock" speedhack: scale the delta time fed into the
// scheduler. Implementation lives in src/speedhack.cpp; this is the UI.
//
// Speed convention:
//   0    -> nulo (passes through normal time, same as disabling)
//   0.1  -> very slow, almost frame-by-frame
//   0.25 -> slow practice
//   1.0  -> normal
//   >1   -> faster, no upper limit
void GUI::renderSpeedhackSection() {
    zBot* mgr = zBot::get();

    ImGui::Text("Speedhack (clock)");

    ImGui::Checkbox("Enabled##sh", &mgr->speedHackEnabled);
    ImGui::SameLine();
    ImGui::Checkbox("Audio pitch", &mgr->speedHackAudio);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pitch the music to match the game speed.\n"
                          "Recommended; otherwise the song desyncs.");
    }

    // Free-form numeric input. Negative values are clamped to 0 because
    // a negative speed would run the scheduler backwards (don't do
    // that). Increment / decrement steps make small tweaks easier on
    // touch.
    float speed = static_cast<float>(mgr->speed);
    if (ImGui::InputFloat("Speed (x)", &speed, 0.05f, 0.25f, "%.3f")) {
        if (speed < 0.f) speed = 0.f;
        mgr->speed = static_cast<double>(speed);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("0    = off\n"
                          "0.1  = ~frame by frame\n"
                          "0.25 = slow practice\n"
                          "1.0  = normal\n"
                          ">1   = faster (no limit)");
    }

    // One-tap presets. Tapping a preset also auto-enables the
    // speedhack so the change is immediately audible / visible.
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
        // Highlight the preset that matches the current speed so the
        // user can see at a glance what's selected.
        bool matches = std::fabs(mgr->speed - kPresets[i].value) < 1e-4;
        if (matches) {
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(255, 200, 60, 220));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 215, 90, 240));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(255, 230, 120, 255));
            ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(20, 20, 20, 255));
        }
        if (ImGui::SmallButton(kPresets[i].label)) {
            mgr->speed = kPresets[i].value;
            mgr->speedHackEnabled = (kPresets[i].value > 0.0);
        }
        if (matches) ImGui::PopStyleColor(4);
    }

    if (mgr->speed <= 0.0) {
        ImGui::TextDisabled("(speed = 0 -> speedhack inactive)");
    }
}

// ---------------------------------------------------------------------------
// Macro section: name field + saved list + IO buttons
// ---------------------------------------------------------------------------
void GUI::refreshMacros() {
    macros = zReplay::listSaved();
    if (selectedMacro >= static_cast<int>(macros.size())) {
        selectedMacro = -1;
    }
    macrosDirty = false;
}

void GUI::renderMacroSection() {
    zBot* mgr = zBot::get();

    if (macrosDirty) refreshMacros();

    ImGui::Text("Macro file (no dots, .gdr is added automatically)");
    ImGui::InputText("Name", mgr->loadName, IM_ARRAYSIZE(mgr->loadName),
        ImGuiInputTextFlags_CallbackCharFilter, blockDots);

    if (ImGui::Button("Save")) {
        if (mgr->currentReplay) {
            std::string clean = sanitizeName(mgr->loadName);
            if (!clean.empty()) mgr->currentReplay->name = clean;
            mgr->currentReplay->save();
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
    ImGui::Separator();

    // Saved macros browser ---------------------------------------------------
    if (ImGui::CollapsingHeader("Saved macros", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("%zu macro%s on disk",
                    macros.size(), macros.size() == 1 ? "" : "s");
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            macrosDirty = true;
        }

        if (macros.empty()) {
            ImGui::TextDisabled("Save a recording to see it here.");
        } else {
            // Scrollable list. Height in items so it scales reasonably
            // on small mobile screens.
            ImGui::PushItemWidth(-1);
            if (ImGui::BeginListBox("##macrolist",
                    ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6.f))) {
                for (int i = 0; i < static_cast<int>(macros.size()); ++i) {
                    const std::string& n = macros[i];
                    bool selected = (i == selectedMacro);
                    if (ImGui::Selectable(n.c_str(), selected,
                            ImGuiSelectableFlags_AllowDoubleClick)) {
                        selectedMacro = i;
                        // Mirror selection into the name buffer so the
                        // existing Save/Load/Clear buttons keep working.
                        std::strncpy(mgr->loadName, n.c_str(),
                                     IM_ARRAYSIZE(mgr->loadName) - 1);
                        mgr->loadName[IM_ARRAYSIZE(mgr->loadName) - 1] = '\0';

                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            // Double-tap = load + jump straight to playback.
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
                ImGui::EndListBox();
            }
            ImGui::PopItemWidth();

            // Action row for the selected macro. All buttons gated on a
            // valid selection so they can't run on garbage indices.
            bool hasSel = (selectedMacro >= 0 &&
                           selectedMacro < static_cast<int>(macros.size()));

            if (!hasSel) ImGui::BeginDisabled();

            if (ImGui::Button("Load##sel")) {
                const std::string& n = macros[selectedMacro];
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
                const std::string& n = macros[selectedMacro];
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

            // Modal so a stray tap on a touchscreen doesn't nuke a macro.
            if (ImGui::BeginPopupModal("Delete macro?", nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize)) {
                if (hasSel) {
                    ImGui::Text("Permanently delete \"%s\"?",
                                macros[selectedMacro].c_str());
                } else {
                    ImGui::TextDisabled("No macro selected.");
                }
                ImGui::Separator();
                if (ImGui::Button("Delete", ImVec2(120, 0)) && hasSel) {
                    const std::string n = macros[selectedMacro];
                    if (zReplay::deleteByName(n)) {
                        // If the deleted macro is the one currently loaded,
                        // drop it from memory too so we don't keep a stale
                        // reference around.
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
    }
}

// ---------------------------------------------------------------------------
// Main panel
// ---------------------------------------------------------------------------
void GUI::renderMainPanel() {
    ImGui::SetNextWindowSize(ImVec2(420, 540), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320, 380), ImVec2(800, 1200));

    // Built-in close (X) is rendered by ImGui when we pass &visible.
    if (!ImGui::Begin("ZBOT-MOBILE", &visible)) {
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(1.f, 0.78f, 0.17f, 1.f), "ZBOT-MOBILE v1.1.0");

    // Big, finger-friendly close button on the same row as the title.
    // Useful on phones where the tiny native X is hard to hit.
    ImGui::SameLine(ImGui::GetWindowWidth() - 50.f);
    if (ImGui::Button("X", ImVec2(36.f, 24.f))) {
        visible = false;
    }

    ImGui::Separator();

    renderReplayInfo();
    renderStateSwitcher();

    ImGui::Separator();
    renderSpeedhackSection();

    ImGui::Separator();

    zBot* mgr = zBot::get();
    ImGui::Checkbox("Auto-Safe Mode", &mgr->autoSafeMode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Forces practice mode on level start so any new\n"
                          "checkpoint you set doesn't count as a real run\n"
                          "(no submission, no ban risk). Also blocks death\n"
                          "events while a macro is recording or playing.\n"
                          "The level itself stays visually unmodified.");
    }
    ImGui::SameLine();
    ImGui::Checkbox("Clickbot SFX", &mgr->clickbotEnabled);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Play a click sound for every input during playback.");
    }

    ImGui::Separator();
    renderMacroSection();

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Top-level renderer
// ---------------------------------------------------------------------------
void GUI::renderer() {
    renderFloatingBall();
    if (visible) renderMainPanel();
}

void GUI::setup() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.GrabRounding   = 4.0f;
    style.ScrollbarSize  = 14.0f; // bigger thumb for touch
    style.ItemSpacing    = ImVec2(8.f, 6.f);
    style.FramePadding   = ImVec2(8.f, 6.f);
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
