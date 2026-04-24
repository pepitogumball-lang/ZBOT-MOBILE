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
#include <chrono>
#include <ctime>
#include <cstdint>
#include <climits>

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
    // clean, monotonic input stream. Three passes:
    //   1) stable sort by frame (preserves intra-frame input order)
    //   2) per-(player, button) walk that drops any input matching the
    //      previous tracked state for that key.
    //   3) optional spam-safety pass that pushes events forward in time
    //      so a press is held for at least `minHoldFrames` and so
    //      consecutive press events of the same button are at least
    //      `minGapFrames` apart. This is what stops GD from eating
    //      same-frame press+release tuples produced by extreme spam
    //      taps. Pass 0/0 to skip the pass entirely.
    // After this runs, the macro looks like a deliberate, hand-crafted
    // input list — no orphaned releases, no double presses, no bouncing,
    // no sub-frame ghosts.
    void cleanInputs(int minHoldFrames = 0, int minGapFrames = 0) {
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

        // Spam-safety spacing pass. Track, per (player, button), the
        // frame of the last press and last release, and shove events
        // forward in time when they violate the minimum-spacing
        // contract. Frame numbers only ever increase, never decrease,
        // so we keep the input stream monotonic afterwards.
        if (minHoldFrames > 0 || minGapFrames > 0) {
            int lastPress  [2][8];
            int lastRelease[2][8];
            for (int p = 0; p < 2; ++p)
                for (int b = 0; b < 8; ++b) {
                    lastPress[p][b]   = INT_MIN / 2;
                    lastRelease[p][b] = INT_MIN / 2;
                }

            for (auto& in : cleaned) {
                int p = in.player2 ? 1 : 0;
                int b = in.button;
                if (b < 0 || b >= 8) continue;

                if (in.down) {
                    int minF = lastRelease[p][b] + minGapFrames;
                    if (in.frame < minF) in.frame = minF;
                    lastPress[p][b] = in.frame;
                } else {
                    int minF = lastPress[p][b] + minHoldFrames;
                    if (in.frame < minF) in.frame = minF;
                    lastRelease[p][b] = in.frame;
                }
            }

            // Spacing pushes can re-order events globally, so re-sort.
            std::stable_sort(cleaned.begin(), cleaned.end(),
                [](const zInput& a, const zInput& b) {
                    return a.frame < b.frame;
                });
        }

        inputs = std::move(cleaned);
    }

    void save(int minHoldFrames = 0, int minGapFrames = 0) {
        author = GJAccountManager::get()->m_username;
        // Run the clean pass before serialising so saved macros are
        // always tidy regardless of how messy the recording was.
        cleanInputs(minHoldFrames, minGapFrames);
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

    // Lightweight metadata-only entry for the saved-macros list. We
    // intentionally do NOT load the full input stream here — the GUI
    // shows hundreds of these and parsing every .gdr each refresh would
    // stutter the menu. File size + last-write time are cheap stat()
    // calls and good enough to help the user pick the right macro.
    struct MacroFileInfo {
        std::string name;       // display name (no extension)
        std::uintmax_t size;    // bytes on disk
        std::time_t mtime;      // unix seconds, last modification
    };

    // Same listing semantics as listSaved() but returns size + mtime
    // for each macro. Sorted by name (case-insensitive).
    static std::vector<MacroFileInfo> listSavedDetailed() {
        std::vector<MacroFileInfo> out;
        auto dir = macrosDir();

        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            std::filesystem::create_directories(dir, ec);
            return out;
        }

        for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            if (entry.path().extension() != ".gdr") continue;

            MacroFileInfo info;
            info.name = entry.path().stem().string();

            std::error_code sec;
            info.size = entry.file_size(sec);
            if (sec) info.size = 0;

            // file_clock -> system_clock conversion. std::chrono::clock_cast
            // is C++20 but still patchy on some Android NDK toolchains, so
            // we use the portable now()-anchored offset trick.
            sec.clear();
            auto ftime = entry.last_write_time(sec);
            if (sec) {
                info.mtime = 0;
            } else {
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - std::filesystem::file_time_type::clock::now()
                          + std::chrono::system_clock::now());
                info.mtime = std::chrono::system_clock::to_time_t(sctp);
            }

            out.push_back(std::move(info));
        }

        std::sort(out.begin(), out.end(),
            [](const MacroFileInfo& a, const MacroFileInfo& b) {
                return std::lexicographical_compare(
                    a.name.begin(), a.name.end(),
                    b.name.begin(), b.name.end(),
                    [](char x, char y) {
                        return std::tolower(static_cast<unsigned char>(x)) <
                               std::tolower(static_cast<unsigned char>(y));
                    });
            });

        return out;
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
