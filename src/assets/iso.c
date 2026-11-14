/* GameCube/Wii ISO Parser */
#include "iso.h"
#include "gamedata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

struct ISOArchive {
    FILE* file;
    char* path;
    GCDiscHeader header;
    GCFSTEntry* fst;
    char* string_table;
    int fst_count;
    ISOFileEntry* entries;
    bool is_wii;
    bool is_rvz;
    /* RVZ specific */
    void* rvz_data;
    size_t rvz_size;
    /* extracted game-data folder backend (iso_open_gamedata) */
    bool is_dir;
    char** host_paths;      /* per FST entry: <dir>/files/<path> */
    FILE* cur_file;         /* host file of cur_index, kept open between reads */
    int cur_index;
};

/* Read big-endian values */
static uint32_t read_be32(const void* ptr) {
    const uint8_t* b = (const uint8_t*)ptr;
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static uint16_t read_be16(const void* ptr) {
    const uint8_t* b = (const uint8_t*)ptr;
    return (b[0] << 8) | b[1];
}

static uint32_t read_be24(const void* ptr) {
    const uint8_t* b = (const uint8_t*)ptr;
    return (b[0] << 16) | (b[1] << 8) | b[2];
}

/* RVZ (compressed Wii/GC image): detected only, so iso_open can reject it */
static bool iso_read_rvz_header(FILE* f, ISOArchive* arc) {
    /* RVZ format: magic "RVZ\0", version, then compressed blocks */
    uint8_t magic[4];
    if (fread(magic, 1, 4, f) != 4) return false;
    if (magic[0] != 'R' || magic[1] != 'V' || magic[2] != 'Z' || magic[3] != 0) {
        fseek(f, 0, SEEK_SET);
        return false;
    }
    arc->is_rvz = true;
    arc->is_wii = true;  /* RVZ is typically Wii */
    return true;
}

int iso_entry_next(ISOArchive* arc, int index);
static bool s_iso_quiet;
void iso_set_quiet(bool quiet) { s_iso_quiet = quiet; }

/* Build entries[] from arc->fst (fst_size bytes).  Shared by the image and
 * the extracted-folder backends so FST indices (= DVD entry numbers), names
 * and sizes are identical whichever one is in use.  Rejects an FST whose
 * entry count / name offsets do not fit inside it. */
static bool iso_build_entries(ISOArchive* arc, uint32_t fst_size) {
    if (fst_size < sizeof(GCFSTEntry)) return false;
    /* First entry is root directory - contains count */
    arc->fst_count = read_be32(&arc->fst[0].file_size);
    if (arc->fst_count <= 0 || (uint64_t)arc->fst_count * sizeof(GCFSTEntry) > fst_size) {
        arc->fst_count = 0;
        return false;
    }

    /* String table follows FST entries */
    arc->string_table = (char*)&arc->fst[arc->fst_count];
    uint32_t strtab_size = fst_size - (uint32_t)arc->fst_count * sizeof(GCFSTEntry);
    for (int i = 0; i < arc->fst_count; i++) {
        uint32_t no = read_be24(arc->fst[i].name_offset);
        if (i > 0 && (no >= strtab_size || !memchr(arc->string_table + no, 0, strtab_size - no))) {
            arc->fst_count = 0;
            return false;
        }
    }

    /* Build file entry list */
    arc->entries = calloc(arc->fst_count, sizeof(ISOFileEntry));
    if (!arc->entries) return false;

    for (int i = 0; i < arc->fst_count; i++) {
        GCFSTEntry* fst = &arc->fst[i];
        ISOFileEntry* entry = &arc->entries[i];

        entry->is_directory = (fst->type == 1);
        uint32_t no = read_be24(fst->name_offset);
        entry->name = strdup(no < strtab_size ? &arc->string_table[no] : "");
        entry->offset = read_be32(&fst->file_offset);
        entry->size = read_be32(&fst->file_size);
        entry->is_directory = (fst->type == 1);

        if (entry->is_directory) {
            entry->parent_index = read_be32(&fst->file_offset);
            entry->first_child = i + 1;
            entry->next_sibling = read_be32(&fst->file_size);
        } else {
            entry->parent_index = -1;  /* Will be filled in below */
        }
    }

    /* Build parent/child relationships */
    for (int i = 1; i < arc->fst_count; i++) {
        if (!arc->entries[i].is_directory) {
            /* Find parent directory */
            for (int j = i - 1; j >= 0; j--) {
                if (arc->entries[j].is_directory) {
                    arc->entries[i].parent_index = j;
                    break;
                }
            }
        }
    }
    return true;
}

static void iso_print_opened(ISOArchive* arc, const char* what, const char* path) {
    if (s_iso_quiet) return;
    printf("%s opened: %s\n", what, path);
    printf("  Game Code: %.6s\n", arc->header.game_code);
    printf("  Game Name: %.64s\n", arc->header.game_name);
    printf("  Disc Type: %s\n", arc->is_wii ? "Wii" : "GameCube");
    printf("  Files: %d\n", arc->fst_count);
}

ISOArchive* iso_open(const char* path) {
    ISOArchive* arc = calloc(1, sizeof(ISOArchive));
    if (!arc) return NULL;

    arc->path = strdup(path);
    arc->file = fopen(path, "rb");
    if (!arc->file) {
        free(arc->path);
        free(arc);
        return NULL;
    }

    /* Check for RVZ */
    if (iso_read_rvz_header(arc->file, arc)) {
        fprintf(stderr, "RVZ format detected (not fully supported yet)\n");
        fclose(arc->file);
        free(arc->path);
        free(arc);
        return NULL;
    }

    /* Read disc header */
    if (fread(&arc->header, 1, sizeof(GCDiscHeader), arc->file) != sizeof(GCDiscHeader)) {
        fclose(arc->file);
        free(arc->path);
        free(arc);
        return NULL;
    }

    /* Verify magic */
    if (read_be32(&arc->header.magic) != GC_DISC_MAGIC) {
        if (read_be32(&arc->header.wii_magic) == WII_DISC_MAGIC) {
            arc->is_wii = true;
        } else {
            fprintf(stderr, "Invalid GC/Wii disc magic: 0x%08X\n",
                    read_be32(&arc->header.magic));
            fclose(arc->file);
            free(arc->path);
            free(arc);
            return NULL;
        }
    }

    /* Read FST */
    uint32_t fst_offset = read_be32(&arc->header.fst_offset);
    uint32_t fst_size = read_be32(&arc->header.fst_size);

    arc->fst = (fst_size >= sizeof(GCFSTEntry) && fst_size <= 0x1000000)
             ? malloc(fst_size) : NULL;
    if (!arc->fst || fseek(arc->file, (long)fst_offset, SEEK_SET) != 0 ||
        fread(arc->fst, 1, fst_size, arc->file) != fst_size ||
        !iso_build_entries(arc, fst_size)) {
        fprintf(stderr, "Unreadable disc FST: %s\n", path);
        iso_close(arc);
        return NULL;
    }

    iso_print_opened(arc, "ISO", path);
    return arc;
}

/* ---- extracted game-data folder backend ----------------------------------
 * <dir>/sys/boot.bin (disc header, 0x440 bytes), <dir>/sys/fst.bin (the raw
 * disc FST) and <dir>/files/<FST path> (one host file per FST file entry).
 * The FST comes from the same bytes as on the disc, so entry numbers, names,
 * sizes and directory order are exactly those of the image; only the data
 * reads go to host files instead of image offsets. */
static char* read_host_file(const char* path, uint32_t max, uint32_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n <= 0 || (unsigned long)n > max) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    char* buf = malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); fclose(f); return NULL; }
    fclose(f);
    *out_size = (uint32_t)n;
    return buf;
}

