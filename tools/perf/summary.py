#!/usr/bin/env python3
"""summary.py run... : stage-clear -> stage-select window stats (frames from
the first goal (2/53) to the end; dump frames excluded) per run."""
import sys, os
WS = os.environ.get('PERF_RUNS', 'runs')
def q(v, x): v = sorted(v); return v[min(len(v) - 1, int(x * len(v)))] if v else float('nan')
print(f"{'run':22s} {'frames':>6s} {'p50':>6s} {'p99':>6s} {'max':>7s} {'>20ms':>5s} | {'work p50':>8s} {'p99':>6s} {'max':>6s} | {'copytex':>7s} | underruns | mem private MiB (start->end of window) | gpu MiB")
for r in sys.argv[1:]:
    p = os.path.join(WS, r, 'frames.txt')
    if not os.path.exists(p): print(r, 'no frames.txt'); continue
    rows, mem, und = [], [], []
    start = None
    for ln in open(p):
        if ln.startswith('#mem'):
            a = ln.split(); mem.append((int(a[1]), int(a[3]) >> 20, (int(a[7]) >> 20) if len(a) > 7 else -1)); continue
        if ln.startswith('#underrun'):
            a = ln.split(); und.append(int(a[1])); continue
        if ln.startswith('#'): continue
        a = ln.split(); f = int(a[0])
        if start is None and a[7] == '2' and a[8] == '53': start = f
        if start is None or f % 1000 in (0, 1, 2): continue
        ft, pace, swap, ct = float(a[1]), float(a[2]), float(a[3]), float(a[5])
        rows.append((ft, ft - pace - swap, ct))
    ft = [x[0] for x in rows]; wk = [x[1] for x in rows]; ct = [x[2] for x in rows]
    u = sum(1 for f in und if start and f >= start and f % 1000 not in (0, 1, 2))
    m = [x for x in mem if start and x[0] >= start]
    print(f"{r:22s} {len(ft):6d} {q(ft,.5):6.2f} {q(ft,.99):6.2f} {max(ft) if ft else 0:7.2f} {sum(1 for x in ft if x > 20):5d} | {q(wk,.5):8.2f} {q(wk,.99):6.2f} {max(wk) if wk else 0:6.2f} | {sum(ct)/max(1,len(ct)):7.2f} | {u:9d} | {m[0][1] if m else -1}->{m[-1][1] if m else -1} (run start {mem[0][1] if mem else -1}) | {m[0][2] if m else -1}->{m[-1][2] if m else -1}")
