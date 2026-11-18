/* Curated, layout-independent hash of the simulation state (E3, E1 item 10).
 * Compiled with the decomp headers, so fields are read by name: the result is
 * the same on i686 and x64 (struct sizes differ, field values do not).  Only
 * non-pointer fields; floats by bit pattern.  Regions are hashed separately so
 * a desync can be narrowed down (checksum_latest_regions). */
#include "netplay/checksum.h"
#include "types.h"
#include "ball.h"
#include "mode.h"
#include "stage.h"
#include "input.h"
#include "world.h"
#include "camera.h"
#include "race2.h"
#include "boat.h"
#include "pilot2.h"

extern uint32_t pc_rand_state(void);
extern u32 lbl_14_data_1340;               /* boat RNG state (boat_5.c) */
extern u8 unlock_info[0x84];               /* struct UnlockInfo, same layout on both arches */

static const char *const k_names[CHECKSUM_REGIONS] = {
    "core", "rng", "balls", "pads", "world/camera", "race2", "boat", "pilot2/unlock"
};
const char *pc_state_hash_region_name(int i) { return i >= 0 && i < CHECKSUM_REGIONS ? k_names[i] : "?"; }

static void h_vec(PcHash *h, const Vec *v) { pch_f32(h, v->x); pch_f32(h, v->y); pch_f32(h, v->z); }

static void h_ball(PcHash *h, const struct Ball *b)
{
    int i;
    pch_u32(h, (uint8_t)b->status | (uint32_t)(uint8_t)b->unk1 << 8 | (uint32_t)(uint8_t)b->unk2 << 16 |
               (uint32_t)(uint8_t)b->state << 24);
    h_vec(h, &b->pos); h_vec(h, &b->prevPos); h_vec(h, &b->vel);
    pch_u32(h, (uint16_t)b->rotX | (uint32_t)(uint16_t)b->rotY << 16);
    pch_u32(h, (uint16_t)b->rotZ | (uint32_t)(uint8_t)b->playerId << 16 | (uint32_t)(uint8_t)b->rank << 24);
    pch_f32(h, b->currRadius); pch_f32(h, b->accel); pch_f32(h, b->restitution);
    pch_s32(h, b->bananas); pch_s32(h, b->score); pch_s32(h, b->unk80);
    h_vec(h, &b->unk84);
    pch_u32(h, (uint16_t)b->unk90 | (uint32_t)(uint16_t)b->unk92 << 16);
    pch_u32(h, b->unk94);
    pch_u32(h, (uint32_t)b->flags); pch_u32(h, (uint32_t)(b->flags >> 32));
    pch_u32(h, (uint32_t)b->prevFlags); pch_u32(h, (uint32_t)(b->prevFlags >> 32));
    for (i = 0; i < 12; i++) pch_f32(h, ((const float *)&b->unk30)[i]);   /* Mtx */
    pch_f32(h, b->speed);
    h_vec(h, &b->unk11C);
    pch_u32(h, (uint16_t)b->winStreak | (uint32_t)(uint16_t)b->goalTime << 16);
    pch_s32(h, b->bonus);
    pch_f32(h, b->unk13C);
}

