#include "common/game_save.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>

namespace cdmp::save {
namespace {

constexpr char kQuest[] = "QuestSaveData";
constexpr char kMission[] = "MissionStateData";
constexpr uint32_t kPayloadMagic = 0x54534443;  // "CDST"
constexpr uint32_t kPayloadVersion = 1;
constexpr size_t kMaxBlob = 64u << 20;           // a save is ~3.5 MB

uint32_t rd32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
uint16_t rd16(const uint8_t* p) {
    uint16_t v;
    std::memcpy(&v, p, 2);
    return v;
}
void wr32(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }
void wr16(uint8_t* p, uint16_t v) { std::memcpy(p, &v, 2); }
void put32(Bytes& b, uint32_t v) {
    const size_t at = b.size();
    b.resize(at + 4);
    wr32(b.data() + at, v);
}
bool fail(std::string* why, const char* text) {
    if (why) *why = text;
    return false;
}

// --- SHA-256 / HMAC ---------------------------------------------------------------------------------------------
constexpr uint32_t kK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98,
    0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8,
    0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
    0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2};

uint32_t rotr(uint32_t v, int n) { return (v >> n) | (v << (32 - n)); }
uint32_t rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

struct Sha256 {
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t buf[64]{};
    size_t used = 0;
    uint64_t total = 0;

    void block(const uint8_t* p) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t{p[i * 4]} << 24) | (uint32_t{p[i * 4 + 1]} << 16) | (uint32_t{p[i * 4 + 2]} << 8) | p[i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], k = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t t1 = k + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + kK[i] + w[i];
            const uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            k = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += k;
    }
    void add(const uint8_t* p, size_t n) {
        total += n;
        while (n) {
            const size_t take = std::min(n, 64 - used);
            std::memcpy(buf + used, p, take);
            used += take; p += take; n -= take;
            if (used == 64) {
                block(buf);
                used = 0;
            }
        }
    }
    std::array<uint8_t, 32> finish() {
        const uint64_t bits = total * 8;
        const uint8_t one = 0x80, zero = 0;
        add(&one, 1);
        while (used != 56) add(&zero, 1);
        uint8_t len[8];
        for (int i = 0; i < 8; ++i) len[i] = static_cast<uint8_t>(bits >> (56 - i * 8));
        add(len, 8);
        std::array<uint8_t, 32> out{};
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 4; ++j) out[i * 4 + j] = static_cast<uint8_t>(h[i] >> (24 - j * 8));
        return out;
    }
};

// --- the key ----------------------------------------------------------------------------------------------------
bool key_for(uint16_t version, uint8_t key[32]) {
    static constexpr uint8_t kBase[31] = {0xC4, 0x1B, 0x8E, 0x73, 0x0D, 0xF2, 0x59, 0xA6, 0x37, 0xCC, 0x04,
                                          0xE9, 0xB1, 0x2F, 0x96, 0x68, 0xDA, 0x10, 0x7A, 0x85, 0x3E, 0x61,
                                          0xF9, 0x22, 0x4D, 0xB8, 0x0A, 0xD7, 0x5C, 0x13, 0xEF};
    std::string material;
    if (version == 1) {
        material = "^Qgbrm/.#@`zsr]\\@rvfal#\"";
    } else if (version == 2) {
        material = "^Pearl--#Abyss__@!!";
    } else {
        return false;
    }
    material += "PRIVATE_HMAC_SECRET_CHECK";
    for (int i = 0; i < 31; ++i) key[i] = kBase[i] ^ static_cast<uint8_t>(material[i]);
    key[31] = 0;
    return true;
}

// --- schema helpers ---------------------------------------------------------------------------------------------
struct Reader {
    const Bytes& b;
    size_t pos;
    bool ok = true;
    uint16_t u16() {
        if (pos + 2 > b.size()) { ok = false; return 0; }
        const uint16_t v = rd16(b.data() + pos);
        pos += 2;
        return v;
    }
    uint32_t u32() {
        if (pos + 4 > b.size()) { ok = false; return 0; }
        const uint32_t v = rd32(b.data() + pos);
        pos += 4;
        return v;
    }
    std::string str() {
        const uint32_t n = u32();
        if (!ok || n > 4096 || pos + n > b.size()) { ok = false; return {}; }
        std::string s(reinterpret_cast<const char*>(b.data() + pos), n);
        pos += n;
        return s;
    }
};

