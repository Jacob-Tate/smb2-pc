#!/usr/bin/env python3
"""Make the PC copy of a decomp C file's text match what the game expects.

1. Shift-JIS literals.  The decomp keeps some Japanese text as UTF-8
   literals; the GC build runs MWCC through sjiswrap, which re-encodes the
   source to Shift-JIS (code page 932).  GCC/Clang keep UTF-8 (clang cannot
   change its execution charset), so non-ASCII literals are rewritten as SJIS
   \\x escapes.  The bytes match the GC objects.

2. Missing European text.  The game's text tables have one entry per locale
   (English, German, French, Spanish, Italian, Japanese).  On the US disc the
   German/French/Spanish/Italian entries of about half of them were never
   written: "", "--", "---", "***" or placeholders like "NOCARD GER".  Those
   entries are replaced with the table's English entry, so a European
   language shows English where the disc has no translation.  English and
   Japanese never read these entries, so their text is unchanged.  A table
   is a brace list of string literals / char-array names made of records of
   6 locales x L lines (L = 1..4; one {...6...} per table, flattened lists of
   records, how_to.c's 2-line texts) with a non-empty plain-ASCII English
   first line in every record.

The PC build compiles the copies this script writes (CMakeLists.txt, see
docs/language.md).  Comments and code are copied as-is and line numbers do
not change.

usage: pc_decomp_text.py <in.c> <out.c>
       pc_decomp_text.py --list <files...>   (print the files that need it)
"""
import os
import re
import sys

ENC = "cp932"  # sjiswrap: Windows code page 932


