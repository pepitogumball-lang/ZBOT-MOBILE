#ifndef _replay_hpp
#define _replay_hpp

#include <Geode/Geode.hpp>
#include <gdr.hpp>
#include <fstream>
#include <filesystem>

using namespace geode::prelude;

struct zInput : gdr::Input<> {
    zInput() = default;
    zInput(uint64_t frame, uint8_t button, bool player2, bool down)
        : Input(frame, button, player2, down) {}
};

struct zReplay : gdr::Replay<zReplay, zInput> {
    std::string name;

    zReplay() : Replay("zBot", 1) {}

    void save() {
        author = GJAccountManager::get()->m_username;
        duration = inputs.size() > 0 ? (float)inputs.back().frame / (float)framerate : 0;

        auto dir = geode::prelude::Mod::get()->getSaveDir() / "replays";
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        auto res = exportData();
        if (res.isOk()) {
            auto data = res.unwrap();
            std::ofstream f(dir / (name + ".gdr"), std::ios::binary);
            f.write(reinterpret_cast<const char*>(data.data()), data.size());
            f.close();
        }
    }

    static zReplay* fromFile(const std::string& fileName) {
        auto dir = geode::prelude::Mod::get()->getSaveDir() / "replays";
        auto filePath = dir / (fileName.ends_with(".gdr") ? fileName : fileName + ".gdr");
        
        if (std::filesystem::exists(filePath)) {
            auto res = zReplay::importData(filePath);
            if (res.isOk()) {
                zReplay* ret = new zReplay();
                *static_cast<gdr::Replay<zReplay, zInput>*>(ret) = res.unwrap();
                ret->name = fileName;
                return ret;
            }
        }
        return nullptr;
    }

    void purgeAfter(int frame) {
        inputs.erase(std::remove_if(inputs.begin(), inputs.end(), [frame](const zInput& input) {
            return input.frame >= (uint64_t)frame;
        }), inputs.end());
    }

    void addInput(uint64_t frame, uint8_t button, bool player2, bool down) {
        inputs.emplace_back(frame, button, player2, down);
    }
};

#endif
