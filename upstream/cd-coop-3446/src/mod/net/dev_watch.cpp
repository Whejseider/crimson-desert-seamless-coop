#include "mod/net/dev_watch.h"

#include <windows.h>
#include <tlhelp32.h>

#include <atomic>
#include <cstring>
#include <format>
#include <string>
#include <thread>

#include "mod/log.h"

namespace cdmp::net {
namespace {

struct Hit {
    uint64_t rip, regs[16];
    float xmm0, xmm1;
    int slot;              // which watched address (debug register 0..3)
    uint64_t stack[256];   // the first 256 qwords at rsp (return addresses among them)
};

constexpr int kMaxHits = 16;
Hit g_hits[kMaxHits];
std::atomic<int> g_count{0};
std::atomic<int> g_want{0};
std::atomic<bool> g_armed{false};
std::atomic<uint64_t> g_addr[4] = {};  // what each debug register was set to (0 = unused)
PVOID g_handler = nullptr;

// The slot of a real hit of ours, or -1. The game's copy protection raises single-step exceptions of its own (with
// Dr6 bits set); those must go on to its handler untouched.
int our_slot(const CONTEXT* c) {
    const DWORD64 dr[4] = {c->Dr0, c->Dr1, c->Dr2, c->Dr3};
    for (int s = 0; s < 4; ++s) {
        const uint64_t a = g_addr[s].load();
        if ((c->Dr6 & (1ull << s)) && a && dr[s] == a && (c->Dr7 & (1ull << (s * 2)))) return s;
    }
    return -1;
}

LONG CALLBACK on_exception(EXCEPTION_POINTERS* e) {
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT* c = e->ContextRecord;
    const int slot = our_slot(c);
    if (slot < 0) return EXCEPTION_CONTINUE_SEARCH;
    c->Dr6 = 0;
    if (g_armed.load()) {
        const int i = g_count.fetch_add(1);
        if (i < kMaxHits && i < g_want.load()) {
            Hit& h = g_hits[i];
            h.rip = c->Rip;
            h.slot = slot;
            const uint64_t r[16] = {c->Rax, c->Rcx, c->Rdx, c->Rbx, c->Rsp, c->Rbp, c->Rsi, c->Rdi,
                                    c->R8,  c->R9,  c->R10, c->R11, c->R12, c->R13, c->R14, c->R15};
            std::memcpy(h.regs, r, sizeof r);
            std::memcpy(&h.xmm0, &c->Xmm0, 4);
            std::memcpy(&h.xmm1, &c->Xmm1, 4);
            std::memcpy(h.stack, reinterpret_cast<const void*>(c->Rsp), sizeof h.stack);  // our own thread's stack
        }
        if (i + 1 >= g_want.load()) c->Dr7 &= ~0xFFull;  // this thread stops; the others are cleared by the worker
    } else {
        c->Dr7 &= ~0xFFull;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

// Dr0..Dr3 = addrs and their enable bits on every thread of the game except the caller.
void set_all(const std::vector<uintptr_t>& addrs, int len, bool on) {
    const DWORD me = GetCurrentThreadId(), pid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te{sizeof te};
    const uint64_t lenbits = len == 1 ? 0 : len == 2 ? 1 : len == 8 ? 2 : 3;
    int n = 0;
    for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
        if (te.th32OwnerProcessID != pid || te.th32ThreadID == me) continue;
        HANDLE t = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (!t) continue;
        if (SuspendThread(t) != static_cast<DWORD>(-1)) {
            CONTEXT c{};
            c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if (GetThreadContext(t, &c)) {
                c.Dr7 &= ~0xFFFF00FFull;  // all four slots off
                if (on) {
                    DWORD64* dr[4] = {&c.Dr0, &c.Dr1, &c.Dr2, &c.Dr3};
                    for (size_t s = 0; s < addrs.size() && s < 4; ++s) {
                        *dr[s] = addrs[s];
                        c.Dr7 |= (1ull << (s * 2)) | (1ull << (16 + s * 4)) | (lenbits << (18 + s * 4));  // Ls, write, len
                    }
                }
                if (SetThreadContext(t, &c)) ++n;
            }
            ResumeThread(t);
        }
        CloseHandle(t);
    }
    CloseHandle(snap);
    log::info("Dev watch: {} on {} threads", on ? "set" : "cleared", n);
}

// "exe+0x..." or "<module>+0x..." for a code address, "" when it is in no module.
std::string where(uint64_t a) {
    HMODULE m = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(a), &m) || !m) {
        return {};
    }
    if (m == GetModuleHandleW(nullptr)) return std::format("exe+0x{:X}", a - reinterpret_cast<uint64_t>(m));
    char path[MAX_PATH] = {};
    GetModuleFileNameA(m, path, MAX_PATH);
    const char* name = std::strrchr(path, '\\');
    return std::format("{}+0x{:X}", name ? name + 1 : path, a - reinterpret_cast<uint64_t>(m));
}

}  // namespace

void dev_watch_start(uintptr_t addr, int len, int hits) { dev_watch_start_many({addr}, len, hits); }

void dev_watch_start_many(const std::vector<uintptr_t>& addrs, int len, int hits) {
    if (addrs.empty()) return;
    if (g_armed.exchange(true)) {
        log::warn("Dev watch: one is already running");
        return;
    }
    if (!g_handler) g_handler = AddVectoredExceptionHandler(1, on_exception);
    g_count = 0;
    g_want = std::min(hits, kMaxHits);
    for (size_t s = 0; s < 4; ++s) g_addr[s] = s < addrs.size() ? addrs[s] : 0;
    std::thread([addrs, len] {
        set_all(addrs, len, true);
        for (size_t s = 0; s < addrs.size() && s < 4; ++s) {
            log::info("Dev watch: slot {} = writes to 0x{:X} ({} bytes)", s, addrs[s], len);
        }
        for (int i = 0; i < 300 && g_count.load() < g_want.load(); ++i) Sleep(100);  // at most 30 s
        set_all(addrs, len, false);
        g_armed = false;
        for (auto& a : g_addr) a = 0;
        const int n = std::min(g_count.load(), g_want.load());
        for (int i = 0; i < n; ++i) {
            const Hit& h = g_hits[i];
            log::info("Dev watch: hit {} (slot {}) at {} | rax {:X} rcx {:X} rdx {:X} rbx {:X} rsi {:X} rdi {:X} r8 {:X} "
                      "r9 {:X} r14 {:X} r15 {:X} | xmm0 {} xmm1 {}",
                      i, h.slot, where(h.rip), h.regs[0], h.regs[1], h.regs[2], h.regs[3], h.regs[6], h.regs[7],
                      h.regs[8], h.regs[9], h.regs[14], h.regs[15], h.xmm0, h.xmm1);
            std::string code;
            for (int k = 0; k < 256; ++k) {
                if (h.stack[k] < 0x10000) continue;
                if (const std::string w = where(h.stack[k]); !w.empty()) code += std::format(" [rsp+0x{:X}]={}", k * 8, w);
            }
            log::info("Dev watch: code addresses on the stack:{}", code);
        }
        if (n == 0) log::info("Dev watch: no writes in 30 s");
    }).detach();
}

}  // namespace cdmp::net
