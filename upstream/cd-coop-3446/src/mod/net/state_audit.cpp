#include "mod/net/state_audit.h"

#include <format>

#include "mod/net/coop_duel.h"
#include "mod/net/coop_hunt.h"
#include "mod/net/enemy_relay.h"
#include "mod/net/native_suppress.h"
#include "mod/net/party_scaling.h"
#include "mod/net/puppet_visibility.h"
#include "mod/net/remote_puppet.h"
#include "mod/net/stage_cinema.h"
#include "mod/net/weather_sync.h"

namespace cdmp::net {

StateAudit state_audit() {
    StateAudit a;
    const auto item = [&](const char* name, long long value, bool ok) {
        a.line += std::format("{}{} {}", a.line.empty() ? "" : " | ", name, value);
        if (!ok) {
            a.clean = false;
            a.leaks += std::format("{}{} {}", a.leaks.empty() ? "" : ", ", name, value);
        }
    };
    item("partner", puppet_spawned(), !puppet_spawned());
    item("seat", puppet_seated(), !puppet_seated());
    item("hidden NPC", puppet_hidden_key(), puppet_hidden_key() == 0);
    const size_t here = enemy_copies_present(), sent = enemy_copies_sent();
    item("enemy copies here", static_cast<long long>(here), here == 0);
    item("enemy copies sent", static_cast<long long>(sent), sent == 0);
    const size_t held = suppress_held_count();
    item("our enemies hidden", static_cast<long long>(held), held == 0);
    const float mult = scaling_multiplier();
    a.line += std::format(" | scaling x{:.2f}", mult);
    if (mult != 1.0f) {
        a.clean = false;
        a.leaks += std::format("{}scaling x{:.2f}", a.leaks.empty() ? "" : ", ", mult);
    }
    const size_t weather = weather_host_stages_running();
    item("host weather stages", static_cast<long long>(weather), weather == 0);
    item("cutscene copy", cinema_copy_active(), !cinema_copy_active());
    const bool duel = duel_hud().active, hunt = !hunt_hud().title.empty();
    item("duel", duel, !duel);
    item("hunt", hunt, !hunt);
    return a;
}

}  // namespace cdmp::net
