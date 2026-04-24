#ifndef _gui_hpp
#define _gui_hpp

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include "replay.hpp"
#include <vector>
#include <string>

using namespace geode::prelude;

class GUI {
public:
    // Main panel visibility (toggled by tapping the floating ball).
    bool visible = false;

    // Floating ball state. The ball is draggable so the user can move
    // it out of the way of game UI on any screen size. Position is in
    // ImGui screen coordinates and persists for the lifetime of the
    // GUI singleton (i.e. the whole game session).
    ImVec2 ballPos = ImVec2(20.f, 80.f);
    bool ballDragging = false;

    // Cached list of macros found on disk with metadata (size + mtime).
    // Refreshed on demand and whenever the user saves / deletes a macro.
    // Kept on the GUI so we don't hit the filesystem every frame.
    std::vector<zReplay::MacroFileInfo> macros;
    int selectedMacro = -1;
    bool macrosDirty = true;

    // EclipseMenu-style search filter for the macro browser. Empty
    // string means "show everything". Matched case-insensitively
    // against the macro display name.
    char macroFilter[64] = "";

    // Optional fonts (kept for future expansion; currently unused).
    ImFont* s = nullptr;
    ImFont* l = nullptr;

    void setup();
    void renderer();

    // Floating, draggable circular icon that opens / closes the panel.
    void renderFloatingBall();

    // Main panel + tabs (EclipseMenu-inspired layout).
    void renderMainPanel();
    void renderHomeTab();
    void renderMacroTab();
    void renderSpeedTab();
    void renderSpamTab();
    void renderSettingsTab();

    void renderReplayInfo();
    void renderStateSwitcher();

    // Filesystem helpers for the macro list.
    void refreshMacros();

    // EclipseMenu-style theme.
    void applyTheme();

    // Decides whether the floating ball should render this frame.
    // The ball is the "summon" handle so we keep it visible whenever
    // possible — only `onlyShowInMenu` actually hides it.
    bool shouldRenderBall();

    // Decides whether the main panel should render this frame.
    // Honours every visibility toggle (hideWhilePlaying, hideInEditor,
    // hideAfterFinish, onlyShowInMenu) so the heavy panel doesn't
    // occlude gameplay while still letting the user tap the ball to
    // bring it back during a pause.
    bool shouldRenderPanel();

    static GUI* get() {
        static GUI* instance = new GUI();
        return instance;
    }
};

#endif
