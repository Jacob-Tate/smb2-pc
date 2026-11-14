"""Step-by-step check of the port's MusyX effects against the DOL code.

Runs main.dol's own sndAuxCallbackPrepareReverbHI / sndAuxCallbackReverbHI /
sndAuxCallbackUpdateSettingsReverbHI and sndAuxCallbackPrepareChorus /
sndAuxCallbackChorus / sndAuxCallbackUpdateSettingsChorus in a Gekko
interpreter (ppcemu.py) and the port's musyx_fx.c (fxtest32/64.exe, built by build_fxtest.sh) on the
same input, and compares every output sample bit for bit.

  python3 fxcheck.py [nblocks]"""
import sys, os, struct, random, subprocess, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ppcemu import CPU, f32

W = os.environ.get('MUSYXFX_OUT', '/tmp/musyxfx')   # build_fxtest.sh output dir
NB = int(sys.argv[1]) if len(sys.argv) > 1 else 60


def mk_input(nb, seed):
    rnd = random.Random(seed)
    blocks = []
    for b in range(nb):
        blk = [0] * 480
        for c in range(3):
            for i in range(160):
                n = b * 160 + i
                if n == 0:
                    v = 20000 if c != 1 else -15000          # impulse
                elif 40 <= b < 60:
                    v = int(rnd.gauss(0, 3000))               # noise burst
                elif 60 <= b < 70:
                    v = int(8000 * math.sin(n * 0.05 * (c + 1)))   # tones
                else:
                    v = 0
                blk[c * 160 + i] = v
        blocks.append(blk)
    return blocks


def hooks(cpu):
    def salMalloc(c):
        c.r[3] = c.malloc(c.r[3])
    def memset(c):
        d, v, n = c.r[3], c.r[4] & 0xFF, c.r[5]
        c.mem[d - 0x80000000:d - 0x80000000 + n] = bytes([v]) * n
    def powf(c):
        c.f[1] = f32(math.pow(c.f[1], c.f[2]))
    def salFree(c):
        pass
    cpu.hooks[0x80052008] = salMalloc
    cpu.hooks[0x80052030] = salFree
    cpu.hooks[0x800033A8] = memset
    cpu.hooks[0x80076890] = powf


def emu_run(kind, params, blocks, update=None):
    cpu = CPU()
    hooks(cpu)
    st = cpu.malloc(0x200)
    buf = cpu.malloc(480 * 4)
    info = cpu.malloc(16)
    cpu.wr32(info, buf); cpu.wr32(info + 4, buf + 640); cpu.wr32(info + 8, buf + 1280)
    if kind == 'rev':
        for k, v in enumerate(params):   # coloration mix time damping preDelay crosstalk
            cpu.wrf(st + 0x1C8 + 4 * k, v)
        cpu.r[3] = st
        cpu.call(0x80052110)
        assert cpu.r[3] == 1, 'prepare failed'
        cb = 0x8005205C
    else:
        for k, v in enumerate(params):
            cpu.wr32(st + 0x90 + 4 * k, v)
        cpu.r[3] = st
        cpu.call(0x80053834)
        assert cpu.r[3] == 1
        cb = 0x80053430
    out = []
    for bi, blk in enumerate(blocks):
        if update and bi == update[0]:
            if kind == 'rev':
                for k, v in enumerate(update[1]):
                    cpu.wrf(st + 0x1C8 + 4 * k, v)
                cpu.r[3] = st
                cpu.call(0x800520B8)
            else:
                for k, v in enumerate(update[1]):
                    cpu.wr32(st + 0x90 + 4 * k, v)
                cpu.r[3] = st
                cpu.call(0x8005379C)
        for i, v in enumerate(blk):
            cpu.wr32(buf + 4 * i, v)
        cpu.r[3], cpu.r[4], cpu.r[5] = 0, info, st
        cpu.call(cb)
        out.append([struct.unpack('>i', cpu.mem[buf + 4 * i - 0x80000000:buf + 4 * i + 4 - 0x80000000])[0]
                    for i in range(480)])
    return out, cpu.count


def port_run(exe, kind, params, blocks, update=None):
    """update (block, params): split into two runs is not possible with the
    exe (state); instead the exe gets an 'U' marker file - not supported, so
    updates are checked by the emu-only comparison of two runs."""
    fin, fout = os.path.join(W, 'fx_in.bin'), os.path.join(W, 'fx_out.bin')
    with open(fin, 'wb') as f:
        for blk in blocks:
            f.write(struct.pack('<480i', *blk))
    args = [os.path.join(W, exe), kind] + ['%.9g' % p if isinstance(p, float) else str(p) for p in params]
    args += [subprocess.check_output(['wslpath', '-w', fin]).decode().strip(),
             subprocess.check_output(['wslpath', '-w', fout]).decode().strip()]
    if update:
        args += ['upd', str(update[0])] + ['%.9g' % p if isinstance(p, float) else str(p) for p in update[1]]
    subprocess.check_call(args, cwd=W)
    d = open(fout, 'rb').read()
    return [list(struct.unpack_from('<480i', d, 1920 * b)) for b in range(len(blocks))]


def compare(name, a, b):
    bad = 0
    first = None
    for bi, (x, y) in enumerate(zip(a, b)):
        for i in range(480):
            if x[i] != y[i]:
                bad += 1
                if first is None:
                    first = (bi, i, x[i], y[i])
    peak = max(abs(v) for blk in a for v in blk)
    print('%-34s %5d blocks  peak %6d  mismatches %d%s' % (name, len(a), peak, bad,
          '' if not first else '  first: block %d idx %d dol %d port %d' % first))
    return bad == 0


ok = True
blocks = mk_input(NB, 1)
cases = [
    ('rev', 'reverb game default', [0.9, 0.65, 3.0, 0.6, 0.1, 0.0]),
    ('rev', 'reverb crosstalk/no predelay', [0.5, 1.0, 1.0, 0.0, 0.0, 0.7]),
    ('rev', 'reverb short predelay 2ms', [0.3, 0.3, 0.5, 1.0, 0.002, 0.25]),
    ('cho', 'chorus game default', [15, 0, 500]),
    ('cho', 'chorus modulated 15/7/500', [15, 7, 500]),
    ('cho', 'chorus 20/15/50 (fast, deep)', [20, 15, 50]),
    ('cho', 'chorus 5/3/200', [5, 3, 200]),
]
cases += [
    ('rev', 'reverb update @30 -> 0.5/0.9/1.5/0.2/0.05/0.3', [0.9, 0.65, 3.0, 0.6, 0.1, 0.0],
     (30, [0.5, 0.9, 1.5, 0.2, 0.05, 0.3])),
    ('rev', 'reverb update @30 -> predelay 5 (rejected)', [0.9, 0.65, 3.0, 0.6, 0.1, 0.0],
     (30, [0.9, 0.65, 3.0, 0.6, 5.0, 0.0])),
    ('cho', 'chorus update @30 -> 25/10/300', [15, 0, 500], (30, [25, 10, 300])),
]
for case in cases:
    kind, name, params = case[:3]
    upd = case[3] if len(case) > 3 else None
    p = [f32(x) for x in params] if kind == 'rev' else params
    if upd and kind == 'rev':
        upd = (upd[0], [f32(x) for x in upd[1]])
    dol, n = emu_run(kind, p, blocks, upd)
    for exe in ('fxtest32.exe', 'fxtest64.exe'):
        port = port_run(exe, kind, p, blocks, upd)
        ok &= compare('%s [%s]' % (name, exe[6:8]), dol, port)
print('ALL BIT-EXACT' if ok else 'MISMATCH')
