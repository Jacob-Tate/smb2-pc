# SMB2 PC: Online multiplayer + port fixes (master plan)

> **This file is the single source of truth.** Any session (or a fresh one after context
> runs out) resumes by reading **§0 Resume protocol**, then **§1 Status board**.
> Created 2026-11-17. Reference project: `C:\Users\jacob\source\mario\partyboard`
> (Mario Party 4 port with working online play, branch `netplay`).

---

## 0. Resume protocol (read first, every session)

1. Read §1 (status board) and §8 (session log, newest entry at the bottom).
2. For every task marked `IN PROGRESS`: open its handoff file `docs/handoff/<ID>.md`
   (if present) and check its branch/worktree (`git branch -a`, `git worktree list`).
   Decide: resume it (spawn a new subagent with the handoff file as context) or reset it to `TODO`.
3. Pick the next tasks: status `TODO`, all `Deps` are `DONE`, and no file-lane conflict with
   anything already `IN PROGRESS` (§3.3). Run independent ones in parallel.
4. Before spawning: set the task to `IN PROGRESS` in §1 with the branch name and date, save
   the file. After the subagent returns: verify (§3.4), merge, set `DONE`/`BLOCKED`, append
   to §8. **Update this file after every task, not at the end of the session.**
5. Questions only the user can answer go in §2 "Open questions". Don't block on them if a
   default is listed; proceed with the default and note it.

---

## 1. Status board

Statuses: `TODO` · `IN PROGRESS (<branch>, <date>)` · `REVIEW` · `DONE (<commit>)` · `BLOCKED (<why>)` · `USER` (needs the user)

