/* Minimal runtime for the SMB2_SANITIZE_BOUNDS test build (CMake option,
 * off by default).
 *
 * The game TUs are compiled with -fsanitize=array-bounds,object-size (clang)
 * or -fsanitize=bounds,object-size (GCC) in recover mode, which emits calls
 * to the __ubsan_handle_* entry points below instead of trapping.  No
 * compiler-rt / libubsan is linked: these handlers log each failing source
 * location (first few hits, then a count) to stderr as "[oob] ..." lines and
 * let the game continue, so one run collects every report.
 *
 * The data layouts are the stable ubsan ABI shared by clang and GCC
 * (compiler-rt ubsan_handlers.h).  This file itself must not be
 * instrumented (CMakeLists.txt adds -fno-sanitize=all). */
#ifdef SMB2_SANITIZE_BOUNDS
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct { const char *file; uint32_t line, col; } UbLoc;
typedef struct { uint16_t kind, info; char name[1]; } UbType;
typedef struct { UbLoc loc; const UbType *array_type, *index_type; } UbOobData;
typedef struct { UbLoc loc; const UbType *type; uint8_t log_align, check_kind; } UbMismatchData;
typedef struct { UbLoc loc; } UbPtrOvfData;

extern long g_dbg_frame_counter; /* platform_sdl2.c: frame number of the dumps */

#define SITE_MAX 4096
#define REPORT_PER_SITE 3
static struct { const char *file; uint32_t line, col; char kind; long hits; } s_sites[SITE_MAX];
static int s_nsites;
static int s_summary_armed;

static const char *short_file(const char *f)
{
    const char *p, *best = f;
    /* drop everything before src/ (decomp or pc_port) */
    for (p = f; *p; p++)
        if ((p[0] == 's' && p[1] == 'r' && p[2] == 'c' && (p[3] == '/' || p[3] == '\\')) &&
            (p == f || p[-1] == '/' || p[-1] == '\\'))
            best = p + 4;
    return best;
}

static void summary(void)
{
    int i;
    fprintf(stderr, "[oob] summary: %d site(s)\n", s_nsites);
    for (i = 0; i < s_nsites; i++)
        fprintf(stderr, "[oob]   %c %s:%u:%u hits=%ld\n", s_sites[i].kind,
                short_file(s_sites[i].file), s_sites[i].line, s_sites[i].col,
                s_sites[i].hits);
    fflush(stderr);
}

/* returns the hit number of this site (1-based), 0 = table full */
static long site_hit(const UbLoc *loc, char kind)
{
    int i;
    if (!s_summary_armed) {
        s_summary_armed = 1;
        atexit(summary);
#ifdef _WIN32
        /* addresses below are absolute; the 32-bit exe is relocatable */
        fprintf(stderr, "[oob] module base %p\n", (void *)GetModuleHandle(NULL));
#endif
    }
    for (i = 0; i < s_nsites; i++)
        if (s_sites[i].line == loc->line && s_sites[i].col == loc->col &&
            s_sites[i].kind == kind && s_sites[i].file == loc->file)
            return ++s_sites[i].hits;
    if (s_nsites >= SITE_MAX)
        return 0;
    s_sites[s_nsites].file = loc->file;
    s_sites[s_nsites].line = loc->line;
    s_sites[s_nsites].col = loc->col;
    s_sites[s_nsites].kind = kind;
    s_sites[s_nsites].hits = 1;
    s_nsites++;
    return 1;
}

static long long int_value(const UbType *t, uintptr_t v)
{
    unsigned bits = 1u << (t->info >> 1);
    int is_signed = t->info & 1;
    if (t->kind != 0 || bits > sizeof(uintptr_t) * 8)
        return (long long)v;
    if (bits < 64) {
        uint64_t m = (1ull << bits) - 1;
        uint64_t x = (uint64_t)v & m;
        if (is_signed && (x >> (bits - 1)))
            return (long long)(x | ~m);
        return (long long)x;
    }
    return is_signed ? (long long)(int64_t)v : (long long)(uint64_t)v;
}

void __ubsan_handle_out_of_bounds(UbOobData *d, uintptr_t index)
{
    long n = site_hit(&d->loc, 'B');
    if (n >= 1 && n <= REPORT_PER_SITE)
        fprintf(stderr, "[oob] bounds %s:%u:%u index %lld out of range for %s (frame %ld, caller %p)\n",
                short_file(d->loc.file), d->loc.line, d->loc.col,
                int_value(d->index_type, index), d->array_type->name,
                g_dbg_frame_counter, __builtin_return_address(0));
}
void __ubsan_handle_out_of_bounds_abort(UbOobData *d, uintptr_t index)
{
    __ubsan_handle_out_of_bounds(d, index);
}

/* With only object-size enabled this fires for accesses past the end of the
 * whole object the pointer was derived from (the cross-object class). */
void __ubsan_handle_type_mismatch_v1(UbMismatchData *d, uintptr_t ptr)
{
    static const char *const kinds[] = { "load of", "store to", "reference binding to",
        "member access within", "member call on", "constructor call on",
        "downcast of", "downcast of", "upcast of", "cast to virtual base of",
        "_Nonnull binding to", "dynamic operation on" };
    long n = site_hit(&d->loc, 'O');
    if (n >= 1 && n <= REPORT_PER_SITE)
        fprintf(stderr, "[oob] objsize %s:%u:%u %s %p (type %s) past its object (frame %ld, caller %p)\n",
                short_file(d->loc.file), d->loc.line, d->loc.col,
                d->check_kind < sizeof kinds / sizeof kinds[0] ? kinds[d->check_kind] : "access",
                (void *)ptr, d->type->name, g_dbg_frame_counter,
                __builtin_return_address(0));
}
void __ubsan_handle_type_mismatch_v1_abort(UbMismatchData *d, uintptr_t ptr)
{
    __ubsan_handle_type_mismatch_v1(d, ptr);
}

void __ubsan_handle_pointer_overflow(UbPtrOvfData *d, uintptr_t base, uintptr_t result)
{
    long n = site_hit(&d->loc, 'P');
    if (n >= 1 && n <= REPORT_PER_SITE)
        fprintf(stderr, "[oob] ptrovf %s:%u:%u %p -> %p (frame %ld)\n",
                short_file(d->loc.file), d->loc.line, d->loc.col,
                (void *)base, (void *)result, g_dbg_frame_counter);
}
void __ubsan_handle_pointer_overflow_abort(UbPtrOvfData *d, uintptr_t base, uintptr_t result)
{
    __ubsan_handle_pointer_overflow(d, base, result);
}
#endif
