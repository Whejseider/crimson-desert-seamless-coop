# Draft: Nexus Mods page (NOT POSTED; for M7, after the two-PC tests pass)

**Name:** Crimson Desert Seamless Co-op+
**Category:** Gameplay / Multiplayer. **Version:** 0.1.0-alpha. **Game version:** 1.0.0.3446 (Steam).

## Summary (one line)
Play Crimson Desert with a Steam friend: invite them from the Steam overlay or use "Join Game", see each other in the
world, and fight the same enemies.

## Description
Crimson Desert Seamless Co-op+ turns the single-player game into a two-player co-op adventure. Each of you plays in
your own save; your friend appears in your world as a real character, with your Steam names over your heads, on the
minimap and on the map. Near the host, the guest fights the host's enemies, and every blow counts.

It is built on Crimson Desert Multiplayer (cdmp) by ByJunter and its port to the current game version by nouzert1,
and adds Steam invites and "Join Game", rich presence, name plates, a save guard, party scaling and a one-click
installer.

**Alpha.** Expect rough edges. Read "Known issues" and back up your saves (the installer does it for you).

## Features
- Steam invites (Ctrl+F1) and "Join Game" from the friends list; no IPs, no ports. Friends-only, or invite-only.
- Your friend as a fully animated character: walking, running, climbing, gliding, riding, fighting, matching gear.
- Steam names over heads (fading with distance), the other player on the minimap and map.
- Shared enemies near the host; boss fights where the host lands the final blow so quests never break.
- Save guard: automatic save backups when a session starts; a warning if the game saves during co-op.
- Party scaling (experimental): tougher enemies with two players.
- One-click install / uninstall / vanilla mode; a diagnostics report for bug reports.

## Requirements
- Crimson Desert (Steam), game version 1.0.0.3446. Both players need the same Co-op+ version.
- Steam overlay enabled. Ultimate ASI Loader is included (MIT, by ThirteenAG).
- Recommended companions (install separately from their pages; credits below): Fat Stacks (momenaya),
  PrivateStorageAnywherePLUS (Stevi), Character Creator (Khione95), Barber Unlocked (Aerophus / AlexeySova).

## Installation
1. Back up `%LOCALAPPDATA%\Pearl Abyss\CD\save` (the installer also does this).
2. Unzip, double-click `Install Co-op+.cmd`. 3. Start the game through Steam. 4. Ctrl+F1 to invite a friend.
Uninstall: `Uninstall Co-op+.cmd`.

## Known issues
- Two players only. Quests, story progress and most loot are not shared yet.
- Do not save while the other player is in your world (the save guard backs up and warns).
- After a game update the mod turns itself off until it is updated.
- (Fill in from the two-PC test results.)

## Credits and permissions
- ByJunter (cdmp, GPL-3.0) and nouzert1 (cd-coop-3446, GPL-3.0): the co-op engine.
- ThirteenAG: Ultimate ASI Loader (MIT). Dear ImGui (Omar Cornut, MIT), SafetyHook (cursey, BSL-1.0), Zydis (MIT),
  toml++ (Mark Gillard, MIT).
- Companion mods are linked, never re-uploaded; see their pages for their permissions.
- Source code (GPL-3.0): included in the download as source.zip.
- Not affiliated with or endorsed by Pearl Abyss.
