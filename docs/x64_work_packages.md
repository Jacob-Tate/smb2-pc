# x64 conversion: checklist and change inventory

Read [x64.md](x64.md) section 0 first: the build, the DPTR / `uptr` / `sptr` /
`PC_PTR_SCALE` / `PC_A32` toolbox, and how to debug. Section 1 of x64.md is the
inventory the areas below were cut from. The per-area lists are the original
plan for the conversion; all of them are done (x64.md "Status" has the
results) and they are kept as a map of where the 32-bit assumptions were.

## Checklist for changes that touch pointer-sized data

- **GC build stays byte-identical.** Run
  ```
  cd decomp && python3 configure.py && ninja && dtk shasum -c config/GM2E8P/build.sha1
  ```
  (`dtk` is downloaded to `build/tools/dtk`) and expect 21 OK. PC-only code goes inside
  `#ifndef __MWERKS__`, or uses the types.h macros, which are identical on GC.
- **The 32-bit build must keep working.** Check it with `modes.sh` on GL and D3D12.
- **Choosing a fix.** Pick the smallest change that keeps the GC layout:
  1. A struct whose layout is fixed by disc data, a static table, a literal
     size/offset or an overlay, and whose pointees all live in GC RAM or the exe:
     add `DPTR` to its pointer fields. Change any `T **` local that walks a DPTR
     array to `T * DPTR *`.
  2. An integer field or global holding a pointer: use `uptr`. Use `sptr` where
     it was signed — GC RAM is at 0x80000000, so an s32 round trip sign-extends.
  3. A raw byte buffer holding pointer-bearing structs: `PC_PTR_SCALE`.
  4. Address words in a u32 table: `PC_A32()` plus `pc_port/tools/gen_addr32_fixups.py`.
  5. Host memory (CRT malloc, renderer objects) must **never** go into DPTR or u32.
- **Test** with the harness (`tools/x64test/modes.sh`; see its header for
  `SMB2_X64_WS`, `SMB2_ISO`, `tools/ppmtool` and `tools/memcard_pristine`).
  Runs only write below `$SMB2_X64_WS` and always pass `--save-dir`.
  ```
  T=pc_port/tools/x64test/modes.sh
  $T gl32 $WS/build32/bin <modes> -- --renderer gl
  $T gl64 $WS/build64/bin <modes> -- --renderer gl
  $T dx64 $WS/build64/bin <modes> -- --renderer d3d12
  $T --compare gl32 gl64 dx64 <modes>     # sheets in $WS/runs/sheets/gl32_vs_gl64_dx64/<mode>/
  ```
  Frames past the attract demo aren't deterministic, so judge the sheets
  visually. Crashes in a dump symbolize with llvm-symbolizer (x64.md section 0).
- **Done** means:
  - the affected modes run on x64 GL and D3D12 for the whole script, with no
    crash and no watchdog;
  - sheets match 32-bit apart from known nondeterminism;
  - GC is 21 OK and 32-bit is unchanged;
  - no new pointer-cast warnings (`-DSMB2_PTR_WARNINGS=ON`);
  - a short note is added to x64.md section 0 "Status".

## Status when the conversion of gameplay code started

| Mode (modes.sh) | x64 GL / D3D12 |
|---|---|
| intro | OK; frames 100–700 identical to 32-bit GL |
| menus | renders the attract demo identically to frame ~900, then hangs (watchdog) in adv_demo.c `fn_1_11D60` |
| challenge, race, target, bowling, boat (launcher scripts) | all crash at ~frame 1300–1400 with a corrupted heap free list. `SMB2_HEAP_CHECK=1` pins it to **stage heap: something writes 0x10–0x48 bytes past the end of the stagedef buffer** (`stage_load.c` `uncompData`, "stcoli.c" 0x191, 0x1840 bytes for the first Challenge stage). That is x64-only: the same check on the 32-bit build is clean. It blocked every gameplay mode. |
| dogfight | not run yet (same path as above) |
| baseball | OSPanic by design until the embedded stagedefs were converted |

## Change inventory by area

### Story-mode cutscenes: scene, dialog, author

**Files:**
- `decomp/src/main_loop/scnplay.c`
- `scnplay_fx.c`, `scnplay_effect.c`, `scnplay_draw.c`
- `dialog.c`
- `author.c`, `include/author_preload.inc`
- `endian_fix.c` (`pc_swap_scene_data`, `pc_swap_dialog_tbl`)

