#ifndef ASSETS_H
#define ASSETS_H

#include "platform.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Asset types */
typedef enum {
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MODEL,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_MUSIC,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_RAW,
} AssetType;

/* Asset handle */
typedef struct Asset Asset;

/* Asset manager */
typedef struct AssetManager AssetManager;

/* Asset manager API */
AssetManager* asset_manager_create(const char* base_path);
void asset_manager_destroy(AssetManager* mgr);

/* Load asset synchronously */
Asset* asset_load(AssetManager* mgr, const char* path, AssetType type);

/* Load asset asynchronously */
typedef void (*AssetLoadCallback)(Asset* asset, void* user_data);
bool asset_load_async(AssetManager* mgr, const char* path, AssetType type, AssetLoadCallback cb, void* user_data);

/* Get asset data */
void* asset_get_data(Asset* asset);
size_t asset_get_size(Asset* asset);
AssetType asset_get_type(Asset* asset);
const char* asset_get_path(Asset* asset);

/* Reference counting */
void asset_retain(Asset* asset);
void asset_release(Asset* asset);

/* Texture-specific */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t mip_levels;
    void* gpu_handle;
} TextureInfo;

bool asset_get_texture_info(Asset* asset, TextureInfo* out_info);

/* Model-specific (GMA format) */
typedef struct {
    uint32_t vertex_count;
    uint32_t face_count;
    uint32_t bone_count;
    uint32_t material_count;
    float bounds_min[3];
    float bounds_max[3];
} ModelInfo;

bool asset_get_model_info(Asset* asset, ModelInfo* out_info);

/* Sound-specific */
typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bits_per_sample;
    uint32_t duration_ms;
    bool is_streaming;
} SoundInfo;

bool asset_get_sound_info(Asset* asset, SoundInfo* out_info);

/* Global asset manager */
extern AssetManager* g_asset_manager;

/* Convenience functions */
bool assets_init(const char* base_path);
void assets_shutdown(void);
Asset* assets_load(const char* path, AssetType type);

/* File system utilities */
bool fs_exists(const char* path);
size_t fs_get_size(const char* path);
bool fs_read_file(const char* path, void** out_data, size_t* out_size);
void fs_free_file(void* data);
char* fs_get_base_path(void);
char* fs_get_pref_path(const char* org, const char* app);

/* Archive handling (for GameCube disc images) */
typedef struct Archive Archive;
Archive* archive_open(const char* path);
void archive_close(Archive* arc);
bool archive_extract(Archive* arc, const char* file_path, void** out_data, size_t* out_size);
bool archive_has_file(Archive* arc, const char* file_path);
int archive_list_files(Archive* arc, char*** out_files);

#ifdef __cplusplus
}
#endif

#endif /* ASSETS_H */