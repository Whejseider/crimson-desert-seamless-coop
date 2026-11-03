# Network stress report (roadmap T11, 2026-11-03)

Setup: one PC, game 1.0.0.3479, the echo peer (cdmp_peer relay) started with each network condition; the bandits
test save 60 m from the Rodbel camp with the guest's whole 150 m zone copied ([dev] zone_copy_test, 12-13 bandit
copies loaded at once), the other player's character spawned. Baseline = same place, no peer. Scripts:
build/port3479/stress_test.sh + stress_report.py (phases split by Ctrl+F12 marks in cdmp.log).

Conditions: good = 50 ms delay; bad = 150 ms + 50 ms jitter + 5 % loss; awful = 300 ms + 100 ms jitter + 15 % loss;
lossy = 100 ms + 20 ms jitter + 30 % loss (applied by the relay to the game traffic; the relay answers pings
itself, so "rtt" below does not include the simulated delay).

## Run 1 (stress1, 90 s per phase)
| phase | FPS | out kB/s | in kB/s | rtt ms | enemy copies loaded | connects | link lost | silences | resyncs | loop p95 ms | crash |
|---|---|---|---|---|---|---|---|---|---|---|---|
| baseline | 70 | 0.0 | 0.0 | - | 0 | 0 | 0 | 0 | 0 | - | 0 |
| good | 70 | 4.5 | 4.4 | 31.7 | 12 | 1 | 1 | 0 | 0 | 35.2 | 0 |
| bad | 70 | 5.5 | 5.2 | 53.4 | 12 | 1 | 1 | 0 | 0 | 49.2 | 0 |
| awful | 70 | 8.6 | 7.0 | 75.7 | 12 | 1 | 1 | 0 | 0 | 42.6 | 0 |
| lossy | 70 | 5.9 | 4.2 | 47.4 | 12 | 1 | 1 | 0 | 0 | 40.4 | 0 |
| after | - | - | - | - | 0 | 0 | 0 | 0 | 0 | - | 0 |

## Run 2 (stress2, 60 s per phase, with the T11 rate degradation)
| phase | FPS | out kB/s | in kB/s | rtt ms | enemy copies loaded | connects | link lost | silences | resyncs | loop p95 ms | crash |
|---|---|---|---|---|---|---|---|---|---|---|---|
| baseline | 71 | 0.0 | 0.0 | - | 0 | 0 | 0 | 0 | 0 | - | 0 |
| good | 70 | 5.0 | 4.8 | 75.7 | 13 | 1 | 1 | 0 | 0 | 102.1 | 0 |
| bad | 70 | 6.8 | 6.4 | 87.1 | 13 | 1 | 1 | 0 | 0 | 123.0 | 0 |
| awful | 71 | 9.8 | 7.9 | 105.0 | 13 | 1 | 1 | 0 | 0 | 122.6 | 0 |
| lossy | 70 | 8.7 | 6.0 | 87.2 | 13 | 1 | 1 | 0 | 0 | 121.8 | 0 |
| after | - | 0.0 | 0.0 | - | 0 | 0 | 0 | 0 | 0 | - | 0 |

## Findings
- **No crash, no lost link, no spurious removal** of the other player's character in any condition; every zone
  copy loaded even at 30 % loss (the reliable channel retransmits).
- **FPS cost: none measurable** (70-71 fps in every phase, the baseline included): the mod's work runs on its own
  network thread.
- **Bandwidth:** 4.5-10 kB/s each way on average; bursts up to ~24 kB/s while a zone of copies is sent (8 KB per
  enemy load). Loss raises the outgoing side (retransmissions).
- The network loop's enemy / zone work (its own thread) takes p95 35-120 ms per 10 s window depending on the run;
  it never reached the game thread. Worth profiling later (it varies between runs at the same spot).

## Budgets and degradation
- Entities: the guest's zone holds at most 48 copies (enemy_relay kZoneMax): beyond that the radius shrinks to the
  nearest 48 (never under the minimum zone radius).
- Rate: a poor link (rtt over 250 ms, or more than 32 reliable fragments waiting) halves the state update rate
  (20 -> 10 Hz) for at least 5 s, before anything else is given up; full rate again when rtt is under 150 ms and
  fewer than 8 wait. Logged ("Net: poor link ..." / "link good again").
- Seen in run 2: the reliable backlog crosses 32 for a moment whenever a batch of enemy copies is sent; the 5 s
  hold keeps that from flapping.
