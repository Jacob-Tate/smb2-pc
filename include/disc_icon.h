#ifndef DISC_ICON_H
#define DISC_ICON_H
/* Game icon generated from the user's own disc (never shipped: Sega art).
 * Source: /memcard/sys.bin @0x1800, the 32x32 RGB5A3 memory-card save icon
 * (AiAi).  Sizes: 16 = 2x2 box filter of the 32, 32 = original, 48 = area
 * downsample of the 64, 64/128/256 = Scale2x^1/2/3; any other size = area
 * downsample of the next larger Scale2x level.  Pixels are RGBA8 (R first). */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ISOArchive;

#define DISC_ICON_OFFSET 0x1800u
#define DISC_ICON_BYTES  (32u * 32u * 2u)

/* Decode the 32x32 icon from the raw sys.bin bytes at DISC_ICON_OFFSET. */
void disc_icon_decode(const uint8_t* rgb5a3_tiles, uint8_t out_rgba[32 * 32 * 4]);
/* Read /memcard/sys.bin from the archive and decode its icon. */
bool disc_icon_from_archive(struct ISOArchive* arc, uint8_t out_rgba[32 * 32 * 4]);
/* Render the icon at size x size (1..1024). */
void disc_icon_render(const uint8_t src32[32 * 32 * 4], int size, uint8_t* out_rgba);

/* Encoders (malloc'd buffer, caller frees).  PNG uses stored deflate. */
uint8_t* disc_icon_png(const uint8_t* rgba, int w, int h, size_t* out_size);
/* .ico with 16,32,48,64,128 (32-bit BMP) + 256 (PNG). */
uint8_t* disc_icon_ico(const uint8_t src32[32 * 32 * 4], size_t* out_size);

#ifdef __cplusplus
}
#endif
#endif