| ID | Task | Deps | Lane | Status |
|---|---|---|---|---|
| P0.1 | Fork repo, create `online` integration branch | – | git | DONE (fork Jacob-Tate/smb2-pc, branch `online`) |
| P0.2 | WSL toolchain + decomp + SDL2 deps | – | env | DONE (2026-11-17) |
| P0.3 | `tools/win_build.sh` wrapper + smoke test | P0.2 | build | DONE (fa5fad2) |
| P0.4 | Headless test harness doc + smoke scripts | P0.3 | tools | DONE (bc95b0b) |
| A1 | Default to windowed instead of fullscreen | P0.3 | settings | REVIEW (merged 81bb9a9; user visual check pending) |
| B1 | Disc reader abstraction (`DiscReader`) in iso.c | P0.3 | disc | DONE (0be1727) |
| B2 | RVZ decoder (zstd, GC-only, junk regen) | B1 | disc | DONE (7e17185) |
| B3 | RVZ in dialogs, installer, help text, gamedata.c | B2, **C2c** (both edit platform_sdl2.c) | disc+installer | DONE (c5b51f6) |
| B4 | RVZ verification vs. the user's ISO | B2 | disc | DONE (SHA-256 identical, in B2) |
| B5 | (Optional) RVZ hardening: check header SHA-1s / zstd frame checksums | B2 | disc | TODO (low priority) |
| C1 | Steam Controller diagnosis (research) | – | input | DONE (docs/handoff/C1.md) |
| C2a | `--list-controllers` diagnostic flag | – | input | SKIPPED (SDL2 can't support the controller anyway; the flag moves into C2d, on SDL3) |
| C2b | SDL3 3.4.x static build for i686 + x86_64 MinGW (`tools/build_sdl3_mingw.sh`) | P0.2 | env | DONE (49c8af2) |
| C2c | Port platform/video/render/audio/assets glue to SDL3 **together with C2d** (one exe can't link SDL2 and SDL3) | C2b, P0.4 | platform+input | REVIEW (merged c01ae12; user checks controller, keyboard, dialogs) |
| C2d | Port input to SDL3 gamepad API; Steam Controller works | (inside C2c) | input | REVIEW (merged c01ae12, inside C2c) |
| C2e | README/docs for SDL3; pin version | C2d | docs | DONE (README build steps + credits done in C2c) |
| D1 | Dear ImGui integration: **SDL3** + OpenGL backend | C2c | overlay | DONE (227973c) |
| D2 | Dear ImGui: Direct3D 12 backend | D1 | overlay+d3d12 | DONE (c2733e4) |
| D3 | Overlay input routing + gamepad nav + open chord | D1 | overlay+input | REVIEW (merged a214292; user checks gamepad nav + cursor hide) |
| D4 | Overlay tabs: Video/Graphics, Audio, Input, Game, Interface | D3 | overlay | TODO |
| D5 | Replace old F1 overlay; FPS counter + toasts | D4 | overlay | TODO |
| E1 | Determinism audit (read-only report) | – (source only) | none | DONE (docs/handoff/E1.md; runtime tests T1–T9 go to E4) |
| E2 | Deterministic session mode (`--det-session`) | E1 | game shim | DONE (f688666) |
| E3 | Input record/replay + state checksum | E2 | input+shim | DONE (3e0e393) |
| E4 | Local determinism test (two runs, same checksums) | E3 | tools | IN PROGRESS (task/E4, 2026-11-17) |
| E5 | Protocol header + relay server (port from partyboard) | E1 | server | DONE (1f9afa1) |
| E6 | Netplay client (ENet, lockstep) | E3, E5 | netplay | DONE (merge + fixup d42e44d) |
| E7 | Online lobby UI in the overlay | D4, E6 | overlay+netplay | TODO |
| E8 | Session start flow + sim-affecting settings sync | E6 | netplay | TODO |
| E9 | Disconnect → CPU/neutral pad, desync handling | E6 | netplay | TODO |
| E10 | Server Docker image, CI, Portainer stack | E5 | server | DONE (ce09eff; CI green, image published) – deploy is USER |
| E11 | Two-PC LAN test + internet test (user) | E7, E8, E9, E10 | – | USER |
| F1 | "Friend build": single self-extracting `SMB2.exe` (like partyboard's) | P0.3 | packaging | REVIEW (merged 2f493e9; user checks Shift-start dialog) |
| F2 | Build number in window title, log, overlay, and as the netplay build id | C2c (window title in platform_sdl2.c) | platform | DONE (c5b51f6, fix 372d700) |
| E12 | (Stretch) Rollback feasibility spike | E4 | none | TODO |
| X1 | Integer divide-by-zero crash (0xC0000094) in Mode Select: script `1350:START,1450:START,1550:START,1800:A,2300:A,2400:RIGHT,2420:RIGHT`, no pad; also on pre-E3 builds | – | game shim | DONE (merged; vectored handler emulates PPC divw) |
| E13 | (Stretch) Per-player full-screen view online | E11 | render | TODO |

Dependency graph (parallel lanes after P0.3):

```
P0.1 ─┐
P0.2 ─┴─ P0.3 ─┬─ P0.4
               ├─ A1
               ├─ B1 ─ B2 ─┬─ B3
               │           └─ B4
               ├─ C1 ─ C2
               ├─ D1 ─┬─ D2
               │      └─ D3 ─ D4 ─ D5 ───────────────┐
               └─ E1 ─┬─ E2 ─ E3 ─┬─ E4 ─ E12         │
                      └─ E5 ─┬────┴─ E6 ─┬─ E7 ◄─────┘
                             └─ E10      ├─ E8
                                         └─ E9 ──► E11 ─ E13
```

---

## 2. Decisions and open questions

### Decisions (change only with the user's agreement; record changes here)

| # | Decision | Why |
|---|---|---|
| D-1 | **Lockstep input relay** first (same model as partyboard): only pad input crosses the network; every PC runs the full game. Rollback is a later spike (E12). | The port already has deterministic math (`docs/detmath.md`) and frame-locked synth audio (`docs/audio.md`), so lockstep is mostly done. Rollback needs state snapshots, which is a big separate project. |
| D-2 | Reuse partyboard's **ENet/UDP relay server** design and code (rooms with 4-letter codes, checksums every 60 frames). New default port **7855/udp** so it can run next to partyboard (7845). | Proven code. The server never runs game code. |
| D-3 | Overlay uses **Dear ImGui** (pinned v1.91.x), styled after partyboard's overlay (tabs, left list + right help pane, toasts, FPS corner). Partyboard uses RmlUi on Aurora; smb2-pc has its own OpenGL 3.3 and D3D12 renderers, and ImGui ships official SDL2, OpenGL3 and DX12 backends, so no render backend has to be written by hand. | Least custom rendering code; gamepad navigation built in. |
| D-4 | The project gets a C++ language (`project(... LANGUAGES C CXX)`) for ImGui and the netplay client. Game code stays C. Overlay code goes in `src/overlay/`, netplay in `src/netplay/`. | ImGui is C++. Keep the C game/shim boundary clean via `extern "C"` headers. |
| D-5 | RVZ is decoded **natively in C** (`src/assets/rvz.c`) with vendored zstd (decompressor only). Supported: GameCube RVZ, compression zstd or none. bzip2/LZMA/LZMA2 RVZ is rejected with a clear message ("re-convert in Dolphin with zstd"). | Dolphin's default RVZ compression is zstd. Avoids a Rust (nod) dependency in a mingw cross build. |
| D-6 | Online sessions start from a **fresh deterministic boot** of the game with a session memory card (no real saves read or written), and lockstep runs from frame 0. | Removes boot-time and save-data differences between PCs; simplest correct start. |
| D-8 | **Migrate the port from SDL2 2.30.9 to SDL3 (3.4.10 or newer 3.4.x), statically linked** (C2b–C2e). The overlay (D1+) is built on the SDL3 ImGui backend, so D1 waits for C2c. | C1 found: the new Steam Controller ("Triton", VID 0x28DE PID 0x1302–0x1305) is only supported by SDL3 ≥ 3.4.0 (`SDL_hidapi_steam_triton.c`, on by default on Windows); no SDL2 release has it. sdl2-compat can't be linked statically on MinGW. Official SDL3 MinGW devel package is DLL-only → build static libs from source. Estimate ~330 `SDL_` references in 20 files. |
| D-9 | **Target only the Windows x64 build for now** (user, 2026-11-17). Build and test x64 only; i686 builds are not required, and cross-arch (x64 vs i686) netplay compatibility is out of scope (drops E1's T6 / E4's i686 runs). | User priority. Overrides the "both x64 and i686" rule in §3.2 and task acceptance lines. |
| D-7 | Subagents never edit this file. They write `docs/handoff/<ID>.md` and return a report; the orchestrator updates this file. | No merge conflicts on the plan between parallel worktrees. |

### Open questions for the user (defaults in bold; proceed with default if no answer)

| # | Question | Default |
|---|---|---|
| Q-1 | Which Steam Controller? | **ANSWERED 2026-11-17: the new (2026) one, game launched directly (not via Steam). It works in partyboard, which uses SDL3 3.4.10 (`extern/aurora/cmake/AuroraDependencyVersions.cmake`); smb2-pc uses SDL2 2.30.9.** See C1/C2. |
| Q-2 | GitHub fork name for smb2-pc? | **ANSWERED: `Jacob-Tate/smb2-pc` (created), branch `online`; `upstream` = girlyoulookthebest/smb2-pc** |
| Q-3 | Server host: same Docker Swarm node as partyboard? | **ANSWERED: yes, same node, UDP 7855, image `ghcr.io/jacob-tate/smb2-server`** |
| Q-6 | Sudo in WSL | The user runs sudo commands themselves: give them the exact command and wait. |
| Q-4 | Existing users' `settings.dat` already says `display=borderless`. Migrate them to windowed too? | **No; only new installs default to windowed** |
| Q-5 | Keep the old in-game "Options > Game Screen" menu after the new overlay exists? | **Keep it (it's part of the game's menus); only the F1 overlay is replaced** |

---

## 3. Orchestration (how to drive Sonnet 5.5 subagents)

### 3.1 Roles

- **Orchestrator** = the main session (Opus). Reads this plan, picks tasks, spawns subagents,
  verifies, merges, updates §1/§8. Doesn't write large amounts of code itself.
- **Implementer** = `Agent` tool, `subagent_type: "general-purpose"`, `model: "sonnet"`.
  **Don't use `isolation: "worktree"`**: on this machine it fails (drive-letter case mismatch) and bases
  worktrees on `main`. Instead the orchestrator creates the worktree first:
  `git -C smb2-pc worktree add -b task/<ID> ../wt/<ID> online` →
  `C:\Users\jacob\source\super_monkey\wt\<ID>` (`/mnt/c/Users/jacob/source/super_monkey/wt/<ID>` in WSL),
  and the prompt tells the agent to work only there. Remove with `git worktree remove ../wt/<ID>` after merging. Read-only tasks (E1, E12, C1 analysis)
  skip the worktree.
- One task per subagent. If a task looks bigger than ~1 day of human work, split it first and
  add the pieces to §1.

### 3.2 Subagent prompt template (copy, fill in the `<>` parts)

```
You are implementing task <ID> "<title>" in the SMB2 PC port.
Repo: C:\Users\jacob\source\super_monkey\smb2-pc (you are in a git worktree of it).
Read first, in this order:
  1. docs/ONLINE_PLAN.md §4 (environment & build) and the section for task <ID> (§5/§6/§7).
  2. docs/handoff/<ID>.md if it exists (a previous attempt's notes: continue from there).
  3. The files listed under "Files" for the task.
Reference project (read-only, never edit): C:\Users\jacob\source\mario\partyboard

Rules:
- Work on branch task/<ID>. Commit in small steps with clear messages. Don't push.
- Don't edit docs/ONLINE_PLAN.md. Keep docs/handoff/<ID>.md up to date as you go
  (what's done, what's left, gotchas, exact commands that worked), so another agent can
  continue if you stop mid-way. Commit it with your work.
- Build with tools/win_build.sh (§4.3). The build must pass for x64 (i686 not required, D-9).
- Follow the repo's existing conventions: C99 game/shim code, docs/detmath.md rules for
  anything game code touches, docs/x64.md for pointer-size issues.
- Don't touch files outside the task's "Files" list without saying why in the report.
- Meet every acceptance criterion and run every verification command in the task.
  If one cannot be met, stop and report BLOCKED with the reason; don't fake it.

Final message (your report) must contain exactly these sections:
STATUS: DONE | BLOCKED | PARTIAL
BRANCH / COMMITS: <branch>, <short hashes>
CHANGES: <files + one line each>
VERIFICATION: <each acceptance criterion → command run → result>
FOLLOW-UPS: <anything new for the plan, or "none">
```

### 3.3 Parallelism lanes (avoid merge conflicts)

Two tasks may run in parallel only if their **Lane** columns in §1 differ. Hot files:

| File | Owned by lane |
|---|---|
| `CMakeLists.txt` | first come; later tasks rebase. D1 and E6 both add targets: run D1 before E6. |
| `src/platform/platform_sdl2.c` (event loop) | overlay (D3) / input (C2) / netplay (E6): serialize these |
| `src/main/main.c` (CLI flags) | any; additions only, rebase on conflict |
| `src/platform/pc_settings.c` | settings (A1) / overlay (D4): A1 first |
| `src/input/input_sdl2.c` | input (C2) / overlay (D3) / netplay (E3): serialize |
| `src/assets/iso.c`, `gamedata.c` | disc lane only |
| `src/render/render_*.c` | overlay (D1, D2) / render (E13) |

### 3.4 Orchestrator verification checklist (after each subagent)

1. Read the report. Status not DONE → record in §1 as BLOCKED/PARTIAL with the reason; decide.
2. `git -C <worktree> log --oneline online..task/<ID>` and skim `git diff online...task/<ID> --stat`.
3. Re-run the task's verification commands yourself (at least the build and one functional check).
4. Merge: `git checkout online && git merge --no-ff task/<ID>`; rebuild both arches.
5. Update §1 (status + commit), add a §8 log entry, save. Remove the worktree when merged.
6. If the task needs a human check (anything visual, controllers, two PCs), mark `REVIEW` and
   list the exact steps for the user in §8.

---

## 4. Environment and project facts (so nobody re-derives them)

### 4.1 Layout

```
C:\Users\jacob\source\super_monkey\
├── smb2-pc\                       this repo (origin = github.com/girlyoulookthebest/smb2-pc)
├── smb2-decomp\                   clone of girlyoulookthebest/smb2-decomp (user cloned it; NOT
│                                  the README default ../decomp: always pass
│                                  -DSMB2_DECOMP_DIR=/mnt/c/Users/jacob/source/super_monkey/smb2-decomp)
├── toolchains\llvm-mingw-20241217-ucrt-ubuntu-20.04-x86_64\   (P0.2)
├── Super Monkey Ball 2 (USA).iso  user's disc image (1,459,978,240 bytes)
└── Super Monkey Ball 2 (USA).rvz  same game as RVZ (646,679,824 bytes): B4's test input
```

In WSL these are under `/mnt/c/Users/jacob/source/super_monkey/`. WSL distro: `Ubuntu` (WSL2).

### 4.2 Port architecture (key files)

| Area | Files | Notes |
|---|---|---|
| Entry, CLI flags | `src/main/main.c` | Flags incl. `--input-script`, `--exit-frame`, `--dump-interval`, `--audio-null`, `--save-dir`. Calls `_prolog()` (the game's own main loop). |
| Window/events | `src/platform/platform_sdl2.c` | SDL2 2.30.9 static. F1 → `pc_settings_overlay_request()` at ~line 174. Alt+Enter → `pcs_toggle_fullscreen`. |
| Settings | `src/platform/pc_settings.c`, `include/pc_settings.h` | `set_defaults()` ~line 74 sets `PCS_BORDERLESS` (the fullscreen default). Stored in `settings.dat`. |
| Old F1 overlay | `src/game/shim/pc_settings_menu.c` (`pc_settings_overlay_run`, modal loop, game font), called from `src/game/shim/video_shim.c:352` | Pauses the game while open. |
| Input | `src/input/input_sdl2.c` | Fills `pad_status_groups[4]` (GC `PADStatus`) in `input_poll_gc_pads()` ~line 742; keyboard on port 1; hot-plug; input scripts (`SMB2_INPUT_SCRIPT`); virtual pads for tests. Only `SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS` is set: **no HIDAPI/Steam hints**. |
| Stick emulation | `src/input/gc_stick.c`, `docs/stick.md` | |
| Disc | `src/assets/iso.c` (`ISOArchive`, raw `FILE*` + `fseek`/`fread`), `include/iso.h`, `src/assets/gamedata.c` (extraction/verify, opens the ISO directly ~line 348) | RVZ is detected by magic and **rejected** in `iso_open` (~line 152). Also an extracted-folder backend. |
| Disc pickers | `src/platform/platform_sdl2.c` ~lines 753, 814, 967, 1208; `installer/smb2.nsi` ~lines 199, 888, 961 | Filters say `*.iso;*.gcm`. Installer runs `smb2_pc_port.exe --extract-iso <img> --gamedata <dir> --progress`. |
| Renderers | `src/render/render_opengl.c`, `render_d3d12.c`, `gx_core.c`, `gx_hfr.c` (frame interpolation) | |
| Memory | `src/game/shim/gc_mem.c` | Game heap = one 24 MiB arena mapped at 0x80000000. Useful for checksums and E12. |
| Determinism | `include/pc_detmath.h`, `src/detmath/`, `docs/detmath.md` | `rand` = GC MSL LCG with seed 1; math bit-identical across x86/x64. |
| Audio | `src/game/shim/musyx_shim.c` (synth on game thread, game-time ticks), `src/audio/audio_miniaudio.c` (DSP worker thread, not observable by the game) | Already deterministic per `docs/audio.md`. |
| Time | `src/game/shim/os_shim.c` `OSGetTime`/`OSGetTick` (~line 428), `card_shim.c` ~424 | Wall clock: audit in E1. |
| Tests/tools | `tools/parity`, `tools/x64test`, `tools/stick_test`, `tools/hfr`, `tools/perf` | |

### 4.3 Build (WSL, cross-compiled; see README "Building from source")

- Requirements in WSL: `git cmake ninja-build python3 make mingw-w64`, llvm-mingw 20241217,
  SDL2 2.30.9 MinGW static libs in `smb2-pc/mingw_deps/{i686,x86_64}-w64-mingw32`.
- The decomp has to be built once (`python3 configure.py && ninja` with the ISO copied to
  `decomp/orig/GM2E8P/`).
- After P0.3, **always build through** `tools/win_build.sh` from Windows:
  `wsl -d Ubuntu -- bash -lc "cd /mnt/c/<path to repo or worktree> && tools/win_build.sh [x64|i686|both]"`
  It must accept absolute `SMB2_DECOMP_DIR`, `LLVM_MINGW_ROOT` and SDL deps paths so it works
  from any git worktree (worktrees don't contain the gitignored `mingw_deps/`).
- **Calling WSL from the Bash tool (Git Bash) – gotchas:**
  - Git Bash rewrites `/mnt/c/...` arguments into `C:/Program Files/Git/mnt/c/...`. Prefix the
    call with `MSYS_NO_PATHCONV=1`.
  - `wsl -- bash -lc '...$var...'` loses `$` expansions. Write the commands to a script file
    (scratchpad or `tools/`) and run `MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/.../script.sh`.
  - WSL output may contain NUL bytes (UTF-16 from wsl.exe itself): pipe through `tr -d '\0'`.
  - WSL distro is Ubuntu 26.04, Python 3.14; 32 cores.
- Output: `build64/bin/smb2_pc_port.exe` (+ `assets/`), `build32/bin/...`.
- **Verified 2026-11-17**: `tools/win_build.sh both` builds clean (RelWithDebInfo; the many -Wpedantic
  warnings in render_opengl.c are normal). In a worktree it symlinks `mingw_deps` to the main checkout's.
  Smoke: `build64/bin/smb2_pc_port.exe --iso "<iso>" --save-dir <tmp> --audio-null --windowed --exit-frame 900`
  → exit 0, log `[exit-frame] reached frame 900 - exiting.`
- No Python on Windows (only in WSL). `.gitattributes` forces LF for `*.sh`, `*.py`, CMake files.
- Run from Windows: `build64\bin\smb2_pc_port.exe --iso "C:\Users\jacob\source\super_monkey\Super Monkey Ball 2 (USA).iso" --save-dir <temp dir>`.
  Always pass a throwaway `--save-dir` in tests so the user's saves/settings are untouched.

### 4.4 Partyboard files to reuse (read-only reference)

| Partyboard | Use for |
|---|---|
| `docs/NETPLAY.md` | Overall design, determinism table, protocol history, lessons learned |
| `include/netplay/protocol.h` | Starting point for `include/netplay/protocol.h` (E5) |
| `server/main.cpp`, `server/test_client.cpp`, `server/CMakeLists.txt`, `server/Dockerfile`, `server/portainer-stack.yml`, `server/README.md` | Relay server (E5, E10) |
| `.github/workflows/server.yml` | Server CI (E10) |
| `src/port/netplay.c`, `include/port/netplay.h`, `src/port/inputsync.c` | Client lockstep, lobby state machine, toasts (E3, E6, E9) |
| `src/port/ui/online.cpp` | Lobby UX (E7): server/name, host, join by code, P1–P4 list |
| `src/port/ui/overlay.cpp`, `settings.cpp`, `graphics_tuner.cpp`, `res/rml/*.rcss` | Overlay look and tab content (D4, D5) |

---

## 5. Phase P0 and port fixes

### P0.1 Fork + integration branch (USER)
User: fork `girlyoulookthebest/smb2-pc` on GitHub (default name Q-2). Then the orchestrator runs:
`git remote rename origin upstream && git remote add origin <fork> && git checkout -b online && git push -u origin online`.
Until then, work locally on `online` without pushing.

### P0.2 WSL toolchain, decomp, SDL2
- **Goal**: Everything in README "Building from source" §1–§2 installed under the layout in §4.1.
- **Steps**: start WSL Ubuntu; `apt install` the packages (needs sudo: if a password prompt
  blocks, report BLOCKED and ask the user to run the apt line); download llvm-mingw 20241217 to
  `../toolchains`; clone decomp to `../decomp`, copy the ISO to `decomp/orig/GM2E8P/`, build it;
  build/unpack SDL2 2.30.9 into `smb2-pc/mingw_deps/` as in the README.
- **Acceptance**: `ninja` in decomp finishes OK; both `mingw_deps/*/lib/libSDL2.a` exist.
- Write exact commands that worked into `docs/handoff/P0.2.md`, and copy them to §4.3 afterwards.

### P0.3 Build wrapper + smoke test
- **Files**: new `tools/win_build.sh`.
- **Goal**: one command builds x64/i686/both with absolute paths for decomp, llvm-mingw and SDL deps
  (env vars with defaults from §4.1), works from a worktree, prints the exe path.
- **Acceptance**: `both` builds clean. Smoke run (x64):
  `smb2_pc_port.exe --iso <iso> --save-dir %TEMP%\smb2test --audio-null --exit-frame 600` exits 0
  and the log shows the game reaching its title/attract loop.

### P0.4 Headless test harness
- **Files**: new `tools/smoke/` scripts (bash or PowerShell), `docs/testing.md`.
- **Goal**: document and script how agents test without a human: `--input-script` token format
  (read `parse_input_script` in `input_sdl2.c`), `--exit-frame`, `--dump-interval` frame dumps,
  `--audio-null`, throwaway `--save-dir`. Provide a script that boots, presses START through to
  the main menu, and exits; and one that enters a party game (e.g. Monkey Race) with 2 pads.
- **Acceptance**: both scripts run unattended and exit 0; screenshots (frame dumps) prove the
  expected screen was reached.

### A1 Windowed by default
- **Files**: `src/platform/pc_settings.c` (`set_defaults`, first-start log line ~175),
  README "Settings"/feature text, `installer/README.txt` if it mentions it.
- **Goal**: first start (no `settings.dat`) opens a window sized by `fit_default_size()`,
  centred. Alt+Enter, `--fullscreen` and the settings still switch to borderless/fullscreen.
  Existing `settings.dat` is respected (Q-4).
- **Acceptance**: run with an empty `--save-dir`: log says windowed, `settings.dat` written with
  `display=windowed`; `--fullscreen` still gives borderless. Human check: REVIEW (window looks right).

### B1 Disc reader abstraction
- **Files**: `src/assets/iso.c`, `include/iso.h`, new `src/assets/disc_reader.{c,h}`, `src/assets/gamedata.c`.
- **Goal**: replace every raw `FILE*` seek/read on the image with
  `DiscReader { int64 size; bool (*read)(DiscReader*, uint64 off, uint32 len, void* dst); void (*close)(DiscReader*); }`.
  Backend 1: plain file (64-bit offsets via `_fseeki64`). `iso_open` picks the backend by magic.
  `gamedata.c` (~348, extraction and verification) uses the same reader.
- **Acceptance**: no behaviour change: smoke test passes; `--extract-iso` of the ISO gives the same
  `manifest.txt` as before the change (diff it).

### B2 RVZ decoder
- **Files**: new `src/assets/rvz.c`, `third_party/zstd/` (zstd 1.5.x `zstddeclib.c` amalgamation,
  BSD licence, add to `installer/LICENSE.txt` and README credits), CMakeLists.
- **Spec references**: Dolphin `docs/WiaAndRvz.md` and `Source/Core/DiscIO/WIABlob.cpp`
  (format: header 1, header 2, partition entries, raw data entries, group entries; RVZ group
  entries with `rvz_packed_size`; RVZ packing of junk data regenerated with Dolphin's lagged
  Fibonacci generator `LaggedFibonacciGenerator.cpp`, seeded per disc ID + offset).
  Read these from GitHub; don't copy GPL code verbatim (Dolphin is GPL-2.0+; this repo is CC0):
  implement from the format description.
- **Scope**: GameCube discs only (no Wii partitions/encryption). Compression: none, zstd.
  Others → clear error naming the compression type. Cache the last decompressed group
  (groups are typically 128 KiB–2 MiB) so sequential reads are fast.
- **Acceptance**: unit tool `tools/rvz_test` decodes the user's RVZ fully; see B4.

### B3 RVZ everywhere the user picks a disc
- **Files**: `src/platform/platform_sdl2.c` (filters/messages ~753, 814, 967, 1208),
  `src/main/main.c` (`--help`), `installer/smb2.nsi` (~199, 888, 961), `installer/README.txt`,
  `installer/LICENSE.txt`, README.
- **Goal**: file filters `*.iso;*.gcm;*.rvz`, texts mention `.rvz`, installer extraction from RVZ works
  (it already calls `--extract-iso`, which uses the reader after B1/B2).
- **Acceptance**: `--extract-iso <rvz> --gamedata <dir>` passes `--verify-gamedata <dir>`;
  portable run with `--iso <rvz>` reaches the title screen (smoke script).

### B4 RVZ verification
- Decode `Super Monkey Ball 2 (USA).rvz` to a temp file and compare it with the ISO:
  CRC32/SHA-1 of the whole image must match (or, if Dolphin scrubbed junk differently, every
  FST file's bytes must match; document which). Time a full decode; report MB/s.
  Delete the temp file afterwards (1.4 GB).

### C1 Steam Controller diagnosis (needs the user)

> **Known so far (2026-11-17):** the controller is the new (2026) Steam Controller, launched
> directly without Steam. Partyboard (SDL3 3.4.10) handles it, so SDL3's HIDAPI driver supports it
> and SDL2 2.30.9 most likely doesn't. C1 now only needs to: (1) confirm with `--list-controllers`
> what SDL2 2.30.9 sees; (2) check whether the latest SDL2 2.32.x supports this controller
> (SDL changelog / `src/joystick/hidapi/`); (3) if not, compare **sdl2-compat** (SDL2 API on
> SDL3, but it's a DLL, while this port links SDL statically) with **migrating the port to
> SDL3** (platform_sdl2.c, input_sdl2.c, file dialog, GL/D3D12 window glue; audio is miniaudio)
> and recommend one. Migrating to SDL3 is now a real option for C2 (split it into subtasks if chosen).

- **Read-only first**: SDL 2.30.9 on Windows does not open the original Steam Controller by
  default (`SDL_HINT_JOYSTICK_HIDAPI_STEAM` is off by default; without Steam running the
  controller is in "lizard mode" = keyboard/mouse). With Steam running, Steam Input normally
  provides a virtual XInput pad, but only for games launched through Steam (e.g. as a
  non-Steam game), and Steam hides the physical device. The 2026 Steam Controller may not be
  known to SDL 2.30.9 at all.
- **Steps**: add a temporary/permanent `--list-controllers` CLI flag (or log at start) that prints
  every SDL joystick: name, VID/PID, GUID, `SDL_IsGameController`, type, HIDAPI or not.
  Ask the user to run it in these cases: (a) Steam closed, (b) Steam open, game started directly,
  (c) game added to Steam as a non-Steam game and started from Steam. Record results in §8.
- **Output**: diagnosis + chosen fix list for C2 (answers Q-1).

### C2a–C2e (supersede "C2" below; full specs in `docs/handoff/C1.md` "FOLLOW-UPS" and §4–6)
- **C2a** `--list-controllers` (SDL2): files `src/main/main.c`, `src/input/input_sdl2.c`, `include/input.h`;
  prints the C1.md §6 output and exits 0 without a window. User runs it with the controller plugged in.
- **C2b** SDL3 static libs: new `tools/build_sdl3_mingw.sh` (CMake, `-DSDL_SHARED=OFF -DSDL_STATIC=ON`,
  toolchains `cmake/mingw32.cmake` and `cmake/llvm-mingw64.cmake`) into `mingw_deps/sdl3/{i686,x86_64}-w64-mingw32`.
  Acceptance: a hello program links statically with both toolchains and runs without SDL3.dll.
- **C2c** platform glue → SDL3: `platform_sdl2.c`, `pc_settings.c` (fullscreen/display modes: biggest part),
  `pc_file_dialog.c`, `render_opengl.c`, `render_d3d12.c`, `audio_miniaudio.c`, `assets.c` (don't free
  `SDL_GetBasePath` under SDL3), `main.c`, `perf.h`, `platform.h`, `video_shim.c`, `pc_quit.c`, CMakeLists.
  Keep file names (renaming `*_sdl2.c` is optional and done last). Acceptance: boots in GL and D3D12, windowed /
  borderless / fullscreen; `settings.dat` and save paths unchanged; P0.4 smoke scripts pass on both arches.
- **C2d** input → SDL3 gamepad API: `input_sdl2.c`, `include/input.h`, `button_prompts.*`, controller
  events in `platform_sdl2.c`. `SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS` is gone: re-verify the
  Nintendo label mapping. Acceptance: vpad scripts give identical traces for Xbox/PS4/PS5/Switch, hot-plug
  without double connect, and (REVIEW) the user's Steam Controller drives the game with Steam closed.
- **C2e** README build steps (SDL3), credits/licence, version pin.

### C2 Steam Controller fix (original notes; superseded by C2a–C2e)
Candidate fixes (choose from C1's results; likely several):
1. `SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1")` before the game-controller subsystem is
   initialised (note `platform_sdl2.c:113` inits it in `SDL_Init`, before `input_init`: the hint
   has to go before that call). Also consider `SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK`.
2. Update SDL2 to the latest 2.32.x (newer HIDAPI drivers and controller DB); update README build steps.
3. If the new (2026) controller is unsupported in SDL2: either add its mapping via the bundled
   `gamecontrollerdb.txt` or document "use Steam Input (add as non-Steam game)". Moving to SDL3 is
   a large change: only propose it, don't do it in this task.
4. Raw joysticks (no mapping) already have a path (`input_sdl2.c`, `p->joy`): make sure they get a
   sane default mapping so an unknown pad works at all.
5. When Steam Input is active, make sure the game doesn't also read keyboard/mouse lizard-mode
   events as port 1 input in a confusing way.
- **Acceptance**: the user confirms it works in the case(s) from C1 (REVIEW); other pads still work
  (virtual pad tests via the existing `vpad` test hooks).

### D1 Dear ImGui: SDL2 + OpenGL
- **Files**: `third_party/imgui/` (pinned v1.91.x: core + `imgui_impl_sdl2`, `imgui_impl_opengl3`,
  `imgui_impl_dx12`), `CMakeLists.txt` (enable CXX, `smb2_overlay` static lib), new
  `src/overlay/overlay.cpp`, `include/overlay.h` (C API: `overlay_init`, `overlay_process_event(SDL_Event*)`,
  `overlay_new_frame`, `overlay_render`, `overlay_is_open`, `overlay_toggle`, `overlay_wants_input`),
  `src/render/render_opengl.c` (draw after the game frame, before swap, at the window's native size).
- **Goal**: a demo window (ImGui demo is fine for now) opens with a key (temporarily F2) over the
  running game in the GL renderer, mouse and keyboard work, 32- and 64-bit builds both work
  (i686 GCC needs `-static-libstdc++ -static-libgcc` or equivalent; no new DLLs next to the exe).
- **Acceptance**: build both; frame dump or screenshot shows the window over the game; no GL state
  leak (game renders identically on the next frame with the overlay closed: compare dumps).

### D2 Dear ImGui: Direct3D 12
- **Files**: `src/render/render_d3d12.c` (+ a small C++ shim if needed since the DX12 backend is C++),
  `src/overlay/`.
- **Goal**: same overlay with `--renderer d3d12`. Use the renderer's command list and back-buffer RTV;
  a dedicated SRV heap for ImGui's font texture. Handle resize/device recreation.
- **Acceptance**: as D1 with `--renderer d3d12`; no D3D12 debug-layer errors if available.

### D3 Overlay input + gamepad navigation
- **Files**: `src/platform/platform_sdl2.c` (event loop), `src/input/input_sdl2.c`, `src/overlay/`.
- **Goal**:
  - Open/close: **F1** (replacing the old overlay in D5; until then F2), and a controller chord
    (default: Back/Select + Start held 0.5 s, or the Guide button), like partyboard's chorded F1.
  - While open: ImGui gets mouse/keyboard/gamepad (`ImGuiConfigFlags_NavEnableGamepad`); the game
    receives **neutral pads** for every local port, not the overlay's navigation input.
  - Offline: pause the game while open (match the old overlay). Online (`netplay_active()`, stub
    returning 0 until E6): never pause; still neutral local input.
  - Mouse cursor shown while open, hidden after 3 s idle otherwise.

### D4 Overlay tabs (partyboard look)
- **Files**: `src/overlay/*.cpp`, `src/platform/pc_settings.c` (only to expose getters/setters; reuse
  its apply functions, don't duplicate logic).
- **Layout**: tab bar; left = list of options, right = help text for the focused option (partyboard
  style, colours/fonts loosely matching `res/rml/overlay.rcss` and `window.rcss`).
- **Tabs** (each row wired to the existing `pc_settings` apply code, saved to `settings.dat`):
  - **Video**: display mode (windowed / borderless / fullscreen), resolution, aspect (original /
    stretched), frame rate (60/120/144/165/240/display/uncapped), vsync, MSAA, renderer (restart
    required note), "Restore default window size", show FPS (corner select).
  - **Audio**: volume.
  - **Input**: per-port controller list (name, type), button prompt style, stick settings from
    `pc_stick_menu.c` if they fit, "Open controller DB folder".
  - **Game**: language, disc image (path + change; restart required), memory card import/export
    (reuse `pc_memcard_ui.c` actions), updates.
  - **Online**: placeholder until E7.
  - **Interface**: open data folder, version, "Quit to desktop".
- **Acceptance**: every option changes the same thing as in Options > Game Screen and persists
  across restart (automate: change via a test hook or script, restart, read `settings.dat`).
  Human check: REVIEW with screenshots in §8.

### D5 Replace old F1 overlay
- **Files**: `src/game/shim/pc_settings_menu.c` (remove the overlay part only; keep the in-game
  Options > Game Screen menu), `src/game/shim/video_shim.c:352`, `platform_sdl2.c` F1 handler,
  README controls section.
- **Goal**: F1 opens the ImGui overlay. Add the FPS counter corner and a toast API
  (`overlay_toast(type, title, body)`, used later by netplay and controller hot-plug messages).
- **Acceptance**: F1 and the chord open the new overlay in both renderers; the old modal overlay is
  gone; controller connect/disconnect shows a toast.

---

## 6. Phase E: online multiplayer

### Design summary (read before any E task)

- Every PC runs the whole game. Each frame, each PC sends its local pad state for frame
  `F + delay` to the relay server; the server bundles all players' pads for frame `F` and
  broadcasts; each PC blocks in its pad read until the bundle for `F` arrives (lockstep).
  Remote ports appear to the game as normally connected controllers.
- Pad wire format per port per frame: buttons u16, stickX/Y s8, substickX/Y s8, triggerL/R u8,
  err s8 = 9 bytes (from GC `PADStatus`).
- Input delay: default 2 frames on LAN; the lobby picks `ceil(maxRTT/2 / 16.67 ms) + 1`, host can override.
- Checksums: every 60 frames each client sends a hash of game state (E3); a mismatch = desync.
- Start: all clients relaunch/enter a fresh deterministic boot (D-6) with the same session
  settings, then lockstep runs from frame 0 (title screen); players navigate the game's own menus
  together (port 1 = host). All party games that are unlocked by play points must be available:
  the session card has them unlocked (E2 finds out how).
- Ports: up to 4; each person owns one port; the overlay lobby lets people pick ports.
- Rendering stays local (any fps/resolution/renderer); only the 60 Hz simulation must match.

### E1 Determinism audit (read-only, no code changes)
- **Output**: `docs/handoff/E1.md` with a table like partyboard's NETPLAY.md "Source of drift | Fix | Where".
- **Check at least**:
  1. Every wall-clock/time source reachable from game code: `OSGetTime`, `OSGetTick`, `OSTicksToCalendarTime`,
     card timestamps, anything seeding RNG (`srand` callers, the game's own RNG(s) besides MSL `rand`).
  2. Disc reads: are DVD reads (`dvd_shim.c`) synchronous on the game thread, or can completion
     timing vary? Async callbacks? (Partyboard hit this.)
  3. Frame pacing: is the simulation tick exactly one per `PADRead`/VI retrace? What happens when
     a frame takes > 16.7 ms: does the game run catch-up ticks or the port skip? (`video_shim.c`,
     `gx_hfr.c`). Lockstep needs "one sim tick per input frame", independent of render fps.
  4. Audio: anything the game observes from the synth (stream status, voice counts) that could
     depend on the DSP worker thread or the output device.
  5. Memory card: presence, contents (unlocks, options like game settings stored in the save),
     and save timing.
  6. PC settings that change simulation: language (dialog timings via `atof` scripts), widescreen
     (`SMB2_PC_WIDE`: camera/aspect used in game logic?), stick emulation settings (`gc_stick.c`
     runs on local input before it goes on the wire? then it's fine), button style, DIP switches.
  7. Pause on focus loss, soft reset, the update check UI on the title screen, the first-run
     disc dialog: anything that changes the frame sequence.
  8. Uninitialised memory / heap layout: does the arena start zeroed and identical? Pointer values
     used as data (hashes, sort keys)? x64 vs i686 differences (cross-arch play must work since
     detmath promises bit-identical results, but pointer-size structs may differ: docs/x64.md).
  9. Where to hook pad input for lockstep: the exact function where the game takes a pad snapshot per
     tick (`input_poll_gc_pads`, the `0x80009A10`/`0x80145210` paths in `input_sdl2.c`).
  10. What to hash for checksums: arena (24 MiB, cost?), plus game `.data/.bss`; or a curated set
      (balls[], players, RNG state, mode/state vars). Measure cost.
  11. Which modes to support online: Main Game Competition (2–4), party games that are decompiled
      (Race, Target, Bowling, Boat, Dogfight, Baseball, Golf, …): list which have multiplayer and
      any per-mode concerns.
- **Acceptance**: every item answered with file:line evidence, and a proposed fix for each source of drift.

### E1 results (required reading for E2–E9; full detail in `docs/handoff/E1.md`)
- Sim is single-threaded, one tick per `video_finish_frame` (`main_loop.c:260-312`); the pacer never
  catches up (`video_shim.c:403-409`); rendering never feeds back. DVD reads are synchronous.
- Wall-clock seeds: `srand(OSGetTime())` (`main_loop.c:252`), boat RNG from `OSGetTick()/4`
  (`boat_0.c:241,300`, `boat_5.c:325`), `rend_efc.c:618` → frame-derived clock in det mode.
- Session card: in-memory and empty; set `unlock_info.partyGames = 0x1B600` (all party games) after
  `clear_unlock_info()`; the title's create-file branch (`adv.c:1747-1754`) builds the save.
- Modal shim screens (F1 overlay, stick screen, updater, memcard UI) poll pads themselves → disable online.
- The hang watchdog kills the process after 8 s without a frame (`stubs_manual.c:139-160`), so the
  lockstep wait must bump `g_shim_frame_pulse` and pump SDL events.
- **Cross-arch: `Ball` is 0x1B0 (i686) vs 0x1B8 (x64)**, so checksums must be a curated field-wise hash
  computed inside a game TU, not raw memory. Allow cross-arch play only after runtime test T6 passes.
- Pad hook: inside `input_poll_gc_pads`, after `game_process_sticks` (~L907) and before the
  pressed/released/repeat derivation (~L945), only for the call from `video_finish_frame`
  (`video_shim.c:337`). Sample local devices into a private buffer; send post-stick-gate values.
  Baseball calls `video_finish_frame` 2 extra times without a tick → count polls, not ticks.
- Online-playable modes: Competition, Race, Target, Bowling, Boat, Dogfight, Baseball, Golf. Billiards,
  Shot, Fight, Soccer and Tennis aren't linked → hide or lock them online.
- Handshake "must match" config: build id, disc manifest hash (`fst_crc32`), locale, session epoch.
- Runtime tests T1–T9 (E1.md) belong in E4; T2 (render independence) and T6 (cross-arch) matter most.

### E2 Deterministic session mode
- **Files**: per E1 (expect `os_shim.c`, `card_shim.c`, `dvd_shim.c`, `video_shim.c`, `main.c`, new `src/netplay/session.c`).
- **Goal**: `--det-session` (also turned on by netplay later) makes a run fully reproducible:
  fixed clock (time derived from frame number + a session epoch), session memory card (in memory,
  everything unlocked, never written to disk), no focus pause, no update check/dialogs, forced
  sim-affecting settings (from E1) taken from a session config struct.
- **Acceptance**: implemented fixes for every E1 item; normal play without the flag is unchanged
  (smoke test + a quick diff of behaviour in the log).

### E3 Input record/replay + checksums
- **Files**: new `src/netplay/inputsync.c` (like partyboard's: pad source = live / record / replay /
  netplay), hook at the point E1 found, `src/netplay/checksum.c`, `main.c` flags
  `--record <file>`, `--replay <file>`, `--checksum-log <file>`, `--max-frames <n>`.
- **Goal**: record all 4 ports' pads per tick to a file; replay drives the game from the file
  instead of live input; checksum (from E1 #10) written every 60 frames to the log.
- **Acceptance**: record a 3-minute session (input script with 2 pads in Monkey Race and one in
  a Main Game stage) and replay it: checksum logs identical.

### E4 Local determinism tests
- **Files**: `tools/determinism/` (script running N replays in parallel, comparing checksum logs;
  partyboard's `tools/autotest.py` is the model).
- **Acceptance**: same replay, 3 runs each on x64 (i686 dropped, D-9), at `--fps 60` and `--fps 144`, GL and
  D3D12: all checksum logs identical. Any mismatch → new E2 fix, re-run.

### E5 Protocol + relay server
- **Files**: `include/netplay/protocol.h`, `server/` (main.cpp, test_client.cpp, CMakeLists.txt,
  README.md), all adapted from partyboard (§4.4).
- **Keep**: ENet/UDP, rooms with 4-letter codes, HELLO with protocol version + build ID, host/join,
  4 slots, START_GAME with session config, FRAME bundles, checksum compare → DESYNC, PERF/NET_STATS,
  late-player detection.
- **Drop/change**: Mario Party characters/boards/turns/save points/reload. Session config instead:
  input delay, sim-affecting settings from E2, game build ID. Port `PB_` names to `SMB_`.
  Default port 7855.
- **Acceptance**: server builds natively on Linux (WSL) and in Docker; `test_client` (two fake
  players, create/join/start/frames/checksum match and mismatch) passes.

### E6 Netplay client (lockstep)
- **Files**: `src/netplay/netplay.c` (+ `include/netplay.h`), ENet vendored for the mingw build
  (link `ws2_32 winmm`), hook in `inputsync.c`, `main.c` flags `--server host:port --create --join CODE --name N --delay N`.
- **Goal**: lockstep as in the design summary; pumping the network while blocked (keep the window
  responsive; partyboard runs frames from an SDL event watch during window drag); timeouts with
  a "waiting for <name>" message; checksums sent every 60 frames.
- **Acceptance**: two local instances against a local server, scripted input on both
  (`--input-script`), `--max-frames 3600`: both checksum logs identical and equal to a `--replay`
  of the recorded inputs. (x64 + i686 mix dropped, D-9.)

### E7 Lobby UI (overlay "Online" tab)
- **Files**: `src/overlay/online.cpp`, title-screen hint ("F1 / Back+Start: Online").
- **Goal**: partyboard-style: server address (remembered) + name; Host (input delay, create room,
  shows code, start) / Join by code; live P1–P4 list with ping; port selection; Leave. Status
  line and toasts for joins/leaves/desync.
- **Acceptance**: drive two instances via CLI-equivalent test hooks; human REVIEW for the UI.

### E8 Session start flow
- **Goal**: on START_GAME every client switches to deterministic session mode (E2) with the
  room's session config and boots fresh (in-process reset of the game if E1 shows it's safe,
  otherwise relaunch the exe with session args like partyboard's `relaunch`). Lockstep from frame 0.
  On session end (host quits / all leave) → back to normal offline mode (relaunch if needed).
- **Acceptance**: from the lobby, 2 local instances start, play to the main menu and into Monkey
  Race via scripted input, matching checksums.

### E9 Disconnects and desyncs
- **Goal**: a dropped player's port gets a neutral pad (or `err = -1` if the game handles a pad
  vanishing mid-game: E1 #11 says which per mode) on the same frame everywhere (server-chosen
  frame, like partyboard's human mask). Session continues while ≥1 human remains.
  Desync: show a toast + message, end the session cleanly, return everyone to the lobby with the
  room kept. (Mid-session rejoin/state transfer is out of scope unless E12 makes snapshots cheap.)
- **Acceptance**: 3 local instances, kill one at frame 1500 (`SMB_NETPLAY_TEST_DROP`), others keep
  matching checksums to 3600; forced desync (`SMB_NETPLAY_TEST_DESYNC`) ends cleanly on all.

### E10 Server deployment
- **Files**: `server/Dockerfile`, `server/portainer-stack.yml` (node label `smb2=true` or reuse
  partyboard's node, UDP 7855 host mode, replicas 1), `.github/workflows/server.yml`
  (build, run test_client, push `ghcr.io/<owner>/smb2-server` on the `online` branch).
- **Acceptance**: `docker build` + `docker run` + `test_client` against it pass in WSL. Pushing
  CI and deploying to the Swarm is a USER step (needs P0.1 and Portainer access).

### E11 Two-PC test (USER)
- LAN first, then internet (UDP 7855 forwarded). Record ping, chosen delay, any desync, how it
  feels (input lag) per mode. Results in §8. If input lag is unacceptable over the internet,
  prioritise E12.

### E12 (Stretch) Rollback feasibility spike (read-mostly)
- Measure: size of all mutable game state (arena + game .data/.bss + shim state: audio synth,
  GX/render state the game reads back, file/DVD state); cost of snapshot/restore per frame;
  Windows write-watch (`VirtualAlloc(MEM_WRITE_WATCH)` + `GetWriteWatch`) to copy only dirty pages;
  how to silence/dedupe audio during re-simulation; render interpolation implications.
- **Output**: go/no-go report with a design if go. Snapshots would also enable mid-game join and
  desync recovery (state transfer).

### E13 (Stretch) Own full-screen view online
- In split-screen modes (Race, Competition), optionally render only the local player's viewport
  full-size. Rendering only; simulation unchanged (must not alter checksums: run E4 again).

---

## 6b. Phase F: distribution and build identity (requested by the user 2026-11-17)

### F1 Friend build (self-extracting single exe)
- **Model**: partyboard `platforms/windows/launcher/launcher.cpp` + `launcher.rc.in` + `tools/package_friends.py`
  (read-only reference). Small launcher exe with the whole build (exe + `assets/` + anything else in `build64/bin`,
  no disc image, no pdb) appended as an LZMS archive (Windows Compression API, `compressapi.h`, `-lcabinet`;
  available in MinGW). On start: unpack once to `%LOCALAPPDATA%\SMB2\builds\<build id>\`, run the game from there
  with the same command line, delete older unpacked builds that aren't running.
- **Saves must survive updates**: the unpacked folder is per build, so the launcher passes
  `--save-dir %APPDATA%\SMB2\friend\` (settings.dat, remembered disc path and the memory card live there) unless the
  user passes their own `--save-dir`.
- **Shift held at start, or `--uninstall`**: confirmation dialog, then delete `%LOCALAPPDATA%\SMB2\builds\`,
  `%APPDATA%\SMB2\friend\` and other dirs the game creates for this mode; never touch the installer's
  `%APPDATA%\SMB2\installed\` or the portable `%APPDATA%\SMB2\smb2_pc_port\` (say so in the dialog).
- Cross-compiled in WSL like the game (x64 llvm-mingw; C++ is fine for the launcher). Packaging script
  `tools/package_friends.sh` (bash; Python only exists in WSL) → `build/package/friends/SMB2.exe` + `README.txt`.
- Build id: until F2 exists, a hash of the archive; designed so F2's build id can replace it.

### F2 Build number
- A build number generated at configure/build time: `<git commit count on HEAD>-<short sha>[+dirty]`
  (fallback when there's no git: timestamp). Exposed as `SMB2_BUILD_ID` (generated header, rebuilt when HEAD
  changes; don't force a full rebuild of the game on every build).
- Shown in: window title (`Super Monkey Ball 2 – build 123 (abc1234)`), first log line, the overlay (D5,
  Interface tab), `--version`. Used as SessionConfig.buildId / HELLO build id (E6) so mismatched builds get a
  clear "different build" error from the server instead of desyncing. F1's launcher uses the same id.

## 7. Definition of done (whole project)

- New installs start windowed; overlay (F1 / controller chord) controls all graphics settings in
  both renderers.
- `.rvz` images work in the portable build, the installer and `--extract-iso`.
- The user's Steam Controller works in the configuration(s) agreed in C1.
- Two PCs can host/join a room through the deployed server and play Main Game Competition and the
  supported party games with matching checksums; disconnects and desyncs are handled cleanly.
- README and docs (`docs/netplay.md` written from this plan + handoffs, `docs/testing.md`) are updated.

---

## 8. Session log (append only; newest at the bottom)

### 2026-11-17: plan created
- Surveyed smb2-pc (`main`, c59b08f, v1.1.8, clean tree) and partyboard (`netplay`, protocol 8).
- Findings that shaped the plan: RVZ is detected and rejected in `iso.c` (~152); fullscreen default
  is `PCS_BORDERLESS` in `pc_settings.c` `set_defaults`; no SDL HIDAPI/Steam hints are set; an F1
  overlay exists but is a modal game-font menu (`pc_settings_menu.c`); determinism groundwork
  (detmath, game-thread synth) already exists; heap is one 24 MiB arena at 0x80000000.
- No decomp checkout and no toolchain yet (P0.2). WSL `Ubuntu` exists (was stopped).
- Next: user does P0.1 (or says to stay local) and answers Q-1; orchestrator starts P0.2.

### 2026-11-17: start
- User answered Q-1 (new Steam Controller, launched directly, works in partyboard), Q-2, Q-3; sudo is run by the user.
- P0.1 DONE: fork `Jacob-Tate/smb2-pc` created; `origin` = fork, `upstream` = girlyoulookthebest; branch `online`.
- Decomp cloned by the user to `super_monkey/smb2-decomp` (a269b10), not `../decomp`.
- P0.2 BLOCKED: `wsl -d Ubuntu` fails with `ERROR_SHARING_VIOLATION` on
  `%LOCALAPPDATA%\wsl\{a221e025-...}\ext4.vhdx` (another process holds the disk). Asked the user to fix
  (Docker Desktop / backup / antivirus / Disk Management attach; or reboot).
- While blocked: started E1 (determinism audit) and the research part of C1 as read-only Sonnet subagents.
- WSL fixed by the user. User installed `mingw-w64 nsis`. SDL2 2.30.9 built/unpacked into
  `smb2-pc/mingw_deps/{i686,x86_64}-w64-mingw32` (i686 built in `~/sdl2build32` in the WSL fs).
  Decomp copy of the disc: `smb2-decomp/orig/GM2E8P/game.rvz`. llvm-mingw → `super_monkey/toolchains/`.
- C1 DONE → decision D-8: migrate to SDL3 (C2a–C2e); D1 now depends on C2c.
- P0.2 DONE (decomp built, 92.6% of game code matched), P0.3 DONE (`tools/win_build.sh`, smoke run OK).
- E1 DONE: lockstep is feasible; summary in §6 "E1 results".
- Launched in parallel (Sonnet, worktrees): C2b, P0.4, A1, B1, E5. The Agent tool's worktree isolation
  failed, so the worktrees were made by hand under `super_monkey/wt/<ID>` (see §3.1).
- A1 merged (81bb9a9): `set_defaults()` → `PCS_WINDOWED`; verified fresh `--save-dir` → windowed 1600x900,
  `display=windowed`. Note: `--exit-frame`/`--input-script` force windowed unless `--fullscreen` is given.
  **User check (REVIEW):** delete or move `settings.dat`, start the game → centred window with a title bar;
  Alt+Enter toggles borderless; Options > Game Screen still switches modes.
- B1 merged (0be1727): `src/assets/disc_reader.{c,h}`; extraction manifest identical to before, verify OK.
  The remembered disc path lives in `settings.dat` inside `--save-dir`, so test runs don't touch the user's.
- B2 started (task/B2, also covers B4 verification).
- C2b merged (49c8af2): SDL3 3.4.10 static libs in `mingw_deps/sdl3/<triplet>` (git-ignored; rebuild with
  `tools/build_sdl3_mingw.sh`). Link libs = same set as SDL2 (see docs/handoff/C2b.md); `SDL_MAIN_HANDLED`, no SDL3main.
  `super_monkey/sdl3_gamepad_check.exe` (x64 hello) prints how many gamepads SDL3 sees: user can run it
  with the Steam Controller plugged in. C2a skipped (the `--list-controllers` flag goes into C2d).
- C2c waits for P0.4 (its acceptance uses the smoke scripts).
- **User ran `sdl3_gamepad_check.exe` with the new Steam Controller, Steam closed: "SDL3 version 3.4.10,
  gamepads: 1"** → SDL3 sees it; the C2c/C2d migration will fix C2.
- E5 merged (1f9afa1): `include/netplay/protocol.h` (SMB_ v1, port 7855, 58-byte SessionConfig), `server/`
  (ENet 1.3.18), `tools/server_test.sh` (77 checks pass; re-run by orchestrator). Notes for E6: drop FRAMEs
  whose `gameSeq` isn't the current game's; host-only END_GAME; `humanMask` authoritative per poll; no
  mid-game join (ERR_IN_GAME). Message table in docs/handoff/E5.md.
- No Docker on this PC (Windows or WSL): E10's image is verified by GitHub Actions after pushing `online`.
- Started E10 (task/E10) and E2 (task/E2). E2 and C2c both touch main.c / video_shim.c / pc_quit.c:
  whichever merges second rebases.
- P0.4 merged (bc95b0b): `docs/testing.md`, `tools/smoke/boot_to_menu.sh` (mode 1/32) and
  `tools/smoke/party_race_2p.sh` (mode 4/186, pad on port 2 via new `VADDX2` script tokens); both PASS on
  the orchestrator's re-run. Limits: one virtual pad (3–4 scripted players need an array); fixed frame timings.
- E10 merged (ce09eff): Dockerfile, `.dockerignore`, `server/portainer-stack.yml` (node label `partyboard=true`,
  UDP 7855 host mode), `.github/workflows/server.yml`. **CI run 44203768447 green**: image built, integration
  test against the container passed, published to `ghcr.io/jacob-tate/smb2-server` (latest + sha).
  **USER steps** (docs/handoff/E10.md): make the GitHub package `smb2-server` public; Portainer > Stacks >
  Add stack `smb2` with `server/portainer-stack.yml`; open UDP 7855; check with
  `docker exec $(docker ps -qf name=smb2_relay) smb2-server-test 127.0.0.1 7855`.
- C2c started as one task covering C2c + C2d (task/C2c): one exe can't link SDL2 and SDL3 at once.
- B2 merged (7e17185): `src/assets/rvz.{c,h}`, zstd 1.5.7 decoder in `third_party/zstd/`, hidden
  `--rvz-dump <in> <out.iso>`. **Decoded RVZ is byte-identical to the ISO** (SHA-256 b3e40531…, re-checked by
  the orchestrator), ~500–690 MB/s. Boots and `--extract-iso` from the RVZ work on both arches. B4 is done
  inside B2. Note: corruption inside a group is only caught if zstd notices it → optional B5.
  B3 waits for C2c (picker strings live in platform_sdl2.c).
- **Server deployed by the user at 10.0.0.5:7855** (Portainer stack `smb2`). Orchestrator ran
  `build/server/smb2-server-test 10.0.0.5 7855` from WSL: 77 checks, ALL PASSED. Use `10.0.0.5:7855` as the
  default/test server address in E6/E7.
- E2 merged (f688666): `--det-session` (+ `--session-epoch`, `--session-locale`), `src/netplay/session.{c,h}`,
  `src/game/shim/pc_session_shim.c` (`[detlog]` fingerprint every 60 polls). Verified by the orchestrator: same
  input script at 60 fps/GL and 144 fps/D3D12 → identical detlogs. Limitations (docs/handoff/E2.md): the pause
  menu's "Quit to Desktop" row can't be removed port-side (decomp sets it) → E9 treats choosing it as a
  disconnect; pad `err` still follows local controllers (E3); watchdog bump in the lockstep wait is E6.
- **C2c+C2d merged (c01ae12): the port is on SDL3 3.4.10 (static).** Conflicts with E2 in CMakeLists/main.c
  resolved (keep both). Merged build: both arches build; boot_to_menu (x64) and party_race_2p (i686) PASS;
  det-session detlog still identical across fps/renderer AND identical to the pre-SDL3 detlog (rand f4c20337 at
  poll 1740), so SDL3 didn't change the simulation. New `--list-controllers`. Behaviour changes forced by SDL3
  (docs/handoff/C2c.md): controllers.ini GUIDs may differ from SDL2's (stick settings re-adjust once); Luna/
  Stadia/Shield pads show GC prompts; 59.94 Hz rounds to 60.
  **User REVIEW**: Steam Controller in game with Steam closed (`--list-controllers` should list VID 0x28DE),
  keyboard-only menus, hot-plug, Switch Pro face buttons, Alt+Enter, F1 overlay, file dialogs (incl. from
  exclusive fullscreen), window drag/resize, first-run disc picker, high-DPI default window size.
- **F1 merged (2f493e9)**: `platforms/windows/launcher/` + `tools/package_friends.sh` → `build/package/friends/SMB2.exe`
  (7.9 MB, zstd-compressed, not LZMS) + README.txt. Unpacks to `%LOCALAPPDATA%\SMB2\builds\<id>`, saves in
  `%APPDATA%\SMB2\friend\`. Game change: in portable mode `--save-dir` now also moves `memcard\` and `logs\`.
  Orchestrator re-packaged from merged online (SDL3) and ran it with redirected AppData: unpack + RVZ boot OK.
  Test env: `SMB2_LAUNCHER_ASSUME_YES=1`; launcher honours LOCALAPPDATA/APPDATA env vars.
  **User REVIEW**: hold Shift while starting SMB2.exe → confirmation dialog lists the two folders; No cancels.
- Smoke scripts now accept relative exe paths (tools/smoke/common.sh).
- Next wave (parallel): B3+F2 (one agent; both small, both edit platform_sdl2.c), D1 (ImGui, SDL3+GL), E3.
- **B3+F2 merged (c5b51f6)**: `.rvz` in every picker/filter/text, installer and READMEs. Build id
  `<commit count>-<sha7>[+dirty]` from `cmake/build_id.cmake` (only `src/platform/build_id.c` recompiles):
  window title "Super Monkey Ball 2 – build N (sha)", first log line, `--version`, old F1 overlay text,
  SessionConfig.build_id, friend-build unpack folder. Orchestrator fix 372d700: WSL git saw the CRLF Windows
  checkout as dirty → now `git -c core.autocrlf=true diff --quiet HEAD`. Notes: `installer/build.sh` assumes
  `../decomp` → pass `SMB2_DECOMP_DIR=/mnt/c/Users/jacob/source/super_monkey/smb2-decomp`; in sibling worktrees
  the build id needs `git.exe` reachable from WSL (it is).
- **D1 merged (227973c)**: Dear ImGui 1.91.9b in `third_party/imgui/` (sdl3 + opengl3 compiled, dx12 copied),
  `smb2_overlay` lib, `include/overlay.h` / `src/overlay/overlay.cpp`, placeholder window on **F2** (temporary),
  test flags `--overlay-open`, `--overlay-open-frames a-b`. GL state verified unchanged after closing. Merged
  build: both arches build, party_race_2p PASS on both, overlay starts. Test caveat: attract/title frames
  ~700–800 differ in pixels between runs even with --det-session (detlog/sim matches) → use menu frames
  after the START presses (1560+) for pixel comparisons; E4 should check whether that's render-only.
- Started D2 (D3D12 overlay). D3 waits for E3 (input_sdl2.c).

### 2026-11-17 (late): context handoff
- Session ending. Still running at handoff: **D2** (worktree `../wt/D2`, branch task/D2) and **E3** (`../wt/E3`,
  task/E3). Their agents may have been stopped with the session: check `git -C ../wt/<ID> log --oneline online..`
  and `docs/handoff/<ID>.md` in each worktree, then resume (new Sonnet agent with the same prompt shape, §3.2,
  "continue from docs/handoff/<ID>.md") or verify + merge if the handoff says DONE.
- Next after those: D3 (after E3), D4, D5, E4, E6, then E7/E8/E9.
- Open user REVIEWs: Steam Controller in game (Steam closed), friend build Shift-start dialog, keyboard-only
  menus, file dialogs, Alt+Enter, hot-plug (see C2c and F1 entries above).

### 2026-11-17 (resume): D2 + E3 restarted
- Found both agents stopped mid-task, no handoff files. D2: commit 97e41fe (DX12 bridge, renderer hooks) + uncommitted
  `overlay.cpp`/`overlay_d3d12.cpp` (log text, SRV heap bind before RenderDrawData). E3: commit 4c62ec8 (inputsync,
  checksum, state hash, CLI flags) + untracked `tools/smoke/record_replay.sh`. Unverified. New Sonnet agents resume both
  in the same worktrees.
- User (D-9): x64 Windows only; no i686 builds, no cross-arch compatibility. D2/E3 agents told mid-run.
- **D2 merged (c2733e4)**: `src/overlay/overlay_d3d12.{cpp,h}` (own 8-slot SRV heap), overlay drawn last in `dx_present`
  then `apply_frame_state()` rebinds the game's state; frame dumps include the overlay. Vendored ImGui dx12 backend patched
  to load D3DCompile/CreateDXGIFactory1 at run time (no link-time d3dcompiler/dxgi import). Test hook
  `SMB2_TEST_RESIZE_AT=<frame>` (resizes + fullscreen toggles). Orchestrator: x64 build OK, D3D12 `--overlay-open-frames`
  dump at 1560 shows the window over Mode Select, boot_to_menu PASS. Pre-existing debug-layer warning (timestamp query
  on frame 1) left for later. `overlay_d3d12_invalidate()` unused (renderer never recreates the device).
- **E3 merged (3e0e393)**: `src/netplay/inputsync.{c,h}` (live/record/replay/netplay sources), `checksum.{c,h}`,
  `src/game/shim/pc_state_hash.c` (curated hash, ~4 us), flags `--record/--replay/--checksum-log/--max-frames/--checksum-ring`.
  `--record` and `--replay` imply `--det-session`; the record header carries epoch/locale/flags (replay ignores
  `--session-*`). Orchestrator re-ran `tools/smoke/record_replay.sh` on merged x64: race (75 lines) + Main Game stage
  (125 lines) replays identical; tampered input and other epoch detected. boot_to_menu + party_race_2p PASS.
  **Gotcha:** the user's Steam Controller (0x28DE/0x1304) injects stray input into scripted runs (party_race_2p failed
  with it on port 1 C-stick). `tools/smoke/common.sh` now sets `SDL_*_IGNORE_DEVICES` to it by default
  (`SMB2_TEST_IGNORE_PAD=` to disable); any other test harness (E4, E6) must do the same. This may also matter for the
  C2c Steam Controller REVIEW (drift / phantom C-stick?).
  Scripted path to a Main Game stage: docs/handoff/E3.md. New bug X1 (divide-by-zero in Mode Select).
- Next wave started (Sonnet, worktrees `../wt/<ID>`): D3, E4, E6, X1. D3 and E6 may both touch platform_sdl2.c: second to merge rebases.
- **D3 merged (a214292)**: F2 / Guide / Back+Start (0.5 s) toggle the ImGui overlay; ImGui gamepad nav; local pads
  neutral while open (gate at the E3 hook, before inputsync, so record/netplay see neutral pads; replay unaffected);
  offline pause in `src/game/shim/pc_overlay_pause.c` (hook in video_finish_frame); `--overlay-test-online`. Orchestrator:
  D3D12 pause/resume check OK, all 3 smoke scripts PASS. Notes: F2's own opening frame can still leak one keyboard
  frame (same as old F1); D5 must decide old-F1 vs new overlay interplay; D4 needs a gamepad-reachable Close.
  **User REVIEW**: open with Back+Start / Guide, navigate with the pad, close; mouse cursor hides after 3 s.
- **X1 merged**: Story Mode's menu screen 0x0C has 0 entries; `fn_3_32E0` does `cursor % 0` on LEFT/RIGHT. Harmless on
  PPC (divw doesn't trap), 0xC0000094 on x86. Fix: x64 vectored exception handler in stubs_manual.c emulates PPC
  (remainder = dividend, quotient 0/-1), logs first 8 hits as `[ppc-div0]`. Optional: decomp-side guard.
- **E6 merged (d42e44d incl. fixup)**: `src/netplay/netplay.{c,h}` ENet lockstep client, ENet 1.3.18 in `third_party/enet/`,
  flags `--server --create --join --name --delay` (+ test `--players --room-code-file --lobby-timeout`), blocking CLI lobby,
  `--record` captures the exchanged bundle. Fixup: D3's stub file removed; `netplay_test_force_active` now lives in netplay.c.
  Tests: `tools/netplay/two_instances.sh` (local Windows server via `tools/netplay/build_server_win.sh`, or `NP_SERVER=`).
  Orchestrator on merged build: `NP_SERVER=10.0.0.5:7855 NP_FRAMES=3600` PASS (60 identical checksums, identical
  recordings, replay matches). Hooks left for later: disc hash in HELLO still 0 (E8 → fst_crc32); session end = `exit()`
  codes 0/3/4 (E9 → back to lobby); err=-1 for non-human ports (E9 per mode); NET_STATS ignored (E7); CLI lobby in main()
  (E7/E8 need an in-game lobby). Untested (human): window drag during a session, "waiting for <name>" stall path.
