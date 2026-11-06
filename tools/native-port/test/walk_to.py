# Walk one copy's character to a point (two-copy tests): hold W in its window and steer the camera with the mouse,
# reading the character's position from that copy's own MoveActorReq rows in its packet log.
#   python walk_to.py <guest pid> <its cdmp_packets.<role>_*.log> <x> <z> [--stop M] [--max S]
# The heading is measured from the character's own movement (the last two fixes), so no camera/yaw convention is
# assumed; the steering gain adapts its sign and size to what each turn actually did.
import math, os, re, subprocess, sys, time

ACT = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'actpid.ps1')
pid, log, tx, tz = int(sys.argv[1]), sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
stop = float(sys.argv[sys.argv.index('--stop') + 1]) if '--stop' in sys.argv else 4.0
max_s = float(sys.argv[sys.argv.index('--max') + 1]) if '--max' in sys.argv else 150.0
ROW = re.compile(r' C>S MoveActorReq +\d+ slot +\S+ len +\d+ \| ((?:[0-9A-F]{2} ){16})')


def act(seq):
    subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', ACT, '-GamePid', str(pid),
                    '-Seq', seq], capture_output=True, timeout=30)


def fix(actor_class=0xA):
    """The latest position of the copy's own character (actor key class 0xA) from its packet log."""
    for tail in (400_000, None):  # the recent rows first; the whole file if the character has not moved lately
        with open(log, 'rb') as f:
            if tail: f.seek(max(0, os.path.getsize(log) - tail))
            text = f.read().decode('utf-8', 'replace')
        p = last_own(text, actor_class)
        if p: return p
    return None


def last_own(text, actor_class):
    for m in reversed(list(ROW.finditer(text))):
        b = bytes(int(h, 16) for h in m.group(1).split())
        key = int.from_bytes(b[0:4], 'little')
        if key >> 28 != actor_class: continue
        import struct
        x, y, z = struct.unpack_from('<3f', b, 4)
        return x, z
    return None


def wrap(a):
    return (a + math.pi) % (2 * math.pi) - math.pi


t0 = time.time()
gain = 350.0  # mouse pixels per radian of heading error (sign and size adapt)
prev = fix()
if not prev: sys.exit('no position yet in ' + log)
print('start %.1f %.1f -> target %.1f %.1f (%.0f m)' % (prev[0], prev[1], tx, tz, math.dist(prev, (tx, tz))))
act('kd:W kd:SHIFT')  # run (Shift held) toward the target
last_turn, last_err = 0, None
best, stalled, side = 1e9, 0, 1
try:
    while time.time() - t0 < max_s:
        time.sleep(0.8)
        cur = fix()
        if not cur: continue
        d = math.dist(cur, (tx, tz))
        if d <= stop:
            print('arrived: %.1f m from the target after %.0f s' % (d, time.time() - t0)); break
        mv = (cur[0] - prev[0], cur[1] - prev[1])
        best = min(best, d)
        stalled = stalled + 1 if d > best - 0.5 else 0
        if math.hypot(*mv) < 0.5 or stalled >= 4:  # blocked (a wall, a roof, climbing): back off, go round, resume
            act('ku:W k:S:900 move:%d:0:12 kd:W w:2200' % (side * 420))
            side = -side
            stalled, best, prev = 0, 1e9, fix() or cur
            continue
        heading = math.atan2(mv[0], mv[1])
        want = math.atan2(tx - cur[0], tz - cur[1])
        err = wrap(want - heading)
        if last_err is not None and last_turn:  # did the last turn reduce the error? if it grew, flip the sign
            if abs(err) > abs(last_err) + 0.3: gain = -gain * 0.8
        turn = max(-900, min(900, int(gain * err)))
        print('  %.0f m | heading %4.0f want %4.0f err %4.0f | turn %d' % (d, math.degrees(heading), math.degrees(want),
                                                                       math.degrees(err), turn))
        if abs(turn) > 15: act('move:%d:0:12' % turn)
        last_turn, last_err, prev = turn, err, cur
    else:
        print('gave up after %.0f s, %.1f m left' % (max_s, math.dist(fix() or prev, (tx, tz))))
finally:
    act('ku:SHIFT ku:W')
