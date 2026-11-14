/* Byte swap and PC loader for GMA model archives (big-endian disc data).
 * The decomp's avload module is asm-only, so this file implements its GMA
 * entry points (mirrored from the asm) and swaps each archive after it is
 * loaded and before the offset->pointer fixup.  Display lists and name
 * strings are not swapped.  Layout: see docs/endian.md, GMA archives. */
#include "shim.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BSWAP32(p) (*(u32 *)(p) = __builtin_bswap32(*(u32 *)(p)))
#define BSWAP16(p) (*(u16 *)(p) = __builtin_bswap16(*(u16 *)(p)))

static u32 sw32(void *p)
{
    u32 v = __builtin_bswap32(*(u32 *)(p));
    *(u32 *)p = v;
    return v;
}

#define GCMF_MAGIC_NATIVE 0x47434D46u /* "GCMF" as a host-endian u32 */

/* GMAModel flags (ref smb-decomp gma.h) */
#define GCMF_STITCHING 0x04
#define GCMF_SKIN      0x08
#define GCMF_EFFECTIVE 0x10

/* Swap one 0x60 GMAShape header in place.  GXColor fields (@0x04/08/0C)
 * and the u8 fields/mtxIndices stay as bytes. */
static void gma_swap_shape_header(u8 *s)
{
    BSWAP32(s + 0x00);          /* flags */
    BSWAP16(s + 0x16);          /* tevLayerIdxs[3] */
    BSWAP16(s + 0x18);
    BSWAP16(s + 0x1A);
    BSWAP32(s + 0x1C);          /* vtxAttrs */
    BSWAP32(s + 0x28);          /* dispListSizes[2] */
    BSWAP32(s + 0x2C);
    BSWAP32(s + 0x30);          /* origin x,y,z */
    BSWAP32(s + 0x34);
    BSWAP32(s + 0x38);
    BSWAP32(s + 0x40);          /* blendFactors */
}

/* Swap a single GCMF model in place (also used for raw .gcm files).
 * Safe to call twice on the same model (archives may alias models):
 * an already-native magic is skipped silently. */