uint64_t pc_state_hash(uint64_t regions[CHECKSUM_REGIONS])
{
    PcHash all, h;
    int i, j;
    uint64_t sub[CHECKSUM_REGIONS];

    /* 0 core: mode machine, frame counters, mode control */
    pch_init(&h);
    pch_s32(&h, main_mode); pch_s32(&h, sub_mode);
    pch_s32(&h, main_mode_request); pch_s32(&h, sub_mode_request);
    pch_u32(&h, frame_counter);
    pch_s32(&h, g_some_frame_counter.frameCounter); pch_f32(&h, g_some_frame_counter.timer);
    pch_u32(&h, g_some_frame_counter.unkC | g_some_frame_counter.unkD << 8);
    pch_u32(&h, g_some_frame_counter.flags);
    pch_s32(&h, modeCtrl.submodeTimer); pch_s32(&h, modeCtrl.difficulty); pch_u32(&h, modeCtrl.courseFlags);
    pch_s32(&h, modeCtrl.unk10); pch_s32(&h, modeCtrl.unk14); pch_s32(&h, modeCtrl.unk18); pch_s32(&h, modeCtrl.unk1C);
    pch_s32(&h, modeCtrl.menuSel); pch_s32(&h, modeCtrl.playerCount); pch_s32(&h, modeCtrl.gameType);
    pch_s32(&h, modeCtrl.currPlayer); pch_s32(&h, modeCtrl.unk30); pch_s32(&h, modeCtrl.unk3C);
    pch_u32(&h, (uint16_t)modeCtrl.unk40 | (uint32_t)modeCtrl.splitscreenMode << 16 | (uint32_t)modeCtrl.unk43 << 24);
    pch_s32(&h, modeCtrl.unk44); pch_s32(&h, modeCtrl.unk48); pch_u32(&h, modeCtrl.wideMode);
    pch_s32(&h, current_stage_id); pch_s32(&h, world_theme);
    sub[0] = pch_final(&h);

    /* 1 rng */
    pch_init(&h);
    pch_u32(&h, pc_rand_state());
    pch_u32(&h, lbl_14_data_1340);
    sub[1] = pch_final(&h);

    /* 2 balls: all four, field-wise (struct Ball is 0x1B0 / 0x1B8) */
    pch_init(&h);
    for (i = 0; i < 4; i++) h_ball(&h, &balls[i]);
    sub[2] = pch_final(&h);

    /* 3 pads as the game sees them (also catches pad-hook bugs) */
    pch_init(&h);
    for (i = 0; i < 4; i++)
        for (j = 0; j < 5; j++) {
            const PADStatus *p = &pad_status_groups[i].unk0[j];
            pch_u32(&h, p->button | (uint32_t)(uint8_t)p->stickX << 16 | (uint32_t)(uint8_t)p->stickY << 24);
            pch_u32(&h, (uint8_t)p->substickX | (uint32_t)(uint8_t)p->substickY << 8 |
                        (uint32_t)p->triggerLeft << 16 | (uint32_t)p->triggerRight << 24);
            pch_u32(&h, (uint8_t)p->err);
        }
    for (i = 0; i < 4; i++) pch_s32(&h, player_pad_map[i]);
    for (i = 0; i < 4; i++) pch_u32(&h, merged_digital_inputs[i]);
    for (i = 0; i < 4; i++) pch_s32(&h, frames_since_last_input_change[i]);
    sub[3] = pch_final(&h);

    /* 4 stage tilt (world_infos) and the cameras: the camera yaw maps the stick
     * to a tilt direction, so it is sim-relevant */
    pch_init(&h);
    for (i = 0; i < 4; i++) {
        const struct World *w = &world_infos[i];
        pch_u32(&h, (uint16_t)w->xrot | (uint32_t)(uint16_t)w->zrot << 16);
        pch_u32(&h, (uint16_t)w->xrotPrev | (uint32_t)(uint16_t)w->zrotPrev << 16);
        pch_u32(&h, (uint8_t)w->state | (uint32_t)(uint8_t)w->playerId << 8 | (uint32_t)(uint16_t)w->unk1C << 16);
        pch_f32(&h, w->maxTilt); h_vec(&h, &w->unk10); h_vec(&h, &w->unk20);
    }
    h_vec(&h, &g_gravity_dir);
    for (i = 0; i < 5; i++) {
        const struct Camera *c = &cameras[i];
        h_vec(&h, &c->eye); h_vec(&h, &c->lookAt);
        pch_u32(&h, (uint16_t)c->rotX | (uint32_t)(uint16_t)c->rotY << 16);
        pch_u32(&h, (uint16_t)c->rotZ | (uint32_t)(uint8_t)c->mode << 16);
        pch_u32(&h, (uint16_t)c->flags | (uint32_t)(uint16_t)c->timerCurr << 16);
    }
    sub[4] = pch_final(&h);

    /* 5 Monkey Race 2 players (pointer-free, same size on both arches) */
    pch_init(&h);
    pch_bytes(&h, lbl_10_bss_1230, sizeof lbl_10_bss_1230);
    sub[5] = pch_final(&h);

    /* 6 Monkey Boat players: the pointer-free fields that carry the race state */
    pch_init(&h);
    for (i = 0; i < 8; i++) {
        const struct BoatPlayer *b = &lbl_14_bss_208[i];
        pch_u32(&h, b->unk0 | b->unk1 << 8 | b->unk2 << 16 | b->unk3 << 24);
        pch_u32(&h, b->unk4 | b->unk5 << 8 | b->unk6 << 16 | b->unk7 << 24);
        pch_u32(&h, b->unk8); pch_u32(&h, b->unkC); pch_s32(&h, b->unk10);
        pch_u32(&h, b->unk14 | b->unk15 << 8 | b->unk16 << 16 | b->unk17 << 24);
        pch_u32(&h, b->unk18 | b->unk19 << 8 | b->unk1A << 16 | b->unk1B << 24);
        pch_u32(&h, b->unk1C | b->unk1D << 8);
        for (j = 0; j < 4; j++) pch_u32(&h, (uint16_t)b->unk1E[j] | (uint32_t)(uint16_t)b->unk26[j] << 16);
        pch_f32(&h, b->unk30); pch_f32(&h, b->unk34);
        pch_u32(&h, (uint16_t)b->unk38 | (uint32_t)(uint16_t)b->unk3A << 16);
        pch_s32(&h, b->unk3C); pch_s32(&h, b->unk40); pch_f32(&h, b->unk44); pch_f32(&h, b->unk4C);
        pch_u32(&h, b->unk2D8);
        pch_f32(&h, b->unk2DC); pch_f32(&h, b->unk2E0); h_vec(&h, &b->unk2E4); h_vec(&h, &b->unk2F0);
        pch_f32(&h, b->unk2FC); pch_f32(&h, b->unk300); pch_f32(&h, b->unk304);
        pch_f32(&h, b->unk308); pch_f32(&h, b->unk30C);
        pch_f32(&h, b->unk314); pch_f32(&h, b->unk318); h_vec(&h, &b->unk31C); h_vec(&h, &b->unk328);
        pch_f32(&h, b->unk338); pch_f32(&h, b->unk33C); h_vec(&h, &b->unk340); h_vec(&h, &b->unk34C);
        pch_s32(&h, b->unk358); pch_f32(&h, b->unk35C);
        h_vec(&h, &b->unk390); h_vec(&h, &b->unk39C);
        pch_u32(&h, b->unk3EC);
    }
    sub[6] = pch_final(&h);

    /* 7 Monkey Target 2 players + unlock info */
    pch_init(&h);
    pch_bytes(&h, lbl_13_bss_70, 5 * sizeof(struct Pilot2Player));
    pch_bytes(&h, unlock_info, sizeof unlock_info);
    sub[7] = pch_final(&h);

    pch_init(&all);
    for (i = 0; i < CHECKSUM_REGIONS; i++) {
        pch_u32(&all, (uint32_t)sub[i]);
        pch_u32(&all, (uint32_t)(sub[i] >> 32));
    }
    if (regions) memcpy(regions, sub, sizeof sub);
    return pch_final(&all);
}