/* Host path of every FST entry ("<root>/files/a/b"), built from the
 * directory nesting (a dir at i spans (i, next)). */
static bool iso_build_host_paths(ISOArchive* arc, const char* root) {
    int stack[64], depth = 0;
    arc->host_paths = calloc(arc->fst_count, sizeof(char*));
    if (!arc->host_paths) return false;
    size_t rl = strlen(root);
    arc->host_paths[0] = malloc(rl + 7);
    if (!arc->host_paths[0]) return false;
    snprintf(arc->host_paths[0], rl + 7, "%s/files", root);
    stack[depth++] = 0;
    for (int i = 1; i < arc->fst_count; i++) {
        while (depth > 1 && i >= iso_entry_next(arc, stack[depth - 1])) depth--;
        const char* parent = arc->host_paths[stack[depth - 1]];
        size_t n = strlen(parent) + 1 + strlen(arc->entries[i].name) + 1;
        arc->host_paths[i] = malloc(n);
        if (!arc->host_paths[i]) return false;
        snprintf(arc->host_paths[i], n, "%s/%s", parent, arc->entries[i].name);
        if (arc->entries[i].is_directory) {
            if (depth >= 64) return false;
            stack[depth++] = i;
        }
    }
    return true;
}

ISOArchive* iso_open_gamedata(const char* dir) {
    ISOArchive* arc = calloc(1, sizeof(ISOArchive));
    char path[1200];
    uint32_t n = 0, fst_size = 0;
    char* hdr;
    if (!arc) return NULL;
    arc->path = strdup(dir);
    arc->cur_index = -1;

    snprintf(path, sizeof path, "%s/sys/boot.bin", dir);
    hdr = read_host_file(path, 0x10000, &n);
    if (!hdr || n < sizeof(GCDiscHeader)) {
        fprintf(stderr, "gamedata: missing/short %s\n", path);
        free(hdr);
        iso_close(arc);
        return NULL;
    }
    memcpy(&arc->header, hdr, sizeof(GCDiscHeader));
    free(hdr);
    if (read_be32(&arc->header.magic) != GC_DISC_MAGIC) {
        fprintf(stderr, "gamedata: %s is not a GameCube disc header\n", path);
        iso_close(arc);
        return NULL;
    }

    snprintf(path, sizeof path, "%s/sys/fst.bin", dir);
    arc->fst = (GCFSTEntry*)read_host_file(path, 0x1000000, &fst_size);
    if (!arc->fst || !iso_build_entries(arc, fst_size) || !iso_build_host_paths(arc, dir)) {
        fprintf(stderr, "gamedata: missing/invalid %s\n", path);
        iso_close(arc);
        return NULL;
    }
    arc->is_dir = true;
    iso_print_opened(arc, "Game data folder", dir);
    return arc;
}

