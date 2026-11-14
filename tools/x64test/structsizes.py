#!/usr/bin/env python3
"""List decomp structs whose size differs between the 32-bit and x64 builds.

usage: structsizes.py <decomp dir> <llvm-mingw root> [header.h ...]

Compiles one TU that includes every decomp header (or the given ones) and
records sizeof() of every complete "struct X" defined in them, once with
i686-w64-mingw32-gcc and once with the x64 clang (+ pc_ptr32.h), then prints
the structs whose sizes differ: "name size32 size64 header".  A struct that
lives in a disc/file image, a static table, a literal-size alloc/memset or an
overlay must have the SAME size (add DPTR / fix the code).  See
docs/x64_work_packages.md.
"""
import os, re, subprocess, sys, tempfile, glob

decomp, llvm = sys.argv[1], sys.argv[2]
heads = sys.argv[3:] or sorted(glob.glob(os.path.join(decomp, 'include', '*.h')))
pc_inc = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'include')
structs = {}
for h in heads:
    for m in re.finditer(r'^struct (\w+)\s*\n?\{', open(h, errors='ignore').read(), re.M):
        structs.setdefault(m.group(1), os.path.basename(h))
# headers that cannot be included standalone are skipped by trial
def build(cc, extra, hs):
    src = ''.join('#include "%s"\n' % os.path.basename(h) for h in hs)
    names = sorted(structs)
    src += 'unsigned int pc_sizes[] = {\n' + ''.join('  sizeof(struct %s),\n' % n for n in names) + '};\n'
    with tempfile.TemporaryDirectory() as d:
        c = os.path.join(d, 't.c'); open(c, 'w').write(src)
        r = subprocess.run([cc, '-S', '-o', '-', '-w', '-std=gnu99', '-fcommon',
                            '-I', os.path.join(decomp, 'include'),
                            '-I', os.path.join(decomp, 'build', 'GM2E8P', 'include')] + extra + [c],
                           capture_output=True, text=True)
    return r, names
def sizes(cc, extra):
    hs = list(heads)
    while True:
        r, names = build(cc, extra, hs)
        if r.returncode == 0:
            vals = [int(v, 0) for v in re.findall(r'\.(?:long|int)\s+(\S+)', r.stdout.split('pc_sizes:')[1])][:len(names)]
            return dict(zip(names, vals))
        # drop structs that are incomplete / headers that fail
        bad = set(re.findall(r"struct (\w+)[\u2019'\"]", r.stderr))
        badh = set(os.path.basename(f) for f in re.findall(r'^(\S+\.h):\d+:\d+: error', r.stderr, re.M))
        if not bad and not badh:
            sys.stderr.write(r.stderr[:3000]); sys.exit(1)
        for b in bad: structs.pop(b, None)
        hs = [h for h in hs if os.path.basename(h) not in badh]
        for n in [n for n, h in structs.items() if h in badh]: structs.pop(n)
s32 = sizes('i686-w64-mingw32-gcc', [])
s64 = sizes(os.path.join(llvm, 'bin', 'x86_64-w64-mingw32-clang'),
            ['-fms-extensions', '-include', os.path.join(pc_inc, 'pc_ptr32.h')])
diff = [(n, s32[n], s64[n], structs.get(n, '?')) for n in sorted(s32) if n in s64 and s32[n] != s64[n]]
for n, a, b, h in diff:
    print('%-32s 0x%-5X 0x%-5X %s' % (n, a, b, h))
print('# %d structs compared, %d differ' % (len(set(s32) & set(s64)), len(diff)), file=sys.stderr)
