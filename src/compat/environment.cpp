#include "compat/environment.hpp"

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

#include "core/log.hpp"
#include "core/paths.hpp"

namespace cdcoop::compat {
namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
std::string file_version(const std::wstring& path) {
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (!size) return {};
    std::vector<unsigned char> buf(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, buf.data())) return {};
    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\", reinterpret_cast<void**>(&ffi), &len) || !ffi) return {};
    return std::to_string(HIWORD(ffi->dwFileVersionMS)) + "." + std::to_string(LOWORD(ffi->dwFileVersionMS)) + "." +
           std::to_string(HIWORD(ffi->dwFileVersionLS)) + "." + std::to_string(LOWORD(ffi->dwFileVersionLS));
}
}  // namespace

Environment scan() {
    Environment e;
    const auto exe = paths::exe_path();
    e.exe_path = paths::utf8(exe);
    e.exe_name = paths::utf8(exe.filename());
    e.exe_version = file_version(exe.native());
    const std::string game_dir = lower(paths::utf8(exe.parent_path()));

    HMODULE mods[1024];
    DWORD needed = 0;
    if (!EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &needed)) return e;
    const std::size_t count = std::min<std::size_t>(needed / sizeof(HMODULE), std::size(mods));
    static const char* kProxies[] = {"dinput8.dll", "version.dll", "winmm.dll", "dxgi.dll", "d3d12.dll",
                                     "dsound.dll", "winhttp.dll", "xinput1_3.dll", "xinput1_4.dll"};
    for (std::size_t i = 0; i < count; ++i) {
        wchar_t buf[MAX_PATH * 2];
        const DWORD n = GetModuleFileNameW(mods[i], buf, static_cast<DWORD>(std::size(buf)));
        const std::filesystem::path p(std::wstring(buf, n));
        LoadedModule m{paths::utf8(p.filename()), paths::utf8(p), false};
        const std::string lp = lower(m.path), ln = lower(m.name);
        m.in_game_dir = lp.rfind(game_dir, 0) == 0;
        if (GetProcAddress(mods[i], "ReShadeRegisterAddon") || ln.find("reshade") != std::string::npos) e.reshade = true;
        if (ln == "steam_api64.dll") e.steam_api = true;
        if (m.in_game_dir)
            for (const char* px : kProxies)
                if (ln == px) e.proxy_dlls.push_back(m.name);
        e.modules.push_back(std::move(m));
    }
    return e;
}

void log_report(const Environment& e) {
    log::info("Game: " + e.exe_name + " version " + (e.exe_version.empty() ? "(none)" : e.exe_version) + " at " + e.exe_path);
    log::info(std::string("steam_api64.dll loaded: ") + (e.steam_api ? "yes" : "no") + ", ReShade: " + (e.reshade ? "yes" : "no"));
    std::string px;
    for (const auto& p : e.proxy_dlls) px += p + " ";
    log::info("Proxy DLLs from the game folder (mod loaders): " + (px.empty() ? std::string("none") : px));
    for (const auto& m : e.modules)
        if (m.in_game_dir) log::info("  game-folder module: " + m.path);
    log::debug("Total modules loaded: " + std::to_string(e.modules.size()));
}
}  // namespace cdcoop::compat