bool iso_archive_is_gamedata(ISOArchive* arc) { return arc && arc->is_dir; }
const char* iso_archive_path(ISOArchive* arc) { return arc ? arc->path : NULL; }
const GCDiscHeader* iso_archive_header(ISOArchive* arc) { return arc ? &arc->header : NULL; }
const char* iso_entry_host_path(ISOArchive* arc, int index) {
    if (!arc || !arc->host_paths || index < 0 || index >= arc->fst_count) return NULL;
    return arc->host_paths[index];
}

/* Read raw image bytes (image backend only): sys/ regions for extraction. */
bool iso_read_raw(ISOArchive* arc, uint32_t offset, uint32_t size, void* dest) {
    if (!arc || !arc->file || arc->is_dir) return false;
    if (fseek(arc->file, (long)offset, SEEK_SET) != 0) return false;
    return fread(dest, 1, size, arc->file) == size;
}

void iso_close(ISOArchive* arc) {
    if (!arc) return;
    if (arc->file) fclose(arc->file);
    if (arc->cur_file) fclose(arc->cur_file);
    if (arc->host_paths) {
        for (int i = 0; i < arc->fst_count; i++) free(arc->host_paths[i]);
        free(arc->host_paths);
    }
    if (arc->fst) free(arc->fst);
    if (arc->entries) {
        for (int i = 0; i < arc->fst_count; i++) {
            free(arc->entries[i].name);
        }
        free(arc->entries);
    }
    if (arc->rvz_data) free(arc->rvz_data);
    free(arc->path);
    free(arc);
}

