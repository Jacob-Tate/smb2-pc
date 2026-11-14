"""Measurements on the port's MusyX effects (fxtest32.exe = musyx_fx.c):
reverb impulse response decay (RT60), chorus delay / modulation."""
import os, struct, subprocess, math, sys
W = os.environ.get('MUSYXFX_OUT', '/tmp/musyxfx')   # build_fxtest.sh output dir


def run(kind, params, blocks):
    fin, fout = os.path.join(W, 'm_in.bin'), os.path.join(W, 'm_out.bin')
    with open(fin, 'wb') as f:
        for blk in blocks:
            f.write(struct.pack('<480i', *blk))
    wp = lambda p: subprocess.check_output(['wslpath', '-w', p]).decode().strip()
    subprocess.check_call([os.path.join(W, 'fxtest32.exe'), kind] + [str(p) for p in params] + [wp(fin), wp(fout)])
    d = open(fout, 'rb').read()
    n = len(d) // 4
    return struct.unpack('<%di' % n, d)


def channel(out, c):
    res = []
    for b in range(len(out) // 480):
        res.extend(out[b * 480 + c * 160:b * 480 + c * 160 + 160])
    return res


def reverb_rt60(params, seconds=8):
    nb = seconds * 200
    blocks = [[0] * 480 for _ in range(nb)]
    for c in range(3):
        blocks[0][c * 160] = 1000000    # impulse (large to stay above the fctiwz truncation floor)
    out = run('rev', params, blocks)
    L = channel(out, 0)
    e = [x * x for x in L]
    e[0] = 0.0                           # drop the dry path
    # Schroeder backward integration
    tot = 0.0
    sch = [0.0] * len(e)
    for i in range(len(e) - 1, -1, -1):
        tot += e[i]
        sch[i] = tot
    db = [10 * math.log10(s / sch[1]) if s > 0 else -999 for s in sch]
    def t_at(level):
        for i, v in enumerate(db):
            if v <= level:
                return i / 32000.0
        return None
    t5, t25, t35, t65 = t_at(-5), t_at(-25), t_at(-35), t_at(-65)
    first = next(i for i, x in enumerate(L) if i > 0 and x != 0)
    peak_wet = max(abs(x) for x in L[1:])
    print('reverb %s' % params)
    print('  dry %d (of 1000000)  first wet sample at %.2f ms  wet peak %d' % (L[0], first / 32.0, peak_wet))
    print('  Schroeder: T20 (-5..-25 dB) -> RT60 %.2f s   T30 (-5..-35) -> RT60 %.2f s   -65 dB at %s s'
          % ((t25 - t5) * 3, (t35 - t5) * 2, '%.2f' % t65 if t65 else '>%d' % seconds))
    # energy in 0.5 s windows
    for k in range(0, seconds * 2):
        seg = e[k * 16000:(k + 1) * 16000]
        print('  %4.1f s  %7.1f dB' % (k * 0.5, 10 * math.log10(sum(seg) / 16000 + 1e-30) - 120))
    return L


def chorus_delay(params, seconds=3, label=''):
    """impulse every 4 blocks (20 ms); output delay = centroid of the response"""
    nb = seconds * 200
    blocks = [[0] * 480 for _ in range(nb)]
    for b in range(0, nb, 4):
        for c in range(3):
            blocks[b][c * 160] = 100000
    out = run('cho', params, blocks)
    L = channel(out, 0)
    res = []
    for b in range(0, nb - 8, 4):
        t0 = b * 160
        win = L[t0:t0 + 640]
        s = sum(abs(x) for x in win)
        if s == 0:
            continue
        cen = sum(i * abs(x) for i, x in enumerate(win)) / s
        res.append((t0 / 32000.0, cen / 32.0))
    ds = [d for _, d in res[5:]]
    print('chorus %s %s: delay min %.3f ms max %.3f ms (p-p %.3f ms)' % (params, label, min(ds), max(ds), max(ds) - min(ds)))
    # modulation period: time between successive maxima of the delay curve
    mx = [t for i, (t, d) in enumerate(res[1:-1], 1) if d >= res[i - 1][1] and d > res[i + 1][1] and d > min(ds) + 0.9 * (max(ds) - min(ds))]
    if len(mx) > 1:
        per = [b - a for a, b in zip(mx, mx[1:])]
        print('  delay maxima at %s s -> period %.3f s' % (', '.join('%.2f' % t for t in mx), sum(per) / len(per)))
    return res


if __name__ == '__main__':
    reverb_rt60([0.9, 0.65, 3.0, 0.6, 0.1, 0.0])
    chorus_delay([15, 0, 500], label='(game default)')
    chorus_delay([15, 7, 500])
    chorus_delay([20, 10, 250])
