#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Platform-agnostic types matching GameCube SDK */
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

#if defined(_WIN32) || defined(__CYGWIN__)
/* Windows defines BOOL as int */
#include <windows.h>
#else
typedef uint32_t BOOL;
#endif

#define TRUE  1
#define FALSE 0

/* Forward declarations */
typedef struct PlatformContext PlatformContext;
typedef struct PlatformConfig PlatformConfig;
typedef struct ISOArchive ISOArchive;

/* Platform configuration */
struct PlatformConfig {
    int window_width;
    int window_height;
    bool fullscreen;
    bool vsync;
    int msaa_samples;
    const char* window_title;
    const char* asset_path;
    bool show_resolution_picker;
    bool opengl;            /* create a GL window + context (else D3D12) */
};

/* Platform function table - implemented by platform backend */
typedef struct {
    /* Lifecycle */
    bool (*init)(PlatformContext* ctx, const PlatformConfig* config);
    void (*shutdown)(PlatformContext* ctx);
    void (*poll_events)(PlatformContext* ctx);
    bool (*should_close)(PlatformContext* ctx);

    /* Timing */
    uint64_t (*get_time_ns)(PlatformContext* ctx);
    uint64_t (*get_ticks_ms)(PlatformContext* ctx);
    void (*sleep_ms)(uint32_t ms);

    /* Window */
    void (*swap_buffers)(PlatformContext* ctx);
    void (*set_window_title)(PlatformContext* ctx, const char* title);
    void (*get_window_size)(PlatformContext* ctx, int* w, int* h);
    void (*set_fullscreen)(PlatformContext* ctx, bool fullscreen);

    /* Resolution picker */
    bool (*show_resolution_picker)(PlatformContext* ctx, int* out_w, int* out_h, bool* out_fullscreen);
} PlatformVTable;

/* Platform context */
struct PlatformContext {
    const PlatformVTable* vtable;
    void* backend_data;  /* SDL, GLFW, etc. specific data */
};

/* Global platform instance */
extern PlatformContext g_platform;

/* Initialize platform with config */
bool platform_init(const PlatformConfig* config);

/* Shutdown platform */
void platform_shutdown(void);

/* Main loop helpers */
void platform_poll_events(void);
bool platform_should_close(void);
uint64_t platform_get_time_ns(void);
uint64_t platform_get_ticks_ms(void);
void platform_sleep_ms(uint32_t ms);
void platform_swap_buffers(void);

/* Window access for renderers / settings */
void* platform_sdl_window(void);               /* SDL_Window* */
void platform_drawable_size(int* w, int* h);    /* pixels */
bool platform_recreate_window(bool opengl);     /* renderer fallback */

/* Resolution picker - returns true if user confirmed, false if cancelled */
bool platform_show_resolution_picker(int* out_width, int* out_height, bool* out_fullscreen);

/* ISO picker - returns true if user selected a file */
bool platform_show_iso_picker(char** out_path);

/* Settings/Options menu */
typedef struct {
    int window_width;
    int window_height;
    bool fullscreen;
    bool vsync;
    int msaa_samples;

    /* Widescreen */
    bool widescreen_enabled;
    float widescreen_aspect;  /* 1.777 for 16:9, 2.37 for 21:9 */
    bool stretch_hud;
    bool stretch_2d;

    /* Audio */
    float master_volume;
    float music_volume;
    float sfx_volume;
    int output_device;

    /* Controls */
    int controller_type;  /* 0=GameCube, 1=Classic, 2=Keyboard */
    bool rumble_enabled;

    /* ISO */
    char* iso_path;
} GameSettings;

/* Settings API */
void settings_load(GameSettings* settings);
void settings_save(const GameSettings* settings);
void settings_get_defaults(GameSettings* settings);
bool platform_show_settings_menu(GameSettings* settings);  /* In-game options menu */

bool platform_startup_iso(void);             /* Load saved ISO or prompt for one */
ISOArchive* platform_get_iso_archive(void);  /* Get loaded ISO */
bool platform_load_iso(const char* path);    /* Load and parse ISO */
bool platform_load_gamedata(const char* dir);   /* extracted folder (validated) */
/* --gamedata, --iso, install.ini gamedata, ..., ISO prompt (docs/installer.md) */
bool platform_resolve_game_data(const char* gamedata_arg, const char* iso_arg);
bool platform_recover_gamedata(const char* why); /* installed: re-extract per user */
void platform_apply_window_icon(void);       /* icon from the disc in use */

/* Save file system (GameCube memory card compatible) */
/* Memory card: 59 blocks × 8192 bytes = ~483KB per card */
/* SMB2 uses 1 save file per game (slot A/B/C) */

#define MEMCARD_BLOCK_SIZE    8192
#define MEMCARD_MAX_BLOCKS    59
#define MEMCARD_MAX_FILES     127
#define MEMCARD_FILENAME_MAX  32

typedef struct {
    char filename[MEMCARD_FILENAME_MAX];
    uint32_t length;
    uint32_t offset;  /* Block offset */
    uint32_t time;    /* Timestamp */
    uint16_t icon_addr;
    uint16_t banner_addr;
    uint8_t  icon_format;
    uint8_t  banner_format;
    uint8_t  reserved[2];
} MEMCardFileInfo;

typedef struct {
    uint32_t magic;           /* "SMB2" */
    uint32_t version;         /* Save format version */
    uint32_t checksum;        /* Simple checksum */
    uint8_t  data[MEMCARD_BLOCK_SIZE * MEMCARD_MAX_BLOCKS - 12];
} SMB2SaveFile;

/* Save API */
const char* platform_get_save_dir(void);           /* Platform-specific save directory */
bool platform_save_exists(int slot);               /* Check if save slot exists (0-2 = A/B/C) */
bool platform_save_read(int slot, void* buffer, size_t size);   /* Read save file */
bool platform_save_write(int slot, const void* buffer, size_t size); /* Write save file */
bool platform_save_delete(int slot);               /* Delete save file */
int platform_save_list(char*** out_names);         /* List all save files */

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */