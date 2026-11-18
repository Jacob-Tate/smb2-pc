/* File-backed GameCube memory card (CARD library) emulation, at the file
 * level (no raw card image).  Slot A (chan 0) is a directory:
 *
 *     memcard/slotA/<name>.bin   raw file contents, stat.length bytes
 *     memcard/slotA/<name>.meta  "key=value" text: CARDStat metadata and the
 *                                directory slot (fileNo)
 *
 * <name> is the GC file name with every byte outside [A-Za-z0-9._-] escaped
 * as %XX.  Slot B (chan 1) always reports CARD_RESULT_NOCARD.  The card is a
 * 16 Mbit (251 user block) card with 8 KiB sectors.
 *
 * Async calls do their I/O synchronously: they set the channel result and
 * call the completion callback before returning.  That is fine for the only
 * caller, decomp main_loop/memcard.c, which polls CARDGetResultCode() and
 * treats BUSY (-1) as "not yet".  The mount callback it passes is the detach
 * callback (card removed), so it is never called. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "netplay/session.h"
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#endif

typedef int32_t s32;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint64_t u64;

#define CARD_RESULT_READY        0
#define CARD_RESULT_BUSY        -1
#define CARD_RESULT_WRONGDEVICE -2
#define CARD_RESULT_NOCARD      -3
#define CARD_RESULT_NOFILE      -4
#define CARD_RESULT_IOERROR     -5
#define CARD_RESULT_BROKEN      -6
#define CARD_RESULT_EXIST       -7
#define CARD_RESULT_NOENT       -8
#define CARD_RESULT_INSSPACE    -9
#define CARD_RESULT_NOPERM     -10
#define CARD_RESULT_LIMIT      -11
#define CARD_RESULT_NAMETOOLONG -12
#define CARD_RESULT_ENCODING   -13
#define CARD_RESULT_CANCELED   -14
#define CARD_RESULT_FATAL_ERROR -128

#define CARD_FILENAME_MAX 32
#define CARD_MAX_FILE     127
#define CARD_READ_SIZE    512
#define CARD_MEM_SIZE_MBIT 16                    /* CARD_MEM_SIZE 16 Mbit */
#define CARD_SECTOR_SIZE  0x2000
#define CARD_USER_BLOCKS  ((CARD_MEM_SIZE_MBIT * 1024 * 1024 / 8) / CARD_SECTOR_SIZE - 5) /* 251 */
#define CARD_SERIAL_NO    0x534D42325043ULL      /* "SMB2PC" */
#define CARD_ATTR_PUBLIC  0x04

/* Real SDK layouts (match decomp main_loop/memcard.c) */
typedef struct {
    s32 chan;
    s32 fileNo;
    s32 offset;
    s32 length;
    u16 iBlock;
} CARDFileInfo;

typedef struct {
    char fileName[CARD_FILENAME_MAX];
    u32 length;
    u32 time;          /* seconds since 2000-01-01 */
    u8 gameName[4];
    u8 company[2];
    u8 bannerFormat;
    u8 __padding;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u32 commentAddr;
    u32 offsetBanner;
    u32 offsetBannerTlut;
    u32 offsetIcon[8];
    u32 offsetIconTlut;
    u32 offsetData;
} CARDStat;

typedef char card_stat_size_check[sizeof(CARDStat) == 0x6C ? 1 : -1];
typedef char card_fileinfo_size_check[sizeof(CARDFileInfo) == 0x14 ? 1 : -1];

typedef void (*CARDCallback)(s32 chan, s32 result);

typedef struct {
    int used;
    char fileName[CARD_FILENAME_MAX + 1];
    u32 length;
    u32 time;
    u8 gameName[4];
    u8 company[2];
    u8 bannerFormat;
    u32 iconAddr;
    u16 iconFormat;
    u16 iconSpeed;
    u32 commentAddr;
    u8 permission;
    u8 copyTimes;
    u16 firstBlock;    /* imported files: their block on the source card */
    u8 *mem;           /* session card (E2): the file's data, never on disk */
} CardEntry;

static CardEntry s_dir[CARD_MAX_FILE];
static int s_mounted;
static int s_loaded;
static s32 s_result[2] = { CARD_RESULT_READY, CARD_RESULT_NOCARD };
static s32 s_xferred[2];
static char s_root[512];

void shim_log_once(const char *sym);
void *DVDGetCurrentDiskID(void);

/* ------------------------------------------------------------------ */
/* paths                                                               */

static void card_mkdir(const char *p)
{
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0777);
#endif
}

static const char *card_root(void)
{
    char base[400];
    char sub[480];

    if (s_root[0])
        return s_root;
    base[0] = 0;
    {   /* installed mode (install.ini): <userdata or --save-dir>\memcard */
        extern const char *pc_userdata_base(void);
        const char *ud = pc_userdata_base();
        if (ud) {
            snprintf(base, sizeof(base), "%s", ud);
            goto have_base;
        }
    }
#ifdef _WIN32
    {
        DWORD n = GetModuleFileNameA(NULL, base, sizeof(base));
        char *slash;
        if (n == 0 || n >= sizeof(base))
            base[0] = 0;
        else {
            slash = strrchr(base, '\\');
            if (!slash) slash = strrchr(base, '/');
            if (slash) slash[1] = 0; else base[0] = 0;
        }
    }
#endif
have_base:
    snprintf(sub, sizeof(sub), "%smemcard", base);
    card_mkdir(sub);
    snprintf(s_root, sizeof(s_root), "%smemcard/slotA/", base);
    card_mkdir(s_root);
    return s_root;
}

static void escape_name(const char *name, char *out, size_t outsz)
{
    size_t o = 0;
    const unsigned char *p = (const unsigned char *)name;

    for (; *p && o + 4 < outsz; p++) {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9') || *p == '.' || *p == '_' || *p == '-')
            out[o++] = (char)*p;
        else
            o += (size_t)snprintf(out + o, outsz - o, "%%%02X", *p);
    }
    out[o] = 0;
}

static void entry_path(const char *fileName, const char *ext, char *out, size_t outsz)
{
    char esc[CARD_FILENAME_MAX * 3 + 1];
    escape_name(fileName, esc, sizeof(esc));
    snprintf(out, outsz, "%s%s%s", card_root(), esc, ext);
}

