#include "mod/render/game_ui.h"

#include <windows.h>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <string_view>

#include "mod/input/actions.h"
#include "mod/input/pad_input.h"
#include "mod/log.h"

namespace cdmp::render::ui {
namespace {

ImFont* g_serif = nullptr;
ImFont* g_serif_bold = nullptr;

constexpr ImU32 kGold = IM_COL32(232, 200, 130, 255);
constexpr ImU32 kText = IM_COL32(232, 228, 220, 255);
constexpr ImU32 kRule = IM_COL32(205, 195, 175, 150);
constexpr ImU32 kCap = IM_COL32(214, 210, 202, 240);
constexpr ImU32 kCapText = IM_COL32(28, 26, 24, 255);

ImU32 with_alpha(ImU32 c, float a) {
    const auto base = static_cast<float>((c >> IM_COL32_A_SHIFT) & 0xFF);
    const auto na = static_cast<ImU32>(std::clamp(base * a, 0.0f, 255.0f));
    return (c & ~IM_COL32_A_MASK) | (na << IM_COL32_A_SHIFT);
}

ImFont* font_or_default(ImFont* f) { return f ? f : ImGui::GetFont(); }

float text_w(ImFont* f, float size, std::string_view s) {
    return font_or_default(f)->CalcTextSizeA(size, FLT_MAX, 0.0f, s.data(), s.data() + s.size()).x;
}

void text(ImDrawList* dl, ImFont* f, float size, float x, float y, ImU32 c, std::string_view s, bool shadow = true) {
    if (shadow) {
        dl->AddText(font_or_default(f), size, ImVec2(x + 1, y + 1), with_alpha(IM_COL32(0, 0, 0, 200), 1.0f), s.data(),
                    s.data() + s.size());
    }
    dl->AddText(font_or_default(f), size, ImVec2(x, y), c, s.data(), s.data() + s.size());
}

// A horizontal band that fades to nothing at both ends (the game's dialog background).
void faded_rect(ImDrawList* dl, float x0, float y0, float x1, float y1, ImU32 c) {
    const float w = x1 - x0, edge = w * 0.22f;
    const ImU32 clear = with_alpha(c, 0.0f);
    dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x0 + edge, y1), clear, c, c, clear);
    dl->AddRectFilled(ImVec2(x0 + edge, y0), ImVec2(x1 - edge, y1), c);
    dl->AddRectFilledMultiColor(ImVec2(x1 - edge, y0), ImVec2(x1, y1), c, clear, clear, c);
}

// ---------------------------------------------------------------- button icons

float cap(ImDrawList* dl, float x, float y, float h, std::string_view label) {
    const float fs = h * 0.62f;
    const float w = std::max(h, text_w(g_serif_bold, fs, label) + h * 0.5f);
    if (dl) {
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), kCap, h * 0.18f);
        dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(90, 86, 80, 255), h * 0.18f, 1.0f);
        text(dl, g_serif_bold, fs, x + (w - text_w(g_serif_bold, fs, label)) * 0.5f, y + (h - fs) * 0.5f - 1, kCapText,
             label, false);
    }
    return w;
}

float pill(ImDrawList* dl, float x, float y, float h, std::string_view label, ImU32 bg) {
    const float fs = h * 0.5f;
    const float w = text_w(g_serif_bold, fs, label) + h * 0.6f;
    if (dl) {
        dl->AddRectFilled(ImVec2(x, y + h * 0.12f), ImVec2(x + w, y + h * 0.88f), bg, h * 0.38f);
        text(dl, g_serif_bold, fs, x + h * 0.3f, y + (h - fs) * 0.5f - 1, IM_COL32(240, 240, 240, 255), label, false);
    }
    return w;
}

