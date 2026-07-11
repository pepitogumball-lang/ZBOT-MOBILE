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


struct zInput : gdr::Input {
  zInput() = default;

// Input data structure for replay events
  zInput(int frame, int button, bool player2, bool down)
    : Input(frame, button, player2, down) {}
};

struct zReplay : gdr::Replay<zReplay, zInput> {
  std::string name;

  //
  // Replay format initialization
  zReplay() : Replay("G-Macro-Android", "2.0.0") {}

  static std::filesystem::path macrosDir() {
#ifdef GEODE_IS_ANDROID
    return std::filesystem::path("/storage/emulated/0/Android/media/com.geode.launcher/game/geode/macros");
#else
    return geode::prelude::Mod::get()->getSaveDir() / "macros";
#endif
  }

  void cleanInputs(int minHoldFrames = 0, int minGapFrames = 0) {
    if (inputs.empty()) return;

    std::stable_sort(inputs.begin(), inputs.end(),
      [](const zInput& a, const zInput& b) {
        return a.frame < b.frame;
      });

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

    if (minHoldFrames > 0 || minGapFrames > 0) {
      int lastPress [2][8];
      int lastRelease[2][8];
      for (int p = 0; p < 2; ++p)
        for (int b = 0; b < 8; ++b) {
          lastPress[p][b]  = INT_MIN / 2;
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

      std::stable_sort(cleaned.begin(), cleaned.end(),
        [](const zInput& a, const zInput& b) {
          return a.frame < b.frame;
        });
    }

    inputs = std::move(cleaned);
  }

  bool save(int minHoldFrames = 0, int minGapFrames = 0) {
    author = GJAccountManager::get()->m_username;
    cleanInputs(minHoldFrames, minGapFrames);
    duration = inputs.size() > 0
      ? static_cast<float>(inputs.back().frame) / static_cast<float>(framerate)
      : 0.f;

    auto dir = macrosDir();
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
      std::filesystem::create_directories(dir, ec);
      if (ec) {
        geode::log::warn("zReplay::save: failed to create dir {}: {}",
          dir.string(), ec.message());
        return false;
      }
    }

    auto path = dir / (name + ".gdr");
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
      geode::log::warn("zReplay::save: failed to open {} for write",
        path.string());
      return false;
    }

    auto data = exportData(false);
    f.write(reinterpret_cast<const char*>(data.data()),
        static_cast<std::streamsize>(data.size()));
    if (!f.good()) {
      geode::log::warn("zReplay::save: write failed for {}", path.string());
      f.close();
      return false;
    }
    f.close();
    if (f.fail()) {
      geode::log::warn("zReplay::save: close/flush failed for {}",
        path.string());
      return false;
    }
    return true;
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
      auto pos = f.tellg();
      f.seekg(0, std::ios::beg);

      constexpr std::streamoff kMaxMacroBytes = 64ll * 1024ll * 1024ll;
      if (pos < 0 || pos > kMaxMacroBytes) {
        geode::log::warn("zReplay::fromFile: bad size {} for {}",
          static_cast<long long>(pos), fileName);
        return nullptr;
      }
      auto size = static_cast<std::size_t>(pos);

      std::vector<uint8_t> data(size);
      f.read(reinterpret_cast<char*>(data.data()),
          static_cast<std::streamsize>(size));
      if (!f.good() && !f.eof()) {
        geode::log::warn("zReplay::fromFile: read failed for {}",
          fileName);
        return nullptr;
      }
      f.close();

      zReplay* ret = new zReplay();
      *ret = importData(data);
      ret->name = fileName;

      return ret;
    }

    return nullptr;
  }

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
        return std::lexicographical_compare(
          a.begin(), a.end(), b.begin(), b.end(),
          [](char x, char y) {
            return std::tolower(static_cast<unsigned char>(x)) <
                std::tolower(static_cast<unsigned char>(y));
          });
      });

    return names;
  }

  struct MacroFileInfo {
    std::string name;    // display name (no extension)
    std::uintmax_t size;  // bytes on disk
    std::time_t mtime;   // unix seconds, last modification
  };

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

// File management and cleanup
  static bool deleteByName(const std::string& fileName) {
    if (fileName.empty()) return false;
    auto dir = macrosDir();
    std::error_code ec;
    bool ok = std::filesystem::remove(dir / (fileName + ".gdr"), ec);
    if (!ok) {
      ok = std::filesystem::remove(dir / fileName, ec);
    }
    return ok;
  }

// Replay data manipulation methods
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
