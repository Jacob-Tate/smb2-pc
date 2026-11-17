/* Dolphin OS shim: arena, heaps, time, reporting.
 * Heap algorithm: simple first-fit free list per heap, 32-byte aligned,
 * functionally equivalent to OSAlloc. */
#include "shim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "netplay/session.h"

/* ------------------------------------------------- arena ---- */
static u32 s_arena_lo, s_arena_hi;
static int s_arena_init;

static void arena_lazy_init(void)
{
    if (!s_arena_init) {
        s_arena_lo = gc_arena_lo_initial();
        s_arena_hi = gc_arena_hi_initial();
        s_arena_init = 1;
    }
}

u32 OSGetArenaLo(void) { arena_lazy_init(); return s_arena_lo; }
u32 OSGetArenaHi(void) { arena_lazy_init(); return s_arena_hi; }
void OSSetArenaLo(u32 lo) { arena_lazy_init(); s_arena_lo = lo; }
void OSSetArenaHi(u32 hi) { arena_lazy_init(); s_arena_hi = hi; }

void *OSAllocFromArenaHi(u32 size, u32 align)
{
    u32 p;
    arena_lazy_init();
    if (align == 0) align = 32;
    p = (s_arena_hi - size) & ~(align - 1);
    s_arena_hi = p;
    return (void *)(uintptr_t)p;
}

/* ------------------------------------------------- heaps ---- */
#define MAX_HEAPS 16
#define ALIGN32(x) (((x) + 31u) & ~31u)

struct HeapCell {
    struct HeapCell *prev;
    struct HeapCell *next;
    u32 size;             /* size including this header */
    u32 pad;              /* keep header 16 bytes, alloc result 32-aligned */
};

struct Heap {
    int in_use;
    struct HeapCell *free_list;  /* NULL = heap unused */
    struct HeapCell *alloc_list;
    u32 start, end;
};

static struct Heap s_heaps[MAX_HEAPS];

/* gx_core.c ("Texture lifetime"): textures decoded from memory that is
 * freed (OSFreeToHeap) or handed to a new heap (OSCreateHeap, OSInitAlloc)
 * are released */
extern void (*pc_tex_mem_freed_hook)(const void *p, u32 bytes);
static void tex_mem_freed(u32 start, u32 end)
{
    if (pc_tex_mem_freed_hook && end > start)
        pc_tex_mem_freed_hook((const void *)(uintptr_t)start, end - start);
}
static int s_current_heap = -1;
static int s_os_alloc_init;

/* ---- SMB2_HEAP_CHECK=1: overflow guards (docs/x64.md) ----
 * Every allocation gets a 16-byte 0xFD guard after the requested size
 * rounded up to 32 (the GC allocator's granularity; bitmap.c relies on it),
 * followed by the caller's return address.  All live cells of all heaps (and
 * the free-list headers) are checked on every alloc/free; the first damaged
 * one is reported with its allocation site (symbolize the address with
 * llvm-symbolizer / addr2line) and the process exits with code 3. */
#define HEAP_GUARD 16
#define HEAP_SITE (sizeof(void *) + sizeof(const char *) + sizeof(int))
static int s_heap_check = -1;
const char *pc_alloc_site_file; /* set by alloc_from_heap_or_panic */
int pc_alloc_site_line;

static int heap_check_on(void)
{
    if (s_heap_check < 0) {
        const char *e = pc_session_active() ? NULL : getenv("SMB2_HEAP_CHECK");   /* D13 */
        s_heap_check = (e && *e && *e != '0');
        if (s_heap_check)
            fprintf(stderr, "[heapcheck] enabled\n");
    }
    return s_heap_check;
}

static const char *heap_site_str(const char *f)
{
    extern char __ImageBase[];
    uintptr_t a = (uintptr_t)f, b = (uintptr_t)__ImageBase;
    return (f && a >= b && a < b + 0x10000000u) ? f : "?";
}

