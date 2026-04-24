#ifndef _replay_hpp
#define _replay_hpp

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>
#include <fstream>
#include <filesystem>

using namespace geode::prelude;

#define ZBF_VERSION 3.0f

struct zInput : gdr::Input {
    zInput() = default;

    zInput(int frame, int button, bool player2, bool down)
        : Input(frame, button, player2, down) {}
};

struct zReplay : gdr::Replay<zReplay, zInput> {
    std::string name;

    zReplay() : Replay("zBot-mobile", "1.0.0") {}

    void save() {
        author = GJAccountManager::get()->m_username;
        duration = inputs.size() > 0
            ? static_cast<float>(inputs.back().frame) / static_cast<float>(framerate)
            : 0.f;

        auto dir = geode::prelude::Mod::get()->getSaveDir() / "replays";
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        std::ofstream f(dir / (name + ".gdr"), std::ios::binary);

        auto data = exportData(false);

        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        f.close();
    }

    static zReplay* fromFile(const std::string& fileName) {
        auto dir = geode::prelude::Mod::get()->getSaveDir() / "replays";
        if (std::filesystem::exists(dir) || std::filesystem::create_directories(dir)) {
            std::ifstream f(dir / (fileName + ".gdr"), std::ios::binary);

            if (!f.is_open()) {
                f = std::ifstream(dir / fileName, std::ios::binary);
                if (!f.is_open()) return nullptr;
            }

            f.seekg(0, std::ios::end);
            auto size = f.tellg();
            f.seekg(0, std::ios::beg);

            std::vector<uint8_t> data(size);
            f.read(reinterpret_cast<char*>(data.data()), size);
            f.close();

            zReplay* ret = new zReplay();
            *ret = importData(data);
            ret->name = fileName;

            return ret;
        }

        return nullptr;
    }

    void purgeAfter(int frame) {
        inputs.erase(std::remove_if(inputs.begin(), inputs.end(), [frame](const zInput& input) {
            return input.frame >= frame;
        }), inputs.end());
    }

    void addInput(int frame, int button, bool player2, bool down) {
        inputs.emplace_back(frame, button, player2, down);
    }
};

#endif