**Work:**
- **Scene, SceneObj, SceneAnim** (scnplay.c:24-44, 9 pointer fields, relocated at
  scnplay.c:1158-1173): add DPTR. Fix the `T **` locals.
- **DialogTbl/DialogData** (dialog.c:17-43; relocated at dialog.c:1549-1556 via
  `(u32)` arithmetic): add DPTR.
- **Dialog / TextEntry** (dialog.c:47-99) are runtime structs:
  - `inst->dialog` is allocated with the literal `0xA0` (scnplay.c:901). Either
    DPTR the fields or allocate `sizeof`.
  - `sprite->userVar = (int)box` (dialog.c:716) — check what `Sprite.userVar`
    holds.
- **scnplay_fx.c:2508/667 and scnplay_effect.c:1904** (an index stored in a
  pointer): verify only.

**Test:**
- A `story` mode in modes.sh: Story Mode → the opening
  cutscene, `dlg/` + `scndat_*.bin`.
- Also run `menus`.

---

### Baseball

**Files:**
- `decomp/include/baseball_stage_data.inc`
- `decomp/src/baseball/*.c`
- `endian_fix.c` (`pc_fix_embedded_stagedefs`, `sd_*`)
- `tools/gen_addr32_fixups.py` (extend it)

**Work:**
- **Embedded stagedefs.** StageDef now keeps its 32-bit layout (DPTR), so the
  embedded u32 images are valid again. Replace the x64 `PC_BB_REF` tags:
  - Make `P0..P9(i)` expand to `PC_A32(&lbl_18_data_XXXX[i])`.
  - Teach the generator to expand `Pn(i)` into fixups, then apply them before
    `pc_fix_embedded_stagedefs`.
  - Remove the OSPanic guard in baseball_20.c.
  - `pc_fix_embedded_stagedefs` does `(u32)arrays[i]` arithmetic. That is fine
    below 4 GiB but should become `uptr`.
- **Literal sizes:** memsets `0x84` (baseball_7.c:216, _8.c:48/63), `0x54`
  (_21.c:635/898), `0x70` (_21.c:706), `0x20` (_20.c:106); `itemGroupCount*0xA0`
  (_20.c:62); the offset `(u8*)lbl_18_bss_19270 + r0*8 + 0x380` (_26.c:2182);
  `stage + 0x604` (_21.c:243).
- **Pointer stores:** `fn_18_27A0(0x1E, (s32)fn)` (_5.c:14, _6.c:1478) — fine
  below 2 GiB, but make it `sptr`. Also `(u32)play->unk64 != 0`-style tests.

**Test:** the `baseball` mode, GL and D3D12.

---

### Race (race2) and Target (pilot2)

**Files:**
- `decomp/src/race2/*.c`
- `decomp/src/pilot2/*.c`
- `decomp/include/race2.h`, `pilot2.h`

**Work:**
- **Effect memsets `0xB0`:** race2_0.c ×10 and race2_3.c:56. Effect is 176 bytes
  on 32-bit and 184 on x64.
- **Item memsets `0xB4`:** race2_0.c:2051/2118.
- Effect and Item live in effect.h/item.h (core runtime): decide DPTR vs
  `sizeof` there first.
- **`Ball.unk150` (u32 holding RacePlayer*, 78 reads):** OK below 4 GiB. Leave it
  unless it breaks.
- **`fn_8006991C((s32)model)`** (race2_0.c:1897-1898): check the callee's types.

**Test:** the `race` and `target` modes.

---

### Boat, Dogfight, Bowling

**Files:**
- `decomp/src/boat/*.c`
- `decomp/src/dogfight/*.c`
- `decomp/src/rel_mini_bowling/*.c`
- `decomp/include/boat.h`

**Work:**
- **Literal sizes:**
  - Effect `0xB0` memsets: boat_2.c:681/685/900/1194/1416
  - Item `0xB4`: boat_2.c:722
  - Ball `0x1B0`: boat_2.c:1117
  - BoatPlayer `0x4F0`: boat_1.c:1138, boat_2.c:1154
  - alloc `*0xB40`: boat_0.c:270
- **`(Vec *)((u8 *)ball->ape + 0x258)`** (boat_2.c:1935): struct Ape grows on x64.
  Use the field.
- **Function pointers stored in s32:** boat_0.c:498-502, read at boat_2.c:8-9.
  Fine below 2 GiB, but use `sptr`/`uptr`.
