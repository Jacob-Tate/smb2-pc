/* Extracted game data: --extract-iso, --verify-gamedata, runtime check.
 * Interface: docs/installer.md; format: include/gamedata.h. */
#include "gamedata.h"
#include "disc_icon.h"
#include "iso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PSEP '\\'
#else
#include <unistd.h>
#define PSEP '/'
#endif

#define MANIFEST_MAGIC "SMB2PC-GAMEDATA 1"
#define PARTIAL_MARKER ".smb2-extract-partial"
#define CHUNK (4u << 20)

/* Files the port cannot run without (DVDOpen'd by the boot/menus/game
 * modes, first ones read at boot; docs/installer.md "What the port reads").  A disc whose FST
 * lacks one is not a usable SMB2 image even if the ID matches. */
static const char* const s_required[] = {
    "/memcard/sys.bin",
    "/font/fnt_asc_24x24.bin",
    "/init/common.gma",
    "/init/common.tpl",
    "/init/common.lz",
    "/init/common_p.lz",
    "/bmp/bmp_com.lz",
    "/ape/skl.arc",
    "/stgname/usa.str",
    "/snd/mkb/allse.pool",
    "/snd/mkb/allse.proj",
    "/snd/mkb/allse.sdir",
    "/snd/mkb/allse.samp",
    "/mkb2.sel_ngc.rel",
    "/mkb2.main_game.rel",
    NULL,
};

