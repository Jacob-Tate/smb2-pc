/* Memory card save import/export (src/game/shim/pc_gci.c) and the file-level
 * card store it works on (src/game/shim/card_shim.c). */
#ifndef PC_CARD_H
#define PC_CARD_H

#include <stdint.h>
#include <stddef.h>

#define PC_CARD_BLOCK      0x2000
#define PC_CARD_NAME_MAX   32
#define PC_CARD_MAX_FILES  127
#define PC_CARD_USER_BLOCKS 251   /* the port's card: 16 Mbit, 5 system blocks */

/* One file on the card: the GameCube directory entry (DEntry) fields that
 * card_shim keeps in <name>.meta, plus the directory slot. */
typedef struct {
    char fileName[PC_CARD_NAME_MAX + 1];
    uint8_t gameName[4];
    uint8_t company[2];
    uint8_t bannerFormat;
    uint32_t time;          /* seconds since 2000-01-01 */
    uint32_t iconAddr;
    uint16_t iconFormat;
    uint16_t iconSpeed;
    uint8_t permission;
    uint8_t copyTimes;
    uint16_t firstBlock;    /* block on the card it came from; 0 = not known */
    uint32_t length;        /* bytes, a multiple of PC_CARD_BLOCK */
    uint32_t commentAddr;
    int fileNo;             /* directory slot, -1 = not on the card */
} PcCardFile;

/* ---- card_shim.c ---- */
const char *pc_card_slot_dir(void);    /* "<base>memcard/slotA/" */
const char *pc_card_base_dir(void);    /* "<base>memcard/" */
/* Files on the card as stored on disk (not the mounted directory); returns
 * the count, at most `max`. */
int pc_card_list(PcCardFile *out, int max);
/* malloc'd copy of a listed file's data (f->length bytes), NULL on error */
uint8_t *pc_card_read(const PcCardFile *f);
/* Writes a file to the card, replacing one with the same name (whose
 * .bin/.meta are first copied to memcard/backup/; the backup's file name
 * goes to `backup`, empty when nothing was replaced).  0 = ok, else a
 * negative PC_CARD_E_* code. */
int pc_card_store(const PcCardFile *f, const uint8_t *data, char *backup, size_t backupsz);
enum { PC_CARD_E_IO = -1, PC_CARD_E_FULL = -2, PC_CARD_E_NOENT = -3 };
/* the card reads as removed from now on (after an in-game import, until the
 * game is restarted) */
void pc_card_freeze(void);

/* ---- pc_gci.c ---- */
/* What happened, for the log / command line (text, English) and the in-game
 * screen (codes, shown in the game's language). */
enum {
    PCG_OK, PCG_E_OPEN, PCG_E_FORMAT, PCG_E_GCI_SIZE, PCG_E_OTHER_GAME, PCG_E_OTHER_REGION,
    PCG_E_RAW_HEADER, PCG_E_RAW_DIR, PCG_E_RAW_BAT, PCG_E_NO_SAVE, PCG_E_FULL, PCG_E_WRITE,
    PCG_E_NO_FILES_ON_CARD, PCG_E_EXPORT_WRITE, PCG_E_NOT_CONVERTIBLE, PCG_E_SAVE_CRC
};
enum { PCG_F_IMPORTED, PCG_F_REPLACED, PCG_F_SKIPPED, PCG_F_FAILED, PCG_F_EXPORTED };
typedef struct {
    char name[PC_CARD_NAME_MAX + 1];
    uint8_t gameName[4], company[2];
    int status;         /* PCG_F_* */
    int err;            /* PCG_E_* for PCG_F_FAILED */
} PcGciEntry;
typedef struct {
    int err;            /* PCG_E_* of the whole operation */
    int imported, replaced, skipped, failed, exported;
    int nentries;
    PcGciEntry entries[PC_CARD_MAX_FILES];
    char text[6144];
} PcGciReport;
const char *pc_gci_err_text(int err);   /* English */
/* Game data save (storymode_save_files, 0xF98 bytes): ranking tables saved
 * by port builds before 1.1.2 (no layout mark) in their byte-reversed
 * layout are converted in place and marked; 1 = converted, 0 = current. */
int pc_save_rank_fix(uint8_t *story);
/* marks the save's ranking tables as being in the current layout */
void pc_save_rank_mark(uint8_t *story);

/* GM2E8P files found in a .gci or raw card image, not yet written */
typedef struct {
    int count;
    PcCardFile files[PC_CARD_MAX_FILES];
    uint8_t *data[PC_CARD_MAX_FILES];
    int exists[PC_CARD_MAX_FILES];   /* a save with this name is on the card */
} PcGciSet;

/* Reads `path` (.gci or raw card image, by content).  Files of other games
 * are listed in rep as skipped.  0 = ok (set->count may still be 0 when the
 * card holds no SMB2 file), else -1 with the reason in rep. */
int pc_gci_load(const char *path, PcGciSet *set, PcGciReport *rep);
/* Writes every file of the set to the card (backing up replaced saves). */
int pc_gci_store(PcGciSet *set, PcGciReport *rep);
void pc_gci_free(PcGciSet *set);
/* load + store; returns the number of files imported, -1 on error */
int pc_gci_import(const char *path, PcGciReport *rep);
/* Every GM2E8P file on the card as <dir>/<maker>-<game>-<name>.gci;
 * `first` (optional) is the path to use for the first file.  Returns the
 * number written, -1 on error. */
int pc_gci_export(const char *dir, const char *first, PcGciReport *rep);
/* Default .gci file name of a card file ("8P-GM2E-super_monkey_ball_2.dat.gci") */
void pc_gci_default_name(const PcCardFile *f, char *out, size_t outsz);
/* memcard/import/: imports every .gci/.raw there and moves it to
 * import/done/ (failed ones to import/failed/).  Returns the number of
 * files processed; the report is kept for pc_gci_startup_report(). */
int pc_gci_import_dropins(void);
const PcGciReport *pc_gci_startup_report(void);   /* NULL if nothing was dropped in */
void pc_gci_startup_report_clear(void);
/* main.c: --import-save / --export-save (exit code, -1 = not given), and the
 * drop-in folder at startup */
int pc_gci_cli(int argc, char **argv);
void pc_gci_startup(void);

#endif
