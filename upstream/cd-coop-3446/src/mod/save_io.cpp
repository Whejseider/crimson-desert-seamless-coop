#include "mod/save_io.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <cwctype>
#include <filesystem>
#include <format>
#include <mutex>
#include <string>
#include <system_error>

#include <safetyhook.hpp>

#include "mod/log.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace cdmp::save_io {
namespace {

namespace fs = std::filesystem;

SafetyHookInline g_create, g_create2, g_move, g_replace, g_delete, g_mkdir, g_rmdir, g_attr, g_attrex, g_find,
    g_findex, g_copy, g_copyex, g_copy2, g_setattr;
bool g_log = false;

std::wstring g_game_root;   // lower case, backslashes: ...\pearl abyss\cd\save\ (no account)
std::wstring g_worlds_root; // Documents\CrimsonDesert-CoopPlus\Worlds\ (as Windows gave it)
std::mutex g_world_mutex;
std::wstring g_world;       // the active world's name ("" = none)
std::wstring g_world_root;  // g_worlds_root + name + backslash
std::atomic<bool> g_active{false};

fs::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p)) && p) out = p;
    CoTaskMemFree(p);
    return out;
}

std::string narrow(const std::wstring& w) {
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring lower_slashes(const wchar_t* p) {
    std::wstring s(p);
    for (auto& c : s) c = c == L'/' ? L'\\' : static_cast<wchar_t>(towlower(c));
    return s;
}

// Under the game's save folder? Then where the rest starts (the account part), else npos.
size_t account_part(const std::wstring& low) {
    if (g_game_root.empty() || low.compare(0, g_game_root.size(), g_game_root) != 0) return std::wstring::npos;
    size_t i = g_game_root.size();
    const size_t start = i;
    while (i < low.size() && iswdigit(low[i])) ++i;
    if (i == start || (i < low.size() && low[i] != L'\\')) return std::wstring::npos;  // save\<digits>[\...] only
    return start;
}

bool is_save_path(const wchar_t* p) { return p && account_part(lower_slashes(p)) != std::wstring::npos; }

// The path the game should use: the active world's copy, or the path itself.
std::wstring redirect(const wchar_t* p) {
    if (!p) return {};
    if (!g_active.load(std::memory_order_acquire)) return p;
    const std::wstring low = lower_slashes(p);
    const size_t at = account_part(low);
    if (at == std::wstring::npos) return p;
    std::wstring rest(p + at);
    for (auto& c : rest) c = c == L'/' ? L'\\' : c;
    std::lock_guard lock(g_world_mutex);
    return g_world_root + rest;
}

std::string caller() {
    void* frames[16] = {};
    const USHORT n = RtlCaptureStackBackTrace(2, 16, frames, nullptr);
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    std::string out;
    for (USHORT i = 0; i < n && out.size() < 60; ++i) {
        const auto a = reinterpret_cast<uintptr_t>(frames[i]);
        if (a >= base && a < base + 0x20000000) out += std::format(" exe+0x{:X}", a - base);
    }
    return out;
}

void note(const char* what, const wchar_t* p, const std::wstring& used, const char* result) {
    if (!g_log || !is_save_path(p)) return;
    log::info("SaveIO: {} {}{} -> {} |{}", what, narrow(p), used != p ? " => " + narrow(used) : std::string(), result,
              caller());
}

HANDLE WINAPI on_create(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD flags,
                        HANDLE tmpl) {
    const std::wstring p = redirect(name);
    const HANDLE h = g_create.stdcall<HANDLE>(name ? p.c_str() : name, access, share, sa, disp, flags, tmpl);
    note("CreateFileW", name, p, h == INVALID_HANDLE_VALUE ? "failed" : "ok");
    return h;
}

HANDLE WINAPI on_create2(LPCWSTR name, DWORD access, DWORD share, DWORD disp, void* params) {
    const std::wstring p = redirect(name);
    const HANDLE h = g_create2.stdcall<HANDLE>(name ? p.c_str() : name, access, share, disp, params);
    note("CreateFile2", name, p, h == INVALID_HANDLE_VALUE ? "failed" : "ok");
    return h;
}

BOOL WINAPI on_move(LPCWSTR from, LPCWSTR to, DWORD flags) {
    const std::wstring a = redirect(from), b = redirect(to);
    const BOOL ok = g_move.stdcall<BOOL>(from ? a.c_str() : from, to ? b.c_str() : to, flags);
    note("MoveFileExW", from, a, ok ? "ok" : "failed");
    return ok;
}

BOOL WINAPI on_replace(LPCWSTR target, LPCWSTR repl, LPCWSTR backup, DWORD flags, void* x, void* y) {
    const std::wstring a = redirect(target), b = redirect(repl), c = redirect(backup);
    const BOOL ok = g_replace.stdcall<BOOL>(target ? a.c_str() : target, repl ? b.c_str() : repl,
                                            backup ? c.c_str() : backup, flags, x, y);
    note("ReplaceFileW", target, a, ok ? "ok" : "failed");
    return ok;
}

BOOL WINAPI on_delete(LPCWSTR name) {
    const std::wstring p = redirect(name);
    const BOOL ok = g_delete.stdcall<BOOL>(name ? p.c_str() : name);
    note("DeleteFileW", name, p, ok ? "ok" : "failed");
    return ok;
}

BOOL WINAPI on_mkdir(LPCWSTR name, LPSECURITY_ATTRIBUTES sa) {
    const std::wstring p = redirect(name);
    const BOOL ok = g_mkdir.stdcall<BOOL>(name ? p.c_str() : name, sa);
    note("CreateDirectoryW", name, p, ok ? "ok" : "failed");
    return ok;
}

BOOL WINAPI on_rmdir(LPCWSTR name) {
    const std::wstring p = redirect(name);
    const BOOL ok = g_rmdir.stdcall<BOOL>(name ? p.c_str() : name);
    note("RemoveDirectoryW", name, p, ok ? "ok" : "failed");
    return ok;
}

DWORD WINAPI on_attr(LPCWSTR name) {
    const std::wstring p = redirect(name);
    const DWORD a = g_attr.stdcall<DWORD>(name ? p.c_str() : name);
    note("GetFileAttributesW", name, p, a == INVALID_FILE_ATTRIBUTES ? "none" : "ok");
    return a;
}

BOOL WINAPI on_attrex(LPCWSTR name, GET_FILEEX_INFO_LEVELS lvl, void* info) {
    const std::wstring p = redirect(name);
    const BOOL ok = g_attrex.stdcall<BOOL>(name ? p.c_str() : name, lvl, info);
    note("GetFileAttributesExW", name, p, ok ? "ok" : "none");
    return ok;
}

HANDLE WINAPI on_find(LPCWSTR name, LPWIN32_FIND_DATAW data) {
    const std::wstring p = redirect(name);
    const HANDLE h = g_find.stdcall<HANDLE>(name ? p.c_str() : name, data);
    note("FindFirstFileW", name, p, h == INVALID_HANDLE_VALUE ? "none" : "ok");
    return h;
}

HANDLE WINAPI on_findex(LPCWSTR name, FINDEX_INFO_LEVELS lvl, void* data, FINDEX_SEARCH_OPS op, void* filter,
                        DWORD fl) {
    const std::wstring p = redirect(name);
    const HANDLE h = g_findex.stdcall<HANDLE>(name ? p.c_str() : name, lvl, data, op, filter, fl);
    note("FindFirstFileExW", name, p, h == INVALID_HANDLE_VALUE ? "none" : "ok");
    return h;
}

// The save also copies files (a CopyFileW to the unredirected path failed with error 3 and the game rolled the save
// back, 2026-11-04): every copy call is redirected too.
BOOL WINAPI on_copy(LPCWSTR from, LPCWSTR to, BOOL fail_if_exists) {
    const std::wstring a = redirect(from), b = redirect(to);
    const BOOL ok = g_copy.stdcall<BOOL>(from ? a.c_str() : from, to ? b.c_str() : to, fail_if_exists);
    note("CopyFileW", to, b, ok ? "ok" : "failed");
    return ok;
}

BOOL WINAPI on_copyex(LPCWSTR from, LPCWSTR to, void* progress, void* data, LPBOOL cancel, DWORD flags) {
    const std::wstring a = redirect(from), b = redirect(to);
    const BOOL ok = g_copyex.stdcall<BOOL>(from ? a.c_str() : from, to ? b.c_str() : to, progress, data, cancel, flags);
    note("CopyFileExW", to, b, ok ? "ok" : "failed");
    return ok;
}

HRESULT WINAPI on_copy2(PCWSTR from, PCWSTR to, void* params) {
    const std::wstring a = redirect(from), b = redirect(to);
    const HRESULT hr = g_copy2.stdcall<HRESULT>(from ? a.c_str() : from, to ? b.c_str() : to, params);
    note("CopyFile2", to, b, SUCCEEDED(hr) ? "ok" : "failed");
    return hr;
}

BOOL WINAPI on_setattr(LPCWSTR name, DWORD attr) {
    const std::wstring p = redirect(name);
    const BOOL ok = g_setattr.stdcall<BOOL>(name ? p.c_str() : name, attr);
    note("SetFileAttributesW", name, p, ok ? "ok" : "failed");
    return ok;
}

template <class F>
bool hook(SafetyHookInline& h, const char* fn, F detour) {
    void* target = GetProcAddress(GetModuleHandleA("kernelbase.dll"), fn);
    if (target) h = safetyhook::create_inline(target, reinterpret_cast<void*>(detour));
    if (!h) log::warn("SaveIO: kernelbase!{} not hooked", fn);
    return static_cast<bool>(h);
}

}  // namespace

