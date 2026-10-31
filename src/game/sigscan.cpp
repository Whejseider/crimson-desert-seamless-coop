#include "game/sigscan.hpp"

#include <cstring>

namespace cdcoop::game {

namespace {
int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
}  // namespace

std::optional<Pattern> parse_pattern(std::string_view text) {
    Pattern p;
    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] == ' ') { ++i; continue; }
        std::size_t j = i;
        while (j < text.size() && text[j] != ' ') ++j;
        const std::string_view tok = text.substr(i, j - i);
        if (tok == "?" || tok == "??") {
            p.bytes.push_back(0); p.mask.push_back(false);
        } else if (tok.size() == 2 && hexval(tok[0]) >= 0 && hexval(tok[1]) >= 0) {
            p.bytes.push_back(static_cast<std::uint8_t>(hexval(tok[0]) * 16 + hexval(tok[1]))); p.mask.push_back(true);
        } else {
            return std::nullopt;
        }
        i = j;
    }
    if (p.bytes.empty() || !p.mask.front()) return std::nullopt;  // must start with a concrete byte
    return p;
}

std::size_t find_first(std::span<const std::uint8_t> h, const Pattern& p, std::size_t start) {
    const std::size_t n = p.bytes.size();
    if (n == 0 || h.size() < n) return npos;
    const std::uint8_t first = p.bytes[0];
    for (std::size_t i = start; i + n <= h.size(); ++i) {
        const void* hit = std::memchr(h.data() + i, first, h.size() - n + 1 - i);
        if (!hit) return npos;
        i = static_cast<std::size_t>(static_cast<const std::uint8_t*>(hit) - h.data());
        bool ok = true;
        for (std::size_t k = 1; k < n; ++k)
            if (p.mask[k] && h[i + k] != p.bytes[k]) { ok = false; break; }
        if (ok) return i;
    }
    return npos;
}

std::vector<std::size_t> find_all(std::span<const std::uint8_t> h, const Pattern& p, std::size_t limit) {
    std::vector<std::size_t> out;
    std::size_t at = 0;
    while (out.size() < limit) {
        at = find_first(h, p, at);
        if (at == npos) break;
        out.push_back(at++);
    }
    return out;
}

std::optional<std::size_t> resolve_rip_relative(std::span<const std::uint8_t> image, std::size_t instr_offset,
                                                std::size_t disp_offset, std::size_t instr_len) {
    if (instr_offset + disp_offset + 4 > image.size()) return std::nullopt;
    std::int32_t disp;
    std::memcpy(&disp, image.data() + instr_offset + disp_offset, 4);
    const auto target = static_cast<std::int64_t>(instr_offset + instr_len) + disp;
    if (target < 0) return std::nullopt;
    return static_cast<std::size_t>(target);
}

}  // namespace cdcoop::game
