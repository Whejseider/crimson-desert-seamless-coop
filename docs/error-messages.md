# What players see when something fails (T29)

Every failure state of Co-op+ and its installer, with the message the player sees and where it appears. Each
message names the next step. Reviewed 2026-11-03.

Where the messages appear:
- **Panel:** the F8 panel, in red at the top, for as long as the problem lasts.
- **Strip:** the same line at the top of the screen for 15 s whenever it changes, so it's seen without F8.
- **Notice:** the game-style notice in the middle of the screen, a few seconds.
- **Window:** a plain Windows message box.
- **Installer:** the installer's console window, in red.

All messages also go to `bin64\cdmp.log`.

## In the game

| State | When | Message (where) | Checked |
|---|---|---|---|
| Game version not supported | A Steam update changed the game | "Co-op+ is off: this mod was made for game version 1.0.0.3479, not X. The game runs normally; update Co-op+ to play together." (panel, the only line) | yes (earlier, port3479) |
| Anti-cheat found | An update added an anti-cheat module or files | "Co-op+ is off: this game now uses anti-cheat (X). Co-op+ will not run beside it, so your account stays safe; the game runs normally. Remove Co-op+ (Uninstall Co-op+.cmd) and wait for word that it is safe." (window). Nothing of the mod is installed | yes (T02, `[dev] online_guard_test`) |
| Different Co-op+ build or game version | Join with a friend on another zip or game version | "Can't play together: NAME has Co-op+ build X on game Y, you have build Z on game W. Both install the same Co-op+ zip." or "...Both update the game in Steam, then install the Co-op+ zip made for that version." (notice 12 s + panel) | yes (T03, `[dev] fake_peer_build`) |
| Different protocol (much older or newer Co-op+) | Same as above, but the other side is too old to compare builds | "Can't play together: NAME has a different Co-op+ version (protocol X, yours Y). Both install the same Co-op+ zip." (panel). **Changed in T29:** was "'NAME' uses protocol X, we use Y: both need the same mod version" | build only |
| Join failed: host not reachable | A guest still not connected after 45 s | "Can't reach NAME yet. Check: both games running with a save loaded, the same Co-op+ zip, Steam online. Still stuck: Ctrl+F2, then Join Game again." (panel + strip). Clears when connected. **New in T29** | yes (errors_test A/B) |
| Steam not ready | Still waiting for the game's Steam login after 60 s (offline mode, Steam not running) | "Steam is not ready for co-op: start the game from Steam while online, with the Steam overlay on (Steam > Settings > In Game)." (panel + strip). **New in T29** | build only |
| Bad room code | `[room] code=` in cdmp.ini isn't 7 letters and digits | "The room code 'X' in cdmp.ini is not valid (7 letters and digits). Fix it under [room] code= and restart the game, or use Join Game in Steam instead." (panel + strip). **New in T29** (was log only) | build only |
| Session lost | The other player's game closed, crashed or lost the network | Guest: "Lost the connection to NAME's world. Reconnecting by itself; [Ctrl+F2] goes back to your own world" (notice). Host: "NAME left your world" (notice). The guest's own enemies come back. Rejoin is automatic when the host returns (T04). **Next step added in T29** | yes (errors_test C) |
| Network didn't start | The connection couldn't be opened (Windows networking, Steam networking) | "Co-op could not start its connection; trying again every 10 s. If this stays, restart the game from Steam and check cdmp.log (Diagnose Co-op+)." (panel + strip). Clears when it starts. **New in T29** (was log only). Windows networking itself failing: "Co-op can't start: Windows networking did not start. Restart the PC; if it stays, send cdmp.log (Diagnose Co-op+)." | build only |
| Save backup failed | The automatic backup when a session starts couldn't be written | "Your saves could not be backed up (REASON). Don't save during co-op, and copy %LOCALAPPDATA%\Pearl Abyss\CD\save somewhere yourself. Is the disk full?" (panel + strip). **New in T29** (was log only) | build only |
| The game saved during co-op | Autosave or a manual save while the friend is in | "The game saved during co-op. Backup: auto-..." (panel, 60 s) | yes (save guard) |
| Safety hooks didn't install | On a rare launch a crash guard can't be installed (about 1 in 80; retried 5 times) | "Shared enemies are off this time (a safety check did not install). Restart the game to try again." or "Your friend's horse and the shared enemies are off this time (...)" (panel + strip). **New in T29** (was log only) | build only (the retry itself: T28) |
| Other mods next to the game | Another ASI loader, .asi mod or ReShade in bin64 | "Other mods next to the game: X. If co-op misbehaves, try without them first." (notice, once) | yes (T28) |
| Invite failed | Ctrl+F1 when it can't work | "only the host can invite (you are in a friend's world)"; "no room yet: load a save, wait a few seconds, then invite again"; "the Steam overlay is not available (Steam > Settings > In Game)" (panel hint) | yes (earlier) |
| Leave as host | Ctrl+F2 as the host | "you are the host: your friend leaves from their side (Ctrl+F2)" (panel hint) | yes (earlier) |
| Challenge failed | Nobody connected, no answer, partner left | "No one to challenge: invite a friend first ([Ctrl+F1])"; "NAME did not answer the challenge"; "Challenge ended: the other player left" (notice) | yes (T08) |
| Summon refused | Cooldown or already together | "You can call NAME again in N s"; "NAME is already with you" (notice) | yes (T19) |

## Installer, uninstaller, diagnose

| State | Message (installer) |
|---|---|
| Incomplete download | "NOT INSTALLED: the package is incomplete (FILE missing). Download it again." |
| Game not found | "NOT INSTALLED: Crimson Desert was not found through Steam. Run this again from PowerShell with -GameDir "<your Steam library>\steamapps\common\Crimson Desert\bin64"." |
| Game running | "NOT INSTALLED: Crimson Desert is running. Close the game first, then run the installer again." |
| Save backup failed | "NOT INSTALLED: the save backup to X came out empty, so nothing was installed. Check that Documents has free space and is not read-only (OneDrive: let it finish syncing), then run the installer again." (**next step added in T29**) |
| Another game version | "Note: your game is version X; this mod was built for Y. On another version the mod switches itself off and the game runs normally." (yellow, installs anyway) |
| Uninstall with nothing installed | "Co-op+ is not installed in X (no coopplus-installed.txt). Nothing removed." |
| Diagnose without a log | "--- no cdmp.log: the mod has not run (is winmm.dll there? did the game start through Steam?)" |

## Not shown in the game (log only), on purpose

- The overlay itself failing to install: there's nowhere to draw. The game runs; Diagnose Co-op+ shows
  "Overlay: installed false" from the log.
- Co-op+ loaded twice by two ASI loaders: the second copy stays off and the first one works normally (T28).
- Developer-only paths (UDP transport outside one PC, `[dev]` switches).
