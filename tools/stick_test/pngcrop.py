# pngcrop.py in.png out.ppm x y w h : crop an 8-bit RGB/RGBA PNG to PPM (no PIL)
import sys, zlib, struct
def load(p):
    d = open(p, 'rb').read(); i = 8; idat = b''
    while i < len(d):
        n, t = struct.unpack('>I4s', d[i:i+8]); c = d[i+8:i+8+n]; i += 12 + n
        if t == b'IHDR': w, h, bd, ct = struct.unpack('>IIBB', c[:10])
        elif t == b'IDAT': idat += c
    bpp = {2: 3, 6: 4}[ct]; raw = zlib.decompress(idat); s = w * bpp
    out = bytearray(h * s); prev = bytearray(s); o = 0
    for y in range(h):
        f = raw[o]; line = bytearray(raw[o+1:o+1+s]); o += 1 + s
        for x in range(s):
            a = line[x-bpp] if x >= bpp else 0; b = prev[x]; c = prev[x-bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        out[y*s:(y+1)*s] = line; prev = line
    return w, h, bpp, out
w, h, bpp, px = load(sys.argv[1]); x0, y0, cw, ch = map(int, sys.argv[3:7])
o = bytearray()
for y in range(y0, y0 + ch):
    for x in range(x0, x0 + cw):
        i = (y * w + x) * bpp; o += px[i:i+3]
open(sys.argv[2], 'wb').write(b'P6\n%d %d\n255\n' % (cw, ch) + bytes(o))
