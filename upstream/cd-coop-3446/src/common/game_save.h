#pragma once
// Co-op+ story sync: read and write the game's save.save files and move the story (quest progress) from one save
// into another. Pure logic, no Windows: unit-tested (tests/tests.cpp) and checked against tools/save/quest_sync.py
// on real saves (tools/savetool).
//
// File (header 0x80 bytes, little endian; layout from NattKh's CrimsonSaveEditor, GPL, checked on our saves):
//   +0x00 "SAVE" | +0x04 u16 version | +0x06 u16 flags | +0x12 u32 blob size | +0x16 u32 payload size
//   +0x1A 16-byte ChaCha20 nonce (first u32 = block counter) | +0x2A HMAC-SHA256 of the LZ4 block
//   +0x80 ChaCha20(LZ4 block(blob))
// Blob: +0x0E u16 tag | u16 0 | u16 type count | the schema (each type: name, u16 field count, fields: name, type
// name, u16 kind, u16 size, u32 aux) | u32 0 | u32 object count | u32 blob size | object table (class, -1, -1,
// offset, size) | the objects.
// Inside objects, every element has an 8-byte run (FF x 8, or an index in FactionSpawnStageManagerSaveData) then a
// u32 self pointer = its own position + 4, and its schema type index (u16) 3 bytes before the run. Anything that moves
// bytes must move those pointers, the object table and the blob size (a stale one crashed the game at load).
//
// Story: the save's QuestSaveData object (stage, mission, quest and quest-gauge states, last completed main quest).
// story_extract() packs it with the schema it was written under; story_apply() puts it in place of another save's,
// adding the types that save lacks, and checks the result. Saves whose shared types have different fields were
// written by different game builds: refused.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cdmp::save {

using Bytes = std::vector<uint8_t>;
using Header = std::array<uint8_t, 0x80>;

// Building blocks (exposed for the tests).
std::array<uint8_t, 32> sha256(const uint8_t* data, size_t size);
std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t key_size, const uint8_t* data, size_t size);
void chacha20(uint8_t* data, size_t size, const uint8_t key[32], const uint8_t nonce16[16]);
std::optional<Bytes> lz4_decompress(const uint8_t* src, size_t size, size_t out_size);
Bytes lz4_compress(const uint8_t* src, size_t size);

struct Decoded {
    Header header{};
    Bytes blob;
};
// nullopt + why: not a save, unknown version, damaged (HMAC), bad sizes.
std::optional<Decoded> decode(const Bytes& file, std::string* why);
// `nonce16`: 16 random bytes (the caller's).
Bytes encode(const Bytes& blob, const Header& header, const uint8_t nonce16[16]);

struct Field {
    std::string name, type;
    uint16_t kind = 0, size = 0;
    uint32_t aux = 0;
    bool operator==(const Field&) const = default;
};
struct Type {
    std::string name;
    std::vector<Field> fields;
    size_t start = 0, end = 0;  // its bytes in the blob, name included
};
struct Schema {
    std::vector<Type> types;
    size_t end = 0;  // where the object table's header starts
    int index_of(const std::string& name) const;
};
std::optional<Schema> parse_schema(const Bytes& blob);

struct Object {
    uint32_t cls, s1, s2, off, size;
};
// The object table; empty when it does not fit the blob.
std::vector<Object> parse_objects(const Bytes& blob, const Schema& schema);

// Positions (relative to `data`) of the 8-byte runs before a self pointer; `base`: where data[0] sits in the blob.
std::vector<size_t> self_pointers(const uint8_t* data, size_t size, size_t base, size_t from = 0);

// Story payload: what a host sends.
std::optional<Bytes> story_extract(const Bytes& blob, std::string* why);

struct StoryCount {
    uint32_t missions = 0, done = 0;  // MissionStateData entries, and those in state 5 (completed)
};
std::optional<StoryCount> story_count(const Bytes& payload);
std::optional<StoryCount> save_story_count(const Bytes& blob);

// `guest` with its story replaced by the payload's; nullopt + why on any problem (nothing is half done).
std::optional<Bytes> story_apply(const Bytes& guest, const Bytes& payload, std::string* why);

}  // namespace cdmp::save
