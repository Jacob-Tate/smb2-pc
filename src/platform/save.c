/* Standalone save format (save.h); not used by the game. */
#include "save.h"
#include "platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global state */
SMB2SaveData g_save_data = {0};
SMB2SaveData* g_current_save = &g_save_data;
int g_current_save_slot = -1;

static uint32_t crc32_table[256];
static bool crc32_table_init = false;

static void crc32_init_table(void) {
    if (crc32_table_init) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1)
                c = 0xEDB88320 ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc32_table[i] = c;
    }
    crc32_table_init = true;
}

uint32_t save_calculate_checksum(const SMB2SaveData* data) {
    crc32_init_table();
    uint32_t crc = 0xFFFFFFFF;

    /* Calculate CRC32 of everything except magic, version, checksum */
    const uint8_t* bytes = (const uint8_t*)data;
    size_t offset = 12;  /* Skip magic(4) + version(4) + checksum(4) */
    size_t len = SMB2_SAVE_SIZE - offset;

    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ bytes[offset + i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

bool save_verify_checksum(const SMB2SaveData* data) {
    uint32_t calc = save_calculate_checksum(data);
    return calc == data->checksum;
}

bool save_system_init(void) {
    printf("Save system initialized\n");
    printf("Save directory: %s\n", platform_get_save_dir());
    return true;
}

void save_system_shutdown(void) {
    /* Auto-save on shutdown if enabled */
    if (g_current_save_slot >= 0 && g_current_save && g_current_save->autosave) {
        save_game_save(g_current_save_slot, g_current_save);
    }
    printf("Save system shut down\n");
}

bool save_slot_exists(int slot) {
    return platform_save_exists(slot);
}

bool save_slot_read(int slot, SMB2SaveData* out_data) {
    if (!out_data) return false;

    if (!platform_save_read(slot, out_data, SMB2_SAVE_SIZE)) {
        return false;
    }

    /* Verify magic and checksum */
    if (out_data->magic != SMB2_SAVE_MAGIC) {
        fprintf(stderr, "Save slot %d: Invalid magic (0x%08X)\n", slot, out_data->magic);
        return false;
    }

    if (out_data->version != SMB2_SAVE_VERSION) {
        fprintf(stderr, "Save slot %d: Version mismatch (%d vs %d)\n",
                slot, out_data->version, SMB2_SAVE_VERSION);
        return false;
    }

    if (!save_verify_checksum(out_data)) {
        fprintf(stderr, "Save slot %d: Checksum mismatch!\n", slot);
        return false;
    }

    printf("Loaded save slot %d (play time: %u sec)\n",
           slot, out_data->play_time_seconds);
    return true;
}

bool save_slot_write(int slot, const SMB2SaveData* data) {
    if (!data) return false;

    /* Create a copy to update checksum */
    SMB2SaveData save_copy = *data;
    save_copy.checksum = save_calculate_checksum(&save_copy);

    bool result = platform_save_write(slot, &save_copy, SMB2_SAVE_SIZE);
    if (result) {
        printf("Saved to slot %d\n", slot);
    } else {
        fprintf(stderr, "Failed to write save slot %d\n", slot);
    }
    return result;
}

bool save_slot_delete(int slot) {
    bool result = platform_save_delete(slot);
    if (result) {
        printf("Deleted save slot %d\n", slot);
    }
    return result;
}

bool save_slot_format(int slot, SMB2SaveData* out_data) {
    if (!out_data) return false;

    memset(out_data, 0, SMB2_SAVE_SIZE);
    out_data->magic = SMB2_SAVE_MAGIC;
    out_data->version = SMB2_SAVE_VERSION;
    out_data->sound_volume = 100;
    out_data->music_volume = 100;
    out_data->vibration = 1;
    out_data->language = 1;  /* English default */
    out_data->screen_mode = 1;  /* 16:9 default */
    out_data->controller_type = 0;
    out_data->rumble_enabled = 1;
    out_data->autosave = 1;

    /* Unlock default characters */
    out_data->unlocked_chars = SMB2_CHAR_AIAI | SMB2_CHAR_MEEMEE |
                               SMB2_CHAR_BABY | SMB2_CHAR_GONGON;

    out_data->checksum = save_calculate_checksum(out_data);

    return save_slot_write(slot, out_data);
}

bool save_game_create_new(int slot, SMB2SaveData* out_data) {
    return save_slot_format(slot, out_data);
}

bool save_game_load(int slot, SMB2SaveData* out_data) {
    bool result = save_slot_read(slot, out_data);
    if (result) {
        g_current_save_slot = slot;
        if (out_data != g_current_save) {
            *g_current_save = *out_data;
        }
    }
    return result;
}

bool save_game_save(int slot, const SMB2SaveData* data) {
    if (!data) return false;

    /* Update play time before saving */
    SMB2SaveData save_copy = *data;
    /* play_time_frames/seconds would be updated by game loop */

    bool result = save_slot_write(slot, &save_copy);
    if (result) {
        g_current_save_slot = slot;
        *g_current_save = save_copy;
    }
    return result;
}

bool save_game_autosave(const SMB2SaveData* data) {
    if (g_current_save_slot >= 0 && data && data->autosave) {
        return save_game_save(g_current_save_slot, data);
    }
    return true;  /* Not an error if autosave disabled */
}

void save_dump_info(const SMB2SaveData* data) {
    if (!data) return;

    printf("\n=== Save Data Info ===\n");
    printf("Magic: 0x%08X\n", data->magic);
    printf("Version: %u\n", data->version);
    printf("Checksum: 0x%08X (valid: %s)\n", data->checksum,
           save_verify_checksum(data) ? "YES" : "NO");
    printf("Play time: %u sec (%u frames)\n",
           data->play_time_seconds, data->play_time_frames);
    printf("Progress flags: 0x%08X\n", data->progress_flags);
    printf("Unlocked chars: 0x%02X\n", data->unlocked_chars);
    printf("Current: World %u Stage %u\n", data->current_world, data->current_stage);
    printf("Lives: %u, Continues: %u\n", data->current_lives, data->current_continues);
    printf("Score: %d, High Score: %d\n", data->current_score, data->high_score_story);
    printf("Total bananas: %u\n", data->total_bananas);
    printf("Play count: %u, Deaths: %u\n", data->total_play_count, data->deaths);
    printf("Volumes: SND=%u MUS=%u\n", data->sound_volume, data->music_volume);
    printf("========================\n\n");
}

/* Memory card emulation */
bool memcard_mount(int slot, MEMCardStatus* out_status) {
    if (!out_status) return false;

    out_status->mounted = save_slot_exists(slot);
    if (out_status->mounted) {
        SMB2SaveData data;
        if (save_slot_read(slot, &data)) {
            out_status->free_blocks = SMB2_SAVE_BLOCKS;
            snprintf(out_status->volume_label, sizeof(out_status->volume_label),
                     "SMB2_SAVE_%c", 'A' + slot);
        } else {
            out_status->free_blocks = 0;
            strcpy(out_status->volume_label, "CORRUPT");
        }
    } else {
        out_status->free_blocks = SMB2_SAVE_BLOCKS;
        strcpy(out_status->volume_label, "EMPTY");
    }
    return true;
}

bool memcard_unmount(int slot) {
    (void)slot;
    return true;
}

bool memcard_format(int slot) {
    SMB2SaveData data;
    return save_slot_format(slot, &data);
}

bool memcard_get_status(int slot, MEMCardStatus* out_status) {
    return memcard_mount(slot, out_status);
}