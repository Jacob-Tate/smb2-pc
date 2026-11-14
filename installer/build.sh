#!/bin/bash
# Build the Windows installer: SMB2_PC_Setup_<version>.exe (app name "SMB2"; the
# updater and download links rely on the SMB2_PC_Setup_ file name)
#
#   installer/build.sh [options]
#     --version V        version string (default: $SMB2_VERSION or 1.0.0)
#     --exe32 PATH       use this i686 smb2_pc_port.exe (skip building it)
#     --exe64 PATH       use this x86_64 smb2_pc_port.exe (skip building it)
#     --no-build         use the exes already in <pc_port>/build32/bin and build64/bin
#     --test-root WINDIR build a TESTMODE installer (per-user, paths below a test root,
#                        HKCU\Software\SMB2Test + the old name's SMB2PCPortTest only); WINDIR is the default
#                        test root as a Windows path (override at run time with /TESTROOT=)
#     --out DIR          output directory (default: <pc_port>/installer/out)
#
# Environment:
#   MAKENSIS / NSISDIR      makensis to use.  Default: system makensis, else a
#                           local copy unpacked from the Ubuntu nsis debs into
#                           <pc_port>/../tools_nsis (no root needed)
#   LLVM_MINGW_ROOT         llvm-mingw toolchain for the x64 build
#                           (default <pc_port>/../toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64)
#   SMB2_DECOMP_DIR         decomp checkout (default <pc_port>/../decomp)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="$(cd "$HERE/.." && pwd)"
VERSION="${SMB2_VERSION:-1.0.0}"
EXE32="" EXE64="" NOBUILD=0 TESTROOT="" OUT="$HERE/out"
while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift 2;;
        --exe32) EXE32="$2"; shift 2;;
        --exe64) EXE64="$2"; shift 2;;
        --no-build) NOBUILD=1; shift;;
        --test-root) TESTROOT="$2"; shift 2;;
        --out) OUT="$2"; shift 2;;
        *) echo "unknown option $1" >&2; exit 2;;
    esac
done

# ---- makensis
if [ -z "${MAKENSIS:-}" ]; then
    if command -v makensis >/dev/null 2>&1; then
        MAKENSIS=makensis
    else
        LOCAL="$PORT/../tools_nsis"
        if [ ! -x "$LOCAL/root/usr/bin/makensis" ]; then
            echo "makensis not found; fetching the Ubuntu nsis packages into $LOCAL (no root)"
            mkdir -p "$LOCAL/debs"
            (cd "$LOCAL/debs" && apt download nsis nsis-common)
            for d in "$LOCAL"/debs/*.deb; do dpkg -x "$d" "$LOCAL/root"; done
        fi
        MAKENSIS="$LOCAL/root/usr/bin/makensis"
        export NSISDIR="$LOCAL/root/usr/share/nsis"
    fi
fi

# ---- game executables
DECOMP="${SMB2_DECOMP_DIR:-$(cd "$PORT/../decomp" && pwd)}"
build_one() { # dir toolchain extra...
    local dir="$1" tc="$2"; shift 2
    if [ ! -f "$PORT/$dir/build.ninja" ]; then
        cmake -S "$PORT" -B "$PORT/$dir" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$PORT/cmake/$tc" \
            -DSMB2_GAME=ON -DSMB2_DECOMP_DIR="$DECOMP" -DCMAKE_BUILD_TYPE=Release \
            -DSMB2_VERSION="$VERSION" "$@"
    else
        # re-run the configure step: the source lists are globbed, so new
        # files need a fresh glob; the version goes into the exe's VERSIONINFO
        cmake -DSMB2_VERSION="$VERSION" "$PORT/$dir" >/dev/null
    fi
    ninja -C "$PORT/$dir"
}
if [ -z "$EXE32" ]; then
    [ "$NOBUILD" = 1 ] || build_one build32 mingw32.cmake
    EXE32="$PORT/build32/bin/smb2_pc_port.exe"
fi
if [ -z "$EXE64" ]; then
    if [ "$NOBUILD" != 1 ]; then
        : "${LLVM_MINGW_ROOT:=$PORT/../toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}"
        [ -x "$LLVM_MINGW_ROOT/bin/x86_64-w64-mingw32-clang" ] || {
            echo "llvm-mingw not found at $LLVM_MINGW_ROOT: set LLVM_MINGW_ROOT (see README.md)" >&2; exit 1; }
        build_one build64 llvm-mingw64.cmake -DLLVM_MINGW_ROOT="$LLVM_MINGW_ROOT"
    fi
    EXE64="$PORT/build64/bin/smb2_pc_port.exe"
fi
for e in "$EXE32" "$EXE64"; do [ -f "$e" ] || { echo "missing $e" >&2; exit 1; }; done
file "$EXE32" | grep -q 'Intel 80386\|Intel i386' || { echo "$EXE32 is not an i686 PE" >&2; exit 1; }
file "$EXE64" | grep -q 'x86-64' || { echo "$EXE64 is not an x86_64 PE" >&2; exit 1; }

# ---- icon (generated, generic; never game artwork)
[ -f "$HERE/smb2.ico" ] || python3 "$HERE/make_icon.py" "$HERE/smb2.ico"

# a.b.c.d for the version resource
VIV="$(echo "$VERSION" | sed 's/[^0-9.].*$//')"
while [ "$(echo "$VIV" | tr -cd . | wc -c)" -lt 3 ]; do VIV="$VIV.0"; done
VIV="$(echo "$VIV" | cut -d. -f1-4)"

# installed size (KB) for Add/Remove Programs
docs_kb=$(( ( $(du -sb "$PORT/assets" | cut -f1) + $(stat -c %s "$HERE/README.txt" "$HERE/LICENSE.txt" "$HERE/smb2.ico" | paste -sd+ | bc) + 500000 ) / 1024 ))
KB32=$(( docs_kb + $(stat -c %s "$EXE32") / 1024 ))
KB64=$(( docs_kb + $(stat -c %s "$EXE64") / 1024 ))

OUT="$(realpath -m "$OUT")"   # makensis runs from $HERE
mkdir -p "$OUT"
DEFS=(-DVERSION="$VERSION" -DVIVERSION="$VIV" -DEXE32="$EXE32" -DEXE64="$EXE64" -DASSETS_DIR="$PORT/assets" -DPROG_KB_32=$KB32 -DPROG_KB_64=$KB64)
if [ -n "$TESTROOT" ]; then
    OUTFILE="$OUT/SMB2_PC_Setup_${VERSION}_TESTMODE.exe"
    DEFS+=(-DTESTMODE -DTESTROOT="$TESTROOT")
else
    OUTFILE="$OUT/SMB2_PC_Setup_${VERSION}.exe"
fi
cd "$HERE"
"$MAKENSIS" -V2 -WX "${DEFS[@]}" -DOUTFILE="$OUTFILE" smb2.nsi
ls -l "$OUTFILE"
sha256sum "$OUTFILE"
