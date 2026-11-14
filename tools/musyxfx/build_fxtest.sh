#!/bin/bash
# Builds fxtest32.exe (i686 mingw gcc, like the 32-bit port) and fxtest64.exe
# (llvm-mingw clang, like the x64 port) from src/game/shim/musyx_fx.c into
# $MUSYXFX_OUT (default /tmp/musyxfx).  Then:
#   python3 fxcheck.py [blocks]   DOL (Gekko interpreter) vs port, bit for bit
#   python3 fxmeasure.py          reverb RT60, chorus delay/modulation
#   $MUSYXFX_OUT/fxtest32.exe bench 14000   CPU time per 5 ms tick
# LLVM_MINGW_ROOT: the x64 toolchain (skipped if unset/missing).
set -e
H=$(cd "$(dirname "$0")" && pwd); P=$(cd "$H/../.." && pwd)
OUT=${MUSYXFX_OUT:-/tmp/musyxfx}; mkdir -p "$OUT"
LLVM=${LLVM_MINGW_ROOT:-$P/../toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}
SRC="$H/fxtest.c $P/src/game/shim/musyx_fx.c $(ls $P/src/detmath/*.c $P/src/detmath/fdlibm/*.c | tr '\n' ' ')"
FL="-O2 -ffp-contract=off -std=gnu11 -DMUSYX_FX_TEST -include $P/include/pc_detmath.h -I$P/include -I$P/src/game/shim -I${SMB2_DECOMP_DIR:-$P/../decomp}/include"
i686-w64-mingw32-gcc -msse2 -mfpmath=sse $FL $SRC -o "$OUT/fxtest32.exe"
if [ -x "$LLVM/bin/x86_64-w64-mingw32-clang" ]; then
    "$LLVM/bin/x86_64-w64-mingw32-clang" -fms-extensions -include $P/include/pc_ptr32.h $FL $SRC -o "$OUT/fxtest64.exe"
fi
ls -la "$OUT"/fxtest*.exe
