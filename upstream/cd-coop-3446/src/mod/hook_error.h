#pragma once
// Co-op+ (3549 port): the reason under a MidHook failure, for the log. MidHook's own type says only "the allocation"
// (0) or "the inline hook" (1) failed; this is the allocator's or the inline hook's own error under it (inline: 0 bad
// allocation, 1 undecodable instruction, 2 short jump in the trampoline, 3 IP-relative out of range, 4 unsupported
// instruction, 5 could not unprotect, 6 not enough space).
#include <safetyhook.hpp>

namespace cdmp {

inline int hook_failure_detail(const safetyhook::MidHook::Error& e) {
    return e.type == safetyhook::MidHook::Error::BAD_INLINE_HOOK ? static_cast<int>(e.inline_hook_error.type)
                                                                  : static_cast<int>(e.allocator_error);
}

}  // namespace cdmp