/* ------------------------------------------------------------------ */
/* sidecar                                                             */

static int write_meta(const CardEntry *e, int fileNo)
{
    char path[600];
    FILE *f;

    entry_path(e->fileName, ".meta", path, sizeof(path));
    f = fopen(path, "wb");
    if (!f)
        return CARD_RESULT_IOERROR;
    fprintf(f, "fileNo=%d\n", fileNo);
    fprintf(f, "fileName=%s\n", e->fileName);
    fprintf(f, "length=%u\n", (unsigned)e->length);
    fprintf(f, "time=%u\n", (unsigned)e->time);
    fprintf(f, "gameName=%02X%02X%02X%02X\n", e->gameName[0], e->gameName[1], e->gameName[2], e->gameName[3]);
    fprintf(f, "company=%02X%02X\n", e->company[0], e->company[1]);
    fprintf(f, "bannerFormat=%u\n", (unsigned)e->bannerFormat);
    fprintf(f, "iconAddr=0x%08X\n", (unsigned)e->iconAddr);
    fprintf(f, "iconFormat=0x%04X\n", (unsigned)e->iconFormat);
    fprintf(f, "iconSpeed=0x%04X\n", (unsigned)e->iconSpeed);
    fprintf(f, "commentAddr=0x%08X\n", (unsigned)e->commentAddr);
    fprintf(f, "permission=0x%02X\n", (unsigned)e->permission);
    fprintf(f, "copyTimes=%u\n", (unsigned)e->copyTimes);
    if (e->firstBlock)
        fprintf(f, "firstBlock=%u\n", (unsigned)e->firstBlock);
    fflush(f);
    if (ferror(f)) {
        fclose(f);
        return CARD_RESULT_IOERROR;
    }
    fclose(f);
    return CARD_RESULT_READY;
}

static int save_meta(int fileNo)
{
    if (pc_session_active())   /* in-memory card: no sidecar */
        return CARD_RESULT_READY;
    return write_meta(&s_dir[fileNo], fileNo);
}

static void parse_hex_bytes(const char *s, u8 *out, int n)
{
    int i;
    unsigned v;
    for (i = 0; i < n; i++) {
        if (sscanf(s + i * 2, "%2x", &v) != 1)
            return;
        out[i] = (u8)v;
    }
}

/* <name>.meta -> *pe; 0 if it is not a usable entry */
static int parse_meta(const char *path, CardEntry *pe, int *pfileNo)
{
    FILE *f = fopen(path, "rb");
    char line[256];
    CardEntry e;
    int fileNo = -1;

    if (!f)
        return 0;
    memset(&e, 0, sizeof(e));
    e.iconAddr = 0xFFFFFFFFu;
    e.commentAddr = 0xFFFFFFFFu;
    e.permission = CARD_ATTR_PUBLIC;   /* .meta files written before these keys */
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        char *val;
        size_t l;
        if (!eq)
            continue;
        *eq = 0;
        val = eq + 1;
        l = strlen(val);
        while (l && (val[l - 1] == '\n' || val[l - 1] == '\r'))
            val[--l] = 0;
        if (!strcmp(line, "fileNo")) fileNo = atoi(val);
        else if (!strcmp(line, "fileName")) { strncpy(e.fileName, val, CARD_FILENAME_MAX); e.fileName[CARD_FILENAME_MAX] = 0; }
        else if (!strcmp(line, "length")) e.length = (u32)strtoul(val, NULL, 0);
        else if (!strcmp(line, "time")) e.time = (u32)strtoul(val, NULL, 0);
        else if (!strcmp(line, "gameName")) parse_hex_bytes(val, e.gameName, 4);
        else if (!strcmp(line, "company")) parse_hex_bytes(val, e.company, 2);
        else if (!strcmp(line, "bannerFormat")) e.bannerFormat = (u8)strtoul(val, NULL, 0);
        else if (!strcmp(line, "iconAddr")) e.iconAddr = (u32)strtoul(val, NULL, 0);
        else if (!strcmp(line, "iconFormat")) e.iconFormat = (u16)strtoul(val, NULL, 0);
        else if (!strcmp(line, "iconSpeed")) e.iconSpeed = (u16)strtoul(val, NULL, 0);
        else if (!strcmp(line, "commentAddr")) e.commentAddr = (u32)strtoul(val, NULL, 0);
        else if (!strcmp(line, "permission")) e.permission = (u8)strtoul(val, NULL, 0);
        else if (!strcmp(line, "copyTimes")) e.copyTimes = (u8)strtoul(val, NULL, 0);
        else if (!strcmp(line, "firstBlock")) e.firstBlock = (u16)strtoul(val, NULL, 0);
    }
    fclose(f);
    if (!e.fileName[0] || e.length == 0 || (e.length % CARD_SECTOR_SIZE) != 0)
        return 0;
    {   /* a .meta without its .bin holds no save: leave it off the card (a
         * new save under that name backs the .meta up first, see
         * backup_stray) */
        char bin[600];
        struct stat st;
        entry_path(e.fileName, ".bin", bin, sizeof(bin));
        if (stat(bin, &st) != 0) {
            fprintf(stderr, "[card] %s has no data file; ignoring it\n", path);
            return 0;
        }
    }
    *pe = e;
    *pfileNo = fileNo;
    return 1;
}

static void load_meta(const char *path)
{
    CardEntry e;
    int fileNo, i;

    if (!parse_meta(path, &e, &fileNo))
        return;
    for (i = 0; i < CARD_MAX_FILE; i++)
        if (s_dir[i].used && !strcmp(s_dir[i].fileName, e.fileName))
            return;
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE || s_dir[fileNo].used) {
        for (fileNo = 0; fileNo < CARD_MAX_FILE && s_dir[fileNo].used; fileNo++)
            ;
        if (fileNo >= CARD_MAX_FILE)
            return;
    }
    e.used = 1;
    s_dir[fileNo] = e;
}

static u32 card_time_now(void);

static int unescape_name(const char *esc, size_t len, char *out, size_t outsz)
{
    size_t i, o = 0;
    unsigned v;

    for (i = 0; i < len; i++) {
        if (o + 1 >= outsz)
            return 0;
        if (esc[i] == '%') {
            if (i + 2 >= len || sscanf(esc + i + 1, "%2x", &v) != 1)
                return 0;
            out[o++] = (char)v;
            i += 2;
        } else
            out[o++] = esc[i];
    }
    out[o] = 0;
    return o > 0;
}

