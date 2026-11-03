# Co-op+ and other mods

Last reviewed: 2026-11-03, game 1.0.0.3479 (2.04.05). The mod list comes from `NOTES/RESEARCH/nexus-survey.md`.
Nexus "most downloaded" rankings could not be read, so the list covers the mods that kept coming up in that survey.
Nothing here was tested with the real third-party files: we don't download or bundle other authors' mods. The rows
are what follows from how each kind of mod loads. Section 3 says what was tested on this PC.

## 1. What Co-op+ checks at launch (T28)

When the game starts, Co-op+ looks in `bin64` (the folder with `CrimsonDesert.exe`) for:

| Found | Why it matters | What Co-op+ does |
|---|---|---|
| `version.dll`, `dinput8.dll`, `dsound.dll`, `winhttp.dll`, `xinput1_3.dll`, `d3d9.dll`, `d3d11.dll`, `wininet.dll`, `msacm32.dll` | Usually a second copy of Ultimate ASI Loader under another name. Co-op+ ships its own as `winmm.dll`, and two loaders load every `.asi` twice | Warning in `cdmp.log` and an on-screen notice once in the world. If Co-op+ really is loaded twice, the second copy stays off (log: "Co-op+ was loaded twice") |
| `dxgi.dll`, `ReShade64.dll`, `ReShade.ini` | ReShade (or another dxgi proxy). It also hooks the game's drawing, like Co-op+'s panel | Warning and notice only |
| any other `*.asi` | Another native mod, loaded by our `winmm.dll` | Warning and notice only, naming the files |

The check never stops the game or co-op. It also never sends anything anywhere. If co-op misbehaves, the notice's
advice is to take the listed files out first and try again.

## 2. By kind of mod

| Kind | Examples (Nexus id) | Works with Co-op+? | Notes |
|---|---|---|---|
| Data patches (JSON / overlay PAZ through DMM or CDUMM) | Fat Stacks (157), Inventory Expander (56), Infinite Stamina (107), Free Gliding (353), Barber Unlocked (591 / 3528), HUD layouts | Yes, loads separately | **Gameplay patches must be the same on both PCs** (stamina, stacks, cooldowns, loot, inventory caps). If they differ, each player's game follows its own rules and things drift. Cosmetic ones (barber, HUD, map icons) only change your own screen |
| Mod managers | Definitive Mod Manager (633), CDUMM (207) | Yes | They write overlay archives and leave `bin64` alone. Install Co-op+ with its own installer, or as a manual install (see the README) |
| ASI mods loaded by our `winmm.dll` | Outfit Loadouts (3767), Live Transmog (1056), Master Looter (3402), CDAutoLoot (3380), Ultra Parry and Dodge (607 / 2987), DesertLink mods, Crimson FastLoad (3768), Stack Master (3548), teleporters (713) | Usually, but untested | They load next to Co-op+ in the same game. Ones that hook the same game functions (actors, loot, combat, teleport) can clash. Ones that change gameplay must match on both PCs. The partner sees only what goes over the co-op link: a transmog look is not sent, so your friend sees your real armour |
| Another ASI loader | `version.dll` / `dinput8.dll` copies of Ultimate ASI Loader, CDSmoothmove's `version.dll` | Remove the extra loader | Keep one loader. Co-op+'s `winmm.dll` already loads every `.asi` in `bin64`. A mod that ships its own `version.dll` loader: drop that file and keep the mod's `.asi` |
| ReShade presets | Visual Redux (12), other ReShade presets | Expected to work, untested | Both hook the frame. Co-op+'s panel and HUD draw on top. ReShade add-on GUIs (Live Transmog) open their own overlay. If the Co-op+ panel is missing, try with ReShade off |
| Trainers and Cheat Engine tables | Trinity, CE tables (64, 3209) | Not supported | They write game memory Co-op+ also reads (health, position). Don't use them during co-op |
| Character Creator (837) | data + ASI part | Expected to work | Your own look. The partner copy is built from the equipment sent over the link, not from Character Creator data |

## 3. Tested on this PC (2026-11-03, echo stand-in)

- With a stand-in `ReShade.ini` next to the game, the log and notice name it (`build/port3479/compat_test.sh`, run a).
- Without it, the log says "no other ASI loader, .asi mod or ReShade next to the game" (run b).
- A double load can't happen here (one loader). The guard is a per-process named mutex, so a second copy of `cdmp.asi`
  in the same game returns before installing anything.
- Testing this found a real crash, now fixed (T28 in `PROGRESS.md`). On a launch where a crash-guard hook failed the
  first time, the retry wrote into its read-only return stub.

## 4. To test with real mods (2-PC playtest)

`docs/tests/PLAYTEST-1.md`, section "Other mods". Each player installs the mods they normally use, then:
1. starts the game and reads the notice;
2. plays 10 minutes of co-op with a fight and loot;
3. sends both `cdmp.log` files if anything is off.
