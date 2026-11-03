#include "mod/net/coop_sync.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <format>
#include <mutex>
#include <utility>
#include <vector>

#include "mod/log.h"
#include "mod/net/coop_activity.h"
#include "mod/net/enemy_relay.h"
#include "mod/net/remote_puppet.h"

namespace cdmp::net {
namespace {

constexpr uint8_t kSyncType = 15;  // Activity event type: u16 copies we sent | u16 copies of theirs present here
constexpr uint32_t kDigestMs = 10000;
constexpr int kMismatches = 3;     // digests in a row (30 s: copies in flight settle well before)
constexpr uint32_t kTestAfterMs = 30000;

std::mutex g_mutex;
bool g_was_connected = false;
uint32_t g_connected_at = 0, g_last_digest = 0;
int g_misses = 0;
std::string g_resync;
bool g_test_done = false;
std::atomic<int> g_test{0};

}  // namespace

void sync_set_test(int mode) { g_test = mode; }

void sync_tick(bool connected, uint32_t now) {
    std::vector<uint8_t> digest;
    bool induce = false;
    {
        std::lock_guard lock(g_mutex);
        if (connected != g_was_connected) {
            g_was_connected = connected;
            g_connected_at = now;
            g_last_digest = now;
            g_misses = 0;
            g_test_done = false;
        }
        if (!connected) return;
        if (now - g_last_digest >= kDigestMs) {
            g_last_digest = now;
            const auto sent = static_cast<uint16_t>(enemy_copies_sent());
            const auto present = static_cast<uint16_t>(enemy_copies_present());
            digest.resize(4);
            std::memcpy(digest.data(), &sent, 2);
            std::memcpy(digest.data() + 2, &present, 2);
        }
        if (g_test.load() > 0 && !g_test_done && now - g_connected_at >= kTestAfterMs) {
            g_test_done = true;
            induce = true;
        }
    }
    if (!digest.empty()) activity_send(kSyncType, digest);
    if (induce) {
        const bool both = g_test.load() == 1;
        const bool removed = both && enemy_desync_test();
        puppet_set_frozen_ms(both ? 25000 : 60000);
        log::info("Sync: [dev] desync induced (enemy copy removed: {}, the other player's character frozen {} s)",
                  removed ? "yes" : "no", both ? 25 : 60);
    }
}

void sync_on_event(const uint8_t* d, size_t n) {
    if (n < 4) return;
    uint16_t their_sent = 0;
    std::memcpy(&their_sent, d, 2);
    const size_t present = enemy_copies_present();
    std::lock_guard lock(g_mutex);
    if (their_sent == present) {
        if (g_misses) log::info("Sync: enemy copies in step again ({})", present);
        g_misses = 0;
        return;
    }
    ++g_misses;
    log::info("Sync: the other player sent {} enemy copies, {} are here ({} digest{} in a row)", their_sent, present,
              g_misses, g_misses == 1 ? "" : "s");
    if (g_misses >= kMismatches && g_resync.empty()) {
        g_resync = std::format("their enemies: {} sent, {} here", their_sent, present);
        g_misses = 0;
    }
}

std::string sync_take_resync() {
    std::lock_guard lock(g_mutex);
    return std::exchange(g_resync, {});
}

}  // namespace cdmp::net