- **Absolute GC addresses:** boat.h:64, dogfight_7.c:218 (`*(u32*)0x800000F8`).
  These work because GC RAM sits at 0x80000000. Verify only.
- **bowling_5.c:108** `GXGetTexObjData` is implicitly declared, so its returned
  pointer is truncated. Add the prototype.
- **bowling_8.c:131-135:** `(u8*)texObjs + 0x80` assumes `sizeof(GXTexObj) == 0x20`.
  The PC GXTexObj is exactly 0x20 on x64, so this is OK. Keep it that way.

**Test:** the `boat`, `dogfight` and `bowling` modes.

---

### Menu and UI data tables

**Files:**
- `decomp/src/main_loop/how_to.c`
- `name_entry2.c`
- `ape_face.c`, `include/ape_face_data.inc`
- `decomp/src/option/option_menu.c`
- `include/window_menu.inc`
- `sel_ngc/*.c` (data tables only)

**Work:**
- **`void *[]` tables holding packed integer words.** On x64 the stride doubles:
  - how_to.c: 864 entries. `pc_fix_how_to_tables` (how_to.c:7227) is 32-bit-only:
    `(u32)(uintptr_t)raw[0]`, a 3-word stride, and `memcpy` of a 12-byte struct.
  - name_entry2.c:712/782: 394 entries.
  - ape_face_data.inc:38: 225 entries, read as `s32*`; ape_face.c:490 round-trips
    through `int`.
  - option_menu.c: 20.
  - window_menu.inc: 119, benign masks — verify.
- **Fix approach:** give these tables a PC element type that keeps 4-byte words.
  Use u32 words with `PC_A32` fixups for the real pointers, or typed structs
  with DPTR. Then make the readers index 4-byte words.

**Test:**
- `howto` (Challenge → pause → How to play; the script is unverified, so check
  the sheets show the How-to screens)
- `menus`, `challenge`
- the name-entry screen after a ranking.

---

### SDK/shim layer, ARAM, memcard and replay saves

**Files:**
- `pc_port/src/game/shim/ar_shim.c`, `os_shim.c`, `card_shim.c`, `musyx_shim.c`,
  `dvd_shim.c`, `stubs_gen.c`, `stubs_manual.c`, `libc_shim.c`
- `decomp/src/main_loop/memcard*.c`
- `smgr.c`, `disc_load.c`
- `recplay.c`, `cmp_recplay.c`
- `endian_fix.c` (`pc_swap_replay`)

**Work:**
- **ARC handle overflow.** `ShimARCHandle` (four pointers, 0x1C → 0x38 bytes) and
  `ShimARCFileInfo` are written into `u8[0x1C]` / `u8[0xC]` blobs at
  decomp/src/main_loop/ape.c:30-38. Fix by DPTR-ing the shim structs (their
  pointees are GC RAM/ARAM buffers).
- **Audit every PC mirror of a decomp struct** (`struct PcXxx { u8 raw[N]; }`,
  hand-copied layouts) for x64 sizes. The PcFile overflow was one of these.
- **stubs_gen.c K&R stubs** (`int f()`): 5 of them are declared to return
  pointers elsewhere (e.g. `find_stage_or_bg_model`, `fn_80065C94`). Give them
  real prototypes.
- **Implicit declarations.** Grep the clang build log for implicit declarations
  of functions that return pointers.
- **u32 SDK ABI params:** smgr.c:111 → MusyX `user`; disc_load.c:262
  `ARQPostRequest((u32)dest)`. Fine below 4 GiB; verify.
- **MemcardJob:** DPTR (it was allocated as `0xC0`).
- **Memcard and replays.**
  - `struct Replay` is now DPTR, so the layout is unchanged.
  - Verify a memcard save/load round trip on x64. Also check a card file written
    by 32-bit loads on x64 and vice versa: copy `memcard/slotA` between runs.
  - Replay save/load from the memcard.
  - `pc_swap_replay` uses fixed 32-bit offsets, which are correct again with DPTR.
    Verify.
- **platform_sdl2.c:516** writes `sizeof(size_t)` into `.meta`. Check
  compatibility.

**Test:**
- `memcard` (the script is unverified; extend it to save a replay and reload it)
- `challenge` with a replay
- cross-arch card files

---

### Core main_loop runtime and the attract/menu path