struct Payload {
    Bytes schema_blob;  // 0x0E zero bytes + the host's schema, so parse_schema() reads it
    Schema schema;
    uint32_t host_off = 0;
    Bytes block;
};

std::optional<Payload> parse_payload(const Bytes& p, std::string* why) {
    Reader r{p, 0};
    if (r.u32() != kPayloadMagic || !r.ok) { fail(why, "not story data"); return std::nullopt; }
    if (r.u32() != kPayloadVersion) { fail(why, "story data from a different mod version"); return std::nullopt; }
    Payload out;
    out.host_off = r.u32();
    const uint32_t slen = r.u32();
    if (!r.ok || slen > (1u << 20) || r.pos + slen > p.size()) { fail(why, "damaged story data"); return std::nullopt; }
    out.schema_blob.assign(0x0E, 0);
    out.schema_blob.insert(out.schema_blob.end(), p.begin() + r.pos, p.begin() + r.pos + slen);
    r.pos += slen;
    const uint32_t blen = r.u32();
    if (!r.ok || r.pos + blen != p.size() || blen > kMaxBlob) { fail(why, "damaged story data"); return std::nullopt; }
    out.block.assign(p.begin() + r.pos, p.end());
    auto s = parse_schema(out.schema_blob);
    if (!s || s->end != out.schema_blob.size()) { fail(why, "damaged story data (types)"); return std::nullopt; }
    out.schema = std::move(*s);
    return out;
}

// The one object of class `name`, or nullopt.
std::optional<size_t> find_object(const std::vector<Object>& objs, const Schema& s, const char* name) {
    std::optional<size_t> hit;
    for (size_t i = 0; i < objs.size(); ++i) {
        if (s.types[objs[i].cls].name != name) continue;
        if (hit) return std::nullopt;
        hit = i;
    }
    return hit;
}

// Copy of `data` with every element's pointer and type index zeroed (for comparisons across positions/schemas).
Bytes normalized(const uint8_t* data, size_t size, size_t base, std::vector<size_t>* runs = nullptr) {
    Bytes out(data, data + size);
    const auto r = self_pointers(data, size, base, 3);
    for (size_t at : r) {
        wr32(out.data() + at + 8, 0);
        wr16(out.data() + at - 3, 0);
    }
    if (runs) *runs = r;
    return out;
}

StoryCount count_missions(const uint8_t* block, size_t size, size_t base, int mission_type) {
    StoryCount c;
    if (mission_type < 0) return c;
    for (size_t at : self_pointers(block, size, base, 7)) {
        if (rd16(block + at - 3) != mission_type || at + 21 > size) continue;
        ++c.missions;
        // [u16 mask length = 2][mask][u16 type][u8][run][pointer][u32][u32 key][u8 state]: key and state present.
        if (rd16(block + at - 7) == 2 && (block[at - 5] & 3) == 3 && block[at + 20] == 5) ++c.done;
    }
    return c;
}

}  // namespace

std::array<uint8_t, 32> sha256(const uint8_t* data, size_t size) {
    Sha256 s;
    s.add(data, size);
    return s.finish();
}

std::array<uint8_t, 32> hmac_sha256(const uint8_t* key, size_t key_size, const uint8_t* data, size_t size) {
    uint8_t k[64]{};
    if (key_size > 64) {
        const auto h = sha256(key, key_size);
        std::memcpy(k, h.data(), 32);
    } else {
        std::memcpy(k, key, key_size);
    }
    uint8_t ipad[64], opad[64];
    for (int i = 0; i < 64; ++i) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }
    Sha256 in;
    in.add(ipad, 64);
    in.add(data, size);
    const auto inner = in.finish();
    Sha256 out;
    out.add(opad, 64);
    out.add(inner.data(), 32);
    return out.finish();
}