int iso_get_file_count(ISOArchive* arc) {
    return arc ? arc->fst_count : 0;
}

const ISOFileEntry* iso_get_file(ISOArchive* arc, int index) {
    if (!arc || index < 0 || index >= arc->fst_count) return NULL;
    return &arc->entries[index];
}

const ISOFileEntry* iso_find_file(ISOArchive* arc, const char* path) {
    if (!arc || !path) return NULL;

    for (int i = 0; i < arc->fst_count; i++) {
        if (arc->entries[i].name && strcmp(arc->entries[i].name, path) == 0) {
            return &arc->entries[i];
        }
    }
    return NULL;
}

bool iso_extract_file(ISOArchive* arc, int index, void** out_data, size_t* out_size) {
    if (!arc || index < 0 || index >= arc->fst_count) return false;

    ISOFileEntry* entry = &arc->entries[index];
    if (entry->is_directory) return false;

    uint32_t file_offset = entry->offset;
    uint32_t file_size = entry->size;

    /* GC disc FST offsets are absolute byte offsets */
    fseek(arc->file, file_offset, SEEK_SET);

    void* data = malloc(file_size);
    if (!data) return false;

    size_t read = fread(data, 1, file_size, arc->file);
    if (read != file_size) {
        free(data);
        return false;
    }

    *out_data = data;
    *out_size = file_size;
    return true;
}

bool iso_extract_file_to(ISOArchive* arc, int index, const char* out_path) {
    void* data = NULL;
    size_t size = 0;

    if (!iso_extract_file(arc, index, &data, &size)) return false;

    /* Create directory if needed */
    char* dir = strdup(out_path);
    char* last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        struct stat st = {0};
        if (stat(dir, &st) == -1) {
#if defined(_WIN32) || defined(__CYGWIN__)
            mkdir(dir);
#else
            mkdir(dir, 0755);
#endif
        }
    }
    free(dir);

    FILE* f = fopen(out_path, "wb");
    if (!f) {
        free(data);
        return false;
    }

    fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    return true;
}

bool iso_extract_all(ISOArchive* arc, const char* out_dir, bool overwrite) {
    if (!arc) return false;

    struct stat st = {0};
    if (stat(out_dir, &st) == -1) {
#if defined(_WIN32) || defined(__CYGWIN__)
        mkdir(out_dir);
#else
        mkdir(out_dir, 0755);
#endif
    }

    for (int i = 1; i < arc->fst_count; i++) {  /* Skip root */
        ISOFileEntry* entry = &arc->entries[i];
        if (entry->is_directory) continue;

        char out_path[1024];
        snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, entry->name);

        if (!overwrite) {
            if (access(out_path, F_OK) == 0) continue;
        }

        if (!iso_extract_file_to(arc, i, out_path)) {
            fprintf(stderr, "Failed to extract: %s\n", entry->name);
        }
    }
    return true;
}

bool iso_get_game_info(ISOArchive* arc, ISOGameInfo* out_info) {
    if (!arc || !out_info) return false;

    memset(out_info, 0, sizeof(ISOGameInfo));
    strncpy(out_info->game_code, arc->header.game_code, 6);
    out_info->game_code[6] = '\0';
    strncpy(out_info->game_name, arc->header.game_name, 64);
    out_info->game_name[64] = '\0';
    out_info->is_wii = arc->is_wii;

    /* Determine region from game code */
    char region_char = arc->header.game_code[3];
    switch (region_char) {
        case 'J': out_info->region = 'J'; break;
        case 'U': out_info->region = 'U'; break;
        case 'E': out_info->region = 'E'; break;
        case 'P': out_info->region = 'P'; break;  /* Europe/PAL */
        case 'K': out_info->region = 'K'; break;  /* Korea */
        default: out_info->region = '?'; break;
    }

    return true;
}

