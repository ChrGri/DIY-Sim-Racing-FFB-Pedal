"""Pedal roughness from SimHub state logs: the force ripple the foot feels while the pedal moves.

The motor's torque ripple (6 x 4 pole pairs = 24 per motor revolution, 40-60 Hz at normal stroke
speeds) reaches the foot as a load cell force ripple. The servo's velocity loop rejects it more or
less well, depending on its gains and filters (Pr1.01-1.04, notches Pr2.01-2.06, observer Pr6.23).
This script measures that ripple, so parameter sets can be compared by numbers:

    load cell force and ESP pulse position resampled at 2 kHz -> 150 ms windows, grouped by step
    rate -> cubic trend removed, Hann window, FFT -> rms in orders 20-30 per motor revolution
    ("ripple") and in 30-150 Hz ("buzz"), in grams.

With debug flag 128 (servo velocity readout) it also reports the servo velocity deviation from the
ESP command (aliased at 100 Hz, so only a coarse cross-check).

Record with debug flag 64 (or 192): ~20 s of continuous presses at medium speed, the same way in
every run, so that the step-rate bands fill.

Usage:
    python pedal_roughness.py <state log.txt> [<state log.txt> ...]

Needs numpy; uses load_log() from ../logic_analyzer/analyze_session.py.
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "logic_analyzer"))
from analyze_session import load_log  # noqa: E402

FS_HZ = 2000.0
WINDOW_S = 0.15
STEPS_PER_REV = 3200
RATE_BANDS_HZ = [(2000, 4000), (4000, 7000), (7000, 12000)]
ORDER_MIN, ORDER_MAX = 20.0, 30.0
BUZZ_BAND_HZ = (30.0, 150.0)
MAX_LOG_GAP_S = 0.005


def band_rms(windowed, win, f, lo_hz, hi_hz):
    """RMS of the signal within lo_hz..hi_hz (Parseval with the window's power and the FFT
    length, so zero padding does not inflate it; a sine of amplitude A gives A / sqrt(2))."""
    spec = np.fft.rfft(windowed, len(f) * 2 - 2)
    band = (f > lo_hz) & (f < hi_hz)
    return np.sqrt(2 * np.sum(np.abs(spec[band]) ** 2) / (np.sum(win ** 2) * (len(f) * 2 - 2)))


def force_ripple(d):
    t = d["t"]
    tu = np.arange(0, t[-1], 1 / FS_HZ)
    force_kg = np.interp(tu, t, d["pedalForceRaw_fl32"])
    pos = np.interp(tu, t, d["targetPosition_i32"])
    # gap between logged rows around each point: the USB stream drops rows, and the interpolation
    # bridges a gap with a straight line, which would hide the ripple in that window
    gap = np.interp(tu, t[1:], np.diff(t))
    n = int(WINDOW_S * FS_HZ)
    k = np.arange(n)
    win = np.hanning(n)
    f = np.fft.rfftfreq(4096, 1 / FS_HZ)
    res = {b: [] for b in RATE_BANDS_HZ}
    for start in range(0, len(tu) - n, n // 4):
        rate = abs(pos[start + n - 1] - pos[start]) / WINDOW_S
        band = next((b for b in RATE_BANDS_HZ if b[0] <= rate < b[1]), None)
        if band is None or gap[start:start + n].max() > MAX_LOG_GAP_S:
            continue
        seg = force_kg[start:start + n]
        seg = seg - np.polyval(np.polyfit(k, seg, 3), k)
        rev_hz = rate / STEPS_PER_REV
        ripple = band_rms(seg * win, win, f, ORDER_MIN * rev_hz, ORDER_MAX * rev_hz)
        buzz = band_rms(seg * win, win, f, *BUZZ_BAND_HZ)
        res[band].append((ripple * 1000, buzz * 1000))
    return res


def servo_velocity_deviation(d):
    if not np.any(d["servoVelocityRpm_i16"] != 0):
        return None
    t = d["t"]
    tu = np.arange(0, t[-1], 1 / FS_HZ)
    pos = np.interp(tu, t, d["targetPosition_i32"])
    v_cmd = np.convolve(np.gradient(pos) * FS_HZ, np.ones(20) / 20, "same")
    fresh = np.concatenate([[True], np.diff(d["servoStateCycleCount_u32"]) != 0])
    ts = t[fresh]
    v_servo = d["servoVelocityRpm_i16"][fresh] / 60.0 * STEPS_PER_REV
    best = None
    for lag in np.arange(0, 0.03, 0.0005):
        c = np.interp(ts - lag, tu, v_cmd)
        m = np.abs(c) > 3000
        if m.sum() > 20:
            e = np.sqrt(np.mean((v_servo[m] - c[m]) ** 2)) / np.mean(np.abs(c[m]))
            if best is None or e < best[0]:
                best = (e, lag)
    return best


def servo_tracking(d):
    """Following lag (error / target speed), settling after stops, overshoot and the error
    while holding a force at standstill, from the servo's Modbus data (~100 Hz)."""
    fresh = np.concatenate([[True], np.diff(d["servoStateCycleCount_u32"]) != 0])
    ts = d["t"][fresh]
    target = d["servoPositionTarget_i32"][fresh]
    err = d["servoPositionError_i16"][fresh]
    force = d["pedalForceFiltered_fl32"][fresh]
    v = np.gradient(target, ts)
    moving = np.abs(v) > 2000
    lag = -np.polyfit(v[moving], err[moving], 1)[0] if moving.sum() > 20 else float("nan")
    settle, overshoot = [], []
    i = 1
    while i < len(target) - 1:
        if abs(v[i - 1]) > 3000 and abs(target[i + 1] - target[i]) <= 2:
            direction = np.sign(v[i - 1])
            j = i
            peak_over = 0.0
            while j < len(target) and abs(target[j] - target[i]) <= 2 and abs(err[j]) > 10:
                # error sign: pressing (v > 0) lags with a negative error; the opposite sign is overshoot
                peak_over = max(peak_over, err[j] * direction)
                j += 1
            if j < len(target) and abs(target[j] - target[i]) <= 2:
                settle.append(ts[j] - ts[i])
                overshoot.append(peak_over)
            i = j + 1
        else:
            i += 1
    # holding: target unchanged for at least 0.3 s (the settling after a stop excluded)
    since_change = np.zeros(len(target))
    last_change = ts[0]
    for k in range(1, len(target)):
        if abs(target[k] - target[k - 1]) > 1:
            last_change = ts[k]
        since_change[k] = ts[k] - last_change
    still = (since_change > 0.3) & (force > 2.0)
    hold_err = np.std(err[still]) if still.sum() > 20 else float("nan")
    return dict(lag_ms=lag * 1e3, stops=len(settle),
                settle_ms=np.median(settle) * 1e3 if settle else float("nan"),
                overshoot=np.max(overshoot) if overshoot else float("nan"), hold_err=hold_err)


