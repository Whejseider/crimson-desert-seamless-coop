# Crimson Desert Seamless Co-op+ (playtest)

An unofficial fan mod that adds two-player co-op to **Crimson Desert** (Steam). Each player keeps their own save;
your friend joins your world as their own character. It is built on Crimson Desert Multiplayer by ByJunter and its
port by nouzert1. GPL-3.0, and the source of each build is inside its zip (`source.zip`).

**Alpha playtest.** Expect bugs. The installer backs up your saves. Don't save while your friend is in your world.

## Download
Get the newest build from **[Releases > Latest](../../releases/latest)**. Download the file named
`CrimsonDesert-SeamlessCoop-<version>.zip` (not the `-mod-manager` one, unless you use a mod manager).

**Both players need the same zip.**

## Requirements
- Crimson Desert on Steam, game version **1.0.0.3527**. On another version the mod switches itself off.
- You and your friend are Steam friends, with the Steam overlay on.

## Install
1. Close the game.
2. Right-click the downloaded zip > **Properties**. If there's an **Unblock** box at the bottom, tick it and press
   OK. Windows marks downloaded files, and this stops the "Windows protected your PC" prompt.
3. Right-click the zip > **Extract All...** and extract it anywhere, for example the Desktop.
4. In the extracted folder, double-click **`Install Co-op+.cmd`**. It finds the game through Steam, backs up your
   saves to `Documents\CrimsonDesert-CoopPlus-SaveBackups`, and copies 4 files into the game's `bin64` folder.
   - If Windows SmartScreen still appears: **More info > Run anyway**. The scripts are plain text, so you can open
     them in Notepad first.
5. Start Crimson Desert from Steam and load a save. A controls card appears the first time. **Shift+F1** (or hold
   View / Touchpad and click the left stick) opens it again any time.

## Play together
- **Host:** press **Ctrl+F1** (pad: hold View + Y) and invite your friend in the Steam overlay.
- **Friend:** accept the invite, or right-click the host in your Steam friends list > **Join Game**.
- **F8** (pad: View + X) opens the co-op panel: connection, settings, your record.
- **Your saves are never changed by Co-op+.**
- **NEW, experimental (you choose):** a separate save for each friend you play with, and quest steps shared live.
  The first launch asks: **Shift+F5** (pad: View + D-pad Up) turns it on, **Shift+F6** is "not now"; change it any
  time in F8 > Settings. Both players need it on. Tell us in the Discord how it goes.

## Uninstall / turn off
- `Uninstall Co-op+.cmd` removes exactly what was installed.
- `Vanilla mode on-off.cmd` switches the mod off and back on, keeping your settings.

## Report a bug
Double-click `Diagnose Co-op+.cmd` (game closed). It puts a zip on your clipboard with your Steam names, IDs and
Windows user name removed. Paste it in the Discord bug channel with what happened (`BUG-REPORT.txt` has the
questions). Reports from **both** players are best.

## What's in it
See [FEATURES.md](FEATURES.md). Features marked **Needs testing** are the ones that most need your reports.

Not affiliated with or endorsed by Pearl Abyss. Crimson Desert is a trademark of Pearl Abyss Corp.
