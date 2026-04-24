#ifndef _replay_hpp
#define _replay_hpp

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <system_error>
#include <cctype>

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

    // Sort by frame and drop "no-op" toggles so the on-disk macro is a
    // clean, monotonic input stream. Two passes:
    //   1) stable sort by frame (preserves intra-frame input order)
    //   2) per-(player, button) walk that drops any input matching the
    //      previous tracked state for that key.
    // After this runs, the macro looks like a deliberate, hand-crafted
    // input list — no orphaned releases, no double presses, no bouncing.
    void cleanInputs() {
        if (inputs.empty()) return;

        std::stable_sort(inputs.begin(), inputs.end(),
            [](const zInput& a, const zInput& b) {
                return a.frame < b.frame;
            });

        // 8 buttons * 2 players covers every PlayerButton value plus a
        // small headroom; -1 means "unknown / never seen".
        int held[2][8];
        for (int p = 0; p < 2; ++p)
            for (int b = 0; b < 8; ++b)
                held[p][b] = -1;

        std::vector<zInput> cleaned;
        cleaned.reserve(inputs.size());

        for (auto const& in : inputs) {
            int p = in.player2 ? 1 : 0;
            int b = in.button;
            if (b < 0 || b >= 8) {
                cleaned.push_back(in);
                continue;
            }
            int prev = held[p][b];
            int now = in.down ? 1 : 0;
            if (prev == now) continue; // no-op
            held[p][b] = now;
            cleaned.push_back(in);
        }

        inputs = std::move(cleaned);
    }

    void save() {
        author = GJAccountManager::get()->m_username;
        // Run the clean pass before serialising so saved macros are
        // always tidy regardless of how messy the recording was.
        cleanInputs();
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

    // Enumerate every .gdr file in the macros directory and return the
    // names sans extension. Sorted alphabetically so the in-game list
    // stays stable between refreshes. Errors are swallowed: an unreadable
    // directory just yields an empty vector so the GUI renders cleanly.
    static std::vector<std::string> listSaved() {
        std::vector<std::string> names;
        auto dir = macrosDir();

        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            std::filesystem::create_directories(dir, ec);
            return names;
        }

        for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            if (entry.path().extension() != ".gdr") continue;
            names.push_back(entry.path().stem().string());
        }

        std::sort(names.begin(), names.end(),
            [](const std::string& a, const std::string& b) {
                // Case-insensitive sort so "Alpha" and "alpha" sit next
                // to each other in the list.
                return std::lexicographical_compare(
                    a.begin(), a.end(), b.begin(), b.end(),
                    [](char x, char y) {
                        return std::tolower(static_cast<unsigned char>(x)) <
                               std::tolower(static_cast<unsigned char>(y));
                    });
            });

        return names;
    }

    // Delete a macro from disk by its display name (no extension).
    // Returns true on successful removal.
    static bool deleteByName(const std::string& fileName) {
        if (fileName.empty()) return false;
        auto dir = macrosDir();
        std::error_code ec;
        bool ok = std::filesystem::remove(dir / (fileName + ".gdr"), ec);
        if (!ok) {
            // Fall back to the raw name in case the file was saved
            // without the standard extension.
            ok = std::filesystem::remove(dir / fileName, ec);
        }
        return ok;
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
