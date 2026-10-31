#pragma once
// What else is loaded into the game: loaders, ReShade, other mods. Logged at startup for bug reports and used for
// compatibility warnings (and later, the "friend lacks a mod" checks).
#include <string>
#include <vector>

namespace cdcoop::compat {
struct LoadedModule { std::string name; std::string path; bool in_game_dir = false; };
struct Environment {
    std::string exe_name;
    std::string exe_path;
    std::string exe_version;  // "a.b.c.d" from the version resource, or "" if none
    std::vector<LoadedModule> modules;
    bool reshade = false;
    bool steam_api = false;
    std::vector<std::string> proxy_dlls;  // dinput8/version/winmm/dxgi... loaded from the game folder
};
Environment scan();
void log_report(const Environment& e);
}  // namespace cdcoop::compat
