# Changelog

## 0.1.0-alpha (unreleased; single-PC tested on game 1.0.0.3479)
Built on cdmp (ByJunter) as ported to game build 1.0.0.3446 by nouzert1 (cd-coop-3446, commit b7ae133), ported by
Co-op+ to game 1.0.0.3460 and then 1.0.0.3479.

Game 1.0.0.3479 (game version 2.04.05; Co-op+ port, tested in the game on one PC with the echo peer):
- Every address re-found for 3479 (a small patch: code moved by up to 0x70 bytes, data by ~0xFB0, message numbers
  unchanged). The other player's character loads, is drawn and stands next to you again.
- Party scaling is on by default: with a guest in, the host's enemies are tougher (x1.5 health, bosses x1.7,
  more for 3-4 players in scaling.toml).
- Boss fights (user request): if you die during one you wait until your partner beats the boss (then you get up
  where you fell, full health) or is down too (then only "Revive at Checkpoint" is offered). The guest's boss
  health bar can now show (the boss's health was never passed to it). [coop] boss_death_wait=0 turns the wait off.
- Deaths work while connected: once the other player's character had been in your world, the game never showed its
  "Death has found you" screen again and you stayed dead. Co-op+ now shows the same choice itself (Q Instant
  Revive, U Revive at Checkpoint) and sends the game's own revive requests. The checkpoint revive gives full
  health but only sometimes moves you to the checkpoint yet (no loading screen). The other player's character
  stays in your world meanwhile.
- The server messages relayed onto the other player's character (health, buffs, death, revive, wetness...) were
  still matched by their 1.0.0.3424 numbers, so none of them were passed on (and a few unrelated messages were).
  Renumbered, plus one more stale number in the "lethal blow" path (it sent a guild request instead).
- Gear changes are shared live: take off or put on a piece of gear and your friend sees it on your character
  (tested both ways). The old way of catching them did not exist in these builds; the mod now reads your
  character's equipped slots itself.
- Fixed a crash at startup with NVIDIA DLSS Frame Generation (about one launch in five): the overlay no longer
  creates a temporary swap chain to find the game's draw function.
- The name tag sits a little lower, just above the other player's head.
- The name tag is no longer drawn on top of the map, inventory, journal or pause menu.
- Enemies are recognised again: since 1.0.0.3460 every enemy was rejected (a field the mod reads moved by 4
  bytes), so the shared-enemy feature could never start. Bandits now pass the check on both sides.
- Copies of the other player's enemies are OFF for now (`[dev] enemy_copies=1` turns them on for testing): on
  1.0.0.3479 a copy crashes the game right after it loads. Until that is fixed, each player fights the enemies of
  their own world; nothing is hidden.
- The game's clock could never be read (an address check compared the wrong numbers); the mod now falls back to
  estimating it, as designed, instead of waiting forever.

Game 1.0.0.3460 (Co-op+ port, tested in the game on one PC with the echo peer):
- Every address re-found for 3460; the movement messages' new layout; all message numbers from the live game.
- The other player's character now finishes loading, is drawn, and follows (it stayed half-loaded and invisible).
- The other player is loaded only once our own world has finished loading (it could take our control away).
- Camera fix for the name plates on 3460.

Added by Co-op+:
- Steam "Join Game" and overlay invites: everyone hosts their own world (`[net] role=auto`); Ctrl+F1 opens the Steam
  invite dialog; accepting an invite or using "Join Game" switches you into your friend's world, even if your game
  was closed (Steam starts it); Ctrl+F2 leaves. Rich presence shows "hosting (1/2)" or "in <friend>'s world".
- Who can join: `[net] join=friends` (default) or `join=invite` (only friends you invite).
- Your Steam name is your player name; it shows over your head on your friend's screen, fading with distance
  (Ctrl+F8 toggles it; F8 toggles the panel).
- A short message when your friend joins or leaves.
- Save guard: your saves are copied to Documents\CrimsonDesert-CoopPlus-SaveBackups each time a co-op session starts
  (newest 10 kept), and the panel warns if the game saves during co-op.
- Party scaling (experimental, off by default): with a guest in the host's world, enemies take less damage
  (health x1.5, bosses x1.7 from cdcoop\config\scaling.toml).
- After a game update the mod stays off and says so on the panel, instead of looking broken.
- One-click installer (finds the game through Steam, backs up saves), uninstaller, Vanilla mode on-off, Diagnose.

Fixed in the upstream code:
- The wall-climb and mount fixes for the other player's character pointed 0x10 bytes into their game functions on
  1.0.0.3446 and never installed (other player froze on walls); now correct and checked offline.
- The UTF-16 settings profiles were garbled on Windows checkouts (git line-ending conversion).
- `opcodes_resolved()` could not be linked (defined in an anonymous namespace).
