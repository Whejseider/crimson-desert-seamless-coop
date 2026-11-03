#include "mod/render/hud.h"

#include <windows.h>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <mutex>

#include "common/math3d.h"
#include "common/nameplate.h"
#include "mod/game/camera_reader.h"
#include "mod/game/ui_state.h"
#include "mod/log.h"
#include "mod/render/dx12_hook.h"
#include "mod/input/actions.h"
#include "mod/render/game_ui.h"

namespace cdmp::render {
namespace {

// Marker above the character's feet (m). Co-op+: 2.1 floated well clear of the head in the 3460
// echo tests (the name sits another ~28 px above the dot); the player character is ~1.8 m tall.
constexpr float kHeadHeight = 1.95f;
constexpr ImU32 kAccent = IM_COL32(230, 70, 60, 255);
constexpr ImU32 kShadow = IM_COL32(0, 0, 0, 200);

std::mutex g_mutex;
HudData g_data;
HudOptions g_options;

// Co-op+: the same colour at `alpha` of its own opacity (nameplate fade).
ImU32 faded(ImU32 color, float alpha) {
    const auto a = static_cast<ImU32>(((color >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(alpha, 0.0f, 1.0f));
    return (color & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

void draw_remote_marker(ImDrawList* dl, const game::ScreenPoint& p, const HudData& d,
                        float distance) {
    // Co-op+ (brief 6.2): fades and shrinks a little with distance (common/nameplate.h); the game's tag look.
    const float alpha = nameplate::alpha(distance);
    const float scale = nameplate::text_scale(distance);
    const std::string name = d.peer_name.empty() ? (g_options.english ? "Player 2" : "Jugador 2") : d.peer_name;
    const float frac = d.remote->health_max > 0 ? d.remote->health / d.remote->health_max : -1.0f;
    const std::string sub = d.partner_in_menu ? std::format("{:.0f} m  -  in a menu", distance) : std::format("{:.0f} m", distance);
    ui::nameplate(dl, p.x, p.y, name, frac, sub, alpha, scale);
}

}  // namespace

void hud_configure(const HudOptions& options) {
    std::lock_guard lock(g_mutex);
    g_options = options;
}

void hud_update(HudData data) {
    std::lock_guard lock(g_mutex);
    g_data = std::move(data);
}

// The status panel (top left). F8 shows / hides it; the nameplate has its own toggle.
void draw_panel(const HudData& d, const HudOptions& opt, float remote_distance, const std::string& camera_failure) {
    // ---- status panel
    // Co-op+: the game's panel look (game_ui.h): dark warm backing, gold serif title, a thin light rule.
    ImGui::SetNextWindowPos(ImVec2(24, 24));
    ImGui::SetNextWindowBgAlpha(0.72f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));
    ImGui::Begin("cdmp", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::PopStyleVar(2);
    if (ui::serif_bold()) ImGui::PushFont(ui::serif_bold(), 24.0f);
    ImGui::TextColored(ImVec4(0.91f, 0.78f, 0.51f, 1), "Seamless Co-op");
    if (ui::serif_bold()) ImGui::PopFont();
    // GPL credit, always visible with the panel: the co-op engine is theirs.
    ImGui::TextDisabled("Co-op+ alpha, built on cdmp %s by ByJunter, port by nouzert1", CDMP_VERSION);
    ImGui::Separator();
    if (!opt.build_problem.empty()) {
        // Co-op+: an unsupported game build. Nothing else on the panel matters.
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextColored(ImVec4(1, 0.35f, 0.3f, 1), "%s", opt.build_problem.c_str());
        ImGui::PopTextWrapPos();
        ImGui::End();
        return;
    }
    const bool en = opt.english;
    if (d.connected) {
        ImGui::Text(en ? "Network: connected to '%s' (%.1f ms)" : "Red: conectado a '%s' (%.1f ms)",
                    d.peer_name.c_str(), d.rtt_ms);
    } else {
        ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1),
                           en ? "Network: looking for the other player..." : "Red: buscando al otro jugador...");
    }
    // (The guest's boss bar: drawn outside the panel, game style; see hud_draw.)
    if (!d.peer_problem.empty()) {
        // In English from the session; the essential part is the instruction.
        ImGui::TextColored(ImVec4(1, 0.35f, 0.3f, 1), en ? "Different mod versions: both of you update"
                                                          : "Versiones distintas del mod: actualizad los dos");
        ImGui::TextColored(ImVec4(1, 0.35f, 0.3f, 1), "%s", d.peer_problem.c_str());
    }
    if (d.local) {
        ImGui::Text(en ? "You: HP %.0f/%.0f  Stamina %.0f/%.0f" : "Tú: HP %.0f/%.0f  Stamina %.0f/%.0f",
                    d.local->health, d.local->health_max, d.local->stamina, d.local->stamina_max);
    } else {
        ImGui::TextDisabled(en ? "You: %s" : "Tú: %s", d.local_status.c_str());
    }
    if (d.remote && d.local) {
        ImGui::Text(en ? "Other player: HP %.0f/%.0f  %.0f m away" : "Otro jugador: HP %.0f/%.0f  a %.0f m",
                    d.remote->health, d.remote->health_max, remote_distance);
    }
    if (!d.enemy_zone.empty()) ImGui::TextColored(ImVec4(0.75f, 0.85f, 1, 1), "%s", d.enemy_zone.c_str());
    if (!d.scaling.empty()) ImGui::TextColored(ImVec4(0.75f, 0.85f, 1, 1), "%s", d.scaling.c_str());
    if (!d.save_notice.empty()) ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1), "%s", d.save_notice.c_str());
    if (!camera_failure.empty()) ImGui::TextDisabled(en ? "Camera: %s" : "Cámara: %s", camera_failure.c_str());

