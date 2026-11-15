/* Byte-swap helpers for big-endian disc data, called from guarded
 * (#ifndef __MWERKS__) hook points in the decomp loaders right after a file
 * is loaded.  See docs/endian.md. */
#include "shim.h"
#include <stdio.h>
#include <string.h>

#define BSWAP32(p) (*(u32 *)(p) = __builtin_bswap32(*(u32 *)(p)))
#define BSWAP16(p) (*(u16 *)(p) = __builtin_bswap16(*(u16 *)(p)))

/* SMB2's TPL variant: u32 numTextures, then numTextures * 0x10 entries of
 * { u32 format; u32 imageOffset; u16 width; u16 height; u16 unkC; }. */
void pc_swap_tpl(void *fileData)
{
    u8 *p = (u8 *)fileData;
    u32 n, i;

    BSWAP32(p);
    n = *(u32 *)p;
    if (n > 4096) {
        fprintf(stderr, "pc_swap_tpl: suspicious texture count %u\n", n);
        return;
    }
    for (i = 0; i < n; i++) {
        u8 *e = p + 4 + i * 0x10;
        BSWAP32(e + 0x0);  /* format */
        BSWAP32(e + 0x4);  /* imageOffset */
        BSWAP16(e + 0x8);  /* width */
        BSWAP16(e + 0xA);  /* height */
        BSWAP16(e + 0xC);  /* unkC */
    }
}

/* ---- NaomiLib (.lz nlobj) models: nl2ngc.c ----
 *
 * Layout (every field is a 4-byte word, so each region swaps as u32s):
 *   NlObj    { u32 unkTblOff; u32 modelOffs[];  NULL-terminated }
 *   model    two u32 offsets at off-8/off-4, then NlModel
 *            { s32 u_valid; s32 flags; Vec center; f32 radius; meshes... }
 *   NlMesh   0x50-byte header (last word = dispListSize), then
 *            dispListSize bytes of display-list words; flags==0 ends list
 *   unk tbl  u32 childOffs[] NULL-terminated; each child { u32 off; u32
 *            childStructsOff } -> array of { u32 unk0; u32 unk4 }, unk0==0 ends
 *
 * Called before convert_nlobj_offsets_to_pointers(), which then reads the
 * (now host-endian) offsets and turns them into pointers. */

static u32 sw32(void *p)
{
    u32 v = __builtin_bswap32(*(u32 *)p);
    *(u32 *)p = v;
    return v;
}

static void nlobj_swap_model(u8 *base, u32 off)
{
    u32 u_valid;
    u8 *mesh;
    u32 j;

    sw32(base + off - 8);
    sw32(base + off - 4);
    u_valid = sw32(base + off + 0x00);  /* u_valid */
    sw32(base + off + 0x04);            /* flags */
    sw32(base + off + 0x08);            /* center.x */
    sw32(base + off + 0x0C);            /* center.y */
    sw32(base + off + 0x10);            /* center.z */
    sw32(base + off + 0x14);            /* radius */
    if (u_valid == 0xFFFFFFFFu)
        return;

    mesh = base + off + 0x18;
    for (;;) {
        u32 dispListSize;
        if (sw32(mesh) == 0)            /* mesh flags: 0 terminates */
            break;
        for (j = 4; j < 0x50; j += 4)
            sw32(mesh + j);
        dispListSize = *(u32 *)(mesh + 0x4C);
        if (dispListSize > (16u << 20)) {
            fprintf(stderr, "pc_swap_nlobj: suspicious dispListSize %u\n",
                    dispListSize);
            return;
        }
        for (j = 0; j < dispListSize; j += 4)
            sw32(mesh + 0x50 + j);
        mesh += 0x50 + dispListSize;
    }
}

void pc_swap_nlobj(void *data)
{
    u8 *base = (u8 *)data;
    u32 unkTblOff, off;
    u8 *p;

    unkTblOff = sw32(base);
    for (p = base + 4; (off = sw32(p)) != 0; p += 4)
        nlobj_swap_model(base, off);

    if (unkTblOff != 0) {
        for (p = base + unkTblOff; (off = sw32(p)) != 0; p += 4) {
            u8 *child = base + off;
            u32 csOff;
            u8 *cs;
            sw32(child);
            csOff = sw32(child + 4);
            for (cs = base + csOff; sw32(cs) != 0; cs += 8)
                sw32(cs + 4);
        }
    }
}

/* ---- ape skeleton/motion archives (skeleton.c) ----
 *
 * Format: u32 count, then count+1 offsets, then payload.  The game
 * relocates the offsets in place and sets the count's top bit as an
 * "already relocated" flag, so this must run before relocation and be
 * idempotent across repeated calls on the same archive.
 *
 * Once relocated, the top bit is set (host-endian).  An unswapped BE count
 * for these small tables looks huge when read little-endian, so "too big,
 * but plausible after bswap" means BE.
 *
 * Only the offset table is swapped here; the entry payloads have their own
 * swaps below. */
void pc_swap_ape_archive(void *archive)
{
    u32 *p = (u32 *)archive;
    u32 count = p[0];
    u32 swapped = __builtin_bswap32(count);
    u32 i;

    if (count & 0x80000000)         /* already relocated */
        return;
    if (count <= 0x10000)           /* already host-endian */
        return;
    if (swapped > 0x10000) {
        fprintf(stderr, "pc_swap_ape_archive: suspicious count %08x\n", count);
        return;
    }
    for (i = 0; i < swapped + 2; i++)   /* count word + count+1 offsets */
        BSWAP32(p + i);
}

/* ---- ape payload swaps (new_ape.c / skeleton.c / ape_anim.c) ----
 *
 * Three payload layouts share the pc_swap_ape_archive container:
 *   .mta   entries are MtaClips (keyframe curves per bone)
 *   .ska   entries are skeleton bone blobs (init_component_state format)
 *   .skl   a single bone blob, no container
 * plus the standalone .mal clip-catalog files.  Each swap runs exactly
 * once, at the load site, before the game parses or relocates the data. */

/* One init_component_state bone blob: u16 boneCount @0, bones @+4, each
 * { u16 parent; u16 flags; f32 quatA[4]; f32 quatB[4]; f32 pos[3];
 *   char name[]; } with the name padded so the record ends 4-aligned.
 * Returns the end pointer. */
