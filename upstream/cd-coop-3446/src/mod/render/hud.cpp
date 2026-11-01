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
#include "mod/log.h"
#include "mod/render/dx12_hook.h"

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
    // Co-op+ (brief 6.2): fades and shrinks a little with distance (common/nameplate.h).
    const float alpha = nameplate::alpha(distance);
    const float scale = nameplate::text_scale(distance);
    ImFont* font = ImGui::GetFont();
    const float px = ImGui::GetFontSize() * scale;
    auto text = [&](ImVec2 pos, ImU32 color, const char* t) {
        dl->AddText(font, px, ImVec2(pos.x + 1, pos.y + 1), faded(kShadow, alpha), t);
        dl->AddText(font, px, pos, faded(color, alpha), t);
    };
    const ImVec2 c(p.x, p.y);
    dl->AddCircleFilled(c, 7.0f, faded(kShadow, alpha));
    dl->AddCircleFilled(c, 5.0f, faded(kAccent, alpha));
    const std::string name = d.peer_name.empty() ? (g_options.english ? "Player 2" : "Jugador 2") : d.peer_name;
    const ImVec2 size = font->CalcTextSizeA(px, FLT_MAX, 0.0f, name.c_str());
    text(ImVec2(c.x - size.x / 2, c.y - 28), IM_COL32_WHITE, name.c_str());
    if (d.remote->health_max > 0) {
        const ImVec2 a(c.x - 35, c.y - 12), b(c.x + 35, c.y - 7);
        const float frac = std::clamp(d.remote->health / d.remote->health_max, 0.0f, 1.0f);
        dl->AddRectFilled(a, b, faded(IM_COL32(0, 0, 0, 160), alpha));
        dl->AddRectFilled(a, ImVec2(a.x + 70 * frac, b.y), faded(IM_COL32(90, 200, 90, 255), alpha));
    }
    char dist[32];
    std::snprintf(dist, sizeof dist, "%.0f m", distance);
    const ImVec2 ds = font->CalcTextSizeA(px, FLT_MAX, 0.0f, dist);
    text(ImVec2(c.x - ds.x / 2, c.y + 10), IM_COL32(220, 220, 220, 255), dist);
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
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("cdmp", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.25f, 1), "Crimson Desert Seamless Co-op+ (alpha)");
    ImGui::SameLine();
    ImGui::TextDisabled("[F8]");
    // GPL credit, always visible with the panel: the co-op engine is theirs.
    ImGui::TextDisabled("built on cdmp %s by ByJunter, port by nouzert1", CDMP_VERSION);
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
    if (d.boss_active && d.boss_hp_max > 0) {
        // The guest gets no native boss bar: the stage that draws it
        // (BossNameTagView2) runs only in the host's world. So draw one.
        ImGui::Separator();
        const float frac = d.boss_hp / d.boss_hp_max;
        char overlay[96];
        std::snprintf(overlay, sizeof overlay, "%.0f / %.0f", d.boss_hp, d.boss_hp_max);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.75f, 0.15f, 0.15f, 1));
        ImGui::ProgressBar(frac < 0 ? 0.0f : (frac > 1 ? 1.0f : frac), ImVec2(-1, 0), overlay);
        ImGui::PopStyleColor();
        if (!d.boss_name.empty()) {
            ImGui::Text(en ? "Boss: %s" : "Jefe: %s", d.boss_name.c_str());
        }
        if (d.boss_cutscene) {
            // Worth saying out loud: the guest cannot act and nothing lands.
            ImGui::TextColored(ImVec4(1, 0.8f, 0.3f, 1),
                               en ? "Host cutscene: wait" : "Cinematica del anfitrion: espera");
        }
    }
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
    static bool panel_visible = true;
    static bool plates_visible = true;
    static bool plates_init = false;
    static bool f8_was_down = false;
    static game::CameraReader camera;
    static bool camera_init = false;
    static ULONGLONG last_diag = 0;

    // F8: the panel. Ctrl+F8 (Co-op+): the nameplate over the other player.
    const bool f8_down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (f8_down && !f8_was_down) {
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) plates_visible = !plates_visible;
        else panel_visible = !panel_visible;
    }
    f8_was_down = f8_down;

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
        // Co-op+: arrivals and departures, top centre, whatever F8 says.
        ImGui::SetNextWindowPos(ImVec2(width * 0.5f, height * 0.08f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("cdmp_toast", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.5f, 1.0f), "%s", d.toast.c_str());
        ImGui::End();
    }

    // ---- world markers
    if (!cam || !cam_world) return;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    if (d.remote && plates_visible && nameplate::alpha(remote_distance) > 0.0f) {
        const proto::Vec3 head{d.remote->pos.x, d.remote->pos.y + kHeadHeight, d.remote->pos.z};
        if (auto p = game::project(*cam, *cam_world, head, width, height)) {
            draw_remote_marker(dl, *p, d, remote_distance);
        }
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
