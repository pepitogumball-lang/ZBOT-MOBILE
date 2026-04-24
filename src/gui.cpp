#include "gui.hpp"
#include "zBot.hpp"
#include <Geode/modify/LoadingLayer.hpp>

using namespace geode::prelude;

void GUI::renderReplayInfo() {
    zBot* mgr = zBot::get();
    if (mgr->currentReplay) {
        ImGui::Text("Replay: %s", mgr->currentReplay->name.c_str());
        ImGui::Text("TPS: %.0f", mgr->currentReplay->framerate);
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
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_Once);
    ImGui::Begin("ZBOT-MOBILE", &visible);
    
    ImGui::TextColored(ImVec4(1.f, 0.78f, 0.17f, 1.f), "ZBOT-MOBILE v1.0.0");
    ImGui::Separator();

    renderReplayInfo();
    renderStateSwitcher();

    ImGui::Separator();
    
    zBot* mgr = zBot::get();
    float tps = (float)mgr->tps;
    if (ImGui::InputFloat("TPS", &tps)) mgr->tps = (double)tps;

    float speed = (float)mgr->speed;
    if (ImGui::InputFloat("Speed", &speed)) mgr->speed = (double)speed;

    ImGui::Separator();

    // Auto-Safe Mode: keep the level showing normally but skip death events
    // during record/playback. Perfect for clean macros and showcases.
    ImGui::Checkbox("Auto-Safe Mode", &mgr->autoSafeMode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Prevents the player from dying while recording or\n"
                          "playing back a macro. Level stays visually unchanged.");
    }

    if (ImGui::Button("Save Replay")) {
        if (mgr->currentReplay) mgr->currentReplay->save();
    }

    ImGui::End();
}

void GUI::renderer() {
    if (!visible) return;
    renderMainPanel();
}

void GUI::setup() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    // ... custom styling if needed
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

// Toggle UI with a button or shortcut
// For mobile, we might need a dedicated button on screen.