void chacha20(uint8_t* data, size_t size, const uint8_t key[32], const uint8_t nonce16[16]) {
    uint32_t s[16] = {0x61707865, 0x3320646E, 0x79622D32, 0x6B206574};
    for (int i = 0; i < 8; ++i) s[4 + i] = rd32(key + i * 4);
    for (int i = 0; i < 4; ++i) s[12 + i] = rd32(nonce16 + i * 4);
    for (size_t pos = 0; pos < size; pos += 64) {
        uint32_t w[16];
        std::memcpy(w, s, sizeof w);
        for (int r = 0; r < 10; ++r) {
            static constexpr int q[8][4] = {{0, 4, 8, 12}, {1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15},
                                            {0, 5, 10, 15}, {1, 6, 11, 12}, {2, 7, 8, 13}, {3, 4, 9, 14}};
            for (const auto& e : q) {
                uint32_t &a = w[e[0]], &b = w[e[1]], &c = w[e[2]], &d = w[e[3]];
                a += b; d = rotl(d ^ a, 16);
                c += d; b = rotl(b ^ c, 12);
                a += b; d = rotl(d ^ a, 8);
                c += d; b = rotl(b ^ c, 7);
            }
        }
        uint8_t ks[64];
        for (int i = 0; i < 16; ++i) wr32(ks + i * 4, w[i] + s[i]);
        const size_t n = std::min<size_t>(64, size - pos);
        for (size_t i = 0; i < n; ++i) data[pos + i] ^= ks[i];
        ++s[12];
    }
}

std::optional<Bytes> lz4_decompress(const uint8_t* src, size_t size, size_t out_size) {
    Bytes out;
    out.reserve(out_size);
    size_t i = 0;
    while (i < size) {
        const uint8_t tok = src[i++];
        size_t lit = tok >> 4;
        if (lit == 15) {
            uint8_t b;
            do {
                if (i >= size) return std::nullopt;
                b = src[i++];
                lit += b;
            } while (b == 255);
        }
        if (i + lit > size || out.size() + lit > out_size) return std::nullopt;
        out.insert(out.end(), src + i, src + i + lit);
        i += lit;
        if (i >= size) break;
        if (i + 2 > size) return std::nullopt;
        const size_t off = src[i] | (src[i + 1] << 8);
        i += 2;
        size_t ml = tok & 15;
        if (ml == 15) {
            uint8_t b;
            do {
                if (i >= size) return std::nullopt;
                b = src[i++];
                ml += b;
            } while (b == 255);
        }
        ml += 4;
        if (off == 0 || off > out.size() || out.size() + ml > out_size) return std::nullopt;
        const size_t start = out.size() - off;
        for (size_t k = 0; k < ml; ++k) out.push_back(out[start + k]);
    }
    if (out.size() != out_size) return std::nullopt;
    return out;
}

Bytes lz4_compress(const uint8_t* src, size_t n) {
    // Greedy, 4-byte hash, 64 KB window; the last 5 bytes are literals and no match starts in the last 12.
    Bytes out;
    out.reserve(n / 2 + 16);
    std::vector<int64_t> table(1 << 16, -1);
    const auto put_len = [&](size_t v) {
        while (v >= 255) {
            out.push_back(255);
            v -= 255;
        }
        out.push_back(static_cast<uint8_t>(v));
    };
    size_t anchor = 0, i = 0;
    const size_t limit = n > 12 ? n - 12 : 0;
    while (i < limit) {
        const uint32_t seq = rd32(src + i);
        const uint32_t h = (seq * 2654435761u) >> 16;
        const int64_t cand = table[h];
        table[h] = static_cast<int64_t>(i);
        if (cand < 0 || i - static_cast<size_t>(cand) > 0xFFFF || rd32(src + cand) != seq) {
            ++i;
            continue;
        }
        size_t ml = 4;
        const size_t maxm = n - 5 - i;
        while (ml < maxm && src[cand + ml] == src[i + ml]) ++ml;
        const size_t lit = i - anchor;
        out.push_back(static_cast<uint8_t>((std::min<size_t>(lit, 15) << 4) | std::min<size_t>(ml - 4, 15)));
        if (lit >= 15) put_len(lit - 15);
        out.insert(out.end(), src + anchor, src + i);
        const size_t off = i - static_cast<size_t>(cand);
        out.push_back(static_cast<uint8_t>(off));
        out.push_back(static_cast<uint8_t>(off >> 8));
        if (ml - 4 >= 15) put_len(ml - 4 - 15);
        i += ml;
        anchor = i;
    }
    const size_t lit = n - anchor;
    out.push_back(static_cast<uint8_t>(std::min<size_t>(lit, 15) << 4));
    if (lit >= 15) put_len(lit - 15);
    out.insert(out.end(), src + anchor, src + n);
    return out;
}

