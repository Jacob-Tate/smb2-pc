/* Latin-1 glyphs composed from the menu font's own letters (font 3,
 * font/fnt_asc_24x24.bin: 24x24 IA4 pages, Shift-JIS indexed, no accented
 * letters).  Pure functions, no game state: pc_font_ext.c uses them in the
 * game, tools can use them to preview the glyphs.  docs/language.md */
#ifndef PC_FONT_COMPOSE_H
#define PC_FONT_COMPOSE_H

#define PCFC_PAGE_SIZE 0x240   /* 24x24 IA4 */

/* What a cp1252 byte is drawn as. */
typedef struct {
    unsigned short base;    /* font code of the base glyph (0 = none) */
    unsigned char mark;     /* PCFC_* below */
    unsigned char existing; /* 1: just draw `base` (no composing) */
} PcfcRecipe;

enum {
    PCFC_NONE, PCFC_GRAVE, PCFC_ACUTE, PCFC_CIRC, PCFC_TILDE, PCFC_DIAER,
    PCFC_RING, PCFC_CEDILLA, PCFC_STROKE, PCFC_FLIPV, PCFC_ROT180,
    PCFC_SZLIG, PCFC_DEGREE,
};

/* Recipe for cp1252 byte b (0x80..0xFF); returns 0 if b has no glyph. */
int pcfc_recipe(unsigned char b, PcfcRecipe *r);

/* Compose page `out` from the base page (and the font's accent glyph pages,
 * fetched through `page`, which returns NULL for a missing code). */
typedef const unsigned char *(*PcfcGetPage)(unsigned short code, void *user);
int pcfc_compose(const PcfcRecipe *r, PcfcGetPage page, void *user,
                 unsigned char out[PCFC_PAGE_SIZE]);

#endif
