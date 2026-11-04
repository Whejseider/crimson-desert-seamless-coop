#include "mod/net/story_sync.h"

#include <windows.h>
#include <shlobj.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <system_error>
#include <thread>

#include "common/clock.h"
#include "common/game_save.h"
#include "mod/log.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace cdmp::net {
namespace {

namespace fs = std::filesystem;
using save::Bytes;

constexpr uint8_t kAsk = 1, kPart = 2, kNo = 3;
constexpr size_t kPartBytes = 200 * 1024;           // under proto::kMaxRelMessage (256 KB)
constexpr uint32_t kMaxStory = 16u << 20;           // raw; a real one is ~1.9 MB
constexpr uint16_t kMaxParts = 64;
constexpr size_t kSendWhenPendingBelow = 32;        // reliable fragments in flight
constexpr uint64_t kAnswerTimeoutMs = 90000;
constexpr uint64_t kAskEveryMs = 20000;             // a host answers one ask per 20 s
constexpr uint32_t kPendingMagic = 0x50534443;      // "CDSP"

enum class Guest { Idle, Asking, Receiving, Checking, Ready, Failed };

std::mutex g_m;
bool g_share = true;
fs::path g_dev_slot;
bool g_connected = false, g_host = false;
std::string g_peer;
uint64_t g_now = 0;
std::vector<Bytes> g_out;   // messages waiting to be sent
std::string g_toast;
// Guest side.
Guest g_state = Guest::Idle;
uint32_t g_id = 0;
uint64_t g_asked_ms = 0;
std::vector<Bytes> g_parts;
uint16_t g_parts_have = 0;
uint32_t g_story_size = 0;
std::string g_status;
// Host side.
std::atomic<bool> g_building{false};
uint64_t g_last_ask_ms = 0;
size_t g_sent_parts = 0, g_total_parts = 0;
std::string g_host_status;

std::string utf8(const fs::path& p) {
    const auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

fs::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p)) && p) out = p;
    CoTaskMemFree(p);
    return out;
}

fs::path backups_root() { return known_folder(FOLDERID_Documents) / L"CrimsonDesert-CoopPlus-SaveBackups"; }
fs::path pending_path() { return backups_root() / L"story-sync" / L"pending-story.bin"; }

std::optional<Bytes> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return std::nullopt;
    return Bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool write_file(const fs::path& p, const Bytes& b) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    const fs::path tmp = p.wstring() + L".cdmp-new";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return false;
        f.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
        if (!f) return false;
    }
    return MoveFileExW(tmp.c_str(), p.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

// The slot folder "Continue" loads: the one with the newest save.save ([dev] story_slot wins).
fs::path newest_slot() {
    if (!g_dev_slot.empty()) return g_dev_slot;
    const fs::path root = known_folder(FOLDERID_LocalAppData) / L"Pearl Abyss" / L"CD" / L"save";
    std::error_code ec;
    fs::path best;
    fs::file_time_type best_t{};
    for (const auto& user : fs::directory_iterator(root, ec)) {
        if (!user.is_directory(ec)) continue;
        for (const auto& slot : fs::directory_iterator(user.path(), ec)) {
            const fs::path s = slot.path() / L"save.save";
            if (!fs::is_regular_file(s, ec)) continue;
            const auto t = fs::last_write_time(s, ec);
            if (!ec && (best.empty() || t > best_t)) {
                best = slot.path();
                best_t = t;
            }
        }
    }
    return best;
}

std::optional<save::Decoded> load_save(const fs::path& slot, std::string& why) {
    if (slot.empty()) {
        why = "no save found";
        return std::nullopt;
    }
    const auto file = read_file(slot / L"save.save");
    if (!file) {
        why = "could not read " + utf8(slot / L"save.save");
        return std::nullopt;
    }
    return save::decode(*file, &why);
}

void put16(Bytes& b, uint16_t v) { b.insert(b.end(), {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8)}); }
void put32(Bytes& b, uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
uint32_t get32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}
uint16_t get16(const uint8_t* p) {
    uint16_t v;
    std::memcpy(&v, p, 2);
    return v;
}
Bytes header(uint8_t op, uint32_t id) {
    Bytes b{op};
    put32(b, id);
    return b;
}

std::string counts_text(const std::optional<save::StoryCount>& c) {
    return c ? std::format("{} missions done", c->done) : std::string("?");
}