/* ------------------------------------------------------------------ util */
uint32_t gd_crc32(uint32_t crc, const void* data, size_t n)
{
    static uint32_t table[256];
    static int init;
    const uint8_t* p = (const uint8_t*)data;
    if (!init) {
        uint32_t i, j, c;
        for (i = 0; i < 256; i++) {
            for (c = i, j = 0; j < 8; j++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = 1;
    }
    crc = ~crc;
    while (n--) crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static void setmsg(char* msg, size_t n, const char* fmt, ...)
{
    va_list ap;
    if (!msg || !n) return;
    va_start(ap, fmt);
    vsnprintf(msg, n, fmt, ap);
    va_end(ap);
}

static uint32_t be32(const void* p)
{
    const uint8_t* b = (const uint8_t*)p;
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
}

static void pjoin(char* out, size_t n, const char* a, const char* b)
{
    size_t l = strlen(a);
    if (l && (a[l - 1] == '/' || a[l - 1] == '\\'))
        snprintf(out, n, "%s%s", a, b);
    else
        snprintf(out, n, "%s%c%s", a, PSEP, b);
}

/* "a/b/c" (manifest form) -> host separators */
static void host_rel(char* s)
{
#ifdef _WIN32
    for (; *s; s++) if (*s == '/') *s = '\\';
#else
    (void)s;
#endif
}

static int mkdir1(const char* p)
{
#ifdef _WIN32
    return _mkdir(p);
#else
    return mkdir(p, 0755);
#endif
}

static int is_dir(const char* p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int exists(const char* p)
{
    struct stat st;
    return stat(p, &st) == 0;
}

static bool mkdir_p(const char* path)
{
    char buf[1200];
    size_t i, l;
    snprintf(buf, sizeof buf, "%s", path);
    l = strlen(buf);
    while (l > 1 && (buf[l - 1] == '/' || buf[l - 1] == '\\')) buf[--l] = 0;
    i = 1;
    if (l > 2 && (buf[0] == '/' || buf[0] == '\\') && (buf[1] == '/' || buf[1] == '\\')) {
        /* UNC: start after "\\server\share" */
        int seps = 0;
        for (i = 2; i < l; i++)
            if ((buf[i] == '/' || buf[i] == '\\') && ++seps == 2) break;
        i++;    /* never stat/mkdir the share root itself (msvcrt stat fails on it) */
    }
    for (; i <= l; i++) {
        if (buf[i] == '/' || buf[i] == '\\' || buf[i] == 0) {
            char c = buf[i];
            buf[i] = 0;
            /* skip the "C:" root */
            if (!(i == 2 && buf[1] == ':') && !is_dir(buf)) {
                if (mkdir1(buf) != 0 && !is_dir(buf)) return false;
            }
            buf[i] = c;
        }
    }
    return is_dir(path);
}

/* Recursive delete.  Only ever called on folders this code created and
 * identified (a .partial with our marker, or a gamedata with manifest). */
static bool rm_tree(const char* path)
{
    DIR* d = opendir(path);
    struct dirent* e;
    bool ok = true;
    if (!d) return remove(path) == 0;
    while ((e = readdir(d)) != NULL) {
        char sub[1200];
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        pjoin(sub, sizeof sub, path, e->d_name);
        if (is_dir(sub)) ok = rm_tree(sub) && ok;
        else {
#ifdef _WIN32
            SetFileAttributesA(sub, FILE_ATTRIBUTE_NORMAL);
#endif
            ok = (remove(sub) == 0) && ok;
        }
    }
    closedir(d);
#ifdef _WIN32
    ok = (_rmdir(path) == 0) && ok;
#else
    ok = (rmdir(path) == 0) && ok;
#endif
    return ok;
}

static bool dir_is_empty(const char* path)
{
    DIR* d = opendir(path);
    struct dirent* e;
    bool empty = true;
    if (!d) return true;
    while ((e = readdir(d)) != NULL)
        if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) { empty = false; break; }
    closedir(d);
    return empty;
}

static bool rename_dir(const char* from, const char* to)
{
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(from, to) == 0;
#endif
}

/* test hook: SMB2_TEST_WRITE_LIMIT=<bytes> makes writes fail with ENOSPC
 * after that many bytes (disk-full simulation) */
static uint64_t s_written, s_write_limit = (uint64_t)-1;

static int write_all(FILE* f, const void* p, size_t n)
{
    if (s_written + n > s_write_limit) { errno = ENOSPC; return -1; }
    if (fwrite(p, 1, n, f) != n) return -1;
    s_written += n;
    return 0;
}

static int werr_code(char* msg, size_t msgsz, const char* path)
{
    int e = errno;
    if (e == ENOSPC)
        setmsg(msg, msgsz, "disk full while writing %s", path);
    else
        setmsg(msg, msgsz, "write error on %s: %s", path, strerror(e));
    return GD_ERR_WRITE;
}

/* ------------------------------------------------------------- manifest */
typedef struct {
    char* path;         /* manifest form, '/' separated, relative to the root */
    uint64_t size;
    uint32_t crc;
} GdFile;

typedef struct {
    char disc_id[8];
    int rev;
    char game_name[80];
    uint32_t fst_crc;
    GdFile* files;
    int count, cap;
    uint64_t total;
    bool complete;
} GdManifest;

static void man_free(GdManifest* m)
{
    int i;
    for (i = 0; i < m->count; i++) free(m->files[i].path);
    free(m->files);
    memset(m, 0, sizeof *m);
}

static bool man_add(GdManifest* m, const char* path, uint64_t size, uint32_t crc)
{
    if (m->count == m->cap) {
        int nc = m->cap ? m->cap * 2 : 1024;
        GdFile* nf = (GdFile*)realloc(m->files, (size_t)nc * sizeof *nf);
        if (!nf) return false;
        m->files = nf;
        m->cap = nc;
    }
    m->files[m->count].path = strdup(path);
    m->files[m->count].size = size;
    m->files[m->count].crc = crc;
    if (!m->files[m->count].path) return false;
    m->count++;
    m->total += size;
    return true;
}

static int man_write(const GdManifest* m, const char* dir, char* msg, size_t msgsz)
{
    char path[1200], tmp[1200];
    FILE* f;
    int i, r = 0;
    pjoin(path, sizeof path, dir, "manifest.txt");
    pjoin(tmp, sizeof tmp, dir, "manifest.tmp");
    f = fopen(tmp, "wb");
    if (!f) return werr_code(msg, msgsz, tmp);
    r |= fprintf(f, "%s\n", MANIFEST_MAGIC) < 0;
    r |= fprintf(f, "disc_id=%s\nrev=%d\ngame_name=%s\n", m->disc_id, m->rev, m->game_name) < 0;
    r |= fprintf(f, "fst_crc32=%08x\nfiles=%d\ntotal_bytes=%llu\n", (unsigned)m->fst_crc,
                 m->count, (unsigned long long)m->total) < 0;
    for (i = 0; i < m->count; i++)
        if (!strcmp(m->files[i].path, "icon.ico")) {
            r |= fprintf(f, "icon=icon.ico\nicon_png=icon_256.png\n") < 0;
            break;
        }
    for (i = 0; i < m->count && !r; i++)
        r |= fprintf(f, "file=%08x %llu %s\n", (unsigned)m->files[i].crc,
                     (unsigned long long)m->files[i].size, m->files[i].path) < 0;
    r |= fprintf(f, "end\n") < 0;
    if (s_written + 4096 > s_write_limit) r = 1, errno = ENOSPC;
    if (fclose(f) != 0) r = 1;
    if (r) { int c = werr_code(msg, msgsz, tmp); remove(tmp); return c; }
    remove(path);
    if (rename(tmp, path) != 0) return werr_code(msg, msgsz, path);
    return GD_OK;
}

static bool man_read(GdManifest* m, const char* dir, char* msg, size_t msgsz)
{
    char path[1200], line[2048];
    FILE* f;
    int declared = -1;
    memset(m, 0, sizeof *m);
    pjoin(path, sizeof path, dir, "manifest.txt");
    f = fopen(path, "rb");
    if (!f) { setmsg(msg, msgsz, "no manifest.txt in %s", dir); return false; }
    if (!fgets(line, sizeof line, f) || strncmp(line, MANIFEST_MAGIC, strlen(MANIFEST_MAGIC)) != 0) {
        fclose(f);
        setmsg(msg, msgsz, "%s is not a game data manifest", path);
        return false;
    }
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "file=", 5)) {
            unsigned crc;
            unsigned long long size;
            int n = 0;
            if (sscanf(line + 5, "%8x %llu %n", &crc, &size, &n) < 2 || n <= 0 ||
                !line[5 + n] || !man_add(m, line + 5 + n, size, crc)) {
                fclose(f);
                setmsg(msg, msgsz, "malformed manifest line: %.200s", line);
                man_free(m);
                return false;
            }
        } else if (!strncmp(line, "disc_id=", 8)) snprintf(m->disc_id, sizeof m->disc_id, "%s", line + 8);
        else if (!strncmp(line, "rev=", 4)) m->rev = atoi(line + 4);
        else if (!strncmp(line, "game_name=", 10)) snprintf(m->game_name, sizeof m->game_name, "%s", line + 10);
        else if (!strncmp(line, "fst_crc32=", 10)) m->fst_crc = (uint32_t)strtoul(line + 10, NULL, 16);
        else if (!strncmp(line, "files=", 6)) declared = atoi(line + 6);
        else if (!strcmp(line, "end")) m->complete = true;
    }
    fclose(f);
    if (!m->complete || declared != m->count) {
        setmsg(msg, msgsz, "manifest.txt in %s is incomplete", dir);
        man_free(m);
        return false;
    }
    if (strcmp(m->disc_id, "GM2E8P") != 0) {
        setmsg(msg, msgsz, "game data in %s is %s, not GM2E8P", dir, m->disc_id);
        man_free(m);
        return false;
    }
    return true;
}

