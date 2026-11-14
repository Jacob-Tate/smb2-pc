#ifndef RENDER_H
#define RENDER_H

#include "platform.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* GX-compatible types and structures */
typedef struct { float x, y, z; } Vec;
typedef struct { float x, y, z, w; } Vec4;
typedef float Mtx[3][4];
typedef float Mtx44[4][4];
typedef struct { uint8_t r, g, b, a; } GXColor;

/* Must stay <= 32 bytes: it lives inside the game's opaque GXTexObj
 * (u32 dummy[8]) and is written through game pointers. */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t format;
    uint8_t  wrapS;
    uint8_t  wrapT;
    uint8_t  mipmap;
    /* GPU texture: a gx_core texture record id + its generation, not a
     * pointer (the record may have been freed and reused since; see
     * "Texture lifetime" in gx_core.c) */
    uint32_t tex_id;
    uint32_t tex_gen;
    void*    image_data; /* original texel pointer, for GXGetTexObjData */
} GXTexObj;

typedef struct {
    float mtx[3][4];
} GXTexMtx;

/* GX primitive types */
#define GX_POINTS           0
#define GX_LINES            1
#define GX_LINESTRIP        2
#define GX_TRIANGLES        3
#define GX_TRIANGLESTRIP    4
#define GX_TRIANGLEFAN      5
#define GX_QUADS            6

/* GX vertex attribute flags */
#define GX_VA_POS           (1 << 0)
#define GX_VA_NRM           (1 << 1)
#define GX_VA_CLR0          (1 << 2)
#define GX_VA_CLR1          (1 << 3)
#define GX_VA_TEX0          (1 << 4)
#define GX_VA_TEX1          (1 << 5)
#define GX_VA_TEX2          (1 << 6)
#define GX_VA_TEX3          (1 << 7)
#define GX_VA_TEX4          (1 << 8)
#define GX_VA_TEX5          (1 << 9)
#define GX_VA_TEX6          (1 << 10)
#define GX_VA_TEX7          (1 << 11)
#define GX_VA_POS_MTX_IDX   (1 << 12)
#define GX_VA_NRM_MTX_IDX   (1 << 13)

/* GX compare functions */
#define GX_NEVER            0
#define GX_LESS             1
#define GX_EQUAL            2
#define GX_LEQUAL           3
#define GX_GREATER          4
#define GX_NEQUAL           5
#define GX_GEQUAL           6
#define GX_ALWAYS           7

/* GX blend modes */
#define GX_BM_NONE          0
#define GX_BM_BLEND         1
#define GX_BM_LOGIC         2
#define GX_BM_SUBTRACT      3

/* Values match the GC GX enum exactly: game code passes these numbers. */
#define GX_BL_ZERO          0
#define GX_BL_ONE           1
#define GX_BL_SRCCLR        2   /* as dst factor: source color */
#define GX_BL_INVSRCCLR     3
#define GX_BL_SRCALPHA      4
#define GX_BL_INVSRCALPHA   5
#define GX_BL_DSTALPHA      6
#define GX_BL_INVDSTALPHA   7
#define GX_BL_DSTCLR        GX_BL_SRCCLR    /* as src factor: dest color */
#define GX_BL_INVDSTCLR     GX_BL_INVSRCCLR

/* GX texture formats */
#define GX_TF_I4            0x00
#define GX_TF_I8            0x01
#define GX_TF_IA4           0x02
#define GX_TF_IA8           0x03
#define GX_TF_RGB565        0x04
#define GX_TF_RGB5A3        0x05
#define GX_TF_RGBA8         0x06
#define GX_TF_C4            0x08
#define GX_TF_C8            0x09
#define GX_TF_C14X2         0x0A
#define GX_TF_CMPR          0x0E

/* GX wrap modes */
#define GX_CLAMP            0
#define GX_REPEAT           1
#define GX_MIRROR           2

/* GX cull modes */
#define GX_CULL_NONE        0
#define GX_CULL_FRONT       1
#define GX_CULL_BACK        2
#define GX_CULL_ALL         3

