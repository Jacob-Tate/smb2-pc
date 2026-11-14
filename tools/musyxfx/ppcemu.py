"""Minimal Gekko interpreter: runs main.dol functions directly from the DOL
bytes (integer, FP and the few paired-single ops MusyX's effects use).

FP semantics follow Dolphin (the reference): single-precision ops compute in
double and round to single; fmadds = round_single(double(a*c) + b) (product
exact in double); fctiwz truncates, fctiw rounds to nearest even; both
saturate.  Paired singles: each slot as a single op."""
import struct, math

import os
DOL = os.environ.get('SMB2_DOL', os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                            '../../../decomp/orig/GM2E8P/sys/main.dol'))
BASE = 0x80000000
SIZE = 0x01800000


def f32(x):
    """round a Python float (double) to single precision"""
    try:
        return struct.unpack('<f', struct.pack('<f', x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def bits32(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def s32(x):
    x &= 0xFFFFFFFF
    return x - 0x100000000 if x & 0x80000000 else x


def sat_int(r):
    if r != r:
        return 0x80000000
    if r >= 2147483648.0:
        return 0x7FFFFFFF
    if r < -2147483648.0:
        return 0x80000000
    return int(r) & 0xFFFFFFFF


def round_even(x):
    if x != x or math.isinf(x):
        return x
    f = math.floor(x)
    d = x - f
    if d > 0.5 or (d == 0.5 and (int(f) & 1)):
        return f + 1
    return f


class CPU:
    def __init__(self):
        self.mem = bytearray(SIZE)
        d = open(DOL, 'rb').read()
        for i in range(18):
            o, a, sz = (struct.unpack('>I', d[k:k + 4])[0] for k in (i * 4, 0x48 + i * 4, 0x90 + i * 4))
            if sz and a >= BASE:
                self.mem[a - BASE:a - BASE + sz] = d[o:o + sz]
        self.r = [0] * 32
        self.r[1] = 0x817F0000   # stack
        self.r[2] = 0x801A1400   # _SDA2_BASE_
        self.r[13] = 0x801A0840  # _SDA_BASE_
        self.f = [0.0] * 32      # ps0
        self.p1 = [0.0] * 32     # ps1
        self.cr = 0
        self.lr = self.ctr = 0
        self.ca = 0
        self.hooks = {}
        self.heap = 0x81000000
        self.count = 0

    # memory
    def rd32(self, a): return struct.unpack_from('>I', self.mem, a - BASE)[0]
    def wr32(self, a, v): struct.pack_into('>I', self.mem, a - BASE, v & 0xFFFFFFFF)
    def rd16(self, a): return struct.unpack_from('>H', self.mem, a - BASE)[0]
    def wr16(self, a, v): struct.pack_into('>H', self.mem, a - BASE, v & 0xFFFF)
    def rd8(self, a): return self.mem[a - BASE]
    def wr8(self, a, v): self.mem[a - BASE] = v & 0xFF
    def rdf(self, a): return struct.unpack_from('>f', self.mem, a - BASE)[0]
    def wrf(self, a, v): struct.pack_into('>f', self.mem, a - BASE, f32(v))
    def rdd(self, a): return struct.unpack_from('>d', self.mem, a - BASE)[0]
    def wrd(self, a, v): struct.pack_into('>d', self.mem, a - BASE, v)

    def malloc(self, n):
        a = self.heap
        self.heap = (self.heap + n + 31) & ~31
        self.mem[a - BASE:a - BASE + n] = bytes(n)
        return a

    def setcr(self, fld, lt, gt, eq, so=0):
        v = (lt << 3) | (gt << 2) | (eq << 1) | so
        sh = 28 - 4 * fld
        self.cr = (self.cr & ~(0xF << sh)) | (v << sh)

    def crbit(self, b): return (self.cr >> (31 - b)) & 1

    def setcrbit(self, b, v):
        m = 1 << (31 - b)
        self.cr = (self.cr | m) if v else (self.cr & ~m)

    def cmp_s(self, fld, a, b): a, b = s32(a), s32(b); self.setcr(fld, a < b, a > b, a == b)
    def cmp_u(self, fld, a, b): a &= 0xFFFFFFFF; b &= 0xFFFFFFFF; self.setcr(fld, a < b, a > b, a == b)

    def call(self, addr, maxsteps=50_000_000):
        SENT = 0x80000000 | 0x10
        self.lr = SENT
        pc = addr
        while pc != SENT:
            if pc in self.hooks:
                self.hooks[pc](self)
                pc = self.lr
                continue
            pc = self.step(pc)
            self.count += 1

    def step(self, pc):
        ins = self.rd32(pc)
        op = ins >> 26
        rD = (ins >> 21) & 31
        rA = (ins >> 16) & 31
        rB = (ins >> 11) & 31
        rC = (ins >> 6) & 31
        simm = s32((ins & 0xFFFF) | (0xFFFF0000 if ins & 0x8000 else 0))
        uimm = ins & 0xFFFF
        r = self.r
        npc = pc + 4
        ea0 = lambda: ((r[rA] if rA else 0) + simm) & 0xFFFFFFFF
        if op == 14:   # addi
            r[rD] = ((r[rA] if rA else 0) + simm) & 0xFFFFFFFF
        elif op == 15:  # addis
            r[rD] = ((r[rA] if rA else 0) + (simm << 16)) & 0xFFFFFFFF
        elif op == 7:   # mulli
            r[rD] = (s32(r[rA]) * simm) & 0xFFFFFFFF
        elif op == 8:   # subfic
            v = (simm & 0xFFFFFFFF) + ((~r[rA]) & 0xFFFFFFFF) + 1
            self.ca = v >> 32
            r[rD] = v & 0xFFFFFFFF
        elif op in (12, 13):  # addic, addic.
            v = r[rA] + (simm & 0xFFFFFFFF)
            self.ca = v >> 32
            r[rD] = v & 0xFFFFFFFF
            if op == 13:
                self.cmp_s(0, r[rD], 0)
        elif op == 10:  # cmpli
            self.cmp_u(rD >> 2, r[rA], uimm)
        elif op == 11:  # cmpi
            self.cmp_s(rD >> 2, r[rA], simm)
        elif op == 24:  # ori
            r[rA] = r[rD] | uimm
        elif op == 25:  # oris
            r[rA] = r[rD] | (uimm << 16)
        elif op == 26:  # xori
            r[rA] = r[rD] ^ uimm
        elif op == 27:  # xoris
            r[rA] = r[rD] ^ (uimm << 16)
        elif op == 28:  # andi.
            r[rA] = r[rD] & uimm
            self.cmp_s(0, r[rA], 0)
        elif op == 32: r[rD] = self.rd32(ea0())
        elif op == 33:
            a = ea0(); r[rD] = self.rd32(a); r[rA] = a
        elif op == 34: r[rD] = self.rd8(ea0())
        elif op == 40: r[rD] = self.rd16(ea0())
        elif op == 42: r[rD] = self.rd16(ea0()); r[rD] = r[rD] - 0x10000 & 0xFFFFFFFF if r[rD] & 0x8000 else r[rD]
        elif op == 36: self.wr32(ea0(), r[rD])
        elif op == 37:
            a = ea0(); self.wr32(a, r[rD]); r[rA] = a
        elif op == 38: self.wr8(ea0(), r[rD])
        elif op == 44: self.wr16(ea0(), r[rD])
        elif op == 46:  # lmw
            a = ea0()
            for k in range(rD, 32):
                r[k] = self.rd32(a); a += 4
        elif op == 47:  # stmw
            a = ea0()
            for k in range(rD, 32):
                self.wr32(a, r[k]); a += 4
        elif op == 48:
            v = self.rdf(ea0()); self.f[rD] = v; self.p1[rD] = v
        elif op == 49:
            a = ea0(); v = self.rdf(a); self.f[rD] = v; self.p1[rD] = v; r[rA] = a
        elif op == 50: self.f[rD] = self.rdd(ea0())
        elif op == 52: self.wrf(ea0(), self.f[rD])
        elif op == 54:
            v = self.f[rD]
            if isinstance(v, int):   # fctiw(z) result: 0xFFF80000_xxxxxxxx
                a = ea0(); self.wr32(a, 0xFFF80000); self.wr32(a + 4, v)
            else:
                self.wrd(ea0(), v)
        elif op == 18:  # b
            li = ins & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            tgt = (li if ins & 2 else pc + li) & 0xFFFFFFFF
            if ins & 1:
                self.lr = pc + 4
            npc = tgt
        elif op == 16:  # bc
            bo, bi = rD, rA
            bd = ins & 0xFFFC
            if bd & 0x8000:
                bd -= 0x10000
            if not (bo & 4):
                self.ctr = (self.ctr - 1) & 0xFFFFFFFF
            ctr_ok = (bo & 4) or ((self.ctr != 0) ^ ((bo >> 1) & 1))
            cond_ok = (bo & 16) or (self.crbit(bi) == ((bo >> 3) & 1))
            if ins & 1:
                self.lr = pc + 4
            if ctr_ok and cond_ok:
                npc = (pc + bd) & 0xFFFFFFFF
        elif op == 19:
            xo = (ins >> 1) & 0x3FF
            if xo == 16:  # bclr
                bo, bi = rD, rA
                if not (bo & 4):
                    self.ctr = (self.ctr - 1) & 0xFFFFFFFF
                ctr_ok = (bo & 4) or ((self.ctr != 0) ^ ((bo >> 1) & 1))
                cond_ok = (bo & 16) or (self.crbit(bi) == ((bo >> 3) & 1))
                if ctr_ok and cond_ok:
                    npc = self.lr
                if ins & 1:
                    self.lr = pc + 4
            elif xo == 528:  # bcctr
                npc = self.ctr
                if ins & 1:
                    self.lr = pc + 4
            elif xo == 449:  # cror
                self.setcrbit(rD, self.crbit(rA) | self.crbit(rB))
            elif xo == 0:  # mcrf
                v = (self.cr >> (28 - 4 * (rA >> 2))) & 0xF
                sh = 28 - 4 * (rD >> 2)
                self.cr = (self.cr & ~(0xF << sh)) | (v << sh)
            elif xo == 150:  # isync
                pass
            else:
                raise NotImplementedError('op19 xo %d at %08X' % (xo, pc))
        elif op == 21 or op == 20:  # rlwinm / rlwimi
            sh, mb, me = rB, rC, (ins >> 1) & 31
            v = r[rD] & 0xFFFFFFFF
            rot = ((v << sh) | (v >> (32 - sh))) & 0xFFFFFFFF if sh else v
            if mb <= me:
                m = ((0xFFFFFFFF >> mb) & (0xFFFFFFFF << (31 - me))) & 0xFFFFFFFF
            else:
                m = ((0xFFFFFFFF >> mb) | (0xFFFFFFFF << (31 - me))) & 0xFFFFFFFF
            if op == 21:
                r[rA] = rot & m
            else:
                r[rA] = (rot & m) | (r[rA] & ~m & 0xFFFFFFFF)
            if ins & 1:
                self.cmp_s(0, r[rA], 0)
        elif op == 31:
            npc = self.op31(ins, pc, rD, rA, rB, npc)
        elif op == 59:
            self.op59(ins, rD, rA, rB, rC)
        elif op == 63:
            self.op63(ins, rD, rA, rB, rC)
        elif op == 4:
            self.op4(ins, rD, rA, rB, rC)
        else:
            raise NotImplementedError('op %d at %08X' % (op, pc))
        return npc

    def op31(self, ins, pc, rD, rA, rB, npc):
        r = self.r
        xo = (ins >> 1) & 0x3FF
        rc = ins & 1
        xo9 = xo & 0x1FF
        ra0 = r[rA] if rA else 0
        res_reg = None
        if xo9 == 266:   # add
            r[rD] = (r[rA] + r[rB]) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 10:  # addc
            v = r[rA] + r[rB]; self.ca = v >> 32; r[rD] = v & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 138:  # adde
            v = r[rA] + r[rB] + self.ca; self.ca = v >> 32; r[rD] = v & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 40:  # subf
            r[rD] = (r[rB] - r[rA]) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 8:   # subfc
            v = ((~r[rA]) & 0xFFFFFFFF) + r[rB] + 1; self.ca = v >> 32; r[rD] = v & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 136:  # subfe
            v = ((~r[rA]) & 0xFFFFFFFF) + r[rB] + self.ca; self.ca = v >> 32; r[rD] = v & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 104:  # neg
            r[rD] = (-r[rA]) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 235:  # mullw
            r[rD] = (s32(r[rA]) * s32(r[rB])) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 75:  # mulhw
            r[rD] = ((s32(r[rA]) * s32(r[rB])) >> 32) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 11:  # mulhwu
            r[rD] = ((r[rA] * r[rB]) >> 32) & 0xFFFFFFFF; res_reg = rD
        elif xo9 == 459:  # divwu
            r[rD] = (r[rA] // r[rB]) if r[rB] else 0; res_reg = rD
        elif xo9 == 491:  # divw
            a, b = s32(r[rA]), s32(r[rB])
            r[rD] = (int(a / b) & 0xFFFFFFFF) if b else 0; res_reg = rD
        elif xo == 28: r[rA] = r[rD] & r[rB]; res_reg = rA
        elif xo == 444: r[rA] = r[rD] | r[rB]; res_reg = rA
        elif xo == 316: r[rA] = r[rD] ^ r[rB]; res_reg = rA
        elif xo == 124: r[rA] = (~(r[rD] | r[rB])) & 0xFFFFFFFF; res_reg = rA
        elif xo == 60: r[rA] = r[rD] & ~r[rB] & 0xFFFFFFFF; res_reg = rA
        elif xo == 24:  # slw
            n = r[rB] & 63; r[rA] = (r[rD] << n) & 0xFFFFFFFF if n < 32 else 0; res_reg = rA
        elif xo == 536:  # srw
            n = r[rB] & 63; r[rA] = (r[rD] >> n) if n < 32 else 0; res_reg = rA
        elif xo == 824:  # srawi
            n = rB; v = s32(r[rD])
            self.ca = 1 if (v < 0 and (v & ((1 << n) - 1))) else 0
            r[rA] = (v >> n) & 0xFFFFFFFF; res_reg = rA
        elif xo == 792:  # sraw
            n = r[rB] & 63; v = s32(r[rD])
            if n > 31: n = 31
            self.ca = 1 if (v < 0 and (v & ((1 << n) - 1))) else 0
            r[rA] = (v >> n) & 0xFFFFFFFF; res_reg = rA
        elif xo == 26:  # cntlzw
            v = r[rD]; n = 0
            while n < 32 and not (v & (0x80000000 >> n)): n += 1
            r[rA] = n; res_reg = rA
        elif xo == 922:  # extsh
            v = r[rD] & 0xFFFF; r[rA] = (v - 0x10000) & 0xFFFFFFFF if v & 0x8000 else v; res_reg = rA
        elif xo == 954:  # extsb
            v = r[rD] & 0xFF; r[rA] = (v - 0x100) & 0xFFFFFFFF if v & 0x80 else v; res_reg = rA
        elif xo == 0: self.cmp_s(rD >> 2, r[rA], r[rB])
        elif xo == 32: self.cmp_u(rD >> 2, r[rA], r[rB])
        elif xo == 23: r[rD] = self.rd32((ra0 + r[rB]) & 0xFFFFFFFF)
        elif xo == 55:
            a = (r[rA] + r[rB]) & 0xFFFFFFFF; r[rD] = self.rd32(a); r[rA] = a
        elif xo == 87: r[rD] = self.rd8((ra0 + r[rB]) & 0xFFFFFFFF)
        elif xo == 279: r[rD] = self.rd16((ra0 + r[rB]) & 0xFFFFFFFF)
        elif xo == 343:
            v = self.rd16((ra0 + r[rB]) & 0xFFFFFFFF); r[rD] = (v - 0x10000) & 0xFFFFFFFF if v & 0x8000 else v
        elif xo == 151: self.wr32((ra0 + r[rB]) & 0xFFFFFFFF, r[rD])
        elif xo == 183:
            a = (r[rA] + r[rB]) & 0xFFFFFFFF; self.wr32(a, r[rD]); r[rA] = a
        elif xo == 215: self.wr8((ra0 + r[rB]) & 0xFFFFFFFF, r[rD])
        elif xo == 407: self.wr16((ra0 + r[rB]) & 0xFFFFFFFF, r[rD])
        elif xo == 535:  # lfsx
            v = self.rdf((ra0 + r[rB]) & 0xFFFFFFFF); self.f[rD] = v; self.p1[rD] = v
        elif xo == 567:  # lfsux
            a = (r[rA] + r[rB]) & 0xFFFFFFFF; v = self.rdf(a); self.f[rD] = v; self.p1[rD] = v; r[rA] = a
        elif xo == 663: self.wrf((ra0 + r[rB]) & 0xFFFFFFFF, self.f[rD])
        elif xo == 983:  # stfiwx
            v = self.f[rD]
            self.wr32((ra0 + r[rB]) & 0xFFFFFFFF, v if isinstance(v, int) else bits_lo(v))
        elif xo == 339:  # mfspr
            spr = ((ins >> 16) & 31) | (((ins >> 11) & 31) << 5)
            r[rD] = {8: self.lr, 9: self.ctr, 1: self.ca << 29}[spr]
        elif xo == 467:  # mtspr
            spr = ((ins >> 16) & 31) | (((ins >> 11) & 31) << 5)
            if spr == 8: self.lr = r[rD]
            elif spr == 9: self.ctr = r[rD]
            elif spr == 1: self.ca = (r[rD] >> 29) & 1
            else: raise NotImplementedError('mtspr %d' % spr)
        elif xo == 512:  # mcrxr
            fld = rD >> 2
            self.setcr(fld, 0, 0, self.ca, 0)   # LT=SO GT=OV EQ=CA
            self.ca = 0
        elif xo == 19:  # mfcr
            r[rD] = self.cr
        elif xo == 86 or xo == 54 or xo == 278 or xo == 1014 or xo == 598:  # dcbf dcbst dcbt dcbz sync
            pass
        else:
            raise NotImplementedError('op31 xo %d at %08X' % (xo, pc))
        if rc and res_reg is not None:
            self.cmp_s(0, r[res_reg], 0)
        return npc

    def fwr(self, d, v, single=True):
        v = f32(v) if single else v
        self.f[d] = v
        if single:
            self.p1[d] = v

    def op59(self, ins, d, a, b, c):
        F = self.f
        xo = (ins >> 1) & 31
        if xo == 21: self.fwr(d, F[a] + F[b])
        elif xo == 20: self.fwr(d, F[a] - F[b])
        elif xo == 25: self.fwr(d, F[a] * F[c])
        elif xo == 18: self.fwr(d, F[a] / F[b] if F[b] != 0 else math.copysign(math.inf, F[a]) * math.copysign(1, F[b]))
        elif xo == 29: self.fwr(d, F[a] * F[c] + F[b])         # fmadds
        elif xo == 28: self.fwr(d, F[a] * F[c] - F[b])         # fmsubs
        elif xo == 31: self.fwr(d, -(F[a] * F[c] + F[b]))      # fnmadds
        elif xo == 30: self.fwr(d, -(F[a] * F[c] - F[b]))      # fnmsubs
        else:
            raise NotImplementedError('op59 xo %d' % xo)

    def op63(self, ins, d, a, b, c):
        F = self.f
        xo = (ins >> 1) & 0x3FF
        xo5 = xo & 31
        if xo == 0 or xo == 32:  # fcmpu / fcmpo
            x, y = F[a], F[b]
            un = x != x or y != y
            self.setcr(d >> 2, (x < y) and not un, (x > y) and not un, (x == y) and not un, 1 if un else 0)
        elif xo == 72: F[d] = F[b]
        elif xo == 40: F[d] = -F[b]
        elif xo == 264: F[d] = abs(F[b])
        elif xo == 12: self.fwr(d, F[b])     # frsp
        elif xo == 15:   # fctiwz
            F[d] = sat_int(math.trunc(F[b]) if F[b] == F[b] and not math.isinf(F[b]) else F[b])
        elif xo == 14:   # fctiw (RN = nearest)
            F[d] = sat_int(round_even(F[b]))
        elif xo5 == 21: F[d] = F[a] + F[b]
        elif xo5 == 20: F[d] = F[a] - F[b]
        elif xo5 == 25: F[d] = F[a] * F[c]
        elif xo5 == 18: F[d] = F[a] / F[b]
        elif xo5 == 29: F[d] = math.fma(F[a], F[c], F[b]) if hasattr(math, 'fma') else F[a] * F[c] + F[b]
        else:
            raise NotImplementedError('op63 xo %d' % xo)

    def op4(self, ins, d, a, b, c):
        F, P = self.f, self.p1
        xo10 = (ins >> 1) & 0x3FF
        xo5 = (ins >> 1) & 31
        if xo10 == 528:   # ps_merge00
            F[d], P[d] = F[a], F[b]
        elif xo5 == 12:   # ps_muls0
            x0, x1 = f32(F[a] * F[c]), f32(P[a] * F[c]); F[d], P[d] = x0, x1
        elif xo5 == 13:   # ps_muls1
            x0, x1 = f32(F[a] * P[c]), f32(P[a] * P[c]); F[d], P[d] = x0, x1
        elif xo5 == 25:   # ps_mul
            x0, x1 = f32(F[a] * F[c]), f32(P[a] * P[c]); F[d], P[d] = x0, x1
        elif xo5 == 10:   # ps_sum0
            x0, x1 = f32(F[a] + P[b]), P[c]; F[d], P[d] = x0, x1
        elif xo5 == 11:   # ps_sum1
            x0, x1 = F[c], f32(F[a] + P[b]); F[d], P[d] = x0, x1
        else:
            raise NotImplementedError('op4 xo %d' % xo10)


def bits_lo(v):
    """stfiwx of a non-integer FPR: low word of the double"""
    return struct.unpack('<Q', struct.pack('<d', v))[0] & 0xFFFFFFFF