/* -------------------------------------------------------------- extract */
typedef struct {
    GdProgressFn cb;
    void* user;
    uint64_t done, total;
} Prog;

static void prog_add(Prog* p, uint64_t n)
{
    p->done += n;
    if (p->cb) p->cb(p->done, p->total, p->user);
}

static int64_t file_size64(FILE* f)
{
#ifdef _WIN32
    if (_fseeki64(f, 0, SEEK_END) != 0) return -1;
    return _ftelli64(f);
#else
    if (fseeko(f, 0, SEEK_END) != 0) return -1;
    return (int64_t)ftello(f);
#endif
}

/* Identify the image; fills hdr.  Returns GD_OK or an error code + msg. */
static int check_image(const char* iso, uint8_t hdr[0x440], int64_t* out_size,
                       char* msg, size_t msgsz)
{
    FILE* f = fopen(iso, "rb");
    uint8_t nk[4];
    int64_t size;
    uint32_t fst_off, fst_size;
    if (!f) {
        setmsg(msg, msgsz, "cannot open %s: %s", iso, strerror(errno));
        return GD_ERR_IMAGE;
    }
    size = file_size64(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0x440 || fread(hdr, 1, 0x440, f) != 0x440) {
        fclose(f);
        setmsg(msg, msgsz, "%s is too small to be a GameCube disc image", iso);
        return size < 0x440 && size >= 0 ? GD_ERR_NOT_SMB2 : GD_ERR_IMAGE;
    }
    nk[0] = 0;
    if (size >= 0x204) { fseek(f, 0x200, SEEK_SET); if (fread(nk, 1, 4, f) != 4) nk[0] = 0; }
    fclose(f);
    *out_size = size;

    if (!memcmp(hdr, "RVZ\x01", 4) || !memcmp(hdr, "WIA\x01", 4)) {
        setmsg(msg, msgsz, "%s is a compressed RVZ/WIA image; convert it to ISO first "
               "(Dolphin: right-click the game > Convert File > ISO)", iso);
        return GD_ERR_IMAGE;
    }
    if (!memcmp(hdr, "CISO", 4) || be32(hdr) == 0x01C00BB1u || !memcmp(hdr, "WBFS", 4)) {
        setmsg(msg, msgsz, "%s is a compressed CISO/GCZ/WBFS image; convert it to ISO first "
               "(Dolphin: right-click the game > Convert File > ISO)", iso);
        return GD_ERR_IMAGE;
    }
    if (be32(hdr + 0x1C) != 0xC2339F3Du) {
        if (be32(hdr + 0x18) == 0x5D1C9EA3u)
            setmsg(msg, msgsz, "%s is a Wii disc (%.6s), not Super Monkey Ball 2 (GM2E8P)", iso, hdr);
        else
            setmsg(msg, msgsz, "%s is not a GameCube disc image", iso);
        return GD_ERR_NOT_SMB2;
    }
    if (memcmp(hdr, "GM2E8P", 6) != 0) {
        char name[65];
        snprintf(name, sizeof name, "%.64s", (const char*)hdr + 0x20);
        if (!memcmp(hdr, "GM2", 3))
            setmsg(msg, msgsz, "this is Super Monkey Ball 2 for region '%c' (%.6s); "
                   "only the North American release GM2E8P is supported", hdr[3], hdr);
        else
            setmsg(msg, msgsz, "this disc is '%s' (%.6s), not Super Monkey Ball 2 (GM2E8P)",
                   name, hdr);
        return GD_ERR_NOT_SMB2;
    }
    if (hdr[6] != 0 || hdr[7] != 0) {
        setmsg(msg, msgsz, "unsupported GM2E8P disc %d revision %d (only disc 0 rev 0 is known)",
               hdr[6], hdr[7]);
        return GD_ERR_NOT_SMB2;
    }
    if (!memcmp(nk, "NKIT", 4)) {
        setmsg(msg, msgsz, "%s is an NKit image; restore it to a full ISO first "
               "(NKit Recover / Dolphin Convert File > ISO)", iso);
        return GD_ERR_IMAGE;
    }
    fst_off = be32(hdr + 0x424);
    fst_size = be32(hdr + 0x428);
    if (fst_size < 12 || fst_size > 0x1000000 || (int64_t)fst_off + fst_size > size) {
        setmsg(msg, msgsz, "%s is truncated or corrupt (FST outside the image)", iso);
        return GD_ERR_IMAGE;
    }
    return GD_OK;
}

