#pragma once
// Co-op+: the experimental opt-in (co-op saves + live quests), asked once on the first launch of a build that has it.
//
// [coop] coop_worlds = 1 is the opt-in: a save per friend (save_io.h, coop_worlds.h) and quest steps that follow
// between players (quest_live.h, quest_advance.h). Off by default; off = the mod behaves exactly as before (none of
// those hooks are installed). [general] experimental_asked = 1 once the player answered the card. Turning it on
// applies at once (no restart); turning it off applies at the next start (a co-op world may be in use).

#include <string>

namespace cdmp::optin {

// At start, after the config is read. `available`: co-op is on and the game build is supported.
void init(const std::wstring& ini_path, bool available, bool on, bool asked);
// Late init: the packet hooks are up (the quest server hooks need them).
void set_sniffer_ready(bool ready);

bool enabled();
// The first-launch card is up (it waits for an answer; nothing else may take its keys).
bool card_visible();
// The card's answer, or the F8 setting: on = turn on now; off = from the next start.
void answer(bool on);
// The F8 setting's live value (0 = on, 1 = off, like the panel's other on/off items).
int panel_index();

}  // namespace cdmp::optin
