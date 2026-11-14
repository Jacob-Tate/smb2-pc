# Toolchain for 64-bit Windows cross-build with GCC (x86_64-w64-mingw32).
#
# Only for builds without the game (SMB2_GAME=OFF): GCC has no __ptr32, which
# the disc-layout structs need on x64 (DPTR).  The x64 game build uses
# cmake/llvm-mingw64.cmake; see docs/x64.md.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Static runtime so the exe is standalone
set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc")
