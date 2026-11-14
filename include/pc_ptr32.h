/* Force-included into every TU of the x64 (clang) build -- docs/x64.md.
 *
 * mingw-w64's _mingw.h does "#define __ptr32" (empty, for GCC).  Clang
 * implements __ptr32/__uptr (32-bit pointers, -fms-extensions), which the
 * port uses for pointer fields inside disc-layout data (DPTR in types.h).
 * Pull _mingw.h in first -- it is include-guarded -- then drop its macro. */
#if defined(__clang__) && defined(_WIN64)
#include <_mingw.h>
#undef __ptr32
#endif