static u8 *swap_skl_bones(u8 *data)
{
    u8 *p;
    u32 i, j, count;

    BSWAP16(data);
    count = *(u16 *)data;
    p = data + 4;
    for (i = 0; i < count; i++) {
        u32 len;
        BSWAP16(p); BSWAP16(p + 2);
        p += 4;
        for (j = 0; j < 11; j++)        /* 2 quats + pos */
            BSWAP32(p + j * 4);
        p += 44;
        len = strlen((char *)p);        /* name: bytes, no swap */
        len += 3 - (len & 3);
        p += len + 1;
    }
    return p;
}

/* A standalone .skl slot file (face/hand/tail skeletons). */
void pc_swap_skl(void *data)
{
    swap_skl_bones(data);
}

/* skel.ska: archive whose entries are bone blobs.  Must run before
 * ape_archive_offsets_to_pointers relocates the (swapped) offsets. */
void pc_swap_ska(void *archive)
{
    u32 *p = (u32 *)archive;
    u32 count, i;

    pc_swap_ape_archive(archive);
    count = p[0];
    if (count & 0x80000000 || count > 0x10000)
        return;                         /* relocated already or bogus */
    /* p[1] = names offset, p[2..count+1] = entry offsets */
    for (i = 0; i < count; i++)
        swap_skl_bones((u8 *)archive + p[2 + i]);
}

/* .mta motion archive: entries are MtaClips --
 *   { u16 length; u16 boneCount; } then per bone
 *   { u16 chanFlags; u16 pad; } and for each of 6 channel-flag bits set:
 *   { u16 keyCount; u16 pad; } + keyCount * BoneKey
 *   { u16 type; u16 frame; f32 value, tanIn, tanOut; }  (0x10 bytes). */
void pc_swap_mta(void *archive)
{
    u32 *hdr = (u32 *)archive;
    u32 count, i;

    pc_swap_ape_archive(archive);
    count = hdr[0];
    if (count & 0x80000000 || count > 0x10000)
        return;
    for (i = 0; i < count; i++) {
        u8 *p = (u8 *)archive + hdr[2 + i];
        u32 bones, b;

        BSWAP16(p); BSWAP16(p + 2);
        bones = *(u16 *)(p + 2);
        p += 4;
        for (b = 0; b < bones; b++) {
            u32 chanFlags, c;

            BSWAP16(p); BSWAP16(p + 2);
            chanFlags = *(u16 *)p;
            p += 4;
            /* 9 channels: g_init_skeleton_animation walks three 3-channel
             * groups (keyCount[0..2], [6..8], [12..14]) off 9 flag bits */
            for (c = 0; c < 9; c++, chanFlags >>= 1) {
                u32 keys, k;

                if (!(chanFlags & 1))
                    continue;
                BSWAP16(p); BSWAP16(p + 2);
                keys = *(u16 *)p;
                p += 4;
                for (k = 0; k < keys; k++, p += 0x10) {
                    BSWAP16(p); BSWAP16(p + 2);
                    BSWAP32(p + 4); BSWAP32(p + 8); BSWAP32(p + 12);
                }
            }
        }
    }
}

/* .mal clip catalog (struct ApeMal, ape_internal.h): header, then clip,
 * track, event and group arrays at header offsets; names are strings. */
void pc_swap_mal(void *data)
{
    u8 *d = (u8 *)data;
    u32 clipCount, groupCount, boneCount, eventCount, i;

    BSWAP32(d + 0x00);                  /* f32 version */
    BSWAP16(d + 0x04);                  /* clipCount */
    BSWAP32(d + 0x08);                  /* clipsOff */
    BSWAP32(d + 0x0C);                  /* framesOff */
    BSWAP16(d + 0x10);                  /* eventCount */
    BSWAP32(d + 0x14);                  /* eventsOff */
    BSWAP32(d + 0x18);                  /* unk18 */
    BSWAP32(d + 0x1C);                  /* namesOff */
    BSWAP16(d + 0x22);                  /* flags */
    BSWAP32(d + 0x24);                  /* groupsOff */
    clipCount  = *(u16 *)(d + 0x04);
    groupCount = *(u8 *)(d + 0x06);
    eventCount = *(u16 *)(d + 0x10);
    boneCount  = *(u8 *)(d + 0x21);
    if (clipCount > 0x4000 || boneCount > 0x100) {
        fprintf(stderr, "pc_swap_mal: suspicious counts %u/%u\n",
                clipCount, boneCount);
        return;
    }
    for (i = 0; i < clipCount; i++) {   /* struct ApeMalClip, 0x24 each */
        u8 *c = d + *(u32 *)(d + 0x08) + i * 0x24;
        BSWAP16(c + 0x00);
        BSWAP16(c + 0x06); BSWAP16(c + 0x08); BSWAP16(c + 0x0A);
        BSWAP16(c + 0x0C); BSWAP16(c + 0x0E);
        BSWAP32(c + 0x10); BSWAP32(c + 0x14);   /* f32, f32 */
        BSWAP32(c + 0x18); BSWAP32(c + 0x1C); BSWAP32(c + 0x20);
    }
    for (i = 0; i < clipCount * boneCount; i++) /* ApeTrack: u8,u8,u16 */
        BSWAP16(d + *(u32 *)(d + 0x0C) + i * 4 + 2);
    for (i = 0; i < eventCount; i++) {  /* struct ApeEvent, 0x10 each */
        u8 *e = d + *(u32 *)(d + 0x14) + i * 0x10;
        BSWAP16(e + 0x00); BSWAP16(e + 0x02); BSWAP16(e + 0x04);
        BSWAP32(e + 0x08);
        BSWAP16(e + 0x0C); BSWAP16(e + 0x0E);
    }
    for (i = 0; i < groupCount; i++) {  /* struct ApeMalGroup, 8 each */
        u8 *g = d + *(u32 *)(d + 0x24) + i * 8;
        BSWAP16(g + 0x00); BSWAP16(g + 0x02); BSWAP32(g + 0x04);
    }
}

/* ---- stagedef (STAGE%03d.lz): stage_load.c ----
 *
 * Mirrors load_stagedef()'s offset-fixup traversal exactly: every 32-bit
 * offset/count the fixup pass reads, plus all scalar payload fields the
 * game reads later, is byte-swapped in place BEFORE the fixup pass runs
 * (which is why every "pointer" field here is still a file offset that we
 * follow manually from the buffer base).  String bytes (model names) are
 * left alone; only the offset words pointing at them are swapped.
 *
 * Several per-item-group arrays (goals, instances, model refs, grid cell
 * triangle index lists, ...) can alias / be sub-ranges of the stagedef's
 * global arrays, so every primitive swap is guarded by an "already
 * swapped" bitmap (1 bit per 2-byte unit), which makes the whole
 * traversal idempotent per unit and immune to shared data. */

