#ifndef _gui_hpp
#define _gui_hpp

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
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

    // Cached list of macro names found on disk (without the .gdr
    // extension). Refreshed on demand and whenever the user saves /
    // deletes a macro. Kept on the GUI so we don't hit the filesystem
    // every frame.
    std::vector<std::string> macros;
    int selectedMacro = -1;
    bool macrosDirty = true;

    // Optional fonts (kept for future expansion; currently unused).
    ImFont* s = nullptr;
    ImFont* l = nullptr;

    void setup();
    void renderer();

    // Floating, draggable circular icon that opens / closes the panel.
    void renderFloatingBall();

    // Main panel sections.
    void renderMainPanel();
    void renderReplayInfo();
    void renderStateSwitcher();
    void renderSpeedhackSection();
    void renderMacroSection();

    // Filesystem helpers for the macro list.
    void refreshMacros();

    static GUI* get() {
        static GUI* instance = new GUI();
        return instance;
    }
};

#endif
