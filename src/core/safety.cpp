#include "core/safety.hpp"

#include <windows.h>

#include <atomic>
#include <string>

#include "core/log.hpp"

namespace cdcoop::safety {
namespace { std::atomic<bool> g_active{true}; }

bool active() { return g_active.load(std::memory_order_acquire); }

void disable(std::string_view reason) {
    if (g_active.exchange(false))
        log::error(std::string("Co-op disabled, the game continues without it: ") + std::string(reason));
}

bool safe_read(std::uintptr_t addr, void* out, std::size_t len) {
    if (addr < 0x10000) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(addr), out, len, &got) && got == len;
}
}  // namespace cdcoop::safety