float dpad(ImDrawList* dl, float x, float y, float h, uint16_t arm) {
    if (dl) {
        const float c = h * 0.5f, t = h * 0.16f, l = h * 0.46f;
        const ImVec2 m(x + c, y + c);
        const ImU32 off = IM_COL32(110, 110, 110, 255), on = IM_COL32(245, 245, 245, 255);
        auto bar = [&](float dx, float dy, uint16_t bit) {
            const ImVec2 a(m.x + std::min(0.0f, dx) - (dx == 0 ? t : 0), m.y + std::min(0.0f, dy) - (dy == 0 ? t : 0));
            const ImVec2 b(m.x + std::max(0.0f, dx) + (dx == 0 ? t : 0), m.y + std::max(0.0f, dy) + (dy == 0 ? t : 0));
            dl->AddRectFilled(a, b, (arm & bit) ? on : off, t * 0.4f);
        };
        bar(0, -l, input::kPadUp);
        bar(0, l, input::kPadDown);
        bar(-l, 0, input::kPadLeft);
        bar(l, 0, input::kPadRight);
    }
    return h;
}

float face(ImDrawList* dl, float x, float y, float h, uint16_t bit, bool playstation) {
    if (!dl) return h;
    const ImVec2 c(x + h * 0.5f, y + h * 0.5f);
    const float r = h * 0.48f;
    if (!playstation) {
        ImU32 col = IM_COL32(96, 176, 72, 255);
        const char* l = "A";
        if (bit == input::kPadB) col = IM_COL32(210, 64, 58, 255), l = "B";
        if (bit == input::kPadX) col = IM_COL32(64, 128, 216, 255), l = "X";
        if (bit == input::kPadY) col = IM_COL32(222, 182, 40, 255), l = "Y";
        dl->AddCircleFilled(c, r, IM_COL32(30, 30, 30, 240));
        dl->AddCircle(c, r * 0.86f, col, 0, h * 0.09f);
        const float fs = h * 0.62f;
        text(dl, g_serif_bold, fs, c.x - text_w(g_serif_bold, fs, l) * 0.5f, c.y - fs * 0.52f, col, l, false);
        return h;
    }
    dl->AddCircleFilled(c, r, IM_COL32(32, 32, 36, 240));
    const float s = h * 0.24f, w = h * 0.08f;
    if (bit == input::kPadA) {  // cross
        const ImU32 col = IM_COL32(126, 178, 232, 255);
        dl->AddLine(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), col, w);
        dl->AddLine(ImVec2(c.x - s, c.y + s), ImVec2(c.x + s, c.y - s), col, w);
    } else if (bit == input::kPadB) {  // circle
        dl->AddCircle(c, s, IM_COL32(232, 92, 92, 255), 0, w);
    } else if (bit == input::kPadX) {  // square
        dl->AddRect(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), IM_COL32(226, 138, 186, 255), 0.0f, w);
    } else {  // triangle
        dl->AddTriangle(ImVec2(c.x, c.y - s * 1.1f), ImVec2(c.x - s * 1.1f, c.y + s * 0.8f),
                        ImVec2(c.x + s * 1.1f, c.y + s * 0.8f), IM_COL32(100, 200, 160, 255), w);
    }
    return h;
}

// One pad button in the family's look.
float pad_button(ImDrawList* dl, float x, float y, float h, uint16_t bit, bool ps) {
    if (bit & input::kPadFace) return face(dl, x, y, h, bit, ps);
    if (bit & input::kPadDpad) return dpad(dl, x, y, h, bit);
    const ImU32 bg = IM_COL32(60, 60, 64, 240);
    if (bit == input::kPadBack) return pill(dl, x, y, h, ps ? "CREATE" : "VIEW", bg);
    if (bit == input::kPadTouchpad) return pill(dl, x, y, h, "TOUCHPAD", bg);
    if (bit == input::kPadStart) return pill(dl, x, y, h, ps ? "OPTIONS" : "MENU", bg);
    if (bit == input::kPadLB) return pill(dl, x, y, h, ps ? "L1" : "LB", bg);
    if (bit == input::kPadRB) return pill(dl, x, y, h, ps ? "R1" : "RB", bg);
    return pill(dl, x, y, h, "?", bg);
}

