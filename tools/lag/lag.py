# Co-op+ lag measurement (2026-11-06): reads bin64/cdmp_lag.csv written by [dev] lag_probe=1 (lag_probe.h) and finds
# how far behind our own character the copy is drawn. With the echo peer the copy replays our own moves, so the time
# shift that best lines the copy's path up with ours (a constant sideways offset fitted out) is the delay from our
# screen to the copy. Also: the time from one of our Moves being sent to its echo arriving (marks 1 and 2).
#   python lag.py <cdmp_lag.csv>
import sys
import numpy as np

STEP_US = 4000


def load(path):
    pos, sent, recv = [], {}, {}
    margins = []
    for line in open(path):
        p = line.strip().split(',')
        if len(p) < 6 or not p[0].isdigit():
            continue
        t, kind = int(p[0]), int(p[1])
        if kind == 0:
            pos.append((t, float(p[2]), float(p[3]), float(p[4]), float(p[5])))
        elif kind == 1:
            sent.setdefault(int(p[2]), t)
        elif kind == 2:
            recv.setdefault(int(p[2]), t)
        elif kind == 3:
            v = int(p[2])
            margins.append(v - (1 << 64) if v >= 1 << 63 else v)
    return np.array(pos), sent, recv, margins


def best_shift(t, me, cp, max_ms=900):
    """Shift (ms) minimising |copy(t) - me(t - shift) - c|, c the mean offset; and the residual (m)."""
    grid = np.arange(t[0] + max_ms * 1000, t[-1], STEP_US)
    if len(grid) < 50:
        return None, None
    cx, cz = np.interp(grid, t, cp[:, 0]), np.interp(grid, t, cp[:, 1])
    best = (None, 1e18)
    for s in range(0, max_ms + 1, 2):
        mx, mz = np.interp(grid - s * 1000, t, me[:, 0]), np.interp(grid - s * 1000, t, me[:, 1])
        dx, dz = cx - mx, cz - mz
        err = np.mean((dx - dx.mean()) ** 2 + (dz - dz.mean()) ** 2)
        if err < best[1]:
            best = (s, err)
    return best[0], float(np.sqrt(best[1]))


def main(path):
    pos, sent, recv, margins = load(path)
    if len(pos) < 100:
        print('lag: too few samples (%d); was the copy there?' % len(pos))
        return
    # a read in the middle of a reload can give (0, 0) for a frame or two: not a position
    gap = np.hypot(pos[:, 3] - pos[:, 1], pos[:, 4] - pos[:, 2])
    good = (gap < 5) & (np.abs(pos[:, 1]) > 1) & (np.abs(pos[:, 3]) > 1)
    if (~good).sum():
        print('lag: %d glitch samples left out' % (~good).sum())
    pos = pos[good]
    t = pos[:, 0]
    me, cp = pos[:, 1:3], pos[:, 3:5]
    # only the stretch where we moved
    speed = np.r_[0, np.hypot(np.diff(me[:, 0]), np.diff(me[:, 1])) / np.maximum(np.diff(t), 1) * 1e6]
    moving = speed > 0.5
    print('lag: %d samples over %.1f s, every %.1f ms on average, moving %.0f%% of the time' %
          (len(t), (t[-1] - t[0]) / 1e6, np.mean(np.diff(t)) / 1000, 100 * moving.mean()))
    s, res = best_shift(t, me, cp)
    print('lag: SCREEN TO COPY %s ms (path residual %.2f m)' % (s, res if res is not None else -1))
    # per 3 s window while moving
    win = []
    w0 = t[0]
    while w0 + 3e6 < t[-1]:
        m = (t >= w0) & (t < w0 + 3e6)
        if moving[m].mean() > 0.4:
            ws, wr = best_shift(t[m], me[m], cp[m], 700)
            if ws is not None:
                win.append(ws)
        w0 += 3e6
    if win:
        print('lag: per 3 s window: min %d, median %d, max %d ms (%d windows)' %
              (min(win), int(np.median(win)), max(win), len(win)))
    if margins:
        m = np.array(margins)
        print('lag: copy positions given to the game ahead of its drawing time: p5 %d, median %d, p95 %d ms; '
              'late (below 0): %.1f%%' % (np.percentile(m, 5), np.median(m), np.percentile(m, 95), 100 * (m < 0).mean()))
    both = [recv[k] - sent[k] for k in sent if k in recv]
    if both:
        b = np.array(both) / 1000
        print('lag: our Move sent -> its echo received: median %.1f ms, p95 %.1f, max %.1f (%d moves)' %
              (np.median(b), np.percentile(b, 95), b.max(), len(b)))


if __name__ == '__main__':
    main(sys.argv[1])
