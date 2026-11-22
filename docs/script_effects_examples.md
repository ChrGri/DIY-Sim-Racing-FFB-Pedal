# Script effects (effect pipelines) – examples

Script effects let you build your own pedal effects from any SimHub property,
without a firmware change. The plugin evaluates your script for every sample on a
500 Hz clock, streams the samples to the pedal and the pedal plays them back on top
of its force curve and the built-in effects.

Where: **Effects → Custom → SCRIPT1 / SCRIPT2** (two channels per pedal, for the
pedal selected at the top). The **Live plot** button opens a window with the last
5 s of every channel.

## How a script works

| Field | Meaning |
|---|---|
| Enable | channel on/off |
| Output | what the result means: **Pedal angle offset (deg)**, **Force offset (N)** or **Relative force** |
| Gain | multiplies the result (0 … 10) |
| Frequency (Hz) for phase | optional script; drives the `phase` variable (see below) |
| Script | NCalc formula, evaluated 500 times per second |

Output conventions:

- **Pedal angle (deg)**: + rotates the pedal towards the driver. Limit ±2°, and the resulting sled
  travel is limited to ±5 mm on the pedal. With the default geometry 1° is about 5 mm sled travel,
  0.1° about 0.5 mm.
- **Force (N)**: + makes the pedal push back harder. Limit ±60 N.
- **Relative force**: share of the force the foot applies right now, `0.1` = 10 % heavier. Limit ±0.8.

Variables:

| Variable | Meaning |
|---|---|
| `t` | time of the sample in seconds (continuous, keeps running) |
| `dt` | sample period, 0.002 s |
| `prev` | previous result of this script (before gain), for smoothing |
| `phase` | oscillator phase in radians (0 … 2π), integrated from the frequency field |
| `pi` | 3.14159… |
| `[Any.SimHub.Property]` | any SimHub property, e.g. `[DataCorePlugin.GameData.Rpms]` |

Functions (plain NCalc): `Sin`, `Cos`, `Abs`, `Min`, `Max`, `Pow`, `Sqrt`, `Exp`,
`Log`, `Round`, `Floor`, `Ceiling`, `Sign`, `if(condition, a, b)`.
SimHub-specific functions such as `isnull()` or `timesincelastevent()` are **not**
available.

Notes:

- SimHub properties update with the telemetry (~60 Hz), the script runs at 500 Hz.
  Vibrations should therefore come from `t` or `phase`, the properties only set
  strength and frequency.
- Use `phase` (with the frequency field) instead of `Sin(2 * pi * f * t)` whenever
  `f` changes, e.g. with RPM. With `t` the waveform jumps on every frequency change.
- Missing properties count as 0, `true`/`false` as 1/0.
- Property names are case sensitive. Check them in SimHub under
  **Available properties** (enable "Show game specific properties" for
  `GameRawData`). Example: it is `ShakeITMotorsV3Plugin`, not `ShakeItMotorsV3Plugin`.
- Start small (Gain 1, amplitudes below 0.1° / 10 N) and raise slowly.

## 1. Tests

**Direction check** (Output: Pedal angle, game or replay running):

```
0.2
```

The pedal should rotate 0.2° (about 1 mm sled travel) towards you.

**Fixed 20 Hz vibration**:

```
0.06 * Sin(2 * pi * 20 * t)
```

**Slow sweep 0.5 Hz** (to see it in the live plot):

```
0.4 * Sin(0.4 * pi * 0.5 * t)
```

## 2. Engine

**Physics-based engine vibration**: crank frequency (RPM / 60), stronger with throttle.
Output: Pedal angle.

Frequency:
```
[DataCorePlugin.GameData.Rpms] / 60
```
Script:
```
0.004 * (0.3 + 0.7 * [DataCorePlugin.GameData.Throttle] / 100) * Sin(phase)
```

**Engine with firing frequency** of a 4-cylinder 4-stroke (2 ignitions per
revolution), limited to what the servo can do:

Frequency:
```
Min([DataCorePlugin.GameData.Rpms] / 60 * 2, 120)
```
Script:
```
0.003 * Sin(phase)
```

**Engine only near the shift point** (last 15 % before max RPM):

Frequency:
```
[DataCorePlugin.GameData.Rpms] / 60
```
Script:
```
0.008 * Max(0, ([DataCorePlugin.GameData.Rpms] / [DataCorePlugin.GameData.MaxRpm] - 0.85) / 0.15) * Sin(phase)
```

**Rev limiter / redline pulse** (rough 8 Hz knock while the redline is reached):

