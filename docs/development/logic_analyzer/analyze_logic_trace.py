"""Analysis of a STEP/DIR logic analyzer export (Saleae Logic 2 "digital.csv").

The export contains one row per level change: time [s] followed by the level of every channel.
The channel with the most transitions is taken as STEP, the other one as DIR.

Reports: time resolution, pulse high times, speed changes between consecutive step periods,
isolated gaps inside motion, direction reversals (DIR-to-step setup time, DIR changes while a
step pulse is high).

Usage:
    python analyze_logic_trace.py [path/to/digital.csv]

Only the Python standard library is needed. plot_logic_trace.py imports load_trace() and
compute_metrics() from this file.
"""
import bisect
import csv
import math
import sys
from collections import Counter

DEFAULT_PATH = r"C:\Users\chris\Downloads\PedalLogicAnalyzerOutput\digital.csv"

# periods shorter than this count as "in motion" (5 ms = 200 Hz)
MOTION_PERIOD_S = 5e-3
# consecutive period ratio above this counts as a sudden slow-down
SLOWDOWN_RATIO = 1.5
# isolated gap: period > GAP_FACTOR x shorter neighbour, below GAP_MAX_S, shorter neighbour below GAP_NEIGHBOUR_MAX_S
GAP_FACTOR = 3.0
GAP_MAX_S = 20e-3
GAP_NEIGHBOUR_MAX_S = 2e-3
# minimum DIR-to-step setup time required by the servo drive
DIR_SETUP_MIN_S = 5e-6


def load_trace(path):
    """Read the Saleae export and return edge lists for STEP and DIR."""
    with open(path, newline="") as f:
        rows = list(csv.reader(f))
    names = rows[0][1:]
    data = [(float(r[0]), [int(x) for x in r[1:]]) for r in rows[1:]]

    trans = {n: [] for n in names}
    for (t, lv), (_, prev) in zip(data[1:], data[:-1]):
        for k, n in enumerate(names):
            if lv[k] != prev[k]:
                trans[n].append((t, lv[k]))
    init = {n: data[0][1][k] for k, n in enumerate(names)}

    step_name = max(names, key=lambda n: len(trans[n]))
    dir_name = [n for n in names if n != step_name][0]

    stamps = sorted(set(round(t * 1e9) for t, _ in data))
    # sample period = common divisor of all timestamps; shortest edge spacing is a signal property
    resolution_ns = 0
    for s in stamps:
        resolution_ns = math.gcd(resolution_ns, s)
    min_edge_spacing_ns = min(b - a for a, b in zip(stamps[:-1], stamps[1:]))

    return {
        "path": path,
        "duration_s": data[-1][0],
        "rows": len(data),
        "names": names,
        "trans": trans,
        "init": init,
        "step_name": step_name,
        "dir_name": dir_name,
        "resolution_ns": resolution_ns,
        "min_edge_spacing_ns": min_edge_spacing_ns,
        "rises": [t for t, v in trans[step_name] if v == 1],
        "falls": [t for t, v in trans[step_name] if v == 0],
        "dir_edges": trans[dir_name],
        "dir_init": init[dir_name],
    }


