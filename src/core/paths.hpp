#pragma once
#include <filesystem>
#include <string>

namespace cdcoop::paths {
// Folder our .asi lives in (the game folder, or its scripts/ / plugins/ subfolder).
std::filesystem::path module_dir();
// <module_dir>/cdcoop : config/, logs/, backups/
std::filesystem::path data_dir();
std::filesystem::path config_dir();
std::filesystem::path logs_dir();
std::filesystem::path exe_path();
std::string utf8(const std::filesystem::path& p);
}  // namespace cdcoop::paths