#include "stage.h"

#define SD_WINDOW (8u << 20)      /* max plausible stagedef size */
#define SD_MAX_COUNT 100000

static u8 sd_done[SD_WINDOW / 2 / 8];   /* 1 bit per 2-byte unit */
static u8 *sd_base;
static int sd_abs;   /* offset fields hold absolute pointers (embedded data) */
static int sd_warned_off;

static int sd_mark(uptr off)
{
    u32 unit;

    if (off >= SD_WINDOW || (off & 1)) {
        if (!sd_warned_off) {
            fprintf(stderr, "pc_swap_stagedef: bad offset 0x%llx, skipping\n", (unsigned long long)off);
            sd_warned_off = 1;
        }
        return 0;
    }
    unit = off >> 1;
    if (sd_done[unit >> 3] & (1u << (unit & 7)))
        return 0;
    sd_done[unit >> 3] |= (u8)(1u << (unit & 7));
    return 1;
}

static void sdw32(void *p)
{
    if (sd_mark((uptr)((u8 *)p - sd_base)))
        BSWAP32(p);
}

static void sdw16(void *p)
{
    if (sd_mark((uptr)((u8 *)p - sd_base)))
        BSWAP16(p);
}

static void sdw32n(void *p, s32 n)
{
    u8 *q = p;
    while (n-- > 0) { sdw32(q); q += 4; }
}

static void sdw16n(void *p, s32 n)
{
    u8 *q = p;
    while (n-- > 0) { sdw16(q); q += 2; }
}

/* Read an already-swapped offset field; NULL for 0 or implausible. */
static void *sd_ptr(void *field)
{
    u32 off = *(u32 *)field;

    if (off == 0)
        return NULL;
    if (sd_abs) {
        if ((uptr)off - (uptr)sd_base >= SD_WINDOW || (off & 1)) {
            fprintf(stderr, "pc_swap_stagedef: implausible pointer 0x%x, skipping\n", off);
            return NULL;
        }
        return (void *)(uptr)off;
    }
    if (off >= SD_WINDOW || (off & 1)) {
        fprintf(stderr, "pc_swap_stagedef: implausible offset 0x%x, skipping\n", off);
        return NULL;
    }
    return sd_base + off;
}

static s32 sd_count(s32 n, const char *what)
{
    if (n < 0 || n > SD_MAX_COUNT) {
        fprintf(stderr, "pc_swap_stagedef: suspicious %s count %d, skipping\n", what, (int)n);
        return 0;
    }
    return n;
}

static void sd_keyframes(struct StageKeyframeChannel *ch, const char *what)
{
    struct StageKeyframe *k = sd_ptr(&ch->frames);
    s32 n = sd_count(ch->count, what);
    s32 i;

    for (i = 0; k != NULL && i < n; i++)
        sdw32n(&k[i], 5);   /* easeType, time, value, tangentIn, tangentOut */
}

static void sd_itemgroup_anim(void *field)
{
    struct StageItemGroupAnim *a = sd_ptr(field);
    struct StageKeyframeChannel *ch;
    s32 i;

    if (a == NULL)
        return;
    sdw32n(a, 12);   /* 6 { count, frames } pairs */
    ch = (struct StageKeyframeChannel *)a;
    for (i = 0; i < 6; i++)
        sd_keyframes(&ch[i], "item group anim keyframe");
}

static void sd_bg_anim(void *field)
{
    struct StageBgAnim *a = sd_ptr(field);
    struct StageKeyframeChannel *ch;
    s32 i;

    if (a == NULL)
        return;
    sdw32n(a, 24);   /* loopStart, loopEnd, 11 channels */
    ch = &a->scaleX;
    for (i = 0; i < 11; i++)
        sd_keyframes(&ch[i], "bg anim keyframe");
}

static void sd_texture_scroll(void *field)
{
    float *ts = sd_ptr(field);

    if (ts != NULL)
        sdw32n(ts, 2);   /* scroll speed x, y */
}

static void sd_effect(void *field)
{
    struct StageEffectHeader *e = sd_ptr(field);
    struct StageNightWindowAnim *nw;
    struct StageStormFireAnim *sf;
    s32 i, n;

    if (e == NULL)
        return;
    sdw32n(e, 5);
    nw = sd_ptr(&e->fx1Keyframes);
    n = sd_count(e->nightWindowAnimCount, "night window anim");
    for (i = 0; nw != NULL && i < n; i++) {
        sdw32n(&nw[i].pos, 3);
        sdw16n(&nw[i].rotX, 3);   /* id is a byte */
    }
    sf = sd_ptr(&e->fx2Keyframes);
    n = sd_count(e->stormFireAnimCount, "storm fire anim");
    for (i = 0; sf != NULL && i < n; i++)
        sdw32n(&sf[i].pos, 3);    /* frameOffset is a byte */
    sd_texture_scroll(&e->textureScroll);
}

static void sd_goal_array(void *field, s32 count)
{
    struct StageGoal *g = sd_ptr(field);
    s32 i, n = sd_count(count, "goal");

    for (i = 0; g != NULL && i < n; i++) {
        sdw32n(&g[i].pos, 3);
        sdw16n(&g[i].rotX, 3);   /* type/flags are bytes */
    }
}

/* bumpers and jamabars share { Vec pos; s16 rot[3]; pad; Vec scale } (0x20,
 * struct StageBumper / StageJamabar in stobj.c) */
static void sd_bumper_array(void *field, s32 count, const char *what)
{
    u8 *e = sd_ptr(field);
    s32 i, n = sd_count(count, what);

    for (i = 0; e != NULL && i < n; i++, e += 0x20) {
        sdw32n(e + 0x00, 3);   /* pos */
        sdw16n(e + 0x0C, 3);   /* rot */
        sdw32n(e + 0x14, 3);   /* scale */
    }
}

/* struct StageBanana (item.h): { Vec pos; s32 type } */
static void sd_banana_array(void *field, s32 count)
{
    u8 *e = sd_ptr(field);
    s32 i, n = sd_count(count, "banana");

    for (i = 0; e != NULL && i < n; i++, e += 0x10)
        sdw32n(e, 4);
}

