#pragma once
#include <Geode/Geode.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>

using namespace geode::prelude;

//
//
//

// arreglado lo del bug raro
enum class ZLogLevel { Debug, Info, Warn, Error };

struct ZLogEntry {
  ZLogLevel level;
  std::string tag;  // e.g. "RECORD", "PLAYBACK", "EPOCH", "ERROR"
  std::string text; // the message body
  std::string full; // pre-formatted line shown in the console
};

// funciona? si
class ZBotLogger {
public:
  static ZBotLogger& get() {
    static ZBotLogger inst;
    return inst;
  }

  void log(ZLogLevel level, const std::string& tag, const std::string& msg) {
    std::lock_guard<std::mutex> lk(m_mtx);
    std::string line = ts() + " [" + tag + "] " + msg;
    ZLogEntry e{ level, tag, msg, line };
    m_entries.push_back(e);
    if (m_entries.size() > 2000) {
      m_entries.erase(m_entries.begin(),
              m_entries.begin() + 500);
    }
    m_dirty = true;
    log::debug("[ZBOT/{}] {}", tag, msg);
    if (m_file.is_open()) {
      m_file << line << "\n";
      m_file.flush();
    }
  }

  void clear() {
    std::lock_guard<std::mutex> lk(m_mtx);
    m_entries.clear();
    m_dirty = true;
    if (m_file.is_open()) m_file.close();
    openFile();
  }

  std::vector<ZLogEntry> snapshot() {
    std::lock_guard<std::mutex> lk(m_mtx);
    return m_entries;
  }

  std::string allText() {
    std::lock_guard<std::mutex> lk(m_mtx);
    std::string out;
    out.reserve(m_entries.size() * 80);
    for (auto& e : m_entries) out += e.full + "\n";
    return out;
  }

// arreglado lo del bug raro
  bool dirty() {
    std::lock_guard<std::mutex> lk(m_mtx);
    bool d = m_dirty;
    m_dirty = false;
    return d;
  }

private:
  ZBotLogger() { openFile(); }

// esto lo hizo Alberto a las 3 am
  void openFile() {
    auto dir = Mod::get()->getSaveDir();
    m_file.open(dir / "zbot_debug.log",
          std::ios::out | std::ios::trunc);
  }

  std::string ts() {
    auto now = std::chrono::system_clock::now();
    auto t  = std::chrono::system_clock::to_time_t(now);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
  }

  std::mutex m_mtx;
  std::ofstream m_file;
  std::vector<ZLogEntry> m_entries;
  bool m_dirty = false;
};

// Convenience macros
#define ZLOG_DEBUG(tag, msg) do { std::ostringstream _s; _s << msg; \
  ZBotLogger::get().log(ZLogLevel::Debug, tag, _s.str()); } while(0)
#define ZLOG_INFO(tag, msg) do { std::ostringstream _s; _s << msg; \
  ZBotLogger::get().log(ZLogLevel::Info, tag, _s.str()); } while(0)
#define ZLOG_WARN(tag, msg) do { std::ostringstream _s; _s << msg; \
  ZBotLogger::get().log(ZLogLevel::Warn, tag, _s.str()); } while(0)
#define ZLOG_ERROR(tag, msg) do { std::ostringstream _s; _s << msg; \
  ZBotLogger::get().log(ZLogLevel::Error, tag, _s.str()); } while(0)

#define ZLOG(msg) do { std::ostringstream _s; _s << msg; \
  ZBotLogger::get().log(ZLogLevel::Info, "DBG", _s.str()); } while(0)
