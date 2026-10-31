CRIMSON DESERT SEAMLESS CO-OP+  (alpha, @VERSION@)
===================================================

Play Crimson Desert with a Steam friend. Unofficial fan mod; not made or endorsed
by Pearl Abyss. ALPHA: expect rough edges, and back up your saves (the installer
does it for you).

Built on "Crimson Desert Multiplayer" by ByJunter and its port to the current game
version by nouzert1. See CREDITS.md.


BEFORE YOU START
----------------
- Both players: Crimson Desert on Steam, game version 1.0.0.3446 (the current
  patch when this was built). On another version the mod switches itself off and
  the game runs normally.
- Both players need THIS SAME ZIP. Different builds warn "Different mod versions".
- You must be Steam friends.
- Steam overlay on (Steam > Settings > In Game > Enable the Steam Overlay).


INSTALL (each player, game closed)
---------------------------------
1. Unzip this folder anywhere (Desktop is fine).
2. Double-click  "Install Co-op+.cmd".
   It finds the game through Steam, copies your saves to
   Documents\CrimsonDesert-CoopPlus-SaveBackups, and puts three files into the
   game's bin64 folder: winmm.dll (Ultimate ASI Loader), cdmp.asi (the mod) and
   cdmp.ini (settings).
3. Start the game from Steam as usual.

To remove it: double-click "Uninstall Co-op+.cmd" (game closed).
To play without it for a while: "Vanilla mode on-off.cmd" switches Co-op+ off and on
again, keeping your settings.
Something wrong? "Diagnose Co-op+.cmd" writes a report to your Desktop to send along.


PLAYING TOGETHER
----------------
- Load your save. Everyone starts as the host of their own world.
- Host: press Ctrl+F1 to open the Steam overlay and invite your friend.
- Friend: accept the invite, or right-click the host in the Steam friends list and
  choose "Join Game". If the friend's game is closed, Steam starts it and the mod
  joins as soon as they load a save.
- The friend appears in your world as a character, on the minimap and the map.
- Ctrl+F2 (guest): leave and go back to your own world.
- F8: show/hide the co-op panel (top left): connection, ping, room code.
- Ctrl+F8: show/hide your friend's name over their head.

Privacy: by default any Steam friend who plays with Co-op+ can "Join Game" into your
world. To allow only friends you invite, open the game's bin64\cdmp.ini and set
join=invite under [net].

Fallback: the panel shows a 7-letter ROOM CODE for the host. A friend can type it
into cdmp.ini under [room] code=  and restart the game.


WHAT WORKS / WHAT DOESN'T (YET)
-------------------------------
- Each player plays in THEIR OWN save and sees the other walking, running,
  climbing, gliding, fighting and riding, with matching gear.
- Hostile enemies near the host are shared: the guest sees the host's enemies and
  can damage and kill them. Bosses: the guest helps, the host lands the final
  blow (protects the host's quest).
- Not shared yet: quests, story progress, most loot (experimental), civilians.
- Two players at a time.

IMPORTANT: do not save while the other player is in your world. Quit without
saving, or wait until they leave. Saving during a session is the most likely way to
damage a save. As a safety net the mod copies your saves to
Documents\CrimsonDesert-CoopPlus-SaveBackups (folders named auto-...) each time a
co-op session starts, keeps the newest 10, and tells you on the panel if the game
saved during co-op (the game also autosaves on its own). To restore one: game closed,
copy that folder's contents back into %LOCALAPPDATA%\Pearl Abyss\CD\save.


PROBLEMS?
---------
The mod writes cdmp.log in the game's bin64 folder. Send that file (and say what
happened) when you report a problem. It contains in-game data such as positions,
not personal information.


LICENSE
-------
GPL v3 (LICENSE.txt). The full source code of exactly this build is in source.zip.
Third-party parts and their licenses: THIRD-PARTY-NOTICES.txt.