// --- host --------------------------------------------------------------------------------------------------------
void host_answer(uint32_t id) {
    std::thread([id] {
        std::string why;
        std::optional<Bytes> story;
        const fs::path slot = newest_slot();
        if (const auto d = load_save(slot, why)) story = save::story_extract(d->blob, &why);
        std::lock_guard lock(g_m);
        g_building = false;
        if (!story) {
            log::warn("Story sync: could not read our story for the guest: {}", why);
            Bytes no = header(kNo, id);
            const std::string text = "the host's save could not be read (" + why + ")";
            no.insert(no.end(), text.begin(), text.end());
            g_out.push_back(std::move(no));
            return;
        }
        const Bytes comp = save::lz4_compress(story->data(), story->size());
        const size_t count = (comp.size() + kPartBytes - 1) / kPartBytes;
        for (size_t i = 0; i < count; ++i) {
            Bytes part = header(kPart, id);
            put16(part, static_cast<uint16_t>(i));
            put16(part, static_cast<uint16_t>(count));
            put32(part, static_cast<uint32_t>(story->size()));
            const size_t from = i * kPartBytes, to = std::min(comp.size(), from + kPartBytes);
            part.insert(part.end(), comp.begin() + from, comp.begin() + to);
            g_out.push_back(std::move(part));
        }
        g_sent_parts = 0;
        g_total_parts = count;
        const auto c = save::story_count(*story);
        log::info("Story sync: sending our story to {} ({}, {} KB in {} parts, from {})", g_peer, counts_text(c),
                  comp.size() / 1024, count, utf8(slot));
        g_host_status = std::format("Sending your story to {} ({})", g_peer, counts_text(c));
    }).detach();
}

// --- guest -------------------------------------------------------------------------------------------------------
void guest_fail(const std::string& why) {
    g_state = Guest::Failed;
    g_status = "Story copy: not done. " + why;
    g_toast = "Co-op+\n" + g_status;
    g_parts.clear();
    log::warn("Story sync: {}", why);
}

// All parts in: unpack, dry-run the copy on our newest save, keep it for the next start.
void guest_check(Bytes comp, uint32_t size, std::string peer) {
    std::thread([comp = std::move(comp), size, peer] {
        std::string why, fit;
        std::optional<save::StoryCount> theirs, ours;
        const auto story = save::lz4_decompress(comp.data(), comp.size(), size);
        bool ok = false;
        if (!story) {
            why = "the story data arrived damaged; try again";
        } else if (!(theirs = save::story_count(*story))) {
            why = "the story data is not readable; try again";
        } else if (const auto d = load_save(newest_slot(), fit); !d) {
            why = "your own save could not be read (" + fit + ")";
        } else if (!save::story_apply(d->blob, *story, &fit)) {
            why = fit;
        } else {
            ours = save::save_story_count(d->blob);
            Bytes file;
            put32(file, kPendingMagic);
            put32(file, 1);
            put16(file, static_cast<uint16_t>(std::min<size_t>(peer.size(), 200)));
            file.insert(file.end(), peer.begin(), peer.begin() + std::min<size_t>(peer.size(), 200));
            file.insert(file.end(), story->begin(), story->end());
            ok = write_file(pending_path(), file);
            if (!ok) why = "could not write " + utf8(pending_path());
        }
        std::lock_guard lock(g_m);
        if (g_state != Guest::Checking) {  // cancelled meanwhile: nothing may be left for the next start
            std::error_code ec;
            if (ok) fs::remove(pending_path(), ec);
            return;
        }
        if (!ok) {
            guest_fail(why);
            return;
        }
        g_state = Guest::Ready;
        std::string behind;
        if (ours && theirs && theirs->done < ours->done) {
            behind = std::format(" Careful: {} is behind you ({} vs your {}): your save would go back to their story.",
                                 peer, theirs->done, ours->done);
        }
        g_status = std::format(
            "Story from {} received ({}; yours: {}). It goes into your save at your next game start: quit to the "
            "desktop, start the game again and press Continue. Your save is backed up first. Set this to \"no\" to "
            "cancel.{}",
            peer, counts_text(theirs), counts_text(ours), behind);
        g_toast = "Co-op+\nStory from " + peer + " received: it goes into your save when you next start the game.";
        log::info("Story sync: story from {} kept for the next start ({}; ours {})", peer, counts_text(theirs),
                  counts_text(ours));
    }).detach();
}

