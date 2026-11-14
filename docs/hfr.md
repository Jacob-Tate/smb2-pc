# High frame rate rendering (HFR)

Shows more than 60 frames per second (120 / 144 / 165 / 240 / the display's
refresh rate / uncapped) on high-refresh monitors without changing the
game's speed or logic.  The decompiled game is a fixed 60 Hz tick
(GameCube VI rate): every tick advances physics, cameras, animation, timers
and effects by exactly one step.  HFR keeps the 60 Hz tick untouched and
draws extra frames *between* ticks by replaying what the last two ticks
drew, with transforms, vertices, lights and colours blended.

Status: complete feature (pc_port + decomp).  The frame
rate setting at **60 (the default) is exactly today's code path** (section 9).

## 1. Using it

* **Options > Game Screen > Frame rate** and the **F1 overlay** row
  "Frame rate": `60` (default, no interpolation) -> `Display (N)` (the
  monitor's refresh rate) -> `120` -> `144` -> `165` -> `240` ->
  `Uncapped`.  Applies immediately (next tick), no restart.
* `settings.dat`: `fps=60|display|120|144|165|240|uncapped|<n>`.
* Command line: `--fps 60|120|144|240|display|uncapped|<n>` (overrides
  settings.dat for the run).
* V-Sync on: the target is capped at the display refresh (asking for more
  presents than the display shows would only block).  `Display` with V-Sync
  on is the recommended setting for a 120/144/240 Hz monitor; with V-Sync
  off (tearing allowed) or on a VRR (G-Sync / FreeSync) display any target
  works and `Uncapped` draws as many in-between frames as fit.
* A 60 Hz display with `Display` gives 60 = off.

## 2. How a frame flows

```
decomp main_loop.c loop (one iteration = one 60 Hz tick):
  gx_start_new_frame (gx_core.c)       present of the previous tick frame;
                                       hfr_select (proxy on/off), hfr_tick_begin
  inputs, mode_tick(), pools, events   (logic - unchanged)
  polydisp_main() + draw_ui()          (3D, EFB-copy passes, 2D) -> GX calls
                                       -> gx_core -> GxGpu proxy (recorded)
  video_finish_frame (video_shim.c)    hfr_tick_end (matching), audio, input,
                                       F1 overlay, then the pacer:
                                       in-between frames (replays at alpha j/k),
                                       the tick frame (replay at alpha 1),
                                       wait for the 60 Hz deadline
```

### Recording (src/render/gx_hfr.c)

A `GxGpu` proxy sits in front of the real backend (gx_core.c selects it at
each tick start when the frame-rate setting is not 60).  While a tick's
stream is open it records every replayable call: viewport, scissor, blend,
depth, cull, clears, texture binds, texture sampler state, asynchronous EFB
copies (`copy_tex`) and draws (vertices, 2D quad UV rects, a copy of
`GxDrawConsts`, `GxTevConsts` once per serial).  Two streams are kept: the
previous tick (P) and the newest (C).

**Deferred recording.**  Recorded calls are not sent to the GPU when they
arrive.  The tick frame itself is drawn by the replay at alpha = 1, so the
main pass is not submitted twice.  Recorded calls are sent early ("flush")
only when something needs the frame so far: an EFB copy or readback (the
offscreen pass it copies), a texture re-upload (draws before it must see
the old contents), present, frame capture (F1, frame dumps).  If memory
runs out while recording, everything is sent at once from then on and the
tick is not replayed.  `SMB2_HFR_DEFER=0` disables deferral (A/B checks).

Texture destruction is deferred by two ticks (both streams may reference
the texture).  A texture re-uploaded during tick C that stream P uses
("hazard") keeps P from being drawn (in-between frames then use C only).

### Matching (hfr_tick_end)

Each draw gets two keys.  The full key: identity tag (section 3),
primitive, vertex count, 2D flag, bound textures 0-2, draw modes / texgen
masks, TEV / channel / indirect configuration (without the animated values:
lights, konst and register colours).  The loose key: tag, primitive,
vertex count, 2D flag.

1. Per full key, the draws of P and C pair **in order** when that is
   consistent: same count, and every pair is the same mesh (model draws:
   equal model-space first and last vertex) or (CPU-transformed draws)
   close in view space.
2. Otherwise **nearest first**: candidate pairs within the jump distance,
   sorted by a distance made of the view-space move, the model-space
   first/last vertex distance x4 (the same mesh is ~0, a neighbouring strip
   or tile is not) and the orientation difference.  Model draws only pair
   with the same mesh (first/last vertex within 2%).  Groups too big for
   all pairs (hundreds of particles / tiles) search a window around the
   proportional position.
3. Leftovers pair through the loose key (texture flipbooks), strictly.
4. Every pair is analysed: a jump (view-space move > 4 + 25% of the
   distance, rotation > 150 deg, 2D move > 160 EFB px, 2D UVs differ, 2D
   vertex count differs) unpairs it.  Equal constant blocks = nothing to
   blend; a rigid transform turning > 3 deg = slerp; vertices that differ
   = vertex blend; TEV lights / colours that differ = TEV blend.
5. A camera cut (game hook, a camera view jump, or > 50% of the paired 3D
   draws jumping) unpairs everything.

Cost: ~0.3-1 ms per tick for 7500 draws (sorts, one memcmp of the vertices
per pair).

### Replay (hfr_render(alpha))

* alpha < 0.5 draws the **previous** stream's ops, alpha >= 0.5 the
  newest, each paired draw blended between its two ticks at alpha.
  Continuous motion is interpolated; **discrete changes happen at the
  midpoint**: an object appearing or disappearing, a timer digit changing,
  a sprite frame, a camera cut, text being typed.  3D and 2D change at the
  same moment - the HUD no longer runs one tick ahead of the scene (the
  prototype drew 2D from the newest tick only).
* Matrices: projection lerp (same projection type), modelview / normal
  matrix element-wise below 3 deg of rotation per tick (exact for
  translation, consistent across the pieces of an object), **rotation slerp
  + per-axis scale lerp + translation lerp** above (spinning objects keep
  their size: element-wise blending shrinks a 60 deg/tick spin by 13% at the
  midpoint).  Mirrored transforms keep their reflection.
* Texture matrices element-wise; scroll translations that wrapped by whole
  repeats blend the short way, other jumps keep the base value.
* CPU-transformed vertices (skinned monkeys, effects, 2D sprites): position,
  normal and colour lerp; UVs as drawn.
* TEV: light positions / directions / colours, konst and register colours,
  material / ambient colours, post-texgen matrices lerp; draw constants: TEV
  colour registers, fog range and colour lerp.  Colour fades are smooth.
* EFB copies (`copy_tex`, GPU-side since f7ffdc1/590b599): **re-executed in
  every replay** from the interpolated offscreen pass, texture only
  (`GXCOPY_NO_READBACK`, no readback, no CPU work; GC memory keeps the tick's
  copy, so game logic never sees in-between data).  A copy whose own
  texture was sampled earlier in the tick before any copy into it (a
  feedback effect) is not re-executed: it keeps the tick's result.

## 3. Game-side hooks (decomp `include/pc_hfr.h`, empty on GC)

| Hook | Where | Why |
|---|---|---|
| `HFR_TAG(BALL, i)` | ball_draw.c `g_draw_ball_and_ape` | ball + its monkey per player |
| `HFR_TAG(ITEM, item->id)` | item.c `draw_items` | bananas etc. (pool slot) |
| `HFR_TAG(STOBJ, index)` | stobj.c `g_draw_stobjs` | goal bags, bumpers, ... |
| `HFR_TAG(EFFECT, uid)` | effect.c `g_draw_effects` | particles: spawn / die without order shifts |
| `HFR_TAG(STAGE_IG, i)` | stage_draw.c `g_draw_stage_models` | animated stage parts per item group |
| `HFR_NODE_ALLOC/BEGIN/END` | ord_tbl.c | depth-sorted draws (drawn later, in another order) keep the tag they were queued under |
| `HFR_VIEW(cam, view)` | camera.c `camera_apply_viewport[_2]`, default projection | camera view per tick: cut when it turns > 43 deg or moves more than 3x its speed + 3 units in one tick |
| `HFR_CUT(CAMERA_RESET)` | camera.c `camera_clear` | stage start, respawn, replay / mode camera resets.  Soft: cuts only if a camera view then moved more than 1.5x its recent speed + 0.5 units or turned > 10 deg (baseball's intro pan resets its camera every tick while panning smoothly) |
| main mode change | video_shim.c (PC side) | menus <-> game <-> minigames |

A tag change ends the pending vertex batch (gx_core `gx_hfr_flush`) so a
batch never spans two objects; with HFR off the hooks return at once.
Minigame-private objects (boat, race, baseball ...) need no tags: the
generic matching (nearest position + mesh identity) pairs them.

## 4. Frame pacing (video_shim.c `hfr_subframes`)

* The 60 Hz deadline is unchanged (tick frame presented at `next_ns`).
* In-between phases on the target's grid, `tick0 + j/fps` (exact alpha j/k
  for multiples of 60); each frame is replayed, then presented at its phase
  (waitable timer + 1 ms spin).  A phase the slow tick already passed is
  drawn late at the alpha of its real present time if it still fits.
* A frame is only started if it and the tick frame still fit before
  `next_ns` (running averages of the replay's CPU time and, with V-Sync off,
  the present call's time; with V-Sync on present blocking is pacing).
* **No game slowdown**: `kcap` (most in-between frames per tick) drops to
  what fit whenever a tick still ends > 1.5 ms late (GPU backpressure the
  estimates missed) and recovers by one per 30 calm ticks.  A GPU that
  cannot keep up gets fewer in-between frames, the game keeps 60 ticks/s.
* The tick frame (alpha 1) is always the last replay of the tick.
* GPU timeline (D3D12, timestamp queries): a frame's GPU work starts at its
  present (or when the GPU is free); the tick frame of the previous tick
  occupies the GPU from tick0.  An in-between frame is only started if its
  GPU work ends, and the tick frame's CPU replay is done, before `next_ns`.
  Its alpha is taken at its GPU start (the tick frame's display is delayed
  by the same GPU time, so motion stays even).  GL has no reliable GPU
  timing (GL_TIME_ELAPSED includes submission gaps): the GL pacer relies on
  the present time and the cap.
* Estimates (replay CPU time, present call time) are updated by every
  replay and every present, so they cannot freeze after a hitch.
* **Degrade**: a tick that ends > 1.5 ms late without any in-between frame
  (recording + matching + replay cost more than this machine has, e.g. the
  10k-draw goal party-ball scene on a loaded CPU) turns recording off for
  2 s: the frame is drawn directly exactly like the 60 fps path, so HFR is
  never slower than 60 fps.
* Uncapped: back to back, same fit test.
* F1 overlay ticks skip in-between frames (the overlay presents its own).

## 5. Input latency

Input is sampled once per tick, as before, and the tick frame reaches the
screen at the same time as at 60 fps (presented at the next tick start).
The in-between frames do not delay it: they are shown *before* it, during
the interval in which 60 fps keeps showing the previous tick.  So:

* added latency for the newest tick: **0** (same present time as 60 fps);
* the first visible response to an input comes earlier: the first
  in-between frame after the tick already blends 1/k of the new tick in
  (at alpha >= 0.5 it is drawn from the new tick's stream), i.e. 1/k tick
  after logic instead of a full tick (240 Hz: ~4 ms after the tick);
* cost: the replays run between the tick's logic and its present, so on a
  machine that cannot fit them the pacer drops in-between frames rather
  than delaying the tick frame (section 4).

Compared with an ideal renderer that would draw the newest state at every
present, interpolation shows the scene up to one tick (16.7 ms) behind;
extrapolation (alpha > 1) would remove that but mispredicts bounces and
turns, so it is not used.

## 6. Per-effect decisions (EFB copies, 2D)

| Effect | Path | HFR result |
|---|---|---|
| Reflections / mirrors, water (boat, bg_wat, ripples), wormhole, stage-select previews | async GPU copy | re-executed per frame: at the render rate |
| Feedback effects (pass samples its own last copy) | async GPU copy | tick's copy kept (60 Hz), consistent |
| Z / non-convertible formats, CPU fallback | sync copy | tick's copy (60 Hz) |
| Pause screenshot | copy | static anyway |
| HUD sliding elements, banners, "GO", fades | 2D quads | positions / colours interpolated (same UVs) |
| Timers, digits, text being typed, sprite frames | 2D quads, other UVs | discrete, switch at the midpoint with the 3D |

## 7. Testing tools

* `--dump-ranges a-b,c-d`: dumps every tick in the ranges plus its
  in-between frames (`frame_T.ppm`, `frame_T_j.ppm` = in-between j shown
  before tick frame T) and switches the pacer to fixed phases j/k.
  `--dump-from N` = every tick from N.
* `tools/hfr/hfrcap.sh <label> <bindir> <mode> <ranges> [-- args]`: a
  `tools/x64test/modes.sh` mode at 240 fps (1280x720) with dump ranges, then
  `tools/hfr/hfrcheck` per range (per-step frame differences; 32x32 blocks
  whose change lands in one step = "pops", PNG strips of those ticks for
  review) and an overview strip per range.  Legitimate discrete changes
  (timer digits, a camera cut) show up as pops too; review the strips.
* `tools/hfr/hfrperf.py log.txt [min_draws]`: the `[hfr]` 5-second windows
  as a table (fps, interval mean / p50 / p95 / p99 / max, late presents,
  replay ms, replays per tick, kcap, 3D draws per tick).
* `SMB2_HFR_DEBUG=<tick>`: list both streams' draws and their pairs for
  that tick (`SMB2_HFR_DEBUG_FULL=1`: matrices and vertices of small draws).
* modes.sh `hfr_roll` (Challenge 1-1 at full tilt: fast ball spin, goal
  party ball, replay) and `hfr_fall` (fall-out, respawn cut).

## 8. Evidence and measurements

All runs: x64 build, RTX 3080, Samsung Odyssey G8 3840x2160 @ 240 Hz, runs
started from WSL with other builds running on the same host.  Paths are
relative to the test workspace root (`$SMB2_X64_WS`, see tools/x64test/modes.sh).

### In-between frames, all modes (`runs/s3` final build; `runs/s1` earlier)

`tools/hfr/hfrcap.sh` at 240 fps (3 in-between frames per tick, fixed
phases), 1280x720, GL and D3D12 split over the modes; per mode
`check_<range>.txt` (step differences per tick), `strip_<tick>.png` (tick
T-1, in-betweens 1-3, tick T) and `pops/pop_<tick>.png` (enlarged strips of
ticks with blocks whose change lands in one step).  Every flagged tick was
reviewed: all are legitimate discrete changes at the midpoint (timer digits,
score / time-bonus counters, text being typed, a cut during a fade); no
popping, ghosting or wrong blends remain.

| mode | ranges | notes |
|---|---|---|
| intro (attract) | 300, 500, 650 | 3D attract + sparkle particles |
| menus | 1455-1475, 1555-1575 (slides), 1900 | menu slide-ins interpolated (2D), attract cut at the fade |
| challenge, `hfr_roll` | 2400-3110 | full-tilt roll (ball spin slerp), goal: party ball splits coherently (fixed: tiles mismatched before mesh identity), confetti, GOAL text, replay, next stage fly-in |
| `hfr_fall` | 2500, 2900 | fall-out, respawn (camera-reset cut) |
| howto2 | 3700-4400 | How to play pages |
| story | 2000-8000 | cutscene with dialogue (text typed discretely), story select |
| race / target / bowling / boat / dogfight / baseball | 2700-3300 | all six launcher party games; boat water/reflection copies re-executed per frame; dogfight damage flash (overlay on alternate ticks) switches at the midpoint |
| baseball intro pan | `runs/tbb3` 2280, 2330 | camera reset every tick while panning: interpolated (even steps 9.1/9.1/9.1/9.1; before the soft-cut test: 0/20.6/0/0) |
| menus text change | `runs/t2d/menus` 1461 | message box text replaced: clean switch at the midpoint (before the 2D rule: letters flew to the old text's letters) |
| story select | `runs/t2d/story` 8000 | bubble sprites move individually: paired (mutual nearest) and interpolated |
| 32-bit build | `runs/gl32`, `runs/dx32` hfr_roll 2600 | same step pattern as x64 (6.6/6.9/6.9/7.1), no crashes |

Statistics per mode (`[hfr]` lines, s1): EFB copies re-executed 11576 (boat),
12672 (bowling), 5094 (story), 2149 (menus), kept (feedback / sync) 0
everywhere; texture hazards 0; camera cuts by hook / view jump / heuristic
e.g. story 28 / 26 / 0.

Bugs found and fixed with this (all in the git history):
the first in-between frame showed the tick frame (a present drew the
deferred tick frame first; 943f1a5), letters of a replaced message flew
to the old text (2D nearest pairing; now 2D pairs only when the group moved
together or the pair is unambiguous), baseball's per-tick camera reset
disabled interpolation, and the goal party ball is 4-vertex strip
pieces that share their first vertex with the neighbouring strip; pairing by
position + first vertex swapped pieces mid-opening.  Mesh identity is now
first + last vertex, model draws only pair with the same mesh.

### Frame rate on the real display (`runs/p_*`)

Final build (`runs/f_*`, after the in-between fix in 943f1a5):

| run | scenes | windows | fps (median / worst) | GPU ms/frame | interval p50 / p95 / p99 ms | replay ms | match ms | first response ms | 3D draws/tick |
|---|---|---|---|---|---|---|---|---|---|
| f_dx_240/hfr_roll | gameplay | 7 | 86 / 60 | 5.82 | 15.35 / 16.70 / 16.75 | 1.12 | 1.02 | 5.1 | 4347 |
| f_dx_240/hfr_roll | light | 6 | 237 / 107 | 0.38 | 4.15 / 4.28 / 6.32 | 0.10 | 0.07 | 4.5 | 70 |
| f_gl_240/hfr_roll | gameplay | 8 | 105 / 62 | n/a | 9.43 / 16.65 / 16.65 | 1.75 | 1.07 | 7.5 | 4576 |
| f_gl_240/hfr_roll | light | 5 | 228 / 157 | n/a | 4.15 / 4.20 / 8.35 | 0.09 | 0.06 | 4.3 | 0 |
| f_dx_120/hfr_roll | gameplay | 8 | 81 / 60 | 6.76 | 12.43 / 16.73 / 16.80 | 1.10 | 0.97 | 8.8 | 3758 |
| f_dx_120/hfr_roll | light | 5 | 120 / 89 | 0.35 | 8.30 / 8.45 / 8.55 | 0.10 | 0.06 | 8.7 | 0 |
| f_gl_120/hfr_roll | gameplay | 8 | 79 / 60 | n/a | 15.08 / 16.65 / 16.75 | 2.08 | 1.09 | 9.4 | 5290 |
| f_gl_120/hfr_roll | light | 5 | 120 / 91 | n/a | 8.30 / 8.30 / 8.40 | 0.07 | 0.06 | 8.4 | 0 |
| f_dx_240/boat | gameplay | 4 | 114 / 107 | 5.29 | 8.35 / 12.97 / 16.67 | 1.47 | 0.83 | 9.4 | 4484 |
| f_dx_240/boat | light | 6 | 240 / 105 | 0.49 | 4.15 / 4.25 / 4.57 | 0.10 | 0.06 | 4.5 | 62 |
| f_dx_240nv/hfr_roll | gameplay | 7 | 114 / 61 | 5.96 | 7.90 / 16.65 / 16.75 | 1.02 | 0.92 | 5.5 | 4068 |
| f_dx_240nv/hfr_roll | light | 6 | 237 / 105 | 0.44 | 4.15 / 4.25 / 6.35 | 0.10 | 0.07 | 4.5 | 68 |

Broader matrix on the build before that fix (`runs/p_*`; it drew one extra
replay per tick, so these numbers are slightly pessimistic):

3840x2160 borderless fullscreen, MSAA 4x, V-Sync on unless `nv`, modes.sh
scripts (`hfr_roll` = Challenge 1-1 at full tilt, goal + party ball +
replay, then stage 2).  Medians over 5-second windows; "gameplay" = windows
with >= 1000 3D draws per tick, "light" = menus / loading / pause.
Interval percentiles are present-to-present times.  GPU ms = D3D12
timestamps per frame (GL: n/a).

| run | scenes | windows | fps (median / worst) | GPU ms/frame | interval p50 / p95 / p99 ms | replay ms | match ms | first response ms | 3D draws/tick |
|---|---|---|---|---|---|---|---|---|---|
| p_gl_240/hfr_roll | gameplay | 8 | 85 / 60 | n/a | 12.70 / 16.65 / 16.65 | 1.06 | 1.02 | n/a | 4526 |
| p_gl_240/hfr_roll | light | 5 | 239 / 129 | n/a | 4.15 / 4.20 / 4.25 | 0.06 | 0.05 | n/a | 0 |
| p_dx_240/hfr_roll | gameplay | 7 | 103 / 60 | 6.08 | 7.25 / 16.70 / 16.75 | 0.94 | 0.88 | n/a | 4180 |
| p_dx_240/hfr_roll | light | 6 | 203 / 104 | 0.78 | 4.15 / 10.45 / 10.57 | 0.08 | 0.06 | n/a | 13 |
| p_gl_120/hfr_roll | gameplay | 8 | 88 / 60 | n/a | 9.27 / 16.65 / 16.70 | 1.29 | 1.02 | n/a | 5105 |
| p_gl_120/hfr_roll | light | 5 | 120 / 93 | n/a | 8.30 / 8.45 / 8.45 | 0.06 | 0.06 | n/a | 0 |
| p_dx_120/hfr_roll | gameplay | 8 | 80 / 61 | 5.45 | 13.03 / 16.67 / 16.75 | 1.55 | 1.54 | n/a | 3815 |
| p_dx_120/hfr_roll | light | 5 | 120 / 90 | 1.06 | 8.30 / 8.45 / 8.50 | 0.09 | 0.05 | n/a | 0 |
| p_dx_240nv/hfr_roll | gameplay | 7 | 84 / 60 | 5.78 | 11.70 / 16.70 / 16.75 | 0.92 | 0.93 | n/a | 3185 |
| p_dx_240nv/hfr_roll | light | 6 | 231 / 106 | 0.54 | 4.15 / 5.90 / 7.22 | 0.11 | 0.10 | n/a | 69 |
| p_dx_uncnv/hfr_roll | gameplay | 7 | 86 / 60 | 6.89 | 12.50 / 16.70 / 16.80 | 0.90 | 0.83 | n/a | 3181 |
| p_dx_uncnv/hfr_roll | light | 6 | 1277 / 232 | 0.38 | 0.30 / 3.42 / 6.12 | 0.07 | 0.07 | n/a | 68 |
| p_gl_240nv/hfr_roll | gameplay | 8 | 74 / 60 | n/a | 16.60 / 16.65 / 16.70 | 1.28 | 1.02 | n/a | 5122 |
| p_gl_240nv/hfr_roll | light | 5 | 239 / 158 | n/a | 4.15 / 4.20 / 4.30 | 0.07 | 0.06 | n/a | 0 |
| p_dx_240/boat | gameplay | 4 | 97 / 93 | 6.36 | 9.72 / 14.20 / 15.10 | 1.30 | 0.86 | n/a | 4436 |
| p_dx_240/boat | light | 6 | 236 / 104 | 0.89 | 4.15 / 4.78 / 5.62 | 0.10 | 0.06 | n/a | 64 |
| p_gl_240/boat | gameplay | 5 | 60 / 60 | n/a | 16.60 / 16.90 / 17.55 | 3.01 | 0.91 | n/a | 4639 |
| p_gl_240/boat | light | 5 | 239 / 157 | n/a | 4.15 / 4.25 / 4.35 | 0.09 | 0.07 | n/a | 0 |

| run | scenes | windows | fps (median / worst) | GPU ms/frame | interval p50 / p95 / p99 ms | replay ms | match ms | first response ms | 3D draws/tick |
|---|---|---|---|---|---|---|---|---|---|
| p_dx_120/boat | gameplay | 5 | 95 / 93 | 6.09 | 9.55 / 14.20 / 15.10 | 1.28 | 0.83 | n/a | 4308 |
| p_dx_120/boat | light | 5 | 120 / 91 | 0.75 | 8.30 / 8.45 / 8.55 | 0.09 | 0.06 | n/a | 0 |
| p_gl_120/boat | gameplay | 5 | 61 / 60 | n/a | 16.65 / 16.85 / 17.25 | 3.11 | 0.86 | n/a | 4606 |
| p_gl_120/boat | light | 5 | 115 / 94 | n/a | 8.30 / 8.45 / 16.65 | 0.06 | 0.06 | n/a | 0 |
| p_dx_240/menus | gameplay | 5 | 108 / 74 | 4.18 | 10.25 / 16.65 / 16.70 | 0.79 | 0.38 | n/a | 1920 |
| p_dx_240/menus | light | 1 | 152 / 152 | 1.55 | 4.15 / 16.65 / 16.70 | 0.11 | 0.06 | n/a | 157 |
| p_dx_240/dogfight | gameplay | 5 | 175 / 124 | 3.98 | 5.50 / 6.80 / 11.95 | 0.78 | 0.61 | n/a | 2211 |
| p_dx_240/dogfight | light | 5 | 239 / 136 | 0.50 | 4.15 / 4.35 / 4.45 | 0.09 | 0.06 | n/a | 0 |
| p_dx_240/baseball | gameplay | 4 | 179 / 121 | 2.21 | 5.10 / 9.07 / 10.93 | 1.46 | 1.30 | n/a | 6530 |
| p_dx_240/baseball | light | 6 | 224 / 100 | 0.76 | 4.15 / 6.32 / 8.35 | 0.08 | 0.06 | n/a | 68 |
| p_gl_240/story | gameplay | 25 | 93 / 60 | n/a | 11.55 / 16.65 / 16.65 | 1.22 | 1.48 | n/a | 4316 |
| p_gl_240/story | light | 2 | 160 / 153 | n/a | 4.15 / 16.65 / 16.67 | 0.32 | 0.12 | n/a | 479 |
| p_dx_240/story | gameplay | 25 | 120 / 60 | 4.25 | 9.60 / 16.65 / 16.70 | 1.00 | 1.12 | n/a | 3584 |
| p_dx_240/story | light | 2 | 154 / 153 | 1.59 | 4.15 / 16.65 / 16.70 | 0.12 | 0.08 | n/a | 288 |
| p_dx_display/hfr_roll | gameplay | 7 | 110 / 62 | 5.77 | 6.25 / 16.65 / 16.70 | 0.91 | 0.90 | n/a | 4197 |
| p_dx_display/hfr_roll | light | 6 | 236 / 106 | 0.61 | 4.15 / 4.85 / 5.57 | 0.09 | 0.07 | n/a | 69 |
| p_gl_uncnv/hfr_roll | gameplay | 8 | 76 / 60 | n/a | 16.65 / 16.65 / 16.65 | 1.39 | 1.02 | n/a | 5055 |
| p_gl_uncnv/hfr_roll | light | 5 | 1399 / 302 | n/a | 0.55 / 1.55 / 2.15 | 0.06 | 0.06 | n/a | 0 |

Reading the table (window detail, `tools/hfr/hfrperf.py runs/p_dx_240/hfr_roll/log.txt`):

* Menus / light scenes: 240 fps locked (p50 4.15, p99 4.3 ms) at 240,
  120 locked at 120; uncapped (V-Sync off) 1300-1400 fps.
* Light gameplay (stage 2, 2.7-3.2k draws, GPU ~3 ms/frame at 4K):
  D3D12 240 -> 180-199 fps (p50 4.2-4.7 ms, p99 8.1-8.4 ms; final build);
  120 -> 120 locked (p99 9.2).
* Party games (D3D12 240): dogfight 175, baseball 179, boat 97 (copies),
  story cutscenes 120 (median).
* Heavy windows (goal party ball + replay, 8-10k draws, GPU 6-12 ms/frame at
  4K): 60-64 - the GPU cannot draw a second frame in the tick; the pacer
  keeps the game at 60 (worst window over all runs: 60 fps).
* GL vs D3D12: GL's replay is slower per draw and has no GPU timing, so it
  reaches fewer in-between frames in gameplay (boat stays at 60: its EFB
  copies blit the whole 4K frame per copy).  **D3D12 is the recommended
  renderer for high frame rates at 4K.**
* No game slowdown: every 5 s window in every run >= 60 ticks/s; ticks
  that would run late degrade to the plain 60 fps path.
* Before the D3D12 shader fix, the same gameplay was GPU-bound at 60 fps
  even without HFR (100% GPU at 4K MSAA 4x; now ~40-55% at 60 fps).

### Latency

The tick frame is presented at the same time as at 60 fps (no added
latency for the newest tick).  The first frame that shows the new tick
(the first in-between frame) is presented, measured from the tick start
(input already read): **240 fps: 4.5-5.1 ms, 120 fps: 8.4-8.8 ms, vs 16.7 ms
at 60 fps** (statistic `first` in the `[hfr]` line, table column "first
response").  In heavy windows without in-between frames it is 16.7 ms,
like 60 fps.

## 9. 60 fps = today

With the frame-rate setting at 60 the proxy is never selected
(`hfr_select` returns the backend itself): nothing is recorded or
replayed, the hooks return at once (no batch flush), and the pacer takes
the 60 Hz path.  Verified against a dx12-x64 baseline build (x64, modes
intro + menus + challenge, 51 dumped frames per renderer; the game is not
fully deterministic, so each frame is judged against a second baseline run):

| renderer | base = base2 and base = new | base = base2, base != new | base != base2 |
|---|---|---|---|
| GL | 20 | 1 (menus 500: max diff 1, a few pixels) | 30 |
| D3D12 | 20 | 3 (frame 600 of intro/menus/challenge: sparkle trail) | 28 |

The exceptions are run-to-run variation: frame 600's sparkle trail differs
between the two GL baseline runs too (and matches base vs new there), and
GL menus 500 differs by one level in a handful of pixels.  Lists:
`runs/gl60.txt`, `runs/dx60.txt`.  The D3D12 pixel shader change (array
selects, used-texcoord mask) is arithmetically identical and part of this
check.

Determinism: the logic loop, its inputs and its order are untouched;
replays run no game code and never touch gx_core state the game reads
(GC-memory EFB copies keep the tick's copy: replays use
`GXCOPY_NO_READBACK`), so replays, ghosts, memory cards, RNG and the
32/64-bit identity are unaffected at any frame rate.

## 10. Known artefacts / limits

* **GPU cost at 4K**: the renderer draws every GX batch as its own draw
  (4-10k draws per frame in gameplay), and the D3D12 GPU time per frame
  (timestamps) is ~4 ms for 3k draws and 6-9 ms for 8-10k draws at
  3840x2160 with MSAA 4x, about half of it per-draw overhead (720p: 1.5 /
  3-5 ms).  So at 4K, 240 Hz is reached in menus and light scenes, light
  gameplay runs 120-170 fps and the heaviest scenes stay near 60-110.
  Making the backends cheaper per draw (merging GX batches, bindless
  constants) is renderer work outside HFR.
* Discrete content (timer digits, text being typed, sprite frames,
  objects appearing) changes at the midpoint between ticks, i.e. it may
  appear up to half a tick earlier or later than at 60 fps.
* EFB copies made through the synchronous / CPU path (Z copies, formats
  the GPU conversion does not cover) and feedback passes keep the tick's
  copy (60 Hz).  In the tested modes every copy took the async path and
  none was kept (statistics `copies N kept 0`).
* Objects whose model-space vertices change completely between ticks
  (CPU-deformed meshes with a new vertex order) are not paired: they step
  at 60 Hz (switch at the midpoint).
* Rigid transforms turning more than ~150 degrees per tick are not
  interpolated (ambiguous direction); a fast-rolling ball's texture can
  alias at 60 Hz anyway.
* The `[hfr]` statistics line is printed every 5 s while HFR is on.