```
if([DataCorePlugin.GameData.CarSettings_RPMRedLineReached] > 0, 0.05 * Sign(Sin(2 * pi * 8 * t)), 0)
```

**iRacing: shift light** (raw telemetry, `ShiftIndicatorPct` 0 … 1):

```
if([DataCorePlugin.GameRawData.Telemetry.ShiftIndicatorPct] > 0.9, 0.04 * Sin(2 * pi * 30 * t), 0)
```

## 3. Brake

**ABS, strength follows the brake pressure** (Output: Pedal angle):

```
if([DataCorePlugin.GameData.ABSActive] > 0, 0.08 * [DataCorePlugin.GameData.Brake] / 100 * Sin(2 * pi * 25 * t), 0)
```

**ABS as a square-wave knock** (more like a real ABS valve):

```
if([DataCorePlugin.GameData.ABSActive] > 0, 0.06 * Sign(Sin(2 * pi * 12 * t)), 0)
```

**Weight transfer: brake gets heavier with deceleration** (Output: Relative force).
`AccelerationSurge` is negative while braking; check the unit and sign in the live
values of your game and adjust the factor:

```
Max(0, -[DataCorePlugin.GameData.AccelerationSurge]) * 0.05
```

**Same, smoothed** (low-pass with `prev`, ~0.1 s):

```
prev + 0.02 * (Max(0, -[DataCorePlugin.GameData.AccelerationSurge]) * 0.05 - prev)
```

**Brake fade with hot brakes** (Output: Relative force, lighter above 600 °C):

```
-0.15 * Min(1, Max(0, ([DataCorePlugin.GameData.BrakesTemperatureMax] - 600) / 300))
```

**iRacing: ABS from raw telemetry**:

```
if([DataCorePlugin.GameRawData.Telemetry.BrakeABSactive] > 0, 0.08 * Sin(2 * pi * 25 * t), 0)
```

## 4. Wheel slip and lock

**Wheel slip / lock from ShakeIt Motors** (needs the "wheelsliplock" effect exported
in ShakeIt Motors; the export is 0 … 100):

```
[ShakeITMotorsV3Plugin.Export.wheelsliplock.All] / 100 * 0.06 * Sin(2 * pi * 30 * t)
```

**Wheel slip from this plugin's own wheel slip detection**:

```
[DIY_FFB_Pedal.WheelSlipEffectReading] / 100 * 0.06 * Sin(2 * pi * 35 * t)
```

**Traction control on the throttle**:

```
if([DataCorePlugin.GameData.TCActive] > 0, 0.05 * Sin(2 * pi * 20 * t), 0)
```

## 5. Road and kerbs

**iRacing: kerbs** – iRacing reports the rumble strip pitch per tyre. Use the
pitch of the front tyres as frequency, capped for the servo.

Frequency:
```
Min(Max([DataCorePlugin.GameRawData.Telemetry.TireLF_RumblePitch], [DataCorePlugin.GameRawData.Telemetry.TireRF_RumblePitch]), 60)
```
Script:
```
if(Max([DataCorePlugin.GameRawData.Telemetry.TireLF_RumblePitch], [DataCorePlugin.GameRawData.Telemetry.TireRF_RumblePitch]) > 0, 0.06 * Sin(phase), 0)
```

**iRacing: bumps from the suspension speed** (front left shock velocity in m/s):

```
Max(-0.2, Min(0.2, [DataCorePlugin.GameRawData.Telemetry.LFshockVel] * 0.4))
```

**Vertical hits** (any game, heave acceleration, check the unit in your game):

```
Max(-0.3, Min(0.3, [DataCorePlugin.GameData.AccelerationHeave] * 0.02))
```

## 6. Chassis and wheel

**iRacing: steering wheel force blended into the brake** (similar to "FFB
blending" on commercial pedals; `SteeringWheelPctTorque` is the share of the
maximum wheel torque). Output: Force:

```
3 * [DataCorePlugin.GameRawData.Telemetry.SteeringWheelPctTorque]
```

**Cornering load on the throttle** (Output: Relative force):

```
Abs([DataCorePlugin.GameData.AccelerationSway]) * 0.02
```

**Pit lane: soft reminder pulse every second**:

```
if([DataCorePlugin.GameData.IsInPitLane] > 0, if((t % 1) < 0.05, 0.1, 0), 0)
```

## 7. Combining

Two channels per pedal are summed. A typical brake setup:

- SCRIPT1 (Pedal angle): ABS from section 3
- SCRIPT2 (Relative force): weight transfer from section 3

Everything is summed with the built-in effects (ABS/TC, RPM, G-force, …). If you
rebuild an effect as a script, switch the built-in one off.
