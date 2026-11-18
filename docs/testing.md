# Testing the game without a human

Everything here runs from **Git Bash on Windows** against the built exe
(`build64/bin/smb2_pc_port.exe`; build with `tools/win_build.sh`, see
`docs/ONLINE_PLAN.md` section 4.3). Quick start:

```
tools/smoke/boot_to_menu.sh       # ~30 s: title -> Mode Select, PASS/FAIL
tools/smoke/party_race_2p.sh      # ~65 s: Monkey Race 2, 2 players, PASS/FAIL
```
Both take an optional exe path, honour `SMB2_ISO` (disc image; default is the dev
machine's ISO) and `SMOKE_OUT` (output dir, default `mktemp -d`), print `PASS:`/`FAIL:`
and exit 0/non-zero (2 = missing exe/ISO). They print the artifact dir (log, frame dumps).

Always use a throwaway `--save-dir` and `--windowed` (the scripts do). The game runs at
roughly 40-60 frames per second here.

## Command-line flags (test relevant)

| Flag | Meaning |
|---|---|
| `--iso <file>` / `--gamedata <dir>` | disc image / extracted game folder |
| `--save-dir <dir>` | settings.dat + memory card live here; use a temp dir |
| `--windowed` | no fullscreen (automated runs already default to windowed) |
| `--audio-null` | audio without an output device (no sound card needed) |
| `--exit-frame N` | `exit(0)` when frame N is swapped; log `[exit-frame] reached frame N - exiting.` |
| `--input-script "<events>"` | scripted input (below); same as env `SMB2_INPUT_SCRIPT` |
| `--dump-interval N` | write `frame_NNNNN.ppm` when frame is 120 or a multiple of N |
| `--dump-from N` | dump every frame from N on |
| `--dump-ranges "a-b,c-d"` | dump every frame in the ranges (max 32) |
| `--overlay-open` / `--overlay-open-frames a-b` | open the ImGui overlay at start / for swapped frames a..b-1 (GL only; frame dumps include it; F2 toggles by hand) |
| `--trace-frame N` | GL call trace of frame N on stderr |
| `--renderer gl\|d3d12`, `--width/--height`, `--msaa`, `--fps`, `--no-vsync`, `--language`, `--button-style`, `--perf`, `--audio-wav f` | as in `--help` |

Note: the exit check runs *before* the dump in a frame, so to dump frame N use
`--exit-frame N+1`.

## Environment variables

Input/dump: `SMB2_INPUT_SCRIPT`, `SMB2_DUMP_INTERVAL` (same as the flags),
`SMB2_BALL_LOG=1` (`[ballf]` per-frame ball state in gameplay), `SMB2_BUTTON_STYLE`,
`SMB2_RENDERER`.
Test hooks in the game shims: `SMB2_PC_LAUNCHER=1` (single-player test launcher menu that
starts Challenge/Practice/party games directly; used by `tools/parity/scenes.sh`),
`SMB2_PC_STAGE=<id>|<id,id..>|floors`, `SMB2_PC_WIDE=1`, `SMB2_PC_DIP=<hex>` (debug DIP
switches), `SMB2_PC_ROLL=1`, `SMB2_PC_GOLF_WIND`.
Rendering debug: `SMB2_D3D12_{DEBUG,FLIPFACE,NOCULL,NODEPTH,TRACECOPY,DEBUGVIS,FORCEFAIL}`,
`SMB2_HFR_{FIXED,SWITCH,DEFER,PROFILE,DEBUG}`, `SMB2_TEX_{DEBUG,BUDGET}`,
`SMB2_COPY_CPU_RESAMPLE`, `SMB2_NO_PERSISTENT_VBO`, `SMB2_PERF_FRAMES`.
Other: `SMB2_HEAP_LOG`, `SMB2_HEAP_CHECK`, `SMB2_DVD_TRACE`, installer/updater test hooks
(`SMB2_UPDATE_*`, `SMB2_PC_LOCAL_GAMEDATA`, `SMB2_*_PICK_TEST`).

## Input script format

`"<when>:<TOKEN>,<when>:<TOKEN>,..."`; `<when>` is a frame number (token held 10 frames) or
`a-b` (held for frames a..b-1). Frames count input polls (one per game frame, from 1); this
is within a frame or so of the swap count used by `--exit-frame`. Max 4096 events. Events
may overlap. Unknown tokens are skipped silently; the log echoes each parsed event as
`[input-script] a-b TOKEN` and each change as `[input-script] frame N held=0x....`.
Scripts act on **player 1** (port 1, which is always connected).

* Buttons: `START A B X Y L R Z UP DOWN LEFT RIGHT`
* Main stick: `SU SD SL SR` (full deflection, game-side value +-100), `GXn`/`GYn` (exact
  game-side value), `SXn SYn` (raw emulated GC pad value -128..127, runs through the
  game's stick processing). C stick: `CXn CYn`.
* Analog triggers: `TL TR` (full), `TLnnn TRnnn` (0-255), `ROW`/`ROWnn` (alternate L/R pulls
  every nn frames, Monkey Boat paddling).
* `AUTO`: closed-loop steering of ball 0 toward the stage origin while in main mode 2.
* Overlay hotkeys: `F1`, `ESC`, `STK`, `UPD`, `MC`.
* **Virtual pads** (SDL virtual joysticks, go through the real pad mapping):
  `VADDX VADDP4 VADDP5 VADDS` plug in an Xbox/PS4/PS5/Switch pad on the first free port
  (with nothing else connected that is port 1!); `VADDX2 VADDP42 VADDP52 VADDS2` do the
  same but skip port 1, so the script/keyboard stays player 1 and the pad is **player 2**
  (added in P0.4). `VDEL` unplugs. While plugged in, `VBn` holds SDL button n
  (A=0 B=1 X=2 Y=3 BACK=4 GUIDE=5 START=6 LSTICK=7 RSTICK=8 LSHOULDER=9 RSHOULDER=10
  UP=11 DOWN=12 LEFT=13 RIGHT=14), `VLT/VRT[n]` triggers, `VLXn VLYn VRXn VRYn` stick axes
  (-32768..32767). Only one virtual pad exists at a time, so 2 players max via script
  (3-4 players is not scriptable yet).

Examples:
```
1350:START,1450:START,1550:START                  # skip intro, reach Mode Select
1600:VADDX2,3100:VB0                               # P2 pad appears, presses A at 3100
200-400:SU,300:A                                   # hold stick up 200..399, tap A
```
The title/intro needs ~1350 frames before it accepts START; presses before that are lost
(use the timings in `tools/smoke/*.sh` and `tools/parity/scenes.sh` as a model). Timing is
deterministic for a given build, so the scripts use fixed frames and then check the log.

## Frame dumps

Dumps are binary PPM (P6, 1600x900 for the default window) named `frame_NNNNN.ppm`
(`frame_NNNNN_j.ppm` for HFR in-between frames), written to the **exe's current working
directory**: run with `cd <outdir>`. The log gets `[framedump] frame N: WxH, lit/total
non-black px, ...` per dump (a near-0 lit count means a black frame).
To view: convert to PNG with `tools/ppmtool.c` (needs zlib; build in WSL:
`wsl -d Ubuntu -- gcc -O2 -o /tmp/ppmtool /mnt/c/.../tools/ppmtool.c -lz`, then
`ppmtool topng in.ppm out.png [downscale]`, `ppmtool sheet out.png <scale> a.ppm b.ppm ...`
for a contact sheet, `ppmtool diff a.ppm b.ppm`). Use `MSYS_NO_PATHCONV=1` and `/mnt/c/...`
paths when calling WSL from Git Bash. A downscale of 3-4 keeps PNGs cheap to view.

## Reading the log

Once a second (every 60 frames): `[frame N] mode M/S (req RM/RS)`: `main_mode`/`sub_mode`
and the requested next ones (-1 = none), names in `src/main_loop/mode.c` of the decomp.
Main modes: 0 MD_ADV (boot/title/attract), 1 MD_SEL (menus), 2 MD_GAME (main game),
3 MD_TEST, 4 MD_MINI (party games), 5 MD_OPTION, 6 MD_AUTHOR, 7 MD_EXOPT. Sub modes are an
index into `SUB_MODE_NAMES` (0 = SMD_ADV_TOP). Useful ones:

| mode/sub | name | what you see |
|---|---|---|
| 0/25 | SMD_ADV_FIRST_LOGO_MAIN | first boot logo |
| 0/8 | SMD_ADV_DEMO_MAIN | intro movie / attract demo |
| 0/10, 0/20 | SMD_ADV_START_MAIN, SMD_ADV_TITLE_MAIN | start / title screen |
| 1/32 | SMD_SEL_NGC_MAIN | Mode Select and the party/option submenus |
| 2/51 | SMD_GAME_PLAY_MAIN | Main Game stage in play |
| 4/186 | SMD_MINI_RACE2_MAIN | Monkey Race 2 |

In main mode 2 the heartbeat also prints `[ball]` telemetry. Other lines:
`[input] pressed=0x....` (pad bits), `[input] port N: ... connected`, `[vpad] ...`,
`[exit-frame]`, `[framedump]`. Menus that stay in 1/32 (Mode Select, Party Game Select,
Number of Players, character/course select) are told apart by frame dumps, not by mode.

## Typical durations

Intro to Mode Select: ~1600 frames (~30 s). Party Games > Monkey Race 2 > 2 players >
race countdown: ~3700 frames (~65 s). Menu transitions have loading screens; leave 200-300
frames between presses on different screens.

## Known gaps

* No assertion API beyond log/frames; compare dumps with `ppmtool diff`.
* Only one virtual pad: no scripted 3rd/4th player.
* The race smoke test exits at the countdown; race inputs are not scripted.
