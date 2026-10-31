# Building Co-op+ on cd-coop-3446 (plan, 2026-10-31)

Decision D10: Co-op+ builds on ByJunter's cdmp as ported by nouzert1 (`upstream/cd-coop-3446`, GPL-3.0).

## What upstream already gives us (author-reported, build 3446 unless noted)
| Brief feature | Upstream state | Our work |
|---|---|---|
| 6.1 Steam join | Steam P2P (`ISteamNetworkingMessages`), friends-only, friend auto-discovery; connect verified live | **Steam lobbies**: overlay/friends "Join Game", invites, rich presence `connect`; Co-op menu (Host/Invite/Party/Leave/Settings); friendly version messages; reconnect; 2 -> 4 players |
| 6.2 Presence | Native remote player via loopback (`ActorLoadDataAck` + relayed Move/Action/Rotate): run, climb, glide, combat, gear, rope; minimap/map marker; ImGui overlay marker. Position bug fixed in 270e2cc, not yet re-verified | Re-verify; nameplates with distance fade + occlusion + toggle; pings/waypoints; party HUD; emotes (optional) |
| 6.3 Progression | Each player in own save; host's quests protected; boss "half" | Shared quest/discovery/map flags (dev quest commands lead, coop-anything #51); separate co-op save slot + backups (`sim/progression`) |
| 6.4 Loot | Shared loot implemented, not live-confirmed | Per-player instanced loot, currency/quest to all, other modes (`sim/loot`) |
| 6.5 Scaling | None (boss rules only) | Party-size health/damage/stagger scaling from `config/scaling.toml` (`sim/scaling`), debug overlay |
| 6.6 Customization | Gear sync | Appearance sync, Character Creator/Barber compatibility, fallback look |
| 6.7 Polish | ImGui dev overlay, ini config | Game-style UI, TOML settings screen, one-click vanilla mode |
| Robustness | Per-build hardcoded `offsets_<build>.h`; refuses unknown builds | Signature scanning + `probe.toml` so a patch needs data, not a rebuild |

## Steps
1. **M0 on the current build** (needs GO + the PC): build upstream `cdmp.asi` with MSVC on the PC as-is, run its
   own single-PC relay test (`cdmp_peer --mode relay`), and our probe. Record the exe build, which upstream offsets still
   hold, and whether Denuvo/Themida interfere with loading (stop and report if they block).
2. **One DLL**: a top-level Windows target that compiles upstream `src/mod` + `src/common` together with our
   `cdcoop_core` (sim/, config, sigscan). Upstream `main`/startup gains calls into our layers; changes to upstream files
   are kept small and listed in `upstream/README.md`. Output: `CrimsonDesertCoop.asi`, config in `cdcoop/config/*.toml`
   (upstream ini keys mapped into TOML).
3. **Signatures**: turn `offsets_3446.h` entries into AOB signatures (from the 3446 binary on the PC), verify on the
   current patch, fall back to the per-build table if a signature fails.
4. **M1 lobby**: Steam lobby create/join/invite + rich presence over the flat API (same no-SDK approach as upstream),
   keeping upstream's transport and protocol versioning.
5. Then M2-M7 as in `docs/architecture.md`, starting with re-verifying presence on two PCs.

## Licence and credit
- Whole project GPL-3.0 (LICENSE). Source must be published with every release (it is: this repo).
- Credit ByJunter and nouzert1 in README, CREDITS.md, the Nexus page and the in-game About panel.
- Courtesy contact (not required by GPL): tell nouzert1 we're building on the port and offer our patches back.
