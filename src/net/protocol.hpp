#pragma once
// Versioned binary wire protocol (little-endian). Transport-agnostic: net/steam_* sends these bytes over
// ISteamNetworkingMessages (unreliable channel for snapshots, reliable channel for events).
//
// Packet = Header + payload.  Header: magic "CDC1" (4) | protocol version u16 | message type u8 | payload length u16
// Unknown message types are skipped (forward compatible); a different protocol major version is rejected with a
// friendly "please update" message. See docs/protocol.md.
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace cdcoop::net {

inline constexpr std::array<std::uint8_t, 4> kMagic = {'C', 'D', 'C', '1'};
inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::size_t kHeaderSize = 4 + 2 + 1 + 2;
inline constexpr std::size_t kMaxPayload = 1100;  // stays under Steam's unreliable MTU (~1200)

enum class MsgType : std::uint8_t {
    Hello = 1,           // reliable: first message both ways
    HelloReply = 2,      // reliable: host accepts/refuses
    PlayerSnapshot = 3,  // unreliable, 10-20 Hz
    Chat = 4,            // reliable
    Ping = 5,            // unreliable
    Leave = 6,           // reliable
};

enum class RefuseReason : std::uint8_t { None = 0, ProtocolMismatch, GameVersionMismatch, SessionFull, ModMismatch };

struct Hello {
    std::uint16_t protocol_version = kProtocolVersion;
    std::string mod_version;
    std::string game_build;   // the exe's file version, compared host vs guest
    std::uint64_t steam_id = 0;
    std::string display_name;
    std::uint32_t content_hash = 0;  // hash of gameplay-affecting config/mods, warns on mismatch
};

struct HelloReply {
    bool accepted = false;
    RefuseReason reason = RefuseReason::None;
    std::uint8_t player_slot = 0;
    std::string message;  // human-readable, shown to the guest
};

// Movement state bits (gliding, climbing...) carried by snapshots.
enum MoveFlags : std::uint16_t {
    kGrounded = 1 << 0, kJumping = 1 << 1, kGliding = 1 << 2, kClimbing = 1 << 3,
    kSwimming = 1 << 4, kMounted = 1 << 5, kInCombat = 1 << 6, kDead = 1 << 7, kLedgeGrab = 1 << 8,
};

struct PlayerSnapshot {
    std::uint32_t tick = 0;        // sender's simulation tick
    std::uint8_t player_slot = 0;
    float pos[3] = {0, 0, 0};
    float vel[3] = {0, 0, 0};
    float yaw = 0;
    std::uint32_t anim_id = 0;     // game's animation/action id (found in Phase B)
    float anim_time = 0;
    std::uint16_t move_flags = 0;
    float health_frac = 1.0f;      // 0..1
};

struct Chat { std::uint8_t player_slot = 0; std::string text; };
struct Ping { std::uint32_t nonce = 0; std::uint64_t send_time_us = 0; };
struct Leave { std::uint8_t player_slot = 0; };

using Message = std::variant<Hello, HelloReply, PlayerSnapshot, Chat, Ping, Leave>;

std::vector<std::uint8_t> encode(const Message& m);

enum class DecodeError { None, TooShort, BadMagic, VersionMismatch, Truncated, UnknownType, BadField };

struct DecodeResult {
    std::optional<Message> message;
    DecodeError error = DecodeError::None;
    std::uint16_t peer_version = 0;
};

DecodeResult decode(std::span<const std::uint8_t> bytes);

// Low-level helpers (exposed for tests).
class Writer {
public:
    void u8(std::uint8_t v) { buf_.push_back(v); }
    void u16(std::uint16_t v);
    void u32(std::uint32_t v);
    void u64(std::uint64_t v);
    void f32(float v);
    void str(const std::string& s);  // u16 length + bytes (truncated at 512)
    std::vector<std::uint8_t>& bytes() { return buf_; }
private:
    std::vector<std::uint8_t> buf_;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> b) : b_(b) {}
    bool u8(std::uint8_t& v);
    bool u16(std::uint16_t& v);
    bool u32(std::uint32_t& v);
    bool u64(std::uint64_t& v);
    bool f32(float& v);
    bool str(std::string& s);
    std::size_t remaining() const { return b_.size() - pos_; }
private:
    std::span<const std::uint8_t> b_;
    std::size_t pos_ = 0;
};

}  // namespace cdcoop::net
