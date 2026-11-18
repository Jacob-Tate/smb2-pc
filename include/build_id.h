#ifndef SMB2_BUILD_ID_H
#define SMB2_BUILD_ID_H
/* Build identity generated at build time (cmake/build_id.cmake):
 * "<git commit count>-<short sha>[+dirty]", e.g. "57-8262da1" (fallback
 * without git: "<UTC yyyymmddHHMM>-nogit").  At most ~24 chars; fits the
 * netplay protocol's SMB_NET_MAX_BUILD_ID (40) as is. */
#ifdef __cplusplus
extern "C" {
#endif
const char* smb2_build_id(void);      /* "57-8262da1" */
const char* smb2_build_number(void);  /* "57" */
const char* smb2_build_sha(void);     /* "8262da1" or "8262da1+dirty" */
#ifdef __cplusplus
}
#endif
#endif
