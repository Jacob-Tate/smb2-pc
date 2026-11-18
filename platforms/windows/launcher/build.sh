#!/usr/bin/env bash
# Builds the friend-build launcher (x64 llvm-mingw, fully static) -> <out dir>/launcher.exe
#   platforms/windows/launcher/build.sh [out dir]      (run in WSL; tools/package_friends.sh calls it)
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SRC=$(cd "$HERE/../../.." && pwd)
SMB2_ROOT=${SMB2_ROOT:-/mnt/c/Users/jacob/source/super_monkey}
LLVM_MINGW_ROOT=${LLVM_MINGW_ROOT:-$SMB2_ROOT/toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}
OUT=${1:-$SRC/build/launcher}
mkdir -p "$OUT"
T=x86_64-w64-mingw32
CC=$LLVM_MINGW_ROOT/bin/$T-clang
CXX=$LLVM_MINGW_ROOT/bin/$T-clang++
WINDRES=$LLVM_MINGW_ROOT/bin/$T-windres
"$WINDRES" -I "$HERE" "$HERE/launcher.rc" -O coff -o "$OUT/launcher_rc.o"
"$CC" -O2 -w -c "$SRC/third_party/zstd/zstddeclib.c" -o "$OUT/zstd.o"
"$CXX" -std=c++17 -O2 -Wall -municode -mwindows -static -static-libgcc -static-libstdc++ \
    -fno-exceptions -Wl,--gc-sections -s \
    "$HERE/launcher.cpp" "$OUT/zstd.o" "$OUT/launcher_rc.o" -o "$OUT/launcher.exe" -lshell32 -lole32 -luuid
echo "[launcher] built $OUT/launcher.exe ($(stat -c %s "$OUT/launcher.exe") bytes)"