    // The room code, ALWAYS shown -- with the code when we have one, and an explicit
    // "no code" plus the reason when we do not.
    //
    // It used to be drawn only when a code existed, which made two very different
    // situations look identical from the player's side: "the mod has not managed to
    // open a room yet" and "the mod is not even trying" both rendered as nothing at
    // all. That cost several rounds of testing whose only visible symptom was an
    // absent line. A line that is always present, and says which case it is, turns
    // the panel into the diagnostic it should have been from the start.
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1, 1), en ? "Room code:" : "Codigo de sala:");
    ImGui::SameLine();
    if (!d.room_code.empty()) {
        // The host reads this out; the guest sees what they typed.
        if (d.room_hosting) {
            ImGui::TextColored(ImVec4(0.4f, 1, 0.5f, 1), "%s   %s", d.room_code.c_str(),
                               en ? "(tell your friend)" : "(diselo a tu amigo)");
        } else {
            ImGui::Text("%s   %s", d.room_code.c_str(), en ? "(joining...)" : "(entrando...)");
        }
    } else if (d.room_status.empty()) {
        ImGui::TextDisabled(en ? "(no code)" : "(sin codigo)");
    } else {
        // Say WHY, in the place the code would be, using the room module's own status
        // text -- so the panel and cdmp.log agree instead of the player having to go
        // read a file to find out what went wrong.
        ImGui::TextDisabled(en ? "(no code: %s)" : "(sin codigo: %s)", d.room_status.c_str());
    }
    if (!d.coop_hint.empty()) ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.6f, 1), "%s", d.coop_hint.c_str());
    ImGui::End();
}

