#!/usr/bin/env python3
"""Generate smb2.ico: a generic glossy ball (no Sega/Nintendo artwork).

Pure Python (no PIL).  Writes a Windows .ico with 16/24/32/48/64/256 px
32-bit BGRA BMP entries.  Usage: make_icon.py <out.ico>
"""
import math
import struct
import sys

SS = 4  # supersampling factor per axis


def clamp(v):
    return 0 if v < 0 else 255 if v > 255 else int(v + 0.5)


def shade(u, v):
    """Colour at normalised point (u, v) in [-1, 1]^2 -> (r, g, b, a) floats."""
    r2 = u * u + v * v
    if r2 > 1.0:
        return (0.0, 0.0, 0.0, 0.0)
    z = math.sqrt(1.0 - r2)
    # light from upper left, front
    lx, ly, lz = -0.45, -0.55, 0.70
    ln = math.sqrt(lx * lx + ly * ly + lz * lz)
    lx, ly, lz = lx / ln, ly / ln, lz / ln
    diff = max(0.0, u * lx + v * ly + z * lz)
    # inner core (abstract figure inside a clear ball): warm orange disc
    cu, cv = u + 0.05, v - 0.12
    core = cu * cu + cv * cv < 0.36 ** 2
    if core:
        base = (240.0, 150.0, 40.0)
    else:
        base = (40.0, 120.0, 230.0)
    amb = 0.35
    col = [c * (amb + (1.0 - amb) * diff) for c in base]
    # rim darkening
    rim = (1.0 - z) ** 2
    col = [c * (1.0 - 0.45 * rim) for c in col]
    # specular highlight
    hx, hy, hz = lx, ly, lz + 1.0
    hn = math.sqrt(hx * hx + hy * hy + hz * hz)
    spec = max(0.0, (u * hx + v * hy + z * hz) / hn) ** 40
    col = [c + 255.0 * 0.9 * spec for c in col]
    # thin dark outline for small sizes
    if r2 > 0.92 ** 2:
        col = [c * 0.55 for c in col]
    return (col[0], col[1], col[2], 255.0)


def render(size):
    px = []
    margin = 0.5 if size <= 16 else 1.0
    radius = size / 2.0 - margin
    cx = cy = size / 2.0
    for y in range(size):
        row = []
        for x in range(size):
            acc = [0.0, 0.0, 0.0, 0.0]
            for sy in range(SS):
                for sx in range(SS):
                    fx = x + (sx + 0.5) / SS
                    fy = y + (sy + 0.5) / SS
                    r, g, b, a = shade((fx - cx) / radius, (fy - cy) / radius)
                    acc[0] += r * a / 255.0
                    acc[1] += g * a / 255.0
                    acc[2] += b * a / 255.0
                    acc[3] += a
            n = SS * SS
            a = acc[3] / n
            if a > 0:
                r = acc[0] / n * 255.0 / a
                g = acc[1] / n * 255.0 / a
                b = acc[2] / n * 255.0 / a
            else:
                r = g = b = 0
            row.append((clamp(r), clamp(g), clamp(b), clamp(a)))
        px.append(row)
    return px


def bmp_entry(size):
    px = render(size)
    hdr = struct.pack('<IiiHHIIiiII', 40, size, size * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    xor = bytearray()
    for y in range(size - 1, -1, -1):  # bottom-up
        for (r, g, b, a) in px[y]:
            xor += bytes((b, g, r, a))
    mask_stride = ((size + 31) // 32) * 4
    andm = bytearray()
    for y in range(size - 1, -1, -1):
        bits = bytearray(mask_stride)
        for x, (_, _, _, a) in enumerate(px[y]):
            if a == 0:
                bits[x // 8] |= 0x80 >> (x % 8)
        andm += bits
    return hdr + bytes(xor) + bytes(andm)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else 'smb2.ico'
    sizes = [16, 24, 32, 48, 64, 256]
    blobs = [bmp_entry(s) for s in sizes]
    data = struct.pack('<HHH', 0, 1, len(sizes))
    off = 6 + 16 * len(sizes)
    for s, b in zip(sizes, blobs):
        data += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0, 1, 32, len(b), off)
        off += len(b)
    for b in blobs:
        data += b
    with open(out, 'wb') as f:
        f.write(data)


if __name__ == '__main__':
    main()
