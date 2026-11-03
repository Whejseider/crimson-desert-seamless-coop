#pragma once
// Co-op+: the overlay drawn the way the game draws its own screens (user 2026-11-02: "I want the UI for everything
// we have added to look like it belongs in the game"). The game's look, from its own screens: a serif face (Windows'
// Palatino Linotype is the closest one every PC has), dark bands that fade out to the sides with thin light rules
// above and below (its dialogs), warm gold titles, the red "Death has found you." with a red brush stroke, and key
// prompts drawn as small light key caps. Button prompts follow the input method (input/pad_input.h): key caps on a
// keyboard, coloured A/B/X/Y on Xbox pads, the four shapes on PlayStation pads.
//
// Texts may carry action tokens, "{Challenge} Challenge  {Decline} Decline": each token is drawn as that action's
// button(s) (input/actions.h).

#include <string>

struct ImDrawList;
struct ImFont;

namespace cdmp::render::ui {

// Once, right after ImGui::CreateContext: the serif fonts.
void load_fonts();

// The serif font (nullptr until loaded: ImGui's default is used).
ImFont* serif();
ImFont* serif_bold();

// A centred notice band at `y` (screen px): title (gold, may be empty) over body text, game dialog style.
void band(float screen_w, float y, const std::string& title, const std::string& body, float alpha = 1.0f);

// A one-line band (lighter), e.g. the running state of a challenge.
void strip(float screen_w, float y, const std::string& text, float alpha = 1.0f);

// The game's death screen: darkened screen, red title with its brush stroke, the question, the choices.
void death_screen(float screen_w, float screen_h, const std::string& title, const std::string& question,
                  const std::string& choices);

// Text with action tokens at `x, y`, `size` px high (returns the width drawn). Measure only when dl is null.
float rich_text(ImDrawList* dl, float x, float y, float size, const std::string& text, unsigned int color,
                ImFont* font = nullptr);

}  // namespace cdmp::render::ui
