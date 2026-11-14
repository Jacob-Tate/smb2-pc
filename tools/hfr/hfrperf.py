#!/usr/bin/env python3
"""Summarise the [hfr] statistics lines of a run log (docs/hfr.md).

  hfrperf.py log.txt [min_draws]

One row per 5-second [hfr] window: the tick frame number and game mode at
its end, presents per second, present-interval mean / p50 / p95 / p99 / max,
late presents, replay CPU ms, replays per tick, kcap and 3D draws per tick.
Windows with fewer than min_draws 3D draws per tick (menus, loading) can be
filtered out."""
import re, sys
log = sys.argv[1]
mind = float(sys.argv[2]) if len(sys.argv) > 2 else 0
frame, mode = 0, '?'
pat = re.compile(r'\[hfr\] ([\d.]+) fps \| gpu ([\d.]+) ms/frame \| interval mean ([\d.]+) sd ([\d.]+) p50 ([\d.]+) p95 ([\d.]+) p99 ([\d.]+) max ([\d.]+) ms, >1.5x target (\d+) \| replay ([\d.]+) ms x ([\d.]+)/tick \(cap (\d+)\) match ([\d.]+) ms(?: first ([\d.]+) ms)? \| 3D draws/tick (\d+)')
print('%6s %6s %7s %6s %6s %6s %6s %6s %7s %5s %7s %5s %4s %6s %6s %6s' % ('frame', 'mode', 'fps', 'gpu', 'mean', 'p50', 'p95', 'p99', 'max', 'late', 'replay', 'r/t', 'cap', 'match', 'first', 'draws'))
for l in open(log, errors='replace'):
    m = re.match(r'\[frame (\d+)\] mode (\d+/\d+)', l)
    if m:
        frame, mode = int(m.group(1)), m.group(2)
        continue
    m = pat.search(l)
    if m:
        g = m.groups()
        if float(g[14]) < mind:
            continue
        print('%6d %6s %7s %6s %6s %6s %6s %6s %7s %5s %7s %5s %4s %6s %6s %6s' % (frame, mode, g[0], g[1], g[2], g[4], g[5], g[6], g[7], g[8], g[9], g[10], g[11], g[12], g[13] or '-', g[14]))
