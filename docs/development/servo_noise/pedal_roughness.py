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


def force_ripple(d):
    t = d["t"]
    tu = np.arange(0, t[-1], 1 / FS_HZ)
    force_kg = np.interp(tu, t, d["pedalForceRaw_fl32"])
    pos = np.interp(tu, t, d["targetPosition_i32"])
    n = int(WINDOW_S * FS_HZ)
    k = np.arange(n)
    win = np.hanning(n)
    f = np.fft.rfftfreq(4096, 1 / FS_HZ)
    res = {b: [] for b in RATE_BANDS_HZ}
    for start in range(0, len(tu) - n, n // 4):
        rate = abs(pos[start + n - 1] - pos[start]) / WINDOW_S
        band = next((b for b in RATE_BANDS_HZ if b[0] <= rate < b[1]), None)
        if band is None:
            continue
        seg = force_kg[start:start + n]
        seg = seg - np.polyval(np.polyfit(k, seg, 3), k)
        amp = np.abs(np.fft.rfft(seg * win, 4096)) * 2 / win.sum()
        rev_hz = rate / STEPS_PER_REV
        ripple = np.sqrt(np.sum(amp[(f > ORDER_MIN * rev_hz) & (f < ORDER_MAX * rev_hz)] ** 2) / 2)
        buzz = np.sqrt(np.sum(amp[(f > BUZZ_BAND_HZ[0]) & (f < BUZZ_BAND_HZ[1])] ** 2) / 2)
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
        dev = servo_velocity_deviation(d)
        if dev is not None:
            print(f"{'':34s}servo velocity deviation from command: {dev[0] * 100:.1f} % of speed "
                  f"(lag {dev[1] * 1e3:.1f} ms, aliased at 100 Hz)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    main(sys.argv[1:])