static void heap_check_all(const char *when)
{
    int i;
    for (i = 0; i < MAX_HEAPS; i++) {
        struct Heap *h = &s_heaps[i];
        struct HeapCell *c;
        if (!h->in_use)
            continue;
        for (c = h->alloc_list; c; c = c->next) {
            u8 *user = (u8 *)c + 32;
            u32 req = c->pad;
            u8 *g = user + ALIGN32(req); /* GC rounds sizes to 32: slack is legal */
            int k;
            if ((uintptr_t)c < h->start || (uintptr_t)c >= h->end) {
                fprintf(stderr, "[heapcheck] %s: heap %d alloc list corrupt at %p\n",
                        when, i, (void *)c);
                fflush(stderr);
                exit(3);
            }
            for (k = 0; k < HEAP_GUARD; k++) {
                if (g[k] != 0xFD) {
                    void *site; const char *file; int line;
                    memcpy(&site, g + HEAP_GUARD, sizeof site);
                    memcpy(&file, g + HEAP_GUARD + sizeof site, sizeof file);
                    memcpy(&line, g + HEAP_GUARD + sizeof site + sizeof file, sizeof line);
                    fprintf(stderr, "[heapcheck] %s: heap %d block %p (size 0x%X) "
                            "overrun at +0x%X (rounded); allocated by %s:0x%X (pc %p)\n", when, i,
                            (void *)user, req, req + k, heap_site_str(file), line, site);
                    fflush(stderr);
                    exit(3);
                }
            }
        }
        for (c = h->free_list; c; c = c->next) {
            uintptr_t a = (uintptr_t)c;
            if (a < h->start || a >= h->end || c->size < 32 ||
                a + c->size > h->end ||
                (c->next && (uintptr_t)c->next <= a)) {
                struct HeapCell *b, *best = NULL;
                fprintf(stderr, "[heapcheck] %s: heap %d free cell %p corrupt "
                        "(size 0x%X next %p)\n", when, i, (void *)c, c->size,
                        (void *)c->next);
                for (b = h->alloc_list; b; b = b->next)
                    if (b < c && (!best || b > best))
                        best = b;
                if (best) {
                    u8 *g = (u8 *)best + 32 + ALIGN32(best->pad);
                    const char *file; int line;
                    memcpy(&file, (u8 *)best + sizeof(struct HeapCell), sizeof file);
                    memcpy(&line, g + HEAP_GUARD + sizeof(void *) + sizeof file, sizeof line);
                    fprintf(stderr, "[heapcheck]   preceding live block %p size 0x%X "
                            "(ends %p) allocated by %s:0x%X\n", (void *)((u8 *)best + 32),
                            best->pad, (void *)((u8 *)best + best->size),
                            heap_site_str(file), line);
                }
                fflush(stderr);
                exit(3);
            }
        }
    }
}

/* Out-of-memory report: free cells and live blocks of one heap (with their
 * allocation sites when SMB2_HEAP_CHECK=1 recorded them). */
static void heap_dump(int heap)
{
    struct Heap *h = &s_heaps[heap];
    struct HeapCell *c;
    u32 nfree = 0, tfree = 0, nlive = 0, tlive = 0, maxfree = 0;

    for (c = h->free_list; c; c = c->next) {
        nfree++;
        tfree += c->size;
        if (c->size > maxfree)
            maxfree = c->size;
    }
    for (c = h->alloc_list; c; c = c->next) {
        nlive++;
        tlive += c->size;
    }
    fprintf(stderr, "[heap %d] 0x%08X-0x%08X size %u: %u live blocks %u bytes, "
            "%u free cells %u bytes (largest %u)\n", heap, h->start, h->end,
            h->end - h->start, nlive, tlive, nfree, tfree, maxfree);
    for (c = h->free_list; c; c = c->next)
        fprintf(stderr, "[heap %d]   free %p size %u\n", heap, (void *)c, c->size);
    for (c = h->alloc_list; c; c = c->next) {
        const char *file = NULL;
        int line = 0;
        void *site = NULL;
        if (heap_check_on()) {
            u8 *g = (u8 *)c + 32 + ALIGN32(c->pad);
            memcpy(&site, g + HEAP_GUARD, sizeof site);
            memcpy(&file, g + HEAP_GUARD + sizeof site, sizeof file);
            memcpy(&line, g + HEAP_GUARD + sizeof site + sizeof file, sizeof line);
        }
        fprintf(stderr, "[heap %d]   live %p size %u %s:%d (pc %p)\n", heap,
                (void *)((u8 *)c + 32), c->size, file ? heap_site_str(file) : "?",
                line, site);
    }
    fflush(stderr);
}

