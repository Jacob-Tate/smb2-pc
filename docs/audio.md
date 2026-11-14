# Audio: MusyX emulation and the PC "DSP"

The GameCube game drives sound through MusyX (the `snd*` API, SoundMacros,
DSP-ADPCM streams) on top of the AX DSP ucode. The PC port splits that in two.

## Threads and timing

* **Synth (game thread)**, `src/game/shim/musyx_shim.c`: the game's `snd*`
  calls, the SoundMacro interpreter, voice lifetimes (`sndFXCheck`) and the
  stream update callbacks. It runs in fixed 5 ms ticks of *game* time (10
  ticks per 3 video frames, from `musyx_frame` in `video_finish_frame`), so
  everything the game can observe is deterministic and independent of the
  audio device.
* **DSP (worker thread)**, `src/audio/audio_miniaudio.c`: renders what the
  synth tells it. The interface is `include/audio_dsp.h`: commands go into a
  lock-free single-producer / single-consumer queue, the worker consumes them
  in order and renders exactly `ADSP_TICK_FRAMES` (240 frames at 48 kHz) per
  tick into the device FIFO (and `--audio-wav`). The game thread never waits
  on audio; a command that does not fit the queue is dropped and counted.
* Memory the worker may still read (sample PCM of a popped group, effect
  buffers) is freed on the worker in command order (`adsp_free`).

The synth services streams on every 4th tick (MusyX `streamHandle`,
0x8003E1E8), so stream callbacks run at 50 Hz; `smgr.c` uses that count as
its music clock (1.2 calls per 60 fps frame) to cue sel_ngc on the beat.

## What the synth covers

* `sndStream*`: DSP-ADPCM ring streams as `smgr.c` drives them (music, stream
  SE).
* `sndPushGroup` / `sndPopGroup`: project FX tables, pool SoundMacros,
  sdir/samp samples (decoded to PCM16 at push, because the game frees samp
  afterwards).
* `sndFXStartParaInfo` / `KeyOff` / `Check` / `SetCtrl(14)`: a SoundMacro
  interpreter for the opcodes this game's SFX banks use.
* Aux sends: MIDI ctrl 0x5B (reverb, aux A) and 0x5D (chorus, aux B) with
  MusyX's send law.

Accepted but ignored: ADSR tables, vibrato, keymaps and layers (minigame
banks only), the surround output, voice priorities / age, volume groups.

## Volume and pan

`salCalcVolume` (0x8004C4F8) looks the linear product of a voice's volume
factors up in `musyx_vol_tab` (0x80113EC8, about (v/127)^2.2, so half volume
is -13 dB) and applies pan and surround pan with the piecewise-linear law in
`musyx_vol_tab[129..132]` = {0, 0.7079, 1, 1} (pan 64 = centre = -3 dB per
side). The surround channel goes to the DSP's S mix, which this AX ucode does
not put into the L/R output, so the port drops it.

## Aux effects (`src/game/shim/musyx_fx.c`)

On the GC the MusyX aux effects are CPU code: every 5 ms synth frame
`salHandleAuxProcessing` (0x8004C398) hands each aux bus (160 s32 samples of
L, R and S at 32 kHz, 16-bit sample scale) to the registered callback, which
processes it in place. The DSP mixes the result back two frames (10 ms) later
(`aux_process` in audio_miniaudio.c reproduces the triple buffering). SMB2
registers the stock effects in `SoundEffectInit`:

* aux A: `sndAuxCallbackReverbHI` (0x8005205C)
* aux B: `sndAuxCallbackChorus` (0x80053430)

musyx_fx.c is a transcription of those DOL functions that keeps every
float-vs-int choice, operation order, single rounding of each
`fadds`/`fmuls`, the fused `fmadds`/`fnmsubs`, and truncating `fctiwz` vs
round-to-nearest `fctiw`, so it produces the same integers as the original for
the same input. Function and table addresses are noted at each function.

* **Fused multiply-add.** Gekko's `fmadds` rounds a*c+b once to single.
  Dolphin, the reference the port is checked against, computes it in double
  and rounds to single; for single operands the product is exact in double, so
  `(float)((double)a * c + b)` matches Dolphin (it can differ from hardware
  only when the double sum is exactly a single-precision tie).
* **Denormals** are kept (the GC runs with FPSCR[NI] = 0).
* **State and threading.** The DOL's effect structs hold pointers and do not
  fit the game's 32-bit struct layout on x64, so the effect state lives in
  port-side mirror objects keyed by the game's struct. Prepare /
  UpdateSettings / Shutdown queue the DOL function to the worker
  (`adsp_run`), which runs it in command order between ticks: the callback
  never sees a half-modified effect and the effect timeline is
  deterministic. The game's struct only supplies the settings.
* Quirks of the original (the one-sample "pre-delay", only 8 of the 128
  chorus filter phases used) are reproduced. Where the DOL reads freed or
  uninitialised memory, the port uses defined values instead. Each case is
  noted at the code.

`tools/musyxfx/` checks the port bit for bit against the DOL code run in a
Gekko interpreter (`build_fxtest.sh`, then `fxcheck.py`).

## Output

The worker mixes voices, streams and aux returns, applies the user volume and
a peak limiter with one tick of look-ahead (the disc music is mastered near
full scale, so music + SFX summed linearly would clip where the GC DSP
saturates), then hands the tick to the miniaudio device FIFO. The FIFO keeps
latency bounded (ticks more than 200 ms ahead are dropped) and, after an
underrun, waits for 50 ms of audio before resuming.
