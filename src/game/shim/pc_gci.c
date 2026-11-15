/* Memory card save import/export: GameCube .gci files and raw card images
 * (Dolphin's MemoryCardA.*.raw, GCMM dumps of a real card) <-> the port's
 * file-level card (card_shim.c: memcard/slotA/<name>.bin + .meta).
 *
 * .gci = the file's 64-byte directory entry (DEntry, big endian) followed by
 * its data, blockCount * 8 KiB:
 *   0x00 gameName[4]   0x04 company[2]   0x06 0xFF   0x07 bannerFormat
 *   0x08 fileName[32]  0x28 time         0x2C iconAddr
 *   0x30 iconFormat    0x32 iconSpeed    0x34 permission  0x35 copyTimes
 *   0x36 firstBlock    0x38 blockCount   0x3A 0xFFFF      0x3C commentAddr
 * Every field goes to the .meta key of the same name (blockCount as length
 * in bytes), so an exported .gci is byte-identical to the imported one.
 *
 * Raw card image = 8 KiB blocks: 0 header, 1/2 directory + its backup (127
 * DEntry, update counter, checksums), 3/4 block allocation table + backup
 * (next-block map for blocks 5..), 5.. file data.  Of each pair the copy
 * with valid checksums and the newer update counter is used, as the SDK's
 * __CARDVerify does.  Only GM2E8P files are imported.
 *
 * Byte order: the game data save (super_monkey_ball_2.dat) holds raw copies
 * of game variables (decomp memcard_mkb2.c g_save_game_data), so a GameCube
 * file has them big endian and the port's little endian.  Import and export
 * swap every multi-byte field (s_swap, from the variables' declared types)
 * and redo the file checksum.  Replays (compressed replay data) are not
 * converted. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "pc_card.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define BLK PC_CARD_BLOCK
#ifdef _WIN32
#define PSEP "\\"
#else
#define PSEP "/"
#endif
#define DENTRY_SIZE 0x40

static const u8 k_game[4] = { 'G', 'M', '2', 'E' };
static const u8 k_maker[2] = { '8', 'P' };

static u16 be16(const u8 *p) { return (u16)((p[0] << 8) | p[1]); }
static u32 be32(const u8 *p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
static void put16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void put32(u8 *p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }

static void make_dir(const char *p)
{
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0777);
#endif
}

/* ------------------------------------------------------------------ report */

const char *pc_gci_err_text(int err)
{
    switch (err) {
    case PCG_OK: return "ok";
    case PCG_E_OPEN: return "the file can't be read";
    case PCG_E_FORMAT: return "not a .gci save file or a GameCube memory card image";
    case PCG_E_GCI_SIZE: return "the .gci file is damaged (its size does not match its block count)";
    case PCG_E_OTHER_GAME: return "this save is for another game, not Super Monkey Ball 2";
    case PCG_E_OTHER_REGION: return "this save is from the European or Japanese version of Super Monkey Ball 2; "
                                    "this port runs the USA version (GM2E8P) and can't use it";
    case PCG_E_RAW_HEADER: return "the memory card image is damaged (bad header checksum or size)";
    case PCG_E_RAW_DIR: return "the memory card image is damaged (both directory copies have bad checksums)";
    case PCG_E_RAW_BAT: return "the memory card image is damaged (bad block allocation table)";
    case PCG_E_NO_SAVE: return "the memory card has no Super Monkey Ball 2 (USA) save";
    case PCG_E_FULL: return "the port's memory card is full";
    case PCG_E_WRITE: return "the save could not be written";
    case PCG_E_NO_FILES_ON_CARD: return "there is no Super Monkey Ball 2 save to export yet";
    case PCG_E_EXPORT_WRITE: return "the .gci file could not be written";
    case PCG_E_NOT_CONVERTIBLE: return "only the game data save (super_monkey_ball_2.dat) can be converted; "
                                       "replays are not";
    case PCG_E_SAVE_CRC: return "the save is damaged (its checksum does not match)";
    }
    return "error";
}