std::optional<Decoded> decode(const Bytes& file, std::string* why) {
    if (file.size() < 0x80 || std::memcmp(file.data(), "SAVE", 4) != 0) {
        fail(why, "not a Crimson Desert save");
        return std::nullopt;
    }
    uint8_t key[32];
    if (!key_for(rd16(file.data() + 4), key)) {
        fail(why, "a save format this mod does not know");
        return std::nullopt;
    }
    const uint32_t usize = rd32(file.data() + 0x12), psize = rd32(file.data() + 0x16);
    if (psize > file.size() - 0x80 || usize > kMaxBlob) {
        fail(why, "the save file is cut short");
        return std::nullopt;
    }
    Bytes comp(file.begin() + 0x80, file.begin() + 0x80 + psize);
    chacha20(comp.data(), comp.size(), key, file.data() + 0x1A);
    const auto mac = hmac_sha256(key, 32, comp.data(), comp.size());
    if (std::memcmp(mac.data(), file.data() + 0x2A, 32) != 0) {
        fail(why, "the save file is damaged (its check failed)");
        return std::nullopt;
    }
    auto blob = lz4_decompress(comp.data(), comp.size(), usize);
    if (!blob) {
        fail(why, "the save file is damaged (bad compression)");
        return std::nullopt;
    }
    Decoded d;
    std::memcpy(d.header.data(), file.data(), 0x80);
    d.blob = std::move(*blob);
    return d;
}

Bytes encode(const Bytes& blob, const Header& header, const uint8_t nonce16[16]) {
    uint8_t key[32];
    key_for(rd16(header.data() + 4), key);
    Bytes comp = lz4_compress(blob.data(), blob.size());
    Bytes out(header.begin(), header.end());
    wr32(out.data() + 0x12, static_cast<uint32_t>(blob.size()));
    wr32(out.data() + 0x16, static_cast<uint32_t>(comp.size()));
    std::memcpy(out.data() + 0x1A, nonce16, 16);
    const auto mac = hmac_sha256(key, 32, comp.data(), comp.size());
    std::memcpy(out.data() + 0x2A, mac.data(), 32);
    chacha20(comp.data(), comp.size(), key, nonce16);
    out.insert(out.end(), comp.begin(), comp.end());
    return out;
}

int Schema::index_of(const std::string& name) const {
    for (size_t i = 0; i < types.size(); ++i)
        if (types[i].name == name) return static_cast<int>(i);
    return -1;
}

std::optional<Schema> parse_schema(const Bytes& blob) {
    Reader r{blob, 0x0E};
    r.u16();
    r.u16();
    const uint16_t count = r.u16();
    if (!r.ok || count == 0) return std::nullopt;
    Schema s;
    size_t start = r.pos;
    std::string name = r.str();
    for (uint16_t i = 0; i < count && r.ok; ++i) {
        Type t;
        t.name = std::move(name);
        t.start = start;
        const uint16_t fc = r.u16();
        for (uint16_t f = 0; f < fc && r.ok; ++f) {
            Field fl;
            fl.name = r.str();
            fl.type = r.str();
            fl.kind = r.u16();
            fl.size = r.u16();
            fl.aux = r.u32();
            t.fields.push_back(std::move(fl));
        }
        t.end = r.pos;
        s.types.push_back(std::move(t));
        if (i != count - 1) {
            start = r.pos;
            name = r.str();
        }
    }
    if (!r.ok) return std::nullopt;
    s.end = r.pos;
    return s;
}

std::vector<Object> parse_objects(const Bytes& blob, const Schema& schema) {
    if (schema.end + 12 > blob.size()) return {};
    const uint32_t count = rd32(blob.data() + schema.end + 4);
    const size_t base = schema.end + 12;
    if (count > 100000 || base + size_t{count} * 20 > blob.size()) return {};
    std::vector<Object> out(count);
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* e = blob.data() + base + i * 20;
        out[i] = {rd32(e), rd32(e + 4), rd32(e + 8), rd32(e + 12), rd32(e + 16)};
        if (out[i].cls >= schema.types.size() || out[i].off > blob.size() || out[i].size > blob.size() - out[i].off)
            return {};
    }
    return out;
}

