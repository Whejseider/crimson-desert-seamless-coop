#include "mod/net/fast_lane.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include "common/clock.h"
#include "mod/log.h"

namespace cdmp::net {
namespace {

constexpr size_t kMaxQueued = 4096;  // datagrams waiting for the network loop (a stalled loop drops the oldest)
constexpr size_t kMaxSends = 256;

std::thread g_thread;
std::atomic<bool> g_run{false};
Transport* g_transport = nullptr;
std::function<void(proto::GameEvent)> g_send_event;

std::mutex g_queue_mutex;
std::condition_variable g_queue_cv;
std::deque<FastDatagram> g_queue;
bool g_woken = false;

std::mutex g_peer_mutex;
std::optional<PeerAddr> g_peer;
std::atomic<bool> g_connected{false};

std::mutex g_hot_mutex;  // held while the handler runs, so it cannot be replaced (or its CoopLink go) meanwhile
std::function<bool(const proto::GameEvent&, uint32_t)> g_hot;

std::mutex g_send_mutex;
std::vector<proto::GameEvent> g_sends;

// The other player's hot event in this datagram, if it is one (cheap checks first: the kind byte comes late).
bool take_hot(std::span<const uint8_t> bytes, const PeerAddr& from, uint32_t now) {
    if (!g_connected.load(std::memory_order_acquire)) return false;
    {
        std::lock_guard lock(g_peer_mutex);
        if (!g_peer || !(*g_peer == from)) return false;
    }
    const auto packet = proto::decode(bytes);
    if (!packet || packet->version != proto::kVersion) return false;
    const auto* ev = std::get_if<proto::GameEvent>(&packet->body);
    if (!ev || !fast_lane_hot(ev->kind)) return false;
    std::lock_guard lock(g_hot_mutex);
    return g_hot && g_hot(*ev, now);
}

void send_pending() {
    std::vector<proto::GameEvent> out;
    {
        std::lock_guard lock(g_send_mutex);
        if (g_sends.empty()) return;
        out.swap(g_sends);
    }
    for (auto& ev : out) g_send_event(std::move(ev));
}

void run() {
    std::array<uint8_t, proto::kMaxPacketSize> buf{};
    uint64_t handled = 0, passed = 0;
    uint32_t log_at = now_ms();
    while (g_run.load(std::memory_order_acquire)) {
        send_pending();
        PeerAddr from;
        const auto n = g_transport->recv(from, buf, 1);
        if (!n) continue;
        const uint32_t now = now_ms();
        const std::span<const uint8_t> bytes(buf.data(), *n);
        const bool hot = take_hot(bytes, from, now);
        (hot ? handled : passed) += 1;
        {
            std::lock_guard lock(g_queue_mutex);
            if (g_queue.size() >= kMaxQueued) g_queue.pop_front();
            g_queue.push_back({from, std::vector<uint8_t>(bytes.begin(), bytes.end()), hot});
        }
        g_queue_cv.notify_one();
        if (now - log_at >= 30000) {
            log_at = now;
            log::info("Fast lane: {} datagrams to the copy at once, {} for the network loop", handled, passed);
        }
    }
}

}  // namespace

bool fast_lane_hot(proto::EventKind kind) {
    using K = proto::EventKind;
    // ... and their enemies' actions and positions (the copies of the host's enemies a guest sees, in step with them)
    return kind == K::Move || kind == K::StartAction || kind == K::Rotate || kind == K::WallUp || kind == K::Docking ||
           kind == K::EnemyAct || kind == K::EnemyPos;
}

void fast_lane_start(Transport& transport, std::function<void(proto::GameEvent)> send_event) {
    if (g_run.exchange(true)) return;
    g_transport = &transport;
    g_send_event = std::move(send_event);
    {
        std::lock_guard lock(g_queue_mutex);
        g_queue.clear();
        g_woken = false;
    }
    g_thread = std::thread(run);
    SetThreadPriority(g_thread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
    log::info("Fast lane: on (the other player's movement goes to their copy as it arrives; ours goes out as captured)");
}

void fast_lane_stop() {
    if (!g_run.exchange(false)) return;
    if (g_thread.joinable()) g_thread.join();
    g_connected = false;
    {
        std::lock_guard lock(g_send_mutex);
        g_sends.clear();
    }
    g_transport = nullptr;
    g_send_event = nullptr;
}

bool fast_lane_running() { return g_run.load(std::memory_order_acquire); }

std::optional<FastDatagram> fast_lane_next(int timeout_ms) {
    std::unique_lock lock(g_queue_mutex);
    if (g_queue.empty() && timeout_ms > 0 && !g_woken) {
        g_queue_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [] { return !g_queue.empty() || g_woken; });
    }
    g_woken = false;
    if (g_queue.empty()) return std::nullopt;
    FastDatagram d = std::move(g_queue.front());
    g_queue.pop_front();
    return d;
}

void fast_lane_wake() {
    {
        std::lock_guard lock(g_queue_mutex);
        g_woken = true;
    }
    g_queue_cv.notify_one();
}

void fast_lane_set_peer(const std::optional<PeerAddr>& peer, bool connected) {
    {
        std::lock_guard lock(g_peer_mutex);
        g_peer = peer;
    }
    g_connected.store(connected && peer.has_value(), std::memory_order_release);
}

std::optional<PeerAddr> fast_lane_peer() {
    std::lock_guard lock(g_peer_mutex);
    return g_peer;
}

void fast_lane_set_hot_handler(std::function<bool(const proto::GameEvent&, uint32_t)> handler) {
    std::lock_guard lock(g_hot_mutex);
    g_hot = std::move(handler);
}

bool fast_lane_send(const proto::GameEvent& ev) {
    if (!fast_lane_running() || !g_connected.load(std::memory_order_acquire)) return false;
    std::lock_guard lock(g_send_mutex);
    if (g_sends.size() >= kMaxSends) return true;  // the lane is stuck; dropping beats queueing old movement
    g_sends.push_back(ev);
    return true;
}

}  // namespace cdmp::net
