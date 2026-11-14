/* Generic asset manager for the assets/ folder (include/assets.h).  Disc
 * files go through dvd_shim.c. */
#include "assets.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

/* Asset structure */
struct Asset {
    char* path;
    AssetType type;
    void* data;
    size_t size;
    int ref_count;
    bool loaded;
};

/* Asset manager structure */
struct AssetManager {
    char* base_path;
    Asset** assets;
    int asset_count;
    int asset_capacity;
};

/* Archive structure (placeholder for GC disc images) */
struct Archive {
    char* path;
    void* handle;
};

/* Global asset manager */
AssetManager* g_asset_manager = NULL;

/* Helper: join paths */
static char* path_join(const char* base, const char* file) {
    size_t base_len = strlen(base);
    size_t file_len = strlen(file);
    char* result = malloc(base_len + 1 + file_len + 1);
    if (!result) return NULL;
    strcpy(result, base);
    if (base_len > 0 && base[base_len - 1] != '/') {
        result[base_len] = '/';
        strcpy(result + base_len + 1, file);
    } else {
        strcpy(result + base_len, file);
    }
    return result;
}

/* Helper: get file extension */
static const char* get_extension(const char* path) {
    const char* dot = strrchr(path, '.');
    return dot ? dot + 1 : "";
}

/* AssetManager API */
AssetManager* asset_manager_create(const char* base_path) {
    AssetManager* mgr = calloc(1, sizeof(AssetManager));
    if (!mgr) return NULL;

    mgr->base_path = strdup(base_path ? base_path : ".");
    mgr->asset_capacity = 256;
    mgr->assets = calloc(mgr->asset_capacity, sizeof(Asset*));

    printf("Asset manager created with base path: %s\n", mgr->base_path);
    return mgr;
}

void asset_manager_destroy(AssetManager* mgr) {
    if (!mgr) return;

    for (int i = 0; i < mgr->asset_count; i++) {
        Asset* asset = mgr->assets[i];
        if (asset) {
            free(asset->path);
            free(asset->data);
            free(asset);
        }
    }
    free(mgr->assets);
    free(mgr->base_path);
    free(mgr);
}

static Asset* asset_create(AssetManager* mgr, const char* path, AssetType type) {
    Asset* asset = calloc(1, sizeof(Asset));
    if (!asset) return NULL;

    asset->path = strdup(path);
    asset->type = type;
    asset->ref_count = 1;
    asset->loaded = false;

    /* Resize array if needed */
    if (mgr->asset_count >= mgr->asset_capacity) {
        mgr->asset_capacity *= 2;
        mgr->assets = realloc(mgr->assets, mgr->asset_capacity * sizeof(Asset*));
    }
    mgr->assets[mgr->asset_count++] = asset;

    return asset;
}

