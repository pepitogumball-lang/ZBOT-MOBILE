#ifndef _replay_hpp
#define _replay_hpp

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>
#include "macro_utils.hpp"
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
#include <stdexcept>
#include <exception>

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
    // Usar el save dir de Geode en todas las plataformas, incluido Android.
    // Esto evita problemas con scoped storage en Android 10+ donde la ruta
    // hardcodeada /storage/emulated/0/... puede no ser accesible.
    return geode::prelude::Mod::get()->getSaveDir() / "macros";
  }

  void cleanInputs(int minHoldFrames = 0, int minGapFrames = 0, bool dedupe = true) {
    // minHoldFrames/minGapFrames ya NO desplazan frames: hacerlo rompía la
    // sincronización frame-perfecta entre grabación y reproducción. Se
    // conservan en la firma por compatibilidad, pero no se usan.
    (void)minHoldFrames;
    (void)minGapFrames;

    if (inputs.empty()) return;

    // Ordenar siempre es seguro y necesario para el playback.
    zmacro::sortByFrame(inputs);
    // Deduplicar solo si el usuario lo tiene activado; respeta su elección.
    if (dedupe) zmacro::dedupeNoOps(inputs);
  }

  bool save(int minHoldFrames = 0, int minGapFrames = 0, bool dedupe = true) {
    // Autor: usar el nombre de cuenta si está disponible; si no, cadena vacía.
    if (auto* acc = GJAccountManager::get()) {
      author = acc->m_username;
    }

    if (name.empty()) {
      geode::log::warn("zReplay::save: nombre de macro vacío, no se guarda");
      return false;
    }

    cleanInputs(minHoldFrames, minGapFrames, dedupe);
    updateDuration();

    auto path = macrosDir() / (name + ".gdr");
    return writeBinary(path);
  }

  // Copia de seguridad periódica durante la grabación.
  //
  // Se escribe en "<nombre>.autosave.gdr" (NUNCA en el macro real) para que un
  // guardado intermedio no pueda pisar un run perfecto ya guardado. El archivo
  // sigue siendo un .gdr válido, así que aparece en la lista de macros y se
  // puede replicar si Android mata el juego a mitad de una grabación.
  bool saveSnapshot(int minHoldFrames = 0, int minGapFrames = 0, bool dedupe = true) {
    if (auto* acc = GJAccountManager::get()) {
      author = acc->m_username;
    }

    if (name.empty()) return false;

    cleanInputs(minHoldFrames, minGapFrames, dedupe);
    updateDuration();
    return writeBinary(macrosDir() / (name + ".autosave.gdr"));
  }

  // Borra la copia de seguridad (al conseguir guardar el run real).
  static void removeSnapshot(const std::string& snapName) {
    if (snapName.empty()) return;
    std::error_code ec;
    std::filesystem::remove(macrosDir() / (snapName + ".autosave.gdr"), ec);
  }

  void updateDuration() {
    duration = (!inputs.empty() && framerate > 0.f)
      ? static_cast<float>(inputs.back().frame) / framerate
      : 0.f;
  }

  // Escribe los datos exportados en 'path'. DRY: lo comparten el guardado
  // normal y las copias de seguridad.
  bool writeBinary(const std::filesystem::path& path) {
    auto dir = path.parent_path();
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
      std::filesystem::create_directories(dir, ec);
      if (ec) {
        geode::log::warn("zReplay::writeBinary: failed to create dir {}: {}",
          dir.string(), ec.message());
        return false;
      }
    }

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) {
      geode::log::warn("zReplay::writeBinary: failed to open {} for write",
        path.string());
      return false;
    }

    auto data = exportData(false);
    f.write(reinterpret_cast<const char*>(data.data()),
        static_cast<std::streamsize>(data.size()));
    if (!f.good()) {
      geode::log::warn("zReplay::writeBinary: write failed for {}",
        path.string());
      f.close();
      return false;
    }
    f.close();
    if (f.fail()) {
      geode::log::warn("zReplay::writeBinary: close/flush failed for {}",
        path.string());
      return false;
    }
    return true;
  }

  static zReplay* fromFile(const std::string& fileName) {
    // Rechazar nombres con separadores o traversal por seguridad.
    if (fileName.empty() || fileName.find('/') != std::string::npos
        || fileName.find('\\') != std::string::npos
        || fileName.find("..") != std::string::npos) {
      geode::log::warn("zReplay::fromFile: nombre inválido '{}'", fileName);
      return nullptr;
    }

    auto dir = macrosDir();
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
      std::filesystem::create_directories(dir, ec);
      return nullptr;
    }

    std::ifstream f(dir / (fileName + ".gdr"), std::ios::binary);
    if (!f.is_open()) {
      f = std::ifstream(dir / fileName, std::ios::binary);
      if (!f.is_open()) return nullptr;
    }

    f.seekg(0, std::ios::end);
    auto pos = f.tellg();
    f.seekg(0, std::ios::beg);

    constexpr std::streamoff kMaxMacroBytes = 64ll * 1024ll * 1024ll;
    if (pos <= 0 || pos > kMaxMacroBytes) {
      geode::log::warn("zReplay::fromFile: tamaño inválido {} para {}",
        static_cast<long long>(pos), fileName);
      return nullptr;
    }
    auto size = static_cast<std::size_t>(pos);

    std::vector<uint8_t> data(size);
    f.read(reinterpret_cast<char*>(data.data()),
        static_cast<std::streamsize>(size));
    if (!f.good() && !f.eof()) {
      geode::log::warn("zReplay::fromFile: fallo de lectura para {}", fileName);
      return nullptr;
    }
    f.close();

    // importData lanza excepciones con datos corruptos/truncados. Nunca debe
    // tirar el juego: se captura y se devuelve nullptr.
    zReplay* ret = nullptr;
    try {
      ret = new zReplay();
      *ret = importData(data);
    } catch (std::exception const& e) {
      geode::log::warn("zReplay::fromFile: archivo corrupto '{}': {}",
        fileName, e.what());
      delete ret;
      return nullptr;
    } catch (...) {
      geode::log::warn("zReplay::fromFile: archivo corrupto '{}' (excepción)",
        fileName);
      delete ret;
      return nullptr;
    }

    if (!ret) return nullptr;

    // Algunos GDR traen metadatos rotos (framerate 0, negativo o absurdo).
    // Eso haría duraciones infinitas y algoritmos derivados del tiempo.
    // El 240 es el valor por defecto de GDR.
    if (!(ret->framerate > 0.f) || ret->framerate > 100000.f) {
      geode::log::warn("zReplay::fromFile: framerate inválido ({}) en '{}', se usa 240",
        ret->framerate, fileName);
      ret->framerate = 240.f;
    }
    if (ret->framerate > 1000.f) {
      // Framerate "válido" pero poco realista (2.2 corre a ~60 y los macros se
      // guardan a 240 TPS). Se limita para que los cálculos no exploten.
      ret->framerate = 1000.f;
    }

    // Normalizar: ordenar por frame para garantizar monotonicidad en playback.
    zmacro::sanitize(ret->inputs);
    ret->name = fileName;
    return ret;
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
    if (fileName.find('/') != std::string::npos
        || fileName.find('\\') != std::string::npos
        || fileName.find("..") != std::string::npos) {
      geode::log::warn("zReplay::deleteByName: nombre inválido '{}'", fileName);
      return false;
    }
    auto dir = macrosDir();
    std::error_code ec;
    bool ok = std::filesystem::remove(dir / (fileName + ".gdr"), ec);
    if (!ok) {
      ok = std::filesystem::remove(dir / fileName, ec);
    }
    // Si existía, borrar también la copia de seguridad del mismo nombre para
    // que no quede un ".autosave.gdr" huérfano en la lista de macros.
    if (ok) {
      std::error_code ec2;
      std::filesystem::remove(dir / (fileName + ".autosave.gdr"), ec2);
    }
    return ok;
  }

// Replay data manipulation methods
  void purgeAfter(int frame) {
    zmacro::purgeAfterFrame(inputs, frame);
  }

  void addInput(int frame, int button, bool player2, bool down) {
    if (frame < 0) frame = 0;
    inputs.emplace_back(frame, button, player2, down);
  }
};

#endif
