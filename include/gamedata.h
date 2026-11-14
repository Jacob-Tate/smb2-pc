#ifndef GAMEDATA_H
#define GAMEDATA_H
/* Extracted game data ("gamedata" folder): --extract-iso / --verify-gamedata
 * and the runtime validity check.  Layout (docs/installer.md):
 *   <dir>/manifest.txt   disc id, rev, per-file size + CRC32 (written last)
 *   <dir>/sys/boot.bin   disc header 0x000-0x440
 *   <dir>/sys/bi2.bin    0x440-0x2440
 *   <dir>/sys/fst.bin    the disc FST
 *   <dir>/files/...      every FST file, same names/sizes as on the disc
 *   <dir>/icon.ico, icon_256.png   generated from /memcard/sys.bin */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    GD_OK = 0,
    GD_ERR_NOT_SMB2 = 2,    /* not SMB2 / wrong region / revision */
    GD_ERR_IMAGE = 3,       /* unreadable / corrupt / unsupported image */
    GD_ERR_WRITE = 4,       /* write error / disk full */
    GD_ERR_OTHER = 5,
    GD_ERR_MISMATCH = 6,    /* --verify-gamedata: mismatch / missing */
};

uint32_t gd_crc32(uint32_t crc, const void* data, size_t n);   /* zlib CRC-32 */

/* done/total in bytes; called often (throttle yourself). */
typedef void (*GdProgressFn)(uint64_t done, uint64_t total, void* user);

/* Extract iso -> dir (via dir.partial, atomic rename).  Returns GD_* and a
 * one-line message. */
int gamedata_extract(const char* iso, const char* dir, GdProgressFn cb, void* user,
                     char* msg, size_t msgsz);
/* Full check: every manifest file present with the recorded size + CRC32. */
int gamedata_verify(const char* dir, GdProgressFn cb, void* user, char* msg, size_t msgsz);
/* Cheap runtime check: manifest parses, disc id matches, every file exists
 * with the recorded size.  true = usable. */
bool gamedata_quick_check(const char* dir, char* msg, size_t msgsz);

/* CLI entry points (headless; print PROGRESS/RESULT lines; return exit code) */
int gamedata_cli_extract(const char* iso, const char* dir, bool progress);
int gamedata_cli_verify(const char* dir, bool progress);

#ifdef __cplusplus
}
#endif
#endif
