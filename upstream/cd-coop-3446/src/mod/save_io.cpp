#include "mod/save_io.h"

#include <windows.h>

#include <intrin.h>

#include <cwchar>
#include <format>
#include <string>

#include <safetyhook.hpp>

#include "mod/log.h"

namespace cdmp::save_io {
namespace {

SafetyHookInline g_create, g_create2, g_move, g_replace, g_delete;

bool is_save_path(const wchar_t* p) {
    if (!p) return false;
    std::wstring s(p);
    for (auto& c : s) c = static_cast<wchar_t>(towlower(c));
    return s.find(L"\\cd\\save\\") != std::wstring::npos || s.find(L"/cd/save/") != std::wstring::npos;
}

std::string narrow(const wchar_t* p) {
    if (!p) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, p, -1, nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, p, -1, out.data(), n, nullptr, nullptr);
    return out;
}

// Where in the game the call came from (the first return address inside the exe).
std::string caller() {
    void* frames[12] = {};
    const USHORT n = RtlCaptureStackBackTrace(2, 12, frames, nullptr);
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    std::string out;
    for (USHORT i = 0; i < n; ++i) {
        const auto a = reinterpret_cast<uintptr_t>(frames[i]);
        if (a >= base && a < base + 0x20000000) {
            out += std::format(" exe+0x{:X}", a - base);
            if (out.size() > 60) break;
        }
    }
    return out.empty() ? " (not from the exe)" : out;
}

HANDLE WINAPI on_create(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD flags,
                        HANDLE tmpl) {
    const HANDLE h = g_create.stdcall<HANDLE>(name, access, share, sa, disp, flags, tmpl);
    if (is_save_path(name)) {
        log::info("SaveIO: CreateFileW {} access 0x{:X} disposition {} -> {} | thread {} |{}", narrow(name), access,
                  disp, h == INVALID_HANDLE_VALUE ? "failed" : "ok", GetCurrentThreadId(), caller());
    }
    return h;
}

HANDLE WINAPI on_create2(LPCWSTR name, DWORD access, DWORD share, DWORD disp, void* params) {
    const HANDLE h = g_create2.stdcall<HANDLE>(name, access, share, disp, params);
    if (is_save_path(name)) {
        log::info("SaveIO: CreateFile2 {} access 0x{:X} disposition {} -> {} | thread {} |{}", narrow(name), access,
                  disp, h == INVALID_HANDLE_VALUE ? "failed" : "ok", GetCurrentThreadId(), caller());
    }
    return h;
}

BOOL WINAPI on_move(LPCWSTR from, LPCWSTR to, DWORD flags) {
    const BOOL ok = g_move.stdcall<BOOL>(from, to, flags);
    if (is_save_path(from) || is_save_path(to)) {
        log::info("SaveIO: MoveFileExW {} -> {} flags 0x{:X} -> {} |{}", narrow(from), narrow(to), flags,
                  ok ? "ok" : "failed", caller());
    }
    return ok;
}

BOOL WINAPI on_replace(LPCWSTR target, LPCWSTR repl, LPCWSTR backup, DWORD flags, void* a, void* b) {
    const BOOL ok = g_replace.stdcall<BOOL>(target, repl, backup, flags, a, b);
    if (is_save_path(target) || is_save_path(repl)) {
        log::info("SaveIO: ReplaceFileW {} <- {} -> {} |{}", narrow(target), narrow(repl), ok ? "ok" : "failed",
                  caller());
    }
    return ok;
}

BOOL WINAPI on_delete(LPCWSTR name) {
    const BOOL ok = g_delete.stdcall<BOOL>(name);
    if (is_save_path(name)) log::info("SaveIO: DeleteFileW {} -> {} |{}", narrow(name), ok ? "ok" : "failed", caller());
    return ok;
}

template <class F>
void hook(SafetyHookInline& h, const char* dll, const char* fn, F detour) {
    void* target = GetProcAddress(GetModuleHandleA(dll), fn);
    if (!target) {
        log::warn("SaveIO: {}!{} not found", dll, fn);
        return;
    }
    h = safetyhook::create_inline(target, reinterpret_cast<void*>(detour));
    log::info("SaveIO: {}!{} {}", dll, fn, h ? "hooked" : "hook failed");
}

}  // namespace

void install_log() {
    hook(g_create, "kernelbase.dll", "CreateFileW", on_create);
    hook(g_create2, "kernelbase.dll", "CreateFile2", on_create2);
    hook(g_move, "kernelbase.dll", "MoveFileExW", on_move);
    hook(g_replace, "kernelbase.dll", "ReplaceFileW", on_replace);
    hook(g_delete, "kernelbase.dll", "DeleteFileW", on_delete);
}

}  // namespace cdmp::save_io