# --------------------------------------------------------------- 1. SJIS
def escape_literal(body):
    """body: literal text between the quotes (source form, UTF-8)."""
    out = []
    i = 0
    last_was_hex = False
    while i < len(body):
        ch = body[i]
        if ch == "\\":
            j = i + 1
            if j < len(body) and body[j] == "x":
                j += 1
                while j < len(body) and body[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(body[i:j])
                last_was_hex = True
            else:
                j = min(j + 1, len(body))
                out.append(body[i:j])
                last_was_hex = False
            i = j
            continue
        if ord(ch) < 0x80:
            if last_was_hex and ch in "0123456789abcdefABCDEF":
                out.append('""')  # end the \x escape before a hex digit
            out.append(ch)
            last_was_hex = False
        else:
            for b in ch.encode(ENC):
                out.append("\\x%02X" % b)
            last_was_hex = True
        i += 1
    return "".join(out)


def convert(src):
    """SJIS-escape the non-ASCII string literals."""
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            j = src.find("\n", i)
            j = n if j < 0 else j
            out.append(src[i:j])
            i = j
        elif c == "/" and i + 1 < n and src[i + 1] == "*":
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(src[i:j])
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                if src[j] == "\\":
                    j += 1
                elif src[j] == "\n":
                    break
                j += 1
            body = src[i + 1:j]
            if any(ord(ch) >= 0x80 for ch in body):
                if c == "'":
                    raise SystemExit("non-ASCII char literal: %r" % body)
                body = escape_literal(body)
            out.append(c + body + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


# ------------------------------------------------- 2. missing European text
TOKEN = re.compile(r'''
    (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>"(?:[^"\\\n]|\\.)*")
  | (?P<char>'(?:[^'\\\n]|\\.)*')
  | (?P<ident>[A-Za-z_][A-Za-z0-9_]*)
  | (?P<number>\d[\w.]*)
  | (?P<ws>\s+)
  | (?P<punct>.)
''', re.S | re.X)


def unescape(lit):
    """bytes of one C string literal token (with quotes)."""
    s = lit[1:-1]
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\":
            n = s[i + 1]
            if n == "x":
                m = re.match(r"[0-9a-fA-F]+", s[i + 2:])
                out.append(int(m.group(0), 16) & 0xFF)
                i += 2 + len(m.group(0))
                continue
            if n in "01234567":
                m = re.match(r"[0-7]{1,3}", s[i + 1:])
                out.append(int(m.group(0), 8) & 0xFF)
                i += 1 + len(m.group(0))
                continue
            out += {"n": b"\n", "t": b"\t", "r": b"\r"}.get(n, n.encode("latin-1"))
            i += 2
            continue
        out += c.encode("latin-1")
        i += 1
    return bytes(out)


PLACEHOLDER = re.compile(rb"^[A-Z0-9_ ]+ (GER|FRA|SPA|ITA)$")


def missing(v):
    return v.strip(b"-*") == b"" or PLACEHOLDER.match(v) is not None


def plain_text(v):
    return v != b"" and all(0x20 <= b < 0x7F or b == 0x0A for b in v)


def fill_missing_european(src):
    toks = [(m.lastgroup, m.group(0), m.start(), m.end()) for m in TOKEN.finditer(src)]
    code = [t for t in toks if t[0] not in ("comment", "ws")]
    # char NAME[] = "...";  (sel_ngc_data.inc keeps its strings this way)
    arrays = {}
    for i in range(len(code) - 5):
        if code[i][1] == "char" and code[i + 1][0] == "ident" and code[i + 2][1] == "[":
            j = i + 3
            while j < len(code) and code[j][1] != "]":
                j += 1
            if j + 2 < len(code) and code[j + 1][1] == "=" and code[j + 2][0] == "string":
                k = j + 2
                v = b""
                while k < len(code) and code[k][0] == "string":
                    v += unescape(code[k][1])
                    k += 1
                if k < len(code) and code[k][1] == ";":
                    arrays[code[i + 1][1]] = v

    edits = []
    stack = []
    for i, t in enumerate(code):
        if t[1] == "{":
            stack.append(i)
        elif t[1] == "}" and stack:
            start = stack.pop()
            body = code[start + 1:i]
            if any(x[1] in "{}" for x in body):
                continue  # only innermost lists
            elems, cur = [], []
            for x in body:
                if x[1] == ",":
                    elems.append(cur)
                    cur = []
                else:
                    cur.append(x)
            if cur:
                elems.append(cur)
            vals = []
            for e in elems:
                if e and all(x[0] == "string" for x in e):
                    vals.append((b"".join(unescape(x[1]) for x in e), " ".join(x[1] for x in e), e))
                elif len(e) == 1 and e[0][0] == "ident" and e[0][1] in arrays:
                    vals.append((arrays[e[0][1]], e[0][1], e))
                else:
                    vals = None
                    break
            if not vals:
                continue
            # records of 6 locales x L lines (how_to.c keeps 2-line texts so)
            for L in (1, 2, 3, 4):
                if len(vals) % (6 * L) != 0:
                    continue
                recs = [vals[r:r + 6 * L] for r in range(0, len(vals), 6 * L)]
                if all(plain_text(r[0][0]) for r in recs):
                    break
            else:
                continue
            for r in recs:
                for k in range(1, 5):
                    lines = r[k * L:(k + 1) * L]
                    if all(missing(x[0]) for x in lines):
                        for n, x in enumerate(lines):
                            e = x[2]
                            edits.append((e[0][2], e[-1][3], r[n][1]))
    if not edits:
        return src
    out = []
    pos = 0
    for a, b, text in sorted(edits):
        out.append(src[pos:a])
        out.append(text)
        pos = b
    out.append(src[pos:])
    return "".join(out)


def process(src):
    return fill_missing_european(convert(src))


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "--list":
        for path in sys.argv[2:]:
            with open(path, encoding="utf-8") as f:
                src = f.read()
            if process(src) != src:
                print(path)
        return
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    with open(sys.argv[1], encoding="utf-8") as f:
        src = f.read()
    res = process(src)
    tmp = sys.argv[2] + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="") as f:
        f.write(res)
    os.replace(tmp, sys.argv[2])


if __name__ == "__main__":
    main()
