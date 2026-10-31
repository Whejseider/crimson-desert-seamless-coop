#include "core/log.hpp"

#include <windows.h>

#include <cstdio>
#include <mutex>

namespace cdcoop::log {
namespace {
std::mutex g_mu;
FILE* g_file = nullptr;
Level g_min = Level::Info;
const char* name(Level l) {
    switch (l) { case Level::Debug: return "DEBUG"; case Level::Info: return "INFO "; case Level::Warn: return "WARN "; default: return "ERROR"; }
}
}  // namespace

void open(const std::filesystem::path& file, Level min_level) {
    std::lock_guard lk(g_mu);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    // Keep the previous session's log next to the new one for bug reports.
    std::filesystem::rename(file, std::filesystem::path(file).replace_extension(".prev.log"), ec);
    g_file = _wfopen(file.c_str(), L"wb");
    g_min = min_level;
}

void set_level(Level l) { std::lock_guard lk(g_mu); g_min = l; }

Level parse_level(std::string_view s) {
    if (s == "debug") return Level::Debug;
    if (s == "warn") return Level::Warn;
    if (s == "error") return Level::Error;
    return Level::Info;
}

void write(Level l, std::string_view msg) {
    std::lock_guard lk(g_mu);
    if (!g_file || l < g_min) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::fprintf(g_file, "%02u:%02u:%02u.%03u [%s] %.*s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, name(l),
                 static_cast<int>(msg.size()), msg.data());
    std::fflush(g_file);
}
}  // namespace cdcoop::log
