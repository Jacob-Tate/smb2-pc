/* ARAM + ARC shim.
 *
 * ARAM: the GameCube's 16MB auxiliary RAM, used by SMB2 as a disc file cache
 * and by the sound system.  We back it with a host allocation; ARAM addresses
 * are plain offsets into the block.  ARQ transfers become memcpy with the
 * callback fired immediately.
 *
 * ARC: the SDK's in-memory archive reader (.arc files, magic 0x55AA382D).
 * The game's ArcHandle (0x1C) and ArcFile (0xC) are the SDK layouts. */
#include "shim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------------------------------------- ARAM ---- */
#define ARAM_SIZE 0x1000000u

static u8 *s_aram;

static void aram_ensure(void)
{
    if (!s_aram) {
        s_aram = calloc(1, ARAM_SIZE);
        if (!s_aram) {
            fprintf(stderr, "FATAL: cannot allocate ARAM block\n");
            exit(1);
        }
    }
}

void ARInit(u32 *stackBase, u32 numEntries)
{
    (void)stackBase; (void)numEntries;
    aram_ensure();
}

u32 ARGetBaseAddress(void) { return 0x4000; }
u32 ARGetSize(void) { return ARAM_SIZE; }

void ARQInit(void)
{
    aram_ensure();
}

/* source/dest are an ARAM offset and a main-RAM address; the main-RAM one
 * is a full host pointer (uptr: u32 on GC/32-bit, 64-bit on x64). */
void ARQPostRequest(void *request, u32 owner, u32 type, u32 priority, uptr source,
                    uptr dest, u32 length, void (*callback)(u32))
{
    (void)owner; (void)priority;
    aram_ensure();
    if (type == 0) {
        /* MRAM -> ARAM */
        if ((u64)dest + length <= ARAM_SIZE)
            memcpy(s_aram + dest, (void *)(uintptr_t)source, length);
        else
            fprintf(stderr, "ARQ: ARAM write out of range 0x%x+0x%x\n", (u32)dest, length);
    } else {
        /* ARAM -> MRAM */
        if ((u64)source + length <= ARAM_SIZE)
            memcpy((void *)(uintptr_t)dest, s_aram + source, length);
        else
            fprintf(stderr, "ARQ: ARAM read out of range 0x%x+0x%x\n", (u32)source, length);
    }
    if (callback)
        callback((u32)(uintptr_t)request);
}

/* ----------------------------------------------------- ARC ---- */
/* SDK layouts; the game treats these as opaque u8 blobs of the same size
 * (ape.c: struct ArcHandle { u8[0x1C] }, struct ArcFile { u8[0xC] }).  On
 * x64 the pointer fields are DPTR so the layout stays 0x1C / 0xC: all of
 * them point into the archive image, which is loaded into GC RAM. */
struct ShimARCHandle {
    void * DPTR archiveStartAddr;
    u8   * DPTR FSTStart;
    u8   * DPTR fileStart;
    u32   entryNum;
    char * DPTR FSTStringStart;
    u32   FSTLength;
    u32   currDir;
}; /* 0x1C */

struct ShimARCFileInfo {
    struct ShimARCHandle * DPTR handle;
    u32 startOffset;
    u32 length;
}; /* 0xC */

_Static_assert(sizeof(struct ShimARCHandle) == 0x1C, "ARCHandle must stay 0x1C");
_Static_assert(sizeof(struct ShimARCFileInfo) == 0xC, "ARCFileInfo must stay 0xC");

static u32 be32(const u8 *p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

#define ARC_MAGIC 0x55AA382Du
#define FST_ENTRY(h, i)      ((h)->FSTStart + (u32)(i) * 12)
#define FST_IS_DIR(e)        ((e)[0] == 1)
#define FST_NAME_OFS(e)      ((((u32)(e)[1]) << 16) | (((u32)(e)[2]) << 8) | (e)[3])
#define FST_DATA(e)          be32((e) + 4)   /* file offset / parent dir */
#define FST_SIZE(e)          be32((e) + 8)   /* file size / next index  */

void ARCInitHandle(void *bin, void *handle)
{
    struct ShimARCHandle *h = handle;
    u8 *b = bin;
    u32 magic = be32(b);
    u32 fstStart = be32(b + 4);
    u32 fstSize = be32(b + 8);
    u32 fileStart = be32(b + 12);

    if (magic != ARC_MAGIC)
        fprintf(stderr, "ARCInitHandle: bad magic %08x\n", magic);
    if ((u64)(uintptr_t)b + fstStart + fstSize > 0xFFFFFFFFull) {
        fprintf(stderr, "FATAL: ARCInitHandle: archive %p is above 4 GiB (DPTR fields)\n", (void *)b);
        exit(1);
    }
    h->archiveStartAddr = b;
    h->FSTStart = b + fstStart;
    h->fileStart = b + fileStart;
    h->entryNum = FST_SIZE(FST_ENTRY(h, 0));  /* root's next = entry count */
    h->FSTStringStart = (char *)(h->FSTStart + h->entryNum * 12);
    h->FSTLength = fstSize;
    h->currDir = 0;
}

static int arc_seg_eq(const char *name, const char *seg, u32 len)
{
    u32 i;
    for (i = 0; i < len; i++) {
        char a = name[i], b = seg[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b || name[i] == '\0')
            return 0;
    }
    return name[len] == '\0';
}

int ARCOpen(void *handle, char *path, void *info)
{
    struct ShimARCHandle *h = handle;
    struct ShimARCFileInfo *fi = info;
    const char *p = path;
    u32 cur = h->currDir;

    if (*p == '/') {
        cur = 0;
        while (*p == '/') p++;
    }
    while (*p) {
        const char *seg = p;
        u32 len, next, j;
        int found = -1;
        while (*p && *p != '/') p++;
        len = (u32)(p - seg);
        while (*p == '/') p++;
        if (len == 0 || (len == 1 && seg[0] == '.'))
            continue;
        if (len == 2 && seg[0] == '.' && seg[1] == '.') {
            cur = FST_DATA(FST_ENTRY(h, cur)); /* parent dir index */
            continue;
        }
        next = FST_SIZE(FST_ENTRY(h, cur));
        for (j = cur + 1; j < next && j < h->entryNum; ) {
            const u8 *e = FST_ENTRY(h, j);
            if (arc_seg_eq(h->FSTStringStart + FST_NAME_OFS(e), seg, len)) {
                found = (int)j;
                break;
            }
            j = FST_IS_DIR(e) ? FST_SIZE(e) : j + 1;
        }
        if (found < 0) {
            fprintf(stderr, "ARCOpen: not found: %s\n", path);
            return 0;
        }
        cur = (u32)found;
    }

    {
        const u8 *e = FST_ENTRY(h, cur);
        if (FST_IS_DIR(e))
            return 0;
        fi->handle = h;
        fi->startOffset = FST_DATA(e);
        fi->length = FST_SIZE(e);
    }
    return 1;
}

void *ARCGetStartAddrInMem(void *info)
{
    struct ShimARCFileInfo *fi = info;
    return (u8 *)fi->handle->archiveStartAddr + fi->startOffset;
}
