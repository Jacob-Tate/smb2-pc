#!/usr/bin/env python3
"""Generate src/game/shim/mathutil_tables_gen.c from GM2E8P main.dol.

mathutil's trig on the GameCube is table driven (hand-written Sega asm):
  0x8011BF60  float[0x4001]  sin(i * pi/2 / 0x4000), quarter wave
  0x8012BF80  float[0x4001]  tan(i * pi/2 / 0x4000)
  0x8013BFA0  float[0x1000][2]  atan(r) for r in [i/4096, (i+1)/4096):
                               angle_s16 = fctiw(r * a[i] + b[i])
The tables are not reproducible from a libm (about 1000 of the sin entries
differ from (float)sin(x)), so they are copied from the DOL, stored as float
bit patterns.  See docs/detmath.md.

usage: gen_mathutil_tables.py <main.dol> <out.c>
"""
import struct
import sys

TABLES = [
    ("g_mathutil_sin_table", 0x8011BF60, 0x4001),
    ("g_mathutil_tan_table", 0x8012BF80, 0x4001),
    ("g_mathutil_atan_table", 0x8013BFA0, 0x2000),
]


def main():
    dol = open(sys.argv[1], "rb").read()
    offs = struct.unpack(">18I", dol[0:72])
    addrs = struct.unpack(">18I", dol[72:144])
    sizes = struct.unpack(">18I", dol[144:216])

    def read(addr, n):
        for o, a, s in zip(offs, addrs, sizes):
            if a <= addr and addr + n <= a + s:
                return dol[o + addr - a:o + addr - a + n]
        raise SystemExit("0x%08X not in a DOL section" % addr)

    out = []
    out.append("/* Auto-generated from GM2E8P main.dol by tools/gen_mathutil_tables.py.\n"
               " * mathutil's sin/tan/atan lookup tables (float bit patterns); the DOL's\n"
               " * table-driven math_sin/math_tan/math_atan2 asm reads them.  See\n"
               " * docs/detmath.md. */\n")
    out.append("#include <stdint.h>\n")
    for name, addr, count in TABLES:
        words = struct.unpack(">%dI" % count, read(addr, count * 4))
        out.append("\n/* 0x%08X */\nconst uint32_t %s[%d] = {\n" % (addr, name, count))
        for i in range(0, count, 8):
            out.append("    " + " ".join("0x%08X," % w for w in words[i:i + 8]) + "\n")
        out.append("};\n")
    sin_t = struct.unpack(">%df" % 0x4001, read(0x8011BF60, 0x4001 * 4))
    assert sin_t[0] == 0.0 and sin_t[0x4000] == 1.0, "sin table sanity"
    open(sys.argv[2], "w").write("".join(out))


if __name__ == "__main__":
    main()
