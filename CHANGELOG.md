# Changelog

## 0.1.0-alpha (unreleased; not yet tested in the game)
Built on cdmp (ByJunter) as ported to game build 1.0.0.3446 by nouzert1 (cd-coop-3446, commit b7ae133).

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
