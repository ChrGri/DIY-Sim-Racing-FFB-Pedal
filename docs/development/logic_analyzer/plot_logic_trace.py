"""Plots for a STEP/DIR logic analyzer export (Saleae Logic 2 "digital.csv").

Writes into an output folder (default: ./images next to this script):
    overview.png          signed step rate over the whole capture, direction changes marked
    waveform_steady.png   raw STEP/DIR levels during fast, steady motion
    waveform_reversal.png raw STEP/DIR levels around a direction change
    waveform_gap.png      raw STEP/DIR levels around the first isolated gap
    metrics.png           histograms of the metrics from analyze_logic_trace.py

Usage:
    python plot_logic_trace.py [path/to/digital.csv] [output_dir]

Needs matplotlib and numpy (pip install matplotlib numpy).
"""
import bisect
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from analyze_logic_trace import (DEFAULT_PATH, DIR_SETUP_MIN_S, GAP_FACTOR, MOTION_PERIOD_S,
                                 SLOWDOWN_RATIO, compute_metrics, load_trace)


def dir_level_at(tr, t):
    """DIR level at time t (initial level toggled once per DIR edge before t)."""
    times = [e[0] for e in tr["dir_edges"]]
    n = bisect.bisect_right(times, t)
    return tr["dir_edges"][n - 1][1] if n > 0 else tr["dir_init"]


def level_series(edges, init, t0, t1):
    """Step-shaped (t, level) series of one channel inside [t0, t1]."""
    times = [e[0] for e in edges]
    i = bisect.bisect_right(times, t0)
    lvl = edges[i - 1][1] if i > 0 else init
    ts, ls = [t0], [lvl]
    while i < len(edges) and edges[i][0] <= t1:
        ts += [edges[i][0], edges[i][0]]
        ls += [lvl, edges[i][1]]
        lvl = edges[i][1]
        i += 1
    ts.append(t1)
    ls.append(lvl)
    return np.array(ts), np.array(ls)