static void sd_cone_array(void *field, s32 count)
{
    struct StageColiCone *c = sd_ptr(field);
    s32 i, n = sd_count(count, "coli cone");

    for (i = 0; c != NULL && i < n; i++) {
        sdw32n(&c[i].pos, 3);
        sdw16n(&c[i].rot, 4);   /* rot xyz + flags */
        sdw32n(&c[i].scale, 3);
    }
}

static void sd_sphere_array(void *field, s32 count)
{
    struct StageColiSphere *s = sd_ptr(field);
    s32 i, n = sd_count(count, "coli sphere");

    for (i = 0; s != NULL && i < n; i++) {
        sdw32n(&s[i].pos, 4);   /* pos + radius */
        sdw16(&s[i].flags);
    }
}

static void sd_cylinder_array(void *field, s32 count)
{
    struct StageColiCylinder *c = sd_ptr(field);
    s32 i, n = sd_count(count, "coli cylinder");

    for (i = 0; c != NULL && i < n; i++) {
        sdw32n(&c[i].pos, 5);   /* pos, radius, height */
        sdw16n(&c[i].rot, 4);   /* rot xyz + flags */
    }
}

static void sd_fallout_array(void *field, s32 count)
{
    struct FalloutVolume *f = sd_ptr(field);
    s32 i, n = sd_count(count, "fallout volume");

    for (i = 0; f != NULL && i < n; i++) {
        sdw32n(&f[i].pos, 6);   /* pos + scale */
        sdw16n(&f[i].rotX, 3);
    }
}

static void sd_reflective_array(void *field, s32 count)
{
    struct StageReflectiveModel *r = sd_ptr(field);
    s32 i, n = sd_count(count, "reflective");

    for (i = 0; r != NULL && i < n; i++)
        sdw32n(&r[i], 2);   /* modelName offset, model; unk8 left alone */
}

static void sd_instance_array(void *field, s32 count)
{
    struct StageModelInstance *in = sd_ptr(field);
    s32 i, n = sd_count(count, "instance");

    for (i = 0; in != NULL && i < n; i++) {
        sdw32n(&in[i].modelRef, 4);   /* modelRef offset + pos */
        sdw16n(&in[i].rotX, 4);       /* rot xyz + flags */
        sdw32n(&in[i].scale, 3);
    }
}

/* struct StageButton (stobj.c): { Vec pos; s16 rot[3]; s16 type; s16 animGroupId } */
static void sd_button_array(void *field, s32 count)
{
    u8 *e = sd_ptr(field);
    s32 i, n = sd_count(count, "button");

    for (i = 0; e != NULL && i < n; i++, e += 0x18) {
        sdw32n(e + 0x00, 3);
        sdw16n(e + 0x0C, 5);
    }
}

static void sd_wormhole_array(void *field, s32 count)
{
    struct StageWormhole *w = sd_ptr(field);
    s32 i, n = sd_count(count, "wormhole");

    for (i = 0; w != NULL && i < n; i++) {
        sdw32n(&w[i].unk0, 4);   /* unk0 + pos */
        sdw16n(&w[i].rotX, 3);
        sdw32(&w[i].dest);       /* offset to another wormhole */
    }
}

static void sd_generator_array(void *field, s32 count)
{
    struct StageGenerator *g = sd_ptr(field);
    s32 i, n = sd_count(count, "generator");
    s32 j;

    for (i = 0; g != NULL && i < n; i++) {
        struct StageKeyframeChannel *ch = &g[i].scaleX;

        sdw32n(&g[i].name, 6);   /* name offset, loopStart, loopEnd, pos */
        sdw16n(&g[i].rotX, 3);
        sdw32n(ch, 16);          /* 8 { count, frames } channels */
        for (j = 0; j < 8; j++)
            sd_keyframes(&ch[j], "generator keyframe");
        /* unk60 left alone */
    }
}

static void sd_modelref_ptr_array(void *field, s32 count)
{
    void **p = sd_ptr(field);
    s32 n = sd_count(count, "model ref");

    if (p != NULL)
        sdw32n(p, n);   /* offsets to StageModelRefs inside modelList */
}

static void sd_bound_sphere(void *field)
{
    struct Sphere *s = sd_ptr(field);

    if (s != NULL)
        sdw32n(s, 4);
}

/* The triangle array has no explicit count: it is indexed by the grid cell
 * lists (s16 indices, terminated by a negative index), so swap the cells
 * first and swap triangles 0..maxIndex. */
static void sd_grid_and_triangles(struct StageCollHeader *coll)
{
    u32 *cells;   /* per-cell offset words (s16 * DPTR in StageCollHeader;
                   * a native s16 ** would stride 8 bytes on x64) */
    struct StageColiTri *tris;
    s32 nx, nz, ncells, i;
    s32 maxIdx = -1;

    cells = sd_ptr(&coll->gridCellTris);
    tris = sd_ptr(&coll->triangles);
    if (cells == NULL) {
        if (tris != NULL)
            fprintf(stderr, "pc_swap_stagedef: triangles without a grid, not swapped\n");
        return;
    }
    nx = coll->gridCellCountX;
    nz = coll->gridCellCountZ;
    if (nx < 0 || nz < 0 || nx > 4096 || nz > 4096 || nx * nz > (1 << 20)) {
        fprintf(stderr, "pc_swap_stagedef: suspicious grid %dx%d, skipping\n", (int)nx, (int)nz);
        return;
    }
    ncells = nx * nz;
    sdw32n(cells, ncells);   /* the per-cell offset table */
    for (i = 0; i < ncells; i++) {
        s16 *p = sd_ptr(&cells[i]);
        u32 steps = 0;

        if (p == NULL)
            continue;
        for (;;) {
            sdw16(p);
            if (*p < 0)
                break;
            if (*p > maxIdx)
                maxIdx = *p;
            p++;
            if (++steps > 1000000u) {
                fprintf(stderr, "pc_swap_stagedef: unterminated grid cell list\n");
                break;
            }
        }
    }
    if (tris != NULL && maxIdx >= 0) {
        if (maxIdx >= SD_MAX_COUNT) {
            fprintf(stderr, "pc_swap_stagedef: suspicious triangle index %d, skipping\n", (int)maxIdx);
            return;
        }
        for (i = 0; i <= maxIdx; i++) {
            sdw32n(&tris[i].pos, 6);     /* pos + normal */
            sdw16n(&tris[i].rot, 4);     /* rot xyz + flags */
            sdw32n(&tris[i].vert2, 8);   /* vert2/vert3 + edge2/edge3 normals */
        }
    }
}