void pc_swap_gma_model(void *modelData)
{
    u8 *m = (u8 *)modelData;
    u32 flags, headerSize, tevCount, shapeCount, i;

    if (*(u32 *)m == GCMF_MAGIC_NATIVE)
        return;                             /* already swapped */
    if (memcmp(m, "GCMF", 4) != 0) {
        fprintf(stderr, "pc_swap_gma: bad GCMF magic %08x\n", *(u32 *)m);
        return;
    }
    BSWAP32(m + 0x00);                      /* magic -> native "GCMF" */
    flags = sw32(m + 0x04);
    BSWAP32(m + 0x08);                      /* boundSphereCenter x,y,z */
    BSWAP32(m + 0x0C);
    BSWAP32(m + 0x10);
    BSWAP32(m + 0x14);                      /* boundSphereRadius */
    BSWAP16(m + 0x18);                      /* tevLayerCount */
    BSWAP16(m + 0x1A);                      /* opaqueShapeCount */
    BSWAP16(m + 0x1C);                      /* translucentShapeCount */
    /* mtxCount/pad @0x1E are u8 */
    headerSize = sw32(m + 0x20);
    /* @0x24 texObjs pointer slot: runtime-only, left alone.
     * @0x28 mtxIndexes[8]: u8s, left alone. */

    tevCount = *(u16 *)(m + 0x18);
    shapeCount = *(u16 *)(m + 0x1A) + *(u16 *)(m + 0x1C);
    if (tevCount > 1000 || shapeCount > 10000 ||
        headerSize < 0x40 || headerSize > (16u << 20)) {
        fprintf(stderr, "pc_swap_gma: suspicious model (tev %u shapes %u "
                "headerSize 0x%x)\n", tevCount, shapeCount, headerSize);
        return;
    }

    /* tev layers */
    for (i = 0; i < tevCount; i++) {
        u8 *lyr = m + 0x40 + i * 0x20;
        BSWAP32(lyr + 0x0);                 /* flags */
        BSWAP16(lyr + 0x4);                 /* texIndex */
        /* @0x6 lodBias / @0x7 maxAniso are bytes, @0x8 is the runtime
         * texObj pointer, rest is filler */
    }

    /* model matrices between the tev layers and the shapes: STITCHING
     * models, and SKIN models whose bind matrices draw_poly(model, NULL)
     * (asm 0x8006977C) uses (e.g. AiAi's hair).  Swap the block whenever
     * mtxCount says it is there and it fits before the shapes. */
    if ((flags & GCMF_STITCHING) ||
        (m[0x1E] != 0 &&
         0x40 + tevCount * 0x20 + (u32)m[0x1E] * 0x30 <= headerSize)) {
        u8 *mtx = m + 0x40 + tevCount * 0x20;
        u32 words = (u32)m[0x1E] * 12;      /* mtxCount * Mtx(3x4 f32) */
        for (i = 0; i < words; i++)
            BSWAP32(mtx + i * 4);
    }

    if (flags & (GCMF_SKIN | GCMF_EFFECTIVE)) {
        /* 0x20 pre-header of u32s, then flat 0x60-stride shape headers,
         * then the CPU-read deformable payload the pre-header points to
         * (offsets relative to the pre-header):
         *   @0x00 vertex count   @0x04 skin source stream offset
         *   @0x08 vertex array offset   @0x0C indexed stream offset
         * See avdisp_shim.c for the payload layouts (from the GC asm
         * fn_8006A3D8 / fn_8006A498 / g_avdisp_smth_with_transforms). */
        u8 *ph = m + headerSize;
        u8 *sp;
        u32 vtxCount, skinOff, vtxOff, streamOff;
        u32 total, maxOff, j, nVtx;

        for (i = 0; i < 8; i++)
            BSWAP32(ph + i * 4);
        sp = ph + 0x20;
        for (i = 0; i < shapeCount; i++, sp += 0x60)
            gma_swap_shape_header(sp);

        vtxCount  = *(u32 *)(ph + 0x00);
        skinOff   = *(u32 *)(ph + 0x04);
        vtxOff    = *(u32 *)(ph + 0x08);
        streamOff = *(u32 *)(ph + 0x0C);
        if (vtxOff > (16u << 20) || streamOff > (16u << 20) ||
            skinOff > (16u << 20) || vtxCount > (1u << 20)) {
            fprintf(stderr, "pc_swap_gma: suspicious deformable pre-header "
                    "(%u verts, offs %x/%x/%x)\n", vtxCount, skinOff,
                    vtxOff, streamOff);
            return;
        }

        /* bone parent-index table (ref fn_80069964): mtxCount s16s at
         * ph + [ph+0x10]; bg_lava reads it through fn_80069964 */
        if (flags & GCMF_SKIN) {
            u32 parentOff = *(u32 *)(ph + 0x10);

            if (parentOff <= (16u << 20))
                for (i = 0; i < m[0x1E]; i++)
                    BSWAP16(ph + parentOff + i * 2);
        }

        if (flags & 0x01 /* GCMF_16BIT */) {
            /* 16-bit payload (u16 stream entries, s16 vertices): nothing
             * CPU-writes it, so it stays big-endian and the draw path
             * (avdisp_shim.c emit_indexed_s16) decodes it explicitly. */
            return;
        }

        /* 1) indexed triangle-strip streams, u32 entries: repeated
         * { count, count x byte offset }; each shape's dispListSizes[i]
         * (just swapped above) is the total entry count of its stream */
        total = 0;
        sp = ph + 0x20;
        for (i = 0; i < shapeCount; i++, sp += 0x60) {
            if (sp[0x13] & 0x1)
                total += *(u32 *)(sp + 0x28);
            if (sp[0x13] & 0x2)
                total += *(u32 *)(sp + 0x2C);
        }
        if (total > (16u << 20) / 4) {
            fprintf(stderr, "pc_swap_gma: suspicious deformable stream "
                    "size %u\n", total);
            return;
        }
        for (j = 0; j < total; j++)
            BSWAP32(ph + streamOff + j * 4);

        /* walk the swapped stream for the highest referenced vertex */
        maxOff = 0;
        j = 0;
        while (j < total) {
            u32 cnt = *(u32 *)(ph + streamOff + j * 4);
            u32 k;

            j++;
            if (cnt == 0 || cnt > total - j) {
                fprintf(stderr, "pc_swap_gma: bad deformable strip count "
                        "%u\n", cnt);
                break;
            }
            for (k = 0; k < cnt; k++, j++) {
                u32 off = *(u32 *)(ph + streamOff + j * 4);

                if (off > maxOff)
                    maxOff = off;
            }
        }

        /* 2) f32 vertex array, stride 0x40: swap the 12 words of
         * pos/nrm/tex0-2 (@0x00-0x2F); @0x30 RGBA8 and @0x34 boneCount
         * are bytes.  Cover every indexed vertex plus (for SKIN) the
         * pre-header count the skinning loop writes. */
        nVtx = (total != 0) ? maxOff / 0x40 + 1 : 0;
        if ((flags & GCMF_SKIN) && vtxCount > nVtx)
            nVtx = vtxCount;
        for (i = 0; i < nVtx; i++)
            for (j = 0; j < 12; j++)
                BSWAP32(ph + vtxOff + i * 0x40 + j * 4);

        /* 3) SKIN source stream: one 0x20 entry per vertex-bone pair
         * (vertex i consumes boneCount_i consecutive entries): bind pos
         * f32x3 @0x00, nrm f32x3 @0x0C, u16 matrix slot @0x18, f32
         * weight @0x1C */
        if (flags & GCMF_SKIN) {
            u32 entries = 0;

            for (i = 0; i < vtxCount; i++)
                entries += ph[vtxOff + i * 0x40 + 0x34];
            for (i = 0; i < entries; i++) {
                u8 *e = ph + skinOff + i * 0x20;

                for (j = 0; j < 6; j++)
                    BSWAP32(e + j * 4);     /* pos + nrm */
                BSWAP16(e + 0x18);          /* bone matrix slot */
                BSWAP32(e + 0x1C);          /* weight */
            }
        }
    } else {
        u8 *sp = m + headerSize;
        for (i = 0; i < shapeCount; i++) {
            u8 *p;
            u32 dlFlags, size0, size1;

            gma_swap_shape_header(sp);
            dlFlags = sp[0x13];
            size0 = *(u32 *)(sp + 0x28);
            size1 = *(u32 *)(sp + 0x2C);
            if (size0 > (16u << 20) || size1 > (16u << 20)) {
                fprintf(stderr, "pc_swap_gma: suspicious dispListSize "
                        "%u/%u\n", size0, size1);
                return;
            }
            p = sp + 0x60;                  /* display lists: not swapped */
            if (dlFlags & 0x1)
                p += size0;
            if (dlFlags & 0x2)
                p += size1;
            if (dlFlags & 0xC) {            /* extra display lists */
                u32 ex0, ex1;
                BSWAP32(p + 0x8);           /* GMAExtraDispLists sizes */
                BSWAP32(p + 0xC);
                ex0 = *(u32 *)(p + 0x8);
                ex1 = *(u32 *)(p + 0xC);
                if (ex0 > (16u << 20) || ex1 > (16u << 20)) {
                    fprintf(stderr, "pc_swap_gma: suspicious extra "
                            "dispListSize %u/%u\n", ex0, ex1);
                    return;
                }
                p += 0x20 + ex0 + ex1;
            }
            sp = p;
        }
    }
}

