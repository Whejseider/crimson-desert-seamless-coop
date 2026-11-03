# Online integrity, anti-cheat and terms (roadmap T02, 2026-11-03)

## What the game has (Steam PC, 1.0.0.3479 / game version 2.04.05)
- **No anti-cheat.** No EasyAntiCheat, BattlEye, XIGNCODE, Denuvo Anti-Cheat or similar module is loaded and none of
  their files are in the game folder (bin64 listing checked). Denuvo **Anti-Tamper** is DRM: it protects the exe and
  checks the licence (Steam page: "5 different PC within a day machine activation limit"); it does not scan for cheats.
  Sources: PCGamesN and Insider Gaming on the Steam page's Denuvo note (March 2026).
- **No online play of its own.** No co-op, PvP or matchmaking; Pearl Abyss said online modes are not in development.
  The "server" the client talks to (the messages Co-op+ reads and injects) is the **in-process server of the
  single-player game** (NOTES/DECISIONS D42, docs/progression-sync.md), not a Pearl Abyss server.
- **Online pieces that exist:** Steam (ownership, overlay, achievements, cloud); Denuvo's licence check; **cross-save**
  (patch 1.14, July 2026) through a linked Pearl Abyss account, which carries the save between platforms; crash
  reports (sentry.dll / crashpad_handler.exe) and analytics (librdkafka.dll) the game sends itself.
- Minigame leaderboard messages exist (UpdateMiniGameLeaderboardDataReq 2300 / PlayerNameReq 2265) but none was sent
  in ~200 recorded sessions, arm wrestling matches included.

## Risks and what Co-op+ does about them
| Risk | Co-op+ |
|---|---|
| An update adds an anti-cheat | `online_guard` checks at every start for anti-cheat modules in the game and anti-cheat files in the game folder; if found, **nothing** of Co-op+ is installed (no overlay, no hooks, no co-op) and a message says so. Tested with a simulated module ([dev] online_guard_test). |
| A game update changes the exe | Co-op+ only runs on the exact build it was made for (already: "Co-op+ is off: this mod was made for game version ..."). |
| A co-op-influenced save reaches cross-save / Steam Cloud | Co-op+ never saves; the README rule is "nobody saves while connected"; the save guard backs saves up at session start and warns on the panel if the game saved during co-op. Session state is not written to the save (each player's world is their own). |
| Minigame results influenced by a versus reach a leaderboard | The versus only ever plays local matches; no leaderboard message was seen. Logged if one ever appears during a challenge. |
| Telemetry of our own | None. Co-op+ sends nothing anywhere except to the friend you play with (Steam networking or your room code). |

## Terms
Pearl Abyss has published no modding policy for Crimson Desert (no mod support at launch). Like every other mod on
Nexus for this game, Co-op+ modifies the local single-player game only, ships no game files and touches no account,
store or online feature. Safe install path: the one-click installer (adds winmm.dll, cdmp.asi, cdmp.ini to bin64 and
backs up saves); uninstall removes exactly those; "Vanilla mode on-off" turns Co-op+ off without uninstalling.