static void sd_coll_header(struct StageCollHeader *c)
{
    /* header words/halves (unkA6/unkA7 are bytes; unkB0 and the 0xDC..0x49B
     * tail are unknown and left untouched) */
    sdw32n(&c->initPos, 3);
    sdw16n(&c->initRot, 4);       /* initRot xyz + animLoopType */
    sdw32(&c->anim);
    sdw32n(&c->conveyorVel, 3);
    sdw32n(&c->triangles, 2);     /* triangles, gridCellTris */
    sdw32n(&c->gridOriginX, 6);   /* grid origin/step/cell counts */
    sdw32n(&c->goalCount, 24);    /* 0x44..0xA0: count/offset pairs */
    sdw16(&c->animGroupId);
    sdw32n(&c->buttonCount, 2);
    sdw32n(&c->boundSphere, 10);  /* 0xB4..0xD8 */

    /* payloads (may alias the stagedef's global arrays; bitmap-guarded) */
    sd_itemgroup_anim(&c->anim);
    sd_grid_and_triangles(c);
    sd_goal_array(&c->goals, c->goalCount);
    sd_bumper_array(&c->bumpers, c->bumperCount, "bumper");
    sd_bumper_array(&c->jamabars, c->jamabarCount, "jamabar");
    sd_banana_array(&c->bananas, c->bananaCount);
    sd_cone_array(&c->coliCones, c->coliConeCount);
    sd_sphere_array(&c->coliSpheres, c->coliSphereCount);
    sd_cylinder_array(&c->coliCylinders, c->coliCylinderCount);
    sd_fallout_array(&c->falloutVolumes, c->falloutVolumeCount);
    sd_reflective_array(&c->reflectives, c->reflectiveCount);
    sd_instance_array(&c->instances, c->instanceCount);
    sd_modelref_ptr_array(&c->modelRefs, c->modelRefCount);
    sd_button_array(&c->buttons, c->buttonCount);
    sd_bound_sphere(&c->boundSphere);
    sd_wormhole_array(&c->wormholes, c->wormholeCount);
    sd_generator_array(&c->generators, c->generatorCount);
    sd_texture_scroll(&c->textureScroll);
}

static void sd_bg_model_array(void *field, s32 count, const char *what)
{
    struct StageBgModel *bg = sd_ptr(field);
    s32 i, n = sd_count(count, what);

    for (i = 0; bg != NULL && i < n; i++) {
        sdw32n(&bg[i].flags, 3);   /* flags, name offset, model */
        sdw32n(&bg[i].pos, 3);
        sdw16n(&bg[i].rotX, 4);    /* rot xyz + animFlags */
        sdw32n(&bg[i].scale, 4);   /* scale + translucency */
        sdw32n(&bg[i].anim, 2);    /* anim, effect */
        sd_bg_anim(&bg[i].anim);
        sd_effect(&bg[i].effect);
    }
}

static void sd_extra_channels(struct StageExtraChannels *ch)
{
    sd_keyframes(&ch->a, "extra keyframe");
    sd_keyframes(&ch->b, "extra keyframe");
    sd_keyframes(&ch->c, "extra keyframe");
}

static void sd_traverse(struct StageDef *sd);

void pc_swap_stagedef(void *data)
{
    sd_base = data;
    sd_abs = 0;
    sd_warned_off = 0;
    memset(sd_done, 0, sizeof(sd_done));
    sd_traverse(data);
}

/* Stagedefs compiled into a REL as u32 word tables (baseball_stage_data.inc)
 * with their internal pointers already resolved.  Each word holds the
 * big-endian value, so on PC 32-bit fields are already right but 16-bit
 * fields and byte/string data are not.  Byte-swap every word back to the
 * on-disc byte image, then run the normal traversal in absolute-pointer
 * mode, which swaps exactly the 32/16-bit fields (pointer words included)
 * the same way a loaded STAGExxx.lz is swapped. */
void pc_fix_embedded_stagedefs(void *const *arrays, const u32 *sizes, int narrays,
                               void *const *defs, int ndefs)
{
    uptr lo = ~(uptr)0, hi = 0;
    int i;

    for (i = 0; i < narrays; i++) {
        uptr a = (uptr)arrays[i];
        u32 j;
        u32 *w = arrays[i];

        if (a < lo) lo = a;
        if (a + sizes[i] > hi) hi = a + sizes[i];
        for (j = 0; j < sizes[i] / 4; j++)
            BSWAP32(&w[j]);
    }
    if (hi - lo > SD_WINDOW) {
        fprintf(stderr, "pc_fix_embedded_stagedefs: data spans 0x%llx bytes, not fixed\n",
                (unsigned long long)(hi - lo));
        return;
    }
    sd_base = (u8 *)lo;
    sd_abs = 1;
    sd_warned_off = 0;
    memset(sd_done, 0, sizeof(sd_done));
    for (i = 0; i < ndefs; i++)
        sd_traverse(defs[i]);
    sd_abs = 0;
}

