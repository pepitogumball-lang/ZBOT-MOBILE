#include "gui.hpp"
#include "zBot.hpp"
#include "replay.hpp"
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

// Block '.' from macro names. The .gdr extension is appended automatically
// at save time, so a name with a dot in it would create files like
// "my.macro.gdr" which break the load lookup.
static int blockDots(ImGuiInputTextCallbackData* data) {
    if (data->EventChar == '.') return 1; // reject
    return 0;
}

void GUI::renderReplayInfo() {
    zBot* mgr = zBot::get();
    if (mgr->currentReplay) {
        ImGui::Text("Replay: %s", mgr->currentReplay->name.c_str());
        ImGui::Text("Inputs: %zu  TPS: %.0f", mgr->currentReplay->inputs.size(), mgr->currentReplay->framerate);
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

void GUI::renderMainPanel() {
    ImGui::SetNextWindowSize(ImVec2(380, 460), ImGuiCond_Once);
    ImGui::Begin("ZBOT-MOBILE", &visible);

    ImGui::TextColored(ImVec4(1.f, 0.78f, 0.17f, 1.f), "ZBOT-MOBILE v1.0.0");
    ImGui::Separator();

    renderReplayInfo();
    renderStateSwitcher();

    ImGui::Separator();

    zBot* mgr = zBot::get();

    // ---- Speedhack ----
    ImGui::Text("Speedhack");
    ImGui::Checkbox("Enabled##sh", &mgr->speedHackEnabled);
    ImGui::SameLine();
    ImGui::Checkbox("Audio pitch", &mgr->speedHackAudio);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Pitch the music to match the game speed.\n"
                          "Auto-engages when the speedhack is on.");
    }

    float speed = (float)mgr->speed;
    if (ImGui::InputFloat("Speed", &speed, 0.05f, 0.25f, "%.3f")) {
        if (speed > 0.f) mgr->speed = (double)speed;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Try 0.25, 0.5, 2.0, etc.\n"
                          "Macros are always saved at normal 240 TPS,\n"
                          "so the slow-mo clicks come out perfect at full speed.");
    }

    ImGui::Separator();

    // ---- Auto-Safe Mode ----
    ImGui::Checkbox("Auto-Safe Mode", &mgr->autoSafeMode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Forces practice mode on level start so any new\n"
                          "checkpoint you set doesn't count as a real run\n"
                          "(no submission, no ban risk). Also blocks death\n"
                          "events while a macro is recording or playing.\n"
                          "The level itself stays visually unmodified.");
    }

    ImGui::Separator();

    // ---- Macro IO ----
    ImGui::Text("Macro file (no dots, .gdr is added automatically)");
    ImGui::InputText("Name", mgr->loadName, IM_ARRAYSIZE(mgr->loadName),
        ImGuiInputTextFlags_CallbackCharFilter, blockDots);

    if (ImGui::Button("Save")) {
        if (mgr->currentReplay) {
            // Defensive strip in case anything else (paste, etc.) sneaks
            // a dot past the input filter.
            std::string clean(mgr->loadName);
            clean.erase(std::remove(clean.begin(), clean.end(), '.'), clean.end());
            if (!clean.empty()) mgr->currentReplay->name = clean;
            mgr->currentReplay->save();
            Notification::create("Macro saved", NotificationIcon::Success, 1.0f)->show();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        std::string clean(mgr->loadName);
        clean.erase(std::remove(clean.begin(), clean.end(), '.'), clean.end());
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

    ImGui::End();
}

void GUI::renderer() {
    // Always show a small floating toggle button on the screen so the
    // panel can be opened/closed on Android where there's no keyboard.
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    ImGui::Begin("##ZBToggle", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
    if (ImGui::Button(visible ? "Hide ZBOT" : "Show ZBOT")) {
        visible = !visible;
    }
    ImGui::End();

    if (visible) renderMainPanel();
}

void GUI::setup() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
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
