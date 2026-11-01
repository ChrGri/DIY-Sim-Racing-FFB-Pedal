"""Combined analysis of a SimHub pedal state log and a Saleae STEP/DIR capture of the same session.

The log's targetPosition_i32 is the pulse counter (PCNT) at the start of each control cycle; the
capture contains every pulse. Both are aligned by cross-correlating their velocity profiles and
refined with a linear clock fit (offset + drift). Then the script reports and plots:

    task cycle timing, fresh load cell rate, servo following lag,
    delivered vs commanded step rate, pulse-to-pulse period change,
    pulse gaps at speed (classified: late cycle vs end of travel).

Usage:
    python analyze_session.py <state_log.txt> <digital.csv> [output_dir]

Record the log with debug flag 64 (every cycle) or 68 (1 kHz); both work, 64 gives the cycle
timing of more cycles. Needs numpy and matplotlib. Default output_dir: ./analysis next to the
capture.
"""
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from analyze_logic_trace import compute_metrics, load_trace

LEAD_BUDGET_US = 750.0      # step command lead (3 cycles), see Main.cpp STEP_COMMAND_LEAD_S
SPEED_GAP_HZ = 2000.0       # gaps at model speeds above this count as "at speed"

# reference palette (light mode)
SURF, INK, INK2, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"
BLUE, ORANGE, AQUA, RED = "#2a78d6", "#eb6834", "#1baf7a", "#e34948"
plt.rcParams.update({
    "figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF,
    "axes.edgecolor": GRID, "axes.labelcolor": INK2, "xtick.color": INK2, "ytick.color": INK2,
    "text.color": INK, "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
    "axes.spines.top": False, "axes.spines.right": False, "font.size": 10,
    "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "legend.frameon": False, "lines.linewidth": 1.6,
})


# ------------------------------------------------------------------------------------ loading
def load_log(path):
    with open(path) as f:
        lines = f.read().splitlines()
    hdr = [h.strip() for h in lines[0].split(',')]
    rows = []
    for line in lines[1:]:
        parts = line.split(',')
        if len(parts) == len(hdr):
            try:
                rows.append([float(x) for x in parts])
            except ValueError:
                pass
    A = np.array(rows)
    d = {h: A[:, i] for i, h in enumerate(hdr)}
    t = d['timeInUs_u32'].copy()
    wraps = np.cumsum(np.concatenate([[0], np.diff(t) < -2**31])) * 2**32   # micros() wrap
    d['t'] = (t + wraps - t[0]) * 1e-6
    return d


