# cdmp (ByJunter) and the cd-coop-3446 port: what exists, read first-hand

Retrieved 2026-10-31 by cloning (read-only, outside the project folder: `/tmp/claude-0/ext/`). Confidence: **high** for
what the repos say about themselves; claims about live behaviour are the authors' own (not verified by us).

## Key takeaways
- **cdmp is a real, substantial co-op base**: C++20 ASI (`cdmp.asi` via Ultimate ASI Loader as `winmm.dll`), ~20k lines,
  unit tests, ImGui DX12 overlay, Steam transport, and ~70 dated research notes (Spanish, English index). **GPL-3.0.**
  Author stopped and explicitly invites forks ("Pull it apart, fix it, build on it").
- **The key technical insight**: Crimson Desert still contains the MMO (BDO-lineage) client/server split, talking over an
  unencrypted in-memory loopback of **~1,115 "Troc" message types** (byte packets, per-opcode handlers, RTTI-named).
  Hooking `ClientNet::PostPacket` / `ClientNet::OnRecvPacket` lets a mod read and inject messages. A second player is
  spawned the MMO way (`ActorLoadDataAck`) and driven by replaying their `MoveActor`/`StartAction`/`RotateActor` as Acks.
  This is the mechanism that makes "fully animated remote players" cheap: the game animates remote actors natively.
- **Status upstream (build 1.0.0.2976 = patch 2.03.02)**: player presence works (movement, climbing, gliding, combat
  actions, gear, rope); shared time of day; minimap/map marker; Steam friends-only P2P via `ISteamNetworkingMessages`
  resolved from the game's own `steam_api64.dll`, no callbacks (same approach as our D3). Experimental host-authoritative
  shared enemies. Horse broken. Quests/loot/bosses not shared.
- **cd-coop-3446 (nouzert1, GPL-3 fork)**: porting cdmp to 1.0.0.3446 (game auto-updated 2976 -> 3424 -> 3446). Last commit
  2026-10-31 01:10 -03:00. Steam connect verified live; position sync bug found and fixed in commit 270e2cc but not
  re-verified; shared loot implemented, not live-confirmed; bosses "half" (guest damages boss, but gets no quest/loot).
- **Weakness to fix**: cdmp hardcodes addresses for one build (refuses to run otherwise). Every patch means re-finding
  everything. The brief's signature-scanning design (and our data-driven probe) is the right fix.

## Hard-won facts worth keeping regardless of which code we use
- Loader is also injected into `crashpad_handler.exe`: filter by exe name (our `general.expected_exe` does this).
- Exe: `CrimsonDesert.exe` in `...\Crimson Desert\bin64\`. Steam AppID **3321460**. Saves: `%LOCALAPPDATA%\Pearl Abyss\CD\save`.
- "Steam and the game's protection do not allow two copies on the same PC": real tests need **two PCs and two Steam
  accounts that own the game**. Single-PC testing works with a relay/echo peer that plays your own character back.
- Game crashes are reported to Pearl Abyss by crashpad: use only function-entry inline hooks / SafetyHook mid-hooks,
  guard every read, no work in DllMain.
- Overlay: vtable patching of Present fails (Streamline calls Present directly); inline hooks on Present/Present1/
  ResizeBuffers(1) + ExecuteCommandLists work, installed late (after WorldSystem exists + window visible + 5 s).
- Remote actors are interpolated ~200 ms behind with no extrapolation; pausing stops the game clock (re-stamp samples).
- Actor keys are generation+index handles that change every load and wrap after 30-60 min: never use them (or pointers)
  as cross-PC ids. Sequencer / faction-node spawn data give stable identity for enemies.
- Enemy AI is simulated client-side; the "server" half computes damage. Enemies rarely pick the guest as a target
  (aggro lists favour the host); forced retargeting is short-lived. A 24-copy zone cap was hit.
- Bosses are per-player scripted stages driven by the client.
- Kills via injected stat/cheat requests give no XP/loot/progress; a collision-attack finishing blow gives full rewards.
- Loot lives on the corpse on the killer's server, so copies give nothing unless handled.
- Security: peer data must be validated (load templates, floats, counts); friends-only; rate limits.

## Licence consequences (GPL-3.0)
- Using any cdmp code (fork or copied files) makes our distributed mod a derivative: **it must be GPL-3 and ship/offer
  source**. Our own code can be dual/relicensed by us. Third-party deps they use: Ultimate ASI Loader (MIT), Dear ImGui
  (MIT), SafetyHook (BSL-1.0), Zydis/Zycore (MIT) - all GPL-compatible.
- Reading their research and re-implementing ideas clean-room is allowed under any licence, but is slower.
- cdmp itself avoided copying an unlicensed earlier attempt (blizz3010/CrimsonDesertCoop); we must do the same.

## Sources
- https://github.com/ByJunter/CrimsonDesertMultiplayer (commit b62a572, 2026-10-10): README, docs/ROADMAP.md,
  docs/research/README.md, CONTRIBUTING.md
- https://github.com/nouzert1/cd-coop-3446 (HEAD 2026-10-31): README
- Nexus: https://www.nexusmods.com/crimsondesert/mods/3677 (not reachable from this session)

## cd-coop-3446 layout (checked 2026-10-31, HEAD b7ae133)
- ~24.4k lines: `src/common` (protocol, reliable channel, sessions, UDP, enemy tables for 2976, pattern scanner),
  `src/mod/game` (player/camera readers, actor lookup, RTTI, safe memory, **per-build offset headers
  `offsets_3424.h`, `offsets_3446.h`**), `src/mod/net` (40 files: capture, puppet, coop, native_suppress, time_sync,
  minimap, aggro/attack probes), `src/mod/render` (overlay).
- Still per-build hardcoded offsets: each patch needs a new header. Ours would replace these with signatures + probe.
- New docs since upstream: port notes for 3424/3446, a hang diagnosis, "why bosses don't finish", "enemy relay never
  connected", and a two-PC test kit dated today.
