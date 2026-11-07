#pragma once
// Datagram transport behind the session: plain UDP (development on one PC,
// or a friend over a VPN / open port) or Steam networking (no port
// forwarding; src/mod/net/steam_transport.h). Datagrams are at most
// proto::kMaxPacketSize bytes, unreliable and unordered on both.

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>

#include "common/udp.h"

namespace cdmp::net {

// Address of the other player, whatever the transport. Never a memory address.
struct PeerAddr {
    enum class Kind : uint8_t { None, Udp, Steam };
    Kind kind = Kind::None;
    uint64_t value = 0;  // Udp: ip (network order) << 16 | port   Steam: SteamID64

    static PeerAddr udp(const Endpoint& ep) {
        return {Kind::Udp, (static_cast<uint64_t>(ep.ip) << 16) | ep.port};
    }
    static PeerAddr steam(uint64_t steam_id) { return {Kind::Steam, steam_id}; }
    Endpoint endpoint() const {
        return {static_cast<uint32_t>(value >> 16), static_cast<uint16_t>(value & 0xFFFF)};
    }
    bool valid() const { return kind != Kind::None; }
    std::string str() const {
        if (kind == Kind::Udp) return "udp:" + endpoint().str();
        if (kind == Kind::Steam) return "steam:" + std::to_string(value);
        return "none";
    }
    bool operator==(const PeerAddr&) const = default;
};

class Transport {
public:
    virtual ~Transport() = default;
    virtual bool send(const PeerAddr& to, std::span<const uint8_t> data) = 0;
    // Waits up to timeout_ms for one datagram; its size, or nullopt.
    virtual std::optional<size_t> recv(PeerAddr& from, std::span<uint8_t> buf, int timeout_ms) = 0;
    // Periodic housekeeping (accept sessions, status). Cheap; call every loop.
    virtual void poll(uint32_t /*now_ms*/) {}
    // False while it cannot send yet (e.g. Steam not logged on).
    virtual bool ready() const { return true; }
    virtual std::string status() const = 0;
};

class UdpTransport final : public Transport {
public:
    // port 0 = ephemeral (a guest); a host listens on a fixed port.
    // loopback_only keeps a single-PC test off the LAN and the firewall.
    bool open(uint16_t port, bool loopback_only) {
        if (!sock_.open(port, loopback_only)) return false;
        loopback_only_ = loopback_only;
        return true;
    }
    // Co-op+ direct play (Radmin VPN, LAN, any VPN): bound to one local address, and every datagram that does not come
    // from `only_ip` (the friend's address, network byte order) is dropped unread.
    //
    // Invites (2026-11-07, Plitos's handoff: "Copy invite / Paste invite"): `token` != 0 also lets in an address that
    // first sends a knock carrying that token (the invite's 64-bit random secret, new for every Host press), so the
    // host needs no guest address. `only_ip` 0 = only knocks. A guest given a token knocks before each send until the
    // host has answered. The knock never reaches the game protocol; a wrong one is dropped and counted.
    bool open_direct(const Endpoint& local, uint32_t only_ip, uint64_t token = 0, bool knock_as_guest = false) {
        if (!sock_.open_at(local.ip, local.port)) return false;
        loopback_only_ = local.is_loopback();
        local_ = local;
        allowed_n_ = 0;
        if (only_ip) allowed_[allowed_n_++] = only_ip;
        direct_ = true;
        token_ = token;
        knock_ = knock_as_guest && token != 0;
        return true;
    }
    static constexpr size_t kKnockSize = 16;  // "CDKNOCK1" + the token (little endian)
    bool send(const PeerAddr& to, std::span<const uint8_t> data) override {
        if (to.kind != PeerAddr::Kind::Udp) return false;
        if (knock_ && !heard_) {
            uint8_t k[kKnockSize] = {'C', 'D', 'K', 'N', 'O', 'C', 'K', '1'};
            for (int i = 0; i < 8; ++i) k[8 + i] = static_cast<uint8_t>(token_ >> (8 * i));
            sock_.send_to(to.endpoint(), std::span<const uint8_t>(k, kKnockSize));
        }
        return sock_.send_to(to.endpoint(), data);
    }
    std::optional<size_t> recv(PeerAddr& from, std::span<uint8_t> buf, int timeout_ms) override {
        Endpoint ep;
        auto n = sock_.recv_from(ep, buf, timeout_ms);
        if (n && direct_) {
            const bool knock = *n == kKnockSize && std::memcmp(buf.data(), "CDKNOCK1", 8) == 0;
            if (knock) {
                uint64_t t = 0;
                for (int i = 0; i < 8; ++i) t |= uint64_t{buf[8 + i]} << (8 * i);
                if (token_ && t == token_ && !allowed(ep.ip)) {
                    if (allowed_n_ < allowed_.size()) allowed_[allowed_n_++] = ep.ip;
                    ++admitted_;
                } else if (!token_ || t != token_) {
                    ++dropped_;
                }
                return std::nullopt;  // a knock is never game data
            }
            if (!allowed(ep.ip)) {
                ++dropped_;
                return std::nullopt;  // not our friend: never read
            }
            heard_ = true;
        }
        if (n) from = PeerAddr::udp(ep);
        return n;
    }
    std::string status() const override {
        if (direct_) {
            std::string s = "direct udp " + Endpoint{local_.ip, sock_.local_port()}.str();
            if (token_ && !knock_) s += " (invite";
            else s += " (";
            for (size_t i = 0; i < allowed_n_; ++i) {
                const std::string a = Endpoint{allowed_[i], 0}.str();
                s += (i || (token_ && !knock_) ? ", " : "only from ") + a.substr(0, a.rfind(':'));
            }
            return s + ")";
        }
        return "udp port " + std::to_string(sock_.local_port()) +
               (loopback_only_ ? " (loopback only)" : " (all interfaces)");
    }
    uint64_t dropped() const { return dropped_; }
    uint64_t admitted() const { return admitted_; }  // addresses let in by the invite's knock

private:
    bool allowed(uint32_t ip) const {
        for (size_t i = 0; i < allowed_n_; ++i) {
            if (allowed_[i] == ip) return true;
        }
        return false;
    }
    UdpSocket sock_;
    bool loopback_only_ = true;
    Endpoint local_{};
    bool direct_ = false;
    std::array<uint32_t, 4> allowed_{};
    std::atomic<size_t> allowed_n_{0};
    uint64_t token_ = 0;
    bool knock_ = false;
    std::atomic<bool> heard_{false};
    std::atomic<uint64_t> dropped_{0};
    std::atomic<uint64_t> admitted_{0};
};

}  // namespace cdmp::net
