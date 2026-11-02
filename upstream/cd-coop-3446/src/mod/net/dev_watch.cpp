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
    uint64_t stack[64];  // the first 64 qwords at rsp (return addresses among them)
};

constexpr int kMaxHits = 16;
Hit g_hits[kMaxHits];
std::atomic<int> g_count{0};
std::atomic<int> g_want{0};
std::atomic<bool> g_armed{false};
PVOID g_handler = nullptr;

LONG CALLBACK on_exception(EXCEPTION_POINTERS* e) {
    if (e->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP || !(e->ContextRecord->Dr6 & 1)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* c = e->ContextRecord;
    c->Dr6 = 0;
    if (g_armed.load()) {
        const int i = g_count.fetch_add(1);
        if (i < kMaxHits && i < g_want.load()) {
            Hit& h = g_hits[i];
            h.rip = c->Rip;
            const uint64_t r[16] = {c->Rax, c->Rcx, c->Rdx, c->Rbx, c->Rsp, c->Rbp, c->Rsi, c->Rdi,
                                    c->R8,  c->R9,  c->R10, c->R11, c->R12, c->R13, c->R14, c->R15};
            std::memcpy(h.regs, r, sizeof r);
            std::memcpy(&h.xmm0, &c->Xmm0, 4);
            std::memcpy(&h.xmm1, &c->Xmm1, 4);
            std::memcpy(h.stack, reinterpret_cast<const void*>(c->Rsp), sizeof h.stack);  // our own thread's stack
        }
        if (i + 1 >= g_want.load()) c->Dr7 &= ~1ull;  // this thread stops; the others are cleared by the worker
    } else {
        c->Dr7 &= ~1ull;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

// Dr0 = addr and its enable bits on every thread of the game except the caller.
void set_all(uintptr_t addr, int len, bool on) {
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
                if (on) {
                    c.Dr0 = addr;
                    c.Dr7 = (c.Dr7 & ~(0xFull << 16)) | 1ull | (1ull << 16) | (lenbits << 18);  // L0, write, length
                } else {
                    c.Dr7 &= ~1ull;
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

}  // namespace

void dev_watch_start(uintptr_t addr, int len, int hits) {
    if (g_armed.exchange(true)) {
        log::warn("Dev watch: one is already running");
        return;
    }
    if (!g_handler) g_handler = AddVectoredExceptionHandler(1, on_exception);
    g_count = 0;
    g_want = std::min(hits, kMaxHits);
    std::thread([addr, len] {
        set_all(addr, len, true);
        log::info("Dev watch: writes to 0x{:X} ({} bytes)", addr, len);
        for (int i = 0; i < 300 && g_count.load() < g_want.load(); ++i) Sleep(100);  // at most 30 s
        set_all(addr, len, false);
        g_armed = false;
        const auto base = reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr));
        const int n = std::min(g_count.load(), g_want.load());
        for (int i = 0; i < n; ++i) {
            const Hit& h = g_hits[i];
            log::info("Dev watch: hit {} at exe+0x{:X} | rax {:X} rcx {:X} rdx {:X} rbx {:X} rsi {:X} rdi {:X} r8 {:X} "
                      "r9 {:X} r14 {:X} r15 {:X} | xmm0 {} xmm1 {}",
                      i, h.rip - base, h.regs[0], h.regs[1], h.regs[2], h.regs[3], h.regs[6], h.regs[7], h.regs[8],
                      h.regs[9], h.regs[14], h.regs[15], h.xmm0, h.xmm1);
            if (i == 0) {
                std::string code;
                for (int k = 0; k < 64; ++k) {
                    if (h.stack[k] > base + 0x1000 && h.stack[k] < base + 0x6000000) {
                        code += std::format(" [rsp+0x{:X}]=exe+0x{:X}", k * 8, h.stack[k] - base);
                    }
                }
                log::info("Dev watch: code addresses on the stack:{}", code);
            }
        }
        if (n == 0) log::info("Dev watch: no writes to 0x{:X} in 30 s", addr);
    }).detach();
}

}  // namespace cdmp::net
