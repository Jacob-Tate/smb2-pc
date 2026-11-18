/* Hand-written implementations for symbols the auto-generated stub file
 * (stubs_gen.c) must not cover with generic stubs. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void shim_log_once(const char *sym);

/* ---- monotonic clock for os_shim's host_ns() ---- */
#ifdef _WIN32
#include <windows.h>
#ifdef _WIN64
/* Crash/watchdog dumps below name the 32-bit registers; on x64 read the
 * 64-bit ones (printed truncated to 32 bits -- fine below 4 GiB, which is
 * where all game memory lives, see docs/x64.md). */
#define Eax Rax
#define Ebx Rbx
#define Ecx Rcx
#define Edx Rdx
#define Esi Rsi
#define Edi Rdi
#define Ebp Rbp
#define Esp Rsp
#define Eip Rip
#endif
unsigned long long shim_qpc_ns(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER c;
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c);
    return (unsigned long long)((double)c.QuadPart * 1e9 / (double)freq.QuadPart);
}
#endif

/* ---- DOL libc-ish helpers ---- */
void memcpy2(void *dest, void *src, uint32_t size) { memcpy(dest, src, size); }

int return_1(void *info) { (void)info; return 1; }

/* ARCFileInfo = { handle, startOffset, length } */
uint32_t arcGetLength(void *info) { return ((uint32_t *)info)[2]; }

void g_very_similar_to_puts(const char *s)
{
    if (s) { fputs(s, stdout); fflush(stdout); }
}

/* ---- memory card: see card_shim.c (file-backed Slot A) ---- */

/* ---- misc hardware ---- */
void AIInit(void *stack) { (void)stack; }
void hwExit(void) { }

/* Globals set by the skipped DOL init (g_init_console, asm 0x80006840):
 * the console refresh rate as int and float (adv_demo fn_1_10FA8 divides
 * by it). */
int lbl_80198B38 = 60;
float lbl_80198B34 = 60.0f;
int lbl_80198B3C = 0;

/* ---- crash diagnostics: print the faulting address, then die ---- */
#ifdef _WIN32
static LONG WINAPI shim_crash_filter(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    CONTEXT *c = ep->ContextRecord;
    unsigned long *sp;
    int i;

    fprintf(stderr, "\n*** CRASH: exception 0x%08lX at %p",
            (unsigned long)er->ExceptionCode, er->ExceptionAddress);
    if (er->ExceptionCode == 0xC0000005 /* ACCESS_VIOLATION */ &&
        er->NumberParameters >= 2) {
        fprintf(stderr, " (%s %p)",
                er->ExceptionInformation[0] ? "write to" : "read of",
                (void *)(uintptr_t)er->ExceptionInformation[1]);
    }
    fprintf(stderr, " ***\n");
    fprintf(stderr, "module base: 0x%08lX (nm assumes 0x00400000)\n",
            (unsigned long)(uintptr_t)GetModuleHandle(NULL));
#ifdef _WIN64
    fprintf(stderr, "rax=%016llX rbx=%016llX rcx=%016llX rdx=%016llX\n"
                    "rsi=%016llX rdi=%016llX rbp=%016llX rsp=%016llX rip=%016llX\n"
                    "r8 =%016llX r9 =%016llX r10=%016llX r11=%016llX\n"
                    "r12=%016llX r13=%016llX r14=%016llX r15=%016llX\n",
            c->Rax, c->Rbx, c->Rcx, c->Rdx, c->Rsi, c->Rdi, c->Rbp, c->Rsp, c->Rip,
            c->R8, c->R9, c->R10, c->R11, c->R12, c->R13, c->R14, c->R15);
#else
    fprintf(stderr, "eax=%08lX ebx=%08lX ecx=%08lX edx=%08lX\n"
                    "esi=%08lX edi=%08lX ebp=%08lX esp=%08lX eip=%08lX\n",
            c->Eax, c->Ebx, c->Ecx, c->Edx,
            c->Esi, c->Edi, c->Ebp, c->Esp, c->Eip);
#endif
    /* raw stack dump: symbolize offline against nm output */
    sp = (unsigned long *)c->Esp;
    for (i = 0; i < 128; i++) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(sp + i, &mbi, sizeof mbi) ||
            mbi.State != MEM_COMMIT || !(mbi.Protect & 0xEE))
            break;
        fprintf(stderr, "stack[%03d] %08lX%s", i, (unsigned long)sp[i],
                (i % 4 == 3) ? "\n" : "  ");
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    fflush(stdout);
    return EXCEPTION_EXECUTE_HANDLER; /* terminate */
}

