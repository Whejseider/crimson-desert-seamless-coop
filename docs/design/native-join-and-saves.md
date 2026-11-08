# Native shared world: joining from the main menu, and where co-op progress is saved (B01 design, 2026-11-07)

Context: the big update's B01 puts Co-op+ on ByJunter's native base (the guest plays IN the host's world).
User 2026-11-07: joining only from the main menu is fine. Facts this design rests on (PROGRESS 2026-11-07 19:30):

- Two DLLs: `cdmp.asi` (native base) and `coopplus.asi` (Co-op+ layer, `-DCDMP_NATIVE_LAYER=ON`).
- The native role is a **process** role, read from `cdmp.ini` at start: host stages (`[session2] accept_tunnel`...)
  and guest stages (`guest=1 guest_mode=remote`) cannot run together.
- The guest's client connects to a server **at the title screen** (~30 s after launch, before any menu choice):
  its first packet (ConnectCryptogramToServerReq 2493) decides remote (the host's world) or local, for the whole
  run until it goes back to the title.
- The native host never writes the real save folder: `save_guard=redirect` plays in a sandbox copy made at every
  start (`%LOCALAPPDATA%\Pearl Abyss\CD\cdmp_save_sandbox`, the previous one kept as `_prev`).
- The guest (`guest_own_save=1`) uploads its own slot; the host's server loads and saves it as slot 204 in the
  sandbox and sends the results back to the guest's redirected folder (`save_cdmp_guest`).

## Modes (what `cdmp.ini` says at launch)

| Mode | Native base | Saves |
|---|---|---|
| **Solo** (default; every launch after a session ends) | session2 off: nothing hooked | the player's own, untouched |
| **Host** | host stages, Steam tunnel accepting | a co-op save of its own for that friend (below) |
| **Guest** | guest stages, remote, tunnel to the host's SteamID | the guest's co-op save for that friend |

`coopplus.asi` writes the mode. It never edits the player's other settings: the native keys live in a section block
Co-op+ owns, and Solo removes them again.

## The flow (title screen only)

1. Both start the game (Solo mode). Co-op+'s title menu (coop_lobby.h): Invite Friends / Join through Steam or an
   invite code, as today; the two layers link over their own channel (no world shared yet).
2. The host presses **Start Co-op** and picks the journey (Continue our co-op save / From my solo progress / New).
3. Both layers write their native mode (host: Host + the friend's SteamID + the co-op save; guest: Guest + the
   host's SteamID + its co-op save) and the Co-op+ link keys (`+connect_lobby` / the invite), show a card "The game
   restarts to start co-op (about 30 s)", and restart: a detached helper waits for this process to end, then runs
   `steam://rungameid/3321460`.
4. After the restart the host's game loads the co-op save at the title (Continue, driven by the lobby as today);
   the guest's client connects to the host's world at its title (the tunnel is up because the host restarted
   first; the guest's restart waits for the host's "ready" over the Co-op+ link) and Continue logs it in.
5. Leaving (either player, or the session ends): the layer writes Solo for the next launch. The run itself goes on
   until the player quits to the title, where a card offers "Restart in solo".

Later (not in the first version): hold the guest client's first connection while the Co-op+ title menu is up, so
joining needs no restart.

## Co-op saves on the native base

- Host: the redirect's sandbox becomes `...\CD\coop_worlds\<friend SteamID>` and is **kept** between runs
  (`[session2] sandbox_dir`, `sandbox_seed=keep|copy|new`):
  - Continue our co-op save: `keep` (the folder as it was left);
  - From my solo progress: `copy` (today's behaviour: a copy of the real save folder, the old co-op folder set
    aside as `<dir>_old_<date>`);
  - New co-op save: `new` (an empty account folder: the game starts New Game).
- Guest: its redirected folder becomes `...\CD\coop_guest\<host SteamID>` (one per host), kept.
- The real save folder is never written by either role (unchanged rule).

## Steps

1. Native: `sandbox_dir` + `sandbox_seed` (save_hooks.cpp), guest folder per host (role save dir), unit tests.
2. Layer: `native_mode.{h,cpp}`: write / clear the mode block in cdmp.ini; the restart helper.
3. Layer: the lobby's Start Co-op drives it (both sides), the "restarts" card, the guest waits for the host.
4. Harness: a run that starts both in Solo, links them, presses Start Co-op, and checks both restart into the
   shared world; then Leave -> Solo next launch.
5. Package: both DLLs in the zip, installer, uninstaller, docs.