def compute_metrics(tr):
    rises, falls = tr["rises"], tr["falls"]
    m = {}

    # pulse high time: rising edge -> next falling edge
    highs = []
    for r in rises:
        i = bisect.bisect_right(falls, r)
        if i < len(falls):
            highs.append(falls[i] - r)
    m["highs"] = highs

    # period = time between consecutive rising edges; 1/period = momentary step rate
    per = [b - a for a, b in zip(rises[:-1], rises[1:])]
    m["periods"] = per

    # ratio of consecutive periods while in motion (both periods < MOTION_PERIOD_S)
    m["ratios"] = [per[k] / per[k - 1] for k in range(1, len(per))
                   if per[k] < MOTION_PERIOD_S and per[k - 1] < MOTION_PERIOD_S]

    # isolated gaps: one period much longer than its shorter neighbour
    gaps = []
    for k in range(1, len(per) - 1):
        nb = min(per[k - 1], per[k + 1])
        if per[k] > GAP_FACTOR * nb and per[k] < GAP_MAX_S and nb < GAP_NEIGHBOUR_MAX_S:
            gaps.append((rises[k], per[k], per[k - 1], per[k + 1]))
    m["gaps"] = gaps

    # direction reversals
    setup, cut = [], []
    for t, _ in tr["dir_edges"]:
        i = bisect.bisect_right(rises, t)
        if i < len(rises):
            setup.append(rises[i] - t)          # DIR edge -> next rising STEP edge
        if i > 0:
            prv = rises[i - 1]                  # last rising STEP edge before the DIR edge
            j = bisect.bisect_right(falls, prv)
            if j < len(falls) and falls[j] > t:  # that pulse was still high
                cut.append((t, falls[j] - prv))
    m["setup"] = setup
    m["cut"] = cut
    return m


def pct(sorted_list, p):
    return sorted_list[min(len(sorted_list) - 1, int(len(sorted_list) * p / 100))]


def report(tr, m):
    print(f"file: {tr['path']}")
    print(f"duration {tr['duration_s']:.3f} s, rows {tr['rows']}")
    for n in tr["names"]:
        print(f"  {n}: {len(tr['trans'][n])} transitions, initial level {tr['init'][n]}")
    print(f"STEP = {tr['step_name']}, DIR = {tr['dir_name']}")
    print(f"sample period (time resolution): {tr['resolution_ns']} ns, shortest spacing between edges: "
          f"{tr['min_edge_spacing_ns']} ns")
    print(f"step rising edges: {len(tr['rises'])}")

    hs = sorted(m["highs"])
    print(f"high time: min {hs[0]*1e6:.2f} us, p1 {pct(hs, 1)*1e6:.2f}, median {pct(hs, 50)*1e6:.2f}, "
          f"max {hs[-1]*1e6:.1f} us")
    short = sorted(h for h in hs if h < 3e-6)
    print(f"pulses with high time < 3 us: {len(short)}  ({', '.join(f'{h*1e6:.2f}' for h in short[:10])})")

    js = sorted(m["ratios"])
    if js:
        big = sum(1 for j in js if j > SLOWDOWN_RATIO)
        print(f"consecutive period ratio >{SLOWDOWN_RATIO} (sudden slow-down inside motion): {big} of {len(js)} "
              f"({100*big/len(js):.2f} %)")
        print(f"period ratio p1 {pct(js, 1):.2f}, p99 {pct(js, 99):.2f}")

    gaps = m["gaps"]
    print(f"isolated gaps inside motion (period > {GAP_FACTOR:.0f}x shorter neighbour): {len(gaps)}")
    for g in gaps[:10]:
        print(f"   t={g[0]:.6f}s gap {g[1]*1e6:.0f} us between periods {g[2]*1e6:.0f}/{g[3]*1e6:.0f} us")
    if gaps:
        hist = Counter(round(g[1] * 1e4) / 10 for g in gaps)
        print("   gap length histogram (ms):", ", ".join(f"{k}:{v}" for k, v in sorted(hist.items())[:15]))

    print(f"direction changes: {len(tr['dir_edges'])}")
    s = sorted(m["setup"])
    if s:
        print(f"DIR change -> next step rising edge: min {s[0]*1e6:.2f} us, median {pct(s, 50)*1e6:.1f} us")
        print(f"   setups < {DIR_SETUP_MIN_S*1e6:.0f} us: {sum(1 for x in s if x < DIR_SETUP_MIN_S)} of {len(s)}")
    print(f"DIR changed while a step pulse was high: {len(m['cut'])}")
    for c in m["cut"][:10]:
        print(f"   t={c[0]:.6f}s pulse high time {c[1]*1e6:.2f} us")


if __name__ == "__main__":
    trace = load_trace(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PATH)
    report(trace, compute_metrics(trace))