/* A <name>.bin whose .meta is missing or unreadable would otherwise be
 * invisible to the game: it would see "no save", create a new one over it,
 * and only the stray backup would remain.  Instead list it on the card with
 * this game's ID, so the game reads it and its own checksum decides whether
 * it is good (loads) or not (its "corrupt" dialog, after a backup).  The
 * .meta is rewritten by the game's next save (CARDSetStatus). */
static void adopt_orphan_bins(void)
{
    DIR *d = opendir(card_root());
    struct dirent *de;
    const u8 *id = (const u8 *)DVDGetCurrentDiskID();

    if (!d)
        return;
    while ((de = readdir(d)) != NULL) {
        size_t l = strlen(de->d_name);
        char name[CARD_FILENAME_MAX + 2], path[600];
        struct stat st;
        CardEntry *e;
        int i, no = -1, known = 0;

        if (l <= 4 || strcmp(de->d_name + l - 4, ".bin") != 0)
            continue;
        if (!unescape_name(de->d_name, l - 4, name, sizeof(name)) || strlen(name) > CARD_FILENAME_MAX)
            continue;
        for (i = 0; i < CARD_MAX_FILE; i++) {
            if (s_dir[i].used && !strcmp(s_dir[i].fileName, name))
                known = 1;
            else if (!s_dir[i].used && no < 0)
                no = i;
        }
        snprintf(path, sizeof(path), "%s%s", card_root(), de->d_name);
        if (known || no < 0 || stat(path, &st) != 0 || st.st_size <= 0 ||
            (st.st_size % CARD_SECTOR_SIZE) != 0)
            continue;
        e = &s_dir[no];
        memset(e, 0, sizeof(*e));
        strcpy(e->fileName, name);
        e->length = (u32)st.st_size;
        e->time = card_time_now();
        memcpy(e->gameName, id, 4);
        memcpy(e->company, id + 4, 2);
        e->iconAddr = 0xFFFFFFFFu;
        e->commentAddr = 0xFFFFFFFFu;
        e->permission = CARD_ATTR_PUBLIC;
        e->used = 1;
        fprintf(stderr, "[card] %s has no usable .meta; listing it as this game's file\n", de->d_name);
    }
    closedir(d);
}