float plus(ImDrawList* dl, float x, float y, float h) {
    const float fs = h * 0.6f;
    if (dl) text(dl, g_serif, fs, x + 2, y + (h - fs) * 0.5f - 1, kText, "+");
    return text_w(g_serif, fs, "+") + 4;
}

// The action's button(s) for the current input method.
float action_glyph(ImDrawList* dl, float x, float y, float h, input::Action a) {
    const auto& b = input::binding(a);
    const auto m = input::method();
    float w = 0;
    if (m == input::Method::Keyboard) {
        auto key_name = [](int vk) -> std::string {
            if (vk >= VK_F1 && vk <= VK_F12) return "F" + std::to_string(vk - VK_F1 + 1);
            if (vk == 0x1B) return "Esc";
            return std::string(1, static_cast<char>(vk));
        };
        if (b.ctrl) w += cap(dl, x + w, y, h, "Ctrl"), w += plus(dl, x + w, y, h);
        if (b.shift) w += cap(dl, x + w, y, h, "Shift"), w += plus(dl, x + w, y, h);
        return w + cap(dl, x + w, y, h, key_name(b.vk));
    }
    const bool ps = m == input::Method::PlayStation;
    if (b.chord) {
        w += pad_button(dl, x + w, y, h, ps ? input::kPadTouchpad : input::kPadBack, ps);
        w += plus(dl, x + w, y, h);
    }
    return w + pad_button(dl, x + w, y, h, b.pad_shown, ps);
}

}  // namespace

void load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    const char* dir = "C:\\Windows\\Fonts\\";
    for (const char* name : {"pala.ttf", "georgia.ttf", "constan.ttf"}) {
        const std::string p = std::string(dir) + name;
        if (!std::filesystem::exists(p)) continue;
        g_serif = io.Fonts->AddFontFromFileTTF(p.c_str(), 20.0f);
        const std::string bold = std::string(dir) + (std::string_view(name) == "pala.ttf" ? "palab.ttf" : name);
        if (std::filesystem::exists(bold)) g_serif_bold = io.Fonts->AddFontFromFileTTF(bold.c_str(), 20.0f);
        if (g_serif) {
            io.FontDefault = g_serif;
            log::info("Overlay: game-style font {}", name);
            break;
        }
    }
    if (!g_serif_bold) g_serif_bold = g_serif;
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 2.0f;
    s.WindowBorderSize = 0.0f;
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.035f, 0.03f, 0.78f);
    s.Colors[ImGuiCol_Text] = ImVec4(0.91f, 0.89f, 0.86f, 1.0f);
    s.Colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.76f, 0.68f, 0.45f);
    s.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.70f, 0.16f, 0.13f, 1.0f);
}

ImFont* serif() { return g_serif; }
ImFont* serif_bold() { return g_serif_bold; }

float rich_text(ImDrawList* dl, float x, float y, float size, const std::string& s, unsigned int color, ImFont* f) {
    if (!f) f = g_serif;
    float w = 0;
    size_t at = 0;
    while (at < s.size()) {
        const size_t open = s.find('{', at);
        const size_t close = open == std::string::npos ? std::string::npos : s.find('}', open);
        const size_t lit_end = close == std::string::npos ? s.size() : open;
        if (lit_end > at) {
            const std::string_view lit(s.data() + at, lit_end - at);
            if (dl) text(dl, f, size, x + w, y, color, lit);
            w += text_w(f, size, lit);
        }
        if (close == std::string::npos) break;
        const auto a = input::action_by_name(std::string_view(s.data() + open + 1, close - open - 1));
        if (a != input::Action::Count) {
            const float h = size * 1.15f;
            w += action_glyph(dl, x + w, y + (size - h) * 0.5f, h, a) + size * 0.25f;
        } else {
            const std::string_view raw(s.data() + open, close - open + 1);
            if (dl) text(dl, f, size, x + w, y, color, raw);
            w += text_w(f, size, raw);
        }
        at = close + 1;
    }
    return w;
}

