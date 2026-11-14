#!/usr/bin/env python3
"""perf_frames.py frames.txt [dump_interval] -- per-phase frame-time stats.

frames.txt comes from SMB2_PERF_FRAMES=<file> with --perf (pass it through
WSLENV): one line per presented frame (frame ms pace swap texcreate copytex
audio mode sub), "#mem" lines every 60 frames (private bytes, working set,
GPU memory, GXInitTexObj textures/texels) and "#underrun" lines.
story_script.sh makes the Story input script, summary.py tabulates the
stage-clear -> stage-select window of several runs.
Phases: consecutive runs of (mode,sub); stage select (2/88) visits are
numbered.  Frames N and N+1 for N % dump_interval == 0 are excluded (frame
dumps + the trace they arm)."""
import sys
path = sys.argv[1]; di = int(sys.argv[2]) if len(sys.argv) > 2 else 0
rows, mem, und, mis = [], [], [], []
for ln in open(path):
    if ln.startswith('#mem'):
        p = ln.split(); mem.append((int(p[1]), int(p[3]) >> 20, int(p[5]) >> 20, (int(p[7]) >> 20) if len(p) > 7 else -1)); continue
    if ln.startswith('#missed'):
        p = ln.split(); mis.append((int(p[1]), int(p[3]))); continue
    if ln.startswith('#underrun'):
        p = ln.split(); und.append((int(p[1]), int(p[3]))); continue
    if ln.startswith('#'): continue
    p = ln.split()
    f = int(p[0])
    if di and (f % di == 0 or (f - 1) % di == 0): continue
    rows.append((f, float(p[1]), float(p[2]), float(p[3]), int(p[7]), int(p[8])))
def stats(v):
    v = sorted(v); n = len(v)
    if not n: return "-"
    q = lambda x: v[min(n - 1, int(x * n))]
    return f"n={n:5d} p50 {q(.5):6.2f} p99 {q(.99):6.2f} max {v[-1]:7.2f} >20ms {sum(1 for x in v if x > 20):4d} >33ms {sum(1 for x in v if x > 33.4):4d}"
segs = []
for r in rows:
    k = (r[4], r[5])
    if not segs or segs[-1][0] != k: segs.append([k, []])
    segs[-1][1].append(r)
ss = 0
print("phase(mode/sub) frames            frame ms                                       work ms (frame-pace-swap)")
for k, rs in segs:
    if len(rs) < 30: continue
    name = f"{k[0]}/{k[1]}"
    if k == (2, 88): ss += 1; name += f" stage-select#{ss}"
    print(f"{name:22s} {rs[0][0]:6d}-{rs[-1][0]:6d} {stats([r[1] for r in rs])} | work p50 {sorted(r[1]-r[2]-r[3] for r in rs)[len(rs)//2]:6.2f} p99 {sorted(r[1]-r[2]-r[3] for r in rs)[int(len(rs)*.99)]:6.2f}")
if mem:
    print("mem (frame: private MiB / ws MiB / gpu MiB):", ' '.join(f"{m[0]}:{m[1]}/{m[2]}/{m[3]}" for m in mem[::max(1, len(mem)//16)]), "last %d:%d/%d/%d" % mem[-1])
print("underruns:", len(und), (f"last total {und[-1][1]}; frames " + ' '.join(str(f) for f, _ in und[:40])) if und else "")
if mis:
    print("missed vblanks (DXGI):", mis[-1][1], "at frames", ' '.join(str(f) for f, _ in mis[:40]))
