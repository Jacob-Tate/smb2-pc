# Deterministic math (all builds bit-identical)

Game physics (Monkey Boat, Monkey Dogfight, ball physics, AI) must give the
same results on every build of the port: the 32-bit GCC/msvcrt build, the
x64 clang/UCRT build, and future Linux, macOS or ARM64 builds. The C runtimes
disagree in the last bit of functions like `sinf` and `atan2f`, and one such
bit is enough for physics runs to diverge within a few hundred frames.

## Rules

1. **Game code never calls the C runtime's libm, `rand`, `srand`, `qsort` or `atof`.**
   `include/pc_detmath.h` is force-included (`-include`) into every game TU:
   the decomp sources (`smb2_game`), the shim layer (`smb2_shim`), and
   `src/render/gx_core.c`, which builds the projection and light matrices
   that the game reads back. The header includes `<math.h>` and `<stdlib.h>`
   first, then maps the calls to `pc_*` functions in `src/detmath/`
   (library `smb2_detmath`):
   - Math names (`sinf(x)` → `pc_sinf(x)`) are function-like macros, so
     variables and fields called `floor` or `tan` are left alone.
   - `rand`/`srand`/`qsort`/`atof` are object-like macros, so the decomp's own
     `int rand(void);` declarations are renamed along with the calls.
   - A transcendental function that isn't implemented (`exp`, `log`,
     `sinh`, `hypot`, `sincos`, …) maps to an undefined
     `pc_libm_missing_*` symbol. A new use then fails at link time instead
     of silently going back to the CRT.
2. **Arithmetic is IEEE single/double with no excess precision and no
   contraction.**
   - i686 uses `-msse2 -mfpmath=sse` (x87 excess precision changes game
     results, e.g. the boat race start).
   - All targets use `-ffp-contract=off`. GCC defaults to `fast` and clang
     to `on`. Baseline x86/x86-64 has no FMA, but ARM64 and
     `-march=native` builds do: with contraction on, aarch64 clang emits
     about 150 FMAs in fdlibm alone (see the test below).
   - `pc_detmath.h` and `pc_libm.c` reject `__FAST_MATH__` and any
     `FLT_EVAL_METHOD` other than 0 with `#error`.
   - Float→double promotions follow C rules and are identical on both
     compilers.
   - No `long double` is used in game code.

## What `src/detmath` provides

