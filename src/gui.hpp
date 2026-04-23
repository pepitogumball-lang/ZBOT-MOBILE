#ifndef _gui_hpp
#define _gui_hpp

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>

using namespace geode::prelude;

class GUI {
public:
    bool visible = false;
    ImFont* s = nullptr;
    ImFont* l = nullptr;

    void setup();
    void renderer();
    void renderMainPanel();
    void renderReplayInfo();
    void renderStateSwitcher();

    static GUI* get() {
        static GUI* instance = new GUI();
        return instance;
    }
};

#endif
