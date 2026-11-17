#!/usr/bin/env bash
# tools/win_build.sh - build the Windows exes from WSL, from the main checkout or any git worktree.
#
#   tools/win_build.sh [x64|i686|both] [--clean] [--release]
#
# From Windows (Git Bash):
#   MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/<repo or worktree>/tools/win_build.sh both
#
# Paths (override with env vars):
#   SMB2_ROOT        workspace with the decomp, toolchains and the main checkout
#   SMB2_DECOMP_DIR  built smb2-decomp checkout
#   LLVM_MINGW_ROOT  llvm-mingw 20241217 (64-bit build)
#   SMB2_DEPS_DIR    mingw_deps with SDL (git-ignored, so worktrees symlink to the main checkout's)
#   SMB2_BUILD_TYPE  default RelWithDebInfo (--release: Release)
set -euo pipefail

ARCH=both; CLEAN=0; BTYPE=${SMB2_BUILD_TYPE:-RelWithDebInfo}
for a in "$@"; do
    case "$a" in
        x64|i686|both) ARCH=$a ;;
        --clean) CLEAN=1 ;;
        --release) BTYPE=Release ;;
        *) echo "usage: $0 [x64|i686|both] [--clean] [--release]" >&2; exit 2 ;;
    esac
done

SRC=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SMB2_ROOT=${SMB2_ROOT:-/mnt/c/Users/jacob/source/super_monkey}
SMB2_DECOMP_DIR=${SMB2_DECOMP_DIR:-$SMB2_ROOT/smb2-decomp}
LLVM_MINGW_ROOT=${LLVM_MINGW_ROOT:-$SMB2_ROOT/toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}
SMB2_DEPS_DIR=${SMB2_DEPS_DIR:-$SMB2_ROOT/smb2-pc/mingw_deps}
export LLVM_MINGW_ROOT

[ -d "$SMB2_DECOMP_DIR/build/GM2E8P" ] || { echo "decomp not built: $SMB2_DECOMP_DIR" >&2; exit 1; }
# CMakeLists.txt looks for SDL in <source>/mingw_deps
if [ ! -e "$SRC/mingw_deps" ]; then
    ln -s "$SMB2_DEPS_DIR" "$SRC/mingw_deps"
    echo "[win_build] linked mingw_deps -> $SMB2_DEPS_DIR"
fi

build_one() {   # $1 = build dir, $2 = toolchain file
    local dir=$SRC/$1
    [ "$CLEAN" = 1 ] && rm -rf "$dir"
    if [ ! -f "$dir/build.ninja" ]; then
        cmake -S "$SRC" -B "$dir" -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE="$SRC/cmake/$2" \
            -DCMAKE_BUILD_TYPE="$BTYPE" \
            -DSMB2_GAME=ON \
            -DSMB2_DECOMP_DIR="$SMB2_DECOMP_DIR" \
            -DLLVM_MINGW_ROOT="$LLVM_MINGW_ROOT"
    fi
    ninja -C "$dir"
    echo "[win_build] OK: $dir/bin/smb2_pc_port.exe"
}

case "$ARCH" in
    x64)  build_one build64 llvm-mingw64.cmake ;;
    i686) build_one build32 mingw32.cmake ;;
    both) build_one build64 llvm-mingw64.cmake; build_one build32 mingw32.cmake ;;
esac
