# Draft: courtesy issue for nouzert1/cd-coop-3446 (NOT POSTED; needs the user's OK)

Title: Thank you, and a few patches you may want (Windows UTF-16 profiles, Steam join)

Hi! Thanks for porting cdmp to 1.0.0.3446 and for publishing it under the GPL. We are building a co-op mod for
friends on top of your port (credited to ByJunter and you in our README, credits file and package), and we'd like to
give back what we find:

0. **PointClimb / RideOn / RideOff Enter are 0x10 too high on 3446.** `kPointClimbEnter = 0x75F630`,
   `kRideOnEnter = 0x74E8E0`, `kRideOffEnter = 0x74F8B0` point into the middle of their functions, so the head checks
   in `puppet_install_climb_fix` fail ("PointClimb::Enter not recognised; wall climbs will freeze"). The 3446 vtables
   (`probe vtables ".?AVPointClimbMovementMethod@pa@@"` -> exe+0x59196A8, RideOn -> exe+0x5919810, RideOff ->
   exe+0x5919868) hold slot +0x38 = **0x75F620 / 0x74E8D0 / 0x74F8A0**, and the bytes there are exactly
   `kPointClimbEnterHead` / `kRideEnterHead`. (The comment's RideOn/RideOff vtable addresses 0x5917760 / 0x59177B8
   look like 3424 values.) We added the three to verify-port's prologue list.
   Also stale but failing closed: `game_errors.cpp` kErrorLogger 0x35D8D0 (byte 0x00 on 3446) and the aggro_test hooks.
   And `opcodes_resolved()` is defined inside the anonymous namespace in troc_sniffer.cpp, so it never links when called.
1. **UTF-16 profiles get corrupted on Windows checkouts.** `.gitattributes` has `kit/** text eol=crlf`, which also
   applies to `kit/coop/cdmp.ini.*` and `kit/public/cdmp.ini.*`. Those files are UTF-16 LE; on a Windows clone git
   inserts a single 0x0D byte before every LF, which shifts the UTF-16 code units and turns the profile into garbage
   (a package built from such a checkout ships unreadable cdmp.ini files). One line fixes it:
   `kit/**/cdmp.ini* -text` after the `kit/**` rule.
2. **Steam "Join Game", overlay invites and rich presence** without a server: registering for
   GameLobbyJoinRequested_t / GameRichPresenceJoinRequested_t through SteamAPI_RegisterCallback works because the
   exe pumps SteamAPI_RunCallbacks itself (classic dispatch; its import table shows RunCallbacks/RegisterCallback,
   no ManualDispatch), plus `+connect_lobby <id>` on the command line when Steam starts the game for a join.
   Happy to send it as a PR if you're interested.
3. Your offline verifier passes every check against the current Steam exe (PE timestamp 0x6AE46DD0), and the
   two-instance loopback test passes with MSVC 14.44.

No need to reply. Good luck with the two-PC test!