void install(bool log) {
    g_log = log;
    const fs::path game = known_folder(FOLDERID_LocalAppData) / L"Pearl Abyss" / L"CD" / L"save";
    g_game_root = lower_slashes((game.wstring() + L"\\").c_str());
    g_worlds_root = (known_folder(FOLDERID_Documents) / L"CrimsonDesert-CoopPlus" / L"Worlds").wstring() + L"\\";
    bool ok = true;
    ok &= hook(g_create, "CreateFileW", on_create);
    ok &= hook(g_create2, "CreateFile2", on_create2);
    ok &= hook(g_move, "MoveFileExW", on_move);
    ok &= hook(g_replace, "ReplaceFileW", on_replace);
    ok &= hook(g_delete, "DeleteFileW", on_delete);
    ok &= hook(g_mkdir, "CreateDirectoryW", on_mkdir);
    ok &= hook(g_rmdir, "RemoveDirectoryW", on_rmdir);
    ok &= hook(g_attr, "GetFileAttributesW", on_attr);
    ok &= hook(g_attrex, "GetFileAttributesExW", on_attrex);
    ok &= hook(g_find, "FindFirstFileW", on_find);
    ok &= hook(g_findex, "FindFirstFileExW", on_findex);
    ok &= hook(g_copy, "CopyFileW", on_copy);
    ok &= hook(g_copyex, "CopyFileExW", on_copyex);
    ok &= hook(g_copy2, "CopyFile2", on_copy2);
    ok &= hook(g_setattr, "SetFileAttributesW", on_setattr);
    log::info("SaveIO: file hooks {}{} (saves: {})", ok ? "installed" : "INCOMPLETE", log ? ", logging" : "",
              narrow(game.wstring()));
}

