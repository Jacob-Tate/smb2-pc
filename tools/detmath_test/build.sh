#!/bin/bash
# Build detmath_test with every toolchain the port uses (docs/detmath.md):
#   detmath_test_i686.exe  i686-w64-mingw32-gcc (msvcrt)   = 32-bit game build
#   detmath_test_x64.exe   llvm-mingw x86_64 clang (UCRT)  = x64 game build
#   detmath_test_i686c.exe llvm-mingw i686 clang (UCRT)    (extra data point)
#   detmath_test_linux     host gcc x86_64 (glibc)         (future Linux port)
#   detmath_aarch64.o      llvm-mingw aarch64 clang, compile only: check that
#                          no fused multiply-add is emitted (-ffp-contract=off)
#   mathutil_test_{i686,x64,i686c}.exe  the GC-table trig of
#                          src/game/shim/mathutil.c (needs the decomp headers;
#                          no Linux build: the game headers are Windows/32-bit
#                          or clang-x64 only)
# Usage: build.sh <outdir> [decomp_dir]   (LLVM_MINGW_ROOT from the env)
# Flags mirror CMakeLists.txt: -O2, SSE2 math on i686, -ffp-contract=off,
# and -fno-builtin -fno-math-errno for the src/detmath sources.
set -eu
OUT=$1
DECOMP=${2:-}
LLVM=${LLVM_MINGW_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)/../toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}
SRC=$(cd "$(dirname "$0")/../.." && pwd)
mkdir -p "$OUT"
LIB=("$SRC"/src/detmath/*.c "$SRC"/src/detmath/fdlibm/*.c)
COMMON=(-O2 -ffp-contract=off -I"$SRC/include")
build() { # cc outfile extra-flags...
  local cc=$1 out=$2; shift 2
  local objs=() f o
  local test=${TEST:-detmath_test} extra=()
  for f in "${LIB[@]}"; do
    o="$OUT/obj_$(basename "$out")_$(basename "$f").o"
    $cc "${COMMON[@]}" "$@" -fno-builtin -fno-math-errno -c "$f" -o "$o"
    objs+=("$o")
  done
  if [ "$test" = mathutil_test ]; then
    local inc=(-I"$DECOMP/include" -I"$DECOMP/build/GM2E8P/include" -I"$SRC/src/game/shim")
    o="$OUT/obj_$(basename "$out")_mathutil.o"
    $cc "${COMMON[@]}" "$@" -w "${inc[@]}" -include "$SRC/include/pc_detmath.h" \
        -c "$SRC/src/game/shim/mathutil.c" -o "$o"
    objs+=("$o")
    o="$OUT/obj_$(basename "$out")_tables.o"
    $cc "${COMMON[@]}" "$@" -c "$SRC/src/game/shim/mathutil_tables_gen.c" -o "$o"
    objs+=("$o")
    extra=("${inc[@]}" -w)
  fi
  $cc "${COMMON[@]}" "$@" "${extra[@]}" "$SRC/tools/detmath_test/$test.c" "${objs[@]}" -o "$out" -lm
  rm -f "${objs[@]}"
}
build i686-w64-mingw32-gcc "$OUT/detmath_test_i686.exe" -msse2 -mfpmath=sse -static
build "$LLVM/bin/x86_64-w64-mingw32-clang" "$OUT/detmath_test_x64.exe" -static
build "$LLVM/bin/i686-w64-mingw32-clang" "$OUT/detmath_test_i686c.exe" -msse2 -mfpmath=sse -static
build gcc "$OUT/detmath_test_linux"
if [ -n "$DECOMP" ]; then
  P32=(-include "$SRC/include/pc_ptr32.h" -fms-extensions)
  TEST=mathutil_test build i686-w64-mingw32-gcc "$OUT/mathutil_test_i686.exe" -msse2 -mfpmath=sse -static
  TEST=mathutil_test build "$LLVM/bin/x86_64-w64-mingw32-clang" "$OUT/mathutil_test_x64.exe" -static "${P32[@]}"
  TEST=mathutil_test build "$LLVM/bin/i686-w64-mingw32-clang" "$OUT/mathutil_test_i686c.exe" -msse2 -mfpmath=sse -static
fi
# aarch64: compile the library only and look for fused multiply-adds
: > "$OUT/aarch64_fma.txt"; : > "$OUT/aarch64_fma_on.txt"
for f in "${LIB[@]}"; do
  "$LLVM/bin/aarch64-w64-mingw32-clang" "${COMMON[@]}" -fno-builtin -fno-math-errno -c "$f" -o "$OUT/a64.o"
  "$LLVM/bin/llvm-objdump" -d "$OUT/a64.o" | grep -E '\bf(n?madd|n?msub)\b' | sed "s|^|$(basename "$f"): |" >> "$OUT/aarch64_fma.txt" || true
  "$LLVM/bin/aarch64-w64-mingw32-clang" "${COMMON[@]}" -ffp-contract=on -fno-builtin -fno-math-errno -c "$f" -o "$OUT/a64.o"
  echo "$(basename "$f"): with -ffp-contract=on: $("$LLVM/bin/llvm-objdump" -d "$OUT/a64.o" | grep -cE '\bf(n?madd|n?msub)\b') fma" >> "$OUT/aarch64_fma_on.txt"
done
rm -f "$OUT/a64.o"
echo "aarch64 FMAs with -ffp-contract=off: $(wc -l < "$OUT/aarch64_fma.txt") (want 0)"
