#include "mod/crash_log.h"

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>

#include "mod/log.h"

namespace cdmp::crash_log {
namespace {

// Each faulting address is logged at most 3 times; guarded reads repeat the same few.
constexpr int kSlots = 1024;
std::atomic<uintptr_t> g_addr[kSlots];
std::atomic<int> g_count[kSlots];
std::atomic<int> g_lines{0};
constexpr int kMaxLines = 2000;
PVOID g_handler = nullptr;

bool first_times(uintptr_t addr) {
    size_t i = (addr >> 2) % kSlots;
    for (int probe = 0; probe < kSlots; ++probe, i = (i + 1) % kSlots) {
        uintptr_t cur = g_addr[i].load();
        if (cur == 0 && g_addr[i].compare_exchange_strong(cur, addr)) cur = addr;
        if (cur == addr) return g_count[i].fetch_add(1) < 3;
    }
    return false;
}

bool interesting(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    case 0xC0000374:  // STATUS_HEAP_CORRUPTION
    case 0xC0000409:  // STATUS_STACK_BUFFER_OVERRUN (fail-fast)
    case 0xE06D7363:  // C++ throw
        return true;
    default:
        return false;
    }
}

// "module+0xRVA" for an address, or the bare address.
std::string where(uintptr_t addr) {
    HMODULE mod = nullptr;
    char buf[MAX_PATH + 32];
    if (addr && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(addr), &mod) &&
        mod) {
        char path[MAX_PATH];
        const DWORD n = GetModuleFileNameA(mod, path, MAX_PATH);
        const char* name = path;
        for (DWORD i = 0; i < n; ++i) {
            if (path[i] == '\\' || path[i] == '/') name = path + i + 1;
        }
        std::snprintf(buf, sizeof buf, "%s+0x%llX", n ? name : "?",
                      static_cast<unsigned long long>(addr - reinterpret_cast<uintptr_t>(mod)));
    } else {
        std::snprintf(buf, sizeof buf, "0x%llX", static_cast<unsigned long long>(addr));
    }
    return buf;
}

// Up to max qwords from the stack that point into a loaded module; -1 if unreadable.
int stack_modules(uintptr_t rsp, uintptr_t* out, int max) {
    int found = 0;
    __try {
        const auto* sp = reinterpret_cast<const uintptr_t*>(rsp);
        for (int i = 0; i < 256 && found < max; ++i) {
            const uintptr_t v = sp[i];
            HMODULE mod = nullptr;
            if (v >= 0x10000 &&
                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(v), &mod) &&
                mod) {
                out[found++] = v;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return found ? found : -1;
    }
    return found;
}

LONG CALLBACK on_exception(EXCEPTION_POINTERS* ep) {
    const auto* rec = ep->ExceptionRecord;
    if (!interesting(rec->ExceptionCode)) return EXCEPTION_CONTINUE_SEARCH;
    if (!first_times(reinterpret_cast<uintptr_t>(rec->ExceptionAddress)) || g_lines.fetch_add(1) >= kMaxLines) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    const auto* ctx = ep->ContextRecord;
    std::string text = std::format("Crash log: exception 0x{:08X} at {} (thread {}){}",
                                   rec->ExceptionCode, where(reinterpret_cast<uintptr_t>(rec->ExceptionAddress)),
                                   GetCurrentThreadId(), rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE ? " non-continuable" : "");
    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
        text += std::format(" {} 0x{:X}", rec->ExceptionInformation[0] == 0   ? "reading"
                                          : rec->ExceptionInformation[0] == 1 ? "writing"
                                                                              : "executing",
                            rec->ExceptionInformation[1]);
    }
    // Return addresses on the stack that land in a module: a cheap backtrace that needs no
    // unwinding (good enough to tell the mod, Steam and the game apart).
    uintptr_t frames[8];
    const int found = stack_modules(ctx->Rsp, frames, 8);
    text += " | stack:";
    for (int i = 0; i < found; ++i) text += " " + where(frames[i]);
    if (found < 0) text += " (unreadable)";
    log::write(log::Level::Warn, text);
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void install() {
    if (!g_handler) g_handler = AddVectoredExceptionHandler(1, on_exception);
}

void note_detach(bool process_exit) {
    log::write(log::Level::Warn, process_exit ? "Crash log: process is exiting (ExitProcess)"
                                              : "Crash log: mod unloaded (FreeLibrary)");
}

}  // namespace cdmp::crash_log
