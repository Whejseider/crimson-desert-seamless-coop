#include "net/protocol.hpp"

#include <cmath>
#include <cstring>

namespace cdcoop::net {

void Writer::u16(std::uint16_t v) { u8(v & 0xff); u8(v >> 8); }
void Writer::u32(std::uint32_t v) { u16(v & 0xffff); u16(v >> 16); }
void Writer::u64(std::uint64_t v) { u32(static_cast<std::uint32_t>(v)); u32(static_cast<std::uint32_t>(v >> 32)); }
void Writer::f32(float v) { std::uint32_t u; std::memcpy(&u, &v, 4); u32(u); }
void Writer::str(const std::string& s) {
    const std::size_t n = s.size() > 512 ? 512 : s.size();
    u16(static_cast<std::uint16_t>(n));
    buf_.insert(buf_.end(), s.begin(), s.begin() + static_cast<std::ptrdiff_t>(n));
}

bool Reader::u8(std::uint8_t& v) { if (remaining() < 1) return false; v = b_[pos_++]; return true; }
bool Reader::u16(std::uint16_t& v) {
    std::uint8_t a, b; if (!u8(a) || !u8(b)) return false; v = static_cast<std::uint16_t>(a | (b << 8)); return true;
}
bool Reader::u32(std::uint32_t& v) {
    std::uint16_t a, b; if (!u16(a) || !u16(b)) return false; v = a | (static_cast<std::uint32_t>(b) << 16); return true;
}
bool Reader::u64(std::uint64_t& v) {
    std::uint32_t a, b; if (!u32(a) || !u32(b)) return false; v = a | (static_cast<std::uint64_t>(b) << 32); return true;
}
bool Reader::f32(float& v) { std::uint32_t u; if (!u32(u)) return false; std::memcpy(&v, &u, 4); return true; }
bool Reader::str(std::string& s) {
    std::uint16_t n; if (!u16(n) || n > 512 || remaining() < n) return false;
    s.assign(reinterpret_cast<const char*>(b_.data() + pos_), n); pos_ += n; return true;
}

namespace {

struct Encoder {
    Writer& w;
    MsgType operator()(const Hello& m) {
        w.u16(m.protocol_version); w.str(m.mod_version); w.str(m.game_build); w.u64(m.steam_id);
        w.str(m.display_name); w.u32(m.content_hash); return MsgType::Hello;
    }
    MsgType operator()(const HelloReply& m) {
        w.u8(m.accepted ? 1 : 0); w.u8(static_cast<std::uint8_t>(m.reason)); w.u8(m.player_slot); w.str(m.message);
        return MsgType::HelloReply;
    }
    MsgType operator()(const PlayerSnapshot& m) {
        w.u32(m.tick); w.u8(m.player_slot);
        for (float f : m.pos) w.f32(f);
        for (float f : m.vel) w.f32(f);
        w.f32(m.yaw); w.u32(m.anim_id); w.f32(m.anim_time); w.u16(m.move_flags); w.f32(m.health_frac);
        return MsgType::PlayerSnapshot;
    }
    MsgType operator()(const Chat& m) { w.u8(m.player_slot); w.str(m.text); return MsgType::Chat; }
    MsgType operator()(const Ping& m) { w.u32(m.nonce); w.u64(m.send_time_us); return MsgType::Ping; }
    MsgType operator()(const Leave& m) { w.u8(m.player_slot); return MsgType::Leave; }
};

bool finite_all(const PlayerSnapshot& s) {
    for (float f : s.pos) if (!std::isfinite(f)) return false;
    for (float f : s.vel) if (!std::isfinite(f)) return false;
    return std::isfinite(s.yaw) && std::isfinite(s.anim_time) && std::isfinite(s.health_frac);
}

}  // namespace

std::vector<std::uint8_t> encode(const Message& m) {
    Writer body;
    const MsgType type = std::visit(Encoder{body}, m);
    Writer out;
    for (auto b : kMagic) out.u8(b);
    out.u16(kProtocolVersion);
    out.u8(static_cast<std::uint8_t>(type));
    out.u16(static_cast<std::uint16_t>(body.bytes().size()));
    auto& o = out.bytes();
    o.insert(o.end(), body.bytes().begin(), body.bytes().end());
    return o;
}

DecodeResult decode(std::span<const std::uint8_t> bytes) {
    DecodeResult r;
    if (bytes.size() < kHeaderSize) { r.error = DecodeError::TooShort; return r; }
    if (std::memcmp(bytes.data(), kMagic.data(), 4) != 0) { r.error = DecodeError::BadMagic; return r; }
    Reader h(bytes.subspan(4));
    std::uint16_t ver, len; std::uint8_t type;
    h.u16(ver); h.u8(type); h.u16(len);
    r.peer_version = ver;
    if (ver != kProtocolVersion) { r.error = DecodeError::VersionMismatch; return r; }
    if (bytes.size() - kHeaderSize < len) { r.error = DecodeError::Truncated; return r; }
    Reader p(bytes.subspan(kHeaderSize, len));
    bool ok = false;
    switch (static_cast<MsgType>(type)) {
        case MsgType::Hello: {
            Hello m;
            ok = p.u16(m.protocol_version) && p.str(m.mod_version) && p.str(m.game_build) && p.u64(m.steam_id) &&
                 p.str(m.display_name) && p.u32(m.content_hash);
            if (ok) r.message = m;
            break;
        }
        case MsgType::HelloReply: {
            HelloReply m; std::uint8_t acc, reason;
            ok = p.u8(acc) && p.u8(reason) && p.u8(m.player_slot) && p.str(m.message);
            if (ok) { m.accepted = acc != 0; m.reason = static_cast<RefuseReason>(reason); r.message = m; }
            break;
        }
        case MsgType::PlayerSnapshot: {
            PlayerSnapshot m;
            ok = p.u32(m.tick) && p.u8(m.player_slot) && p.f32(m.pos[0]) && p.f32(m.pos[1]) && p.f32(m.pos[2]) &&
                 p.f32(m.vel[0]) && p.f32(m.vel[1]) && p.f32(m.vel[2]) && p.f32(m.yaw) && p.u32(m.anim_id) &&
                 p.f32(m.anim_time) && p.u16(m.move_flags) && p.f32(m.health_frac);
            if (ok && !finite_all(m)) { r.error = DecodeError::BadField; return r; }
            if (ok) r.message = m;
            break;
        }
        case MsgType::Chat: { Chat m; ok = p.u8(m.player_slot) && p.str(m.text); if (ok) r.message = m; break; }
        case MsgType::Ping: { Ping m; ok = p.u32(m.nonce) && p.u64(m.send_time_us); if (ok) r.message = m; break; }
        case MsgType::Leave: { Leave m; ok = p.u8(m.player_slot); if (ok) r.message = m; break; }
        default: r.error = DecodeError::UnknownType; return r;
    }
    if (!ok) r.error = DecodeError::Truncated;
    return r;
}

}  // namespace cdcoop::net