/* Widescreen DOL patch: unused placeholder, only logs */
void iso_apply_widescreen_patch(ISOArchive* arc, const WidescreenConfig* config) {
    if (!arc || !config || !config->enabled) return;

    /* Find main.dol */
    const ISOFileEntry* dol = iso_find_file(arc, "sys/main.dol");
    if (!dol) {
        /* Try alternate locations */
        dol = iso_find_file(arc, "main.dol");
    }
    if (!dol) {
        fprintf(stderr, "main.dol not found for widescreen patch\n");
        return;
    }

    /* Extract DOL */
    void* dol_data = NULL;
    size_t dol_size = 0;
    if (!iso_extract_file(arc, dol - arc->entries, &dol_data, &dol_size)) {
        fprintf(stderr, "Failed to extract main.dol\n");
        return;
    }

    printf("Widescreen patch would be applied to main.dol (aspect: %.2f)\n", config->aspect_ratio);
    printf("  This requires DOL relocation parsing - implement for production\n");

    free(dol_data);
}

/* Asset cache: offline conversion helpers, not used by the game */
struct AssetCache {
    char* cache_dir;
    char* texture_dir;
    char* model_dir;
    char* audio_dir;
};

AssetCache* asset_cache_create(const char* cache_dir) {
    AssetCache* cache = calloc(1, sizeof(AssetCache));
    if (!cache) return NULL;

    cache->cache_dir = strdup(cache_dir);

    asprintf(&cache->texture_dir, "%s/textures", cache_dir);
    asprintf(&cache->model_dir, "%s/models", cache_dir);
    asprintf(&cache->audio_dir, "%s/audio", cache_dir);

#if defined(_WIN32) || defined(__CYGWIN__)
    mkdir(cache->cache_dir);
    mkdir(cache->texture_dir);
    mkdir(cache->model_dir);
    mkdir(cache->audio_dir);
#else
    mkdir(cache->cache_dir, 0755);
    mkdir(cache->texture_dir, 0755);
    mkdir(cache->model_dir, 0755);
    mkdir(cache->audio_dir, 0755);
#endif

    return cache;
}

void asset_cache_destroy(AssetCache* cache) {
    if (!cache) return;
    free(cache->cache_dir);
    free(cache->texture_dir);
    free(cache->model_dir);
    free(cache->audio_dir);
    free(cache);
}

/* TPL (GameCube Texture) header */
typedef struct {
    uint32_t magic;       /* "TPL\0" */
    uint32_t version;
    uint32_t num_textures;
    uint32_t texture_offset;
} TPLHeader;

typedef struct {
    uint32_t format;
    uint16_t width;
    uint16_t height;
    uint32_t wrap_s;
    uint32_t wrap_t;
    uint32_t min_filter;
    uint32_t mag_filter;
    float lod_bias;
    uint8_t edge_lod;
    uint8_t min_lod;
    uint8_t max_lod;
    uint8_t unpacked;
    uint32_t data_offset;
    uint32_t palette_offset;
} TPLTextureHeader;