u32 OSInitAlloc(u32 lo, u32 hi, int maxHeaps)
{
    (void)maxHeaps;
    tex_mem_freed(lo, hi);
    memset(s_heaps, 0, sizeof(s_heaps));
    s_current_heap = -1;
    s_os_alloc_init = 1;
    /* real OSInitAlloc reserves heap bookkeeping at lo; ours is static */
    (void)hi;
    return lo;
}

int OSCreateHeap(u32 start, u32 end)
{
    int i;
    start = ALIGN32(start);
    end &= ~31u;
    for (i = 0; i < MAX_HEAPS; i++) {
        if (!s_heaps[i].in_use) {
            struct HeapCell *cell = (struct HeapCell *)(uintptr_t)start;
            tex_mem_freed(start, end);
            cell->prev = NULL;
            cell->next = NULL;
            cell->size = end - start;
            s_heaps[i].in_use = 1;
            s_heaps[i].free_list = cell;
            s_heaps[i].alloc_list = NULL;
            s_heaps[i].start = start;
            s_heaps[i].end = end;
            return i;
        }
    }
    fprintf(stderr, "OSCreateHeap: out of heaps\n");
    return -1;
}

void OSDestroyHeap(int heap)
{
    /* not a texture free: alloc.c destroys the main heap while keeping its
     * blocks (g_remake_initial_main_heap_somehow); memory is reused only
     * once a heap is created over it (OSCreateHeap) */
    if (heap >= 0 && heap < MAX_HEAPS)
        memset(&s_heaps[heap], 0, sizeof(s_heaps[heap]));
}

/* The game reads the SDK's current-heap variable directly in places. */
s32 currentHeap[2];

int OSSetCurrentHeap(int heap)
{
    int old = s_current_heap;
    s_current_heap = heap;
    currentHeap[0] = heap;
    return old;
}

void *OSAllocFromHeap(int heap, u32 size)
{
    struct Heap *h;
    struct HeapCell *cell;
    u32 need;

    if (heap < 0 || heap >= MAX_HEAPS || !s_heaps[heap].in_use) {
        fprintf(stderr, "OSAllocFromHeap: bad heap %d\n", heap);
        return NULL;
    }
    h = &s_heaps[heap];
    need = ALIGN32(size + 32); /* 32-byte header keeps 32-alignment */
    if (heap_check_on()) {
        heap_check_all("alloc");
        need = ALIGN32(ALIGN32(size) + 32 + HEAP_GUARD + HEAP_SITE);
    }

    for (cell = h->free_list; cell; cell = cell->next) {
        if (cell->size >= need) {
            struct HeapCell *alloc;
            if (cell->size - need >= 64) {
                /* split: keep the tail free */
                struct HeapCell *rest = (struct HeapCell *)((u8 *)cell + need);
                rest->size = cell->size - need;
                rest->prev = cell->prev;
                rest->next = cell->next;
                if (rest->prev) rest->prev->next = rest; else h->free_list = rest;
                if (rest->next) rest->next->prev = rest;
                cell->size = need;
            } else {
                if (cell->prev) cell->prev->next = cell->next; else h->free_list = cell->next;
                if (cell->next) cell->next->prev = cell->prev;
            }
            alloc = cell;
            alloc->prev = NULL;
            alloc->next = h->alloc_list;
            if (h->alloc_list) h->alloc_list->prev = alloc;
            h->alloc_list = alloc;
            if (heap_check_on()) {
                void *site = __builtin_return_address(0);
                u8 *g = (u8 *)alloc + 32 + ALIGN32(size);
                alloc->pad = size;
                memset(g, 0xFD, HEAP_GUARD);
                memcpy(g + HEAP_GUARD, &site, sizeof site);
                memcpy(g + HEAP_GUARD + sizeof site, &pc_alloc_site_file, sizeof(const char *));
                /* also in the header's spare 8 bytes (x64: 24-byte HeapCell),
                 * which survives overruns of the block's tail */
                if (sizeof(struct HeapCell) + sizeof(const char *) <= 32)
                    memcpy((u8 *)alloc + sizeof(struct HeapCell), &pc_alloc_site_file, sizeof(const char *));
                memcpy(g + HEAP_GUARD + sizeof site + sizeof(const char *), &pc_alloc_site_line, sizeof(int));
            }
            return (u8 *)alloc + 32;
        }
    }
    fprintf(stderr, "OSAllocFromHeap: heap %d out of memory (want %u)\n", heap, size);
    heap_dump(heap);
    return NULL;
}

