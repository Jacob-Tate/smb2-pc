#!/usr/bin/env bash
# Build SDL3 as STATIC libs for i686 (distro MinGW GCC) and x86_64 (llvm-mingw clang).
# Run inside WSL Ubuntu:  bash tools/build_sdl3_mingw.sh [--force] [i686|x86_64|both]
# Installs to <super_monkey>/smb2-pc/mingw_deps/sdl3/<triplet> (git-ignored, shared by worktrees).
set -euo pipefail

SDL3_VERSION="3.4.10"
SDL3_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(dirname "$SELF_DIR")"
SM_ROOT="${SM_ROOT:-/mnt/c/Users/jacob/source/super_monkey}"
DEPS="${SDL3_DEPS_DIR:-$SM_ROOT/smb2-pc/mingw_deps/sdl3}"
export LLVM_MINGW_ROOT="${LLVM_MINGW_ROOT:-$SM_ROOT/toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}"
WORK="$HOME/sdl3build-${SDL3_VERSION}"

FORCE=0; WHICH=both
for a in "$@"; do
  case "$a" in
    --force) FORCE=1 ;;
    i686|x86_64|both) WHICH="$a" ;;
    *) echo "usage: $0 [--force] [i686|x86_64|both]"; exit 2 ;;
  esac
done

for t in cmake ninja curl tar; do
  command -v "$t" >/dev/null || { echo "missing $t: sudo apt install cmake ninja-build curl"; exit 1; }
done

mkdir -p "$WORK"
TARBALL="$WORK/SDL3-${SDL3_VERSION}.tar.gz"
SRC="$WORK/SDL3-${SDL3_VERSION}"
need_src=0
for arch in i686 x86_64; do
  [ "$WHICH" = both -o "$WHICH" = "$arch" ] || continue
  [ -f "$DEPS/$arch-w64-mingw32/lib/libSDL3.a" ] && [ $FORCE = 0 ] || need_src=1
done
if [ $need_src = 1 ] && [ ! -d "$SRC" ]; then
  [ -f "$TARBALL" ] || curl -fL --retry 3 -o "$TARBALL" "$SDL3_URL"
  tar -xzf "$TARBALL" -C "$WORK"
fi

build() {
  local arch="$1" toolchain="$2"
  local triplet="$arch-w64-mingw32" prefix="$DEPS/$arch-w64-mingw32"
  if [ -f "$prefix/lib/libSDL3.a" ] && [ $FORCE = 0 ]; then
    echo "[$arch] already installed at $prefix (use --force to rebuild)"; return
  fi
  local bdir="$WORK/build-$arch"
  rm -rf "$bdir"
  echo "[$arch] configure"
  cmake -S "$SRC" -B "$bdir" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$REPO/cmake/$toolchain" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DSDL_SHARED=OFF -DSDL_STATIC=ON \
    -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF -DSDL_INSTALL_TESTS=OFF
  cmake --build "$bdir" -j"$(nproc)"
  cmake --install "$bdir"
  [ -f "$prefix/lib/libSDL3.a" ] || { echo "[$arch] libSDL3.a missing after install"; exit 1; }
  echo "[$arch] OK -> $prefix"
}

[ "$WHICH" = both -o "$WHICH" = i686 ]   && build i686   mingw32.cmake
[ "$WHICH" = both -o "$WHICH" = x86_64 ] && build x86_64 llvm-mingw64.cmake
echo "SDL3 $SDL3_VERSION done."
