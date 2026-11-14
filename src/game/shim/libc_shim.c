/* Game-specific libc-adjacent symbols.
 * memcpy_handler is the game's own DOL memcpy wrapper. */
#include "shim.h"
#include <string.h>

void *memcpy_handler(void *dst, const void *src, u32 n)
{
    return memcpy(dst, src, n);
}