void OSFreeToHeap(int heap, void *ptr)
{
    struct Heap *h;
    struct HeapCell *cell, *it, *prev;

    if (!ptr || heap < 0 || heap >= MAX_HEAPS || !s_heaps[heap].in_use)
        return;
    h = &s_heaps[heap];
    cell = (struct HeapCell *)((u8 *)ptr - 32);
    if (heap_check_on())
        heap_check_all("free");

    /* the GC SDK trusts the caller; we don't: a double-free or a free of a
     * pointer this heap never handed out would corrupt the free list, so
     * verify the cell is actually on the alloc list first */
    for (it = h->alloc_list; it && it != cell; it = it->next)
        ;
    if (!it) {
        fprintf(stderr, "OSFreeToHeap: heap %d does not own %p - ignored\n",
                heap, ptr);
        return;
    }

    tex_mem_freed((u32)(uintptr_t)ptr, (u32)(uintptr_t)cell + cell->size);

    /* unlink from alloc list */
    if (cell->prev) cell->prev->next = cell->next; else h->alloc_list = cell->next;
    if (cell->next) cell->next->prev = cell->prev;

    /* insert into free list sorted by address, coalescing */
    prev = NULL;
    for (it = h->free_list; it && it < cell; it = it->next)
        prev = it;
    cell->prev = prev;
    cell->next = it;
    if (prev) prev->next = cell; else h->free_list = cell;
    if (it) it->prev = cell;
    /* coalesce forward */
    if (it && (u8 *)cell + cell->size == (u8 *)it) {
        cell->size += it->size;
        cell->next = it->next;
        if (it->next) it->next->prev = cell;
    }
    /* coalesce backward */
    if (prev && (u8 *)prev + prev->size == (u8 *)cell) {
        prev->size += cell->size;
        prev->next = cell->next;
        if (cell->next) cell->next->prev = prev;
    }
}

/* GC game code frees through a one-argument function pointer
 * (os_free_to_heap_func_ptr) while currentHeap changes per scene, so the
 * pointer being freed often belongs to a *different* heap than the current
 * one.  Find the owning heap by address range. */
void pc_free_to_owning_heap(void *ptr)
{
    int i;
    uintptr_t a = (uintptr_t)ptr;

    if (!ptr)
        return;
    for (i = 0; i < MAX_HEAPS; i++) {
        if (s_heaps[i].in_use && a >= s_heaps[i].start && a < s_heaps[i].end) {
            OSFreeToHeap(i, ptr);
            return;
        }
    }
    fprintf(stderr, "pc_free_to_owning_heap: %p not in any heap - ignored\n", ptr);
}

u32 OSCheckHeap(int heap)
{
    struct Heap *h;
    struct HeapCell *cell;
    u32 free_total = 0;

    if (heap < 0 || heap >= MAX_HEAPS || !s_heaps[heap].in_use)
        return (u32)-1;
    h = &s_heaps[heap];
    /* Like the real SDK: usable bytes per free cell, header excluded.
     * alloc.c allocates exactly OSCheckHeap(heap) to relocate the main
     * heap, so including headers here makes that alloc fail by 32. */
    for (cell = h->free_list; cell; cell = cell->next)
        free_total += cell->size - 32;
    if (heap_check_on() && free_total >= 64) /* guard + site, 32-rounded */
        free_total -= 64;
    return free_total;
}