| Function | Implementation |
|---|---|
| `sin cos tan atan atan2 pow floor` (double) | fdlibm 5.3. The sources are copied **unchanged** from SDL 2.30.9 `src/libm` (uClibc's fdlibm, Sun license header kept). SDL's headers are replaced by `fdlibm/math_libm.h` (renames to `pc_*`), `fdlibm/math_private.h` (word access via 64-bit shifts, so it doesn't depend on endianness) and a no-op `SDL_assert.h`. |
| `asin acos` (double) | fdlibm `e_asin.c` / `e_acos.c`, written out (SDL has no copy). The constants were checked against the GameCube's `__ieee754_acos` in main.dol. |
| `ceil` | `-floor(-x)` (exact) |
| `sqrt sqrtf` | The hardware instruction (`sqrtsd`/`sqrtss`/`fsqrt`, via `__builtin_sqrt` with `-fno-math-errno`). IEEE 754 requires it to be correctly rounded, so it is identical everywhere. NaN results are canonicalized. |
| `sinf cosf tanf asinf acosf atanf atan2f powf floorf ceilf` | `(float)f((double)x)`, which is exactly how the GameCube's MSL implements them (main.dol `sinf` etc. are 0x24-byte wrappers around the double fdlibm routines). |
| `lrint lrintf` | Round-half-even via ±2^52, saturating like PPC `fctiw` (NaN → INT32_MIN). The result is the same whatever `sizeof(long)` is. |
| `rand srand` | The GameCube MSL LCG (main.dol 0x80071F90): `x = x*1103515245 + 12345`, return `(x >> 16) & 0x7FFF`, seed 1. msvcrt/UCRT use 214013/2531011 and glibc uses something else again. |
| `qsort` | The GameCube MSL heapsort (main.dol 0x80071E18), transcribed from the asm. Equal keys therefore end up in the same order on every build and on the GC (boat and race rankings sort with ties). Counts are 32-bit like on the GC; the decomp passes `int`/`u32`. |
| `atof` | Decimal parser (dialog.c reads `\t<seconds>` timings from the script files). Up to 19 significant digits go into an integer, trailing zeros are stripped, then one multiply or divide by an exact power of ten. For fewer than 2^53 and exponents within ±22 (all game data) this is correctly rounded, the same as any correct strtod. Outside that range it is still deterministic. |

`fabs`/`fabsf` stay native: they are exact, and compilers inline them.

The fdlibm results are at most 1 ulp from glibc's (correctly rounded) results
over the whole float range.

## Remaining C runtime math

`libsmb2_game.a` and `libsmb2_shim.a` reference only `pc_*` (check with
`llvm-nm -u`), plus `fabs` on x64. The exes still import libm for SDL,
miniaudio and the renderers, which don't feed game state.

## GameCube mathutil (GC-exact trig)

On the GameCube, mathutil's trig is hand-written Sega asm in main.dol and
uses no libm at all:

- `math_sin`, `math_sin_cos_v` and the `mtxa_rotate_*` helpers read a
  16385-entry quarter-wave float table at 0x8011BF60.
- `math_tan` reads a 16385-entry table at 0x8012BF80.
- `math_atan2` and `math_atan` reduce to a ratio r in [0,1) and evaluate
  `fctiw(fmadds(r, a[i], b[i]))`, where i = floor(r·4096), from a
  slope/intercept table at 0x8013BFA0. Then they fold the octant.

`src/game/shim/mathutil_tables_gen.c` (generated by
`tools/gen_mathutil_tables.py` from main.dol) holds these tables. mathutil.c
reproduces the asm exactly:
- table indexing and quadrant/octant folding;
- `fnabs` giving -0.0;
- `fmadds` computed as a correctly rounded fused single-precision operation
  in portable C;
- the asm's special cases for zero, infinity and NaN.

So these five functions are bit-identical to the GameCube, not just across
PC builds.

`math_sqrt`/`math_rsqrt` use IEEE sqrt. The GC refines a hardware
`frsqrte` estimate with two single-precision Newton steps, so it can differ
from IEEE by an ulp.

## Verification

- `tools/detmath_test/build.sh <outdir>` builds `detmath_test` with four
  toolchains: i686 GCC/msvcrt (the 32-bit game build), x86-64 clang/UCRT
  (the x64 game build), i686 clang/UCRT and Linux GCC/glibc. It also
  compiles the library for aarch64 and counts FMA instructions.
- Run each binary and diff the lines starting with `pc`:
  - The test covers about 70M float inputs per unary function (every 61st
    bit pattern, plus edge values and all 65536 s16 angles), 20M random
    pairs for `atan2f`/`powf`, 10M random doubles per double function, 2M
    `rand` values, and 2000 `qsort` runs with many ties.
  - The `crt` lines show how much the C runtimes disagree with each other
    on the same inputs.
- `build.sh <outdir> <decomp_dir>` also builds `mathutil_test_*.exe`. These
  hash the GC-table `math_sin`/`math_tan`/`math_sin_cos_v` for all 65536
  angles, and `math_atan2`/`math_atan` for 20M random pairs plus round
  trips. They also compare against libm: `math_atan2` is within 1 s16 unit
  of the rounded `atan2` everywhere, and differs by exactly 1 unit in about
  0.007% of cases.
- Game runs: with pinned seeds, gl32 and gl64 must produce the same frames
  in dogfight and boat (see the x64 test harness, `tools/x64test/modes.sh`).

Expected results: every `pc` line is identical across the four toolchains
and `pc_*` is within 1 ulp of glibc, while the `crt` lines differ (msvcrt vs
UCRT `sinf` on tens of thousands of the 70M inputs). aarch64 must show 0 FMAs
in `src/detmath`. Pinned-seed gl32/gl64 game runs differ only by ±1 rendering
noise along polygon edges (GL driver), never in game state.

## Out-of-range float→integer conversions

C leaves a float→integer cast undefined when the value doesn't fit the
target type.
- On the GC, `fctiwz` saturates to s32, and storing to a u8/s16 field then
  keeps the low bits (wraps).
- GCC (i686) happens to do the same.
- Clang exploits the UB: `(s8)(0.98f * s16val)` compiles to a 16-bit
  conversion with no 8-bit wrap. Code that relies on the wrap converts
  through s32 first: `(s8)(s32)(...)` (e.g. the story stage-select balls in
  `scen_disp.c`).
- `-fno-strict-float-cast-overflow` doesn't help: it saturates to the narrow
  type, which matches neither the GC nor GCC.

To find sites, build x64 with
`CFLAGS=-fsanitize=float-cast-overflow LDFLAGS=-fsanitize=float-cast-overflow`
(UBSan ships with llvm-mingw) and grep the logs for `runtime error`. The
game modes report about 40 sites, mostly `(s16)` of angles slightly beyond
±32768 and a few u8 colours. On x86 clang converts through 32 bits for
s16/u16, so they behave like the GC, but a future compiler or ARM build may
not. The fix is the same `(sNN)(s32)` pattern, which keeps the GC build
byte-identical.

Values beyond the s32 range are a different case:
- `stcoli.c` 2.1e9 and `mathutil.h` ±inf → int give 0x80000000 on x86
  (both builds agree).
- They saturate on ARM and on the GC.
- A future ARM port needs an explicit fctiw-style helper there.

## Still target-dependent (not used in game state)

- **Float→integer conversion of values outside the s32 range** (above).
- **printf-style float formatting:** only used for debug/config text.
- **The GPU:** shaders run on the same GPU for both builds on one machine.
