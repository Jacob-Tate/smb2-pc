#!/usr/bin/env python3
"""Find globals defined with one type and declared with another (x64 hazard).

usage: crossdecl.py <decomp dir> [pc_port/src dir]

Reports every global whose definition is an integer / byte array (u8 x[N],
u32 x, int x, ...) while some other declaration ("extern ... x") uses a
pointer or struct type, or vice versa.  On 32-bit these alias harmlessly;
on x64 the pointer / struct is bigger than the storage.  See
docs/x64.md, Debugging and tools.
"""
import re, sys, glob, os
roots = sys.argv[1:]
files = []
for r in roots:
    files += glob.glob(os.path.join(r, '**', '*.c'), recursive=True)
    files += glob.glob(os.path.join(r, '**', '*.h'), recursive=True)
    files += glob.glob(os.path.join(r, '**', '*.inc'), recursive=True)
INT = r'(?:u8|s8|u16|s16|u32|s32|int|unsigned int|uptr|sptr|char|BOOL|float)'
defs, decls = {}, {}
dre = re.compile(r'^(?:static\s+)?(' + INT + r')\s+(\w+)\s*(\[[^\]]*\])?\s*(?:__attribute__\(\([^)]*\)\)\s*)?[;=]', re.M)
xre = re.compile(r'^\s*extern\s+([\w\s]+?)\s*(\*+)?\s*(\w+)\s*(\[[^\]]*\])?\s*;', re.M)
for f in files:
    t = open(f, errors='ignore').read()
    for m in dre.finditer(t):
        if m.group(0).startswith('static'):
            continue
        defs.setdefault(m.group(2), []).append((f, m.group(1) + (m.group(3) or '')))
    for m in xre.finditer(t):
        ty = m.group(1).strip()
        if m.group(2) or ty.startswith('struct '):
            decls.setdefault(m.group(3), []).append((f, ty + (m.group(2) or '') + (m.group(4) or '')))
n = 0
for name in sorted(set(defs) & set(decls)):
    for f, ty in defs[name]:
        others = sorted(set('%s (%s)' % (d, os.path.relpath(df, roots[0] + '/..')) for df, d in decls[name]))
        print('%-28s defined %-12s in %s; declared %s' % (name, ty, os.path.relpath(f, roots[0] + '/..'), '; '.join(others)))
        n += 1
print('# %d mismatching globals' % n, file=sys.stderr)
