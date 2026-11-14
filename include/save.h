#ifndef SAVE_H
#define SAVE_H

#include "platform.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standalone save format (save.c), not used by the game: its memory card
 * saves go through card_shim.c in the GC format. */

#define SMB2_SAVE_MAGIC       0x534D4232  /* "SMB2" */
#define SMB2_SAVE_VERSION     1
#define SMB2_SAVE_BLOCKS      8           /* 8 blocks = 64KB per save file */
#define SMB2_SAVE_SIZE        (SMB2_SAVE_BLOCKS * MEMCARD_BLOCK_SIZE)

/* Game progress flags (bitfield) */
#define SMB2_FLAG_STORY_COMPLETE      (1 << 0)
#define SMB2_FLAG_CHALLENGE_COMPLETE  (1 << 1)
#define SMB2_FLAG_ALL_BONUS           (1 << 2)
#define SMB2_FLAG_ALL_STAGES_UNLOCKED (1 << 3)
#define SMB2_FLAG_MASTER_UNLOCKED     (1 << 4)
#define SMB2_FLAG_SOUND_TEST          (1 << 5)
#define SMB2_FLAG_GALLERY_COMPLETE    (1 << 6)

/* Character unlock flags */
#define SMB2_CHAR_AIAI      (1 << 0)
#define SMB2_CHAR_MEEMEE    (1 << 1)
#define SMB2_CHAR_BABY      (1 << 2)
#define SMB2_CHAR_GONGON    (1 << 3)
#define SMB2_CHAR_JAM       (1 << 4)
#define SMB2_CHAR_UNLOCKED_MASK 0x1F

/* World/Stage completion */
#define SMB2_MAX_WORLDS     10
#define SMB2_MAX_STAGES     10
#define SMB2_MAX_STORY_STAGES 100

typedef struct {
    uint32_t magic;              /* SMB2_SAVE_MAGIC */
    uint32_t version;            /* SMB2_SAVE_VERSION */
    uint32_t checksum;           /* CRC32 of data[0..SMB2_SAVE_SIZE-13] */
    uint32_t play_time_frames;   /* Total play time in frames (60fps) */
    uint32_t play_time_seconds;  /* Total play time in seconds */

    /* Progress */
    uint32_t progress_flags;     /* SMB2_FLAG_* */
    uint32_t unlocked_chars;     /* SMB2_CHAR_* bitmask */
    uint8_t  completed_stages[SMB2_MAX_STORY_STAGES / 8];  /* Bitfield */
    uint8_t  stage_ranks[SMB2_MAX_STORY_STAGES];           /* Rank per stage (S/A/B/C/D) */

    /* Story mode */
    uint8_t  current_world;
    uint8_t  current_stage;
    uint8_t  current_lives;
    uint8_t  current_continues;
    int32_t  current_score;
    int32_t  high_score_story;

    /* Challenge mode */
    int32_t  challenge_high_scores[SMB2_MAX_WORLDS];
    uint8_t  challenge_unlocked[SMB2_MAX_WORLDS];

    /* Options */
    uint8_t  sound_volume;       /* 0-127 */
    uint8_t  music_volume;       /* 0-127 */
    uint8_t  vibration;          /* 0=off, 1=on */
    uint8_t  language;           /* 0=JP, 1=EN, 2=FR, 3=DE, 4=ES, 5=IT */
    uint8_t  screen_mode;        /* 0=4:3, 1=16:9 */
    uint8_t  controller_type;    /* 0=GC, 1=Classic, 2=Wii Remote */
    uint8_t  rumble_enabled;     /* 0=off, 1=on */
    uint8_t  autosave;           /* 0=off, 1=on */

    /* Statistics */
    uint32_t total_bananas;
    uint32_t total_play_count;
    uint32_t continues_used;
    uint32_t deaths;

    /* Replay data (optional) */
    uint8_t  has_replay;
    uint32_t replay_size;
    uint8_t  replay_data[1024];  /* Compressed replay */

    /* Padding to fill block */
    uint8_t  reserved[SMB2_SAVE_SIZE - 256];  /* Remainder */
} SMB2SaveData;

/* Save system API */
bool save_system_init(void);
void save_system_shutdown(void);

/* Slot management (0=A, 1=B, 2=C) */
bool save_slot_exists(int slot);
bool save_slot_read(int slot, SMB2SaveData* out_data);
bool save_slot_write(int slot, const SMB2SaveData* data);
bool save_slot_delete(int slot);
bool save_slot_format(int slot, SMB2SaveData* out_data);  /* Create new empty save */

/* High-level game save API */
bool save_game_create_new(int slot, SMB2SaveData* out_data);
bool save_game_load(int slot, SMB2SaveData* out_data);
bool save_game_save(int slot, const SMB2SaveData* data);
bool save_game_autosave(const SMB2SaveData* data);  /* Auto-save to last used slot */

/* Utility */
uint32_t save_calculate_checksum(const SMB2SaveData* data);
bool save_verify_checksum(const SMB2SaveData* data);
void save_dump_info(const SMB2SaveData* data);  /* Debug print */

/* Memory card emulation (for GameCube compatibility) */
typedef struct {
    bool mounted;
    uint32_t free_blocks;
    char volume_label[32];
} MEMCardStatus;

bool memcard_mount(int slot, MEMCardStatus* out_status);
bool memcard_unmount(int slot);
bool memcard_format(int slot);
bool memcard_get_status(int slot, MEMCardStatus* out_status);

/* Global current save data (used by game) */
extern SMB2SaveData* g_current_save;
extern int g_current_save_slot;

/* Convenience macros */
#define SAVE_SLOT_A 0
#define SAVE_SLOT_B 1
#define SAVE_SLOT_C 2

#ifdef __cplusplus
}
#endif

#endif /* SAVE_H */