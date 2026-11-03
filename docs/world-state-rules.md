# What a co-op session changes in each world (roadmap T20)

Each player plays in their **own world and save**. The host's world is the one the two of you fight in; the guest's
world keeps running underneath, with its enemies near the host held back while the session lasts.

| Change during co-op | In the HOST's world / save | In the GUEST's world / save |
|---|---|---|
| Enemies the two of you kill (camps, patrols) | Dead, as if the host killed them; the camp counts as cleared for the host (quest credit, loot, respawn timers as normal) | The guest's own enemies there were only hidden: they come back when the session ends (the state audit checks it 30 s later). Exception: with shared loot on, the guest's own enemy of the same kind that stood in for a kill the guest helped with is killed to give the guest its drop -- that one stays dead in the guest's world |
| Bosses | The host's boss fight, the host's quest and the host's reward | Untouched: the guest's own boss (if any) is still there |
| Gathering, chests, doors, levers, items picked up | What the HOST does happens in the host's world | What the GUEST does happens in the guest's world (each of you opens your own chests and gathers your own nodes) |
| Quests, knowledge, map | The host's own progress | The guest's own progress (the guest gets the "Quest Updated" notices of the host, nothing more) |
| Time of day and weather | The host's | The guest sees the host's while connected; its own come back after the session |
| Crimes and bounties | Only the host's own | Only the guest's own |
| Party scaling (tougher enemies) | Only while a guest is in; gone when they leave | -- |

Saving: the game's own saves keep working, but **nobody should save while the other player is connected**
(Co-op+ backs saves up when a session starts and warns if the game saved during one). After the session each player
saves as usual and keeps exactly what happened in their own world, as above.

Tests behind these rules (one PC, PROGRESS.md): audit1-3 (T08: nothing of the session left 30 s after it ends, the
guest's hidden enemies back), loot1 (T10: the guest's stand-in enemy killed for its drop), leave3 (T07), the
boss audit (T13, docs/research/boss-audit.md).
