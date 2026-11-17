#ifndef RVZ_H
#define RVZ_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GameCube RVZ container (Zstandard or uncompressed groups).  Used by
 * disc_reader.c as a backend; callers normally go through DiscReader. */
typedef struct RvzImage RvzImage;

/* Takes ownership of f on success (closed by rvz_close).  On failure f is
 * NOT closed and NULL is returned with a one-line reason in err. */
RvzImage* rvz_open(FILE* f, char* err, size_t errsz);
/* Logical (decoded) disc size. */
int64_t rvz_size(const RvzImage* r);
bool rvz_read(RvzImage* r, uint64_t offset, uint32_t len, void* dst);
void rvz_close(RvzImage* r);

#ifdef __cplusplus
}
#endif

#endif /* RVZ_H */