static uint64_t free_space(const char* dir)
{
#ifdef _WIN32
    ULARGE_INTEGER avail;
    char buf[1200];
    size_t l;
    snprintf(buf, sizeof buf, "%s", dir);
    /* walk up to an existing folder */
    while (!is_dir(buf)) {
        char* s1 = strrchr(buf, '\\');
        char* s2 = strrchr(buf, '/');
        char* s = s1 > s2 ? s1 : s2;
        if (!s) return (uint64_t)-1;
        *s = 0;
        if (!buf[0]) return (uint64_t)-1;
    }
    l = strlen(buf);
    if (l < sizeof buf - 2 && buf[l - 1] != '\\' && buf[l - 1] != '/') { buf[l] = '\\'; buf[l + 1] = 0; }
    if (GetDiskFreeSpaceExA(buf, &avail, NULL, NULL)) return avail.QuadPart;
#else
    (void)dir;
#endif
    return (uint64_t)-1;
}

/* extract one region/entry into path, crc'ing as it goes */
static int extract_to(ISOArchive* arc, int index, uint32_t raw_off, uint32_t size,
                      const char* path, uint8_t* buf, uint32_t* out_crc, Prog* p,
                      char* msg, size_t msgsz)
{
    FILE* f = fopen(path, "wb");
    uint32_t crc = 0, off = 0;
    if (!f) return werr_code(msg, msgsz, path);
    while (off < size) {
        uint32_t n = size - off > CHUNK ? CHUNK : size - off;
        bool ok = index >= 0 ? iso_file_read_at(arc, index, off, n, buf) == 0
                             : iso_read_raw(arc, raw_off + off, n, buf);
        if (!ok) {
            fclose(f);
            setmsg(msg, msgsz, "read error in the disc image at %s (image truncated or unreadable)",
                   index >= 0 ? iso_entry_name(arc, index) : "system area");
            return GD_ERR_IMAGE;
        }
        crc = gd_crc32(crc, buf, n);
        if (write_all(f, buf, n) != 0) { int c = werr_code(msg, msgsz, path); fclose(f); return c; }
        off += n;
        prog_add(p, n);
    }
    if (s_written > s_write_limit) errno = ENOSPC;
    if (fclose(f) != 0) return werr_code(msg, msgsz, path);
    *out_crc = crc;
    return GD_OK;
}

static int write_blob(const char* path, const void* data, size_t n, char* msg, size_t msgsz)
{
    FILE* f = fopen(path, "wb");
    if (!f) return werr_code(msg, msgsz, path);
    if (write_all(f, data, n) != 0) { int c = werr_code(msg, msgsz, path); fclose(f); return c; }
    if (fclose(f) != 0) return werr_code(msg, msgsz, path);
    return GD_OK;
}