/* Swap a whole GMA archive in place, right after load and before the
 * offset->pointer fixup in g_init_gma below. */
void pc_swap_gma(void *gmaFileData)
{
    u8 *p = (u8 *)gmaFileData;
    u8 *modelsBase;
    u32 numModels, modelsBaseOff, i;

    numModels = __builtin_bswap32(*(u32 *)p);   /* peek before committing */
    if (numModels >= 10000) {
        fprintf(stderr, "pc_swap_gma: suspicious numModels %u (%08x) - "
                "not swapping\n", numModels, *(u32 *)p);
        return;
    }
    BSWAP32(p + 0);                             /* numModels */
    modelsBaseOff = sw32(p + 4);                /* base offset of models */
    if (modelsBaseOff < 8 || modelsBaseOff > (64u << 20)) {
        fprintf(stderr, "pc_swap_gma: suspicious modelsBase offset 0x%x\n",
                modelsBaseOff);
        return;
    }
    modelsBase = p + modelsBaseOff;

    for (i = 0; i < numModels; i++) {           /* entry table */
        BSWAP32(p + 8 + i * 8 + 0);             /* modelOffset */
        BSWAP32(p + 8 + i * 8 + 4);             /* nameOffset */
    }
    /* name blob between the entries and modelsBase: strings, untouched */

    for (i = 0; i < numModels; i++) {
        u32 modelOff = *(u32 *)(p + 8 + i * 8);
        if (modelOff == 0xFFFFFFFFu)            /* null-model sentinel */
            continue;
        pc_swap_gma_model(modelsBase + modelOff);
    }
}

/* ---- PC implementations of the asm-only avload GMA entry points ----
 *
 * Mirrored from the GC asm (auto_03_80053ABC_text.s); they use the
 * decomp's C file/heap APIs (like bitmap.c's load_bmp) instead of the
 * original DVD function-pointer table.  g_init_gma_model_materials
 * (asm 0x8006A9C8) is weak here; avdisp_shim.c provides the real one. */

/* struct File, file.h: 0x50 bytes on GC/32-bit, larger on x64 (the
 * DVDFileInfo inside holds pointers); keep headroom and check it. */
