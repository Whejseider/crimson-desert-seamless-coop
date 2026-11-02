# Crimson Desert features, from the web (research 2026-11-02)

Sources read (via fetch): Fextralife wiki (Minigames, Duo, Shot Contest: Bow, Arm Wrestling, Rock Paper Scissors,
Mounts partly), crimsondesertwiki.net mount guide (pre-launch, March 2026, many "TBD"), Steam guide "All Mounts"
(March 2026, flagged removed by Steam), Wikipedia (Crimson_Desert). Refused: crimsondesert.gaming.tools (403),
thegameswiki.com (429), fandom (402), Epic store guide (403). Search snippets only from gamerant, beebom, games.gg,
thegamer etc. Treat details as community-sourced; verify in game data (docs/research/game-features-troc.md).

## Facts that matter for co-op
- Single-player only by design; Pearl Abyss built it on the Black Desert Online engine (BlackSpace) -> client/server
  message architecture (what this mod uses). Released 2026-03-19. Player character: Kliff (Greymanes); Damiane joins
  in Chapter 3 with her own horse breed; Oongka also playable in some parts? ("each playable character's moveset").
- DLC "Charting the Unknown" (announced 2026-09-03, slated 2026-10-29): ship travel beyond Pywel with Oongka and
  Damiane, mechs, naval combat, underwater exploration, expanded base-building. If installed, the exe would change
  (our build is 1.0.0.3479; check the game version on each launch).

## Minigames (Fextralife, edited 2026-05-01) -- all 1v1 vs an NPC unless noted
| Name | Where | Play | Fee / reward | Notes |
|---|---|---|---|---|
| Rock Paper Scissors | City of Hernand, east of Provisioner's Shop (a child) | best of 3 | free; random items, Trust +5 | 23 h cooldown |
| Arm Wrestling | Hernand Inn (main quest "Where Rumors Gather" introduces it) | mash Square/X to push the gauge | copper/silver; replay ~20 alms | challenge "Hero's Handshake I": 3 wins in a row + champion |
| Duo (card game, "seotda" in file names) | Hernand Inn 2nd floor; Beighen west of the Grindstone | 20 cards 1-10, pot: Check/All-in/Half-Raise/Double-Raise/Call/Fold, 10 s auto-Call | 15 silver to sit; consecutive wins raise rewards | MULTI-SEAT table (1-3 opponents) -> natural co-op/PvP fit. Challenge "A Bloom of High Stakes" |
| Shot Contest: Bow | Lioncrest Manor, Hernand | hold to draw, release; first to 10 targets | 80 copper | |
| Shot Contest: Gun | City of Hernand | first to 10 random targets | 1 silver | |
| Unarmed Duel: Boxing | Goldenfist Arena, Hernand | bare hands; other attacks = DQ; deplete HP | 30 copper | |
| Weapon Duel: Spear | City of Hernand | spear only; deplete HP | 45 copper | |
| Wrestling Match | Kharonso, Hernand | grappling only; deplete HP | 1 silver | |
Also mentioned without pages: "sparring with a Wandering Freesword", "All-Out Duel", "Hand Wrestling",
"Spearsmanship Contest", horse racing ("ride a horse in a racing game"; a temporary "Race Horse" mount exists),
gambling with tavern goers. No fishing contest or dice game found.

## Mounts
- Horses (tamed by a balance minigame; stats Speed/Stamina/Strength/Health/Attack/Defense; trust 1-5 by riding,
  petting, feeding; gear: saddle, horseshoes, stirrups, champron, barding at Saddlery; stables register/retrieve/heal):
  Herspia (first horse), Brianto (Damiane's breed), Priden, Numont (wild), legendary Royler (white, Pailune),
  Rokade (black, Grace Estate), Camora (red, Redtree Forest). "Race Horse" (temporary).
- Combat mounts: Brown/Black Bear (stagger then mount; temporary unless quest-tamed), Direwolf, Raptor (unverified),
  Lion, Cow, Red Dodo, Giant Iguana (listed, no details).
- Flying: Blackstar Dragon (end of Chapter 11, fire from the air; one search snippet: 15 min ride, 50 min cooldown,
  no riding over populated areas -- unverified), "Wyvern" (one guide; maybe the same creature), Cloudcart (hot-air
  balloon; crafted via Emberwind Workshop + Engineer at camp, dispatch mission).
- Vehicles: ATAG mech (Chapter 10 siege), "War Robot" (maybe same), Pywel Wagon (steal, or camp + Brice the Wagon
  Master + dispatch), Delesyia Train (board, not owned), Skiff (river/coast boat). Gliding exists (not a mount).
- No source mentions a passenger seat / two riders.

## Other systems (search snippets; to verify)
- Life skills: fishing (4 rods, 5 legendary fish, no bait), cooking (buffs; recipes from quests/NPCs), crafting,
  smithing/upgrades, gathering (ore, timber, hide, plants), hunting, farming, ranching, logging, mining.
- Housing: hand-placed furniture; kitchen enables cooking, workshops crafting. Storage box added post-launch.
- Greymane Camp at Howling Hill (from Chapter 3): upgrades unlock crafting, combat training, fast travel, story;
  comrades (recruited via faction quests and Marinus's rumors) with skills (Escort, Farmer, Rancher, Fisherman, Smith,
  Logger, Miner; Builder, Engineer, Craftsman) and dispatch missions.
- Factions: Greymanes (Kliff, Oongka, Yann, Naira), Black Bears (Myurdin), House Celeste, House of Wells.
- Combat: combos, grappling, elemental weapons, Abyss powers, horseback combat, large bosses (dragons). Skill tree
  gives new attacks, not stats. Lock-on. Force Palm (healing variant heals horses).
- World: day/night, weather, crime/witness/wanted (seen in packets: ResetNearWitnessCrimeCountAck, "Wanted" UI),
  trust with NPCs, Abyss Nexus fast travel, puzzles, collectibles (Abyss artifacts), photo mode.
