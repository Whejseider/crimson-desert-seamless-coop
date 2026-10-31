#include "mod/net/save_guard.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <format>
#include <mutex>
#include <system_error>
#include <vector>

#include "common/clock.h"
#include "mod/log.h"
#include "mod/net/troc_sniffer.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace cdmp::net {
namespace {

namespace fs = std::filesystem;

// Fallbacks from the 2976 message table (docs/research/data/troc_messages_2976.csv);
// resolved by name at runtime (opcode_for), so a patch that renumbers them is fine.
constexpr uint16_t kOpSaveAutoReq = 3037;
constexpr uint16_t kOpSaveToFileCurrentPlayerReq = 2910;
constexpr uint64_t kBackupEveryMs = 10 * 60 * 1000;
constexpr size_t kKeepBackups = 10;
constexpr uint64_t kNoticeMs = 20000;

std::atomic<bool> g_enabled{true};
std::atomic<bool> g_connected{false};
std::atomic<uint64_t> g_saves_in_coop{0};
std::atomic<uint64_t> g_last_save_ms{0};
std::mutex g_mutex;
std::string g_peer;
std::string g_last_backup;  // folder name of the newest backup
uint64_t g_last_backup_ms = 0;

// Paths in the log as UTF-8 (a user name may hold characters the ANSI code page lacks).
std::string utf8(const fs::path& p) {
    const auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

// The save opcodes, by name once the sniffer has read the game's message table.
std::atomic<uint32_t> g_ops{0};  // auto | file << 16, 0 = not resolved yet
void save_ops(uint16_t& auto_op, uint16_t& file_op) {
    uint32_t v = g_ops.load();
    if (!v) {
        auto_op = opcode_for("SaveAutoReq", kOpSaveAutoReq);
        file_op = opcode_for("SaveToFileCurrentPlayerReq", kOpSaveToFileCurrentPlayerReq);
        if (opcodes_resolved()) g_ops = static_cast<uint32_t>(auto_op) | (static_cast<uint32_t>(file_op) << 16);
        return;
    }
    auto_op = static_cast<uint16_t>(v & 0xFFFF);
    file_op = static_cast<uint16_t>(v >> 16);
}

fs::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR p = nullptr;
    fs::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p)) && p) out = p;
    CoTaskMemFree(p);
    return out;
}

// Copies the save folder; returns the backup's folder name ("" on failure).
std::string backup_saves() {
    const fs::path local = known_folder(FOLDERID_LocalAppData);
    const fs::path docs = known_folder(FOLDERID_Documents);
    if (local.empty() || docs.empty()) return {};
    const fs::path src = local / L"Pearl Abyss" / L"CD" / L"save";
    std::error_code ec;
    if (!fs::exists(src, ec)) {
        log::warn("Save guard: no save folder at {}; nothing to back up", utf8(src));
        return {};
    }
    SYSTEMTIME t;
    GetLocalTime(&t);
    const std::string name = std::format("auto-{:04}-{:02}-{:02}_{:02}-{:02}-{:02}-before-coop", t.wYear, t.wMonth,
                                         t.wDay, t.wHour, t.wMinute, t.wSecond);
    const fs::path root = docs / L"CrimsonDesert-CoopPlus-SaveBackups";
    const fs::path dst = root / name;
    fs::create_directories(dst, ec);
    if (ec) {
        log::warn("Save guard: cannot create {}: {}", utf8(dst), ec.message());
        return {};
    }
    fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        log::warn("Save guard: backup to {} failed: {}", utf8(dst), ec.message());
        return {};
    }
    // Keep the newest automatic backups only; the installer's own backups are not touched.
    std::vector<fs::path> autos;
    for (const auto& e : fs::directory_iterator(root, ec)) {
        if (e.is_directory(ec) && e.path().filename().string().rfind("auto-", 0) == 0) autos.push_back(e.path());
    }
    std::sort(autos.begin(), autos.end());  // names start with the date: oldest first
    while (autos.size() > kKeepBackups) {
        fs::remove_all(autos.front(), ec);
        autos.erase(autos.begin());
    }
    log::info("Save guard: saves backed up to {}", utf8(dst));
    return name;
}

}  // namespace

void save_guard_configure(bool enabled) {
    g_enabled = enabled;
    log::info("Save guard: {}", enabled ? "on (backup when a co-op session starts, warning on saves during co-op)"
                                        : "off");
}

void save_guard_set_connected(bool connected, const std::string& peer_name) {
    const bool was = g_connected.exchange(connected);
    if (!connected || was || !g_enabled) return;
    {
        std::lock_guard lock(g_mutex);
        g_peer = peer_name;
        if (g_last_backup_ms && now_ms() - g_last_backup_ms < kBackupEveryMs) return;
    }
    // A few MB of files: fine on the mod's network thread, never the game's.
    const std::string name = backup_saves();
    std::lock_guard lock(g_mutex);
    if (!name.empty()) {
        g_last_backup = name;
        g_last_backup_ms = now_ms();
    }
}

void save_guard_observe_c2s(const uint8_t* packet, size_t size) {
    if (!g_enabled || !g_connected || size < 5) return;
    uint16_t op = 0;
    std::memcpy(&op, packet, 2);
    uint16_t auto_op = 0, file_op = 0;
    save_ops(auto_op, file_op);
    if (op != auto_op && op != file_op) return;
    const uint64_t n = ++g_saves_in_coop;
    g_last_save_ms = now_ms() ? now_ms() : 1;
    std::lock_guard lock(g_mutex);
    log::warn("Save guard: the game saved ({}) while '{}' is in the session [{} so far]; if this save misbehaves, "
              "restore Documents\\CrimsonDesert-CoopPlus-SaveBackups\\{}",
              op == auto_op ? "autosave" : "manual save", g_peer, n,
              g_last_backup.empty() ? "(no backup this session)" : g_last_backup);
}

std::string save_guard_hud_text() {
    const uint64_t at = g_last_save_ms.load();
    if (!at || now_ms() - at > kNoticeMs) return {};
    std::lock_guard lock(g_mutex);
    return g_last_backup.empty() ? "The game saved during co-op (no backup yet)"
                                 : "The game saved during co-op. Backup: " + g_last_backup;
}

}  // namespace cdmp::net
