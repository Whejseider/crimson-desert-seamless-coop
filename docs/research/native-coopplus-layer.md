# Co-op+ on the native base: the layer move (plan, 2026-11-06)

Step 4 of docs/research/native-port-plan.md. The native base (ByJunter's NativeServer, ported to 1.0.0.3549 in
upstream/cdmp-native, branch coopplus-3549) now gives a shared world on one PC with two copies: join with the
guest's own save, spawn together, movement both ways, the other player visible, shared combat (same enemy HP and
kills), menus that never pause the world, guest scene routing, the observer milestone. What remains is moving the
Co-op+ layer (upstream/cd-coop-3446/src/mod) on top of it.

## Recommended shape: one repo, two DLLs

- `cdmp.asi` = the native base (the shared world). Unchanged role.
- `coopplus.asi` = the Co-op+ layer built from upstream/cd-coop-3446 with a new `[coop] native=1` mode. In that mode
  every system that SIMULATES a shared world is off (the native server does it for real), and every system that
  ADDS to the game stays on. Its own Steam channel and its own ini (coopplus.ini) so the two never share a key.
- Why two DLLs and not one merge first: the Co-op+ code is 23k lines of net/ plus UI, built and tested on the old
  base; a merge into session2's 222 files would re-test everything at once. Two DLLs let each feature be switched
  on and verified one by one in the two-copy harness, and the shipping Co-op+ (old base) keeps working unchanged
  until the native build is ready. A later merge into one DLL stays possible.
- Conflicts to avoid: only one Present hook (the native overlay stays off: `[overlay] enabled=0`); Co-op+ must not
  hook the packet functions the native base hooks (its sniffer, enemy relay and capture are off in native mode);
  separate ini and log files.

## What the native world makes unnecessary (off in native mode)

remote_puppet, puppet_minimap, puppet_visibility (the other player is a real actor now), enemy_relay,
native_suppress, guest_hits, player_hits, aggro_test, attack_probe (combat is the server's), loot_twin (loot is the
server's; kill_loot_per_user in the native base), local_capture, troc_sniffer, load_trace, piece4_test,
dev_memscan / dev_script / dev_watch (dev only), quest_live, quest_advance, story_sync (one world: the guest's own
save keeps its own progress through guest_own_save), coop_worlds (the shared world IS the host's), horse_diag (the
native one is used), save_guard (the native save redirect protects the saves), time_sync / weather_sync (the
native clock gate; weather to be decided: the native weather_sync is not ported yet, see PROGRESS).

## What moves (on in native mode, each verified in the harness)

| Group | Files | Needs from the world |
|---|---|---|
| UI | render/hud, render/game_ui, render/dx12_hook, settings_menu, input/* | the local player (old-base readers still valid on 3549) |
| Party frame, nameplates, colours | hud + coop_sync | the partner's state: from its own channel, or read from the partner's native actor |
| Chat, emotes | coop_chat, coop_activity | its own channel |
| Challenges | coop_duel, coop_race, coop_fishing, coop_hunt, coop_hideseek, scoreboard, session_summary | its own channel; duel damage needs a review (the server decides hits now) |
| Revive | coop_revive, revive_prompt | the partner's death state (now native) |
| Kill feed, boss rules, scaling | kill_feed, boss_rules, boss_safe, party_scaling | native kills (VaryHPAck / death) instead of relayed ones |
| Party journal | party_quests, quest_events | its own channel |
| Travel / summon | coop_travel | the native fast travel (fast_travel ported, user 2's server funnel not yet) |
| Joining | steam_join, room_code, join_args | must also start the native session (peer address, roles): the biggest design item |
| Safety | optin, online_guard, crash_log, save_io | unchanged |
| Slow motion block | slowmo | likely replaced by ui_pause (native) |

## Order

1. Native mode switch in Co-op+ (config key, the off-list above, coopplus.ini, coopplus.asi target).
2. Load both DLLs in the two-copy harness; Co-op+ UI on both copies; nothing else on.
3. Party frame + chat over its own channel; then challenges one by one; then revive / kill feed / boss rules.
4. Joining: one flow that starts both (room code -> native peer settings).
5. Then the remaining native modules (user2_stages, weather, user 2's fast-travel funnel, horse_follow layout,
   guest_seq_play, docked gimmicks / rope / tree bend) as the features need them.