**Files:**
- `decomp/src/main_loop/adv_demo.c`
- `stcoli.c`, `background.c`, `effect.c`, `item.c`, `ball*.c`, `camera.c`, `wormhole.c`
- `window.c`, `textdraw.c`, `sprite.c`, `alloc.c`, `main_loop.c`, `thread.c`, `ord_tbl.c`
- `ape_anim.c`, `ape_motion.c`, `skeleton.c`, `nl2ngc.c`, `stage*.c`, `bitmap.c`
- `decomp/src/main_game/*.c`
- headers: `ball.h`, `effect.h`, `item.h`, `sprite.h`, `background.h`, `stage.h`,
  `avdisp.h`, `nl2ngc.h`, `ape_internal.h`, `skeleton.h`, `camera.h`

**Work:**
0. **Stage-heap corruption (first; blocks every gameplay mode).**
   - **Symptom:** run `SMB2_HEAP_CHECK=1 modes.sh hc64 $WS/build64/bin challenge`. It
     reports the free cell right after the stagedef buffer (`uncompData`,
     stage_load.c "stcoli.c" 0x191) as overwritten 0x10–0x48 bytes past its end.
     The guard right after the buffer is intact, so this is a non-contiguous
     write.
   - **Not on 32-bit:** clean there.
   - **Suspects:**
     - code that writes runtime data into the stagedef image using a struct that
       is bigger on x64: `StageState`, `ItemGroup`, `Seesaw`, `DynamicStagePart`
       (see the size table)
     - the endian_fix.c walker
     - `load_stagedef` / `stage_load.c` relocations