/* GX TEV stages */
#define GX_MAX_TEVSTAGE     16
#define GX_MAX_TEXCOORD     8
#define GX_MAX_TEXMAP       8

/* Renderer API */
bool render_init(const char* backend_name, int width, int height, bool vsync, int msaa);
void render_shutdown(void);
void render_begin_frame(void);
void render_end_frame(void);
void render_resize(int width, int height);

/* GX-compatible API */
void GXInit(void);
void GXSetViewport(float left, float top, float width, float height, float nearZ, float farZ);
void GXSetScissor(uint32_t left, uint32_t top, uint32_t width, uint32_t height);
void GXSetDispCopySrc(uint16_t left, uint16_t top, uint16_t width, uint16_t height);
void GXSetDispCopyDst(uint16_t width, uint16_t height);
void GXSetCopyFilter(uint8_t aa, uint8_t samplePattern[12][2], uint8_t vf, uint8_t vfilter[7]);
void GXSetBlendMode(int type, int srcFactor, int dstFactor, int op);
void GXSetZMode(uint8_t compareEnable, int func, uint8_t updateEnable);
void GXSetZCompLoc(uint8_t beforeTex);
void GXSetAlphaCompare(int comp0, uint8_t ref0, int op, int comp1, uint8_t ref1);
void GXSetCullMode(int mode);
void GXSetNumChans(uint8_t n);
void GXSetNumTexGens(uint8_t n);
void GXSetNumTevStages(uint8_t n);
void GXSetNumIndStages(uint8_t n);
void GXSetTevOrder(int stage, int texCoord, int texMap, int color);
void GXSetTevColorIn(int stage, int a, int b, int c, int d);
void GXSetTevColorOp(int stage, int tevOp, int bias, int scale, int clamp, int outReg);
void GXSetTevAlphaIn(int stage, int a, int b, int c, int d);
void GXSetTevAlphaOp(int stage, int tevOp, int bias, int scale, int clamp, int outReg);
void GXSetTevKColor(int id, GXColor color);
void GXSetTevKColorSel(int stage, int sel);
void GXSetTevKAlphaSel(int stage, int sel);
void GXSetTevSwapMode(int stage, int ras, int tex);
void GXSetTevSwapModeTable(int id, int r, int g, int b, int a);
void GXSetTexCoordGen(int dst, int func, int src, int mtx);
void GXSetTexCoordGen2(int dst, int func, int src, int mtx, int normalize, int postMtx);
void GXLoadTexMtxImm(const Mtx mtx, int id, int type);
void GXLoadPosMtxImm(const Mtx mtx, int id);
void GXLoadNrmMtxImm(const Mtx mtx, int id);
void GXInitTexObj(GXTexObj* obj, void* image, uint16_t width, uint16_t height, int format, int wrapS, int wrapT, uint8_t mipmap);
void GXBegin(int type, int vtxFmt, uint16_t nverts);
void GXEnd(void);
void GXPosition3f32(float x, float y, float z);
void GXNormal3f32(float x, float y, float z);
void GXColor4u8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void GXTexCoord2f32(float s, float t);

/* Matrix math helpers (compatible with mathutil.h) */
void C_MTXPerspective(Mtx44 m, float fovY, float aspect, float n, float f);
void C_MTXOrtho(Mtx44 m, float top, float bottom, float left, float right, float n, float f);
void C_MTXLookAt(Mtx m, const Vec* camPos, const Vec* up, const Vec* target);
void MTXIdentity(Mtx m);
void MTXTrans(Mtx m, float x, float y, float z);
void MTXScale(Mtx m, float x, float y, float z);
void MTXRotRad(Mtx m, int axis, float rad);
void MTXConcat(const Mtx a, const Mtx b, Mtx ab);
void MTXInverse(const Mtx src, Mtx inv);
void MTXTranspose(const Mtx src, Mtx dst);
void MTXMultVec(const Mtx m, const Vec* src, Vec* dst);
void MTXMultVecSR(const Mtx m, const Vec* src, Vec* dst);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_H */