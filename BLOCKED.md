# BLOCKED (roadmap tasks or parts that could not be done; what was tried, what would unblock)

## T10 (part) -- experience for the guest from kills on the host's enemies
The guest's loot from shared kills works (its own hidden enemy of the same kind is killed on its own server and
looted). Experience does not come with it.
Tried:
1. KillCheatReq 2210 with options 0, 1, 2, 3 on bandits (killopt1, 2026-11-03): option 0 kills at once (death type
   0x10), 1-3 after ~5.3 s (type 0x0F); the killer is our player each time, but no AddExperienceAck /
   AddSubLevelExperienceAck follows. A natural kill does give AddSubLevelExperienceAck.
2. VaryStatByAccInputReq 2565 (the HP stat path that works for our own player) on a bandit's HP (varykill1): no
   answer at all, no death, no experience.
3. AddExperienceByGmReq 2467 (the game's GM experience request): not sent -- its field layout is unknown and a
   wrong cheat request has logged the player out before (SpawnCharacterCheatReq, 2026-11-01).
What would unblock it: the field layout of AddExperienceByGmReq (its TrocTr handler's serializer in the exe), or a
way to make the guest's server count a real blow of the guest on its own hidden enemy (a replayed DoFrameEventReq
naming the hidden native; needs the hidden native's HP brought low first, which (2) could not do).
Also unknown: what AddSubLevelExperience feeds (weapon / skill sub-levels?) -- worth knowing before spending more.

## T16 (part) -- a shared bounty ("the host pays for the guest's crimes too", as an option)
Crimes are per player by design: each player commits them in their own world, the guest's blows only ever reach
copies of the host's ENEMIES (guest_hits.cpp: is_enemy_copy_key), so the host is never punished for the guest.
A shared option would have to put a crime into the OTHER player's server.
Tried / considered:
1. Capture of a crime (crime1, Hernand pub): Kliff did not draw a weapon in town, no crime was recorded; the test save
   already carries a wanted record (WantedRegionDataListAck at login: region 0x000F42C3 and a price).
2. Replaying the crime's own request (WantedAddCrimeRecordReq 2836) into the other server: it names witnesses and
   victims of the OTHER world (actor keys that do not exist there) -- refused or wrong.
3. The game's dev requests (SetWantedForDevReq 2212, SetWantedPriceForDevAck): field layouts unknown; a wrong cheat
   request has logged the player out before.
What would unblock it: one capture of a real crime (theft at a stall or an assault in town, witnessed) to read
WantedAddCrimeRecordReq / ChangeWantedStateAck, and the layout of SetWantedForDevReq.
Shipped instead: each player's bounty stays their own; the other player gets a notice when one becomes wanted.

### Lead for T10 found later (2026-11-03, T17 reading): the host side already turns a guest's lethal blow into a
NORMAL kill with skill XP and loot: VaryStatCheatReq (not VaryStatByAccInputReq) takes the enemy to 1 HP, then
send_collision() (AttackByCollisionReq of our player) kills it -- guest_hits.cpp "motive 8, skill XP, loot". The same
two requests on the guest's own hidden native (the loot twin) may give the guest XP; untested.

## T18 -- Trading and gifting
An item has to leave the giver's save and arrive in the receiver's: two different in-process servers. The giver's
half is a normal game action (discard); the receiver's half needs a request its server accepts that ADDS a chosen
item. None found yet.
Tried:
1. Field sizes of the item requests, read from their handlers in the exe (build/port3479/reqsizes.py):
   AddMinusItemReq [u32, 8], DiscardItemReq [u16, u16, 8], DeleteItemCheatReq [u16, u8],
   RemoveItemToInventoryReq [u8, u16, u8], CreateItemFromTrItemValueCheatReq [] (reads a structure through other
   calls, not plain fields).
2. AddMinusItemReq (u32 item | i64 count) +1 and -1 on item 22007 (9 of them in the bandits test save, addminus1):
   no answer at all, no inventory message, nothing changed.
3. The loot path that works for shared kills (the receiver's own hidden enemy dies and drops loot) cannot carry a
   chosen item: the drop is that enemy's own roll.
What would unblock it: the reader of CreateItemFromTrItemValueCheatReq (its structure), or a capture of any game
flow that hands the player a chosen item (a quest reward, a shop purchase: BuyItem* requests) to replay with the
gift's item. Duplication safety would then be: the giver's discard confirmed by its server BEFORE the receiver's
add is sent, one transaction id, a log of both sides.

## T21 -- the empty arm wrestling table
Objective check (build/port3479/table_check.sh: turn a full circle pressing R; usable = the client starts the
arm wrestling interaction, InteractionDoStepReq):
- no peer: usable (tcn1); peer connected but its copy never loaded ([dev] no_copy=1): usable (tcnc); peer started
  AFTER we arrived, its copy loaded next to us before the check: usable (tclate3);
- the copy present while we (and, in the one-PC echo, the copy with us) arrive at the pub: NOT usable (tc1-3, tco6,
  tcr1, tcu1, tcrs1) -- the game host never offers the game (no InteractionPreemptionStepReq for 0x000F431C).
Tried (all kept in, none sufficient alone):
1. Our server is no longer told that a copy entered a gameplay trigger / sequencer trigger volume (the copy entered
   trigger 0x000F4280 at the table 0.16 s before an NPC changed its AI) -- still unusable.
2. ... nor that a copy used a gimmick (it bumps the pub's stools and benches as it arrives) -- still unusable.
3. ... nor that a copy entered a region (RegionInOutReq: the copy's body entering the pub's regions) -- still unusable.
4. The copy made untargetable for every NPC ([dev] copy_untargetable) -- still unusable.
5. Workaround: a resync after arriving (copy unloaded and loaded again) -- still unusable for that session.
So the NPCs' own client-side AI (or the stage of the table) settles differently when a second player-type actor
arrives with us; once settled, it stays that way.
Caveat: in the echo the copy teleports in WITH us (the test reaches the pub by teleport hops). A real friend walks
in on their own; the two-PC playtest (PLAYTEST 6.7) says whether real play has it.
What would unblock it: the table stage's / game host's offer condition (the interaction's condition check), or a
way to have the copy loaded as a non-player actor near minigame tables.
