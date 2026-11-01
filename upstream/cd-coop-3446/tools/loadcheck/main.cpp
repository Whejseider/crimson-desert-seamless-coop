// Co-op+ dev tool: runs the enemy-zone predicate (common/enemy_load) on every person-type
// ActorLoadDataAck of a packet capture (cdmp_packets_*.bin) and prints the verdict and why.
//   cdmp_loadcheck <capture.bin> [opcode, default 2005 = ActorLoadDataAck on 3460/3479]
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

#include "common/enemy_load.h"

using namespace cdmp;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: cdmp_loadcheck <capture.bin> [opcode]\n");
        return 2;
    }
    const uint16_t op_load = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : 2005;
    std::ifstream f(argv[1], std::ios::binary);
    const std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), {});
    if (d.size() < 12 || std::memcmp(d.data(), "CDPK", 4) != 0) {
        std::fprintf(stderr, "not a capture\n");
        return 2;
    }
    size_t o = 12, n = 0;
    while (o + 24 <= d.size()) {
        uint16_t size = 0, copied = 0;
        std::memcpy(&size, d.data() + o + 10, 2);
        std::memcpy(&copied, d.data() + o + 12, 2);
        o += 24;
        if (o + copied > d.size()) break;
        const uint8_t* p = d.data() + o;
        o += copied;
        if (copied < 5 + 0x40 || (p[0] | p[1] << 8) != op_load) continue;
        const std::span<const uint8_t> load(p + 5, copied - 5);
        const auto head = enemy_load::read_head(load);
        if (!head || (head->type != 3 && head->type != 4)) continue;
        const auto c = enemy_load::zone_check(load, {});
        const auto o2 = enemy_load::origin(load);
        const auto t = enemy_load::last_transform(load);
        std::printf("%08X type %u faction %8u variant %8u %-28s origin %c %08X | %s%s%s\n", head->key, head->type,
                    head->faction, head->variant, enemy_load::name(load).c_str(), static_cast<char>(o2.kind), o2.id,
                    c.verdict == enemy_load::Verdict::Yes            ? "YES"
                    : c.verdict == enemy_load::Verdict::FollowParent ? "follows parent"
                                                                     : "no: ",
                    c.verdict == enemy_load::Verdict::No ? c.why : "", t ? "" : " [no transform]");
        ++n;
    }
    std::printf("%zu person loads\n", n);
    return 0;
}