static int verify_files(const char* dir, const GdManifest* m, Prog* p, char* msg, size_t msgsz);

static int gamedata_extract_(const char* iso, const char* dir, GdProgressFn cb, void* user,
                             char* msg, size_t msgsz);
int gamedata_extract(const char* iso, const char* dir_in, GdProgressFn cb, void* user,
                     char* msg, size_t msgsz)
{
    char dir[1000];
    size_t l;
    snprintf(dir, sizeof dir, "%s", dir_in ? dir_in : "");
    l = strlen(dir);
    while (l > 1 && (dir[l - 1] == '/' || dir[l - 1] == '\\') && dir[l - 2] != ':') dir[--l] = 0;
    return gamedata_extract_(iso, dir, cb, user, msg, msgsz);
}

static int gamedata_extract_(const char* iso, const char* dir, GdProgressFn cb, void* user,
                             char* msg, size_t msgsz)
{
    uint8_t hdr[0x440];
    int64_t img_size = 0;
    char partial[1100], old[1100], path[1200], marker[1200];
    ISOArchive* arc = NULL;
    GdManifest m;
    Prog prog = { cb, user, 0, 0 };
    uint8_t* buf = NULL;
    int rc, i, n, stack_n = 0;
    struct { int next; char path[600]; } stack[64];
    uint64_t data_bytes;
    uint32_t fst_size = 0, crc;
    uint8_t *ico = NULL, *png = NULL;
    size_t ico_n = 0, png_n = 0;
    const char* lim = getenv("SMB2_TEST_WRITE_LIMIT");

    memset(&m, 0, sizeof m);
    s_written = 0;
    s_write_limit = (lim && *lim) ? strtoull(lim, NULL, 10) : (uint64_t)-1;
    if (!iso || !*iso || !dir || !*dir) {
        setmsg(msg, msgsz, "usage: --extract-iso <iso> --gamedata <dir> [--progress]");
        return GD_ERR_OTHER;
    }
    rc = check_image(iso, hdr, &img_size, msg, msgsz);
    if (rc) return rc;

    iso_set_quiet(true);
    arc = iso_open(iso);
    iso_set_quiet(false);
    if (!arc) {
        setmsg(msg, msgsz, "%s: unreadable FST (image corrupt)", iso);
        return GD_ERR_IMAGE;
    }
    n = iso_entry_count(arc);
    data_bytes = 0x440 + 0x2000;
    fst_size = be32(hdr + 0x428);
    data_bytes += fst_size;
    for (i = 1; i < n; i++) {
        const ISOFileEntry* e = iso_get_file(arc, i);
        if (e->is_directory) continue;
        if ((int64_t)e->offset + e->size > img_size) {
            setmsg(msg, msgsz, "%s is truncated or corrupt: %s lies beyond the end of the image "
                   "(image is %lld bytes)", iso, e->name, (long long)img_size);
            rc = GD_ERR_IMAGE;
            goto out;
        }
        if (strchr(e->name, '/') || strchr(e->name, '\\') || !strcmp(e->name, "..") ||
            strchr(e->name, ':')) {
            setmsg(msg, msgsz, "%s: bad FST file name '%s'", iso, e->name);
            rc = GD_ERR_IMAGE;
            goto out;
        }
        data_bytes += e->size;
    }
    for (i = 0; s_required[i]; i++) {
        int e = iso_path_to_index(arc, s_required[i]);
        if (e < 0 || iso_entry_is_dir(arc, e)) {
            setmsg(msg, msgsz, "%s: required game file %s is missing (modified or corrupt image)",
                   iso, s_required[i]);
            rc = GD_ERR_IMAGE;
            goto out;
        }
    }
    /* game icon from /memcard/sys.bin (best effort: a disc without it
     * still extracts; the manifest then has no icon lines) */
    {
        uint8_t px[32 * 32 * 4];
        if (disc_icon_from_archive(arc, px)) {
            uint8_t* big = (uint8_t*)malloc(256 * 256 * 4);
            ico = disc_icon_ico(px, &ico_n);
            if (big) { disc_icon_render(px, 256, big); png = disc_icon_png(big, 256, 256, &png_n); }
            free(big);
            if (!ico || !png) { free(ico); free(png); ico = png = NULL; ico_n = png_n = 0; }
        }
    }
    data_bytes += ico_n + png_n;
    prog.total = data_bytes * 2;    /* extract + verify pass */
    prog_add(&prog, 0);             /* "PROGRESS 0 <total>" */

    /* target folders: <dir>.partial / <dir>.old next to <dir> */
    snprintf(partial, sizeof partial, "%s.partial", dir);
    snprintf(old, sizeof old, "%s.old", dir);
    {
        char man[1200];
        pjoin(man, sizeof man, dir, "manifest.txt");
        if (is_dir(dir) && !exists(man) && !dir_is_empty(dir)) {
            setmsg(msg, msgsz, "%s exists and is not a game data folder (no manifest.txt); "
                   "refusing to replace it", dir);
            rc = GD_ERR_OTHER;
            goto out;
        }
        if (exists(dir) && !is_dir(dir)) {
            setmsg(msg, msgsz, "%s exists and is not a folder", dir);
            rc = GD_ERR_OTHER;
            goto out;
        }
    }
    if (exists(partial)) {
        pjoin(marker, sizeof marker, partial, PARTIAL_MARKER);
        if (!exists(marker) && !dir_is_empty(partial)) {
            setmsg(msg, msgsz, "%s exists but was not created by an extraction; refusing to delete it",
                   partial);
            rc = GD_ERR_OTHER;
            goto out;
        }
        if (!rm_tree(partial)) {
            setmsg(msg, msgsz, "cannot remove stale %s", partial);
            rc = GD_ERR_WRITE;
            goto out;
        }
    }
    {
        uint64_t avail = free_space(partial);
        uint64_t need = data_bytes + (1u << 20) * 2;
        if (avail != (uint64_t)-1 && avail < need) {
            setmsg(msg, msgsz, "not enough disk space for %s: need %llu MB, %llu MB free",
                   dir, (unsigned long long)(need >> 20), (unsigned long long)(avail >> 20));
            rc = GD_ERR_WRITE;
            goto out;
        }
    }
    if (!mkdir_p(partial)) {
        setmsg(msg, msgsz, "cannot create %s: %s", partial, strerror(errno));
        rc = GD_ERR_WRITE;
        goto out;
    }
    pjoin(marker, sizeof marker, partial, PARTIAL_MARKER);
    rc = write_blob(marker, "partial\n", 8, msg, msgsz);
    if (rc) goto fail;

    buf = (uint8_t*)malloc(CHUNK);
    if (!buf) { setmsg(msg, msgsz, "out of memory"); rc = GD_ERR_OTHER; goto fail; }

    snprintf(m.disc_id, sizeof m.disc_id, "%.6s", (const char*)hdr);
    m.rev = hdr[7];
    snprintf(m.game_name, sizeof m.game_name, "%.64s", (const char*)hdr + 0x20);

    /* sys/ */
    pjoin(path, sizeof path, partial, "sys");
    if (!mkdir_p(path)) { rc = werr_code(msg, msgsz, path); goto fail; }
    {
        static const struct { const char* name; uint32_t off; } sys[3] = {
            { "sys/boot.bin", 0 }, { "sys/bi2.bin", 0x440 }, { "sys/fst.bin", 0 } };
        for (i = 0; i < 3; i++) {
            uint32_t off = i == 2 ? be32(hdr + 0x424) : sys[i].off;
            uint32_t sz = i == 0 ? 0x440 : i == 1 ? 0x2000 : fst_size;
            char rel[64];
            snprintf(rel, sizeof rel, "%s", sys[i].name);
            host_rel(rel);
            pjoin(path, sizeof path, partial, rel);
            rc = extract_to(arc, -1, off, sz, path, buf, &crc, &prog, msg, msgsz);
            if (rc) goto fail;
            if (i == 2) m.fst_crc = crc;
            man_add(&m, sys[i].name, sz, crc);
        }
    }

    /* files/: walk the FST in order (a dir at i spans (i, next)) */
    pjoin(path, sizeof path, partial, "files");
    if (!mkdir_p(path)) { rc = werr_code(msg, msgsz, path); goto fail; }
    stack[0].next = n;
    snprintf(stack[0].path, sizeof stack[0].path, "files");
    stack_n = 1;
    for (i = 1; i < n; i++) {
        const ISOFileEntry* e = iso_get_file(arc, i);
        char rel[700], hrel[700];
        while (stack_n > 1 && i >= stack[stack_n - 1].next) stack_n--;
        snprintf(rel, sizeof rel, "%s/%s", stack[stack_n - 1].path, e->name);
        snprintf(hrel, sizeof hrel, "%s", rel);
        host_rel(hrel);
        pjoin(path, sizeof path, partial, hrel);
        if (e->is_directory) {
            if (stack_n >= 64) { setmsg(msg, msgsz, "FST nesting too deep"); rc = GD_ERR_IMAGE; goto fail; }
            if (mkdir1(path) != 0 && !is_dir(path)) { rc = werr_code(msg, msgsz, path); goto fail; }
            stack[stack_n].next = iso_entry_next(arc, i);
            snprintf(stack[stack_n].path, sizeof stack[stack_n].path, "%s", rel);
            stack_n++;
            continue;
        }
        rc = extract_to(arc, i, 0, e->size, path, buf, &crc, &prog, msg, msgsz);
        if (rc) goto fail;
        if (!man_add(&m, rel, e->size, crc)) { setmsg(msg, msgsz, "out of memory"); rc = GD_ERR_OTHER; goto fail; }
    }

    /* icon (generated up front so the progress total is fixed) */
    if (ico) {
        pjoin(path, sizeof path, partial, "icon.ico");
        rc = write_blob(path, ico, ico_n, msg, msgsz);
        if (rc) goto fail;
        man_add(&m, "icon.ico", ico_n, gd_crc32(0, ico, ico_n));
        prog_add(&prog, ico_n);
        pjoin(path, sizeof path, partial, "icon_256.png");
        rc = write_blob(path, png, png_n, msg, msgsz);
        if (rc) goto fail;
        man_add(&m, "icon_256.png", png_n, gd_crc32(0, png, png_n));
        prog_add(&prog, png_n);
    }

    rc = man_write(&m, partial, msg, msgsz);
    if (rc) goto fail;

    /* verify what landed on disk against the CRCs read from the image */
    rc = verify_files(partial, &m, &prog, msg, msgsz);
    if (rc) { rc = rc == GD_ERR_MISMATCH ? GD_ERR_WRITE : rc; goto fail; }
    remove(marker);

    iso_close(arc);
    arc = NULL;
    /* swap in: <dir> -> <dir>.old, .partial -> <dir>, delete .old */
    if (exists(old)) {
        char man[1200];
        pjoin(man, sizeof man, old, "manifest.txt");
        if (exists(man) || dir_is_empty(old)) rm_tree(old);
    }
    if (is_dir(dir)) {
        if (!rename_dir(dir, old)) {
            setmsg(msg, msgsz, "cannot replace existing %s (in use?)", dir);
            rc = GD_ERR_WRITE;
            goto fail;
        }
    }
    if (!rename_dir(partial, dir)) {
        setmsg(msg, msgsz, "cannot rename %s to %s", partial, dir);
        rename_dir(old, dir);       /* put the previous one back */
        rc = GD_ERR_WRITE;
        goto fail;
    }
    if (is_dir(old)) rm_tree(old);
    setmsg(msg, msgsz, "%d files, %llu bytes", m.count, (unsigned long long)m.total);
    rc = GD_OK;
    goto out;

fail:
    if (arc) { iso_close(arc); arc = NULL; }
    if (is_dir(partial)) rm_tree(partial);      /* never leave a broken folder */
out:
    if (arc) iso_close(arc);
    free(buf);
    free(ico);
    free(png);
    man_free(&m);
    return rc;
}

