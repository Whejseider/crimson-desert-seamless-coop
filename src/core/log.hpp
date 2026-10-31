#pragma once
#include <filesystem>
#include <string_view>

namespace cdcoop::log {
enum class Level { Debug, Info, Warn, Error };
void open(const std::filesystem::path& file, Level min_level);
void set_level(Level l);
Level parse_level(std::string_view s);
void write(Level l, std::string_view msg);
inline void debug(std::string_view m) { write(Level::Debug, m); }
inline void info(std::string_view m) { write(Level::Info, m); }
inline void warn(std::string_view m) { write(Level::Warn, m); }
inline void error(std::string_view m) { write(Level::Error, m); }
}  // namespace cdcoop::log