struct PcFile { u8 raw[0x100]; };
#include "file.h"
_Static_assert(sizeof(struct File) <= sizeof(struct PcFile), "PcFile too small");
#define file_open(n, f) file_open((n), (struct File *)(f))
#define file_close(f) file_close((struct File *)(f))
#define file_size(f) file_size((struct File *)(f))
#define file_read(f, d, s, o) file_read((struct File *)(f), (d), (s), (o))
extern int  currentHeap;
extern void *alloc_from_heap_or_panic(int heap, u32 size, char *file, int line);
u32 OSCheckHeap(int heap);

/* SMB2_HEAP_LOG=1: log every GMA/TPL load with the current heap's free space */
static void heap_log(const char *what, const char *file, u32 size)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("SMB2_HEAP_LOG");
        on = (e && *e && *e != '0');
    }
    if (on)
        fprintf(stderr, "[heaplog] %s %s size %u heap %d free %u\n", what, file,
                size, currentHeap, OSCheckHeap(currentHeap));
}

struct PcGMA                                    /* struct GMA, avdisp.h */
{
    u32 numModels;                              /* DPTR: same 0x10 layout */
    u8 * DPTR modelsBase;                       /* on x64 (docs/x64.md) */
    u8 * DPTR modelEntries;                     /* { model ptr; name ptr } */
    char * DPTR namesBase;
};

struct PcGMAEntry
{
    u8 * DPTR model;
    char * DPTR name;
};

void g_init_gma_model_materials(void *model, void *tpl, void *texObjs)
    __attribute__((weak));
__attribute__((weak)) void g_init_gma_model_materials(void *model, void *tpl,
                                                      void *texObjs)
{
    (void)model; (void)tpl; (void)texObjs;
    shim_log_once("g_init_gma_model_materials");
}

static char s_invalidModelName[] = "invalid_model";

/* asm 0x8006A8DC: the single offset->pointer fixup for a GMA archive.
 * PC port: the byte swap runs here, before the fixup touches the data. */
void g_init_gma(struct PcGMA *gma, u8 *fileData, void *tpl)
{
    u32 i;

    pc_swap_gma(fileData);  /* PC port: GMA data is big-endian on disc */

    gma->numModels = *(u32 *)(fileData + 0);
    gma->modelsBase = fileData + *(u32 *)(fileData + 4);
    gma->modelEntries = fileData + 8;
    gma->namesBase = (char *)(gma->modelEntries + gma->numModels * 8);
    for (i = 0; i < gma->numModels; i++) {
        struct PcGMAEntry *entry =
            (struct PcGMAEntry *)(gma->modelEntries + i * 8);
        if ((uptr)entry->model == 0xFFFFFFFFu) { /* null-model sentinel */
            entry->model = 0;
            entry->name = s_invalidModelName;
        } else {
            entry->model = gma->modelsBase + (uptr)entry->model;
            entry->name = gma->namesBase + (uptr)entry->name;
            g_init_gma_model_materials(entry->model, tpl, 0);
        }
    }
}

/* asm 0x80065AA8: load a .gma archive; the GMA struct (0x20 bytes incl.
 * padding) is allocated in front of the raw file data. */
struct PcGMA *g_load_gma(char *file, void *tpl)
{
    struct PcFile f __attribute__((aligned(32)));
    struct PcGMA *gma;
    u8 *fileData;
    u32 size;

    if (!file_open(file, &f))
        return 0;
    size = file_size(&f);
    heap_log("gma", file, size);
    gma = alloc_from_heap_or_panic(currentHeap,
                                   ((size + 0x1F) & ~0x1Fu) + 0x20,
                                   "endian_gma.c", __LINE__);
    /* +0x10 bit0 is the "don't free" flag g_something_freeing_heap tests;
     * it lives in the header padding, so clear it explicitly */
    memset((u8 *)gma + 0x10, 0, 0x10);
    fileData = (u8 *)gma + 0x20;
    if (file_read(&f, fileData, size, 0) < 0) {
        fprintf(stderr, "g_load_gma: cannot read <%s>\n", file);
        file_close(&f);
        return 0;
    }
    file_close(&f);
    g_init_gma(gma, fileData, tpl);
    return gma;
}

/* asm 0x80065CA8: load a .tpl texture archive; the TPL struct (0x20 bytes
 * incl. padding) is allocated in front of the raw file data.  pc_swap_tpl
 * swaps the headers only; the texel payload stays big-endian and is
 * decoded by the renderer.  As in the original there is no LZ path and no
 * imageOffset fixup (consumers use tpl->fileData + imageOffset). */
extern void pc_swap_tpl(void *fileData);