void hud_draw(float width, float height) {
    static bool panel_visible = false;  // Co-op+: hidden until F8 / chord + X (the game shows no debug boxes)
    static bool plates_visible = true;
    static bool plates_init = false;
    static bool f8_was_down = false;
    static game::CameraReader camera;
    static bool camera_init = false;
    static ULONGLONG last_diag = 0;

    // F8: the panel. Ctrl+F8 (Co-op+): the nameplate over the other player. Pads: chord + X / chord + A.
    static input::Edge panel_edge, plates_edge;
    const bool focus = GetForegroundWindow() != nullptr;
    if (plates_edge(input::action_down(input::Action::Nameplates, focus))) plates_visible = !plates_visible;
    else if (panel_edge(input::action_down(input::Action::Panel, focus))) panel_visible = !panel_visible;
    (void)f8_was_down;

    HudData d;
    HudOptions opt;
    {
        std::lock_guard lock(g_mutex);
        d = g_data;
        opt = g_options;
    }
    if (!plates_init) {
        plates_visible = opt.nameplates;
        plates_init = true;
    }
    if (!camera_init) {
        camera.init();
        camera_init = true;
    }
    const auto cam = camera.read();

    // Is the camera position stored in world space or relative to the map
    // chunk like the player's physics position? Whichever is closer to us.
    std::optional<proto::Vec3> cam_world;
    bool cam_is_local = false;
    if (cam && d.local) {
        const float to_world = math::length(math::sub(cam->position, d.local->position));
        const float to_local = math::length(math::sub(cam->position, d.local->local_position));
        cam_is_local = to_local < to_world;
        cam_world = cam_is_local ? math::add(cam->position, d.local->chunk_origin) : cam->position;
    }

    // ---- status panel
    float remote_distance = 0;
    if (d.remote && d.local) remote_distance = math::length(math::sub(d.remote->pos, d.local->position));
    if (panel_visible) draw_panel(d, opt, remote_distance, cam ? std::string() : camera.last_failure());
    if (!d.toast.empty()) {
        // Co-op+: notices, top centre, whatever F8 says; the game's dialog look (game_ui.h). A first line
        // ending in a line break becomes the notice's gold title.
        const auto nl = d.toast.find('\n');
        ui::band(width, height * 0.07f, nl == std::string::npos ? std::string() : d.toast.substr(0, nl),
                 nl == std::string::npos ? d.toast : d.toast.substr(nl + 1));
    }

    if (d.boss_active && d.boss_hp_max > 0) {
        // The guest gets no native boss bar: the stage that draws it (BossNameTagView2) runs only in the host's
        // world. So draw one, the game's way (game_ui.h).
        ui::boss_bar(width, height, d.boss_name.empty() ? std::string("Boss") : d.boss_name, d.boss_hp / d.boss_hp_max,
                     d.boss_cutscene ? "The host is in a cutscene: wait" : "");
    } else if (d.duel_active && d.remote && d.remote->health_max > 0) {
        // Co-op+ duel: the opponent's health, as a boss bar.
        ui::boss_bar(width, height, d.peer_name.empty() ? std::string("Your opponent") : d.peer_name,
                     d.remote->health / d.remote->health_max);
    }
    if (!d.duel_title.empty() && d.duel_title_alpha > 0.01f) {  // the duel's or the hunting contest's
        ui::title_card(width, height, d.duel_title, d.duel_subtitle, d.duel_title_color, d.duel_title_alpha);
    }

    if (!d.activity.empty()) {
        // Co-op+: the other player's cutscene, a running challenge..., under the notices.
        ui::strip(width, height * 0.175f, d.activity);
    }

    if (d.revive_prompt) {
        // Co-op+: the game's own death screen, redrawn (revive_prompt.h): same words, same buttons.
        const char* choices = d.revive_prompt == 2 ? "Your partner is still fighting the boss. Hold on..."
                              : d.revive_prompt == 3 ? "Everyone is down.      {ReviveCheckpoint} Revive at Checkpoint"
                                                     : "{ReviveNow} Instant Revive (30% Health)        "
                                                       "{ReviveCheckpoint} Revive at Checkpoint";
        ui::death_screen(width, height, "Death has found you.", "Return to your path?", choices);
    }

    // Co-op+ (T14): the partner frame, whenever connected (not over a full-screen menu or a cutscene).
    if (d.connected && d.remote && !game::ui_menu_open() && !game::cutscene_playing().value_or(false) &&
        !d.revive_prompt) {
        const float hp = d.remote->health_max > 0 ? d.remote->health / d.remote->health_max : -1.0f;
        const float st = d.remote->stamina_max > 0 ? d.remote->stamina / d.remote->stamina_max : -1.0f;
        std::string status = d.partner_in_menu ? std::string("in a menu") : std::string();
        if (d.local) {
            const std::string dist = std::format("{:.0f} m", remote_distance);
            status = status.empty() ? dist : status + "  -  " + dist;
        }
        ui::party_frame(width, height, d.peer_name.empty() ? std::string("Your friend") : d.peer_name, hp, st, status,
                        ImGui::GetTime());
    }

    // ---- world markers (not over a full-screen menu: they showed on top of the map)
    if (!cam || !cam_world || game::ui_menu_open()) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    if (d.duel_active && d.duel_radius > 0) {
        // Co-op+ duel: the duel ground, a ring of gold on the ground with posts (coop_duel.h); red while we are out.
        const bool out = d.duel_out_left_s >= 0;
        const ImU32 ring = out ? IM_COL32(210, 60, 45, 210) : IM_COL32(232, 200, 130, 170);
        constexpr int kSegments = 96;
        std::optional<game::ScreenPoint> prev;
        for (int i = 0; i <= kSegments; ++i) {
            const float a = 6.2831853f * static_cast<float>(i) / kSegments;
            const proto::Vec3 p{d.duel_centre.x + d.duel_radius * std::cos(a), d.duel_centre.y + 0.08f,
                                d.duel_centre.z + d.duel_radius * std::sin(a)};
            auto sp = game::project(*cam, *cam_world, p, width, height);
            if (sp && prev && (i % 2 == 0)) {
                dl->AddLine(ImVec2(prev->x, prev->y), ImVec2(sp->x, sp->y), IM_COL32(0, 0, 0, 120), 5.0f);
                dl->AddLine(ImVec2(prev->x, prev->y), ImVec2(sp->x, sp->y), ring, 2.5f);
            }
            if (sp && i % 8 == 0 && i < kSegments) {
                const proto::Vec3 top{p.x, p.y + 1.6f, p.z};
                if (auto st = game::project(*cam, *cam_world, top, width, height)) {
                    dl->AddLine(ImVec2(sp->x, sp->y), ImVec2(st->x, st->y), IM_COL32(0, 0, 0, 110), 4.0f);
                    dl->AddLine(ImVec2(sp->x, sp->y), ImVec2(st->x, st->y), ring, 2.0f);
                    dl->AddCircleFilled(ImVec2(st->x, st->y), 3.5f, ring);
                }
            }
            prev = sp;
        }
    }

    if (d.remote && plates_visible && nameplate::alpha(remote_distance) > 0.0f) {
        const proto::Vec3 head{d.remote->pos.x, d.remote->pos.y + kHeadHeight, d.remote->pos.z};
        if (auto p = game::project(*cam, *cam_world, head, width, height)) {
            draw_remote_marker(dl, *p, d, remote_distance);
        }
    }

    // Co-op+ rock-paper-scissors duel: each player's hand over their character, as the town game shows the child's.
    if (d.rps_on) {
        constexpr const char* kHandName[3] = {"Scissors", "Paper", "Rock"};
        const bool reveal = d.rps_reveal_t >= 0;
        // A short "pop" as the hands come out.
        const float pop = reveal ? std::min(1.0f, 0.6f + d.rps_reveal_t * 2.0f) : 1.0f;
        auto medal = [&](const proto::Vec3& feet, int hand, bool picked, bool won, const std::string& who) {
            const proto::Vec3 over{feet.x, feet.y + 2.75f, feet.z};  // above the name tag
            auto p = game::project(*cam, *cam_world, over, width, height);
            if (!p) return;
            const float size = std::clamp(520.0f / std::max(p->depth, 1.0f), 34.0f, 78.0f) * pop;
            const int shown = reveal ? hand : picked ? -2 : -1;
            const ImU32 col = reveal && won ? IM_COL32(232, 200, 130, 255)
                              : reveal && d.rps_winner != 0 ? IM_COL32(170, 160, 150, 255)
                                                            : IM_COL32(222, 214, 200, 255);
            const std::string caption = reveal && hand >= 0 ? kHandName[hand] : picked ? who + " has chosen" : who;
            ui::hand_medallion(dl, p->x, p->y - size * 0.5f, size, shown, caption, col);
        };
        if (d.remote) {
            medal(d.remote->pos, d.rps_theirs, d.rps_theirs_picked, d.rps_winner == 2,
                  d.peer_name.empty() ? std::string("Your friend") : d.peer_name);
        }
        if (d.local) medal(d.local->position, d.rps_ours, d.rps_ours_picked, d.rps_winner == 1, "You");
    }

    std::optional<game::ScreenPoint> self;
    if (d.local) {
        const proto::Vec3 chest{d.local->position.x, d.local->position.y + 1.2f,
                                d.local->position.z};
        self = game::project(*cam, *cam_world, chest, width, height);
        if (opt.self_marker && self) {
            const ImVec2 c(self->x, self->y);
            dl->AddLine(ImVec2(c.x - 8, c.y), ImVec2(c.x + 8, c.y), IM_COL32(80, 220, 255, 255), 2);
            dl->AddLine(ImVec2(c.x, c.y - 8), ImVec2(c.x, c.y + 8), IM_COL32(80, 220, 255, 255), 2);
        }
    }

    // Numbers that let us verify the projection from the log alone: in third
    // person our own chest should land near the horizontal centre.
    const ULONGLONG now = GetTickCount64();
    if (now - last_diag >= 5000 && d.local) {
        last_diag = now;
        const auto st = status();
        log::info("Overlay diag: frames {} | cam {} space, axes {} | cam->player {:.2f} m | "
                  "self on screen {} | fov {:.1f} deg",
                  st.frames_drawn, cam_is_local ? "chunk-local" : "world",
                  cam->axes_from_view ? "from view matrix" : "fallback (cross)",
                  math::length(math::sub(*cam_world, d.local->position)),
                  self ? std::format("({:.0f}, {:.0f}) of {:.0f}x{:.0f}, depth {:.2f}", self->x,
                                     self->y, width, height, self->depth)
                       : std::string("behind camera"),
                  2.0f * std::atan(1.0f / cam->p11) * 57.2958f);
    }
}

}  // namespace cdmp::render