1. **Attract-demo hang.**
   - **Symptom:** adv_demo.c:2031 `fn_1_11D60` (demo shadow blobs) spins, because
     the `continue` at `t < 0` skips `bone++`.
   - **Cause:** a raycast result or bone matrix that is wrong only on x64. Find the
     real bug; don't just patch the loop.
   - **Suspects:**
     - `g_get_ape_anim_smth` bone matrices
     - `raycast_stage_down` (stcoli.c) over itemgroup transforms
     - `ItemGroup` (stage.h:471; runtime, not DPTR'd)
     - the `StageColi*` / grid lookups
2. **Literal sizes and offsets in core code:**
   - `memset(&world_theme, 0, 0xD0)` (background.c:904)
   - `BG_WORK` = `*(void**)((u8*)&world_theme + 0x9C)` and `bgWork + 0xB14/0xB0C`
     (effect.c:49, 2446-2447, 2783, 2790)
   - Effect `0xB0`, Item `0xB4`, Ball `0x1B0`: decide here whether Effect, Item
     and Ball get DPTR fields or the minigame memsets become `sizeof`. (Decided: DPTR, they keep their 32-bit sizes.)
3. **Pointers stored in integers:**
   - `wormholeId = (int)&balls[i]` (wormhole.c:52/65)
   - `g_some_font_ptr_array` (textdraw.c)
   - `g_some_other_heap_lo` (alloc.c)
   - `ArenaBuffer` (main_loop.c:346-357)
   - window.c `limits`
   - the remaining `-Wpointer-to-int-cast` sites in these files: about 60 of the
     268 total
4. **Cross-TU declarations.** Not done yet:
   - `lbl_1_bss_C8AE4`/`C8AE8` (mot_ape.c u32 vs pointer)
   - `OSCreateHeap`/`OSGetArenaLo` declared u32 in main_loop.c and `void*` in alloc.c
5. **Audit raw `u8 name[N]` buffers** that hold pointer-bearing struct arrays (like
   lbl_1_bss_34 and lbl_1_bss_A87E0, which are already fixed).

**Test:** `menus` must reach Mode Select; `challenge`.

## Tools (in `pc_port/tools/x64test/`)

| Tool | What it does |
|---|---|
| `modes.sh` | Runs modes on any build and renderer, then diffs and writes sheets. Passes `SMB2_RENDERER` and `SMB2_HEAP_CHECK` through. |
| `SMB2_HEAP_CHECK=1` (env, implemented in os_shim.c) | Adds a guard and allocation site to every heap block and checks all heaps on every alloc/free. Reports the first overrun block or corrupt free cell with the `alloc_from_heap_or_panic` file:line, then exits 3. This is the fastest way to find x64 size bugs. The 32-bit build runs clean with it. |
| `structsizes.py <decomp> <llvm-mingw>` | Lists every decomp header struct whose size differs between i686 GCC and x64 clang (below). If one of these lives in disc data, a static table, a literal-size alloc/memset or an overlay, it is a bug: fix it with DPTR or sizeof. |
| `crossdecl.py <decomp> <pc_port/src>` | Lists globals defined as an int or byte array but declared as a pointer or struct elsewhere. Entries already widened to `uptr`/`sptr` show up too and are fine. **Open:** `lbl_1_bss_12AC` (u8[0x2C] vs struct ExOptionFile); the mot_ape.c u32 globals vs test_mode pointers (test_mode isn't built). |
| `-DSMB2_PTR_WARNINGS=ON` | 268 pointer↔int cast warnings in the clang build (x64.md section 0). |

### Structs whose size differs on x64 (structsizes.py, 2026-11-11)

**Fix only the ones** that are allocated, cleared or indexed with literal sizes,
overlaid on file or static data, or stored in fixed-size buffers. Runtime structs
allocated with `sizeof` are fine as they are.

```
name                             32-bit 64-bit  header
AnimJoint                        0x2A0   0x340   mot_joint.h
Ape                              0x2F0   0x4A0   ball.h
ApeAnim                          0x54    0x58    ball.h
BBBallFuncs                      0xC     0x18    baseball.h
BBBatFuncs                       0x10    0x20    baseball.h
BBLayer                          0x20    0x28    baseball.h
BBPlay                           0x84    0x98    baseball.h
BBPlayState                      0xC     0x18    baseball.h
BBRules                          0x28    0x38    baseball.h
BBScreen                         0x538   0x738   baseball.h
BBTex                            0x5C    0xB8    baseball.h
BBTimer                          0xC     0x10    baseball.h
BBTrack                          0x1C    0x28    baseball.h
BackgroundInfo                   0xD0    0x100   background.h
Ball                             0x1B0   0x1B8   ball.h
BallEnvArg                       0x38    0x48    avdisp.h
BgLightInfo                      0x48    0x50    light.h
BgModelSearch                    0x8     0x10    background.h
BgShadowGroup                    0x18    0x28    background.h
BilLightGroupName                0x8     0x10    light.h
BitmapGroup                      0x18    0x30    bitmap.h
BoatCourse                       0x28    0x40    boat.h
BoatParticleParam                0x34    0x48    boat.h
BoatPlayer                       0x4F0   0x4F8   boat.h
BoatVortexMesh                   0x14    0x28    boat.h
Bone                             0x174   0x1C8   skeleton.h
CommendWork                      0x28    0x50    baseball.h
DynamicStagePart                 0x18    0x30    stage.h
Effect                           0xB0    0xB8    effect.h
EffectFuncs                      0x10    0x20    effect.h
EndingBit                        0x54    0x60    ending.h
EndingChara                      0x80    0x88    ending.h
EndingInfo                       0xC     0x10    ending.h
EndingMsgLines                   0x1C    0x38    ending.h
EndingWork                       0xC2D0  0xDB18  ending.h
Event                            0x18    0x30    event.h
FontParams                       0x38    0x48    bitmap.h
Item                             0xB4    0xD0    item.h
ItemFuncs                        0x1C    0x38    item.h
ItemGroup                        0xA0    0xA8    stage.h
MenuEntry                        0x34    0x68    sel_ngc.h
MenuScreen                       0x10    0x20    sel_ngc.h
MotionChannel                    0x10    0x20    mot_joint.h
OrdTblNode                       0x8     0x10    nl2ngc.h
PoolInfo                         0x48C   0x4D0   pool.h
PoolInfo_sub                     0x10    0x18    pool.h
Preview                          0x78    0x90    sel_ngc.h
PreviewFileInfo                  0x3C    0x48    sel_ngc.h
Race2CourseInfo                  0x70    0x98    race2.h
Race2DrawNode                    0x10    0x18    race2.h
ScenInfo                         0x38    0x40    scen.h
Seesaw                           0x20    0x40    stage.h
SeesawFuncs                      0x20    0x38    stage.h
SelNgcMenuWork                   0x40    0x48    sel_ngc.h
ShadowReq                        0x38    0x40    shadow.h
Skeleton                         0x70    0x80    skeleton.h
SoundEffectDesc                  0x10    0x18    sound.h
SoundGroupDesc                   0x14    0x20    sound.h
Sprite                           0xD0    0xF0    sprite.h
StageBgLightInfo                 0x4C    0x58    light.h
StageState                       0x5C    0x60    stage.h
Stobj                            0xCC    0xE0    stobj.h
StoryModeWorldInfo               0x8     0x10    course.h
ViewMatrices                     0x110   0x120   camera.h
```