static void sd_traverse(struct StageDef *sd)
{
    struct StageCollHeader *coll;
    s32 i, n;

    /* StageDef header: all fields are 32-bit floats/counts/offsets except
     * bgTheme (s8, 0xA4) and the unknown byte region 0xA5.  0x6C is the
     * s32 start position count (1; 9 in Monkey Race 131-140, 4 in Monkey
     * Boat 143-148), read as *(s32 *)unk6C by race2_0.c and boat_2.c. */
    sdw32n(&sd->loopStart, 28);         /* 0x00..0x6C */
    sdw32n(&sd->reflectiveCount, 13);   /* 0x70..0xA0 */
    sdw32n(&sd->buttonCount, 12);       /* 0xA8..0xD4 */

    /* per-item-group collision headers */
    coll = sd_ptr(&sd->collHeaders);
    n = sd_count(sd->itemGroupCount, "item group");
    for (i = 0; coll != NULL && i < n; i++)
        sd_coll_header(&coll[i]);

    /* top-level arrays (several alias per-item-group arrays; the swap
     * bitmap keeps that idempotent) */
    {
        struct StartPos *sp = sd_ptr(&sd->startPos);

        if (sp != NULL) {
            /* Race and Boat stages carry several start positions; the
             * count is the 0x6C word, bounded by the falloutY block that
             * directly follows the array. */
            s32 cnt = *(s32 *)sd->unk6C;
            float *fy = sd_ptr(&sd->falloutY);

            if (cnt < 1 || cnt > 16)
                cnt = 1;
            if (fy != NULL && (u8 *)fy > (u8 *)sp &&
                ((u8 *)fy - (u8 *)sp) / (s32)sizeof(struct StartPos) < cnt)
                cnt = ((u8 *)fy - (u8 *)sp) / (s32)sizeof(struct StartPos);
            if (cnt < 1)
                cnt = 1;
            for (i = 0; i < cnt; i++) {
                sdw32n(&sp[i].pos, 3);
                sdw16n(&sp[i].xrot, 3);
            }
        }
    }
    {
        float *fy = sd_ptr(&sd->falloutY);

        if (fy != NULL)
            sdw32(fy);
    }
    sd_goal_array(&sd->goals, sd->goalCount);
    sd_bumper_array(&sd->bumpers, sd->bumperCount, "bumper");
    sd_bumper_array(&sd->jamabars, sd->jamabarCount, "jamabar");
    sd_banana_array(&sd->bananas, sd->bananaCount);
    sd_cone_array(&sd->coliCones, sd->coliConeCount);
    sd_sphere_array(&sd->coliSpheres, sd->coliSphereCount);
    sd_cylinder_array(&sd->coliCylinders, sd->coliCylinderCount);
    sd_fallout_array(&sd->falloutVolumes, sd->falloutVolumeCount);
    sd_reflective_array(&sd->reflectives, sd->reflectiveCount);
    sd_instance_array(&sd->instances, sd->instanceCount);
    sd_modelref_ptr_array(&sd->modelRefs, sd->modelRefCount);
    sd_button_array(&sd->buttons, sd->buttonCount);
    sd_wormhole_array(&sd->wormholes, sd->wormholeCount);
    sd_generator_array(&sd->generators, sd->generatorCount);

    if (sd->unk80 != NULL)
        fprintf(stderr, "pc_swap_stagedef: unk80 present, payload not swapped\n");
    if (sd->unkD4 != NULL)
        fprintf(stderr, "pc_swap_stagedef: unkD4 present, payload not swapped\n");

    /* sd->fog: only the type byte is read (struct StageFog); nothing to swap */

    {
        struct StageFogAnim *fa = sd_ptr(&sd->fogAnim);

        if (fa != NULL) {
            struct StageKeyframeChannel *ch = &fa->start;

            sdw32n(fa, 12);   /* 6 channels */
            for (i = 0; i < 6; i++)
                sd_keyframes(&ch[i], "fog anim keyframe");
        }
    }
    {
        struct StageLightAnim *la = sd_ptr(&sd->lightAnim);

        n = sd_count(sd->lightAnimCount, "light anim");
        for (i = 0; la != NULL && i < n; i++) {
            struct StageKeyframeChannel *ch = &la[i].dirX;
            s32 j;

            sdw32n(ch, 6);   /* unk0/unk20 regions left untouched */
            for (j = 0; j < 3; j++)
                sd_keyframes(&ch[j], "light anim keyframe");
        }
    }
    {
        struct StageBgGuest *g = sd_ptr(&sd->bgGuests);

        n = sd_count(sd->bgGuestCount, "bg guest");
        for (i = 0; g != NULL && i < n; i++) {
            sdw32n(&g[i].pos, 3);
            sdw16n(&g[i].rotX, 3);   /* colorIdx/apeIdx are bytes */
            sdw32(&g[i].scale);      /* unk18 left alone */
        }
    }
    {
        struct StageReflectionPlane *rp = sd_ptr(&sd->dynamicReflectionPlane);

        if (rp != NULL) {
            sdw32n(&rp->flags, 4);   /* flags + pos */
            sdw16n(&rp->rot, 5);     /* rot xyz, itemgroupIdx, drawMask */
        }
    }

    sd_bg_model_array(&sd->bgModels, sd->bgModelCount, "bg model");
    sd_bg_model_array(&sd->fgModels, sd->fgModelCount, "fg model");

    {
        struct StageExtraData *ex = sd_ptr(&sd->extraData);

        if (ex != NULL) {
            struct StageExtraChannels *ch;
            struct StageKeyframeChannel *kc;

            sdw32n(ex, 26);   /* the whole 0x68-byte struct is words */
            sd_extra_channels(&ex->channels);
            ch = sd_ptr(&ex->list);
            n = sd_count(ex->listCount, "extra channel list");
            for (i = 0; ch != NULL && i < n; i++) {
                sdw32n(&ch[i], 6);
                sd_extra_channels(&ch[i]);
            }
            kc = &ex->unk20;
            for (i = 0; i < 9; i++)
                sd_keyframes(&kc[i], "extra keyframe");
        }
    }

    {
        struct StageModelRef *ref = sd_ptr(&sd->modelList);

        n = sd_count(sd->modelListCount, "model list");
        for (i = 0; ref != NULL && i < n; i++) {
            struct StageModelInfo *lod;
            s32 j, nl;

            sdw32n(&ref[i], 3);   /* flags, lodCount, lods offset */
            lod = sd_ptr(&ref[i].lods);
            nl = sd_count(ref[i].lodCount, "lod");
            for (j = 0; lod != NULL && j < nl; j++) {
                /* lod->unk0 is unknown byte data; left untouched */
                sdw32n(&lod[j].name, 3);   /* name offset, model, minSize */
            }
        }
    }

    {
        struct StageExtraAnims *ea = sd_ptr(&sd->extraAnims);

        if (ea != NULL) {
            sdw32n(&ea->cupPos, 3);   /* golf cup position (floats) */
            sdw32n(&ea->animA, 2);
            sd_itemgroup_anim(&ea->animA);
            sd_itemgroup_anim(&ea->animB);
        }
    }
}