/* Convert TPL to KTX2/PNG */
bool asset_cache_convert_texture(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size) {
    if (!cache || !entry || !data || size < sizeof(TPLHeader)) return false;

    const TPLHeader* tpl = (const TPLHeader*)data;
    if (read_be32(&tpl->magic) != 0x54504C00) return false;  /* "TPL\0" */

    uint32_t num_textures = read_be32(&tpl->num_textures);
    uint32_t tex_offset = read_be32(&tpl->texture_offset);

    const uint8_t* base = (const uint8_t*)data;

    for (uint32_t i = 0; i < num_textures; i++) {
        const TPLTextureHeader* tex = (const TPLTextureHeader*)(base + tex_offset + i * sizeof(TPLTextureHeader));

        uint32_t format = read_be32(&tex->format);
        uint16_t width = read_be16(&tex->width);
        uint16_t height = read_be16(&tex->height);
        uint32_t data_off = read_be32(&tex->data_offset);

        const void* tex_data = base + data_off;

        /* Calculate data size based on format */
        size_t tex_size = 0;
        switch (format) {
            case 0x0: case 0x1: tex_size = width * height / 2; break;  /* I4, I8 */
            case 0x2: case 0x3: tex_size = width * height; break;      /* IA4, IA8 */
            case 0x4: tex_size = width * height * 2; break;            /* RGB565 */
            case 0x5: tex_size = width * height * 2; break;            /* RGB5A3 */
            case 0x6: tex_size = width * height * 4; break;            /* RGBA8 */
            case 0xE: tex_size = (width * height) / 2; break;          /* CMPR (DXT1) */
            default: tex_size = width * height * 4; break;
        }

        /* raw texel data behind a small header (w, h, format) */
        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/%s_%d.ktx", cache->texture_dir, entry->name, i);

        FILE* f = fopen(out_path, "wb");
        if (f) {
            /* Simple KTX2-like header */
            fwrite(&width, 2, 1, f);
            fwrite(&height, 2, 1, f);
            fwrite(&format, 4, 1, f);
            fwrite(tex_data, 1, tex_size, f);
            fclose(f);
            printf("Converted texture: %s -> %s (%dx%d, fmt=0x%X)\n",
                   entry->name, out_path, width, height, format);
        }
    }

    return true;
}

/* GMA (GameCube model): stored as is */
bool asset_cache_convert_model(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size) {
    if (!cache || !entry) return false;

    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s/%s.gma", cache->model_dir, entry->name);

    FILE* f = fopen(out_path, "wb");
    if (f) {
        fwrite(data, 1, size, f);
        fclose(f);
        printf("Stored model: %s -> %s\n", entry->name, out_path);
    }
    return true;
}

/* ADX (audio): stored as is */
bool asset_cache_convert_audio(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size) {
    if (!cache || !entry) return false;

    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s/%s.adx", cache->audio_dir, entry->name);

    FILE* f = fopen(out_path, "wb");
    if (f) {
        fwrite(data, 1, size, f);
        fclose(f);
        printf("Stored audio: %s -> %s\n", entry->name, out_path);
    }
    return true;
}

const char* asset_cache_get_texture_path(AssetCache* cache, const char* original_name) {
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s_0.ktx", cache->texture_dir, original_name);
    return path;
}

const char* asset_cache_get_model_path(AssetCache* cache, const char* original_name) {
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s.gma", cache->model_dir, original_name);
    return path;
}

const char* asset_cache_get_audio_path(AssetCache* cache, const char* original_name) {
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s.adx", cache->audio_dir, original_name);
    return path;
}
/* ------------------------------------------------------------------ */
/* Entry-index based access for the GameCube DVD shim.                */
/* GC DVD entry numbers are FST indices, which match entries[] here.  */
/* ------------------------------------------------------------------ */

int iso_entry_count(ISOArchive* arc) {
    return arc ? arc->fst_count : 0;
}

int iso_entry_is_dir(ISOArchive* arc, int index) {
    if (!arc || index < 0 || index >= arc->fst_count) return 0;
    return arc->entries[index].is_directory ? 1 : 0;
}

unsigned int iso_entry_size(ISOArchive* arc, int index) {
    if (!arc || index < 0 || index >= arc->fst_count) return 0;
    return arc->entries[index].size;
}

const char* iso_entry_name(ISOArchive* arc, int index) {
    if (!arc || index < 0 || index >= arc->fst_count) return NULL;
    return arc->entries[index].name;
}

/* FST "next" value: for a directory, one past its last descendant (so its
 * direct children span (index, next) with subdir subtrees skipped via their
 * own next); for a file, simply index + 1. */
int iso_entry_next(ISOArchive* arc, int index) {
    if (!arc || index < 0 || index >= arc->fst_count) return -1;
    if (arc->entries[index].is_directory) {
        int next = (int)arc->entries[index].next_sibling;
        if (next <= index || next > arc->fst_count) next = arc->fst_count;
        return next;
    }
    return index + 1;
}

/* Read size bytes of file entry `index` starting at file-relative offset.
 * Reads past EOF are zero-filled (the game rounds lengths up to 32).
 * Returns 0 on success, -1 on error. */
