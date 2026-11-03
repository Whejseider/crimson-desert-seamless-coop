#pragma once
// Save safety for co-op (Co-op+, brief 6.3: "always back up saves").
//
// Upstream's rule is "do not save while the other player is in your world": the
// other player's character is an actor our own server did not create, and a save
// taken then may reference it. Players forget, and the game autosaves on its own.
// The sniffer only observes, so saves cannot (and should not) be blocked. Instead:
//   - when a co-op session connects, the save folder is copied to
//     Documents\CrimsonDesert-CoopPlus-SaveBackups\auto-<date>-<time> (at most
//     every 10 minutes, the newest 10 kept), so there is always a copy from
//     before the other player arrived;
//   - a save request (SaveAutoReq / SaveToFileCurrentPlayerReq) seen while the
//     other player is connected is logged and shown on the panel, naming the
//     backup to restore if the save turns out damaged.

#include <cstddef>
#include <cstdint>
#include <string>

namespace cdmp::net {

void save_guard_configure(bool enabled);

// Network thread: the co-op session is (still) connected or not. A new
// connection triggers the backup.
void save_guard_set_connected(bool connected, const std::string& peer_name);

// Sniffer thread (capture_observe_c2s).
void save_guard_observe_c2s(const uint8_t* packet, size_t size);
// Sniffer thread (capture_observe_s2c): the server confirms every save, autosaves included.
void save_guard_observe_s2c(const uint8_t* packet, size_t size);

// Panel line ("" when nothing to say).
std::string save_guard_hud_text();

// Co-op+ (T29): why the last automatic backup failed, in plain words with the next step ("" = it worked / none yet).
std::string save_guard_problem();

}  // namespace cdmp::net