Asset* asset_load(AssetManager* mgr, const char* path, AssetType type) {
    if (!mgr) return NULL;

    /* Check if already loaded */
    for (int i = 0; i < mgr->asset_count; i++) {
        if (mgr->assets[i] && strcmp(mgr->assets[i]->path, path) == 0) {
            mgr->assets[i]->ref_count++;
            return mgr->assets[i];
        }
    }

    char* full_path = path_join(mgr->base_path, path);
    if (!full_path) return NULL;

    Asset* asset = asset_create(mgr, path, type);
    if (!asset) {
        free(full_path);
        return NULL;
    }

    FILE* f = fopen(full_path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open asset: %s\n", full_path);
        free(full_path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    asset->size = ftell(f);
    fseek(f, 0, SEEK_SET);

    asset->data = malloc(asset->size);
    if (!asset->data) {
        fclose(f);
        free(full_path);
        return NULL;
    }

    fread(asset->data, 1, asset->size, f);
    fclose(f);
    free(full_path);

    asset->loaded = true;
    printf("Loaded asset: %s (%zu bytes, type %d)\n", path, asset->size, type);
    return asset;
}

bool asset_load_async(AssetManager* mgr, const char* path, AssetType type, AssetLoadCallback cb, void* user_data) {
    /* loads synchronously */
    Asset* asset = asset_load(mgr, path, type);
    if (cb) cb(asset, user_data);
    return asset != NULL;
}

void* asset_get_data(Asset* asset) {
    return asset ? asset->data : NULL;
}

size_t asset_get_size(Asset* asset) {
    return asset ? asset->size : 0;
}

AssetType asset_get_type(Asset* asset) {
    return asset ? asset->type : ASSET_TYPE_RAW;
}

const char* asset_get_path(Asset* asset) {
    return asset ? asset->path : NULL;
}

void asset_retain(Asset* asset) {
    if (asset) asset->ref_count++;
}

void asset_release(Asset* asset) {
    if (!asset) return;
    asset->ref_count--;
    if (asset->ref_count <= 0) {
        /* Find and remove from manager */
        if (g_asset_manager) {
            for (int i = 0; i < g_asset_manager->asset_count; i++) {
                if (g_asset_manager->assets[i] == asset) {
                    free(asset->path);
                    free(asset->data);
                    free(asset);
                    g_asset_manager->assets[i] = g_asset_manager->assets[--g_asset_manager->asset_count];
                    break;
                }
            }
        }
    }
}

bool asset_get_texture_info(Asset* asset, TextureInfo* out_info) {
    if (!asset || !out_info || asset->type != ASSET_TYPE_TEXTURE) return false;
    memset(out_info, 0, sizeof(TextureInfo));
    /* header not parsed */
    return true;
}

bool asset_get_model_info(Asset* asset, ModelInfo* out_info) {
    if (!asset || !out_info || asset->type != ASSET_TYPE_MODEL) return false;
    memset(out_info, 0, sizeof(ModelInfo));
    /* header not parsed */
    return true;
}

bool asset_get_sound_info(Asset* asset, SoundInfo* out_info) {
    if (!asset || !out_info || asset->type != ASSET_TYPE_SOUND) return false;
    memset(out_info, 0, sizeof(SoundInfo));
    /* header not parsed */
    return true;
}

/* Global convenience functions */
bool assets_init(const char* base_path) {
    if (g_asset_manager) return true;
    g_asset_manager = asset_manager_create(base_path);
    return g_asset_manager != NULL;
}

void assets_shutdown(void) {
    if (g_asset_manager) {
        asset_manager_destroy(g_asset_manager);
        g_asset_manager = NULL;
    }
}

Asset* assets_load(const char* path, AssetType type) {
    if (!g_asset_manager) return NULL;
    return asset_load(g_asset_manager, path, type);
}

/* File system utilities */
bool fs_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

size_t fs_get_size(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) return st.st_size;
    return 0;
}

bool fs_read_file(const char* path, void** out_data, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* data = malloc(size);
    if (!data) {
        fclose(f);
        return false;
    }

    fread(data, 1, size, f);
    fclose(f);

    *out_data = data;
    *out_size = size;
    return true;
}

void fs_free_file(void* data) {
    free(data);
}

char* fs_get_base_path(void) {
    char* base = SDL_GetBasePath();
    if (base) {
        char* result = strdup(base);
        SDL_free(base);
        return result;
    }
    return strdup(".");
}

char* fs_get_pref_path(const char* org, const char* app) {
    char* pref = SDL_GetPrefPath(org, app);
    if (pref) {
        char* result = strdup(pref);
        SDL_free(pref);
        return result;
    }
    return NULL;
}

/* Archive handling (placeholder for GameCube disc images) */
Archive* archive_open(const char* path) {
    printf("Archive open requested: %s (not implemented)\n", path);
    Archive* arc = calloc(1, sizeof(Archive));
    arc->path = strdup(path);
    return arc;
}

void archive_close(Archive* arc) {
    if (!arc) return;
    free(arc->path);
    free(arc);
}

bool archive_extract(Archive* arc, const char* file_path, void** out_data, size_t* out_size) {
    /* not implemented */
    (void)arc; (void)file_path; (void)out_data; (void)out_size;
    return false;
}

bool archive_has_file(Archive* arc, const char* file_path) {
    (void)arc; (void)file_path;
    return false;
}

int archive_list_files(Archive* arc, char*** out_files) {
    (void)arc; (void)out_files;
    return 0;
}