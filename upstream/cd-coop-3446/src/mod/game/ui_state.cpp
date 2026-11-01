#include "mod/game/ui_state.h"

#include <windows.h>

#include <cstdint>
#include <cstring>

#include "mod/game/offsets.h"
#include "mod/game/safe_memory.h"
#include "mod/log.h"

namespace cdmp::game {
namespace {
namespace off = build_3479;

// 1 = checked and usable, -1 = refused, 0 = not checked yet.
int g_state = 0;
uintptr_t g_flag = 0;

bool check() {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    uint8_t site[7] = {};
    int32_t rel = 0;
    if (base == off::kImageBase && safe_read(base + off::kUiModePush, site, sizeof site)) {
        std::memcpy(&rel, site + 3, 4);
    }
    const bool ok = site[0] == 0x0F && site[1] == 0xB6 && site[2] == 0x05 &&
                    off::kUiModePush + 7 + rel == off::kGameplayInput;
    if (ok) {
        g_flag = base + off::kGameplayInput;
    } else {
        log::warn("UI state: menu flag not recognised on this build; name tags may show over menus");
    }
    return ok;
}

}  // namespace

bool ui_menu_open() {
    if (g_state == 0) g_state = check() ? 1 : -1;
    if (g_state < 0) return false;
    uint8_t v = 1;
    return safe_read(g_flag, &v, 1) && v == 0;
}

}  // namespace cdmp::game