static void rep_line(PcGciReport *rep, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
static void rep_line(PcGciReport *rep, const char *fmt, ...)
{
    size_t l = strlen(rep->text);
    va_list ap;
    if (l + 2 >= sizeof(rep->text))
        return;
    va_start(ap, fmt);
    vsnprintf(rep->text + l, sizeof(rep->text) - l - 1, fmt, ap);
    va_end(ap);
    l = strlen(rep->text);
    rep->text[l] = '\n';
    rep->text[l + 1] = 0;
}

static PcGciEntry *rep_entry(PcGciReport *rep, const PcCardFile *f, int status, int err)
{
    PcGciEntry *e;
    if (rep->nentries >= PC_CARD_MAX_FILES)
        return NULL;
    e = &rep->entries[rep->nentries++];
    memset(e, 0, sizeof(*e));
    memcpy(e->name, f->fileName, sizeof(e->name));
    memcpy(e->gameName, f->gameName, 4);
    memcpy(e->company, f->company, 2);
    e->status = status;
    e->err = err;
    return e;
}

/* printable "GM2E8P" */
static void code_str(const PcCardFile *f, char out[7])
{
    int i;
    for (i = 0; i < 4; i++) out[i] = isprint(f->gameName[i]) ? (char)f->gameName[i] : '?';
    for (i = 0; i < 2; i++) out[4 + i] = isprint(f->company[i]) ? (char)f->company[i] : '?';
    out[6] = 0;
}

static int is_smb2(const PcCardFile *f)
{
    return memcmp(f->gameName, k_game, 4) == 0 && memcmp(f->company, k_maker, 2) == 0;
}

static int is_smb2_other_region(const PcCardFile *f)
{
    return f->gameName[0] == 'G' && f->gameName[1] == 'M' && f->gameName[2] == '2' &&
           f->gameName[3] != 'E' && memcmp(f->company, k_maker, 2) == 0;
}

/* ------------------------------------------------------------------ byte order */

#define SAVE_FILE_NAME "super_monkey_ball_2.dat"
#define SAVE_SIZE      0x4000
#define SAVE_VERSION   0x12      /* memcard.c memcard_file_descs[0].unk3E */
#define SAVE_DATA      0x2060    /* storymode_save_files, 0xF98 bytes */
#define GATE_OFS       0x31C     /* pad stick gates (DOL lbl_80081AC0), bytes */

u16 fn_800648C8(u32 size, void *data);     /* card_shim.c: the game's file checksum */
extern int8_t lbl_80081AC0[4][8][2];       /* input_sdl2.c: stick gates */

/* multi-byte fields of storymode_save_files: { offset, size, count, stride } */
static const struct { u16 ofs; u8 size; u8 count; u16 stride; } s_swap[] = {
    /* struct StorySave[3] (scen_mode.c): flags, playTime, score, bananas, unk30 */
    { 0x004, 4, 3, 0x84 }, { 0x018, 4, 3, 0x84 }, { 0x01C, 4, 3, 0x84 }, { 0x020, 4, 3, 0x84 },
    { 0x030, 4, 3, 0x84 },
    /* sel.c menu settings (fn_1_125C0) */
    { 0x1E8, 4, 1, 0 },      /* SelMenuState.unk5C */
    { 0x210, 4, 1, 0 },      /* SelSettings8.unk8 */
    { 0x214, 4, 1, 0 },      /* SelSettings9.unk0 */
    { 0x224, 4, 1, 0 },      /* SelSettingsA.unk4 */
    { 0x23E, 2, 1, 0 },      /* SelSettingsE.unk4 */
    { 0x240, 4, 1, 0 },      /* SelSettingsF.unk0 */
    { 0x24C, 4, 1, 0 },      /* SelSettingsG.unk4 */
    { 0x258, 4, 1, 0 },      /* SelSettingsH.unk8 (x2) */
    { 0x264, 4, 1, 0 },
    /* struct UnlockInfo (unlock.c) */
    { 0x278, 4, 1, 0 }, { 0x27C, 4, 1, 0 }, { 0x282, 2, 1, 0 }, { 0x284, 4, 1, 0 }, { 0x288, 2, 1, 0 },
    /* struct NameEntryDefault[5] (name_entry2.c) */
    { 0x364, 4, 5, 0x14 }, { 0x368, 4, 5, 0x14 }, { 0x36C, 4, 5, 0x14 },
    /* struct NameEntryRec[4][5] */
    { 0x3CC, 4, 20, 0x1C }, { 0x3D0, 4, 20, 0x1C }, { 0x3D8, 4, 20, 0x1C },
    /* ranking tables (pc_rank_block_copy), see s_rank below: NETimeRecord[72]
     * at 0x5F4 is all bytes; NERankB[20] (s32 at 8) */
    { 0x95C, 4, 20, 0x10 },
    /* struct NERankTables: NEA[35] (s16 at 8), NEB[5] (s16 at 8, 0xA), NEC bytes */
    { 0xA9C, 2, 35, 0xA }, { 0xBFA, 2, 5, 0xC }, { 0xBFC, 2, 5, 0xC },
    /* NERankF[15] (s32 at 8), NERankB[15] (s32 at 8) */
    { 0xCFC, 4, 15, 0xC }, { 0xDB0, 4, 15, 0x10 },
};

/* Ranking records whose decomp type was u32[] until port 1.1.2: name[7],
 * ape, then numbers; scoreOfs = the s32 field, 0 = none (all bytes).  Those
 * builds stored every 4-byte group byte-reversed (the u32 arrays held the GC
 * bytes as big-endian words), names included, and saved them that way. */
static const struct { u16 ofs; u8 count; u8 size; u8 scoreOfs; } s_rank[] = {
    { 0x5F4, 72, 12, 0 },    /* NETimeRecord: lbl_1_data_6C440..6C758 */
    { 0x954, 20, 16, 8 },    /* NERankB: lbl_1_data_6C7A0..6C890 */
    { 0xCF4, 15, 12, 8 },    /* NERankF: lbl_1_data_6CB40..6CBB8 */
    { 0xDA8, 15, 16, 8 },    /* NERankB: lbl_1_data_6CBF4..6CC94 */
};

/* a name the game could have written: printable up to the NUL within
 * name[7], then the ape number (0..3) */
static int rank_rec_ok(const u8 *r)
{
    int i;
    if (r[7] >= 0x20)
        return 0;
    for (i = 0; i < 7 && r[i]; i++)
        if (r[i] < 0x20 || r[i] > 0x7E)
            return 0;
    return i < 7;
}

/* name_entry2.c: tables whose save records fail this keep their defaults */
int pc_save_rank_rec_ok(const uint8_t *r)
{
    return rank_rec_ok(r);
}

static void rank_rec_reverse(u8 *r, int size, int scoreOfs)
{
    int w;
    for (w = 0; w < size; w += 4) {
        u8 t;
        if (scoreOfs && w == scoreOfs)
            continue;   /* the old u32 word already held the s32 natively */
        t = r[w]; r[w] = r[w + 3]; r[w + 3] = t;
        t = r[w + 1]; r[w + 1] = r[w + 2]; r[w + 2] = t;
    }
}

/* Saves written from 1.1.2 on carry this in the unused end of the game data
 * (the game uses storymode_save_files up to 0xE98; GameCube saves have
 * zeros there, and exported .gci files get the zeros back). */
#define RANK_MARK_OFS 0xF90
static const u8 k_rank_mark[8] = { 'P', 'C', 'R', 'A', 'N', 'K', '0', '1' };

void pc_save_rank_mark(uint8_t *story)
{
    memcpy(story + RANK_MARK_OFS, k_rank_mark, sizeof k_rank_mark);
}

int pc_save_rank_fix(uint8_t *story)
{
    unsigned i, k;
    int now = 0, old = 0;
    if (!memcmp(story + RANK_MARK_OFS, k_rank_mark, sizeof k_rank_mark)) {
        /* marked: current layout, unless an older build saved over it (it
         * keeps the mark but writes its own layout): every record that can
         * tell says old */
        for (i = 0; i < sizeof s_rank / sizeof s_rank[0]; i++)
            for (k = 0; k < s_rank[i].count; k++) {
                u8 r[16];
                int a, b;
                memcpy(r, story + s_rank[i].ofs + k * s_rank[i].size, s_rank[i].size);
                a = rank_rec_ok(r);
                rank_rec_reverse(r, s_rank[i].size, s_rank[i].scoreOfs);
                b = rank_rec_ok(r);
                now += a && !b;   /* valid both ways (e.g. empty): no vote */
                old += b && !a;
            }
        if (now || !old)
            return 0;
        printf("[rank-migrate] marked save re-saved by an older build: ");
    } else
        printf("[rank-migrate] save from a build before 1.1.2: ");
    for (i = 0; i < sizeof s_rank / sizeof s_rank[0]; i++)
        for (k = 0; k < s_rank[i].count; k++)
            rank_rec_reverse(story + s_rank[i].ofs + k * s_rank[i].size, s_rank[i].size, s_rank[i].scoreOfs);
    pc_save_rank_mark(story);
    printf("ranking tables converted to the current byte order\n");
    return 1;
}

static void swap_save_fields(u8 *data)
{
    unsigned i, k;
    for (i = 0; i < sizeof s_swap / sizeof s_swap[0]; i++)
        for (k = 0; k < s_swap[i].count; k++) {
            u8 *p = data + SAVE_DATA + s_swap[i].ofs + k * s_swap[i].stride;
            u8 t;
            if (s_swap[i].size == 2) {
                t = p[0]; p[0] = p[1]; p[1] = t;
            } else {
                t = p[0]; p[0] = p[3]; p[3] = t;
                t = p[1]; p[1] = p[2]; p[2] = t;
            }
        }
}

static int is_game_save(const PcCardFile *f, const u8 *data)
{
    return !strcmp(f->fileName, SAVE_FILE_NAME) && f->length == SAVE_SIZE && data[3] == SAVE_VERSION;
}

/* GameCube byte order -> the port's; 0 or a PCG_E_* code */
static int save_to_pc(const PcCardFile *f, u8 *data)
{
    u16 crc;
    if (!is_game_save(f, data))
        return PCG_E_NOT_CONVERTIBLE;
    crc = fn_800648C8(f->length - 2, data + 2);
    if (be16(data) == crc) {
        swap_save_fields(data);
        pc_save_rank_mark(data + SAVE_DATA);
        crc = fn_800648C8(f->length - 2, data + 2);
        memcpy(data, &crc, 2);   /* the port reads it as a native u16 */
        return 0;
    }
    if (data[0] == (u8)crc && data[1] == (u8)(crc >> 8))
        return 0;                /* already in the port's byte order */
    return PCG_E_SAVE_CRC;
}

/* the port's byte order -> GameCube; 0 or a PCG_E_* code */
static int save_to_gc(const PcCardFile *f, u8 *data)
{
    u16 crc, stored;
    int i, zero = 1;
    if (!is_game_save(f, data))
        return PCG_E_NOT_CONVERTIBLE;
    memcpy(&stored, data, 2);
    crc = fn_800648C8(f->length - 2, data + 2);
    if (stored != crc && stored != 0)   /* 0: early port builds, accepted by memcard.c */
        return PCG_E_SAVE_CRC;
    pc_save_rank_fix(data + SAVE_DATA);
    memset(data + SAVE_DATA + RANK_MARK_OFS, 0, sizeof k_rank_mark);
    swap_save_fields(data);
    /* saves of port builds before 1.1.2 have no stick gates (fn_8000A730
     * was a stub); the GC game loads them from here, so give it the port's */
    for (i = 0; i < 0x40; i++)
        if (data[SAVE_DATA + GATE_OFS + i]) zero = 0;
    if (zero)
        memcpy(data + SAVE_DATA + GATE_OFS, lbl_80081AC0, 0x40);
    crc = fn_800648C8(f->length - 2, data + 2);
    put16(data, crc);
    return 0;
}

/* ------------------------------------------------------------------ DEntry */

static int empty_dentry(const u8 *d)
{
    return be32(d) == 0xFFFFFFFFu;
}

static void parse_dentry(const u8 *d, PcCardFile *f)
{
    memset(f, 0, sizeof(*f));
    memcpy(f->gameName, d + 0x00, 4);
    memcpy(f->company, d + 0x04, 2);
    f->bannerFormat = d[0x07];
    memcpy(f->fileName, d + 0x08, PC_CARD_NAME_MAX);
    f->fileName[PC_CARD_NAME_MAX] = 0;
    f->time = be32(d + 0x28);
    f->iconAddr = be32(d + 0x2C);
    f->iconFormat = be16(d + 0x30);
    f->iconSpeed = be16(d + 0x32);
    f->permission = d[0x34];
    f->copyTimes = d[0x35];
    f->firstBlock = be16(d + 0x36);
    f->length = (u32)be16(d + 0x38) * BLK;
    f->commentAddr = be32(d + 0x3C);
    f->fileNo = -1;
}

static void build_dentry(const PcCardFile *f, u16 firstBlock, u8 *d)
{
    memset(d, 0, DENTRY_SIZE);
    memcpy(d + 0x00, f->gameName, 4);
    memcpy(d + 0x04, f->company, 2);
    d[0x06] = 0xFF;
    d[0x07] = f->bannerFormat;
    strncpy((char *)d + 0x08, f->fileName, PC_CARD_NAME_MAX);
    put32(d + 0x28, f->time);
    put32(d + 0x2C, f->iconAddr);
    put16(d + 0x30, f->iconFormat);
    put16(d + 0x32, f->iconSpeed);
    d[0x34] = f->permission;
    d[0x35] = f->copyTimes;
    put16(d + 0x36, firstBlock);
    put16(d + 0x38, (u16)(f->length / BLK));
    put16(d + 0x3A, 0xFFFF);
    put32(d + 0x3C, f->commentAddr);
}

/* ------------------------------------------------------------------ loading */

static u8 *read_file(const char *path, long *len)
{
    FILE *fp = fopen(path, "rb");
    u8 *buf;
    long n;
    *len = 0;
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0 || (n = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    buf = (u8 *)malloc(n > 0 ? (size_t)n : 1);
    if (buf && n > 0 && fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(fp);
    *len = n;
    return buf;
}

static int save_to_pc(const PcCardFile *f, u8 *data);

/* takes a copy of the data, in the port's byte order: 1 added, 0 not (*err
 * says why) */
static int set_add(PcGciSet *set, const PcCardFile *f, const u8 *data, int *err)
{
    int i;
    if (set->count >= PC_CARD_MAX_FILES)
        return 0;
    *err = PCG_E_FORMAT;
    for (i = 0; i < set->count; i++)
        if (!strcmp(set->files[i].fileName, f->fileName))
            return 0;   /* a card can't hold two files of one name */
    *err = PCG_E_WRITE;
    set->data[set->count] = (u8 *)malloc(f->length);
    if (!set->data[set->count])
        return 0;
    memcpy(set->data[set->count], data, f->length);
    *err = save_to_pc(f, set->data[set->count]);
    if (*err) {
        free(set->data[set->count]);
        return 0;
    }
    set->files[set->count] = *f;
    set->count++;
    return 1;
}

static int gci_name_ok(const PcCardFile *f)
{
    return f->fileName[0] != 0;
}

static int load_gci(const u8 *buf, long len, PcGciSet *set, PcGciReport *rep)
{
    PcCardFile f;
    char code[7];

    if (len < DENTRY_SIZE) {
        rep->err = PCG_E_FORMAT;
        return -1;
    }
    parse_dentry(buf, &f);
    if (!gci_name_ok(&f) || empty_dentry(buf)) {
        rep->err = PCG_E_FORMAT;
        return -1;
    }
    code_str(&f, code);
    if (f.length == 0 || (long)(DENTRY_SIZE + f.length) != len) {
        rep->err = PCG_E_GCI_SIZE;
        rep_line(rep, "%s: header says %u blocks (%u bytes of data), the file has %ld bytes of data",
                 f.fileName, (unsigned)(f.length / BLK), (unsigned)f.length, len - DENTRY_SIZE);
        return -1;
    }
    if (!is_smb2(&f)) {
        rep->err = is_smb2_other_region(&f) ? PCG_E_OTHER_REGION : PCG_E_OTHER_GAME;
        rep_line(rep, "%s: game code %s", f.fileName, code);
        rep_entry(rep, &f, PCG_F_SKIPPED, rep->err);
        rep->skipped++;
        return -1;
    }
    {
        int err;
        if (!set_add(set, &f, buf + DENTRY_SIZE, &err)) {
            rep->err = err;
            rep_line(rep, "%s: %s", f.fileName, pc_gci_err_text(err));
            rep_entry(rep, &f, err == PCG_E_NOT_CONVERTIBLE ? PCG_F_SKIPPED : PCG_F_FAILED, err);
            return -1;
        }
    }
    rep_line(rep, "%s: converted to the port's byte order", f.fileName);
    return 0;
}

/* SDK __CARDCheckSum: sums of the big-endian u16s and of their complements */
static void card_checksum(const u8 *p, int nbytes, u16 *cs, u16 *csi)
{
    int i;
    u16 a = 0, b = 0;
    for (i = 0; i + 1 < nbytes; i += 2) {
        u16 v = be16(p + i);
        a = (u16)(a + v);
        b = (u16)(b + (u16)~v);
    }
    if (a == 0xFFFF) a = 0;
    if (b == 0xFFFF) b = 0;
    *cs = a;
    *csi = b;
}

/* index (0/1) of the usable copy of a directory / BAT pair, -1 if none */
static int pick_copy(int valid0, u16 ctr0, int valid1, u16 ctr1)
{
    if (valid0 && valid1)
        return (int16_t)(ctr0 - ctr1) < 0 ? 1 : 0;
    if (valid0) return 0;
    if (valid1) return 1;
    return -1;
}

static int dir_valid(const u8 *d)
{
    u16 cs, csi;
    card_checksum(d, 0x1FFC, &cs, &csi);
    return cs == be16(d + 0x1FFC) && csi == be16(d + 0x1FFE);
}

static int bat_valid(const u8 *b)
{
    u16 cs, csi;
    card_checksum(b + 4, 0x1FFC, &cs, &csi);
    return cs == be16(b + 0) && csi == be16(b + 2);
}

static int load_raw(const u8 *buf, long len, PcGciSet *set, PcGciReport *rep)
{
    long nblocks = len / BLK;
    u16 cs, csi, sizeMb;
    long cardBlocks;
    const u8 *dir, *bat;
    int i, d, b, found = 0, smb2 = 0;
    u8 *data = NULL;

    card_checksum(buf, 0x1FC, &cs, &csi);
    sizeMb = be16(buf + 0x22);
    cardBlocks = (long)sizeMb * 16;
    if (cs != be16(buf + 0x1FC) || csi != be16(buf + 0x1FE) || sizeMb == 0 || (sizeMb & (sizeMb - 1)) ||
        sizeMb > 128 || cardBlocks > nblocks) {
        rep->err = PCG_E_RAW_HEADER;
        rep_line(rep, "header: checksum %04X/%04X (stored %04X/%04X), size %u Mbit, %ld blocks in the file",
                 cs, csi, be16(buf + 0x1FC), be16(buf + 0x1FE), sizeMb, nblocks);
        return -1;
    }
    d = pick_copy(dir_valid(buf + 1 * BLK), be16(buf + 1 * BLK + 0x1FFA),
                  dir_valid(buf + 2 * BLK), be16(buf + 2 * BLK + 0x1FFA));
    if (d < 0) {
        rep->err = PCG_E_RAW_DIR;
        return -1;
    }
    b = pick_copy(bat_valid(buf + 3 * BLK), be16(buf + 3 * BLK + 4),
                  bat_valid(buf + 4 * BLK), be16(buf + 4 * BLK + 4));
    if (b < 0) {
        rep->err = PCG_E_RAW_BAT;
        return -1;
    }
    dir = buf + (1 + d) * BLK;
    bat = buf + (3 + b) * BLK;
    rep_line(rep, "card image: %u Mbit (%ld blocks), directory copy %d, allocation table copy %d",
             sizeMb, cardBlocks, d + 1, b + 1);

    for (i = 0; i < PC_CARD_MAX_FILES; i++) {
        const u8 *de = dir + i * DENTRY_SIZE;
        PcCardFile f;
        char code[7];
        u32 k, nb;
        u16 blk;
        int bad = 0;

        if (empty_dentry(de))
            continue;
        parse_dentry(de, &f);
        found++;
        code_str(&f, code);
        if (!is_smb2(&f)) {
            rep_line(rep, "skipped %s %s (another game)", code, f.fileName);
            rep_entry(rep, &f, PCG_F_SKIPPED, is_smb2_other_region(&f) ? PCG_E_OTHER_REGION : PCG_E_OTHER_GAME);
            rep->skipped++;
            continue;
        }
        smb2++;
        nb = f.length / BLK;
        free(data);
        data = (u8 *)malloc(f.length ? f.length : 1);
        if (!data) {
            rep->err = PCG_E_WRITE;
            return -1;
        }
        /* follow the chain: the map holds the next block of blocks 5.. */
        blk = f.firstBlock;
        for (k = 0; k < nb && !bad; k++) {
            if (blk < 5 || blk >= cardBlocks) {
                bad = 1;
                break;
            }
            memcpy(data + k * BLK, buf + (long)blk * BLK, BLK);
            blk = be16(bat + 0x0A + (blk - 5) * 2);
        }
        if (nb == 0 || bad || blk != 0xFFFF) {
            rep_line(rep, "%s: broken block chain on the card (first block %u, %u blocks)",
                     f.fileName, f.firstBlock, (unsigned)nb);
            rep_entry(rep, &f, PCG_F_FAILED, PCG_E_RAW_BAT);
            rep->failed++;
            if (!rep->err) rep->err = PCG_E_RAW_BAT;
            continue;
        }
        {
            int err;
            if (!set_add(set, &f, data, &err)) {
                rep_line(rep, "%s: %s", f.fileName, pc_gci_err_text(err));
                if (err == PCG_E_NOT_CONVERTIBLE) {
                    rep_entry(rep, &f, PCG_F_SKIPPED, err);
                    rep->skipped++;
                } else {
                    rep_entry(rep, &f, PCG_F_FAILED, err);
                    rep->failed++;
                    if (!rep->err) rep->err = err;
                }
            }
        }
    }
    free(data);
    rep_line(rep, "%d file(s) on the card, %d of them Super Monkey Ball 2 (USA)", found, smb2);
    if (set->count == 0 && rep->failed == 0) {
        rep->err = PCG_E_NO_SAVE;
        return -1;
    }
    return 0;
}

void pc_gci_free(PcGciSet *set)
{
    int i;
    for (i = 0; i < set->count; i++)
        free(set->data[i]);
    set->count = 0;
}

static int has_ext(const char *path, const char *ext)
{
    size_t l = strlen(path), e = strlen(ext);
    size_t i;
    if (l < e) return 0;
    for (i = 0; i < e; i++)
        if (tolower((unsigned char)path[l - e + i]) != ext[i]) return 0;
    return 1;
}

int pc_gci_load(const char *path, PcGciSet *set, PcGciReport *rep)
{
    long len;
    u8 *buf;
    int r, i, n;
    PcCardFile *on;

    memset(set, 0, sizeof(*set));
    buf = read_file(path, &len);
    if (!buf) {
        rep->err = PCG_E_OPEN;
        return -1;
    }
    /* a .gci is the 64-byte entry + whole blocks, a card image whole blocks */
    if (len >= 5 * BLK && len % BLK == 0 && !has_ext(path, ".gci"))
        r = load_raw(buf, len, set, rep);
    else if (len % BLK == DENTRY_SIZE || has_ext(path, ".gci"))
        r = load_gci(buf, len, set, rep);
    else if (has_ext(path, ".raw") || has_ext(path, ".mci") || has_ext(path, ".gcp")) {
        rep->err = PCG_E_RAW_HEADER;
        rep_line(rep, "%ld bytes is not a whole number of 8 KiB blocks", len);
        r = -1;
    } else {
        rep->err = PCG_E_FORMAT;
        r = -1;
    }
    free(buf);
    if (r < 0) {
        pc_gci_free(set);
        return -1;
    }
    on = (PcCardFile *)malloc(sizeof(PcCardFile) * PC_CARD_MAX_FILES);
    n = on ? pc_card_list(on, PC_CARD_MAX_FILES) : 0;
    for (i = 0; i < set->count; i++) {
        int j;
        set->exists[i] = 0;
        for (j = 0; j < n; j++)
            if (!strcmp(on[j].fileName, set->files[i].fileName))
                set->exists[i] = 1;
    }
    free(on);
    return 0;
}

int pc_gci_store(PcGciSet *set, PcGciReport *rep)
{
    int i, done = 0;
    for (i = 0; i < set->count; i++) {
        char backup[200];
        int r = pc_card_store(&set->files[i], set->data[i], backup, sizeof(backup));
        if (r == 0) {
            if (backup[0]) {
                rep_line(rep, "imported %s (%u blocks), replacing the save that was there "
                         "(kept as memcard\\backup\\%s.bin/.meta)",
                         set->files[i].fileName, (unsigned)(set->files[i].length / BLK), backup);
                rep_entry(rep, &set->files[i], PCG_F_REPLACED, 0);
                rep->replaced++;
            } else {
                rep_line(rep, "imported %s (%u blocks)", set->files[i].fileName,
                         (unsigned)(set->files[i].length / BLK));
                rep_entry(rep, &set->files[i], PCG_F_IMPORTED, 0);
            }
            rep->imported++;
            done++;
        } else {
            int e = r == PC_CARD_E_FULL || r == PC_CARD_E_NOENT ? PCG_E_FULL : PCG_E_WRITE;
            rep_line(rep, "%s: %s", set->files[i].fileName, pc_gci_err_text(e));
            rep_entry(rep, &set->files[i], PCG_F_FAILED, e);
            rep->failed++;
            if (!rep->err) rep->err = e;
        }
    }
    return done;
}

int pc_gci_import(const char *path, PcGciReport *rep)
{
    PcGciSet *set = (PcGciSet *)calloc(1, sizeof(PcGciSet));
    int n;
    if (!set) {
        rep->err = PCG_E_WRITE;
        return -1;
    }
    if (pc_gci_load(path, set, rep) != 0) {
        free(set);
        return -1;
    }
    n = pc_gci_store(set, rep);
    pc_gci_free(set);
    free(set);
    return n;
}

/* ------------------------------------------------------------------ export */

void pc_gci_default_name(const PcCardFile *f, char *out, size_t outsz)
{
    char name[PC_CARD_NAME_MAX + 1];
    char maker[3], game[5];
    int i;
    for (i = 0; f->fileName[i] && i < PC_CARD_NAME_MAX; i++) {
        unsigned char c = (unsigned char)f->fileName[i];
        name[i] = (c < 0x20 || c >= 0x7F || strchr("\\/:*?\"<>|", c)) ? '_' : (char)c;
    }
    name[i] = 0;
    for (i = 0; i < 2; i++) maker[i] = isalnum(f->company[i]) ? (char)f->company[i] : '_';
    maker[2] = 0;
    for (i = 0; i < 4; i++) game[i] = isalnum(f->gameName[i]) ? (char)f->gameName[i] : '_';
    game[4] = 0;
    snprintf(out, outsz, "%s-%s-%s.gci", maker, game, name);
}

int pc_gci_export(const char *dir, const char *first, PcGciReport *rep)
{
    PcCardFile *files = (PcCardFile *)malloc(sizeof(PcCardFile) * PC_CARD_MAX_FILES);
    int n, i, written = 0;
    u16 nextBlock = 5;

    if (!files) {
        rep->err = PCG_E_EXPORT_WRITE;
        return -1;
    }
    n = pc_card_list(files, PC_CARD_MAX_FILES);
    for (i = 0; i < n; i++) {
        char path[1200], name[80];
        u8 dentry[DENTRY_SIZE];
        u8 *data;
        FILE *fp;
        int ok;
        u16 fb;

        /* a block number for files the card never placed (made by the
         * port): where they would sit on a card holding just these files */
        fb = files[i].firstBlock ? files[i].firstBlock : nextBlock;
        nextBlock = (u16)(nextBlock + files[i].length / BLK);
        if (!is_smb2(&files[i]))
            continue;
        pc_gci_default_name(&files[i], name, sizeof(name));
        if (first && written == 0)
            snprintf(path, sizeof(path), "%s", first);
        else {
            size_t l = strlen(dir);
            snprintf(path, sizeof(path), "%s%s%s", dir,
                     (l && dir[l - 1] != '/' && dir[l - 1] != '\\') ? PSEP : "", name);
        }
        data = pc_card_read(&files[i]);
        if (!data) {
            rep_line(rep, "%s: its data file can't be read", files[i].fileName);
            rep_entry(rep, &files[i], PCG_F_FAILED, PCG_E_OPEN);
            rep->failed++;
            continue;
        }
        {
            int err = save_to_gc(&files[i], data);
            if (err) {
                rep_line(rep, "%s: not exported: %s", files[i].fileName, pc_gci_err_text(err));
                rep_entry(rep, &files[i], err == PCG_E_NOT_CONVERTIBLE ? PCG_F_SKIPPED : PCG_F_FAILED, err);
                if (err == PCG_E_NOT_CONVERTIBLE)
                    rep->skipped++;
                else {
                    rep->failed++;
                    if (!rep->err) rep->err = err;
                }
                free(data);
                continue;
            }
        }
        build_dentry(&files[i], fb, dentry);
        fp = fopen(path, "wb");
        ok = fp && fwrite(dentry, 1, DENTRY_SIZE, fp) == DENTRY_SIZE &&
             fwrite(data, 1, files[i].length, fp) == files[i].length;
        if (fp) {
            ok = fflush(fp) == 0 && ok;
            ok = fclose(fp) == 0 && ok;
        }
        free(data);
        if (!ok) {
            remove(path);
            rep_line(rep, "%s: can't write %s", files[i].fileName, path);
            rep_entry(rep, &files[i], PCG_F_FAILED, PCG_E_EXPORT_WRITE);
            rep->failed++;
            if (!rep->err) rep->err = PCG_E_EXPORT_WRITE;
            continue;
        }
        rep_line(rep, "exported %s (%u blocks) -> %s", files[i].fileName,
                 (unsigned)(files[i].length / BLK), path);
        rep_entry(rep, &files[i], PCG_F_EXPORTED, 0);
        rep->exported++;
        written++;
    }
    free(files);
    if (written == 0 && rep->failed == 0) {
        rep->err = PCG_E_NO_FILES_ON_CARD;
        return -1;
    }
    return written;
}

/* ------------------------------------------------------------------ drop-in folder */

static PcGciReport s_startup;
static int s_startup_set;

const PcGciReport *pc_gci_startup_report(void) { return s_startup_set ? &s_startup : NULL; }
void pc_gci_startup_report_clear(void) { s_startup_set = 0; }

/* moves src to <dir>/<name>, adding " (2)" etc. when the name is taken */
static void move_to(const char *src, const char *dir, const char *name)
{
    char dst[1200];
    int k;
    const char *dot = strrchr(name, '.');
    int stem = dot ? (int)(dot - name) : (int)strlen(name);
    make_dir(dir);
    for (k = 1; k < 1000; k++) {
        FILE *t;
        if (k == 1)
            snprintf(dst, sizeof(dst), "%s%s", dir, name);
        else
            snprintf(dst, sizeof(dst), "%s%.*s (%d)%s", dir, stem, name, k, dot ? dot : "");
        t = fopen(dst, "rb");
        if (t) {
            fclose(t);
            continue;
        }
        if (rename(src, dst) == 0)
            return;
        break;
    }
    fprintf(stderr, "[gci] could not move %s to %s\n", src, dir);
}

int pc_gci_import_dropins(void)
{
    char dir[700], done[720], failed[720];
    DIR *d;
    struct dirent *de;
    char (*names)[260] = NULL;
    int n = 0, cap = 0, i;

    snprintf(dir, sizeof(dir), "%simport/", pc_card_base_dir());
    make_dir(dir);   /* so the folder is there to find */
    d = opendir(dir);
    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL) {
        if (!has_ext(de->d_name, ".gci") && !has_ext(de->d_name, ".raw"))
            continue;
        if (strlen(de->d_name) >= 260)
            continue;
        if (n == cap) {
            void *nn = realloc(names, (size_t)(cap ? cap * 2 : 8) * sizeof(*names));
            if (!nn) break;
            names = (char (*)[260])nn;
            cap = cap ? cap * 2 : 8;
        }
        strcpy(names[n++], de->d_name);
    }
    closedir(d);
    if (n == 0) {
        free(names);
        return 0;
    }
    snprintf(done, sizeof(done), "%sdone/", dir);
    snprintf(failed, sizeof(failed), "%sfailed/", dir);
    memset(&s_startup, 0, sizeof(s_startup));
    s_startup_set = 1;
    for (i = 0; i < n; i++) {
        char path[1000];
        PcGciReport r;
        int got;
        memset(&r, 0, sizeof(r));
        snprintf(path, sizeof(path), "%s%s", dir, names[i]);
        got = pc_gci_import(path, &r);
        rep_line(&s_startup, "%s:", names[i]);
        if (r.text[0])
            strncat(s_startup.text, r.text, sizeof(s_startup.text) - strlen(s_startup.text) - 1);
        if (got < 0 || (got == 0 && r.failed))
            rep_line(&s_startup, "  not imported: %s (moved to memcard\\import\\failed)", pc_gci_err_text(r.err));
        if (got < 0 && r.nentries == 0 && s_startup.nentries < PC_CARD_MAX_FILES) {
            /* the dropped file itself, for the in-game notice */
            PcGciEntry *e = &s_startup.entries[s_startup.nentries++];
            memset(e, 0, sizeof(*e));
            snprintf(e->name, sizeof(e->name), "%s", names[i]);
            e->status = PCG_F_FAILED;
            e->err = r.err;
        }
        {
            int k;
            for (k = 0; k < r.nentries && s_startup.nentries < PC_CARD_MAX_FILES; k++)
                s_startup.entries[s_startup.nentries++] = r.entries[k];
        }
        s_startup.imported += r.imported;
        s_startup.replaced += r.replaced;
        s_startup.skipped += r.skipped;
        s_startup.failed += r.failed + (got < 0 && r.skipped == 0 ? 1 : 0);
        if (got < 0 && !s_startup.err) s_startup.err = r.err;
        move_to(path, got > 0 ? done : failed, names[i]);
    }
    free(names);
    return n;
}

/* ------------------------------------------------------------------ command line */

/* --import-save <file> / --export-save <folder>: does it, prints what
 * happened and returns the exit code; -1 when neither option is given. */
int pc_gci_cli(int argc, char **argv)
{
    const char *imp = NULL, *exp = NULL;
    PcGciReport *rep;
    int i, rc = 0, n;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--import-save") && i + 1 < argc) imp = argv[++i];
        else if (!strcmp(argv[i], "--export-save") && i + 1 < argc) exp = argv[++i];
        else if (!strcmp(argv[i], "--import-save") || !strcmp(argv[i], "--export-save")) {
            printf("usage: --import-save <file.gci|card.raw>   or   --export-save <folder>\n");
            return 2;
        }
    }
    if (!imp && !exp)
        return -1;
    rep = (PcGciReport *)calloc(1, sizeof(PcGciReport));
    if (!rep)
        return 1;
    printf("Memory card folder: %s\n", pc_card_slot_dir());
    if (imp) {
        printf("Importing %s\n", imp);
        n = pc_gci_import(imp, rep);
        fputs(rep->text, stdout);
        if (n < 0 || rep->failed) {
            printf("Import failed: %s\n", pc_gci_err_text(rep->err ? rep->err : PCG_E_WRITE));
            rc = 1;
        }
        if (n > 0)
            printf("Imported %d file(s)%s. Start the game to play with the imported save.\n", n,
                   rep->replaced ? "; the replaced save was backed up to memcard\\backup" : "");
    }
    if (exp && rc == 0) {
        memset(rep, 0, sizeof(*rep));
        make_dir(exp);
        printf("Exporting to %s\n", exp);
        n = pc_gci_export(exp, NULL, rep);
        fputs(rep->text, stdout);
        if (n < 0 || rep->failed) {
            printf("Export failed: %s\n", pc_gci_err_text(rep->err ? rep->err : PCG_E_EXPORT_WRITE));
            rc = 1;
        } else
            printf("Exported %d file(s).\n", n);
    }
    fflush(stdout);
    free(rep);
    return rc;
}

/* startup: the drop-in folder, reported on the console / log */
void pc_gci_startup(void)
{
    const PcGciReport *r;
    if (pc_gci_import_dropins() <= 0)
        return;
    r = pc_gci_startup_report();
    printf("[gci] memcard\\import:\n%s", r->text);
    printf("[gci] %d save file(s) imported, %d skipped, %d not imported\n", r->imported, r->skipped,
           r->failed);
    fflush(stdout);
}
