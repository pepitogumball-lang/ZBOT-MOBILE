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

    static std::filesystem::path macrosDir() {
        // On Android, macros live in the launcher's external media folder
        // so they survive uninstall and the user can browse them with any
        // file manager. On other platforms (only used during local dev,
        // since releases are Android-only) we fall back to the mod save dir.
#ifdef GEODE_IS_ANDROID
        return std::filesystem::path("/storage/emulated/0/Android/media/com.geode.launcher/game/geode/macros");
#else
        return geode::prelude::Mod::get()->getSaveDir() / "macros";
#endif
    }

    void save() {
        author = GJAccountManager::get()->m_username;
        duration = inputs.size() > 0
            ? static_cast<float>(inputs.back().frame) / static_cast<float>(framerate)
            : 0.f;

        auto dir = macrosDir();
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            std::filesystem::create_directories(dir, ec);
        }
        std::ofstream f(dir / (name + ".gdr"), std::ios::binary);

        auto data = exportData(false);

        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        f.close();
    }

    static zReplay* fromFile(const std::string& fileName) {
        auto dir = macrosDir();
        std::error_code ec;
        if (std::filesystem::exists(dir, ec) || std::filesystem::create_directories(dir, ec)) {
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
