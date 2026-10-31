#pragma once
// Fail-safe: if anything the mod needs is missing or a hook fails, the mod switches itself off and the game keeps
// running as vanilla. Every subsystem checks safety::active() before touching the game.
#include <cstdint>
#include <string_view>

namespace cdcoop::safety {
bool active();
void disable(std::string_view reason);  // logs once, idempotent
// Reads memory of this process without crashing on a bad pointer (ReadProcessMemory on ourselves).
bool safe_read(std::uintptr_t addr, void* out, std::size_t len);
}  // namespace cdcoop::safety
