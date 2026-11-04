// Co-op+ story sync, on the command line (tests and support; the mod does the same in game, net/story_sync.cpp).
//   cdmp_savetool count <save.save>                        missions known / completed
//   cdmp_savetool extract <host save.save> <story.bin>     what a host sends
//   cdmp_savetool apply <guest save.save> <story.bin> <out save.save>
//   cdmp_savetool roundtrip <save.save> <out save.save>    decode + encode (checks the codec)
//   cdmp_savetool blob <save.save> <out.bin>                the decoded blob (compare with tools/save/cdsave.py)
// Never writes over an input.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "common/game_save.h"

namespace fs = std::filesystem;
using cdmp::save::Bytes;

namespace {

Bytes read_file(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return Bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool write_new(const char* path, const Bytes& b, const char* const* inputs, int n) {
    for (int i = 0; i < n; ++i) {
        std::error_code ec;
        if (fs::equivalent(path, inputs[i], ec)) {
            std::fprintf(stderr, "the output must be a new file\n");
            return false;
        }
    }
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
    return static_cast<bool>(f);
}

int die(const std::string& why) {
    std::fprintf(stderr, "%s\n", why.c_str());
    return 1;
}

std::optional<cdmp::save::Decoded> load(const char* path) {
    std::string why;
    auto d = cdmp::save::decode(read_file(path), &why);
    if (!d) std::fprintf(stderr, "%s: %s\n", path, why.c_str());
    return d;
}

Bytes encode_new(const cdmp::save::Decoded& d, const Bytes& blob) {
    std::random_device rd;
    uint8_t nonce[16];
    for (auto& b : nonce) b = static_cast<uint8_t>(rd());
    return cdmp::save::encode(blob, d.header, nonce);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string cmd = argc > 1 ? argv[1] : "";
    if (cmd == "count" && argc == 3) {
        const auto d = load(argv[2]);
        if (!d) return 1;
        const auto c = cdmp::save::save_story_count(d->blob);
        if (!c) return die("no quest data");
        std::printf("%u missions known, %u done\n", c->missions, c->done);
        return 0;
    }
    if (cmd == "extract" && argc == 4) {
        const auto d = load(argv[2]);
        if (!d) return 1;
        std::string why;
        const auto p = cdmp::save::story_extract(d->blob, &why);
        if (!p) return die(why);
        if (!write_new(argv[3], *p, argv + 2, 1)) return 1;
        const auto c = cdmp::save::story_count(*p);
        std::printf("story: %zu bytes, %u missions, %u done\n", p->size(), c ? c->missions : 0, c ? c->done : 0);
        return 0;
    }
    if (cmd == "apply" && argc == 5) {
        const auto d = load(argv[2]);
        if (!d) return 1;
        const Bytes story = read_file(argv[3]);
        std::string why;
        const auto out = cdmp::save::story_apply(d->blob, story, &why);
        if (!out) return die("not synced: " + why);
        const Bytes file = encode_new(*d, *out);
        const auto back = cdmp::save::decode(file, &why);
        if (!back || back->blob != *out) return die("encode check failed");
        if (!write_new(argv[4], file, argv + 2, 2)) return 1;
        const auto c = cdmp::save::save_story_count(*out);
        std::printf("synced: %zu -> %zu bytes, now %u missions, %u done\n", d->blob.size(), out->size(),
                    c ? c->missions : 0, c ? c->done : 0);
        return 0;
    }
    if (cmd == "roundtrip" && argc == 4) {
        const auto d = load(argv[2]);
        if (!d) return 1;
        const Bytes file = encode_new(*d, d->blob);
        std::string why;
        const auto back = cdmp::save::decode(file, &why);
        if (!back || back->blob != d->blob) return die("round trip failed");
        return write_new(argv[3], file, argv + 2, 1) ? 0 : 1;
    }
    if (cmd == "blob" && argc == 4) {
        const auto d = load(argv[2]);
        if (!d) return 1;
        return write_new(argv[3], d->blob, argv + 2, 1) ? 0 : 1;
    }
    std::fprintf(stderr, "usage: cdmp_savetool count|extract|apply|roundtrip|blob ... (see the source)\n");
    return 2;
}
