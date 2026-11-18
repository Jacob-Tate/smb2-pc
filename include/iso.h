#ifndef ISO_H
#define ISO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GameCube disc structures */
#define GC_DISC_MAGIC   0xC2339F3D  /* big-endian word at 0x1C */
#define WII_DISC_MAGIC  0x5D1C9EA3  /* big-endian word at 0x18 */

/* GameCube disc header (at offset 0x000, 0x440 bytes) */
typedef struct {
    char game_code[4];          /* 0x000: console + code + region, e.g. "GM2E" */
    char maker_code[2];         /* 0x004: e.g. "8P" (Sega) */
    uint8_t disc_id;            /* 0x006 */
    uint8_t version;            /* 0x007 */
    uint8_t audio_streaming;    /* 0x008 */
    uint8_t stream_buf_size;    /* 0x009 */
    uint8_t padding[14];        /* 0x00A */
    uint32_t wii_magic;         /* 0x018: 0x5D1C9EA3 on Wii discs */
    uint32_t magic;             /* 0x01C: 0xC2339F3D on GC discs */
    char game_name[0x3E0];      /* 0x020 */
    uint32_t debug_monitor_offset; /* 0x400 */
    uint32_t debug_monitor_addr;   /* 0x404 */
    uint8_t padding2[24];       /* 0x408 */
    uint32_t dol_offset;        /* 0x420 */
    uint32_t fst_offset;        /* 0x424 */
    uint32_t fst_size;          /* 0x428 */
    uint32_t fst_max_size;      /* 0x42C */
    uint32_t user_pos;          /* 0x430 */
    uint32_t user_len;          /* 0x434 */
    uint32_t unk1;              /* 0x438 */
    uint32_t zero;              /* 0x43C */
} __attribute__((packed)) GCDiscHeader;

/* FST Entry */
typedef struct {
    uint8_t type;               /* 0 = file, 1 = directory */
    uint8_t name_offset[3];     /* 24-bit offset into string table */
    uint32_t file_offset;       /* File offset (for files) or parent dir index (for dirs) */
    uint32_t file_size;         /* File size (for files) or next dir index (for dirs) */
} __attribute__((packed)) GCFSTEntry;

/* ISO/Archive handle */
typedef struct ISOArchive ISOArchive;

typedef struct {
    char* name;
    uint32_t offset;
    uint32_t size;
    bool is_directory;
    int parent_index;
    int first_child;
    int next_sibling;
} ISOFileEntry;

/* Archive API */
ISOArchive* iso_open(const char* path);
void iso_close(ISOArchive* arc);
/* Extracted game-data folder (sys/boot.bin + sys/fst.bin + files/...): same
 * entries/indices/reads as the image it was extracted from.  Structural
 * checks only; gamedata_check() (gamedata.h) validates against manifest.txt. */
ISOArchive* iso_open_gamedata(const char* dir);
void iso_set_quiet(bool quiet);              /* no "ISO opened" banner on stdout */
bool iso_archive_is_gamedata(ISOArchive* arc);
const char* iso_archive_path(ISOArchive* arc);
const GCDiscHeader* iso_archive_header(ISOArchive* arc);
const char* iso_entry_host_path(ISOArchive* arc, int index); /* folder backend only */
bool iso_read_raw(ISOArchive* arc, uint32_t offset, uint32_t size, void* dest); /* image only */
int iso_entry_count(ISOArchive* arc);
int iso_entry_is_dir(ISOArchive* arc, int index);
unsigned int iso_entry_size(ISOArchive* arc, int index);
const char* iso_entry_name(ISOArchive* arc, int index);
int iso_entry_next(ISOArchive* arc, int index);
int iso_file_read_at(ISOArchive* arc, int index, unsigned int offset, unsigned int size, void* dest);
int iso_path_to_index(ISOArchive* arc, const char* path);

/* File listing */
int iso_get_file_count(ISOArchive* arc);
uint32_t iso_fst_crc32(ISOArchive* arc);   /* disc identity (netplay handshake); 0 if unknown */
const ISOFileEntry* iso_get_file(ISOArchive* arc, int index);
const ISOFileEntry* iso_find_file(ISOArchive* arc, const char* path);

/* Extraction */
bool iso_extract_file(ISOArchive* arc, int index, void** out_data, size_t* out_size);
bool iso_extract_file_to(ISOArchive* arc, int index, const char* out_path);
bool iso_extract_all(ISOArchive* arc, const char* out_dir, bool overwrite);

/* Game detection */
typedef struct {
    char game_code[7];
    char game_name[65];
    char region;  /* J, U, E, P */
    bool is_wii;
} ISOGameInfo;

bool iso_get_game_info(ISOArchive* arc, ISOGameInfo* out_info);

/* Widescreen patching */
typedef struct {
    bool enabled;
    float aspect_ratio;  /* 16:9 = 1.777..., 21:9 = 2.37... */
    bool stretch_hud;
    bool stretch_2d;
} WidescreenConfig;

void iso_apply_widescreen_patch(ISOArchive* arc, const WidescreenConfig* config);

/* Asset cache */
typedef struct AssetCache AssetCache;

AssetCache* asset_cache_create(const char* cache_dir);
void asset_cache_destroy(AssetCache* cache);

/* Convert extracted assets to runtime formats */
bool asset_cache_convert_texture(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size);
bool asset_cache_convert_model(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size);
bool asset_cache_convert_audio(AssetCache* cache, const ISOFileEntry* entry, const void* data, size_t size);

/* Get converted asset paths */
const char* asset_cache_get_texture_path(AssetCache* cache, const char* original_name);
const char* asset_cache_get_model_path(AssetCache* cache, const char* original_name);
const char* asset_cache_get_audio_path(AssetCache* cache, const char* original_name);

#ifdef __cplusplus
}
#endif

#endif /* ISO_H */