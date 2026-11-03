#pragma once
// Co-op+ (roadmap T02): co-op never runs next to an online-integrity feature.
//
// Findings (docs/research/online-integrity.md): game 1.0.0.3479 ships no anti-cheat; Denuvo Anti-Tamper is DRM only
// (licence checks, no cheat scanning); the game "server" Co-op+ talks to is the in-process one of a single-player
// game. Should an update add an anti-cheat, its module would be loaded into the game or its files would sit in the
// game folder: then Co-op+ stays off for that session, says so on the panel and the game runs normally.

#include <string>

namespace cdmp::online_guard {

// Empty when nothing was found; otherwise the panel message (what was found, what to do).
// `extra_module` ([dev] online_guard_test): one more module name to look for, to test the guard.
std::string check(const std::wstring& extra_module = {});

}  // namespace cdmp::online_guard
