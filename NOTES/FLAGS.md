# Flags: things that need the user (read when they check in)

Rule (user, 2026-11-02, for the whole project): work every task that can be completed without the user; when a task
needs the user, add it here and move on; stop only when everything is done or flagged. Never ask "continue?".

Format: ID | date | what is blocked | why | what I need from you | status

| ID | Date | Blocked | Why | Needed from you | Status |
|---|---|---|---|---|---|
| F1 | 2026-11-02 | Everything marked "two-PC" in PLAN/PLAYTEST-1 | one PC can only echo my own character | the playtest with your friend (docs/tests/PLAYTEST-1.md) | waiting |
| F2 | 2026-11-02 | Flying mounts (Blackstar Dragon, Wyvern, Cloudcart) and late vehicles (ATAG mech, wagon, skiff) | the test save is early game (Hernand, Chapter ~2); the dragon comes at the end of Chapter 11. Horse riding together works (D30); flying uses the same copy path but must be seen | a save that has the dragon (or any flying mount). Tell me the slot; I would back up all saves first, load it only with co-op tests, never save, and run the save check after | open |
| F3 | 2026-11-02 | Cutscenes shared within 100 m + skip by everyone | none of 35 captures contains a cutscene; I need to trigger the same cutscene repeatedly to find its start/skip messages | either a test save right before a story cutscene (OK to create one in an EMPTY slot from your save? the earlier test save was made that way with your OK), or play to a cutscene once with packet recording on (record=1) and send me the cdmp_packets files | open |
| F4 | 2026-11-02 | Minigames (arm wrestling, archery, wrestling, duels, Duo cards...) | each minigame's protocol must be captured while playing it; the test save is outside Hernand and my keyboard-driving cannot reliably find and talk to the NPCs | play each minigame once with [sniffer] record=1 (arm wrestling at Hernand Inn, bow contest at Lioncrest Manor, wrestling at Kharonso, boxing at Goldenfist Arena, spear duel, Duo table) and keep the cdmp_packets_* files; or a test save standing next to a minigame NPC | open |