static int s_dvd_trace = -1;   /* SMB2_DVD_TRACE=1: log every game file read */
int iso_file_read_at(ISOArchive* arc, int index, unsigned int offset,
                     unsigned int size, void* dest) {
    if (!arc || !dest || index < 0 || index >= arc->fst_count) return -1;
    ISOFileEntry* e = &arc->entries[index];
    if (e->is_directory) return -1;

    unsigned int avail = (offset < e->size) ? (e->size - offset) : 0;
    unsigned int to_read = (size < avail) ? size : avail;

    if (s_dvd_trace < 0) {
        const char* t = getenv("SMB2_DVD_TRACE");
        s_dvd_trace = (t && t[0] == '1') ? 1 : 0;
    }

    if (to_read > 0) {
        if (arc->is_dir) {
            if (arc->cur_index != index) {
                if (arc->cur_file) fclose(arc->cur_file);
                arc->cur_file = fopen(arc->host_paths[index], "rb");
                arc->cur_index = arc->cur_file ? index : -1;
                if (!arc->cur_file) {
                    fprintf(stderr, "gamedata: cannot open %s\n", arc->host_paths[index]);
                    return -1;
                }
            }
            if (fseek(arc->cur_file, (long)offset, SEEK_SET) != 0) return -1;
            if (fread(dest, 1, to_read, arc->cur_file) != to_read) return -1;
        } else {
            if (fseek(arc->file, (long)(e->offset + offset), SEEK_SET) != 0) return -1;
            if (fread(dest, 1, to_read, arc->file) != to_read) return -1;
        }
    }
    if (to_read < size)
        memset((char*)dest + to_read, 0, size - to_read);
    if (s_dvd_trace)   /* crc of what the caller got: backends must agree */
        fprintf(stderr, "[dvdtrace] %d %s ofs=0x%x len=0x%x crc=%08x\n", index, e->name,
                offset, size, (unsigned)gd_crc32(0, dest, size));
    return 0;
}

/* Case-insensitive segment compare against a NUL-terminated name. */
static bool iso_seg_eq(const char* name, const char* seg, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char a = name[i], b = seg[i];
        if (a == '\0') return false;
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return false;
    }
    return name[len] == '\0';
}

/* Resolve a GC-style path ("/", "..", case-insensitive) to an FST index,
 * starting from directory entry dir_index. Returns -1 if not found. */
int iso_path_to_index_from(ISOArchive* arc, int dir_index, const char* path) {
    if (!arc || !path) return -1;
    int cur = (dir_index >= 0 && dir_index < arc->fst_count) ? dir_index : 0;
    const char* p = path;

    if (*p == '/') {
        cur = 0;
        while (*p == '/') p++;
    }
    while (*p) {
        const char* seg = p;
        while (*p && *p != '/') p++;
        size_t len = (size_t)(p - seg);
        while (*p == '/') p++;

        if (len == 0 || (len == 1 && seg[0] == '.')) continue;
        if (len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (cur != 0) {
                int parent = arc->entries[cur].parent_index;
                cur = (parent >= 0) ? parent : 0;
            }
            continue;
        }
        if (!arc->entries[cur].is_directory) return -1;

        /* children of dir `cur` span (cur, next_sibling) */
        int next = (int)arc->entries[cur].next_sibling;
        if (next <= cur || next > arc->fst_count) next = arc->fst_count;
        int found = -1;
        for (int j = cur + 1; j < next; ) {
            if (arc->entries[j].name && iso_seg_eq(arc->entries[j].name, seg, len)) {
                found = j;
                break;
            }
            if (arc->entries[j].is_directory) {
                int skip = (int)arc->entries[j].next_sibling;
                j = (skip > j) ? skip : j + 1;
            } else {
                j++;
            }
        }
        if (found < 0) return -1;
        cur = found;
    }
    return cur;
}

int iso_path_to_index(ISOArchive* arc, const char* path) {
    return iso_path_to_index_from(arc, 0, path);
}
