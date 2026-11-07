#include "mod/game/safe_memory.h"

#include <windows.h>

#include <atomic>
#include <cstring>

namespace cdmp::game {
namespace {

constexpr uintptr_t kMinAddress = 0x10000;
constexpr uintptr_t kMaxAddress = 0x00007FFFFFFFFFFFull;

// Co-op+ (2026-11-07, Borque's FPS report: 90 -> 25 -> 3 fps in a session, our network loop at 300-450 ms a loop):
// every read used to ask VirtualQuery first. VirtualQuery takes the process's address-space lock, which every
// VirtualAlloc / VirtualFree of the game's threads takes too; while the game streams the world (all the time in a
// session: two players, enemy copies loading) our reads and its allocations waited on each other, so the more the
// mod read, the slower the game's frames (slowmo.cpp saw the same: one VirtualQuery per packet, a load ~50 s late).
// Now a small per-thread cache of address ranges VirtualQuery already found committed and readable (not a guard
// page): a read inside one needs no system call. The copy itself stays under __try, so a range freed since is still
// safe (that range is then forgotten). Ranges are re-checked after kTtlMs. A guard page met anyway (a range reused
// for a new thread's stack) is put back as a guard page, so that thread's stack can still grow.
constexpr int kCache = 32;
constexpr uint64_t kTtlMs = 1000;

struct Range {
    uintptr_t lo = 0, hi = 0;
    uint64_t until = 0;
};
thread_local Range t_ok[kCache];
thread_local unsigned t_next = 0;

std::atomic<uint64_t> g_queries{0}, g_hits{0}, g_faults{0};

bool cached(uintptr_t addr, size_t size, uint64_t now) {
    for (const Range& r : t_ok) {
        if (addr >= r.lo && addr + size <= r.hi && now < r.until) return true;
    }
    return false;
}

void remember(uintptr_t lo, uintptr_t hi, uint64_t now) { t_ok[t_next++ % kCache] = {lo, hi, now + kTtlMs}; }

void forget(uintptr_t addr) {
    for (Range& r : t_ok) {
        if (addr >= r.lo && addr < r.hi) r = {};
    }
}

bool readable(DWORD protect) {
    if (protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    return (protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                       PAGE_EXECUTE_WRITECOPY)) != 0;
}

bool pages_readable(uintptr_t addr, size_t size, uint64_t now) {
    uintptr_t cur = addr;
    const uintptr_t end = addr + size;
    while (cur < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        g_queries.fetch_add(1, std::memory_order_relaxed);
        if (VirtualQuery(reinterpret_cast<LPCVOID>(cur), &mbi, sizeof mbi) == 0) return false;
        if (mbi.State != MEM_COMMIT || !readable(mbi.Protect)) return false;
        const auto lo = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        const uintptr_t hi = lo + mbi.RegionSize;
        remember(lo, hi, now);
        cur = hi;
    }
    return true;
}

struct Fault {
    DWORD code = 0;
    uintptr_t addr = 0;
};

int note_fault(EXCEPTION_POINTERS* ep, Fault* f) {
    f->code = ep->ExceptionRecord->ExceptionCode;
    if (ep->ExceptionRecord->NumberParameters >= 2) f->addr = ep->ExceptionRecord->ExceptionInformation[1];
    return EXCEPTION_EXECUTE_HANDLER;
}

// Kept free of C++ objects with destructors so __try is allowed here.
bool guarded_copy(void* out, const void* src, size_t size, Fault* f) {
    __try {
        std::memcpy(out, src, size);
        return true;
    } __except (note_fault(GetExceptionInformation(), f)) {
        return false;
    }
}

void after_fault(const Fault& f, uintptr_t addr) {
    g_faults.fetch_add(1, std::memory_order_relaxed);
    forget(addr);
    if (f.addr) forget(f.addr);
    if (f.code == STATUS_GUARD_PAGE_VIOLATION && f.addr) {
        // The system took the guard off that page when it raised this; put it back.
        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(f.addr), &mbi, sizeof mbi) && mbi.State == MEM_COMMIT) {
            DWORD old = 0;
            VirtualProtect(reinterpret_cast<LPVOID>(f.addr & ~uintptr_t{0xFFF}), 0x1000, mbi.Protect | PAGE_GUARD, &old);
        }
    }
}

}  // namespace

bool safe_read(uintptr_t addr, void* out, size_t size) {
    if (addr < kMinAddress || addr > kMaxAddress - size) return false;
    const uint64_t now = GetTickCount64();  // no system call (shared user data)
    if (cached(addr, size, now)) {
        g_hits.fetch_add(1, std::memory_order_relaxed);
    } else if (!pages_readable(addr, size, now)) {
        return false;
    }
    Fault f;
    if (guarded_copy(out, reinterpret_cast<const void*>(addr), size, &f)) return true;
    after_fault(f, addr);
    return false;
}

std::optional<uintptr_t> read_ptr(uintptr_t addr) {
    auto p = read<uintptr_t>(addr);
    if (!p || *p < kMinAddress || *p > kMaxAddress) return std::nullopt;
    return p;
}

bool has_vtable(uintptr_t obj, uintptr_t expected_vtable) {
    auto vt = read<uintptr_t>(obj);
    return vt && *vt == expected_vtable;
}

SafeReadStats safe_read_stats() {
    return {g_queries.load(std::memory_order_relaxed), g_hits.load(std::memory_order_relaxed),
            g_faults.load(std::memory_order_relaxed)};
}

}  // namespace cdmp::game
