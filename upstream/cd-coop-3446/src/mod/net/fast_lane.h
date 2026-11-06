#pragma once
// Co-op+ low latency (2026-11-06): the other player's movement must not wait for the network loop. That loop also runs
// every co-op feature (enemy relay, activities, our state, revive, chat...): ~30 ms a pass with enemies around (lag
// tests lag3-lag6), so a Move of ours waited up to a pass to go out and one of theirs up to a pass to reach their
// copy. The fast lane is one thread that
//   - receives every datagram (1 ms waits): the other player's own movement (Move, StartAction, Rotate, WallUp,
//     Docking) goes to their copy at once (CoopLink's hot handler); every datagram, handled or not, is then queued for
//     the network loop, which does the session's bookkeeping and everything else as before;
//   - sends our own movement the moment the game captured it (queued by CoopLink's capture sink).
// The network loop waits on the queue instead of the transport.

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "common/protocol.h"
#include "common/transport.h"

namespace cdmp::net {

// The other player's own movement: the event kinds the fast lane hands straight to their copy.
bool fast_lane_hot(proto::EventKind kind);

struct FastDatagram {
    PeerAddr from;
    std::vector<uint8_t> bytes;
    bool event_handled = false;  // its GameEvent went to the copy already: the session only counts it
};

// Starts the thread on `transport` (which must outlive fast_lane_stop). `send_event` sends one of our events through
// the session (it must be safe from that thread).
void fast_lane_start(Transport& transport, std::function<void(proto::GameEvent)> send_event);
void fast_lane_stop();
bool fast_lane_running();

// Network loop: the next datagram, waiting up to `timeout_ms` (ends early on fast_lane_wake).
std::optional<FastDatagram> fast_lane_next(int timeout_ms);
// Ends the network loop's wait (something of ours waits in the slow outbox). Any thread.
void fast_lane_wake();

// Network loop, every pass: who the other player is and whether the session is up. Hot events go both ways only
// while connected (and only from that peer).
void fast_lane_set_peer(const std::optional<PeerAddr>& peer, bool connected);
std::optional<PeerAddr> fast_lane_peer();

// CoopLink: the handler for the other player's hot events (fast lane thread); returns true if it took the event.
// Empty = none (the events go to the network loop as before).
void fast_lane_set_hot_handler(std::function<bool(const proto::GameEvent&, uint32_t now_ms)> handler);
// CoopLink's capture sink: send one of our hot events now. False if the lane cannot (not running or not connected):
// the caller keeps it for the network loop.
bool fast_lane_send(const proto::GameEvent& ev);

}  // namespace cdmp::net