def main(paths):
    print(f"force ripple per step-rate band, median over {WINDOW_S * 1e3:.0f} ms windows, grams rms "
          f"(ripple = orders {ORDER_MIN:.0f}-{ORDER_MAX:.0f}/rev, buzz = {BUZZ_BAND_HZ[0]:.0f}-"
          f"{BUZZ_BAND_HZ[1]:.0f} Hz)")
    header = "".join(f"{f'{lo / 1e3:.0f}-{hi / 1e3:.0f} kHz':>26s}" for lo, hi in RATE_BANDS_HZ)
    print(f"{'':34s}{header}")
    for path in paths:
        d = load_log(path)
        res = force_ripple(d)
        cols = ""
        for b in RATE_BANDS_HZ:
            v = np.array(res[b])
            cols += (f"{np.median(v[:, 0]):7.1f} / {np.median(v[:, 1]):6.1f} (n{len(v):3d})"
                     if len(v) >= 5 else f"{'too few windows':>26s}")
        name = os.path.basename(path).replace("DiyFfbPedalStateLog_", "")[:32]
        print(f"{name:34s}{cols}")
        tr = servo_tracking(d)
        print(f"{'':34s}servo following lag {tr['lag_ms']:.1f} ms, {tr['stops']} stops: settle to 10 counts "
              f"{tr['settle_ms']:.0f} ms, max overshoot {tr['overshoot']:.0f} counts, error while "
              f"holding force at standstill {tr['hold_err']:.1f} counts std")
        dev = servo_velocity_deviation(d)
        if dev is not None:
            print(f"{'':34s}servo velocity deviation from command: {dev[0] * 100:.1f} % of speed "
                  f"(lag {dev[1] * 1e3:.1f} ms, aliased at 100 Hz)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    main(sys.argv[1:])