std::wstring world_folder(const std::wstring& name) { return g_worlds_root + name; }

bool set_world(const std::wstring& name, std::string* why) {
    const auto fail = [&](std::string w) {
        log::warn("SaveIO: world '{}' not used: {}", narrow(name), w);
        if (why) *why = std::move(w);
        return false;
    };
    if (name.empty()) {
        g_active.store(false, std::memory_order_release);
        std::lock_guard lock(g_world_mutex);
        g_world.clear();
        g_world_root.clear();
        log::info("SaveIO: own saves (no co-op world)");
        return true;
    }
    if (!g_create) return fail("the file hooks are not installed");
    for (wchar_t c : name) {
        if (!(iswalnum(c) || c == L'-' || c == L'_')) return fail("bad world name");
    }
    const fs::path root = g_worlds_root + name;
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        // A new world: a copy of the player's own slot folders (never the temporary _new / _backup ones). The
        // copy is made under a temporary name and renamed when complete, so a half copy is never used.
        const fs::path game = known_folder(FOLDERID_LocalAppData) / L"Pearl Abyss" / L"CD" / L"save";
        const fs::path tmp = root.wstring() + L".making";
        fs::remove_all(tmp, ec);
        fs::create_directories(tmp, ec);
        if (ec) return fail("could not create " + narrow(tmp.wstring()) + " (" + ec.message() + ")");
        size_t slots = 0;
        for (const auto& acct : fs::directory_iterator(game, ec)) {
            if (!acct.is_directory(ec)) continue;
            const std::wstring an = acct.path().filename().wstring();
            if (an.empty() || !std::all_of(an.begin(), an.end(), [](wchar_t c) { return iswdigit(c); })) continue;
            for (const auto& slot : fs::directory_iterator(acct.path(), ec)) {
                const std::wstring sn = slot.path().filename().wstring();
                if (!slot.is_directory(ec) || sn.rfind(L"slot", 0) != 0 || sn.find(L'_') != std::wstring::npos) continue;
                fs::create_directories(tmp / an / sn, ec);
                fs::copy(slot.path(), tmp / an / sn, fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                         ec);
                if (ec) return fail("copying " + narrow(slot.path().wstring()) + " failed (" + ec.message() + ")");
                ++slots;
            }
        }
        if (slots == 0) return fail("no save slots to copy");
        fs::rename(tmp, root, ec);
        if (ec) return fail("could not finish " + narrow(root.wstring()) + " (" + ec.message() + ")");
        log::info("SaveIO: new co-op world '{}': {} slot(s) copied from your own saves (they stay as they are)",
                  narrow(name), slots);
    }
    {
        std::lock_guard lock(g_world_mutex);
        g_world = name;
        g_world_root = root.wstring() + L"\\";
    }
    g_active.store(true, std::memory_order_release);
    log::info("SaveIO: co-op world '{}' active: the game's saves are {}", narrow(name), narrow(root.wstring()));
    return true;
}

std::wstring world() {
    std::lock_guard lock(g_world_mutex);
    return g_world;
}

}  // namespace cdmp::save_io
