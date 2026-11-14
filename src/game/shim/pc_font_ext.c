/* PC port: menu-font (font 3, font/fnt_asc_24x24.bin) extensions for the
 * game's other languages.  docs/language.md
 *
 *  1. Japanese kanji.  The font file holds 370 pages, but the US DOL's code
 *     table (FONT_DEFINITIONS[3].string, 305 Shift-JIS codes: punctuation,
 *     digits, A-Z a-z, kana, the pad-button icons, card suits) stops before
 *     the last 65 pages: kanji in Shift-JIS order (pages 305-369) that the
 *     Japanese release's table goes on to list.  With locale 5 the port uses
 *     the US table plus those 65 codes, so the Japanese text's kanji draw.
 *     Other locales keep the US table untouched.
 *
 *  2. Latin-1.  The font has no accented letters.  The European strings'
 *     cp1252 bytes (textdraw.c pc_text_is_lead) and the PC's own text
 *     (pc_text.h) reach the font as codes 0x86xx; their glyphs are composed
 *     on first use from the font's letters and accent marks
 *     (pc_font_compose.c) into free font texture slots. */
#include "gc_intrinsics.h"
#include "bitmap.h"
#include "gx.h"
#include "pc_font_compose.h"
#include <string.h>
#include <stdint.h>

#define FONT_SLOT_COUNT 0x200
extern u32 g_some_font_ptr_array[FONT_SLOT_COUNT];   /* textdraw.c */
extern GXTexObj lbl_1_bss_A1704[FONT_SLOT_COUNT];   /* textdraw.c */
extern u16 lbl_1_bss_A575C[];                       /* textdraw.c: glyph widths */
void aram_font_to_mram_font(u8 fontId, int page, void *dest);
void GXInitTexObj(GXTexObj *obj, void *image, u16 width, u16 height, int format, int wrapS, int wrapT, int mipmap);

/* ---- 1. kanji ---- */
#define US_CODES 305
#define KANJI_CODES 65
static const u16 s_kanji[KANJI_CODES] = {
    /* pages 305..369 of fnt_asc_24x24.bin, read off the glyphs:
     * 扱位右押下何回期気球級競形決減源個合今左在坂使時次自取習初書勝上食人数
     * 設説先戦争走他体対大地着中定点電動同道読入抜負分変本明用落練 */
    0x88B5, 0x88CA, 0x8945, 0x899F, 0x89BA, 0x89BD, 0x89F1, 0x8AFA, 0x8B43, 0x8B85,
    0x8B89, 0x8BA3, 0x8C60, 0x8C88, 0x8CB8, 0x8CB9, 0x8CC2, 0x8D87, 0x8DA1, 0x8DB6,
    0x8DDD, 0x8DE2, 0x8E67, 0x8E9E, 0x8E9F, 0x8EA9, 0x8EE6, 0x8F4B, 0x8F89, 0x8F91,
    0x8F9F, 0x8FE3, 0x9048, 0x906C, 0x9094, 0x90DD, 0x90E0, 0x90E6, 0x90ED, 0x9188,
    0x9196, 0x91BC, 0x91CC, 0x91CE, 0x91E5, 0x926E, 0x9285, 0x9286, 0x92E8, 0x935F,
    0x9364, 0x93AE, 0x93AF, 0x93B9, 0x93C7, 0x93FC, 0x94B2, 0x9589, 0x95AA, 0x95CF,
    0x967B, 0x96BE, 0x9770, 0x978E, 0x97FB,
};
static u16 s_ja_string[US_CODES + KANJI_CODES + 1];
static s16 s_ja_pages[US_CODES + KANJI_CODES];

void pc_font_ext_init(int locale)
{
    struct FontParams *font = &FONT_DEFINITIONS[3];
    int n;

    if (locale != 5)
        return;
    for (n = 0; font->string[n] != 0; n++) {}
    if (n != US_CODES)
        return;   /* not the table this was made for */
    memcpy(s_ja_string, font->string, US_CODES * sizeof(u16));
    memcpy(s_ja_string + US_CODES, s_kanji, sizeof s_kanji);
    s_ja_string[US_CODES + KANJI_CODES] = 0;
    font->string = s_ja_string;
    font->pageIdx = s_ja_pages;
}

/* ---- 2. composed Latin glyphs ---- */
static u8 s_pages[128][PCFC_PAGE_SIZE] __attribute__((aligned(32)));
static s16 s_slot[128];   /* 0 = not made yet, -1 = no glyph, else slot + 1 */
static u16 s_width[128];

static int font_index(u16 code)
{
    const u16 *p = FONT_DEFINITIONS[3].string;
    int j;
    for (j = 0; p[j] != 0; j++)
        if (p[j] == code)
            return j;
    return -1;
}

static u8 s_tmp[4][PCFC_PAGE_SIZE] __attribute__((aligned(32)));
static int s_tmp_n;
static const unsigned char *get_page(unsigned short code, void *user)
{
    int j = font_index(code);
    u8 *buf;
    (void)user;
    if (j < 0)
        return NULL;
    buf = s_tmp[s_tmp_n++ & 3];
    aram_font_to_mram_font(3, j, buf);
    return buf;
}

int pc_font_ext_glyph(u16 code, s32 *slot, u16 *width)
{
    int b = code & 0xFF, k, i, j;
    PcfcRecipe r;

    if ((code >> 8) != 0x86 || b < 0x80)
        return 0;
    k = b - 0x80;
    if (s_slot[k] == 0) {
        s_slot[k] = -1;
        if (pcfc_recipe((u8)b, &r)) {
            if (r.existing) {
                j = font_index(r.base);
                if (j >= 0 && FONT_DEFINITIONS[3].pageIdx[j] >= 0) {
                    s_slot[k] = FONT_DEFINITIONS[3].pageIdx[j] + 1;
                    s_width[k] = lbl_1_bss_A575C[j];
                }
            } else if (pcfc_compose(&r, get_page, NULL, s_pages[k])) {
                for (i = 0; i < FONT_SLOT_COUNT; i++)
                    if (g_some_font_ptr_array[i] == 0)
                        break;
                if (i < FONT_SLOT_COUNT) {
                    g_some_font_ptr_array[i] = (u32)(uintptr_t)s_pages[k];
                    GXInitTexObj(&lbl_1_bss_A1704[i], s_pages[k], 24, 24, 2 /* IA4 */, 0, 0, 0);
                    s_slot[k] = i + 1;
                    /* advance/kerning of the base letter (lone marks: half width) */
                    j = r.base ? font_index(r.base) : font_index(0x8144 /* . */);
                    s_width[k] = j >= 0 ? lbl_1_bss_A575C[j] : FONT_DEFINITIONS[3].spaceWidth;
                    if (r.base == 0)
                        s_width[k] = FONT_DEFINITIONS[3].spaceWidth / 2;
                }
            }
        }
    }
    if (s_slot[k] <= 0)
        return 0;
    *slot = s_slot[k] - 1;
    *width = s_width[k];
    return 1;
}
