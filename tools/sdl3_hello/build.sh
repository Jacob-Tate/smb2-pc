#!/usr/bin/env bash
# Build tools/sdl3_hello for both arches against mingw_deps/sdl3 (run in WSL).
# Output: tools/sdl3_hello/out/hello_{i686,x86_64}.exe
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SM_ROOT="${SM_ROOT:-/mnt/c/Users/jacob/source/super_monkey}"
DEPS="${SDL3_DEPS_DIR:-$SM_ROOT/smb2-pc/mingw_deps/sdl3}"
LLVM="${LLVM_MINGW_ROOT:-$SM_ROOT/toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}"
# System libs needed by static SDL3 (see docs/handoff/C2b.md)
SYSLIBS="-lsetupapi -lwinmm -limm32 -lversion -lole32 -loleaut32 -lcfgmgr32 -lhid -lgdi32 -luser32 -ladvapi32 -lshell32 -luuid -lrpcrt4 -lws2_32"
mkdir -p "$HERE/out"
i686-w64-mingw32-gcc -O2 -I"$DEPS/i686-w64-mingw32/include" "$HERE/hello.c" \
  -o "$HERE/out/hello_i686.exe" -static -static-libgcc \
  "$DEPS/i686-w64-mingw32/lib/libSDL3.a" $SYSLIBS
"$LLVM/bin/x86_64-w64-mingw32-clang" -O2 -I"$DEPS/x86_64-w64-mingw32/include" "$HERE/hello.c" \
  -o "$HERE/out/hello_x86_64.exe" -static \
  "$DEPS/x86_64-w64-mingw32/lib/libSDL3.a" $SYSLIBS
echo built
