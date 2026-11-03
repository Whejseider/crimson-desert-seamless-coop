# Crimson Desert progression (research notes, 2026-11-02 night)

Sources: Game8 (https://game8.co/games/Crimson-Desert) pages 582136 story, 586940 early progression, 585760 map,
602056 wyvern, 587115 Blackstar, 585471 skills, 585839 fast travel; plus method.gg / allthings.how / nerdschalk /
mapmaster pages read earlier (NOTES/LOG.md 2026-11-02 18:25). Fextralife/thegameswiki were unreachable. Game data
facts (keys) are from build/ext-save/*.json and live memory.

## Story
- Prologue "Dead of Night" + 12 chapters: 1 The First Encounter, 2 Golden Greed, 3 Howling Hill, 4 The Price of
  Knowledge, 5 Guest Unbidden, 6 Cracks in the Shield, 7 Homecoming (Pailune quests), 8 Blood Coronation ("Demeniss
  Bound"), 9 The Sage of the Desert, 10 Counterattack, 11 Truth and Reality, 12 The Abyss. Final chapter ~60+ h.
- Main quests = orange markers, faction/side = silver. Some quests need time to pass ("Pass Time", or rest days).
- Our test save (COOP TEST bandits) and all of the user's saves are early (Hernand); the game will not load a start
  position in Delesyia for them and hops stop at region borders (2026-11-02): regions look story-gated.

## Map and fast travel
- Regions: Hernand, Pailune, Demeniss, Delesyia, Crimson Desert (+ DLC Samora Islands).
- Abyss Nexus: unlocked by stepping on it. Abyss Cresset: gives an Abyss Artifact + puzzle; solved = unlocked.
  Fast travel between unlocked Nexus/Cresset points (not while mounted). "Mysterious Energy" ? icons mark them.
- Map discovery = knowledge entries (Knowledge_Node_<Region>_<Place>, knowledge_keys_all.json); a save's
  KnowledgeSaveData list holds them (tools/save/knowledge_swap.py can add one by replacing another).

## Skills
- Per character (Kliff, Damiane, Oongka). Unlocks: by default / prerequisite level / 1 Abyss Artifact / by WATCHING
  someone use it (Force Palm from a small NPC; Focused Force Palm watching a figure in Ch4 on the way to Scholastone).
- Ch1 "Traces": Force Palm, Axiom Force, Flight (glide: jump while airborne). Ch2: Focus. Prologue: Blinding Flash.
- Abyss Artifacts also raise Stamina (max 14), Spirit (12), Health (18).
- Grappling default (Lv5); Winch needs Grappling + Axiom Force Lv3 + 1 artifact.

## Mounts
- Blackstar dragon: end of Chapter 11 (KNOWLEDGE Knowledge_CallDragon 1000175; Riding_Dragon_1 1000799). Our
  dragon test save summons it but Kliff cannot board (missing story/skill state).
- Wild wyvern (Patch 1.08): Fort Windridge, south Delesyia (game: "Del WindCliffFort" -6649, 502, -5519). Shoot it
  down with bow/firearm (no elemental/long-range skills), mount it while stunned; temporary (hostile again after
  dismount). No stamina cost, Windshot attack.
- Baby wyvern pet (1.08): egg in Wyvern's Cradle cave on Three Brothers' Cliff (SW Delesyia; game "Del
  AbandonedWyvernNest" -6736, 548, -5330); Focused Force Palm opens the bismuth wall; place the egg in the working nest
  SW of the cave, wait (minutes real time / ~1 h game time, don't leave). Game8 (May 2026): "will be able to grow and
  be registered as a permanent mount in a future update". The user's search says it grows over 3 in-game days with
  feeding. Keys: Item_Egg_Wyvern 1004389, Animal_Baby_Wyvern_1 1004045, Riding_Wyvern_1000 1004233,
  Knowledge_Wyvern 1001881.
- Horses: Royler, Rokade, Camora, Herspia, Brianto, Priden, Elantrima. Elephant = temporary mount. DLC sea mounts.

## Minigames, knowledge, trust
- Challenge categories: Exploration, Mastery, Combat, Life, Minigame (Duel, Mind Games).
- Minigames: arm wrestling (Knowledge_Minigame_Armwrest 1002833), hand wrestling 1001469, RPS 1002618 (+ per town),
  archery contest 1001651, firearm contest 1001927, duels (unarmed 1003658, all-out 1003785, wrestling 1003786,
  spear 1003787, sword&shield 1003788), horse racing 1003789, Duo 1003790, Five-Card 1003791, Banker's Dice 1003792,
  slapping 1003794. The Hernand RPS boy plays "once in a while" (server-side gate; a fresh load resets it).
- Trust: NPCs and pets; petting +25/day for pets; wild animals become pets by trust.

## Quests / factions / camp
- Factions per region (Greymanes, Hernand houses, Demeniss, Delesyia: Redfox Merchants, Aeserion; Pailune tribes;
  Crimson Desert). Side quests/bounties earn Contribution Points (vendor currency); crimes cost them.
- Greymane camp features unlock with Greymane quests; Greymanes can be sent on resource missions.
- Save: MissionStateData (_uiPosition objectives), quest lists (UpdateQuestListAck / UpdateMissionListAck at login).

## Gaps (to read): fog of war 587606, region gating, trust 586801, baby wyvern 602044, summon mount 587055,
## archery contest 588474, unarmed duel 588743, arm wrestling 587657, leveling 587044.