class Session:
    def __init__(self, log_path, cap_path):
        self.d = load_log(log_path)
        self.tr = load_trace(cap_path)
        self.m = compute_metrics(self.tr)
        self.rises = np.array(self.tr['rises'])
        self._align()
        self._model_setpoint()

    def _align(self):
        d, tr, rises = self.d, self.tr, self.rises
        dir_t = np.array([e[0] for e in tr['dir_edges']])
        dir_v = np.array([e[1] for e in tr['dir_edges']])
        idx = np.searchsorted(dir_t, rises, side='right') - 1
        lvl = np.where(idx >= 0, dir_v[np.maximum(idx, 0)], tr['dir_init'])
        sgn_hi = np.where(lvl == 1, 1, -1)

        t, pos = d['t'], d['targetPosition_i32']
        fs = 1000.0
        v_log = np.gradient(np.interp(np.arange(0, t[-1], 1 / fs), t, pos)) * fs
        best = None
        for sgn in (1, -1):                      # which DIR level is "forward"
            cum = np.cumsum(sgn * sgn_hi)
            v_cap = np.gradient(np.interp(np.arange(0, tr['duration_s'], 1 / fs), rises, cum)) * fs
            n = len(v_cap) + len(v_log)
            xc = np.fft.irfft(np.fft.rfft(v_cap - v_cap.mean(), n) * np.conj(np.fft.rfft(v_log - v_log.mean(), n)), n)
            k = int(np.argmax(xc))
            score = xc[k] / (np.linalg.norm(v_cap) * np.linalg.norm(v_log))
            if best is None or score > best[0]:
                best = (score, sgn, (k if k < n // 2 else k - n) / fs, cum)
        self.corr, self.dir_sign, coarse, self.cum = best

        # refine offset + drift: the log counter must equal the captured pulse count (+ const)
        sel = (t + coarse > 0.01) & (t + coarse < tr['duration_s'] - 0.01)

        def search(drifts, offsets):
            best_r = None
            for drift in drifts:
                for off in offsets:
                    r = pos[sel] - self.cap_count(t[sel] * (1 + drift) + off)
                    share = np.mean(np.abs(r - np.median(r)) <= 1)
                    if best_r is None or share > best_r[0]:
                        best_r = (share, drift, off)
            return best_r
        # coarse (10 ppm, 50 us), then fine (1 ppm, 5 us): at high step rates a few 10 us of
        # misalignment already shift the count by several pulses
        _, drift0, off0 = search(np.linspace(-300e-6, 300e-6, 61), coarse + np.arange(-3e-3, 3e-3, 50e-6))
        self.match_share, self.drift, self.off = search(drift0 + np.arange(-10e-6, 10.5e-6, 1e-6),
                                                        off0 + np.arange(-60e-6, 61e-6, 5e-6))
        self.count_offset = np.median(pos[sel] - self.cap_count(self.to_cap(t[sel])))

    def cap_count(self, tc):
        i = np.searchsorted(self.rises, tc, side='right')
        return np.where(i > 0, self.cum[np.maximum(i - 1, 0)], 0)

    def to_cap(self, tl):
        return tl * (1 + self.drift) + self.off

    def to_log(self, tc):
        return (tc - self.off) / (1 + self.drift)

    def _model_setpoint(self):
        # model setpoint in steps: virtual position (task space) mapped with a cubic fitted at low speed
        d = self.d
        vp, vv, pos = d['admittance_virtualPosition_m'], d['admittance_virtualVelocity_mps'], d['targetPosition_i32']
        slow = np.abs(vv) < 0.005
        self.setpoint = np.polyval(np.polyfit(vp[slow], pos[slow], 3), vp)

    # ------------------------------------------------------------------------------- metrics
    def cycle_times_us(self):
        cons = np.diff(self.d['cycleCount_u32']) == 1
        return self.d['t'][1:][cons], np.diff(self.d['t'])[cons] * 1e6

    def rate_windows(self, win):
        t = self.d['t']
        tw = np.arange(0.05, t[-1] - 0.05, win)
        delivered = (self.cap_count(self.to_cap(tw + win / 2)) - self.cap_count(self.to_cap(tw - win / 2))) / win
        commanded = (np.interp(tw + win / 2, t, self.setpoint) - np.interp(tw - win / 2, t, self.setpoint)) / win
        return tw, commanded, delivered

    def servo_lag(self):
        d = self.d
        ch = np.concatenate([[True], np.diff(d['servoStateCycleCount_u32']) != 0])
        ts, target, err = d['t'][ch], d['servoPositionTarget_i32'][ch], d['servoPositionError_i16'][ch]
        vs = np.gradient(target, ts)
        moving = np.abs(vs) > 2000
        lag = -np.polyfit(vs[moving], err[moving], 1)[0] if moving.sum() > 20 else float('nan')
        return vs, err, lag, ch.sum() / (ts[-1] - ts[0])

    def classified_gaps(self):
        """Capture gaps at speed, with the longest cycle around them and whether the pedal hit full travel."""
        t, cc = self.d['t'], self.d['cycleCount_u32']
        v_model = np.gradient(self.setpoint, t)
        # travel ends: most frequent position near the extremes (the extremes themselves can be overshoots)
        pos = self.d['targetPosition_i32']

        def mode_near(values):
            u, c = np.unique(values, return_counts=True)
            return u[np.argmax(c)]
        pmax = mode_near(pos[pos >= pos.max() - 12])
        pmin = mode_near(pos[pos <= pos.min() + 12])
        res = []
        for g in self.m['gaps']:
            tl = self.to_log(g[0])
            vm = float(np.interp(tl, t, v_model))
            if abs(vm) < SPEED_GAP_HZ:
                continue
            i = np.searchsorted(t, tl)
            lo, hi = max(i - 12, 1), min(i + 12, len(t) - 1)
            dcc = np.diff(cc[lo:hi]); dts = np.diff(t[lo:hi]) * 1e6
            longest = dts[dcc == 1].max() if np.any(dcc == 1) else float('nan')
            p_after = self.cap_count(g[0] + g[1] + 1e-3) + self.count_offset
            at_travel_end = (abs(p_after - pmax) <= 6) or (abs(p_after - pmin) <= 6)
            kind = "end of travel" if at_travel_end else ("late cycle" if longest > LEAD_BUDGET_US * 0.8 else "other")
            res.append(dict(t_cap=g[0], t_log=tl, gap_us=g[1] * 1e6, speed=vm, longest_cycle_us=longest, kind=kind))
        return res


# ------------------------------------------------------------------------------------ report
def report(s):
    print(f"alignment: velocity correlation {s.corr:.3f}, capture time = log time x (1 {s.drift*1e6:+.0f} ppm) + {s.off:.4f} s")
    print(f"           log pulse counter == captured pulses (+-1) in {s.match_share*100:.1f} % of samples")
    _, cyc = s.cycle_times_us()
    print(f"\ntask cycle time: median {np.median(cyc):.0f} us, p99 {np.percentile(cyc, 99):.0f}, "
          f"p99.9 {np.percentile(cyc, 99.9):.0f}, max {cyc.max():.0f} us; > {LEAD_BUDGET_US:.0f} us (lead budget): "
          f"{np.mean(cyc > LEAD_BUDGET_US)*100:.3f} %")
    fr = s.d['pedalForceRaw_fl32']
    print(f"fresh load cell values: {np.count_nonzero(np.diff(fr)) / s.d['t'][-1]:.0f}/s")
    _, _, lag, srate = s.servo_lag()
    print(f"servo following lag: {abs(lag)*1e3:.1f} ms (servo data {srate:.0f} Hz)")

    print("\ndelivered vs commanded step rate (mean / std of relative deviation):")
    for win in (2e-3, 10e-3, 20e-3):
        _, dm, dl = s.rate_windows(win)
        for lo, hi in ((1000, 3000), (3000, 8000), (8000, 20000)):
            b = (np.abs(dm) >= lo) & (np.abs(dm) < hi)
            if b.sum() > 10:
                r = (dl[b] - dm[b]) / np.abs(dm[b])
                print(f"   window {win*1e3:4.0f} ms, {lo/1000:3.0f}-{hi/1000:3.0f} kHz: {np.mean(r)*100:+5.1f} % / {np.std(r)*100:5.1f} %")

    gaps = s.classified_gaps()
    print(f"\npulse gaps at speed (model > {SPEED_GAP_HZ/1000:.0f} kHz): {len(gaps)}")
    for g in gaps:
        print(f"   capture t={g['t_cap']:7.3f} s  gap {g['gap_us']:5.0f} us at {g['speed']/1000:5.1f} kHz, "
              f"longest cycle nearby {g['longest_cycle_us']:4.0f} us -> {g['kind']}")


# ------------------------------------------------------------------------------------ plots
def plot_all(s, out):
    os.makedirs(out, exist_ok=True)
    d, t = s.d, s.d['t']
    t_cyc, cyc = s.cycle_times_us()
    gaps = s.classified_gaps()

    # 1 overview
    tw, v_cmd, v_del = s.rate_windows(5e-3)
    fig, ax = plt.subplots(3, 1, figsize=(12, 8), sharex=True, gridspec_kw={"height_ratios": [1, 1.6, 1]})
    ax[0].plot(t, d["pedalForceFiltered_fl32"], color=BLUE, lw=1.2)
    ax[0].set_ylabel("force [kg]")
    ax[0].set_title("Session overview: foot force, step rate and task cycle time")
    ax[1].plot(tw, v_cmd / 1e3, color=BLUE, lw=2.2, label="commanded by the model")
    ax[1].plot(tw, v_del / 1e3, color=ORANGE, lw=1.0, label="delivered pulses (logic analyzer)")
    for g in gaps:
        ax[1].axvline(g['t_log'], color=RED, lw=0.8, ls=":", zorder=0)
    ax[1].set_ylabel("step rate [kHz]")
    ax[1].legend(loc="upper left", ncol=2)
    ax[1].text(0.995, 0.04, "dotted red: pulse gaps at speed", transform=ax[1].transAxes, ha="right", color=INK2, fontsize=9)
    ax[2].plot(t_cyc, cyc, ".", ms=1.5, color=BLUE, alpha=0.5)
    ax[2].axhline(250, color=INK2, lw=0.8)
    ax[2].axhline(LEAD_BUDGET_US, color=RED, lw=0.8, ls="--")
    ax[2].text(t[-1], LEAD_BUDGET_US + 10, f"lead budget {LEAD_BUDGET_US:.0f} us", ha="right", va="bottom", color=RED, fontsize=9)
    ax[2].set_ylabel("cycle time [us]")
    ax[2].set_ylim(0, max(900, cyc.max() * 1.1))
    ax[2].set_xlabel("log time [s]")
    fig.tight_layout(); fig.savefig(os.path.join(out, "1_overview.png"), dpi=130); plt.close(fig)

    # 2 task timing
    fig, ax = plt.subplots(figsize=(9, 4.2))
    ax.hist(cyc, bins=np.arange(0, max(900, cyc.max() + 20), 10), color=BLUE, edgecolor=SURF, linewidth=0.5)
    ax.set_yscale("log")
    ax.axvline(LEAD_BUDGET_US, color=RED, ls="--", lw=1)
    ax.text(LEAD_BUDGET_US - 5, 3000, "lead budget", color=RED, fontsize=9, ha="right")
    ax.set_title("Pedal task cycle time (consecutive logged cycles)")
    ax.set_xlabel("time between cycle starts [us]"); ax.set_ylabel("cycles")
    ax.text(0.99, 0.70, f"median {np.median(cyc):.0f} us   p99 {np.percentile(cyc, 99):.0f} us   "
            f"p99.9 {np.percentile(cyc, 99.9):.0f} us   max {cyc.max():.0f} us\n"
            f"{np.mean(cyc > LEAD_BUDGET_US)*100:.3f} % of cycles exceed the lead budget",
            transform=ax.transAxes, ha="right", va="top", color=INK, fontsize=9)
    fig.tight_layout(); fig.savefig(os.path.join(out, "2_task_timing.png"), dpi=130); plt.close(fig)

    # 3 pulse accuracy
    bands = [(1000, 3000, "1-3"), (3000, 8000, "3-8"), (8000, 20000, "8-20")]
    wins = [(2e-3, "2 ms"), (10e-3, "10 ms"), (20e-3, "20 ms")]
    res = np.full((len(wins), len(bands)), np.nan); resq = np.full_like(res, np.nan)
    for i, (w, _) in enumerate(wins):
        _, dm, dl = s.rate_windows(w)
        for j, (lo, hi, _) in enumerate(bands):
            b = (np.abs(dm) >= lo) & (np.abs(dm) < hi)
            if b.sum() > 10:
                res[i, j] = np.std((dl[b] - dm[b]) / np.abs(dm[b])) * 100
                resq[i, j] = np.mean(1 / np.sqrt(6) / (np.abs(dm[b]) * w)) * 100   # +-1 pulse counting limit
    fig, ax = plt.subplots(1, 2, figsize=(12, 4.4))
    x = np.arange(len(bands))
    for i, (w, lab) in enumerate(wins):
        ax[0].bar(x + (i - 1) * 0.26, res[i], width=0.24, color=[BLUE, ORANGE, AQUA][i], label=f"window {lab}")
        ax[0].plot(x + (i - 1) * 0.26, resq[i], "_", ms=16, mew=2, color=INK)
    ax[0].set_xticks(x, [b[2] + " kHz" for b in bands])
    ax[0].set_ylabel("rate deviation, std [%]")
    ax[0].set_title("Delivered vs commanded step rate")
    ax[0].legend(loc="upper right")
    ax[0].set_ylim(0, np.nanmax(res) * 1.45)
    ax[0].text(0.02, 0.97, "black ticks: pure counting limit (+-1 pulse per window)",
               transform=ax[0].transAxes, va="top", fontsize=9, color=INK2)
    per = np.diff(s.rises)
    ratio, f_inst = per[1:] / per[:-1], 1 / per[1:]
    ok = (per[1:] < 5e-3) & (per[:-1] < 5e-3)
    pb = [(200, 1000, "0.2-1"), (1000, 3000, "1-3"), (3000, 8000, "3-8"), (8000, 20000, "8-20"), (20000, 60000, "20-60")]
    res_s = s.tr['resolution_ns'] * 1e-9
    for k, (lo, hi, _) in enumerate(pb):
        b = ok & (f_inst >= lo) & (f_inst < hi)
        if b.sum() < 50:
            continue
        ax[1].plot([k, k], np.percentile(ratio[b], [1, 99]), color=BLUE, lw=8, solid_capstyle="round")
        q = 2 * res_s * np.mean(f_inst[b])
        ax[1].plot([k - 0.3, k + 0.3], [1 + q] * 2, color=INK, lw=1.2)
        ax[1].plot([k - 0.3, k + 0.3], [1 - q] * 2, color=INK, lw=1.2)
    ax[1].axhline(1, color=INK2, lw=0.8)
    ax[1].set_xticks(np.arange(len(pb)), [p[2] + " kHz" for p in pb])
    ax[1].set_ylabel("period[k] / period[k-1]")
    ax[1].set_title("Pulse-to-pulse period change (1st-99th percentile)")
    ax[1].text(0.98, 0.95, "black lines: analyzer resolution\n(on two periods)", transform=ax[1].transAxes,
               ha="right", va="top", fontsize=9, color=INK2)
    fig.tight_layout(); fig.savefig(os.path.join(out, "3_pulse_accuracy.png"), dpi=130); plt.close(fig)

    # 4 where pulses stop: the fastest gap of each kind
    late = [g for g in gaps if g['kind'] != "end of travel"]
    end = [g for g in gaps if g['kind'] == "end of travel"]
    fig, ax = plt.subplots(1, 2, figsize=(12, 4.6))
    if late:
        g = max(late, key=lambda x: abs(x['speed']))
        tg = g['t_cap']
        r = s.rises[(s.rises > tg - 0.002) & (s.rises < tg + 0.002)]
        ax[0].step((r[1:] - tg) * 1e3, 1 / np.diff(r) / 1e3, where="pre", color=ORANGE)
        ax[0].plot((r[1:] - tg) * 1e3, 1 / np.diff(r) / 1e3, ".", ms=4, color=ORANGE)
        for j in np.nonzero((t > g['t_log'] - 0.003) & (t < g['t_log'] + 0.003))[0]:
            ax[0].axvline((s.to_cap(t[j]) - tg) * 1e3, color=INK2, lw=0.6, ls=":")
        ax[0].set_xlim(-2, 2)
        ax[0].set_title(f"Gap at {abs(g['speed'])/1000:.1f} kHz ({g['kind']})")
        ax[0].text(0.02, 0.06, f"gap {g['gap_us']:.0f} us, longest cycle nearby {g['longest_cycle_us']:.0f} us",
                   transform=ax[0].transAxes, fontsize=9, color=INK)
    else:
        ax[0].text(0.5, 0.5, "no gaps at speed away from full travel", ha="center", transform=ax[0].transAxes, color=INK2)
        ax[0].set_title("Gaps at speed")
    ax[0].set_xlabel("time around the gap [ms] (dotted: logged cycle starts)")
    ax[0].set_ylabel("instantaneous step rate [kHz]")
    if end:
        g = max(end, key=lambda x: abs(x['speed']))
    else:  # no gap there: show the fastest arrival at full travel anyway
        pos = d['targetPosition_i32']
        near = np.nonzero(pos >= pos.max() - 1)[0]
        v = np.gradient(s.setpoint, t)
        starts = near[np.concatenate([[True], np.diff(near) > 1])]
        k0 = max(starts, key=lambda k: v[max(k - 4, 0)]) if len(starts) else None
        g = dict(t_log=t[k0], speed=v[max(k0 - 4, 0)]) if k0 is not None else None
    if g is not None:
        tl = g['t_log']
        k = (t > tl - 0.004) & (t < tl + 0.004)
        tt = np.linspace(tl - 0.004, tl + 0.004, 800)
        ax[1].plot((tt - tl) * 1e3, s.cap_count(s.to_cap(tt)) + s.count_offset, color=ORANGE, label="delivered pulse position")
        ax[1].plot((t[k] - tl) * 1e3, s.setpoint[k], "o-", ms=3, color=BLUE, lw=1.2, label="model setpoint")
        ax[1].axhline(np.max(s.setpoint[k]), color=INK2, lw=0.6, ls=":")
        ax[1].legend(loc="lower right")
        ax[1].set_title(f"Arrival at full travel ({abs(g['speed'])/1000:.1f} kHz)")
    ax[1].set_xlabel("time around full travel [ms]")
    ax[1].set_ylabel("position [steps]")
    fig.tight_layout(); fig.savefig(os.path.join(out, "4_where_pulses_stop.png"), dpi=130); plt.close(fig)

    # 5 servo lag
    vs, err, lag, _ = s.servo_lag()
    fig, ax = plt.subplots(figsize=(8, 4.6))
    ax.plot(vs / 1e3, err, ".", ms=2.5, color=BLUE, alpha=0.4)
    xx = np.linspace(vs.min(), vs.max(), 10)
    ax.plot(xx / 1e3, -lag * xx, color=ORANGE, lw=2)
    ax.set_xlabel("servo target speed [kHz steps]"); ax.set_ylabel("servo following error [steps]")
    ax.set_title(f"Servo following lag: {abs(lag)*1e3:.1f} ms (error = -lag x speed)")
    ax.text(0.02, 0.05, "servo data via Modbus; slope = following lag", transform=ax.transAxes, color=INK2, fontsize=9)
    fig.tight_layout(); fig.savefig(os.path.join(out, "5_servo_lag.png"), dpi=130); plt.close(fig)
    print(f"\nimages written to {out}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    session = Session(sys.argv[1], sys.argv[2])
    report(session)
    plot_all(session, sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(os.path.abspath(sys.argv[2])), "analysis"))