/* The game calls OSAlloc/OSFree through these in some places */
void *OSAlloc(u32 size) { return OSAllocFromHeap(s_current_heap, size); }
void OSFree(void *p) { OSFreeToHeap(s_current_heap, p); }

/* ------------------------------------------------- time ---- */
#define GC_TICK_HZ 40500000ull /* bus clock / 4 */

#ifdef _WIN32
unsigned long long shim_qpc_ns(void); /* defined in stubs_manual.c (needs windows.h) */
static u64 host_ns(void) { return shim_qpc_ns(); }
#else
static u64 host_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}
#endif

s64 OSGetTime(void)
{
    if (pc_session_active())   /* deterministic session: frame-derived (E1 D1) */
        return pc_session_os_time();
    /* Wall-clock based so save timestamps are real: GC epoch is 2000-01-01. */
    return (s64)(((u64)time(NULL) - 946684800ull) * GC_TICK_HZ);
}

u32 OSGetTick(void)
{
    if (pc_session_active())   /* E1 D2/D3: boat RNG seed, gpu frame time */
        return pc_session_os_tick();
    return (u32)(host_ns() * GC_TICK_HZ / 1000000000ull);
}

struct OSCalendarTime { /* matches SDK layout (s32 fields) */
    s32 sec, min, hour, mday, mon, year, wday, yday, msec, usec;
};

void OSTicksToCalendarTime(s64 ticks, struct OSCalendarTime *ct)
{
    time_t t = (time_t)(ticks / (s64)GC_TICK_HZ) + 946684800; /* GC epoch */
    struct tm tmv;
#ifdef _WIN32
    struct tm *p = gmtime(&t);
    if (p) tmv = *p; else memset(&tmv, 0, sizeof(tmv));
#else
    gmtime_r(&t, &tmv);
#endif
    ct->sec = tmv.tm_sec; ct->min = tmv.tm_min; ct->hour = tmv.tm_hour;
    ct->mday = tmv.tm_mday; ct->mon = tmv.tm_mon; ct->year = tmv.tm_year + 1900;
    ct->wday = tmv.tm_wday; ct->yday = tmv.tm_yday;
    ct->msec = 0; ct->usec = 0;
}

/* ------------------------------------------------- misc ---- */
void OSPanic(const char *file, int line, const char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "\nOSPanic: %s:%d: ", file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(NULL);
    exit(1);
}

void OSReport(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

int OSDisableInterrupts(void) { return 0; }
int OSRestoreInterrupts(int level) { (void)level; return 0; }

u32 OSGetConsoleSimulatedMemSize(void) { return GC_MEM_SIZE; }

u32 *OSGetStackPointer(void) { return NULL; }

/* REL linking: modules are statically linked into the exe; pretend success. */
int OSLink(void *module, void *bss) { (void)module; (void)bss; return 1; }
int OSUnlink(void *module) { (void)module; return 1; }

/* Fonts: only the debug/ROM font uses these; stubbed. */
u16 OSGetFontEncode(void) { return 0; } /* ANSI */
u32 OSInitFont(void *fontData) { (void)fontData; shim_log_once("OSInitFont"); return 0; }

static u32 s_sound_mode = 1; /* stereo */
u32 OSGetSoundMode(void) { return s_sound_mode; }
void OSSetSoundMode(u32 mode) { s_sound_mode = mode; }
u32 OSGetProgressiveMode(void) { return 0; }
void OSSetProgressiveMode(u32 m) { (void)m; }

/* Cache ops are no-ops on PC */
void DCFlushRange(void *addr, u32 size) { (void)addr; (void)size; }
void DCInvalidateRange(void *addr, u32 size) { (void)addr; (void)size; }
void DCStoreRange(void *addr, u32 size) { (void)addr; (void)size; }
void ICInvalidateRange(void *addr, u32 size) { (void)addr; (void)size; }

/* ------------------------------------------------- logging ---- */
void shim_log_once(const char *sym)
{
    /* crude: linear table of seen names */
    static const char *seen[1024];
    static int n;
    int i;
    for (i = 0; i < n; i++)
        if (seen[i] == sym) return;
    if (n < 1024) seen[n++] = sym;
    printf("[shim-stub] %s\n", sym);
}

void shim_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
