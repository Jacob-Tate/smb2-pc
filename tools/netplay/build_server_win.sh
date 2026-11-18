#!/usr/bin/env bash
# Builds the relay server for Windows (x64, static) from WSL, next to the repo's build dirs:
#   MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/<repo>/tools/netplay/build_server_win.sh
# -> build/server_win/smb2-server.exe (+ smb2-server-test.exe).  The ENet source comes from third_party/enet's
# sibling checkout (server/CMakeLists.txt normally fetches v1.3.18 with git; set ENET_SRC to reuse a copy, e.g.
# an earlier build/server/_deps/enet-src).  tools/netplay/two_instances.sh uses this exe for its local server.
set -euo pipefail
R=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
export LLVM_MINGW_ROOT=${LLVM_MINGW_ROOT:-/mnt/c/Users/jacob/source/super_monkey/toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64}
ARGS=()
[ -n "${ENET_SRC:-}" ] && ARGS+=("-DFETCHCONTENT_SOURCE_DIR_ENET=$ENET_SRC")
cmake -S "$R/server" -B "$R/build/server_win" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$R/cmake/llvm-mingw64.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static" "${ARGS[@]}"
cmake --build "$R/build/server_win"
ls "$R"/build/server_win/*.exe