void on_part(uint32_t id, const uint8_t* p, size_t n) {
    if (g_host || id != g_id || (g_state != Guest::Asking && g_state != Guest::Receiving) || n < 8) return;
    const uint16_t index = get16(p), count = get16(p + 2);
    const uint32_t size = get32(p + 4);
    if (count == 0 || count > kMaxParts || index >= count || size == 0 || size > kMaxStory) {
        guest_fail("the host sent story data this version does not understand");
        return;
    }
    if (g_state == Guest::Asking) {
        g_state = Guest::Receiving;
        g_parts.assign(count, {});
        g_parts_have = 0;
        g_story_size = size;
    }
    if (count != g_parts.size() || size != g_story_size || !g_parts[index].empty() || n == 8) return;
    g_parts[index].assign(p + 8, p + n);
    ++g_parts_have;
    g_status = std::format("Story copy: receiving {}'s story ({}/{})", g_peer, g_parts_have, count);
    if (g_parts_have < count) return;
    Bytes comp;
    for (auto& part : g_parts) comp.insert(comp.end(), part.begin(), part.end());
    g_parts.clear();
    g_state = Guest::Checking;
    g_status = "Story copy: checking it fits your save...";
    guest_check(std::move(comp), size, g_peer);
}

}  // namespace

void story_configure(bool share, const std::wstring& dev_slot) {
    std::lock_guard lock(g_m);
    g_share = share;
    g_dev_slot = dev_slot;
    log::info("Story sync: hosts share their story {}{}", share ? "yes" : "no ([coop] share_story=0)",
              dev_slot.empty() ? "" : " | [dev] story_slot " + utf8(dev_slot));
}

void story_set_share(bool share) {
    std::lock_guard lock(g_m);
    g_share = share;
}

void story_startup() {
    const fs::path pending = pending_path();
    std::error_code ec;
    if (!fs::exists(pending, ec)) return;
    const auto file = read_file(pending);
    std::string peer, why;
    std::optional<Bytes> story;
    if (file && file->size() > 10 && get32(file->data()) == kPendingMagic && get32(file->data() + 4) == 1) {
        const uint16_t n = get16(file->data() + 8);
        if (file->size() > 10u + n) {
            peer.assign(reinterpret_cast<const char*>(file->data() + 10), n);
            story = Bytes(file->begin() + 10 + n, file->end());
        }
    }
    const fs::path slot = newest_slot();
    bool done = false;
    fs::path backup;
    std::optional<save::StoryCount> count;
    if (!story) {
        why = "the kept story file is damaged";
    } else if (const auto d = load_save(slot, why)) {
        if (const auto out = save::story_apply(d->blob, *story, &why)) {
            std::random_device rd;
            uint8_t nonce[16];
            for (auto& b : nonce) b = static_cast<uint8_t>(rd());
            const Bytes enc = save::encode(*out, d->header, nonce);
            const auto back = save::decode(enc, &why);
            if (!back || back->blob != *out) {
                why = "the new save did not read back the same";
            } else {
                SYSTEMTIME t;
                GetLocalTime(&t);
                backup = backups_root() /
                         std::format(L"before-story-sync-{:04}-{:02}-{:02}_{:02}-{:02}-{:02}", t.wYear, t.wMonth,
                                     t.wDay, t.wHour, t.wMinute, t.wSecond) /
                         slot.parent_path().filename() / slot.filename();
                fs::create_directories(backup, ec);
                if (!ec) fs::copy(slot, backup, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
                if (ec) {
                    why = "your save could not be backed up first (" + ec.message() + ")";
                } else if (!write_file(slot / L"save.save", enc)) {
                    why = "the save could not be written";
                } else {
                    done = true;
                    count = save::save_story_count(*out);
                }
            }
        }
    }
    fs::remove(pending, ec);
    std::lock_guard lock(g_m);
    if (done) {
        log::info("Story sync: {}'s story is now in {} ({}); the old save is in {}", peer, utf8(slot),
                  counts_text(count), utf8(backup));
        g_toast = std::format("Co-op+\nYour save now follows {}'s story ({}). Your old save is in Documents\\"
                              "CrimsonDesert-CoopPlus-SaveBackups\\{}.",
                              peer, counts_text(count), utf8(backup.parent_path().parent_path().filename()));
    } else {
        log::warn("Story sync: the story from {} was not put into {}: {}", peer, utf8(slot), why);
        g_toast = "Co-op+\nThe story copy was not done: " + why + ". Your save is unchanged.";
    }
}

void story_set_link(bool connected, bool host, const std::string& peer_name, uint64_t now_ms) {
    std::lock_guard lock(g_m);
    g_now = now_ms;
    if (connected && !peer_name.empty()) g_peer = peer_name;
    if (g_connected && !connected && (g_state == Guest::Asking || g_state == Guest::Receiving)) {
        guest_fail("the connection to your friend ended before the story arrived; set this to \"yes\" again once "
                   "you are back together");
    }
    if (!connected) g_out.clear();
    g_connected = connected;
    g_host = host;
    if (g_state == Guest::Asking && now_ms - g_asked_ms > kAnswerTimeoutMs) {
        guest_fail("no answer from your friend (do you both have this Co-op+ version?)");
    }
}

void story_on_message(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 5) return;
    const uint8_t op = bytes[0];
    const uint32_t id = get32(bytes.data() + 1);
    std::lock_guard lock(g_m);
    if (op == kAsk && g_host) {
        if (!g_share) {
            Bytes no = header(kNo, id);
            const std::string text = "your friend has story sharing off ([coop] share_story=0 or the F8 settings)";
            no.insert(no.end(), text.begin(), text.end());
            g_out.push_back(std::move(no));
            log::info("Story sync: {} asked for our story; sharing is off", g_peer);
        } else if (g_building || (g_last_ask_ms && g_now - g_last_ask_ms < kAskEveryMs)) {
            log::info("Story sync: {} asked again; ignored (one ask per 20 s)", g_peer);
        } else {
            g_last_ask_ms = g_now ? g_now : 1;
            g_building = true;
            log::info("Story sync: {} asked for our story", g_peer);
            host_answer(id);
        }
    } else if (op == kPart) {
        on_part(id, bytes.data() + 5, bytes.size() - 5);
    } else if (op == kNo && !g_host && id == g_id && (g_state == Guest::Asking || g_state == Guest::Receiving)) {
        guest_fail(std::string(bytes.begin() + 5, bytes.begin() + std::min<size_t>(bytes.size(), 5 + 300)));
    }
}

