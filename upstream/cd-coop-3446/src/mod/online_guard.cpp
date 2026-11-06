#include "mod/online_guard.h"

#include <windows.h>

#include <array>
#include <filesystem>
#include <format>

#include "mod/log.h"

namespace cdmp::online_guard {
namespace {

// Modules the common PC anti-cheats load into the protected game.
constexpr std::array kModules = {
    L"EasyAntiCheat.dll",     L"EasyAntiCheat_x64.dll", L"EasyAntiCheat_EOS.dll", L"BEClient_x64.dll",
    L"BEClient.dll",          L"BattlEye.dll",          L"x3.xem",                L"xhunter1.sys",
    L"DenuvoAntiCheat.dll",   L"anticheat_x64.dll",     L"ACE-Base64.dll",        L"npggNT.des",
};
// Files and folders the same anti-cheats install next to the game.
constexpr std::array kFiles = {
    L"EasyAntiCheat",        L"BattlEye",          L"start_protected_game.exe", L"EACLauncher.exe",
    L"BEService_x64.exe",    L"XIGNCODE",          L"DenuvoAntiCheat",
};

std::wstring exe_dir() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().wstring();
}

std::string narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w) s += c < 128 ? static_cast<char>(c) : '?';
    return s;
}

}  // namespace

bool first_instance() {
    wchar_t name[64];
    swprintf_s(name, L"Local\\cdmp_coopplus_%lu", GetCurrentProcessId());
    CreateMutexW(nullptr, FALSE, name);  // kept for the life of the process
    return GetLastError() != ERROR_ALREADY_EXISTS;
}

std::string compat_check() {
    const std::filesystem::path dir = exe_dir();
    std::error_code ec;
    std::string found;
    // Ultimate ASI Loader goes by many proxy names; ours is winmm.dll.
    for (const wchar_t* proxy : {L"version.dll", L"dinput8.dll", L"dsound.dll", L"winhttp.dll", L"xinput1_3.dll",
                                 L"d3d9.dll", L"d3d11.dll", L"wininet.dll", L"msacm32.dll"}) {
        if (std::filesystem::exists(dir / proxy, ec)) found += std::format("{}{} (another ASI loader?)", found.empty() ? "" : ", ", narrow(proxy));
    }
    if (std::filesystem::exists(dir / L"dxgi.dll", ec) || std::filesystem::exists(dir / L"ReShade64.dll", ec) ||
        std::filesystem::exists(dir / L"ReShade.ini", ec)) {
        found += std::string(found.empty() ? "" : ", ") + "ReShade / a dxgi.dll proxy";
    }
    for (const auto& e : std::filesystem::directory_iterator(dir, ec)) {
        const std::wstring n = e.path().filename().wstring();
        if (e.path().extension() == L".asi" && n != L"cdmp.asi" && n != L"coopplus.asi") found += std::format("{}{}", found.empty() ? "" : ", ", narrow(n));
    }
    if (found.empty()) {
        log::info("Compatibility: no other ASI loader, .asi mod or ReShade next to the game");
        return {};
    }
    log::warn("Compatibility: next to the game: {}. Co-op+ is tested alone; if anything misbehaves, take those out "
              "first (docs/compatibility.md)", found);
    return "Other mods next to the game: " + found + ". If co-op misbehaves, try without them first.";
}

std::string check(const std::wstring& extra_module) {
    std::wstring found;
    for (const wchar_t* m : kModules) {
        if (GetModuleHandleW(m)) {
            found = m;
            break;
        }
    }
    if (found.empty() && !extra_module.empty() && GetModuleHandleW(extra_module.c_str())) found = extra_module;
    if (found.empty()) {
        const std::wstring dir = exe_dir();
        std::error_code ec;
        for (const wchar_t* f : kFiles) {
            for (const auto& base : {std::filesystem::path(dir), std::filesystem::path(dir).parent_path()}) {
                if (std::filesystem::exists(base / f, ec)) {
                    found = f;
                    break;
                }
            }
            if (!found.empty()) break;
        }
    }
    if (found.empty()) {
        log::info("Online guard: no anti-cheat module or files found; co-op allowed");
        return {};
    }
    const std::string what = narrow(found);
    log::warn("Online guard: found {}: co-op stays off for this session", what);
    return std::format("Co-op+ is off: this game now uses anti-cheat ({}). Co-op+ will not run beside it, so your "
                       "account stays safe; the game runs normally. Remove Co-op+ (Uninstall Co-op+.cmd) and wait "
                       "for word that it is safe.",
                       what);
}

}  // namespace cdmp::online_guard