std::vector<size_t> self_pointers(const uint8_t* data, size_t size, size_t base, size_t from) {
    std::vector<size_t> out;
    for (size_t q = from + 8; q + 4 <= size; ++q) {
        if (rd32(data + q) == base + q + 4) out.push_back(q - 8);
    }
    return out;
}

std::optional<Bytes> story_extract(const Bytes& blob, std::string* why) {
    const auto s = parse_schema(blob);
    if (!s) { fail(why, "could not read the save's layout"); return std::nullopt; }
    const auto objs = parse_objects(blob, *s);
    const auto q = find_object(objs, *s, kQuest);
    if (!q) { fail(why, "no quest data in the save"); return std::nullopt; }
    const Object& o = objs[*q];
    Bytes p;
    put32(p, kPayloadMagic);
    put32(p, kPayloadVersion);
    put32(p, o.off);
    put32(p, static_cast<uint32_t>(s->end - 0x0E));
    p.insert(p.end(), blob.begin() + 0x0E, blob.begin() + s->end);
    put32(p, o.size);
    p.insert(p.end(), blob.begin() + o.off, blob.begin() + o.off + o.size);
    return p;
}

std::optional<StoryCount> story_count(const Bytes& payload) {
    const auto p = parse_payload(payload, nullptr);
    if (!p) return std::nullopt;
    return count_missions(p->block.data(), p->block.size(), p->host_off, p->schema.index_of(kMission));
}

std::optional<StoryCount> save_story_count(const Bytes& blob) {
    const auto p = story_extract(blob, nullptr);
    if (!p) return std::nullopt;
    return story_count(*p);
}

