#!/usr/bin/env python3
"""Aggregate [hfr] windows of several runs into one table (docs/hfr.md).

  hfrtable.py <runs_dir> label/mode [label/mode ...]

Per run, two rows: gameplay/scene windows (>= 1000 3D draws per tick) and
light windows (menus, < 1000).  Values are medians over the 5 s windows
(fps, GPU ms per frame, present-interval p50 / p95 / p99, replay ms,
matching ms) and the worst window's fps."""
import re, sys, statistics as st
pat = re.compile(r'\[hfr\] ([\d.]+) fps \| gpu ([\d.]+) ms/frame \| interval mean ([\d.]+) sd ([\d.]+) p50 ([\d.]+) p95 ([\d.]+) p99 ([\d.]+) max ([\d.]+) ms, >1.5x target (\d+) \| replay ([\d.]+) ms x ([\d.]+)/tick \(cap (\d+)\) match ([\d.]+) ms(?: first ([\d.]+) ms)? \| 3D draws/tick (\d+)')
root = sys.argv[1]
print('| run | scenes | windows | fps (median / worst) | GPU ms/frame | interval p50 / p95 / p99 ms | replay ms | match ms | first response ms | 3D draws/tick |')
print('|---|---|---|---|---|---|---|---|---|---|')
for run in sys.argv[2:]:
    rows = []
    try:
        for l in open('%s/%s/log.txt' % (root, run), errors='replace'):
            m = pat.search(l)
            if m:
                rows.append([float(x) if x is not None else -1.0 for x in m.groups()])
    except OSError:
        continue
    rows = rows[1:]   # first window: startup
    for name, sel in (('gameplay', lambda r: r[14] >= 1000), ('light', lambda r: r[14] < 1000)):
        w = [r for r in rows if sel(r)]
        if not w:
            continue
        med = lambda i: st.median([r[i] for r in w])
        gpu = [r[1] for r in w if r[1] > 0]
        fr = [r[13] for r in w if r[13] >= 0]
        print('| %s | %s | %d | %.0f / %.0f | %s | %.2f / %.2f / %.2f | %.2f | %.2f | %s | %.0f |' % (
            run, name, len(w), med(0), min(r[0] for r in w),
            '%.2f' % st.median(gpu) if gpu else 'n/a', med(4), med(5), med(6), med(9), med(12),
            '%.1f' % st.median(fr) if fr else 'n/a', med(14)))
