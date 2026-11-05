# Crimson Desert Seamless Co-op+ (alpha): features

Two-player co-op for Crimson Desert (Steam, game version 1.0.0.3549). Each of you plays in your own save;
your friend joins your world as their own character. Unofficial fan mod, built on Crimson Desert Multiplayer by
ByJunter and its port by nouzert1.

**How to read the Status column**
- **Works:** checked in the game on one PC against a stand-in second player. It should work, but a report from two
  real players still helps.
- **Needs testing:** built, but one PC can't show it. It needs two real players to confirm.
- **Not yet:** not possible in this version.

## Joining and sessions

| Feature | Status |
|---|---|
| Invite a Steam friend (Ctrl+F1), or they use "Join Game" on you in Steam | Needs testing |
| Room code as a fallback way to join | Needs testing |
| Friends-only or invite-only (setting) | Needs testing |
| Version check: refuses a friend on another Co-op+ build or game version, and says what to update | Works |
| Automatic reconnect after a crash, a long alt-tab or a dropped connection | Works |
| Joining while the host is in a fight, a menu or a loading screen | Works |
| Leaving (Ctrl+F2) returns you to your own world with nothing left behind | Works |
| Automatic resync when things drift apart, plus a manual resync key | Works |

## Your friend in your world

| Feature | Status |
|---|---|
| Their character walks, runs, climbs, glides, swims and fights, in matching gear that updates live | Works |
| Name tag over their head (fades with distance, hidden over menus) | Works |
| On your minimap and map; shared map waypoints | Works |
| Partner frame: their name, health and stamina, with a "Badly hurt" warning | Works |
| Horses (a fixed bug: the horse copy no longer appears where it was first called) | Works |
| Flying mount (Blackstar) | Needs testing |
| Partner sounds: footsteps and swings come from the game itself | Needs testing |
| "In a menu" tag; they can't be hurt while paused | Works |
| Player colours: the host is gold with a diamond, the guest blue with a round mark; a colour-blind-safe option | Works |

## Fighting together

| Feature | Status |
|---|---|
| Shared enemies near the host: both of you see and hit the same ones | Works |
| Tougher enemies with a guest (adjustable in the F8 settings) | Works |
| Kill feed: "<friend> defeated a Deer" | Works |
| Bosses together; the host lands the final blow so the story is safe | Needs testing |
| Shared boss and story cutscenes; a skip needs everyone to press it | Needs testing |
| Revive your friend (hold Shift+F7 by them) | Works |
| NEW: dying in co-op keeps your HUD, menus and horse (a death used to leave them gone after any revive) | Works |
| Respawn option: back to your friend after a checkpoint revive | Works |
| Your friend can't kill your quest NPCs or townsfolk | Works |
| Loot from shared kills: each your own, host only, or first come | Needs testing |
| XP for the guest from shared kills | Not yet |

## Communication and travel

| Feature | Status |
|---|---|
| Text chat (Shift+F2) | Works |
| Quick messages with map pins: Go here, Danger, Loot here, Follow me, Hold still: photo! | Works |
| Go to your friend (Ctrl+F4) | Works |
| Ask your friend to come to you (Shift+F4) | Works |
| Fast travel: your friend is offered to follow | Needs testing |
| Notices: your friend's quest steps, world events near them, when they become wanted | Works |
| Spoiler guard: hide your friend's story (quest names you haven't reached, their story scenes) | Works |

## Challenges

| Feature | Status |
|---|---|
| Duel: a ring, 3-2-1, first to two rounds, nobody dies; ring size option (hits fixed after the first playtest) | Works, needs testing with two players |
| Rock, paper, scissors (first to two hands; no hand animation yet) | Works |
| Hunting contest (3 minutes, the most game wins) | Works |
| Race to the waypoint (first to the host's map waypoint) | Works |
| Arm wrestling with your friend in the opponent's chair | Needs testing |
| Every challenge is one match and ends with a Victory or Defeat card | Works |
| Lasting scoreboard per friend and titles ("the Duelist") | Works |
| Quick rematch | Works |

## World

| Feature | Status |
|---|---|
| Time of day and weather follow the host | Works |
| NEW: Focus does not slow the world in co-op (keeps both worlds in step; F8 setting to turn it back on) | Works, needs testing with two players |
| Resting at a campfire together (the host's Wait moves both clocks) | Needs testing |
| NEW: a co-op save per friend, host and guest (your solo saves untouched); opt in on the first launch or in F8 | Experimental, needs testing |
| NEW: your first session together begins a new journey for both, at the start of the story | Experimental, needs testing |
| NEW: switching between your own save and a co-op save asks first (now, or later from the title screen) | Works |
| NEW: quest steps shared live (go to, defeat, talk): your friend finishes a step you also have, it finishes for you, with rewards | Experimental, needs testing |
| Each player keeps their own character and items, and their own quests while playing; nothing leaks into the other save | Works |
| Item trading or a shared stash | Not yet |
| Doors, levers and breakable objects in sync | Not yet |

## Comfort and safety

| Feature | Status |
|---|---|
| One-click install, uninstall and vanilla on/off | Works |
| Save backups whenever a session starts, and a warning if the game saves during co-op | Works |
| Controls card on the first start, and any time with Shift+F1 (controller buttons when a pad is plugged in) | Works |
| Full controller support (hold View / Touchpad + a button) | Works |
| F8 panel with settings: loot, revive, difficulty, colours, spoilers, connection readout and more | Works |
| Rebindable hotkeys, with warnings for clashes | Works |
| Connection quality: ping and packet loss | Works |
| Plain error messages that say what to do next | Works |
| A warning about other mods that can clash | Works |
| Diagnose: a one-click bug report with personal info removed | Works |
| Turns itself off safely after a game update, or if an anti-cheat ever appears | Works |

**Known issues:** two players only. Co-op saves and live quests are new and experimental (opt in; first two-player session done, fixes from it are in). In co-op you stay on your feet behind the death screen until you revive. The arm wrestling table can be empty for a guest who
arrives already connected; leave the pub and come back. Don't save while your friend is in your world (backups
are made for you).
