/* Common declarations for the GameCube SDK shim layer.
 * Shims are compiled against the decomp's headers (types.h etc.), NOT the
 * pc_port headers, to guarantee struct layouts match what the game TUs saw. */
#ifndef SHIM_H
#define SHIM_H

#include "types.h"  /* decomp include dir */

/* ---- GC memory map emulation (gc_mem.c) ---- */
/* Main RAM mapped at the real GC address if possible. */
#define GC_MEM_BASE  0x80000000u
#define GC_MEM_SIZE  0x01800000u  /* 24 MiB retail */

int  gc_mem_init(void);          /* returns 1 if mapped at GC_MEM_BASE */
u32  gc_arena_lo_initial(void);
u32  gc_arena_hi_initial(void);
u8  *gc_mem_ptr(u32 gc_addr);    /* host pointer for a GC main-RAM address */

/* ---- stub logging ---- */
void shim_log_once(const char *sym);
void shim_trace(const char *fmt, ...);

/* ---- pc_port services (declared here to avoid header clashes) ---- */
struct ISOArchive;
extern struct ISOArchive *platform_get_iso_archive(void);
extern void platform_poll_events(void);
extern int  platform_should_close(void);   /* bool in pc_port; int-compatible */
extern void platform_swap_buffers(void);
extern unsigned long long platform_get_ticks_ms(void);
extern void platform_sleep_ms(unsigned int ms);
extern void input_poll_gc_pads(void);  /* fills pad_status_groups from SDL (local only: modal screens) */
extern void input_poll_gc_pads_game(void);  /* same, for the game's per-poll call: goes through inputsync (E3) */

/* iso.c helpers added for the DVD shim */
extern int  iso_entry_count(struct ISOArchive *arc);
extern int  iso_path_to_index(struct ISOArchive *arc, const char *path);
extern int  iso_path_to_index_from(struct ISOArchive *arc, int dir_index, const char *path);
extern int  iso_entry_is_dir(struct ISOArchive *arc, int index);
extern unsigned int iso_entry_size(struct ISOArchive *arc, int index);
extern const char  *iso_entry_name(struct ISOArchive *arc, int index);
extern int  iso_entry_next(struct ISOArchive *arc, int index);
extern int  iso_file_read_at(struct ISOArchive *arc, int index,
                             unsigned int offset, unsigned int size, void *dest);

#endif
