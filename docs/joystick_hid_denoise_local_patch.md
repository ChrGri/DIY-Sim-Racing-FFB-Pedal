# Local patch: throttle HID denoise (re-apply after upstream firmware)

Local changes on top of DIY FFB Pedal (PCBA V1 / `ControlBoard_PCBA_V1X`).  
**Do not change the bridge.** Re-apply this after merging a new base firmware.

Verified against throttle traces `DiyFfbPedalStateLog_Throttle_Wireless20260908_222448.txt` and `…222830.txt`.

## Why

1. **Force-as-joystick on throttle is noisy.** Small force range (≈4–10.5 kg) maps loadcell/servo hunting into HID. Use **travel** as joystick source.
2. **Idle hunting.** Servo ±1–30 steps at rest leaks into travel HID. Zero the travel joystick while force is below preload.
3. **Upstream denoise was a 1st-order (const-velocity) Kalman on force, then unused on the travel path.** Putting that Kalman on the mapped HID caused overshoot: at 100 % the internal state went above 100, at 0 % below 0. Casting the unclamped float to `uint16_t` wrapped it — **brief HID = 0 at full press, HID = 100 at rest.**
4. Fix: **exponential smoothing on the 0–100 % HID value**, then clamp, then convert to `uint16`.

## Firmware — `ESP32/src/Main.cpp`

Search for `// compute joystick value`.

### A. Travel path: rest-zero below preload

In the non-rudder branch, when `travelAsJoystickOutput_u8 == 1`, do **not** pass raw `pedalArcPercentage` into `NormalizeControllerOutputValue`. Gate it:

```cpp
float travelJoystick_01 =
    constrain(pedalArcPercentage_fl32, 0.0f, 1.0f);
if (filteredReading < dap_calculationVariables_st.forceMin_fl32) {
  travelJoystick_01 = 0.0f;
}
joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(
    travelJoystick_01, 0.0f, 1.0f,
    dap_config_pedalUpdateTask_st.payloadPedalConfig_st
        .maxGameOutput_u8);
```

Force-as-joystick branch stays `filteredReading` / `forceMin` / `forceMax` (no extra Kalman on force for HID).

### B. Replace HID Kalman with EMA + clamp

**Remove** any `kalman_joystick->filteredValue(...)` on force or HID.  
**Do not** assign `eval / 100 * 65535` directly into a `uint16_t` before clamping (out-of-range float → 0 or 65535 on Xtensa).

After `EvalJoystickCubicSpline`:

```cpp
joystickNormalizedToInt32_eval =
    constrain(joystickNormalizedToInt32_eval, 0.0f, 100.0f);

// Exponential HID denoise (no velocity state, so no 0/100 overshoot).
// Slider still "slow to fast": higher kfModelNoiseJoystick_u8 = less lag.
static float joystickSmoothedPercent_fl32 = 0.0f;
static bool joystickSmoothInit_b = false;
if (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfJoystick_u8 ==
    1) {
  float alpha_fl32 =
      1.0f -
      ((float)dap_config_pedalUpdateTask_st.payloadPedalConfig_st
           .kfModelNoiseJoystick_u8 /
       5000.0f);
  alpha_fl32 = constrain(alpha_fl32, 0.0f, 0.999f);
  if (!joystickSmoothInit_b) {
    joystickSmoothedPercent_fl32 = joystickNormalizedToInt32_eval;
    joystickSmoothInit_b = true;
  } else {
    joystickSmoothedPercent_fl32 =
        alpha_fl32 * joystickSmoothedPercent_fl32 +
        (1.0f - alpha_fl32) * joystickNormalizedToInt32_eval;
  }
  joystickNormalizedToInt32_eval = joystickSmoothedPercent_fl32;
} else {
  joystickSmoothInit_b = false;
  joystickSmoothedPercent_fl32 = joystickNormalizedToInt32_eval;
}

float joystickRaw_fl32 = joystickNormalizedToInt32_eval / 100.0f *
                         (float)s_JOYSTICK_MAX_VALUE_U16;
joystickNormalizedToUInt16 = (uint16_t)constrain(
    joystickRaw_fl32, (float)s_JOYSTICK_MIN_VALUE_U16,
    (float)s_JOYSTICK_MAX_VALUE_U16);
```

Tau at 4 kHz (`dt = 250 µs`): `τ = 1.25 / slider` seconds. Slider 20 → **62.5 ms**, 60 → 21 ms, 80 → 16 ms.

`kalman_joystick` may still be allocated in setup; it is unused after this patch and can be deleted.

### Build

```text
pio run -e ControlBoard_PCBA_V1X
```

Output: `ESP32/.pio/build/ControlBoard_PCBA_V1X/firmware.bin`  
OTA copy path used here: `OTA/ControlBoard/Gilphilbert_1_2/firmware.bin`

## Plugin log (optional, not required for the HID fix)

Wireless traces are written in `SimHubPlugin/UICallback/HidRecieveCallback.cs` (wired: `SerialTImer.cs`).  
Append last basic-packet joystick (`Pedal_position_reading[pedal]`) — **no extended-struct / bridge protocol change.**

Header:

```text
, joystickOutput_u16, joystickOutput_pct
```

Row:

```csharp
$",{(UInt16)Pedal_position_reading[pedalSelected]}" +
$",{(Pedal_position_reading[pedalSelected] / 65535.0 * 100.0).ToString("G9")}"
```

`DIYFFBPedalUI.csproj`: T4 import is `Condition="Exists(...TextTemplating.targets)"` so Build Tools can compile without VS T4.

## Throttle profile (SimHub, not firmware)

| Setting | Value |
|---|---|
| Joystick output from travel | on |
| Denoise for Joystick output | on |
| KF for Joystick Denoise | 20 (current; raise toward 40–60 if pumps feel laggy) |
| Filter model | KF const. vel. (loadcell loop, independent of HID EMA) |

## Re-apply checklist after upstream pull

1. Diff `ESP32/src/Main.cpp` around joystick compute. If upstream still KFs force into HID and/or uint16-casts without a float clamp, paste **A** and **B** again.
2. Confirm travel HID is rest-zeroed below preload.
3. Rebuild `ControlBoard_PCBA_V1X` only. Leave bridge firmware alone.
4. Re-send the throttle profile (travel + denoise).
5. Optional: re-add the two log columns and recapture. HID at rest must stay ~0, at full press ~100, no single-sample invert.
