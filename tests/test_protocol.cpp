#include "net/protocol.hpp"
#include "test.hpp"

using namespace cdcoop::net;

TEST(protocol_snapshot_roundtrip) {
    PlayerSnapshot s;
    s.tick = 1234; s.player_slot = 2; s.pos[0] = 1.5f; s.pos[1] = -2.25f; s.pos[2] = 1000.125f;
    s.vel[2] = 3.0f; s.yaw = 1.57f; s.anim_id = 0xDEADBEEF; s.anim_time = 0.5f; s.move_flags = kGliding | kInCombat;
    s.health_frac = 0.75f;
    const auto bytes = encode(s);
    CHECK(bytes.size() < kMaxPayload);
    const auto r = decode(bytes);
    CHECK(r.error == DecodeError::None);
    const auto* d = std::get_if<PlayerSnapshot>(&*r.message);
    CHECK(d && d->tick == 1234 && d->player_slot == 2 && d->pos[2] == 1000.125f && d->anim_id == 0xDEADBEEF &&
          d->move_flags == (kGliding | kInCombat) && d->health_frac == 0.75f);
}

TEST(protocol_hello_roundtrip) {
    Hello h; h.mod_version = "0.1.0"; h.game_build = "1.2.3.4"; h.steam_id = 76561198000000000ull;
    h.display_name = "Kliff"; h.content_hash = 0xABCD1234;
    const auto r = decode(encode(h));
    const auto* d = std::get_if<Hello>(&*r.message);
    CHECK(d && d->display_name == "Kliff" && d->steam_id == 76561198000000000ull && d->content_hash == 0xABCD1234);
}

TEST(protocol_rejects_garbage_and_truncation) {
    CHECK(decode(std::vector<std::uint8_t>{1, 2, 3}).error == DecodeError::TooShort);
    CHECK(decode(std::vector<std::uint8_t>(20, 0)).error == DecodeError::BadMagic);
    auto bytes = encode(Ping{7, 99});
    bytes.pop_back();
    CHECK(decode(bytes).error == DecodeError::Truncated);
}

TEST(protocol_version_mismatch_is_reported) {
    auto bytes = encode(Leave{1});
    bytes[4] = 99;  // protocol version low byte
    const auto r = decode(bytes);
    CHECK(r.error == DecodeError::VersionMismatch && r.peer_version == 99);
}

TEST(protocol_unknown_type_is_skipped_not_crashed) {
    auto bytes = encode(Leave{1});
    bytes[6] = 200;
    CHECK(decode(bytes).error == DecodeError::UnknownType);
}

TEST(protocol_non_finite_snapshot_rejected) {
    PlayerSnapshot s; s.pos[0] = std::nanf("");
    CHECK(decode(encode(s)).error == DecodeError::BadField);
}

TEST(protocol_long_strings_truncated_safely) {
    Chat c; c.text = std::string(2000, 'x');
    const auto r = decode(encode(c));
    const auto* d = std::get_if<Chat>(&*r.message);
    CHECK(d && d->text.size() == 512);
}
