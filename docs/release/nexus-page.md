# Nexus Mods page (NOT POSTED: post after the two-PC playtest, docs/tests/PLAYTEST-1.md)

Paste-ready text for the mod page. Update the version lines and "Known issues" from the playtest first.
Screenshots: `docs/release/shot-list.md`. Files to upload: the player zip (main file), the `-mod-manager.zip`
(optional file), never the `-symbols-KEEP-PRIVATE.zip`.

**Name:** Crimson Desert Seamless Co-op+
**Category:** Gameplay. **Tags:** multiplayer, co-op, gameplay. **Version:** 0.1.0-alpha.
**Game version:** 1.0.0.3479 / 2.04.05 (Steam). On any other version the mod switches itself off.

## Summary (one line, at most 350 characters)
Play Crimson Desert with a Steam friend. Invite them or use "Join Game", explore and fight the same enemies
together, chat, duel, revive each other. Each player keeps their own save.

## Description

Crimson Desert Seamless Co-op+ is an unofficial fan mod that brings two-player co-op to the game. Each of you plays
in your own save. Your friend appears in your world as their own character, with their gear, their name over their
head and a pin on your map. Near the host you fight the same enemies.

It is built on Crimson Desert Multiplayer (cdmp) by ByJunter and its port to newer game versions by nouzert1. It adds
Steam joining, a co-op HUD, shared enemies and many co-op features (below).

**This is an alpha.** Read "Known issues". The installer backs up your saves. Don't save while your friend is in your
world.

### Features
- **Steam joining:** invite with Ctrl+F1 or use "Join Game" from the friends list. No IPs or ports. You choose
  friends-only or invite-only. A room code is the fallback.
- **Your friend in your world:** walking, running, climbing, gliding, riding (horse and flying mount), fighting, and
  matching gear that updates live. Their name over their head; on the minimap and the map.
- **Shared enemies:** near the host, both players see and fight the same enemies. Every blow counts. Enemy toughness
  scales with the party (configurable).
- **Partner frame:** your friend's health and stamina on screen, with a "Badly hurt" warning.
- **Revive each other:** stand by a downed friend and hold Shift+F7 (pad: View + LB). Optional respawn back at your
  friend after a checkpoint revive.
- **Bosses:** fight together. The host lands the final blow, so the host's story stays intact. Shared boss cutscenes;
  a shared cutscene skips only when everyone has pressed skip.
- **Talk:** text chat (Shift+F2) and quick messages (Go here, Danger, Loot here, Follow me...) with map pins.
- **Travel together:** go to your friend (Ctrl+F4), ask them to come to you (Shift+F4), follow when they fast travel.
  Shared waypoints.
- **Challenges:** duel (ring, rounds, nobody dies), rock-paper-scissors, a hunting contest, and arm wrestling where
  your friend sits across the table from you.
