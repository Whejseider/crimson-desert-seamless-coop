#pragma once
// Co-op+ story sync: the guest can take the host's story (quest progress) into their own save.
//
// Quests cannot follow live (each game's own server ignores outside quest requests, D42). So it goes through the save:
//   1. the GUEST picks "Copy my friend's story into my save: yes" in the F8 settings; their mod asks the host;
//   2. the HOST's mod reads the host's newest save on disk, takes its QuestSaveData (common/game_save.h) and sends it
//      (reliable channel, RelKind::Story, in parts); [coop] share_story=0 says no;
//   3. the guest's mod checks it fits the guest's newest save (same game build, a dry run of the whole copy) and
//      keeps it in Documents\CrimsonDesert-CoopPlus-SaveBackups\story-sync\pending-story.bin;
//   4. at the guest's NEXT game start, before any save is loaded, the mod backs up that save's slot folder
//      (...SaveBackups\before-story-sync-<time>), puts the host's story in, checks it and writes it.
// Everything else of the guest's save stays theirs: character, items, money, skills, map, position.
// Picking "no" before the restart cancels. Undo: copy the backup folder back (README).
//
// Wire (RelKind::Story): u8 op | u32 request id | ...
//   1 ask (guest -> host)
//   2 part (host -> guest): u16 index | u16 count | u32 story size | LZ4 bytes of the story, cut into parts
//   3 no (host -> guest): the reason, text

#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

// At start. share: [coop] share_story (the host lets a guest copy its story). dev_slot: [dev] story_slot, a slot
// folder used instead of the newest one (tests: never the player's own slots).
void story_configure(bool share, const std::wstring& dev_slot);
void story_set_share(bool share);  // F8 settings, live

// Worker thread, at start, before the game loads a save: puts a pending story into the newest save.
void story_startup();

// Network loop: link state (every tick).
void story_set_link(bool connected, bool host, const std::string& peer_name, uint64_t now_ms);
void story_on_message(const std::vector<uint8_t>& bytes);
// Messages to send now (one at a time, while the reliable channel has room).
std::vector<std::vector<uint8_t>> story_take_outgoing(size_t reliable_pending);

// F8 settings: "Copy my friend's story into my save".
void story_set_wanted(bool yes);
int story_wanted_index();  // 0 no, 1 yes (asked / received / waiting for the restart)

std::string story_hud_text();   // panel line ("" = nothing to say)
std::string story_take_toast(); // a one-off notice ("" = none)

}  // namespace cdmp::net
