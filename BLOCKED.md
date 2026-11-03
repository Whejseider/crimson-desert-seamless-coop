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
