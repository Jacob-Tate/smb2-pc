/* GameCube memory-map emulation.
 *
 * The game's heap lives in the "arena", a region of GC main RAM
 * (0x80000000..0x81800000).  Some decompiled code reads hardware globals at
 * absolute low-memory addresses (e.g. *(u32 *)0x800000F8 = bus clock), so we
 * try to map the whole 24 MiB block at the real address.  The exe is linked
 * with --large-address-aware, so 0x80000000 is valid user space for a 32-bit
 * process on 64-bit Windows. */
#include "shim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static u8 *s_gc_mem;      /* host pointer to the 24MiB block */
static int s_mapped_real; /* 1 if s_gc_mem == (u8*)GC_MEM_BASE */

/* Low-memory globals the game reads directly (offsets into main RAM) */
#define OS_BASE_CACHED      0x80000000u
#define OS_BUS_CLOCK_OFS    0x000000F8u  /* u32: bus clock, Hz */
#define OS_CORE_CLOCK_OFS   0x000000FCu  /* u32: CPU clock, Hz */
#define OS_SIMMEM_OFS       0x000000F0u  /* u32: simulated memory size */
#define OS_PHYSMEM_OFS      0x00000028u  /* u32: physical memory size */

#define GC_BUS_CLOCK   162000000u
#define GC_CORE_CLOCK  486000000u

#ifdef _WIN32
/* Reserve the GC RAM range before anything else runs: in a 64-bit process
 * the GL driver and SDL allocate bottom-up and can take 0x80000000 before
 * gc_mem_init() runs. */
__attribute__((constructor)) static void gc_mem_reserve_early(void)
{
    VirtualAlloc((void *)(uintptr_t)GC_MEM_BASE, GC_MEM_SIZE, MEM_RESERVE,
                 PAGE_READWRITE);
}
#endif

int gc_mem_init(void)
{
#ifdef _WIN32
    s_gc_mem = VirtualAlloc((void *)(uintptr_t)GC_MEM_BASE, GC_MEM_SIZE,
                            MEM_COMMIT, PAGE_READWRITE);
    if (!s_gc_mem) /* not reserved early: try a fresh reservation */
        s_gc_mem = VirtualAlloc((void *)(uintptr_t)GC_MEM_BASE, GC_MEM_SIZE,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (s_gc_mem == (u8 *)(uintptr_t)GC_MEM_BASE) {
        s_mapped_real = 1;
    } else {
#ifdef _WIN64
        /* The x64 build relies on every game heap address fitting in 32
         * bits (docs/x64.md); a relocated block may not. */
        fprintf(stderr, "FATAL: could not map GC RAM at 0x80000000 (err %lu)\n",
                (unsigned long)GetLastError());
        exit(1);
#endif
        fprintf(stderr,
                "WARNING: could not map GC RAM at 0x80000000 (err %lu); "
                "absolute-address reads will crash\n",
                (unsigned long)GetLastError());
        s_gc_mem = malloc(GC_MEM_SIZE);
        s_mapped_real = 0;
    }
#else
    s_gc_mem = malloc(GC_MEM_SIZE);
    s_mapped_real = ((u32)(uintptr_t)s_gc_mem == GC_MEM_BASE);
#endif
    if (!s_gc_mem) {
        fprintf(stderr, "FATAL: cannot allocate GC main RAM block\n");
        exit(1);
    }
    memset(s_gc_mem, 0, GC_MEM_SIZE);
    { /* E4/T5 test hook: SMB2_TEST_FILL=<hex byte> pre-fills the arena to expose uninitialised reads */
      const char *tf = getenv("SMB2_TEST_FILL");
      if (tf && *tf) memset(s_gc_mem, (int)strtol(tf, NULL, 16), GC_MEM_SIZE); }

    /* Populate low-memory OS globals (big-endian on GC, but the game reads
     * them through normal loads, so native endianness is what it sees). */
    *(u32 *)(s_gc_mem + OS_BUS_CLOCK_OFS)  = GC_BUS_CLOCK;
    *(u32 *)(s_gc_mem + OS_CORE_CLOCK_OFS) = GC_CORE_CLOCK;
    *(u32 *)(s_gc_mem + OS_SIMMEM_OFS)     = GC_MEM_SIZE;
    *(u32 *)(s_gc_mem + OS_PHYSMEM_OFS)    = GC_MEM_SIZE;
    memcpy(s_gc_mem, "GM2E8P", 6); /* disc ID at 0x80000000 */

    printf("GC RAM: 24MiB at %p (%s)\n", (void *)s_gc_mem,
           s_mapped_real ? "real GC address" : "RELOCATED - absolute reads unsafe");
    return s_mapped_real;
}

u8 *gc_mem_ptr(u32 gc_addr)
{
    return s_gc_mem + (gc_addr - GC_MEM_BASE);
}

/* The DOL's static image would occupy the bottom of RAM; SMB2's arena starts
 * above the DOL bss.  Our game's statics live in the exe image instead, so
 * everything above the low-mem globals is free; keep the authentic-ish split. */
u32 gc_arena_lo_initial(void)
{
    return (u32)(uintptr_t)(s_gc_mem + 0x00503100); /* ~matches retail ArenaLo */
}

u32 gc_arena_hi_initial(void)
{
    return (u32)(uintptr_t)(s_gc_mem + GC_MEM_SIZE - 0x10000);
}

/* ---- 64-bit port: addresses must fit in 32 bits (docs/x64.md) ---- */
#include "pc_addr32.h"

#if __SIZEOF_POINTER__ > 4
static void pc_check_low(const char *what, const void *p)
{
    if ((uintptr_t)p > 0xFFFFFFFFu) {
        fprintf(stderr, "FATAL: %s at %p is above 4 GiB; the x64 build "
                        "needs all game memory below 4 GiB\n", what, p);
        exit(1);
    }
}
#endif

void pc_check_address_space(void)
{
#if __SIZEOF_POINTER__ > 4
    static int s_static;
    int local;
    void *heap = malloc(64);

    pc_check_low("exe image (static data)", &s_static);
    pc_check_low("exe image (code)", (const void *)(uintptr_t)&pc_check_address_space);
    pc_check_low("GC RAM", s_gc_mem);
    pc_check_low("GC RAM end", s_gc_mem + GC_MEM_SIZE - 1);
    /* Stack and CRT heap are only reported: game code is not expected to
     * keep their addresses in 32-bit words, but it is worth knowing. */
    printf("x64 address check: image %p, stack %p, malloc %p, GC RAM %p\n",
           (void *)&s_static, (void *)&local, heap, (void *)s_gc_mem);
    free(heap);
#endif
}

#if PC_ADDR32_FIXUPS
void pc_apply_addr32_fixups(const struct PcAddr32Fixup *f, int n, const char *what)
{
    int i;

    for (i = 0; i < n; i++) {
        uintptr_t a = (uintptr_t)f[i].target;
        if (a > 0xFFFFFFFFu) {
            fprintf(stderr, "FATAL: %s fixup %d: address %p does not fit in "
                            "32 bits\n", what, i, f[i].target);
            exit(1);
        }
        *f[i].slot = (u32)a;
    }
}
#endif
