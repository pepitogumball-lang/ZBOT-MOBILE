#pragma once
#include <Geode/Geode.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <mutex>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// ZBot diagnostic logger
// Writes to:  <geode-save-dir>/zbot_debug.log
// Also emits to Geode log::debug so it shows in the Geode log viewer.
// Call ZBotLogger::get().clear() to wipe the log (e.g. on level init).
// Call ZBotLogger::get().contents() to read all lines (for Copy button).
// ---------------------------------------------------------------------------

class ZBotLogger {
public:
    static ZBotLogger& get() {
        static ZBotLogger inst;
        return inst;
    }

    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lk(m_mtx);
        std::string line = ts() + " " + msg;
        m_buf += line + "
";
        log::debug("[ZBOT] {}", msg);
        if (m_file.is_open()) {
            m_file << line << "
";
            m_file.flush();
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_buf.clear();
        if (m_file.is_open()) {
            m_file.close();
        }
        openFile();
        m_buf = "";
    }

    std::string contents() {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_buf;
    }

private:
    ZBotLogger() { openFile(); }

    void openFile() {
        auto dir = Mod::get()->getSaveDir();
        m_file.open(dir / "zbot_debug.log", std::ios::out | std::ios::trunc);
    }

    std::string ts() {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
        return std::string(buf);
    }

    std::mutex   m_mtx;
    std::ofstream m_file;
    std::string  m_buf;
};

#define ZLOG(msg) do {     std::ostringstream _zoss;     _zoss << msg;     ZBotLogger::get().log(_zoss.str()); } while(0)
