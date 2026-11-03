# Final report: roadmap run, 2026-11-03

Build installed in your game: **0.1.0-alpha+7a880b02** (cdmp.asi sha256 starts 076CAE3973B55312). Your saves equal
the 2026-11-01 16:04 backup (savecheck: SAVES OK). The test save is out. bin64\cdmp.ini is the shipped one. The 372
packet-capture files my tests left in bin64 are moved to `build/port3479/packet-archive` (nothing deleted).

## Where every task stands (ROADMAP.md)

| Status | Count | Tasks |
|---|---|---|
| [x] done | 21 | T01-T04, T06-T08, T11, T12, T14, T20, T24, T29-T32, T34, T37-T40 |
| [~] built, needs the two-PC test | 20 | T05, T09, T10, T13, T15-T17, T19, T22, T25-T28, T33, T35, T36, T41-T44 |
| [!] blocked (BLOCKED.md) | 4 | T18 trading, T21 empty arm wrestling table, T45 shared stash, T46 baby wyvern |
| [ ] not started | 3 | T23 doors / levers (plan in PROGRESS.md), T47 voice chat, T48 3-4 players (reasons in PROGRESS.md) |

Every [~] task has numbered steps in `docs/tests/PLAYTEST-1.md`.

## Your requests from 18:00
1. **Controls card with controller buttons.** It shows the buttons of whatever pad is plugged in, even if you
   haven't touched it yet. It opens any time with Shift+F1, or on a pad by holding View/Touchpad and clicking the left
   stick. The F8 panel now says so. Checked with a simulated Xbox pad.
2. **Discord distribution.** Decision: GitHub Releases on a separate public repo that holds only the downloads
   (`crimson-desert-coop-playtest`). Your project repo stays private.
   - Why: one link that is always the newest build (`/releases/latest`), version history with notes, no Discord
     upload limit, and the GPL source travels inside each zip.
   - source.zip now holds only what builds the mod (it used to be the whole repo, notes and all).
   - **One step is left for you:** creating the public repo was blocked for me, because it publishes under your
     GitHub name. Run this once from the project folder:
     `powershell -ExecutionPolicy Bypass -File tools\publish-playtest.ps1`
     It creates the repo with its README (install steps) and FEATURES.md, then uploads the player zip and the
     mod-manager zip. It never uploads the symbols zip. Run it again for each new build.
   - **Paste into Discord:** `docs/release/discord-install.txt` (one message, about 1600 characters, pin it).
3. **Feature list:** `docs/release/FEATURES.md` for Reddit (tables), and `docs/release/discord-features.txt`
   (three messages, each under Discord's 2000-character limit). Each feature is marked **Works** (checked in game
   against a stand-in), **Needs testing** (needs two real players) or **Not yet**.

## Fixed today (found while testing)
- **Crash on a rare launch:** when a crash-guard hook needed a second try (about 1 launch in 80), the retry wrote into
  a read-only page and the game crashed (T28).
- **False "in a menu" flag:** since T09, cutscene, duel and quest events also ran the "partner is in a menu" code.
  The partner could become untargetable mid-duel (T39).
- **Packet loss stuck at 0** after a reconnect (T32).
- **Diagnose leaks:** the redaction first let SteamIDs, room codes and IPs through (a script escaping bug), and it
  removed the word "hosting". The test caught both (T37).

## Still open
- The two-PC playtest: `docs/tests/PLAYTEST-1.md`, which is now also the testers' checklist.
- Nexus page and screenshots: paused by you. The page text and shot list stay in `docs/release/`.
- On this PC the game's audio reaches Windows' loopback almost silent (about −77 dBFS). Worth checking your game
  volume or output device if you ever expect sound in a recording.