static void load_directory(void)
{
    DIR *d;
    struct dirent *de;
    char path[600];

    if (pc_session_active()) {   /* E1 D4: the session card starts empty and lives in RAM */
        if (!s_loaded)
            memset(s_dir, 0, sizeof(s_dir));
        s_loaded = 1;
        return;
    }
    memset(s_dir, 0, sizeof(s_dir));
    d = opendir(card_root());
    if (!d)
        return;
    while ((de = readdir(d)) != NULL) {
        size_t l = strlen(de->d_name);
        if (l > 5 && !strcmp(de->d_name + l - 5, ".meta")) {
            snprintf(path, sizeof(path), "%s%s", card_root(), de->d_name);
            load_meta(path);
        }
    }
    closedir(d);
    adopt_orphan_bins();
    s_loaded = 1;
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */

static u32 card_time_now(void)
{
    if (pc_session_active())   /* E1 D4: fixed timestamp, same on every peer */
        return pc_session_card_time();
    time_t t = time(NULL);
    /* local-time seconds since 2000-01-01, like OSTicksToSeconds(OSGetTime()) */
    struct tm *lt = localtime(&t);
    time_t local = t;
    if (lt) {
        struct tm g = *gmtime(&t);
        local = t + (time_t)difftime(mktime(lt), mktime(&g));
    }
    return (u32)(local - 946684800);
}

static int used_blocks(void)
{
    int i, n = 0;
    for (i = 0; i < CARD_MAX_FILE; i++)
        if (s_dir[i].used)
            n += (int)(s_dir[i].length / CARD_SECTOR_SIZE);
    return n;
}

static int is_this_game(const CardEntry *e)
{
    const u8 *id = (const u8 *)DVDGetCurrentDiskID();
    return memcmp(e->gameName, id, 4) == 0 && memcmp(e->company, id + 4, 2) == 0;
}

static int find_file(const char *name)
{
    int i;
    for (i = 0; i < CARD_MAX_FILE; i++)
        if (s_dir[i].used && is_this_game(&s_dir[i]) &&
            strncmp(s_dir[i].fileName, name, CARD_FILENAME_MAX) == 0)
            return i;
    return -1;
}

/* SDK __CARDGetControlBlock equivalent: only chan 0 has a card */
static int s_frozen;

static s32 check_chan(s32 chan, int needMount)
{
    if (chan != 0 || s_frozen)
        return CARD_RESULT_NOCARD;
    if (needMount && !s_mounted)
        return CARD_RESULT_NOCARD;
    return CARD_RESULT_READY;
}

static const char *const s_opname = NULL; /* shadowed by name-taking ops */

static s32 finish_op(const char *op, const char *name, s32 chan, s32 result, CARDCallback cb)
{
    fprintf(stderr, "[card] %s(%d%s%s) -> %d\n", op, (int)chan, name ? ", " : "", name ? name : "", (int)result);
    if (chan == 0 || chan == 1)
        s_result[chan] = result;
    if (cb && (result == CARD_RESULT_READY || result == CARD_RESULT_IOERROR))
        cb(chan, result);
    return result;
}
#define FINISH(chan, r, cb) finish_op(__func__, s_opname, chan, r, cb)

/* ------------------------------------------------------------------ */
/* backups                                                             */
/*
 * Nothing the port does may silently destroy a save the game cannot read.
 * Before such a file can be deleted or replaced, a byte-exact copy of its
 * .bin/.meta goes to <exe dir>/memcard/backup/ (outside slotA/, so the card
 * never sees it):
 *   - pc_card_backup_file(): called by memcard.c when the game's own
 *     checksum / version check rejects the file ("corrupt" error dialog,
 *     whose only offered action is "Delete");
 *   - backup_stray(): a .bin/.meta on disk under a name the card directory
 *     does not know (e.g. an unparsable .meta), right before a create or
 *     rename would reuse that name.
 * Backups are named <name>.<reason>-<fnv1a of contents>.bin/.meta, so the
 * same bad file is only copied once however often it is retried.
 */

static const char *backup_root(void)
{
    static char root[520];
    if (!root[0]) {
        const char *slot = card_root();          /* "<base>memcard/slotA/" */
        size_t l = strlen(slot);
        if (l >= 6 && l - 6 < sizeof(root) - 8) {
            memcpy(root, slot, l - 6);           /* "<base>memcard/" */
            strcpy(root + (l - 6), "backup/");
            card_mkdir(root);
        }
    }
    return root;
}

static u8 *read_whole(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    u8 *buf;
    long n;

    *len = -1;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (u8 *)malloc(n > 0 ? (size_t)n : 1);
    if (buf && n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    if (buf)
        *len = n;
    return buf;
}

static int write_whole(const char *path, const u8 *buf, long len)
{
    FILE *f = fopen(path, "wb");
    int ok;
    if (!f)
        return 0;
    ok = len <= 0 || fwrite(buf, 1, (size_t)len, f) == (size_t)len;
    fflush(f);
    ok = ok && !ferror(f);
    fclose(f);
    return ok;
}

/* copies <fileName>.bin/.meta from slotA/ to backup/; 1 if anything was
 * copied (or already there); the backup's name without extension -> out */
static int backup_entry_files(const char *fileName, const char *reason, char *out, size_t outsz)
{
    static const char *const ext[2] = { ".bin", ".meta" };
    char src[600], dst[700], esc[CARD_FILENAME_MAX * 3 + 1];
    u8 *data[2];
    long len[2];
    u32 h = 2166136261u;
    int i, copied = 0;
    long k;

    for (i = 0; i < 2; i++) {
        entry_path(fileName, ext[i], src, sizeof(src));
        data[i] = read_whole(src, &len[i]);
        for (k = 0; k < len[i]; k++)
            h = (h ^ data[i][k]) * 16777619u;
    }
    escape_name(fileName, esc, sizeof(esc));
    for (i = 0; i < 2; i++) {
        if (len[i] < 0)
            continue;
        snprintf(dst, sizeof(dst), "%s%s.%s-%08x%s", backup_root(), esc, reason, (unsigned)h, ext[i]);
        if (out)
            snprintf(out, outsz, "%s.%s-%08x", esc, reason, (unsigned)h);
        {
            FILE *t = fopen(dst, "rb");
            if (t) {            /* this exact content is already backed up */
                fclose(t);
                copied = 1;
                continue;
            }
        }
        if (write_whole(dst, data[i], len[i])) {
            fprintf(stderr, "[card] backed up %s%s -> %s\n", esc, ext[i], dst);
            copied = 1;
        } else
            fprintf(stderr, "[card] WARNING: could not back up %s%s to %s\n", esc, ext[i], dst);
    }
    free(data[0]);
    free(data[1]);
    return copied;
}

void pc_card_backup_file(s32 chan, s32 fileNo, const char *reason)
{
    if (pc_session_active())   /* nothing is on disk */
        return;
    if (chan != 0 || fileNo < 0 || fileNo >= CARD_MAX_FILE || !s_dir[fileNo].used)
        return;
    backup_entry_files(s_dir[fileNo].fileName, reason ? reason : "bad", NULL, 0);
}

/* Called right before a create/rename writes <fileName>.bin/.meta, after the
 * SDK-style "name already exists for this game" check passed: anything still
 * on disk under that name is not a file of ours the card knows about (an
 * unparsable .meta, an orphan .bin, another game's file), so keep a copy. */
static void backup_stray(const char *fileName)
{
    if (pc_session_active())
        return;
    backup_entry_files(fileName, "stray", NULL, 0);
}

static void remove_entry_files(const char *fileName)
{
    char path[600];
    entry_path(fileName, ".bin", path, sizeof(path));
    remove(path);
    entry_path(fileName, ".meta", path, sizeof(path));
    remove(path);
}

static s32 delete_fileno(s32 fileNo)
{
    if (pc_session_active()) {
        free(s_dir[fileNo].mem);
        s_dir[fileNo].mem = NULL;
    } else
        remove_entry_files(s_dir[fileNo].fileName);
    memset(&s_dir[fileNo], 0, sizeof(s_dir[fileNo]));
    return CARD_RESULT_READY;
}

/* SDK __CARDUpdateIconOffsets */
static void fill_offsets(const CardEntry *e, CARDStat *st)
{
    u32 offset = e->iconAddr;
    int i, needTlut = 0;

    if (offset == 0xFFFFFFFFu) {
        st->offsetBanner = st->offsetBannerTlut = 0xFFFFFFFFu;
        for (i = 0; i < 8; i++)
            st->offsetIcon[i] = 0xFFFFFFFFu;
        st->offsetIconTlut = 0xFFFFFFFFu;
        st->offsetData = 0;
        return;
    }
    switch (e->bannerFormat & 3) {
    case 1: /* C8 */
        st->offsetBanner = offset;
        offset += 96 * 32;
        st->offsetBannerTlut = offset;
        offset += 2 * 256;
        break;
    case 2: /* RGB5A3 */
        st->offsetBanner = offset;
        offset += 2 * 96 * 32;
        st->offsetBannerTlut = 0xFFFFFFFFu;
        break;
    default:
        st->offsetBanner = st->offsetBannerTlut = 0xFFFFFFFFu;
        break;
    }
    for (i = 0; i < 8; i++) {
        switch ((e->iconFormat >> (2 * i)) & 3) {
        case 1:
            st->offsetIcon[i] = offset;
            offset += 32 * 32;
            needTlut = 1;
            break;
        case 2:
            st->offsetIcon[i] = offset;
            offset += 2 * 32 * 32;
            break;
        default:
            st->offsetIcon[i] = 0xFFFFFFFFu;
            break;
        }
    }
    if (needTlut) {
        st->offsetIconTlut = offset;
        offset += 2 * 256;
    } else
        st->offsetIconTlut = 0xFFFFFFFFu;
    st->offsetData = offset;
}

/* ------------------------------------------------------------------ */
/* game file checksum                                                  */

/* fn_800648C8 (main.dol .text, 0x4C bytes): CRC-16/CCITT-FALSE complement
 * (init 0xFFFF, poly 0x1021, result ~crc) over `size` bytes, using the
 * table at lbl_80080EA0 (identical to the standard 0x1021 table).
 * memcard.c stores it in the first u16 of every game file it writes and
 * checks it on load. */
u16 fn_800648C8(u32 size, void *data)
{
    static u16 table[256];
    const u8 *p = (const u8 *)data;
    u32 crc = 0xFFFF;
    int i, j;

    if (!table[1]) {
        for (i = 0; i < 256; i++) {
            u32 c = (u32)i << 8;
            for (j = 0; j < 8; j++)
                c = (c & 0x8000) ? (c << 1) ^ 0x1021 : c << 1;
            table[i] = (u16)c;
        }
    }
    while (size--)
        crc = (crc << 8) ^ table[((crc >> 8) ^ *p++) & 0xFF];
    return (u16)~crc;
}

/* ------------------------------------------------------------------ */
/* API                                                                 */

s32 CARDProbeEx(s32 chan, s32 *memSize, s32 *sectorSize)
{
    if (chan != 0 || s_frozen)
        return CARD_RESULT_NOCARD;
    if (memSize) *memSize = CARD_MEM_SIZE_MBIT;
    if (sectorSize) *sectorSize = CARD_SECTOR_SIZE;
    return CARD_RESULT_READY;
}

s32 CARDGetResultCode(s32 chan)
{
    if (chan != 0)
        return CARD_RESULT_NOCARD;
    return s_result[0];
}

s32 CARDMountAsync(s32 chan, void *workArea, CARDCallback detachCallback, CARDCallback attachCallback)
{
    (void)workArea;
    (void)detachCallback; /* fired only on card removal, which never happens */
    if (chan != 0 || s_frozen)
        return FINISH(chan, CARD_RESULT_NOCARD, NULL);
    load_directory();
    s_mounted = 1;
    return FINISH(chan, CARD_RESULT_READY, attachCallback);
}

s32 CARDUnmount(s32 chan)
{
    if (chan != 0)
        return CARD_RESULT_NOCARD;
    s_mounted = 0;
    s_result[0] = CARD_RESULT_NOCARD;
    return CARD_RESULT_READY;
}

s32 CARDCheckAsync(s32 chan, CARDCallback cb)
{
    return FINISH(chan, check_chan(chan, 1), cb);
}

s32 CARDFormatAsync(s32 chan, CARDCallback cb)
{
    s32 r = check_chan(chan, 1);
    int i;

    if (r == CARD_RESULT_READY) {
        for (i = 0; i < CARD_MAX_FILE; i++)
            if (s_dir[i].used)
                delete_fileno(i);
        s_xferred[0] += 5 * CARD_SECTOR_SIZE;
    }
    return FINISH(chan, r, cb);
}

s32 CARDFreeBlocks(s32 chan, s32 *byteNotUsed, s32 *filesNotUsed)
{
    s32 r = check_chan(chan, 1);
    int i, files = 0;

    if (r != CARD_RESULT_READY)
        return r;
    for (i = 0; i < CARD_MAX_FILE; i++)
        if (s_dir[i].used)
            files++;
    if (byteNotUsed) *byteNotUsed = (CARD_USER_BLOCKS - used_blocks()) * CARD_SECTOR_SIZE;
    if (filesNotUsed) *filesNotUsed = CARD_MAX_FILE - files;
    return CARD_RESULT_READY;
}

s32 CARDGetSerialNo(s32 chan, u64 *serialNo)
{
    s32 r = check_chan(chan, 1);
    if (r != CARD_RESULT_READY)
        return r;
    if (serialNo) *serialNo = CARD_SERIAL_NO;
    return CARD_RESULT_READY;
}

s32 CARDGetXferredBytes(s32 chan)
{
    return (chan == 0) ? s_xferred[0] : 0;
}

s32 CARDOpen(s32 chan, const char *fileName, CARDFileInfo *fileInfo)
{
    s32 r = check_chan(chan, 1);
    int no;

    if (fileInfo) fileInfo->chan = -1;
    if (r != CARD_RESULT_READY)
        return r;
    no = find_file(fileName);
    fprintf(stderr, "[card] CARDOpen(%d, %s) -> %d\n", (int)chan, fileName, no < 0 ? CARD_RESULT_NOFILE : 0);
    if (no < 0)
        return CARD_RESULT_NOFILE;
    fileInfo->chan = chan;
    fileInfo->fileNo = no;
    fileInfo->offset = 0;
    fileInfo->length = 0;
    fileInfo->iBlock = (u16)(5 + no);
    return CARD_RESULT_READY;
}

s32 CARDClose(CARDFileInfo *fileInfo)
{
    if (fileInfo) fileInfo->chan = -1;
    return CARD_RESULT_READY;
}

s32 CARDCreateAsync(s32 chan, const char *fileName, u32 size, CARDFileInfo *fileInfo, CARDCallback cb)
{
    const char *s_opname = fileName;
    s32 r = check_chan(chan, 1);
    const u8 *id;
    CardEntry *e;
    char path[600];
    FILE *f;
    int no, i, files = 0;

    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (strlen(fileName) > CARD_FILENAME_MAX)
        return FINISH(chan, CARD_RESULT_NAMETOOLONG, NULL);
    if (size == 0 || (size % CARD_SECTOR_SIZE) != 0)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    if (find_file(fileName) >= 0)
        return FINISH(chan, CARD_RESULT_EXIST, NULL);
    for (no = -1, i = 0; i < CARD_MAX_FILE; i++) {
        if (s_dir[i].used) files++;
        else if (no < 0) no = i;
    }
    if (no < 0)
        return FINISH(chan, CARD_RESULT_NOENT, NULL);
    if ((int)(size / CARD_SECTOR_SIZE) > CARD_USER_BLOCKS - used_blocks())
        return FINISH(chan, CARD_RESULT_INSSPACE, NULL);

    backup_stray(fileName);
    id = (const u8 *)DVDGetCurrentDiskID();
    e = &s_dir[no];
    memset(e, 0, sizeof(*e));
    strncpy(e->fileName, fileName, CARD_FILENAME_MAX);
    e->length = size;
    e->time = card_time_now();
    memcpy(e->gameName, id, 4);
    memcpy(e->company, id + 4, 2);
    e->iconAddr = 0xFFFFFFFFu;
    e->commentAddr = 0xFFFFFFFFu;
    e->permission = CARD_ATTR_PUBLIC;   /* as the SDK's CARDCreate */

    if (pc_session_active()) {   /* in-memory file, zero-filled like a fresh one */
        e->mem = (u8 *)calloc(1, size);
        if (!e->mem) {
            memset(e, 0, sizeof(*e));
            return FINISH(chan, CARD_RESULT_IOERROR, cb);
        }
        e->used = 1;
        if (fileInfo) {
            fileInfo->chan = chan;
            fileInfo->fileNo = no;
            fileInfo->offset = 0;
            fileInfo->length = 0;
            fileInfo->iBlock = (u16)(5 + no);
        }
        return FINISH(chan, CARD_RESULT_READY, cb);
    }

    /* zero-filled data file of the full size */
    entry_path(e->fileName, ".bin", path, sizeof(path));
    f = fopen(path, "wb");
    if (!f) {
        memset(e, 0, sizeof(*e));
        return FINISH(chan, CARD_RESULT_IOERROR, cb);
    }
    {
        static const u8 zero[CARD_SECTOR_SIZE];
        u32 left;
        for (left = size; left; left -= CARD_SECTOR_SIZE)
            fwrite(zero, 1, CARD_SECTOR_SIZE, f);
    }
    fflush(f);
    fclose(f);
    e->used = 1;
    if (save_meta(no) != CARD_RESULT_READY) {
        delete_fileno(no);
        return FINISH(chan, CARD_RESULT_IOERROR, cb);
    }
    if (fileInfo) {
        fileInfo->chan = chan;
        fileInfo->fileNo = no;
        fileInfo->offset = 0;
        fileInfo->length = 0;
        fileInfo->iBlock = (u16)(5 + no);
    }
    return FINISH(chan, CARD_RESULT_READY, cb);
}

s32 CARDDeleteAsync(s32 chan, const char *fileName, CARDCallback cb)
{
    const char *s_opname = fileName;
    s32 r = check_chan(chan, 1);
    int no;

    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    no = find_file(fileName);
    if (no < 0)
        return FINISH(chan, CARD_RESULT_NOFILE, NULL);
    return FINISH(chan, delete_fileno(no), cb);
}

s32 CARDFastDeleteAsync(s32 chan, s32 fileNo, CARDCallback cb)
{
    s32 r = check_chan(chan, 1);

    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    if (!s_dir[fileNo].used)
        return FINISH(chan, CARD_RESULT_NOFILE, NULL);
    if (!is_this_game(&s_dir[fileNo]))
        return FINISH(chan, CARD_RESULT_NOPERM, NULL);
    return FINISH(chan, delete_fileno(fileNo), cb);
}

s32 CARDRenameAsync(s32 chan, const char *oldName, const char *newName, CARDCallback cb)
{
    const char *s_opname = newName;
    s32 r = check_chan(chan, 1);
    char oldPath[600], newPath[600];
    CardEntry *e;
    int no;

    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (!*oldName || !*newName)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    if (strlen(oldName) > CARD_FILENAME_MAX || strlen(newName) > CARD_FILENAME_MAX)
        return FINISH(chan, CARD_RESULT_NAMETOOLONG, NULL);
    no = find_file(oldName);
    if (no < 0)
        return FINISH(chan, CARD_RESULT_NOFILE, NULL);
    if (find_file(newName) >= 0)
        return FINISH(chan, CARD_RESULT_EXIST, NULL);
    e = &s_dir[no];
    if (pc_session_active()) {   /* in memory: only the name changes */
        memset(e->fileName, 0, sizeof(e->fileName));
        strncpy(e->fileName, newName, CARD_FILENAME_MAX);
        e->time = card_time_now();
        return FINISH(chan, CARD_RESULT_READY, cb);
    }
    backup_stray(newName);

    entry_path(e->fileName, ".bin", oldPath, sizeof(oldPath));
    entry_path(newName, ".bin", newPath, sizeof(newPath));
    remove(newPath);
    if (rename(oldPath, newPath) != 0)
        return FINISH(chan, CARD_RESULT_IOERROR, cb);
    entry_path(e->fileName, ".meta", oldPath, sizeof(oldPath));
    remove(oldPath);
    memset(e->fileName, 0, sizeof(e->fileName));
    strncpy(e->fileName, newName, CARD_FILENAME_MAX);
    e->time = card_time_now();
    return FINISH(chan, save_meta(no), cb);
}

s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat *stat)
{
    s32 r = check_chan(chan, 1);
    const CardEntry *e;

    if (r != CARD_RESULT_READY)
        return r;
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    e = &s_dir[fileNo];
    if (!e->used)
        return CARD_RESULT_NOFILE;
    if (!is_this_game(e))
        return CARD_RESULT_NOPERM;
    memset(stat, 0, sizeof(*stat));
    memcpy(stat->fileName, e->fileName, CARD_FILENAME_MAX);
    stat->length = e->length;
    stat->time = e->time;
    memcpy(stat->gameName, e->gameName, 4);
    memcpy(stat->company, e->company, 2);
    stat->bannerFormat = e->bannerFormat;
    stat->iconAddr = e->iconAddr;
    stat->iconFormat = e->iconFormat;
    stat->iconSpeed = e->iconSpeed;
    stat->commentAddr = e->commentAddr;
    fill_offsets(e, stat);
    return CARD_RESULT_READY;
}

s32 CARDSetStatusAsync(s32 chan, s32 fileNo, const CARDStat *stat, CARDCallback cb)
{
    s32 r = check_chan(chan, 1);
    CardEntry *e;

    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (fileNo < 0 || fileNo >= CARD_MAX_FILE)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    if (stat->iconAddr != 0xFFFFFFFFu && stat->iconAddr >= 512)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    if (stat->commentAddr != 0xFFFFFFFFu &&
        CARD_READ_SIZE - 64 < stat->commentAddr % CARD_READ_SIZE)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    e = &s_dir[fileNo];
    if (!e->used)
        return FINISH(chan, CARD_RESULT_NOFILE, NULL);
    if (!is_this_game(e))
        return FINISH(chan, CARD_RESULT_NOPERM, NULL);
    e->bannerFormat = stat->bannerFormat;
    e->iconAddr = stat->iconAddr;
    e->iconFormat = stat->iconFormat;
    e->iconSpeed = stat->iconSpeed;
    e->commentAddr = stat->commentAddr;
    e->time = card_time_now();
    return FINISH(chan, save_meta(fileNo), cb);
}

/* SDK __CARDSeek checks shared by read and write */
static s32 seek_check(CARDFileInfo *fi, s32 length, s32 offset, CardEntry **out)
{
    s32 r;
    CardEntry *e;

    if (!fi)
        return CARD_RESULT_FATAL_ERROR;
    r = check_chan(fi->chan, 1);
    if (r != CARD_RESULT_READY)
        return r;
    if (fi->fileNo < 0 || fi->fileNo >= CARD_MAX_FILE)
        return CARD_RESULT_FATAL_ERROR;
    e = &s_dir[fi->fileNo];
    if (!e->used)
        return CARD_RESULT_NOFILE;
    if (offset < 0 || length < 0 || (u32)offset + (u32)length > e->length)
        return CARD_RESULT_LIMIT;
    fi->offset = offset;
    fi->length = length;
    *out = e;
    return CARD_RESULT_READY;
}

s32 CARDReadAsync(CARDFileInfo *fileInfo, void *buf, s32 length, s32 offset, CARDCallback cb)
{
    CardEntry *e;
    char path[600];
    FILE *f;
    size_t got;
    s32 chan = fileInfo ? fileInfo->chan : -1;
    s32 r;

    if ((offset % CARD_READ_SIZE) != 0 || (length % CARD_READ_SIZE) != 0)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    r = seek_check(fileInfo, length, offset, &e);
    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (pc_session_active()) {
        memcpy(buf, e->mem + offset, (size_t)length);
        fileInfo->offset += length;
        s_xferred[0] += length;
        return FINISH(chan, CARD_RESULT_READY, cb);
    }
    entry_path(e->fileName, ".bin", path, sizeof(path));
    f = fopen(path, "rb");
    if (f) {
        fseek(f, offset, SEEK_SET);
        got = fread(buf, 1, (size_t)length, f);
        fclose(f);
    } else {
        /* .meta without its .bin: read as erased sectors, so the game's
         * checksum check reports the file as corrupt (decomp memcard.c
         * would OSPanic on CARD_RESULT_IOERROR at that point) */
        fprintf(stderr, "[card] %s missing, reading as blank\n", path);
        got = 0;
    }
    if (got < (size_t)length)  /* short file: pad like an erased card */
        memset((u8 *)buf + got, 0, (size_t)length - got);
    fileInfo->offset += length;
    s_xferred[0] += length;
    return FINISH(chan, CARD_RESULT_READY, cb);
}

s32 CARDWriteAsync(CARDFileInfo *fileInfo, const void *buf, s32 length, s32 offset, CARDCallback cb)
{
    CardEntry *e;
    char path[600];
    FILE *f;
    s32 chan = fileInfo ? fileInfo->chan : -1;
    s32 r;
    int ok;

    if ((offset % CARD_SECTOR_SIZE) != 0 || (length % CARD_SECTOR_SIZE) != 0)
        return FINISH(chan, CARD_RESULT_FATAL_ERROR, NULL);
    r = seek_check(fileInfo, length, offset, &e);
    if (r != CARD_RESULT_READY)
        return FINISH(chan, r, NULL);
    if (pc_session_active()) {
        memcpy(e->mem + offset, buf, (size_t)length);
        fileInfo->offset += length;
        s_xferred[0] += length + CARD_SECTOR_SIZE;
        return FINISH(chan, CARD_RESULT_READY, cb);
    }
    entry_path(e->fileName, ".bin", path, sizeof(path));
    if (offset == 0 && (u32)length == e->length)
        f = fopen(path, "wb");
    else {
        f = fopen(path, "r+b");
        if (!f) {
            /* recreate a missing data file at full size */
            f = fopen(path, "wb");
            if (f) {
                static const u8 zero[CARD_SECTOR_SIZE];
                u32 left;
                for (left = e->length; left; left -= CARD_SECTOR_SIZE)
                    fwrite(zero, 1, CARD_SECTOR_SIZE, f);
            }
        }
    }
    if (!f)
        return FINISH(chan, CARD_RESULT_IOERROR, cb);
    fseek(f, offset, SEEK_SET);
    ok = fwrite(buf, 1, (size_t)length, f) == (size_t)length;
    fflush(f);
    ok = ok && !ferror(f);
    fclose(f);
    if (!ok)
        return FINISH(chan, CARD_RESULT_IOERROR, cb);
    fileInfo->offset += length;
    s_xferred[0] += length + CARD_SECTOR_SIZE; /* data + directory update */
    return FINISH(chan, CARD_RESULT_READY, cb);
}

/* ------------------------------------------------------------------ */
/* save import/export (pc_gci.c)                                       */
/*
 * These work on the files on disk, not on s_dir: the game re-reads the
 * directory on every mount, and an import is only offered while no memory
 * card job runs (the game is restarted afterwards to read the new save).
 */
#include "pc_card.h"

const char *pc_card_slot_dir(void) { return pc_session_active() ? "" : card_root(); }

/* After an in-game import: the card reads as removed until the restart, so
 * nothing the running game still holds in memory can be saved over the
 * imported files. */
void pc_card_freeze(void)
{
    s_frozen = 1;
    s_mounted = 0;
    s_result[0] = CARD_RESULT_NOCARD;
    printf("[card] card closed until the restart (a save was imported)\n");
}

const char *pc_card_base_dir(void)
{
    static char base[520];
    const char *slot = pc_session_active() ? "" : card_root();   /* session: no disk card */
    size_t l = strlen(slot);
    if (!base[0] && l >= 6 && l - 6 < sizeof(base)) {
        memcpy(base, slot, l - 6);
        base[l - 6] = 0;
    }
    return base;
}

static void entry_to_file(const CardEntry *e, int fileNo, PcCardFile *f)
{
    memset(f, 0, sizeof(*f));
    memcpy(f->fileName, e->fileName, sizeof(f->fileName));
    memcpy(f->gameName, e->gameName, 4);
    memcpy(f->company, e->company, 2);
    f->bannerFormat = e->bannerFormat;
    f->time = e->time;
    f->iconAddr = e->iconAddr;
    f->iconFormat = e->iconFormat;
    f->iconSpeed = e->iconSpeed;
    f->permission = e->permission;
    f->copyTimes = e->copyTimes;
    f->firstBlock = e->firstBlock;
    f->length = e->length;
    f->commentAddr = e->commentAddr;
    f->fileNo = fileNo;
}

int pc_card_list(PcCardFile *out, int max)
{
    DIR *d = pc_session_active() ? NULL : opendir(card_root());   /* session: no disk card */
    struct dirent *de;
    int n = 0, i;

    if (!d)
        return 0;
    while ((de = readdir(d)) != NULL && n < max) {
        size_t l = strlen(de->d_name);
        char path[600];
        CardEntry e;
        int no, dup = 0;
        if (l <= 5 || strcmp(de->d_name + l - 5, ".meta") != 0)
            continue;
        snprintf(path, sizeof(path), "%s%s", card_root(), de->d_name);
        if (!parse_meta(path, &e, &no))
            continue;
        for (i = 0; i < n; i++)
            if (!strcmp(out[i].fileName, e.fileName))
                dup = 1;
        if (!dup)
            entry_to_file(&e, no, &out[n++]);
    }
    closedir(d);
    /* directory order, as the card lists them */
    for (i = 1; i < n; i++) {
        PcCardFile t = out[i];
        int j = i;
        while (j > 0 && (unsigned)out[j - 1].fileNo > (unsigned)t.fileNo) {
            out[j] = out[j - 1];
            j--;
        }
        out[j] = t;
    }
    return n;
}

u8 *pc_card_read(const PcCardFile *f)
{
    char path[600];
    long len;
    u8 *buf, *data;

    if (pc_session_active())
        return NULL;
    entry_path(f->fileName, ".bin", path, sizeof(path));
    data = read_whole(path, &len);
    if (!data || len < 0)
        return NULL;
    if ((u32)len == f->length)
        return data;
    /* short or long .bin: what the game would read (CARDReadAsync pads
     * with zeros) */
    buf = (u8 *)calloc(1, f->length ? f->length : 1);
    if (buf)
        memcpy(buf, data, (size_t)((u32)len < f->length ? (u32)len : f->length));
    free(data);
    return buf;
}

int pc_card_store(const PcCardFile *f, const u8 *data, char *backup, size_t backupsz)
{
    PcCardFile *files;
    CardEntry e;
    char path[600], tmp[620];
    int n, i, no = -1, blocks = 0, replaced = -1;
    u8 used[CARD_MAX_FILE];

    if (pc_session_active())   /* the session card never touches disk */
        return PC_CARD_E_IO;
    if (backup && backupsz)
        backup[0] = 0;
    if (!f->fileName[0] || f->length == 0 || (f->length % CARD_SECTOR_SIZE) != 0)
        return PC_CARD_E_IO;
    files = (PcCardFile *)malloc(sizeof(PcCardFile) * CARD_MAX_FILE);
    if (!files)
        return PC_CARD_E_IO;
    n = pc_card_list(files, CARD_MAX_FILE);
    memset(used, 0, sizeof(used));
    for (i = 0; i < n; i++) {
        if (!strcmp(files[i].fileName, f->fileName)) {
            replaced = i;
            continue;
        }
        blocks += (int)(files[i].length / CARD_SECTOR_SIZE);
        if (files[i].fileNo >= 0 && files[i].fileNo < CARD_MAX_FILE)
            used[files[i].fileNo] = 1;
    }
    if (replaced >= 0 && files[replaced].fileNo >= 0 && files[replaced].fileNo < CARD_MAX_FILE &&
        !used[files[replaced].fileNo])
        no = files[replaced].fileNo;
    for (i = 0; no < 0 && i < CARD_MAX_FILE; i++)
        if (!used[i])
            no = i;
    free(files);
    if (no < 0)
        return PC_CARD_E_NOENT;
    if (blocks + (int)(f->length / CARD_SECTOR_SIZE) > CARD_USER_BLOCKS)
        return PC_CARD_E_FULL;

    /* whatever is on disk under this name (a save, or a stray .bin/.meta) */
    entry_path(f->fileName, ".bin", path, sizeof(path));
    {
        FILE *t = fopen(path, "rb");
        char mpath[600];
        FILE *m;
        entry_path(f->fileName, ".meta", mpath, sizeof(mpath));
        m = fopen(mpath, "rb");
        if (t || m) {
            if (t) fclose(t);
            if (m) fclose(m);
            if (!backup_entry_files(f->fileName, "replaced", backup, backupsz))
                return PC_CARD_E_IO;   /* never replace a save without its copy */
        }
    }

    /* data first, via a temporary file, so a failed write leaves the old one */
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (!write_whole(tmp, data, (long)f->length)) {
        remove(tmp);
        return PC_CARD_E_IO;
    }
    remove(path);
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return PC_CARD_E_IO;
    }
    memset(&e, 0, sizeof(e));
    memcpy(e.fileName, f->fileName, sizeof(e.fileName));
    e.fileName[CARD_FILENAME_MAX] = 0;
    e.length = f->length;
    e.time = f->time;
    memcpy(e.gameName, f->gameName, 4);
    memcpy(e.company, f->company, 2);
    e.bannerFormat = f->bannerFormat;
    e.iconAddr = f->iconAddr;
    e.iconFormat = f->iconFormat;
    e.iconSpeed = f->iconSpeed;
    e.commentAddr = f->commentAddr;
    e.permission = f->permission;
    e.copyTimes = f->copyTimes;
    e.firstBlock = f->firstBlock;
    e.used = 1;
    if (write_meta(&e, no) != CARD_RESULT_READY)
        return PC_CARD_E_IO;
    printf("[card] stored %s (%u bytes, slot %d)%s%s\n", f->fileName, (unsigned)f->length, no,
            backup && backup[0] ? ", old one backed up as " : "", backup && backup[0] ? backup : "");
    return 0;
}
