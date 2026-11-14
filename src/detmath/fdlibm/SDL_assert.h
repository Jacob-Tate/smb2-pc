/* Stand-in for SDL_assert.h (k_rem_pio2.c): the asserts are internal
 * invariants of fdlibm and compile to nothing, as in SDL release builds. */
#define SDL_assert(x) ((void)0)
