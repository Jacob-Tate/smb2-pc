# Toolchain for 32-bit Windows cross-build (i686-w64-mingw32).
# The decompiled game code was written for 32-bit pointers, so this build needs
# no layout adjustments; the x64 build uses cmake/llvm-mingw64.cmake.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Static runtime so the exe is standalone
set(CMAKE_C_FLAGS_INIT "-static-libgcc")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc")