void band(float sw, float y, const std::string& title, const std::string& body, float alpha) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float k = std::max(0.6f, sw / 1920.0f);
    const float ts = 26.0f * k, bs = 20.0f * k;
    const float tw = title.empty() ? 0.0f : text_w(g_serif_bold, ts, title);
    const float bw = rich_text(nullptr, 0, 0, bs, body, kText);
    const float w = std::max({tw, bw, 360.0f * k}) + 260.0f * k;
    const float h = (title.empty() ? 0.0f : ts + 10 * k) + bs + 34 * k;
    const float x0 = sw * 0.5f - w * 0.5f, x1 = sw * 0.5f + w * 0.5f;
    faded_rect(dl, x0, y, x1, y + h, with_alpha(IM_COL32(8, 7, 6, 215), alpha));
    faded_rect(dl, x0, y, x1, y + 1.0f, with_alpha(kRule, alpha));
    faded_rect(dl, x0, y + h - 1.0f, x1, y + h, with_alpha(kRule, alpha));
    float ty = y + 16 * k;
    if (!title.empty()) {
        text(dl, g_serif_bold, ts, sw * 0.5f - tw * 0.5f, ty, with_alpha(kGold, alpha), title);
        ty += ts + 10 * k;
    }
    rich_text(dl, sw * 0.5f - bw * 0.5f, ty, bs, body, with_alpha(kText, alpha));
}

void strip(float sw, float y, const std::string& body, float alpha) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float k = std::max(0.6f, sw / 1920.0f);
    const float bs = 18.0f * k;
    const float bw = rich_text(nullptr, 0, 0, bs, body, kText);
    const float w = bw + 220.0f * k, h = bs + 18 * k;
    const float x0 = sw * 0.5f - w * 0.5f, x1 = sw * 0.5f + w * 0.5f;
    faded_rect(dl, x0, y, x1, y + h, with_alpha(IM_COL32(8, 7, 6, 170), alpha));
    faded_rect(dl, x0, y + h - 1.0f, x1, y + h, with_alpha(kRule, alpha * 0.7f));
    rich_text(dl, sw * 0.5f - bw * 0.5f, y + 8 * k, bs, body, with_alpha(kText, alpha));
}

void death_screen(float sw, float sh, const std::string& title, const std::string& question,
                  const std::string& choices) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float k = std::max(0.6f, sh / 1080.0f);
    // The game greys and darkens the world; we can only darken: an even veil and heavier edges.
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(sw, sh), IM_COL32(12, 12, 14, 120));
    const ImU32 edge = IM_COL32(0, 0, 0, 170), clear = IM_COL32(0, 0, 0, 0);
    dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(sw, sh * 0.25f), edge, edge, clear, clear);
    dl->AddRectFilledMultiColor(ImVec2(0, sh * 0.75f), ImVec2(sw, sh), clear, clear, edge, edge);
    const float ts = 40.0f * k, qs = 22.0f * k, cs = 20.0f * k;
    const float cy = sh * 0.44f;
    // The red brush stroke behind the question.
    faded_rect(dl, sw * 0.5f - 380 * k, cy + ts * 0.95f, sw * 0.5f + 380 * k, cy + ts * 0.95f + 34 * k,
               IM_COL32(120, 14, 14, 150));
    const float tw = text_w(g_serif, ts, title);
    text(dl, g_serif, ts, sw * 0.5f - tw * 0.5f, cy - ts * 0.4f, IM_COL32(196, 48, 40, 255), title);
    const float qw = text_w(g_serif, qs, question);
    text(dl, g_serif, qs, sw * 0.5f - qw * 0.5f, cy + ts * 0.95f + 6 * k, IM_COL32(236, 232, 226, 255), question);
    const float cw = rich_text(nullptr, 0, 0, cs, choices, IM_COL32(220, 216, 210, 255));
    rich_text(dl, sw * 0.5f - cw * 0.5f, cy + ts * 0.95f + 70 * k, cs, choices, IM_COL32(220, 216, 210, 255));
}

}  // namespace cdmp::render::ui
