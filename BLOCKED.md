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
