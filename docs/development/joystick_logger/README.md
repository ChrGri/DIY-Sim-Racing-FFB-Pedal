# Joystick output logger

Use this to compare what the pedal computes against what Windows actually reports on
the gamepad axis. For example: "SimHub looks smooth, but the game axis drops to 0 or
jitters near the bottom of the travel".

## What gets logged

| Log | Written by | Key columns |
|---|---|---|
| Pedal state log, `DiyFfbPedalStateLog_<Pedal>_<Wired\|Wireless>*.txt`, plus the pedal config as `.json` | SimHub plugin, enabled with debug flag 68 | `pedalForceRaw_fl32`, `pedalForceFiltered_fl32`, `pedalTravel_pct`, `joystickPreCurve_u16` (force/travel mapping incl. preload gate, before curve + denoise), `joystickOutputCycle_u16` (final HID value of that cycle), `hostTimeUnixMs` |

`joystickOutputCycle_u16` / `joystickPreCurve_u16` / `pedalTravel_pct` come from the extended
state of the same control cycle. The older `joystickOutput_u16` column is the last
*basic* state, which is only sent every N-th cycle.
| `JoystickLog_*.csv` | `Log-Joystick.ps1` | `hostTimeUnixMs`, `joyId`, `pid`, axes `X Y Z R U V` as seen by winmm/joy.cpl |

Both logs use the same `hostTimeUnixMs` clock, so they can be lined up row by row.
Bridge PID is `0x8331`. Each standalone pedal has its own PID.

## Procedure

1. In the plugin, set debug flag 68 on the pedal being tested (e.g. throttle) and start the
   state log.
2. In a second window, run:
   ```
   powershell -ExecutionPolicy Bypass -File Log-Joystick.ps1 -Seconds 60
   ```
3. Within those 60 s: press the pedal fully once (this shows which axis column it is),
   then release it slowly to 0. Then hold it lightly just above the point where the
   output drops.
4. Stop the state log. Collect the state log `.txt`, its `.json` and the `JoystickLog_*.csv`.

## Reading it

- `joystickPreCurve_u16` toggles to 0 while `pedalTravel_pct` is smooth, and
  `pedalForceFiltered_fl32` dips under preload at the same time → this is the preload
  gate / force floor on the pedal.
- `joystickOutputCycle_u16` already toggles to 0 → the cause is on the pedal side. The
  bridge only forwards it.
- `joystickOutputCycle_u16` is smooth, but the Windows axis jumps → bridge or host side.

The same three signals are also available in the plugin's live plot under "ESP32 & Forces".

Wireless state logs only arrive every ~10-12 ms. That is enough to see 0/value toggling,
but not single-cycle events.