/* ---- /replay/*.rec bundled replay files: recplay.c g_LoadRecFile ----
 *
 * A .rec file is the in-memory struct Replay image with its pointers stored
 * as offsets from the struct base (fn_1_63450 re-adds the base; an offset of
 * 0 means NULL).  Layout (recplay.c struct Replay, 0x50 bytes):
 *   0x00 ReplayHeader { u16 flags; u16 stageId; u8 x4[4]; char name[4];
 *                       f32 grade; s32 score; u8 x14[4] }
 *   0x18 u32 size; 0x1C..0x26 six s16/u16 (maxFrames at 0x26);
 *   0x28 Vec goalVel; 0x34 s16 writePos; 0x36 s16 storedFrames;
 *   0x38 u32 ballFrames off; 0x3C u32 worldFrames off;
 *   0x40 ReplayTracks { s32 startTimer; u16 igCount; u16 ssCount;
 *                       u32 itemGroups off; u32 seesaws off }
 * ballFrames: maxFrames * 0x20 { Vec pos; 6*s16; u32 flags; f32 }.
 * worldFrames: maxFrames * { s16 rotX; s16 rotZ }.
 * itemGroups: igCount * 0xC { u32 id; s16; u32 frames off } — the frame
 * entries are single u8s, so only the track header swaps.
 * seesaws: ssCount * 0xC { u32 id; u32 stateSize; u32 frames off } — each
 * frame is stateSize bytes of 4-byte values (currently one float angle),
 * so the frame data swaps as u32 words. */
void pc_swap_replay(void *fileData, u32 fileSize)
{
    u8 *base = (u8 *)fileData;
    u32 maxFrames, igCount, ssCount, ballOff, worldOff, igOff, ssOff;
    u32 i, j;

    BSWAP16(base + 0x00);   /* header.flags */
    BSWAP16(base + 0x02);   /* header.stageId */
    BSWAP32(base + 0x0C);   /* header.grade */
    BSWAP32(base + 0x10);   /* header.score */
    BSWAP32(base + 0x18);   /* size */
    for (i = 0x1C; i <= 0x26; i += 2)
        BSWAP16(base + i);
    BSWAP32(base + 0x28);   /* goalVel */
    BSWAP32(base + 0x2C);
    BSWAP32(base + 0x30);
    BSWAP16(base + 0x34);   /* writePos */
    BSWAP16(base + 0x36);   /* storedFrames */
    BSWAP32(base + 0x38);   /* ballFrames offset */
    BSWAP32(base + 0x3C);   /* worldFrames offset */
    BSWAP32(base + 0x40);   /* tracks.startTimer */
    BSWAP16(base + 0x44);   /* tracks.itemGroupCount */
    BSWAP16(base + 0x46);   /* tracks.seesawCount */
    BSWAP32(base + 0x48);   /* tracks.itemGroups offset */
    BSWAP32(base + 0x4C);   /* tracks.seesaws offset */

    maxFrames = *(u16 *)(base + 0x26);
    ballOff   = *(u32 *)(base + 0x38);
    worldOff  = *(u32 *)(base + 0x3C);
    igCount   = *(u16 *)(base + 0x44);
    ssCount   = *(u16 *)(base + 0x46);
    igOff     = *(u32 *)(base + 0x48);
    ssOff     = *(u32 *)(base + 0x4C);

    if (*(u32 *)(base + 0x18) > fileSize ||
        ballOff + maxFrames * 0x20 > fileSize ||
        worldOff + maxFrames * 4 > fileSize) {
        fprintf(stderr, "pc_swap_replay: implausible layout (size %u, file %u)\n",
                *(u32 *)(base + 0x18), fileSize);
        return;
    }

    for (i = 0; i < maxFrames; i++) {
        u8 *f = base + ballOff + i * 0x20;
        BSWAP32(f + 0x00);  /* pos */
        BSWAP32(f + 0x04);
        BSWAP32(f + 0x08);
        for (j = 0x0C; j <= 0x16; j += 2)
            BSWAP16(f + j); /* rotX..unk16 */
        BSWAP32(f + 0x18);  /* flags */
        BSWAP32(f + 0x1C);  /* unk1C */
    }
    for (i = 0; i < maxFrames; i++) {
        BSWAP16(base + worldOff + i * 4);
        BSWAP16(base + worldOff + i * 4 + 2);
    }
    if (igOff != 0 && igOff + igCount * 0xC <= fileSize) {
        for (i = 0; i < igCount; i++) {
            u8 *t = base + igOff + i * 0xC;
            BSWAP32(t + 0x0);
            BSWAP16(t + 0x4);
            BSWAP32(t + 0x8);
        }
    }
    if (ssOff != 0 && ssOff + ssCount * 0xC <= fileSize) {
        for (i = 0; i < ssCount; i++) {
            u8 *t = base + ssOff + i * 0xC;
            u32 stateSize, frOff, words;
            BSWAP32(t + 0x0);
            BSWAP32(t + 0x4);
            BSWAP32(t + 0x8);
            stateSize = *(u32 *)(t + 0x4);
            frOff = *(u32 *)(t + 0x8);
            words = (stateSize >> 2) * maxFrames;
            if (frOff != 0 && frOff + words * 4 <= fileSize)
                for (j = 0; j < words; j++)
                    BSWAP32(base + frOff + j * 4);
        }
    }
}

/* ---- dlg/*.bin dialog tables: dialog.c load_dialog_tbl ----
 *
 * File = struct DialogTbl { s32 count; u32 entries off } followed by the
 * entry-offset array and 0x30-byte DialogData records; all pointers are
 * stored as offsets from the table base (the loader adds the base after
 * this swap runs).  DialogData: u32 name off @0x00; u16 @0x06..0x10;
 * f32 @0x14; u16 @0x18..0x20; f32 @0x24,0x28; u32 text off @0x2C.
 * name/text point at char strings (no swap). */
void pc_swap_dialog_tbl(void *fileData)
{
    u8 *base = (u8 *)fileData;
    u32 count, entriesOff, i;

    BSWAP32(base + 0x0);
    BSWAP32(base + 0x4);
    count = *(u32 *)(base + 0x0);
    entriesOff = *(u32 *)(base + 0x4);
    for (i = 0; i < count; i++) {
        u8 *ep = base + entriesOff + i * 4;
        u8 *e;

        BSWAP32(ep);
        e = base + *(u32 *)ep;
        BSWAP32(e + 0x00);  /* name offset */
        BSWAP16(e + 0x06);  /* drawFrame */
        BSWAP16(e + 0x08);  /* mode */
        BSWAP16(e + 0x0A);  /* offsetX */
        BSWAP16(e + 0x0C);  /* offsetY */
        BSWAP16(e + 0x0E);  /* depth */
        BSWAP16(e + 0x10);  /* tailMode */
        BSWAP32(e + 0x14);  /* unk14 */
        BSWAP16(e + 0x18);
        BSWAP16(e + 0x1A);
        BSWAP16(e + 0x1C);
        BSWAP16(e + 0x1E);  /* width */
        BSWAP16(e + 0x20);  /* numLines */
        BSWAP32(e + 0x24);  /* scaleX */
        BSWAP32(e + 0x28);  /* scaleY */
        BSWAP32(e + 0x2C);  /* text offset */
    }
}