std::optional<Bytes> story_apply(const Bytes& guest, const Bytes& payload, std::string* why) {
    const auto p = parse_payload(payload, why);
    if (!p) return std::nullopt;
    const Schema& hs = p->schema;
    const auto gs = parse_schema(guest);
    if (!gs) { fail(why, "could not read your save's layout"); return std::nullopt; }
    const auto gobjs = parse_objects(guest, *gs);
    const auto gq = find_object(gobjs, *gs, kQuest);
    if (!gq) { fail(why, "no quest data in your save"); return std::nullopt; }

    // The same game build wrote both: every type both saves know has the same fields.
    for (const Type& t : hs.types) {
        const int gi = gs->index_of(t.name);
        if (gi >= 0 && gs->types[gi].fields != t.fields) {
            fail(why, "your save and your friend's are from different game versions (update both games, load and "
                      "save once, then try again)");
            return std::nullopt;
        }
    }

    // The host's elements and their types; types your save lacks are appended to its schema.
    const Bytes& blk = p->block;
    const auto runs = self_pointers(blk.data(), blk.size(), p->host_off, 3);
    std::vector<std::string> missing;
    std::map<uint16_t, uint16_t> remap;
    for (size_t at : runs) {
        const uint16_t hi = rd16(blk.data() + at - 3);
        if (hi >= hs.types.size()) { fail(why, "damaged story data (element types)"); return std::nullopt; }
        if (remap.count(hi)) continue;
        int gi = gs->index_of(hs.types[hi].name);
        if (gi < 0) {
            gi = static_cast<int>(gs->types.size() + missing.size());
            missing.push_back(hs.types[hi].name);
        }
        remap[hi] = static_cast<uint16_t>(gi);
    }
    Bytes extra;
    for (const std::string& n : missing) {
        const Type& t = hs.types[hs.index_of(n)];
        extra.insert(extra.end(), p->schema_blob.begin() + t.start, p->schema_blob.begin() + t.end);
    }
    if (gs->types.size() + missing.size() > 0xFFFF) { fail(why, "too many types"); return std::nullopt; }

    const Object& qo = gobjs[*gq];
    const size_t d = extra.size();
    const int64_t growth = static_cast<int64_t>(blk.size()) - qo.size;
    const size_t qend = size_t{qo.off} + qo.size;
    if (qo.off < gs->end) { fail(why, "unexpected save layout"); return std::nullopt; }

    // Build: schema + new types | tables and objects before the quests | the host's quests | the rest.
    Bytes out;
    out.reserve(guest.size() + d + blk.size());
    out.insert(out.end(), guest.begin(), guest.begin() + gs->end);
    out.insert(out.end(), extra.begin(), extra.end());
    out.insert(out.end(), guest.begin() + gs->end, guest.begin() + qo.off);
    out.insert(out.end(), blk.begin(), blk.end());
    out.insert(out.end(), guest.begin() + qend, guest.end());
    wr16(out.data() + 0x12, static_cast<uint16_t>(gs->types.size() + missing.size()));
    const size_t new_end = gs->end + d;
    wr32(out.data() + new_end + 8, static_cast<uint32_t>(out.size()));
    const auto moved = [&](size_t pos) -> size_t {  // where a byte of the guest's lands
        return pos < qo.off ? pos + d : static_cast<size_t>(static_cast<int64_t>(pos + d) + growth);
    };
    for (size_t i = 0; i < gobjs.size(); ++i) {
        uint8_t* e = out.data() + new_end + 12 + i * 20;
        if (i == *gq) {
            wr32(e + 12, static_cast<uint32_t>(qo.off + d));
            wr32(e + 16, static_cast<uint32_t>(blk.size()));
        } else {
            wr32(e + 12, static_cast<uint32_t>(moved(gobjs[i].off)));
        }
    }
    const auto gruns = self_pointers(guest.data(), guest.size(), 0, gs->end);
    size_t kept = 0;
    for (size_t at : gruns) {
        if (at >= qo.off && at < qend) continue;  // the guest's own quests: replaced
        const size_t na = moved(at);
        wr32(out.data() + na + 8, static_cast<uint32_t>(na + 12));
        ++kept;
    }
    const size_t qnew = qo.off + d;
    for (size_t at : runs) {
        wr32(out.data() + qnew + at + 8, static_cast<uint32_t>(qnew + at + 12));
        wr16(out.data() + qnew + at - 3, remap[rd16(blk.data() + at - 3)]);
    }

    // Check the result as the game will read it.
    const auto rs = parse_schema(out);
    if (!rs || rs->end != new_end || rs->types.size() != gs->types.size() + missing.size()) {
        fail(why, "check failed (types)");
        return std::nullopt;
    }
    const auto robjs = parse_objects(out, *rs);
    if (robjs.size() != gobjs.size() || rd32(out.data() + new_end + 8) != out.size()) {
        fail(why, "check failed (object table)");
        return std::nullopt;
    }
    for (size_t i = 0; i < robjs.size(); ++i) {
        if (rs->types[robjs[i].cls].name != gs->types[gobjs[i].cls].name) { fail(why, "check failed (order)"); return std::nullopt; }
        if (i + 1 < robjs.size() && robjs[i].off + robjs[i].size != robjs[i + 1].off) {
            fail(why, "check failed (gaps)");
            return std::nullopt;
        }
        if (i == *gq) continue;
        if (robjs[i].size != gobjs[i].size ||
            normalized(out.data() + robjs[i].off, robjs[i].size, robjs[i].off) !=
                normalized(guest.data() + gobjs[i].off, gobjs[i].size, gobjs[i].off)) {
            fail(why, "check failed (your other data changed)");
            return std::nullopt;
        }
    }
    if (!robjs.empty() && robjs.back().off + robjs.back().size != out.size()) { fail(why, "check failed (end)"); return std::nullopt; }
    std::vector<size_t> rruns;
    const Object& rq = robjs[*gq];
    if (rq.size != blk.size() ||
        normalized(out.data() + rq.off, rq.size, rq.off, &rruns) != normalized(blk.data(), blk.size(), p->host_off) ||
        rruns != runs) {
        fail(why, "check failed (quests)");
        return std::nullopt;
    }
    for (size_t at : runs) {
        if (rs->types[rd16(out.data() + rq.off + at - 3)].name != hs.types[rd16(blk.data() + at - 3)].name) {
            fail(why, "check failed (quest types)");
            return std::nullopt;
        }
    }
    if (self_pointers(out.data(), out.size(), 0, rs->end).size() != kept + runs.size()) {
        fail(why, "check failed (pointers)");
        return std::nullopt;
    }
    return out;
}

}  // namespace cdmp::save
