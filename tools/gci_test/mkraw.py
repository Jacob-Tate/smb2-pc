#!/usr/bin/env python3
"""Builds GameCube memory card images (Dolphin .raw layout) and .gci variants
for the save import tests (tools/gci_test/gci_test.sh).

  mkraw.py card <out.raw> [--mbit N] [--scatter] [--stale-dir] [--other]
                [--corrupt header|dir|dir1|bat|chain] <file.gci>...
      Card with the given .gci files.  --scatter: data blocks out of order
      (the block map must be followed); --stale-dir: directory copy 1 holds an
      older, still valid directory without the files and copy 2 the current
      one, so the newer update counter must win; --other: adds another
      game's file, which must be skipped; --corrupt: break one structure.
  mkraw.py gci <in.gci> <out.gci> truncate|gamecode|region|blocks
      Damaged / foreign variants of a .gci.
  mkraw.py story <in> <out> gc|pc
      Game data save (.gci = GameCube byte order, or the port's .bin) with a
      Story Mode file in slot 1 (name GCIIO, world 3, 1:02:03, score 98765)
      and a valid checksum, to check that both games read it.
"""
import struct, sys

BLK = 0x2000


def checksum(b):
    a = c = 0
    for i in range(0, len(b) - 1, 2):
        v = (b[i] << 8) | b[i + 1]
        a = (a + v) & 0xFFFF
        c = (c + (v ^ 0xFFFF)) & 0xFFFF
    return (0 if a == 0xFFFF else a), (0 if c == 0xFFFF else c)


def header(mbit):
    h = bytearray(b'\xff' * BLK)
    h[0:0x26] = bytes(0x26)
    h[0:12] = b'SMB2TESTCARD'
    struct.pack_into('>QIIIHHH', h, 12, 0x1234567890, 0, 0, 0, 0, mbit, 0)
    h[0x26:0x1FA] = bytes(0x1FA - 0x26)
    struct.pack_into('>H', h, 0x1FA, 0)
    a, c = checksum(h[0:0x1FC])
    struct.pack_into('>HH', h, 0x1FC, a, c)
    return h


def directory(entries, counter):
    d = bytearray(b'\xff' * BLK)
    for i, e in enumerate(entries):
        d[i * 0x40:(i + 1) * 0x40] = e
    struct.pack_into('>H', d, 0x1FFA, counter)
    a, c = checksum(d[0:0x1FFC])
    struct.pack_into('>HH', d, 0x1FFC, a, c)
    return d


def bat(nblocks, chains, counter):
    b = bytearray(BLK)
    used = 0
    for chain in chains:
        for k, blk in enumerate(chain):
            nxt = chain[k + 1] if k + 1 < len(chain) else 0xFFFF
            struct.pack_into('>H', b, 0x0A + (blk - 5) * 2, nxt)
            used += 1
    struct.pack_into('>HHH', b, 4, counter, nblocks - 5 - used, max([max(c) for c in chains] or [4]))
    a, c = checksum(b[4:BLK])
    struct.pack_into('>HH', b, 0, a, c)
    return b


def card(args):
    out = args.pop(0)
    mbit, scatter, stale, other, corrupt = 16, False, False, False, None
    files = []
    while args:
        a = args.pop(0)
        if a == '--mbit': mbit = int(args.pop(0))
        elif a == '--scatter': scatter = True
        elif a == '--stale-dir': stale = True
        elif a == '--other': other = True
        elif a == '--corrupt': corrupt = args.pop(0)
        else: files.append(open(a, 'rb').read())
    if other:
        de = bytearray(files[0][0:0x40]) if files else bytearray(0x40)
        de[0:6] = b'GALE01'
        de[8:0x28] = b'SuperSmashBros0110290334'.ljust(32, b'\0')
        struct.pack_into('>H', de, 0x38, 1)
        files.append(bytes(de) + b'\x5a' * BLK)
    nblocks = mbit * 16
    img = bytearray(b'\0' * (nblocks * BLK))
    img[0:BLK] = header(mbit)
    nxt, entries, chains = 5, [], []
    for f in files:
        de = bytearray(f[0:0x40])
        n = struct.unpack_from('>H', de, 0x38)[0]
        chain = list(range(nxt, nxt + n))
        nxt += n
        if scatter:
            chain = chain[::-1]      # data in reverse block order
        struct.pack_into('>H', de, 0x36, chain[0])
        for k, blk in enumerate(chain):
            img[blk * BLK:(blk + 1) * BLK] = f[0x40 + k * BLK:0x40 + (k + 1) * BLK]
        entries.append(bytes(de))
        chains.append(chain)
    # --stale-dir: copy 1 older (counter 6, no files), copy 2 current (7)
    img[BLK:2 * BLK] = directory([] if stale else entries, 6 if stale else 7)
    img[2 * BLK:3 * BLK] = directory(entries, 7 if stale else 6)
    img[3 * BLK:4 * BLK] = bat(nblocks, chains, 7)
    img[4 * BLK:5 * BLK] = bat(nblocks, chains, 6)
    if corrupt == 'chain' and chains:
        # file 0's first link points past the end of the card (checksums kept valid)
        for o in (3 * BLK, 4 * BLK):
            b = bytearray(img[o:o + BLK])
            struct.pack_into('>H', b, 0x0A + (chains[0][0] - 5) * 2, nblocks + 3)
            a, c = checksum(b[4:BLK])
            struct.pack_into('>HH', b, 0, a, c)
            img[o:o + BLK] = b
    if corrupt == 'header':
        img[0x22] ^= 0x01
    elif corrupt == 'dir1':          # only copy 1: copy 2 must be used
        img[BLK + 5] ^= 0xFF
    elif corrupt == 'dir':
        img[BLK + 5] ^= 0xFF
        img[2 * BLK + 5] ^= 0xFF
    elif corrupt == 'bat':
        img[3 * BLK + 0x10] ^= 0xFF
        img[4 * BLK + 0x10] ^= 0xFF
    open(out, 'wb').write(img)


def gci(args):
    src, out, how = args
    g = bytearray(open(src, 'rb').read())
    if how == 'truncate':
        g = g[:len(g) - 3000]
    elif how == 'gamecode':
        g[0:4] = b'GALE'; g[4:6] = b'01'
    elif how == 'region':
        g[3] = ord('P')
    elif how == 'blocks':
        struct.pack_into('>H', g, 0x38, struct.unpack_from('>H', g, 0x38)[0] + 1)
    open(out, 'wb').write(g)


def crc(data):
    c = 0xFFFF
    for b in data:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) if c & 0x8000 else c << 1
            c &= 0xFFFF
    return c ^ 0xFFFF


def story(args):
    src, out, order = args
    raw = bytearray(open(src, 'rb').read())
    hdr = raw[:0x40] if len(raw) % BLK == 0x40 else b''
    d = raw[len(hdr):]
    e = '>' if order == 'gc' else '<'
    f = 0x2060   # StorySave slot 0 (decomp scen_mode.c)
    struct.pack_into(e + 'I', d, f + 4, 1)
    d[f + 8:f + 0x15] = b'GCIIO'.ljust(13, b'\0')
    d[f + 0x15] = 2
    struct.pack_into(e + 'III', d, f + 0x18, (3600 + 2 * 60 + 3) * 60, 98765, 42)
    struct.pack_into(e + 'H', d, 0, crc(d[2:]))
    open(out, 'wb').write(bytes(hdr) + bytes(d))


if __name__ == '__main__':
    cmd = sys.argv[1]
    {'card': card, 'gci': gci, 'story': story}[cmd](sys.argv[2:])
