# x86_64 Windows game build (docs/x64.md): clang from an llvm-mingw release
# (tested: llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64, LLVM 19.1.6).
# Pass -DLLVM_MINGW_ROOT=<unpacked tarball> (or set the env var).  clang is
# required because disc-layout structs use __ptr32 pointer fields (DPTR).
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
if(NOT LLVM_MINGW_ROOT)
    set(LLVM_MINGW_ROOT $ENV{LLVM_MINGW_ROOT})
endif()
if(NOT LLVM_MINGW_ROOT)
    # default: <pc_port>/../toolchains/<release>, next to the decomp checkout
    get_filename_component(LLVM_MINGW_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../../toolchains/llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64" ABSOLUTE)
endif()
# try_compile projects re-read this file: hand them the same root
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LLVM_MINGW_ROOT)
if(NOT EXISTS "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang")
    message(FATAL_ERROR "llvm-mingw not found at '${LLVM_MINGW_ROOT}': pass "
        "-DLLVM_MINGW_ROOT=<unpacked llvm-mingw release> or set the LLVM_MINGW_ROOT env var")
endif()
set(CMAKE_C_COMPILER ${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang)
set(CMAKE_CXX_COMPILER ${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang++)
set(CMAKE_RC_COMPILER ${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH ${LLVM_MINGW_ROOT}/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# clang 16+ makes these hard errors; the decomp (GCC-built, -w) relies on
# them.  -gline-tables-only: crash dumps symbolize with llvm-symbolizer.
set(CMAKE_C_FLAGS_INIT "-gline-tables-only -fms-extensions -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-function-pointer-types -Wno-error=implicit-int -Wno-error=return-type")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
