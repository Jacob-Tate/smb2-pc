/* PC-only text in the game's own encoding (docs/language.md).
 *
 * The game's menu font (font 3) is indexed by Shift-JIS codes.  PC strings
 * are written as UTF-8 in the source and converted once:
 *   - ASCII stays ASCII;
 *   - Latin-1 / cp1252 letters (U+00A0..U+00FF, U+2026 ...) become the
 *     PC-private two-byte code 0x86 <cp1252 byte>, drawn with glyphs the port
 *     composes from the font's own letters (textdraw.c pc_latin_glyph);
 *   - everything else (kana, fullwidth forms) becomes Shift-JIS (cp932). */
#ifndef PC_TEXT_H
#define PC_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Converted copy of a static UTF-8 string (cached by pointer; NULL -> ""). */
const char* pc_text_game(const char* utf8);
/* Same into a caller buffer (for strings that are not static). */
void pc_text_game_buf(const char* utf8, char* out, int outsize);

/* Pick the string of the current game locale (0 en .. 5 ja) from a 6-entry
 * UTF-8 table (NULL/"" entries fall back to English), converted. */
const char* pc_text_pick(const char* const tbl[6]);

#ifdef __cplusplus
}
#endif
#endif /* PC_TEXT_H */
