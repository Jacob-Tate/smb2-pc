/* The only file that includes the generated build_id.h, so a new commit
 * recompiles just this one TU (see cmake/build_id.cmake). */
#include "build_id.h"
#include "build_id_gen.h"
const char* smb2_build_id(void) { return SMB2_BUILD_ID_STR; }
const char* smb2_build_number(void) { return SMB2_BUILD_NUMBER_STR; }
const char* smb2_build_sha(void) { return SMB2_BUILD_SHA_STR; }
