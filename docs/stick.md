# Analog sticks: GameCube stick emulation and calibration

## How the GameCube game reads the stick

`main_loop.c` calls `g_read_inputs_and_maybe_reset_console` (0x80009A10, DOL
asm, not decompiled) every frame before `process_inputs`. It runs PADRead and
copies the unprocessed pads to `lbl_80145210`, which the Options controller
screens draw from. Then it reshapes each stick:

* It finds the gate sector (triangle 0, P[j], P[j+1]) that holds the stick and
  computes the barycentric weights a and b.
* It maps the stick onto the square (+-105, `0x8007F244`): `a*Q[j] + b*Q[j+1]`.
* It truncates the result to 0..99 per axis and looks it up in a 100-entry
  response curve (`0x8007F1E0`). The curve is 0 up to 12 and 60 at the top.

The gates the game uses:

| | gate (cardinal / diagonal) | where it lives |
|-|-|-|
| control stick | 84 / (59, 59) | `0x80081AC0` (.data, per pad; Options > Controllers > Control Stick adjustment rewrites it) |
| C stick | 80 / (56, 56) | `0x8007F254` (.rodata) |

Gameplay divides by 60 (`world.c`: `stickX / 60.0`, clamped to +-1 per axis).
The result is that full deflection anywhere on a GC stick's octagonal gate
reads 60 on each axis it drives. A full diagonal is (60, 60), so both tilt
axes are at maximum: the "diagonal gate". Full tilt starts at about 78% of
the stick's travel. Below that the curve is soft: half travel gives about
0.35 of full tilt, and the centre has a dead zone of about 12%.

Before this change, the PC port stubbed that function. It handed the game
`SDL axis / 256` directly, so the game got linear values with no response
curve and no dead zone. Full tilt came at 47% of the stick's travel. Any small
sideways part of a "forward" push became a large sideways tilt: 10 degrees
off forward gave 0.37 sideways tilt where a GC pad gives 0.08. That made
forward running feel slower and sideways movement feel too drastic.

## The PC pipeline (src/input/gc_stick.c, input_sdl2.c)

1. **Host stick to emulated GC pad** (what PADRead would return), done the way
   Dolphin's GCPad emulation does it:
   * Normalise the SDL axis. SDL's range is asymmetric (-32768..32767), so each
     half is scaled separately.
   * Divide the distance by the calibrated edge at that angle.
   * Apply the dead zone, with the remaining range rescaled.
   * Apply the sensitivity.
   * Scale to the gate. The octagon has its vertices at the cardinals and
     diagonals, using Dolphin's radii: main stick 0.7937125 and C stick
     0.7221375, "averaged from multiple genuine GameCube controllers".
   * Round with Dolphin's MapFloat.

   At full deflection this gives:

   | | cardinal | diagonal |
   |-|-|-|
   | control stick | 101 | (71, 71) |
   | C stick | 92 | (65, 65) |

2. **The game's own processing.** It is an exact port of the loop above, with
   the DOL's tables. It runs at the end of `input_poll_gc_pads`, so the game
   and the PC overlays both see processed values. `lbl_80145210`,
   `lbl_80081AC0` and `fn_8000A380` are now real, which makes the game's
   Control Stick adjustment work too.

The keyboard (WASD) gives a full octagon deflection: 101 at the cardinals and
(71, 71) at the diagonals.

Triggers are unchanged. They already carried the processed value (0..0x8C).

## Settings: controllers.ini (save directory)

Each controller is stored under its SDL GUID with these fields:

* `gate` is `octagon` (the default) or `circle`.
* `deadzone` is in percent. The default is 5.
* `sensitivity` is in percent. The default, 100, matches a real GC pad.
* `main` and `cstick` hold 32 calibrated edge distances, multiplied by 1000.

Controllers with no entry use the defaults, with a circle of radius 1 as the
assumed edge.

## Controller Sticks screen (src/game/shim/pc_stick_menu.c)

You can open it from F1 > Controllers or from Options > Game Screen >
Controllers. It shows both sticks:

* The grey ring is the host stick's full range.
* The white dots are the calibrated edge.
* The red ring is the dead zone.
* The gold shape is the emulated GC gate.
* The small white dot is the raw host position.
* The green dot is the emulated GC stick.
* Below each stick: the raw host value, then the GC pad value, then the value
  the game plays with (60 is full tilt).

The rows are Controller, Stick gate, Dead zone, Sensitivity, Calibrate,
Reset to defaults and Back.

### How to calibrate

1. Open the screen and pick your controller (Left/Right on "Controller").
2. Select **Calibrate** and press A.
3. Slowly rotate both sticks around their edges, pressed fully outward, two or
   three times. Each stick's percentage turns green at 100%.
4. Press A. Calibration is saved for that controller. A stick that did not
   reach 100% keeps its old calibration. B cancels.

After calibration, pushing the stick against its edge gives exactly the GC
gate: GC values 101 and 71/71, and the game reads 60 at both the cardinals
and the diagonals.

## Test hooks

`--input-script` tokens:

* `SXn` / `SYn` / `CXn` / `CYn` set port 1's emulated GC pad values.
* `VLXn` / `VLYn` / `VRXn` / `VRYn` set the virtual pad's SDL stick axes.
* `GXn` / `GYn` set the processed value exactly.
* `STK` opens the Controller Sticks screen.

`SMB2_BALL_LOG=1` logs the ball's position and velocity every frame.
`[stick]` log lines show the raw value and the processed value whenever they
change.

Unit test: `tools/stick_test/stick_test.c`.

## Verification (2026-11-13)

`tools/stick_test` reports 193 checks, 0 failed. It checks the following
against Dolphin's octagon and MapFloat: the main stick reaches 101 at the
cardinals and (71, 71) at the diagonals, and the C stick 92 and (65, 65). It
also covers 3600 edge angles of both sticks (within 1 unit), monotonicity, no
clamping, the dead zone, calibration of a squircle-shaped stick, and the game
values (60 at full deflection on every driven axis, curve lookups).

All in-game runs are Challenge Beginner 1, holding the stick from frame 2200
to 2500, with `SMB2_BALL_LOG`.

* **Full deflection forward / diagonal / sideways.** The new mapping, the raw
  Dolphin-equivalent values (SY101, SX71/SY71, SX101) and the old mapping give
  identical trajectories. All of them saturate, so top speed was never the
  bug.
* **Half forward.** The old mapping gives full speed (0.262 at +120 frames).
  The new one gives 0.094. Dolphin agrees with the new mapping.
* **10 degrees off forward.** The old mapping drives the ball into the side
  wall: x reaches 5.1, and progress is 4.5 units behind by +120 frames. The
  new mapping's sideways tilt is 5/60.
* **Dolphin, DSU stick byte 176.** This gives GC raw value 38. The HUD speed
  at TIME 59.70 down to 52.70 reads 11 6 8 10 10 11 11 11 mph, identical to
  the PC with SY38.
* **x64 build.** The ball trajectories are identical to the 32-bit build.
