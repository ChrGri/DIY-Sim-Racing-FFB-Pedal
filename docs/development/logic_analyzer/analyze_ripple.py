"""Rotation-locked ripple in the step command, for comparing captures (e.g. firmware variants).

The drivetrain produces a force ripple of about 24 cycles per motor revolution while the servo
moves. The admittance model turns the load cell force into the step command, so the ripple
shows up in the pulse position, more or less amplified depending on the model. This script
measures it from the capture alone:

    signed pulse position at 2 kHz -> 200 ms windows with a 3-9 kHz step rate -> cubic trend
    removed, Hann window, FFT -> rms in orders 20-30 per motor revolution (steps) and, for
    reference, in 8-18 Hz (foot motion).

Usage:
    python analyze_ripple.py [--steps-per-rev N] digital.csv [digital.csv ...]

Needs numpy.
"""
import sys

import numpy as np

from analyze_logic_trace import load_trace

FS_HZ = 2000.0
WINDOW_S = 0.2
RATE_MIN_HZ, RATE_MAX_HZ = 3000.0, 9000.0
ORDER_MIN, ORDER_MAX = 20.0, 30.0
REF_BAND_HZ = (8.0, 18.0)


def pulse_position(tr):
    """Signed pulse count sampled at FS_HZ (DIR high = +1)."""
    rises = np.array(tr['rises'])
    dir_t = np.array([e[0] for e in tr['dir_edges']])
    dir_v = np.array([e[1] for e in tr['dir_edges']])
    idx = np.searchsorted(dir_t, rises, side='right') - 1
    level = np.where(idx >= 0, dir_v[np.maximum(idx, 0)], tr['dir_init'])
    sign = np.where(level == 1, 1, -1)
    counts = np.histogram(rises, bins=np.arange(0, tr['duration_s'], 1 / FS_HZ), weights=sign)[0]
    return np.cumsum(counts).astype(float)


def ripple(path, steps_per_rev):
    pos = pulse_position(load_trace(path))
    n = int(WINDOW_S * FS_HZ)
    k = np.arange(n)
    win = np.hanning(n)
    f = np.fft.rfftfreq(4096, 1 / FS_HZ)
    order_rms, ref_rms = [], []
    for start in range(0, len(pos) - n, n // 4):
        seg = pos[start:start + n]
        rate = abs(seg[-1] - seg[0]) / WINDOW_S
        if not RATE_MIN_HZ < rate < RATE_MAX_HZ:
            continue
        seg = seg - np.polyval(np.polyfit(k, seg, 3), k)
        amp = np.abs(np.fft.rfft(seg * win, 4096)) / win.sum() * 2   # amplitude per bin
        rev_hz = rate / steps_per_rev
        in_order = (f > ORDER_MIN * rev_hz) & (f < ORDER_MAX * rev_hz)
        in_ref = (f > REF_BAND_HZ[0]) & (f < REF_BAND_HZ[1])
        order_rms.append(np.sqrt(np.sum(amp[in_order] ** 2)))
        ref_rms.append(np.sqrt(np.sum(amp[in_ref] ** 2)))
    return len(order_rms), np.median(order_rms) if order_rms else float('nan'), \
        np.median(ref_rms) if ref_rms else float('nan')


if __name__ == "__main__":
    args = sys.argv[1:]
    steps_per_rev = 3200
    if len(args) >= 2 and args[0] == "--steps-per-rev":
        steps_per_rev = int(args[1])
        args = args[2:]
    if not args:
        print(__doc__)
        sys.exit(1)
    print(f"orders {ORDER_MIN:.0f}-{ORDER_MAX:.0f}/rev at {steps_per_rev} steps/rev, "
          f"windows {WINDOW_S*1e3:.0f} ms at {RATE_MIN_HZ/1e3:.0f}-{RATE_MAX_HZ/1e3:.0f} kHz step rate")
    for path in args:
        n, order, ref = ripple(path, steps_per_rev)
        print(f"{path}\n   windows {n:3d}   ripple {order:5.2f} steps   "
              f"(8-18 Hz reference {ref:6.2f} steps)")