- **Shared time and weather** (the host's).
- **Notices:** your friend's quest steps, world events near them, when they become wanted, when they're in a menu or
  a cutscene.
- **Loot:** per player (default), host only, or first come.
- **Safety:** save backups when a session starts and a warning if the game saves during co-op. Version check on join.
  Automatic resync. Your friend can't kill your townsfolk or quest NPCs. Your world is left clean when they leave.
- **Controller support** for everything: hold View or the touchpad button and press a button. Prompts show the
  buttons of your device.
- **One-click** install, uninstall and vanilla mode. A diagnose report for bug reports. A notice names other mods that
  could clash.

### Requirements
- Crimson Desert on Steam, game version 1.0.0.3479 (2.04.05). Both players need the **same Co-op+ zip**.
- Steam friends, with the Steam overlay on.
- Ultimate ASI Loader is included (ThirteenAG, MIT). Nothing else to install.

### Installation
1. Close the game. Unzip the main file anywhere.
2. Double-click `Install Co-op+.cmd`. It finds the game through Steam and backs up your saves. It puts winmm.dll,
   cdmp.asi, cdmp.ini and cdcoop\config\scaling.toml into the game's bin64 folder, and keeps any file it replaces.
3. Start the game from Steam and load a save. Ctrl+F1 invites a friend; F8 opens the co-op panel.

**Uninstall:** `Uninstall Co-op+.cmd` removes exactly what it installed and restores anything it replaced.
**Vanilla mode:** `Vanilla mode on-off.cmd` switches the mod off and on, keeping your settings.
**Manual / Vortex / MO2:** use the optional `-mod-manager.zip`. Its `bin64` folder goes into the game folder (next to
CrimsonDesert.exe). Back up your saves yourself.

### Controls

| Keyboard | Pad (hold View / touchpad +) | Action |
|---|---|---|
| Ctrl+F1 | Y / Triangle | invite a friend |
| Ctrl+F2 | B / Circle | leave (guest) |
| Ctrl+F3 | D-pad Left | quick messages |
| Shift+F2 | - | chat |
| Ctrl+F4 | D-pad Right | go to your friend |
| Shift+F4 | R3 | ask your friend to come to you |
| Shift+F5 / Shift+F6 | D-pad Up / Down | challenge, accept / decline |
| Shift+F7 | LB / L1 (hold) | revive your friend |
| Shift+F8 | RB / R1 | resync |
| F8 | X / Square | co-op panel |
| Ctrl+F8 | A / Cross | name over your friend's head |
| Shift+F1 | L3 | the controls card |

### Known issues
- Two players only.
- Quests and story progress aren't shared: each of you keeps your own.
- The guest gets loot from shared kills, but no XP for them yet.
- No item trading or gifting, and no death-penalty options.
- Doors, levers and breakable objects aren't synced.
- Arm wrestling: if the guest arrives at the pub already connected, the table can be empty for them. Leave the pub
  and come back.
- Animals and townsfolk are each player's own: they stand in the same places, but they aren't the same ones.
- Bosses are finished by the host.
- After a game update the mod switches itself off until it's updated.
- (Fill in from the two-PC playtest.)

### Other mods
Most mods work alongside Co-op+. Gameplay mods (stamina, stacks, loot) must be the same on both PCs, and keep only
one ASI loader. See OTHER-MODS.md in the download.

### Is it safe?
Crimson Desert has no anti-cheat and no online play of its own. Co-op+ changes only your local game, never your
account, the store, cross-save or Steam Cloud. It sends nothing anywhere except to the friend you play with. If an
update ever adds an anti-cheat, Co-op+ installs nothing and tells you.

### Bug reports
Use the Bugs tab with BUG-REPORT.txt from the download. Attach the report "Diagnose Co-op+.cmd" puts on your Desktop
(from both players if you can).

### Credits and permissions
- **ByJunter**: Crimson Desert Multiplayer (cdmp, GPL-3.0), the co-op engine and its research.
  https://www.nexusmods.com/crimsondesert/mods/3677
- **nouzert1**: cd-coop-3446 (GPL-3.0), the port to newer game versions.
- **ThirteenAG**: Ultimate ASI Loader (MIT), included as winmm.dll.
- Libraries: Dear ImGui (Omar Cornut, MIT), SafetyHook (cursey, BSL-1.0), Zydis / Zycore (MIT), toml++ (Mark
  Gillard, MIT).
- No other authors' mod files are included. Mods that inspired features are credited in CREDITS.md.
- Source code (GPL-3.0) is in the download as source.zip.
- Not affiliated with or endorsed by Pearl Abyss. Crimson Desert is a trademark of Pearl Abyss Corp.

### Permissions (for the Nexus "Permissions and credits" tab)
- Upload elsewhere: no (link to this page).
- Modify and reupload: yes, under GPL-3.0 with credit and source.
- Assets: GPL-3.0 code, with credit.
- Use in paid mods: no. Donation points: no.
