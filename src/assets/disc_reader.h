#ifndef DISC_READER_H
#define DISC_READER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Random-access reader over a GameCube disc image, whatever its container.
 * The backend (plain image now, RVZ later) is chosen by file magic in
 * disc_reader_open(); callers see the logical (uncompressed) disc bytes. */
typedef struct DiscReader DiscReader;

/* NULL on failure; err (if given) gets a one-line reason. */
DiscReader* disc_reader_open(const char* path, char* err, size_t errsz);
/* Logical disc size in bytes. */
int64_t disc_reader_size(DiscReader* r);
/* Read len bytes at a 64-bit offset.  false if any part lies outside the
 * disc or the read fails (dst contents are then unspecified). */
bool disc_reader_read(DiscReader* r, uint64_t offset, uint32_t len, void* dst);
void disc_reader_close(DiscReader* r);
/* "iso" (plain image); later "rvz". */
const char* disc_reader_format(DiscReader* r);

#ifdef __cplusplus
}
#endif

#endif /* DISC_READER_H */
