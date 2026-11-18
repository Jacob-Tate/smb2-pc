# C2c + C2d handoff: port migrated from SDL2 to SDL3 3.4.10 (static) -- DONE (REVIEW items for the user below)

Branch task/C2c (worktree wt/C2c). Update this file after every step.

## Status checklist
- [x] CMakeLists.txt: SDL3 from mingw_deps/sdl3/<triplet> (static libSDL3.a), clear FATAL_ERROR when
      tools/build_sdl3_mingw.sh was not run; dropped SDL2main + `main=SDL_main` define (SDL_MAIN_HANDLED stays).
- [x] platform_sdl2.c, pc_settings.c, pc_file_dialog.c, render_opengl.c, render_d3d12.c, audio_miniaudio.c,
      assets.c, main.c, button_prompts.c/.h, input.h, input_sdl2.c: ported, x64 builds.

## Conventions / gotchas
- Source files in the worktree are CRLF (autocrlf=true, index is LF). I converted the edited files to LF
  (`sed -i 's/\r$//'`) so perl/sed edits work; git normalises on commit, no content impact.
- No python on Windows: edits were done with perl scripts (scratchpad) and sed.
- Build: MSYS_NO_PATHCONV=1 wsl -d Ubuntu -- bash /mnt/c/Users/jacob/source/super_monkey/wt/C2c/tools/win_build.sh both --clean | tr -d '\0'

## API mappings used
- Init: SDL_INIT_GAMECONTROLLER -> SDL_INIT_GAMEPAD; SDL_INIT_TIMER/AUDIO dropped (unused; audio is miniaudio).
  SDL_Init/InitSubSystem return bool (success = true).
- Window: SDL_CreateWindowWithProperties (title, centred x/y, w/h, flags). No HIGH_PIXEL_DENSITY so window
  sizes stay in pixels like SDL2 on Windows. FULLSCREEN_DESKTOP -> SDL_WINDOW_FULLSCREEN with no fullscreen mode.
- Fullscreen modes (pc_settings.c pcs_apply_video): windowed = SetWindowFullscreen(false)+SyncWindow;
  exclusive = SDL_GetClosestFullscreenDisplayMode -> SetWindowFullscreenMode(&mode) + SetWindowFullscreen(true);
  borderless = SetWindowFullscreenMode(NULL) + SetWindowFullscreen(true). SDL_SyncWindow after each.
- Displays: SDL_DisplayID; SDL_GetFullscreenDisplayModes (free the array), SDL_GetDesktopDisplayMode (const*),
  SDL_GetDisplayContentScale replaces DPI/96 for the default window size.
- Events: SDL_EVENT_*; window resize = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED; key events use event.key.key/.mod
  (SDL_KMOD_ALT). Pad events carry joystick instance IDs (input_on_device_added/removed take unsigned id).
- HWND: SDL_GetPointerProperty(SDL_GetWindowProperties(w), SDL_PROP_WINDOW_WIN32_HWND_POINTER) in platform_sdl2.c
  (icon), pc_file_dialog.c, render_d3d12.c.
- GL: SDL_GL_GetProcAddress now returns SDL_FunctionPointer: wrapped by gl_getproc() in render_opengl.c;
  SDL_GL_GetDrawableSize -> SDL_GetWindowSizeInPixels; SDL_GL_DeleteContext -> SDL_GL_DestroyContext.
- assets.c: SDL_GetBasePath result no longer freed. SDL_GetPrefPath still SDL_free'd (same dir: %APPDATA%\smb2\smb2_pc_port).
- audio_miniaudio.c: SDL_sem -> SDL_Semaphore, SDL_SemPost -> SDL_SignalSemaphore, SDL_SemWaitTimeout -> SDL_WaitSemaphoreTimeout.
- Icon: SDL_CreateSurfaceFrom(w,h,fmt,pixels,pitch), SDL_DestroySurface.
- Input: SDL_Gamepad API (OpenGamepad/GetGamepadButton/Axis/Type/Name/Joystick), SDL_GetJoysticks id list (SDL_free),
  SDL_GUIDToString, SDL_UpdateGamepads, SDL_GetKeyboardState -> const bool*, SDLK_ letters are upper-case (SDLK_Z).
  Virtual pads: SDL_AttachVirtualJoystick(desc) returns the instance id (no version field; type GAMEPAD),
  SDL_SetJoystickVirtualButton/Axis, SDL_DetachVirtualJoystick(id).
- button_prompts.c: SDL gamepad type numbering changed (XBOX360=2 .. JOYCON_PAIR=10); Luna/Stadia/Shield are gone.

## Nintendo label mapping (SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS is gone)
SDL3 buttons are positional (SOUTH = bottom). sdl_button_to_pad() (input_sdl2.c) maps SOUTH/EAST/WEST/NORTH via
SDL_GetGamepadButtonLabel: label A or Cross -> GC A; B or Circle -> GC B; X or Square -> GC X; Y or Triangle -> GC Y.
Xbox south=A, PS south=Cross, Switch east=A (so Switch labelled-A button = GC A, as before). Unlabelled: by position.

## Done (all steps)
- Both arches build; `objdump -p` shows no SDL DLL import (only system DLLs + opengl32/comdlg32/winhttp).
- tools/smoke boot_to_menu.sh and party_race_2p.sh PASS on build64 and build32 (use absolute exe path).
- GL and D3D12 x windowed/--fullscreen (borderless) --exit-frame 600 exit 0; settings.dat display=fullscreen
  (exclusive, 1280x720 -> drawable 1280x720), borderless, windowed all apply (run without --exit-frame, since
  automated runs force windowed, and kill with `timeout 14`).
- Audio device (WASAPI) starts; --audio-wav non-silent.
- `--list-controllers` added (input_print_controllers in input_sdl2.c; main.c; help text): SDL version, hints,
  every joystick (instance id, name, VID/PID/version, GUID, path, joystick type, gamepad yes/no + type + mapping),
  1 s wait for late devices, verdict, exit 0, no window.
- README "Building from source" -> SDL3 (tools/build_sdl3_mingw.sh), credits and installer/LICENSE.txt (SDL3 3.4.10, 1997-2026).

## Behaviour changes forced by SDL3
- settings.dat / save paths / format unchanged (SDL_GetPrefPath same dir; only the old comparison exe was stale: it
  wrote display=borderless as default, pre-A1).
- controllers.ini is keyed by SDL GUID; SDL3 GUIDs can differ from SDL2's, so stick settings saved under an SDL2
  GUID may not be found again (re-adjust once).
- Luna/Stadia/Shield pads no longer have their own type: they use GameCube prompt glyphs (were Xbox style).
- SDL_INIT_TIMER and SDL_INIT_AUDIO are no longer passed (unused).
- Default window scale on high-DPI uses SDL_GetDisplayContentScale (same 150% -> 1920x1080 intent).
- Refresh rate of a fractional display (59.94) is rounded to the nearest int (SDL2 gave an int mode directly).
- No SDL_main / SDL2main: plain main() with SDL_MAIN_HANDLED.

## Not tested (needs a human)
Alt+Enter, F1 overlay, window drag/resize, file dialogs, first-run disc picker, hot-plug of real pads, Steam Controller
with Steam closed, real keyboard typing (keyboard code unchanged except the key-state array being `const bool*`;
synthetic key injection was blocked in the agent environment).
