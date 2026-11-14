# Languages

## What the original does (US disc, GM2E8P)

* The game keeps all its text in 6-entry tables indexed by the global
  `locale` (0 English, 1 German, 2 French, 3 Spanish, 4 Italian, 5 Japanese),
  e.g. `hud.c` `{ "How to play", "Spielregeln", "Comment jouer", "C\xF3mo jugar ",
  "Come giocare", "あそびかた" }`.  Other locale-dependent things: story dialogue
  files `dlg/<scene><suffix>.bin` (suffix `_e _g _f _s _i ""`, falls back to
  `_e`), stage names `stgname/<usa|ger|fra|spa|ita|jpn>.str`, and layout
  tweaks (`locale == 5` in baseball, race, boat, memcard, name entry ...).
* **The US DOL never reads the console language.**  `main()` stores 0:
  `0x800069F4 li r0,0` / `0x800069F8 stw r0, locale@sda21` (right after
  `read_and_set_os_string_table`, before `g_some_heap_init`).  There is no
  other store to `locale` in the DOL or any REL, and no call to
  `OSGetLanguage` / SRAM language code (`OSGetFontEncode` is only used by the
  debug window / DEMO font).  So the Dolphin IPL language setting has no
  effect on this disc; to see another language on the original, force it
  with a Gecko code (`04198B30 0000000N`, `locale` = 0x80198B30).
* The US disc's European text is unfinished: of the ~390 6-entry tables in the
  code about 45 % have `""` for German/French/Spanish/Italian (all sel_ngc
  menus, How to play, Story menus, view stage ...), the memory card messages
  are placeholders (`"NOCARD GER"`, `"CANNOT_SAVE_NOCARD FRA"`), menu titles
  are `"--"`.  `stgname/{ger,fra,spa,ita,jpn}.str` are byte-identical copies of
  the English list, and only English (`_e`) dialogue exists, plus Japanese
  `scnall.bin` / `scn0a.bin`.
* Encoding: the menu font (font 3, `font/fnt_asc_24x24.bin`, 24x24 IA4 pages)
  is indexed by Shift-JIS codes; the text code (`textdraw.c`,
  `dialog.c`, the HUD typewriters) reads **every byte >= 0x80 as an SJIS lead
  byte** in every language.  The font has no accented letters, and the
  European strings use cp1252 bytes (`\xF3`, `\xDC`, `\x85` ...), so on the
  original "C\xF3mo" eats the `m` ("C o"), "W\xE4hlen" shows "W  len".  The
  title's PRESS START line uses font 4 (an ASCII bitmap sheet), which has
  neither accents nor kana: German and Japanese show garbage there.
* Japanese is complete on the US disc (menus, How to play, HUDs; dialogue only
  for `scnall` / `scn0a`, the rest falls back to English, as on the original).
  The font file has 370 pages, but the DOL's code table lists only the first
  305 (punctuation, digits, Latin, kana, pad icons, card suits); pages 305-369
  are 65 kanji in SJIS order (扱 位 右 ... 落 練), which the Japanese release's
  table goes on to list.  28 of them are used by the Japanese strings
  (初級 中級 上級, 押して, ...).  `達` (in 達人, the Master banner on the name
  entry screen) is not in the font file at all.

## What the port does

* **Setting**: `language` in settings.dat (`auto en de fr es it ja`),
  `--language` (this run only), Options > Game Screen > Language and the F1
  overlay.  It is applied once at boot, where the DOL's `main()` sets
  `locale` (`main.c`, before `_prolog`), so everything locale-dependent is
  picked by the game itself; changing it shows "(restart)".  Auto: Japanese
  if the Windows UI language is Japanese, else English (the US disc's European
  text is too incomplete to pick it unasked).
* **Source encoding** (`tools/pc_decomp_text.py`, CMakeLists.txt): the GC build
  runs MWCC through sjiswrap (UTF-8 source -> cp932); GCC/Clang keep the
  UTF-8 bytes.  So the TUs with Japanese literals (hud.c, how_to.c,
  race2_5.c, boat_10.c, dip.c, window_menu.inc) are compiled from copies
  whose non-ASCII literals are SJIS `\x` escapes, byte for byte the GC
  objects' literals.
* **Decoding** (`textdraw.c` `pc_text_is_lead`, used by textdraw, dialog.c,
  adv_demo.c, scen_disp.c, hud.c): English and Japanese keep the original
  rule.  German/French/Spanish/Italian: only 0x81-0x84 (the SJIS rows the font
  has) and 0x86 lead a 2-byte code; other bytes >= 0x80 are cp1252 letters,
  drawn as code `0x8600 | byte`.
* **Glyphs** (`pc_font_ext.c`, `pc_font_compose.c`): codes 0x86xx are composed
  on first use from the font's own letters and its ` ^ ~ . , glyphs (accented
  capitals are drawn 16 rows tall under the accent), cp1252 punctuation maps
  to the font's existing glyphs (… ‘ ’ “ ” – — × ÷ « »); ¡ ¿ are the font's
  ! ? turned over.  Japanese: the US code table plus the 65 kanji codes.
* **Missing European text** (`tools/pc_decomp_text.py`, build time): in
  the PC copies of the decomp files, every 6-locale table record (1-4 lines
  per locale, string literals or `char x[] = "..."` names) whose German/
  French/Spanish/Italian entry is missing (`""`, `--`, `---`, `***`,
  `XXX GER/FRA/SPA/ITA`) gets the English entry there.  English and Japanese
  never read those entries, so their text is unchanged.
* **Title PRESS START**: the English line when the localized one has
  characters font 4 lacks (German, Japanese).
* **PC text** (`pc_text.c`): written as UTF-8 6-entry tables, converted to the
  game encoding (Latin letters -> 0x86xx, kana -> SJIS).  Settings rows,
  values, descriptions, F1 overlay title/hints, Quit to Desktop.
* **Button names** (`button_prompts.c`): the PlayStation / Xbox / Switch
  rewrites also cover the game's German (A-Knopf, L- und R-Taste, C-Stick),
  French (Bouton A, Stick C, Stick directionnel), Spanish (Bot\xF3n A, Stick
  de Control) and Italian (Pulsante A, Pulsanti L e R) words.

## Known gaps

* German/French/Spanish/Italian: only what the US disc has is translated; the
  rest is English (mixed-language screens).  Tables laid out as one row per
  language inside a 2-D array (name_entry2.c mode banners: `"***"` rows) are
  not covered by the fallback.  Stage names and story dialogue are English (the
  disc has nothing else).
* Japanese: `達` missing from the font (Master banner shows `人`); story
  dialogue is Japanese only for the scenes the disc has (`scnall`, `scn0a`).
* Font 4 / bitmap-font HUD text (title, some party-game HUDs) cannot show
  accents or kana.