/* ---- scndat_*.bin cutscene data: scnplay.c load_scene_data ----
 *
 * File = array of struct Scene (0x14) terminated by objs==0, with offsets
 * from the file base everywhere a pointer appears (fixed up afterwards by
 * g_parse_scene_data).  Each SceneObj (0x10) points at a SceneAnim (0x1C)
 * whose keyframe arrays are laid out channel-after-channel: one u16 count,
 * then count u16 times, count u8 value types (no swap), and per type
 * s_keyframeValueCounts[t] f32 values; one channel per set bit (0..9) in
 * each SceneJointFlags mask. */
void pc_swap_scene_data(void *fileData, u32 fileSize)
{
    static const u8 kfValueCounts[6] = { 0, 1, 2, 3, 1, 1 }; /* = mot_joint.c s_keyframeValueCounts */
    static u32 seenAnims[8192];
    u32 nSeen = 0;
    u8 *base = (u8 *)fileData;
    u8 *scene = base;

    for (;;) {
        u32 objsOff, numObjs, o;

        BSWAP32(scene + 0x00);              /* objs */
        objsOff = *(u32 *)(scene + 0x00);
        if (objsOff == 0)
            break;                          /* terminator: game only reads ->objs */
        BSWAP32(scene + 0x04);              /* name */
        BSWAP16(scene + 0x08);              /* numObjs */
        BSWAP16(scene + 0x0A);              /* duration */
        BSWAP32(scene + 0x0C);              /* stage */
        BSWAP32(scene + 0x10);              /* unk10 */
        numObjs = *(u16 *)(scene + 0x08);
        if (objsOff >= fileSize || objsOff + numObjs * 0x10 > fileSize) {
            fprintf(stderr, "pc_swap_scene_data: implausible objs 0x%x x%u\n",
                    objsOff, numObjs);
            return;
        }
        for (o = 0; o < numObjs; o++) {
            u8 *obj = base + objsOff + o * 0x10;
            u32 animOff, s, dup = 0;

            BSWAP16(obj + 0x00);            /* type */
            BSWAP32(obj + 0x04);            /* anim */
            BSWAP32(obj + 0x08);            /* name */
            BSWAP16(obj + 0x0C);
            BSWAP16(obj + 0x0E);
            animOff = *(u32 *)(obj + 0x04);
            if (animOff == 0 || animOff + 0x1C > fileSize)
                continue;
            for (s = 0; s < nSeen; s++) {
                if (seenAnims[s] == animOff) {
                    dup = 1;
                    break;
                }
            }
            if (dup)
                continue;
            if (nSeen < 8192) {
                seenAnims[nSeen++] = animOff;
            } else {
                fprintf(stderr, "pc_swap_scene_data: anim table full\n");
                return;
            }
            {
                u8 *anim = base + animOff;
                u32 numJoints, i, fIdx;
                u8 *jf, *kc, *tm, *vt, *vals;

                BSWAP32(anim + 0x00);       /* unk0 (never read by the game) */
                BSWAP16(anim + 0x04);       /* numJoints */
                BSWAP32(anim + 0x08);       /* jointFlags */
                BSWAP32(anim + 0x0C);       /* keyframeCounts */
                BSWAP32(anim + 0x10);       /* times */
                BSWAP32(anim + 0x14);       /* valueTypes */
                BSWAP32(anim + 0x18);       /* values */
                if (*(u32 *)(anim + 0x08) == animOff) {
                    /* Self-referential stub (type 2/14/15 motion-ref objs):
                     * all five array offsets point back at the header and no
                     * keyframe data exists; real motion comes from the ape
                     * library.  Poison the aliased jointFlags[0].joint byte
                     * so fn_1_1432A0 consumes no channels instead of reading
                     * header bytes as keyframes. */
                    anim[0] = 0xFF;
                    continue;
                }
                numJoints = *(u16 *)(anim + 0x04);
                jf = base + *(u32 *)(anim + 0x08);
                kc = base + *(u32 *)(anim + 0x0C);
                tm = base + *(u32 *)(anim + 0x10);
                vt = base + *(u32 *)(anim + 0x14);
                vals = base + *(u32 *)(anim + 0x18);
                /* Mirror fn_1_1432A0: a jointFlags entry is consumed (and
                 * only then has serialized keyframe data) when its joint
                 * byte matches the running joint index. */
                fIdx = 0;
                for (i = 0; i < numJoints; i++) {
                    u32 mask, b, nChans = 0, c;

                    if (jf + fIdx * 4 + 4 > base + fileSize) {
                        fprintf(stderr, "pc_swap_scene_data: jointFlags out of bounds\n");
                        return;
                    }
                    if (jf[fIdx * 4] != i)
                        continue;
                    BSWAP16(jf + fIdx * 4 + 2);
                    mask = *(u16 *)(jf + fIdx * 4 + 2);
                    fIdx++;
                    for (b = 1; b <= 0x200; b <<= 1) {
                        if (mask & b)
                            nChans++;
                    }
                    for (c = 0; c < nChans; c++) {
                        u32 count, k, total = 0;

                        if (kc + 2 > base + fileSize) {
                            fprintf(stderr, "pc_swap_scene_data: channel walk out of bounds\n");
                            return;
                        }
                        BSWAP16(kc);
                        count = *(u16 *)kc;
                        kc += 2;
                        if (tm + count * 2 > base + fileSize ||
                            vt + count > base + fileSize) {
                            fprintf(stderr, "pc_swap_scene_data: keyframe arrays out of bounds\n");
                            return;
                        }
                        for (k = 0; k < count; k++) {
                            BSWAP16(tm);
                            tm += 2;
                        }
                        for (k = 0; k < count; k++)
                            total += kfValueCounts[vt[k] < 6 ? vt[k] : 0];
                        vt += count;
                        if (vals + total * 4 > base + fileSize) {
                            fprintf(stderr, "pc_swap_scene_data: values out of bounds\n");
                            return;
                        }
                        for (k = 0; k < total; k++) {
                            BSWAP32(vals);
                            vals += 4;
                        }
                    }
                }
            }
        }
        scene += 0x14;
    }
}
