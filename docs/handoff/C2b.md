# C2b handoff: SDL3 static build (DONE)

## Done
- `tools/build_sdl3_mingw.sh` builds SDL3 **3.4.10** (pinned in the script) static for i686 (distro GCC,
  `cmake/mingw32.cmake`) and x86_64 (llvm-mingw clang, `cmake/llvm-mingw64.cmake`). Release, Ninja,
  `-DSDL_SHARED=OFF -DSDL_STATIC=ON`, tests/examples off. Idempotent (`--force` rebuilds; optional arg
  `i686|x86_64|both`). Builds in `$HOME/sdl3build-3.4.10` (WSL-native), tarball cached there.
- Installed to `smb2-pc/mingw_deps/sdl3/{i686,x86_64}-w64-mingw32/` (`lib/libSDL3.a`, `include/SDL3/`,
  `lib/cmake/SDL3`, `lib/pkgconfig/sdl3.pc`). Existing SDL2 dirs untouched.
- `tools/sdl3_hello/` (hello.c + build.sh) proves static link; exes in `tools/sdl3_hello/out/` (git-ignored).

## Commands
    MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/<repo>/tools/build_sdl3_mingw.sh 2>&1 | tr -d '\0'
    MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/<repo>/tools/sdl3_hello/build.sh | tr -d '\0'
    tools/sdl3_hello/out/hello_i686.exe ; tools/sdl3_hello/out/hello_x86_64.exe   # from Git Bash
First full build takes ~4-5 minutes for both arches.

## Result
Both exes print `SDL3 version 3.4.10` / `gamepads: 0`, exit 0, on Windows. `objdump -p` shows no SDL3.dll
import (only system DLLs: kernel32 user32 gdi32 advapi32 shell32 winmm imm32 ole32 oleaut32 setupapi
version, plus msvcrt / UCRT api-ms-win-crt-*).

## Link libraries for C2c
Authoritative (from generated `sdl3.pc` Libs, identical for both arches):
`-lSDL3 -lm -lkernel32 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion -luuid -ladvapi32
-lsetupapi -lshell32 -ldinput8` (+ `-mwindows` in the .pc; the game keeps its own subsystem choice).
- Same as the existing SDL2 line; no new libs are strictly required. `cfgmgr32`/`hid` are not in the .pc
  (SDL3 loads them dynamically); the hello build passes `-lcfgmgr32 -lhid -lrpcrt4 -lws2_32` as harmless extras.
- Keep `-static` (+ `-static-libgcc` for i686) from the toolchain files.
- Include: `<SDL3/SDL.h>`, include dir `mingw_deps/sdl3/<triplet>/include`. No SDL3main lib; use `SDL_MAIN_HANDLED`.
- Static SDL3 CMake target: `find_package(SDL3 CONFIG)` -> `SDL3::SDL3-static` (config under `lib/cmake/SDL3`).

## Gotchas
- Compiles via the project toolchain files; llvm-mingw toolchain adds `-fms-extensions` etc. to SDL3 too (built fine).
- Install path is under /mnt/c (DrvFs); install step is slowish but fine.
