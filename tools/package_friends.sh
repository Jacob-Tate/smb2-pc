#!/usr/bin/env bash
# tools/package_friends.sh - build the single-file "friend build": build/package/friends/SMB2.exe + README.txt
#
#   tools/package_friends.sh [--no-build]
#
# From Windows (Git Bash):
#   MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/<repo or worktree>/tools/package_friends.sh
#
# 1. builds the x64 game (tools/win_build.sh x64 --release; --no-build reuses build64/bin) and the launcher
#    (platforms/windows/launcher/build.sh)
# 2. archives build64/bin (exe + assets/, no .pdb/.debug/disc images) as "SMBPK001" and compresses it with
#    zstd -19 (needs the zstd CLI: sudo apt install zstd). The launcher decodes with the vendored
#    third_party/zstd/zstddeclib.c, so no Windows-only LZMS API and no new DLL.
# 3. SMB2.exe = launcher.exe + compressed archive + trailer (see launcher.cpp).
#
# Build id (16 chars, names the unpack folder; a different id = a new unpack): build_id() below, derived from
# the game's build id (F2); SMB2_BUILD_ID overrides.
set -euo pipefail

SRC=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BIN=$SRC/build64/bin
OUT=$SRC/build/package/friends
TMP=$SRC/build/package/tmp
command -v zstd >/dev/null || { echo "zstd missing: sudo apt install zstd" >&2; exit 1; }

if [ "${1:-}" != "--no-build" ]; then
    "$SRC/tools/win_build.sh" x64 --release
fi
[ -f "$BIN/smb2_pc_port.exe" ] || { echo "missing $BIN/smb2_pc_port.exe" >&2; exit 1; }
"$SRC/platforms/windows/launcher/build.sh" "$SRC/build/launcher"

rm -rf "$TMP"; mkdir -p "$TMP" "$OUT"

# Single place that decides the build id: exactly 16 chars of [0-9a-zA-Z._-], names the unpack folder.
# Source: the game's build id (build64/gen/build_id.txt, written by cmake/build_id.cmake:
# "<commit count>-<sha7>[+dirty]"), '+' -> '_', padded with '_' / cut to 16.  A dirty tree can differ
# without a new commit, so dirty ids end in 'd' + archive hash chars instead (a new unpack per content).
build_id() {   # $1 = uncompressed archive
    local id=""
    if [ -n "${SMB2_BUILD_ID:-}" ]; then
        id=$SMB2_BUILD_ID
    elif [ -f "$BIN/../gen/build_id.txt" ]; then
        id=$(head -n1 "$BIN/../gen/build_id.txt" | tr -d '\r\n')
        case $id in
            *+dirty) id="${id%+dirty}d$(sha256sum "$1" | head -c 16)" ;;
        esac
    else
        id=$(sha256sum "$1" | head -c 16)
    fi
    id=$(printf '%s' "$id" | tr -c '0-9a-zA-Z._-' '_' | head -c 16)
    printf '%-16s' "$id" | tr ' ' '_'
}

python3 -I - "$BIN" "$TMP/archive.bin" <<'PY'
import os, struct, sys
src, dst = sys.argv[1], sys.argv[2]
SKIP_EXT = {'.pdb', '.debug', '.iso', '.gcm', '.rvz', '.ilk', '.exp', '.lib', '.a', '.log', '.dat', '.ini'}
files = []
for root, dirs, names in os.walk(src):
    dirs.sort()
    for n in sorted(names):
        if os.path.splitext(n)[1].lower() in SKIP_EXT:
            continue
        p = os.path.join(root, n)
        files.append((os.path.relpath(p, src).replace(os.sep, '/'), p))
files.sort()
with open(dst, 'wb') as out:
    out.write(b'SMBPK001' + struct.pack('<I', len(files)))
    for rel, p in files:
        data = open(p, 'rb').read()
        r = rel.encode('utf-8')
        out.write(struct.pack('<H', len(r)) + r + struct.pack('<Q', len(data)) + data)
print('archive: %d files' % len(files))
PY

ID=$(build_id "$TMP/archive.bin")
[ ${#ID} -eq 16 ] || { echo "build id must be 16 chars: '$ID'" >&2; exit 1; }
zstd -q -19 --long=27 -f "$TMP/archive.bin" -o "$TMP/archive.zst"

python3 -I - "$SRC/build/launcher/launcher.exe" "$TMP/archive.zst" "$ID" "$OUT/SMB2.exe" <<'PY'
import struct, sys
launcher, arch, bid, dst = sys.argv[1:5]
l = open(launcher, 'rb').read()
a = open(arch, 'rb').read()
with open(dst, 'wb') as f:
    f.write(l); f.write(a)
    f.write(struct.pack('<QQ', len(l), len(a)) + bid.encode('ascii') + b'SMBLNCH1')
PY

cat > "$OUT/README.txt" <<'TXT'
Super Monkey Ball 2 (PC port) - single-file version
====================================================

SMB2.exe is the whole game in one file. You need your own copy of the game disc image
(Super Monkey Ball 2, USA / GM2E8P) as .iso, .gcm or .rvz. It is not included.
RVZ files must use zstd (Dolphin default) or no compression; others: re-convert in Dolphin.

Use
  1. Put SMB2.exe anywhere (Desktop, a USB stick, ...) and double-click it.
  2. The first start unpacks the game (a few seconds) and then asks for your disc image:
     pick your .iso / .rvz. It is remembered after that.
  3. To update, just replace SMB2.exe with the newer one. Your settings and memory card stay.
     Old unpacked versions are removed automatically.

Where things are stored (all in your user profile, nothing next to SMB2.exe)
  %LOCALAPPDATA%\SMB2\builds\   the unpacked game
  %APPDATA%\SMB2\friend\        settings, remembered disc path, memory card (your saves)

Removing everything
  Hold Shift while starting SMB2.exe (or run SMB2.exe --uninstall). A window lists exactly the two
  folders above and asks for confirmation. A separately installed SMB2 (installer) or the portable
  zip version keep their own data and are not touched, nor is your disc image. Then delete SMB2.exe.

Command line options of the game (e.g. --windowed, --iso <file>) can be given to SMB2.exe as usual.
TXT
rm -rf "$TMP"
echo "[package] build id $ID"
ls -l "$OUT/SMB2.exe"