std::vector<std::vector<uint8_t>> story_take_outgoing(size_t reliable_pending) {
    std::lock_guard lock(g_m);
    std::vector<std::vector<uint8_t>> out;
    if (g_out.empty() || reliable_pending >= kSendWhenPendingBelow) return out;
    out.push_back(std::move(g_out.front()));
    g_out.erase(g_out.begin());
    if (out.back()[0] == kPart && g_total_parts) {
        if (++g_sent_parts == g_total_parts) {
            g_host_status = std::format("Your story went to {}: they get it at their next game start", g_peer);
            log::info("Story sync: all {} parts handed to the reliable channel", g_total_parts);
        }
    }
    return out;
}

void story_set_wanted(bool yes) {
    std::lock_guard lock(g_m);
    std::error_code ec;
    if (!yes) {
        if (g_state != Guest::Idle && g_state != Guest::Failed) {
            fs::remove(pending_path(), ec);
            g_status = "Story copy cancelled: your save stays as it is.";
            log::info("Story sync: cancelled by the player");
        }
        g_state = Guest::Idle;
        g_parts.clear();
        return;
    }
    if (g_state == Guest::Asking || g_state == Guest::Receiving || g_state == Guest::Checking ||
        g_state == Guest::Ready) {
        return;
    }
    if (!g_connected) {
        g_status = "Story copy: join your friend's game first, then pick \"yes\".";
        return;
    }
    if (g_host) {
        g_status = "Story copy: you are the host; your guest picks this to take YOUR story.";
        return;
    }
    std::random_device rd;
    g_id = rd() | 1;
    g_state = Guest::Asking;
    g_asked_ms = g_now;
    g_out.push_back(header(kAsk, g_id));
    g_status = std::format("Story copy: asking {} for their story...", g_peer);
    log::info("Story sync: asked {} for their story", g_peer);
}

int story_wanted_index() {
    std::lock_guard lock(g_m);
    return g_state == Guest::Idle || g_state == Guest::Failed ? 0 : 1;
}

std::string story_hud_text() {
    std::lock_guard lock(g_m);
    if (g_host) return g_host_status;
    return g_status;
}

std::string story_take_toast() {
    std::lock_guard lock(g_m);
    std::string t;
    t.swap(g_toast);
    return t;
}

}  // namespace cdmp::net