#ifdef _WIN64
/* ---- PowerPC integer divide-by-zero semantics (docs/x64.md, X1) ----
 * On the GameCube divw/divwu by zero does not trap: the quotient is
 * undefined (hardware: -1 for a negative dividend else 0) and MWCC computes
 * a % b as a - (a / b) * b, which is the dividend whatever the quotient was.
 * x86 idiv/div traps (0xC0000094), so game code that divides by zero
 * (Mode Select's cursor code on entry-less screens, sel_ngc.c fn_3_32E0)
 * would kill the process.  This vectored handler decodes the faulting
 * F6/F7 /6 /7 instruction, stores quotient = 0 (or -1 for a negative signed
 * dividend) and remainder = dividend, and resumes after it.  Deterministic:
 * the result depends only on the operands.  Anything it cannot decode falls
 * through to the crash filter. */
static volatile LONG s_div0_count;
static LONG CALLBACK shim_div0_handler(EXCEPTION_POINTERS *ep)
{
    CONTEXT *c = ep->ContextRecord;
    const unsigned char *p = (const unsigned char *)(uintptr_t)c->Rip;
    int opsz = 32, rex = 0, sgn, len, mod, rm, n;
    uint64_t a, q, r, mask;

    if (ep->ExceptionRecord->ExceptionCode != 0xC0000094)
        return EXCEPTION_CONTINUE_SEARCH;
    for (n = 0; n < 4; n++) {             /* legacy prefixes */
        if (*p == 0x66) opsz = 16;
        else if (*p != 0x67 && *p != 0xF2 && *p != 0xF3 && *p != 0x2E &&
                 *p != 0x3E && *p != 0x26 && *p != 0x36 && *p != 0x64 && *p != 0x65)
            break;
        p++;
    }
    if ((*p & 0xF0) == 0x40) {
        rex = *p++;
        if (rex & 8) opsz = 64;
    }
    if ((*p != 0xF6 && *p != 0xF7) || ((p[1] >> 3) & 7) < 6)
        return EXCEPTION_CONTINUE_SEARCH;
    if (*p == 0xF6) opsz = 8;
    sgn = ((p[1] >> 3) & 7) == 7;
    mod = p[1] >> 6; rm = p[1] & 7;
    len = 2;                              /* opcode + modrm */
    if (mod != 3) {
        if (rm == 4) { len++; if (mod == 0 && (p[2] & 7) == 5) len += 4; }
        if (mod == 0 && rm == 5) len += 4;
        else if (mod == 1) len += 1;
        else if (mod == 2) len += 4;
    }
    len += (int)(p - (const unsigned char *)(uintptr_t)c->Rip);

    mask = opsz == 64 ? ~0ull : ((1ull << opsz) - 1);
    a = (opsz == 8 ? c->Rax & 0xFF : c->Rax) & mask;
    q = (sgn && (a >> (opsz - 1)) & 1) ? mask : 0;
    r = a;
    if (opsz == 8) {                      /* AL = quotient, AH = remainder */
        c->Rax = (c->Rax & ~0xFFFFull) | (q & 0xFF) | ((r & 0xFF) << 8);
    } else if (opsz == 16) {
        c->Rax = (c->Rax & ~0xFFFFull) | q;
        c->Rdx = (c->Rdx & ~0xFFFFull) | r;
    } else {                              /* 32-bit ops zero-extend */
        c->Rax = q;
        c->Rdx = r;
    }
    c->Rip += len;
    if (InterlockedIncrement(&s_div0_count) <= 8)
        fprintf(stderr, "[ppc-div0] %s divide by zero at %p (dividend %lld, %d-bit): "
                "PPC result q=%lld r=dividend, resumed\n", sgn ? "signed" : "unsigned",
                (void *)(uintptr_t)(c->Rip - len), (long long)a, opsz, (long long)q);
    return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

void shim_install_crash_handler(void)
{
    SetUnhandledExceptionFilter(shim_crash_filter);
#ifdef _WIN64
    AddVectoredExceptionHandler(1, shim_div0_handler);
#endif
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}
#else
void shim_install_crash_handler(void) { }
#endif

/* ---- GC locked cache (16KB L1 scratch) ----
 * The DOL carves ViewMatrices (ord table state) and friends out of this
 * region; main_loop.c stores the returned pointer in g_ord_tbl_stuff[0]
 * and camera.c/polydisp.c write matrices through it. */
static unsigned char s_locked_cache[0x4000] __attribute__((aligned(32)));
void *g_something_with_locked_cache(int mode)
{
    (void)mode;
    return s_locked_cache;
}
void fn_80064FE0(void *p) { (void)p; }  /* release locked cache */

void g_something_with_locked_cache_2(void *dest, int value, uint32_t size)
{
    memset(dest, value, size);
}

/* ---- hang watchdog: if video_finish_frame stops being reached, suspend the
 * main thread, print its eip + stack (symbolize offline like a crash), die. */
#ifdef _WIN32
volatile long g_shim_frame_pulse;
extern volatile long g_shim_watchdog_hold;   /* pc_file_dialog.c: a system dialog is open */
static HANDLE s_main_thread;

static DWORD WINAPI shim_watchdog_main(LPVOID arg)
{
    long last = -1;
    int quiet = 0;
    (void)arg;
    for (;;) {
        Sleep(2000);
        if (g_shim_frame_pulse != last || g_shim_watchdog_hold > 0) {
            last = g_shim_frame_pulse;
            quiet = 0;
            continue;
        }
        if (++quiet < 4) /* 8s without a frame */
            continue;
        /* sample eip repeatedly first: distinguishes a tight spin at one
         * address from a wider loop through many call sites */
        {
            int s;
            for (s = 0; s < 16; s++) {
                CONTEXT sc;
                memset(&sc, 0, sizeof sc);
                sc.ContextFlags = CONTEXT_CONTROL;
                SuspendThread(s_main_thread);
                if (GetThreadContext(s_main_thread, &sc))
                    fprintf(stderr, "[wd sample %02d] eip=%08lX esp=%08lX\n",
                            s, sc.Eip, sc.Esp);
                ResumeThread(s_main_thread);
                Sleep(100);
            }
            fflush(stderr);
        }
        SuspendThread(s_main_thread);
        {
            CONTEXT c;
            unsigned long *sp;
            int i;
            memset(&c, 0, sizeof c);
            c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
            if (GetThreadContext(s_main_thread, &c)) {
                fprintf(stderr, "\n*** WATCHDOG: no frame in 8s; main thread at eip=%08lX ***\n", c.Eip);
                fprintf(stderr, "module base: 0x%08lX (nm assumes 0x00400000)\n",
                        (unsigned long)(uintptr_t)GetModuleHandle(NULL));
                fprintf(stderr, "eax=%08lX ebx=%08lX ecx=%08lX edx=%08lX\n"
                                "esi=%08lX edi=%08lX ebp=%08lX esp=%08lX\n",
                        c.Eax, c.Ebx, c.Ecx, c.Edx, c.Esi, c.Edi, c.Ebp, c.Esp);
                sp = (unsigned long *)c.Esp;
                for (i = 0; i < 128; i++) {
                    MEMORY_BASIC_INFORMATION mbi;
                    if (!VirtualQuery(sp + i, &mbi, sizeof mbi) ||
                        mbi.State != MEM_COMMIT || !(mbi.Protect & 0xEE))
                        break;
                    fprintf(stderr, "stack[%03d] %08lX%s", i, (unsigned long)sp[i],
                            (i % 4 == 3) ? "\n" : "  ");
                }
                fprintf(stderr, "\n");
            }
            fflush(stderr);
            fflush(stdout);
        }
        ExitProcess(6);
    }
}

void shim_start_watchdog(void)
{
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                    GetCurrentProcess(), &s_main_thread,
                    0, FALSE, DUPLICATE_SAME_ACCESS);
    CreateThread(NULL, 0, shim_watchdog_main, NULL, 0, NULL);
}
#else
void shim_start_watchdog(void) { }
#endif