/* --------------------------------------------------------------- verify */
static int verify_files(const char* dir, const GdManifest* m, Prog* p, char* msg, size_t msgsz)
{
    uint8_t* buf = (uint8_t*)malloc(CHUNK);
    int i;
    if (!buf) { setmsg(msg, msgsz, "out of memory"); return GD_ERR_OTHER; }
    for (i = 0; i < m->count; i++) {
        char rel[700], path[1200];
        FILE* f;
        uint64_t got = 0;
        uint32_t crc = 0;
        size_t n;
        snprintf(rel, sizeof rel, "%s", m->files[i].path);
        host_rel(rel);
        pjoin(path, sizeof path, dir, rel);
        f = fopen(path, "rb");
        if (!f) {
            setmsg(msg, msgsz, "missing %s", path);
            free(buf);
            return GD_ERR_MISMATCH;
        }
        while ((n = fread(buf, 1, CHUNK, f)) > 0) {
            crc = gd_crc32(crc, buf, n);
            got += n;
            prog_add(p, n);
        }
        fclose(f);
        if (got != m->files[i].size || crc != m->files[i].crc) {
            setmsg(msg, msgsz, "%s: %s (size %llu crc %08x, expected %llu %08x)", path,
                   got != m->files[i].size ? "wrong size" : "content mismatch",
                   (unsigned long long)got, (unsigned)crc,
                   (unsigned long long)m->files[i].size, (unsigned)m->files[i].crc);
            free(buf);
            return GD_ERR_MISMATCH;
        }
    }
    free(buf);
    return GD_OK;
}