struct PcTPL                                   /* struct TPL, bitmap.h */
{
    u32 numTextures;
    u8 * DPTR texHeaders;
    u8 * DPTR fileData;
    void * DPTR texObjs;
};

struct PcTPL *g_load_tpl(char *file)
{
    struct PcFile f __attribute__((aligned(32)));
    struct PcTPL *tpl;
    u8 *fileData;
    u32 size;

    if (!file_open(file, &f))
        return 0;
    size = file_size(&f);
    heap_log("tpl", file, size);
    tpl = alloc_from_heap_or_panic(currentHeap,
                                   ((size + 0x1F) & ~0x1Fu) + 0x20,
                                   "endian_gma.c", __LINE__);
    fileData = (u8 *)tpl + 0x20;
    if (file_read(&f, fileData, size, 0) < 0) {
        fprintf(stderr, "g_load_tpl: cannot read <%s>\n", file);
        file_close(&f);
        return 0;
    }
    file_close(&f);
    pc_swap_tpl(fileData); /* PC port: TPL headers are big-endian on disc */
    tpl->numTextures = *(u32 *)fileData;
    tpl->texHeaders = fileData + 4;
    tpl->fileData = fileData;
    tpl->texObjs = 0;
    return tpl;
}

/* ---- scene teardown (asm 0x80065BF8 / 0x80065DC4) --------------------
 * On GC these free through the one-arg os_free_to_heap_func_ptr while
 * currentHeap has already moved on to the next scene's heap, so the PC
 * versions free to whichever heap owns the pointer. */
extern void pc_free_to_owning_heap(void *ptr);

void g_something_freeing_heap(struct PcGMA *gma)
{
    u32 i;

    if (!gma)
        return;
    for (i = 0; i < gma->numModels; i++) {
        u8 *model = ((struct PcGMAEntry *)(gma->modelEntries + i * 8))->model;
        if (model) {
            /* per-model GXTexObj array, alloc'd by
             * g_init_gma_model_materials when no shared buffer was given */
            void *texObjs = (void *)(uptr)*(u32 *)(model + 0x24); /* DPTR slot */
            if (texObjs)
                pc_free_to_owning_heap(texObjs);
        }
    }
    if (!(*(u32 *)((u8 *)gma + 0x10) & 1))   /* bit0 = "don't free" */
        pc_free_to_owning_heap(gma);
}

void g_something_freeing_heap_2(struct PcTPL *tpl)
{
    if (!tpl)
        return;
    if (!((uptr)tpl->texObjs & 1)) /* bit0 = "don't free" */
        pc_free_to_owning_heap(tpl);
}

/* asm 0x80065984: load a single raw GCMF model (.gcm, test mode only). */
void *g_load_raw_gma(char *file, void *tpl)
{
    struct PcFile f __attribute__((aligned(32)));
    u8 *model;
    u32 size;

    if (!file_open(file, &f))
        return 0;
    size = file_size(&f);
    model = alloc_from_heap_or_panic(currentHeap, (size + 0x1F) & ~0x1Fu,
                                     "endian_gma.c", __LINE__);
    if (file_read(&f, model, size, 0) < 0) {
        fprintf(stderr, "g_load_raw_gma: cannot read <%s>\n", file);
        file_close(&f);
        return 0;
    }
    file_close(&f);
    pc_swap_gma_model(model); /* PC port: big-endian on disc */
    if (*(u32 *)model != GCMF_MAGIC_NATIVE) {
        fprintf(stderr, "invalid model format <%s>\n", file);
        return 0;
    }
    g_init_gma_model_materials(model, tpl, 0);
    return model;
}

/* asm 0x80006DA8 (load_common_gma_tpl, DOL init): loads init/common.tpl +
 * init/common.gma into init_common_tpl/init_common_gma.  On PC the DOL
 * init chain (main -> g_init_console -> g_some_heap_init) is skipped --
 * main.c jumps straight to the REL _prolog -- so _prolog calls this via an
 * #ifndef __MWERKS__ hook right before g_load_init_common(). */
s32 DVDChangeDir(char *path);

struct PcGMA *init_common_gma;
struct PcTPL *init_common_tpl;

void load_common_gma_tpl(void)
{
    DVDChangeDir("/init");
    init_common_tpl = g_load_tpl("common.tpl");
    init_common_gma = g_load_gma("common.gma", init_common_tpl);
    DVDChangeDir("/");
    if (!init_common_gma || !init_common_tpl)
        fprintf(stderr, "load_common_gma_tpl: FAILED (gma=%p tpl=%p)\n",
                (void *)init_common_gma, (void *)init_common_tpl);
}
