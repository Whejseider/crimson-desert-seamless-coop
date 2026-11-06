# Low latency: the other player drawn ~25 ms after the network (2026-11-06)

A playtester asked for the other player to react as close to real time as possible; duels depend on it. Measured on one
PC (game 1.0.0.3576, scratch world, echo peer: the copy replays our own moves), tools/lag:

| Build | Screen to copy | Network (out and back, inside the mod) | Notes |
|---|---|---|---|
| released (a82c1b48 / f3517194) | **352 ms** | 8 ms median, 20 p95 | 150 ms playout delay + the game's own 200 ms |
| low-latency playout (lag1) | 56 ms | 4 / 12 | buffer in the timestamps, adaptive |
| + fast lane (lag7) | 48 ms | **1.1 / 1.5** | movement no longer waits for the network loop |
| + pose stream, moves-only clock (lag11) | 44 ms (32 per window) | 0.6 / 1.2 | lateness 1-3 ms |
| + halfway prediction (lag12) | **26 ms (24 per window)** | 0.7 / 1.3 | buffer 15-24 ms |
| same, simulated internet 30 ms ± 10 (lag13) | 72 ms | 39 / 49 | buffer grows to ~30 ms by itself |

Path residual (how far the copy's path is from ours once shifted) stayed 0.05-0.06 m throughout: no extra jitter.
Duel 9/9 HIT (du77), death + checkpoint revive (dt77), live quests both ways (ls77), far apart and back (far77) pass.

## How to measure
`build/port3527/lag_test.sh <name>` (copy in tools/lag/lag_test.sh): scratch world, the echo peer (`PEERX` sets its
delay / jitter; default `--delay 0`), `[dev] lag_probe=1`, held keys walk us around. The probe (mod/net/lag_probe.cpp)
writes our position and the copy's every 4 ms plus marks (1 our Move sent, 2 an echo received, 3 a copy position
handed to the game with its margin before the game draws it) to bin64/cdmp_lag.csv; `tools/lag/lag.py` fits the time
shift. The network loop logs `Timing: network loop ...` (wait / work, and per section) every 10 s.

## Where the 352 ms went, and what changed
1. **The game draws a remote actor 200 ms behind its timestamps** (GlobalVariableInt exe+0x6CB9548 on 2976; still 200:
   352 = 150 + 200 + transit). The old code added its own 150 ms (`[coop] delay_ms`) on top. Now the timestamps are
   shifted by `offset + buffer - 200` (coop.cpp `ts_shift`), so the copy is drawn `buffer` ms after the fastest
   transit. The buffer (common/playout_buffer.h) = the 95th percentile of how late moves arrive + the moves' spacing
   (half of it with prediction), at least `[coop] buffer_ms` (default 10), rising 5 ms / falling 1 ms per event so the
   timestamps never step back. Moves go into the game as they arrive (`puppet_set_move_hold(0)`); actions are held
   `buffer - lateness` so they start where the copy is.
2. **The network loop took ~25-40 ms a pass** with enemies around (co-op update ~19 ms, our state ~9 ms, activities
   ~5 ms; `Timing: network loop sections`). Our moves waited up to a pass to go out, theirs up to a pass to come in.
   The **fast lane** (mod/net/fast_lane.cpp) is a thread that receives every datagram (1 ms waits): the other player's
   own movement (Move, StartAction, Rotate, WallUp, Docking) goes straight to their copy (CoopLink::on_hot_event), and
   every datagram is then queued for the network loop, which does the session's bookkeeping
   (`on_datagram(..., event_handled)`) and everything else as before. Our own hot events go out from the same thread
   the moment the capture sink gets them. ClientSession::send has a mutex; the puppet feed functions share one
   (`g_feed_mutex`, remote_puppet.cpp); CoopLink's clock / buffer have `feed_mutex_`.
3. **The sniffer's writer woke every 10 ms**: it now also wakes on every packet we send (`g_c2s_wake`), and feeds co-op
   before writing a recording.
4. **The game stamps its MoveActorReq up to ~25 ms before it leaves**, and its clock wanders against the wall clock by
   30-60 ms (server corrections). Our own clock estimate (capture_game_clock_ms, the clock read is off since 3479) is
   now `steady + max(ts - capture time)` over 1 s (the capture time from the sniffer record), restarted when the game
   clock steps back. The **pose stream** (local_capture.cpp) reads our transform every 1 ms and sends a Move the
   moment a frame moved us (at most every 7 ms), from the last real Move as template; on foot only (`[coop]
   pose_stream`). Real Moves on foot then only update the template (sent again if the stream is silent 100 ms). The
   receiver maps the clock from Moves only (actions and turns carry the late stamps).
5. **Halfway prediction** (CoopLink::predict_after, `[coop] predict`): after each of their Moves on foot, a position
   halfway to the next one from their speed, so the buffer covers half the spacing. The real move lands next and
   wins; at 6 m/s the worst error is a few cm.

## Limits
- The copy can never be closer than the network's one-way time: on the internet the result is that + ~25 ms. Below it
  only by predicting their position ahead (dead reckoning); not done (wrong on dodges and turns, which duels are made
  of).
- Our transform changes every ~20-30 ms at ~56 fps here (the median spacing of the stream); a faster PC gives a
  smaller buffer.
- Positions late for the game: 9-26 % at the 5 ms scale (lag12 margin p5 -19 ms); the path residual did not grow, so
  it is not visible as jitter at these sizes. If a player sees the copy stutter, `[coop] buffer_ms=30` gives margin.