/* FST in sys/fst.bin agrees with the manifest (same files) */
static int check_structure(const char* dir, const GdManifest* m, char* msg, size_t msgsz)
{
    ISOArchive* arc;
    int i, n, files = 0, listed = 0;
    iso_set_quiet(true);
    arc = iso_open_gamedata(dir);
    iso_set_quiet(false);
    if (!arc) { setmsg(msg, msgsz, "%s: sys/boot.bin or sys/fst.bin missing or invalid", dir); return GD_ERR_MISMATCH; }
    n = iso_entry_count(arc);
    for (i = 1; i < n; i++) if (!iso_entry_is_dir(arc, i)) files++;
    for (i = 0; i < m->count; i++) if (!strncmp(m->files[i].path, "files/", 6)) listed++;
    for (i = 0; s_required[i]; i++) {
        int e = iso_path_to_index(arc, s_required[i]);
        if (e < 0) {
            iso_close(arc);
            setmsg(msg, msgsz, "%s: required game file %s missing from the FST", dir, s_required[i]);
            return GD_ERR_MISMATCH;
        }
    }
    iso_close(arc);
    if (files != listed) {
        setmsg(msg, msgsz, "%s: FST has %d files but the manifest lists %d", dir, files, listed);
        return GD_ERR_MISMATCH;
    }
    return GD_OK;
}