def plot_waveform(tr, t0, t1, title, path, note=None):
    step_edges = tr["trans"][tr["step_name"]]
    st, sl = level_series(step_edges, tr["init"][tr["step_name"]], t0, t1)
    dt, dl = level_series(tr["dir_edges"], tr["dir_init"], t0, t1)

    fig, (a1, a2, a3) = plt.subplots(3, 1, figsize=(11, 5.2), sharex=True,
                                     gridspec_kw={"height_ratios": [1, 1, 1.6]})
    a1.plot((st - t0) * 1e3, sl, lw=0.9, color="tab:blue")
    a1.set_ylabel("STEP")
    a2.plot((dt - t0) * 1e3, dl, lw=0.9, color="tab:orange")
    a2.set_ylabel("DIR")
    for a in (a1, a2):
        a.set_yticks([0, 1])
        a.set_ylim(-0.2, 1.2)
        a.grid(alpha=0.3)

    # momentary step rate of every period inside the window, drawn at the period's end
    rises = tr["rises"]
    i0 = bisect.bisect_left(rises, t0)
    i1 = bisect.bisect_right(rises, t1)
    r = np.array(rises[max(i0 - 1, 0):i1])
    if len(r) > 1:
        per = np.diff(r)
        a3.step((r[1:] - t0) * 1e3, 1.0 / per / 1e3, where="pre", color="tab:green")
        a3.plot((r[1:] - t0) * 1e3, 1.0 / per / 1e3, ".", ms=3, color="tab:green")
    a3.set_ylabel("step rate [kHz]\n(1 / period)")
    a3.set_xlabel(f"time [ms] from t = {t0:.4f} s")
    a3.grid(alpha=0.3)
    a3.set_xlim(0, (t1 - t0) * 1e3)
    a1.set_title(title)
    if note:
        a3.text(0.01, 0.95, note, transform=a3.transAxes, va="top", fontsize=9,
                bbox=dict(facecolor="white", alpha=0.8, edgecolor="none"))
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "images")
    os.makedirs(out, exist_ok=True)

    tr = load_trace(csv_path)
    m = compute_metrics(tr)
    rises = np.array(tr["rises"])
    per = np.diff(rises)

    # 1) overview: signed step rate, sign = DIR level (high = +)
    dir_times = [e[0] for e in tr["dir_edges"]]
    dir_lvl = np.array([dir_level_at(tr, t) for t in rises[1:]])
    rate = 1.0 / per * np.where(dir_lvl == 1, 1.0, -1.0)
    moving = per < 0.1
    fig, ax = plt.subplots(figsize=(11, 4))
    ax.plot(rises[1:][moving], rate[moving] / 1e3, ",", color="tab:blue")
    for t in dir_times:
        ax.axvline(t, color="tab:orange", lw=0.4, alpha=0.5)
    for g in m["gaps"]:
        ax.plot(g[0], 0, "v", color="tab:red", ms=5)
    ax.set_xlabel("time [s]")
    ax.set_ylabel("step rate [kHz] (sign = DIR)")
    ax.set_title(f"Step rate over the capture: {len(rises)} pulses, {len(dir_times)} direction changes "
                 f"(orange), {len(m['gaps'])} isolated gaps (red)")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(os.path.join(out, "overview.png"), dpi=120)
    plt.close(fig)

    # 2) steady fast motion: the 10 ms window with the highest mean rate and no DIR change
    win = 10e-3
    best = None
    for k in range(0, len(rises) - 1, 50):
        t0 = rises[k]
        j = bisect.bisect_right(tr["rises"], t0 + win)
        if j - k < 5 or any(t0 <= t <= t0 + win for t in dir_times):
            continue
        if best is None or j - k > best[1]:
            best = (t0, j - k)
    if best:
        plot_waveform(tr, best[0] - 1e-3, best[0] + win, "Fast steady motion (raw levels)",
                      os.path.join(out, "waveform_steady.png"),
                      note=f"rate steps of one sample ({tr['resolution_ns']/1000:g} us) in the period "
                           f"are analyzer quantization, not pulse jitter")

    # 3) direction change with pulses on both sides (smallest setup time > 0)
    cands = []
    for t, _ in tr["dir_edges"]:
        i = bisect.bisect_right(tr["rises"], t)
        if 0 < i < len(tr["rises"]):
            before = t - tr["rises"][i - 1]
            after = tr["rises"][i] - t
            if before < 5e-3 and after < 5e-3:
                cands.append((before + after, t, after))
    if cands:
        _, t, after = min(cands)
        plot_waveform(tr, t - 6e-3, t + 6e-3, "Direction change (raw levels)",
                      os.path.join(out, "waveform_reversal.png"),
                      note=f"DIR edge at 6 ms, next STEP rising edge {after*1e6:.0f} us later")

    # 4) first isolated gap
    if m["gaps"]:
        tg, gl, pb, pa = m["gaps"][0]
        plot_waveform(tr, tg - 8e-3, tg + gl + 8e-3, "Isolated gap inside motion (raw levels)",
                      os.path.join(out, "waveform_gap.png"),
                      note=f"gap {gl*1e3:.2f} ms between periods {pb*1e6:.0f} / {pa*1e6:.0f} us")

    # 5) metric histograms
    fig, axs = plt.subplots(2, 2, figsize=(11, 7.5))
    hs = np.array(m["highs"]) * 1e6
    ax = axs[0, 0]
    ax.hist(hs[hs < 2000], bins=100, color="tab:blue")
    ax.set_yscale("log")
    ax.set_xlabel("pulse high time [us] (< 2 ms shown)")
    ax.set_ylabel("pulses")
    ax.set_title(f"Pulse high time  (min {hs.min():.1f} us, median {np.median(hs):.0f} us)")

    ax = axs[0, 1]
    rr = np.array(m["ratios"])
    ax.hist(np.clip(rr, 0, 3), bins=np.linspace(0, 3, 121), color="tab:green")
    ax.axvline(SLOWDOWN_RATIO, color="tab:red", ls="--", lw=1)
    ax.set_yscale("log")
    ax.set_xlabel("period[k] / period[k-1]  (clipped at 3)")
    ax.set_ylabel("pulse pairs")
    p1, p99 = np.percentile(rr, [1, 99])
    ax.set_title(f"Speed change pulse to pulse  (p1 {p1:.2f}, p99 {p99:.2f}; "
                 f"{(rr > SLOWDOWN_RATIO).sum()} > {SLOWDOWN_RATIO})", fontsize=10)

    ax = axs[1, 0]
    gl = np.array([g[1] for g in m["gaps"]]) * 1e3
    if len(gl):
        ax.hist(gl, bins=np.arange(0, 20.25, 0.25), color="tab:red")
    ax.axvline(65536 / 10e6 * 1e3, color="k", ls=":", lw=1, label="65536 ticks @ 10 MHz = 6.55 ms")
    ax.legend(fontsize=8)
    ax.set_xlabel("gap length [ms]")
    ax.set_ylabel("gaps")
    ax.set_title(f"Isolated gaps (> {GAP_FACTOR:.0f}x shorter neighbour): {len(gl)}")

    ax = axs[1, 1]
    su = np.array(m["setup"]) * 1e6
    if len(su):
        ax.hist(su, bins=np.logspace(0, np.log10(max(su.max(), 10)) + 0.1, 40), color="tab:orange")
    ax.set_xscale("log")
    ax.axvline(DIR_SETUP_MIN_S * 1e6, color="tab:red", ls="--", lw=1, label="servo minimum 5 us")
    ax.legend(fontsize=8)
    ax.set_xlabel("DIR edge -> next STEP rising edge [us]")
    ax.set_ylabel("direction changes")
    ax.set_title(f"DIR setup time  (min {su.min():.0f} us)" if len(su) else "DIR setup time")

    fig.suptitle(f"Metrics  ({os.path.basename(csv_path)}, {tr['duration_s']:.1f} s, "
                 f"resolution {tr['resolution_ns']/1000:g} us)")
    fig.tight_layout()
    fig.savefig(os.path.join(out, "metrics.png"), dpi=120)
    plt.close(fig)
    print(f"images written to {out}")


if __name__ == "__main__":
    main()
