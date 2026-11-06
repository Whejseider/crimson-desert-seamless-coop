#pragma once
// Co-op+ on the native base (docs/research/native-coopplus-layer.md). Built with -DCDMP_NATIVE_LAYER=ON this DLL is
// coopplus.asi, loaded next to the native base's cdmp.asi (ByJunter's NativeServer, which gives both players one
// real shared world). In that build every system that only SIMULATED a shared world is off (the other player's
// puppet, enemy copies and the enemy relay, the enemy zone, horse and equipment copies, damage mirroring, co-op
// worlds, live quest copies, story sync, the slow-motion block, the time sync, the stage cinema) and so is the
// packet sniffer (the native base hooks the same client packet functions). Its own files: coopplus.ini / .log.
// The normal build (the released Co-op+ on the old base) is unchanged.

#include <windows.h>

#include <string>

#include "mod/config.h"

namespace cdmp {

#ifdef CDMP_NATIVE_LAYER
inline constexpr bool kNativeLayer = true;
inline constexpr const wchar_t* kFileStem = L"coopplus";
#else
inline constexpr bool kNativeLayer = false;
inline constexpr const wchar_t* kFileStem = L"cdmp";
#endif

// dir + "cdmp" (or "coopplus") + suffix, e.g. own_file(dir, L".ini"). The native build follows the native base's
// two-copy convention: with CDMP_ROLE=host|guest in the environment its files are coopplus.<role>.ini / .log.
inline std::wstring own_file(const std::wstring& dir, const wchar_t* suffix) {
    std::wstring stem = kFileStem;
    if (kNativeLayer) {
        wchar_t role[16] = {};
        const DWORD n = GetEnvironmentVariableW(L"CDMP_ROLE", role, 16);
        if (n > 0 && n < 16 && (std::wstring(role) == L"host" || std::wstring(role) == L"guest")) stem += L"." + std::wstring(role);
    }
    return dir + stem + suffix;
}

// The native build's overrides, applied right after the ini is read (whatever the ini says). Returns what changed,
// for the log.
inline std::string apply_native_layer(Config& c) {
    if (!kNativeLayer) return {};
    std::string changed;
    auto off = [&](bool& v, const char* name) {
        if (v) changed += std::string(changed.empty() ? "" : ", ") + name;
        v = false;
    };
    off(c.sniffer_enabled, "sniffer");
    off(c.dev_enemy_copies, "enemy copies");
    off(c.coop_enemy_auto, "enemy relay");
    off(c.dev_enemy_relay, "dev enemy relay");
    off(c.coop_horse, "horse copy");
    off(c.coop_equip, "equipment relay");
    off(c.coop_damage, "damage mirror");
    off(c.coop_guest_hits, "guest hits");
    off(c.coop_coop_worlds, "co-op worlds");
    off(c.coop_live_quests, "live quests");
    off(c.coop_share_story, "story sync");
    off(c.coop_block_slowmo, "slow-motion block");
    off(c.coop_stage_cinema, "stage cinema");
    off(c.coop_save_guard, "save guard");
    if (!c.dev_no_copy) changed += std::string(changed.empty() ? "" : ", ") + "the other player's puppet";
    c.dev_no_copy = true;
    if (c.coop_suppress) changed += std::string(changed.empty() ? "" : ", ") + "enemy zone";
    c.coop_suppress = 0;
    if (c.coop_time_sync) changed += std::string(changed.empty() ? "" : ", ") + "time sync";
    c.coop_time_sync = 0;
    return changed;
}

}  // namespace cdmp