int gamedata_verify(const char* dir, GdProgressFn cb, void* user, char* msg, size_t msgsz)
{
    GdManifest m;
    Prog p = { cb, user, 0, 0 };
    int rc;
    if (!dir || !*dir) { setmsg(msg, msgsz, "usage: --verify-gamedata <dir>"); return GD_ERR_OTHER; }
    if (!man_read(&m, dir, msg, msgsz)) return GD_ERR_MISMATCH;
    p.total = m.total;
    prog_add(&p, 0);
    rc = check_structure(dir, &m, msg, msgsz);
    if (!rc) rc = verify_files(dir, &m, &p, msg, msgsz);
    if (!rc) setmsg(msg, msgsz, "%d files, %llu bytes", m.count, (unsigned long long)m.total);
    man_free(&m);
    return rc;
}

bool gamedata_quick_check(const char* dir, char* msg, size_t msgsz)
{
    GdManifest m;
    int i;
    if (!dir || !*dir || !is_dir(dir)) { setmsg(msg, msgsz, "game data folder %s does not exist", dir ? dir : ""); return false; }
    if (!man_read(&m, dir, msg, msgsz)) return false;
    for (i = 0; i < m.count; i++) {
        char rel[700], path[1200];
        struct stat st;
        snprintf(rel, sizeof rel, "%s", m.files[i].path);
        host_rel(rel);
        pjoin(path, sizeof path, dir, rel);
        if (stat(path, &st) != 0 || (uint64_t)st.st_size != m.files[i].size) {
            setmsg(msg, msgsz, "game data file %s is %s", path,
                   stat(path, &st) != 0 ? "missing" : "the wrong size");
            man_free(&m);
            return false;
        }
    }
    if (check_structure(dir, &m, msg, msgsz) != GD_OK) { man_free(&m); return false; }
    man_free(&m);
    return true;
}

/* ------------------------------------------------------------------ CLI */
typedef struct { uint64_t last; bool on; } CliProg;

static void cli_progress(uint64_t done, uint64_t total, void* user)
{
    CliProg* c = (CliProg*)user;
    if (!c->on) return;
    if (done == total || done - c->last >= (16u << 20) || c->last == (uint64_t)-1) {
        printf("PROGRESS %llu %llu\n", (unsigned long long)done, (unsigned long long)total);
        fflush(stdout);
        c->last = done;
    }
}

static int cli_result(int rc, const char* msg)
{
    if (rc == GD_OK) printf("RESULT OK\n");
    else printf("RESULT ERROR %d %s\n", rc, msg);
    fflush(stdout);
    return rc;
}

int gamedata_cli_extract(const char* iso, const char* dir, bool progress)
{
    char msg[1024] = "";
    CliProg c = { (uint64_t)-1, progress };
    int rc = gamedata_extract(iso, dir, cli_progress, &c, msg, sizeof msg);
    if (rc == GD_OK) { printf("INFO extracted %s\n", msg); fflush(stdout); }
    return cli_result(rc, msg);
}

int gamedata_cli_verify(const char* dir, bool progress)
{
    char msg[1024] = "";
    CliProg c = { (uint64_t)-1, progress };
    int rc = gamedata_verify(dir, cli_progress, &c, msg, sizeof msg);
    if (rc == GD_OK) { printf("INFO verified %s\n", msg); fflush(stdout); }
    return cli_result(rc, msg);
}
