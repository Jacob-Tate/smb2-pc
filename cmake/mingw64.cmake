# Toolchain for 64-bit Windows cross-build (x86_64-w64-mingw32).
#
# The decompiled game code was written for 32-bit pointers.  The x64 game
# build (SMB2_GAME=ON) therefore links the exe at a low fixed image base and
# keeps all game memory (GC RAM emulation, exe statics) below 4 GiB, so
# pointer values that the game stores in u32 fields still round-trip.  See
# docs/x64.md for the full design.
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
