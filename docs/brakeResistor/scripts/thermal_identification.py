# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "scipy", "matplotlib"]
# ///
"""Identify a first-order (PT1) thermal model of the brake resistor from a step response.

Model (temperature rise above ambient, dT = T - T_amb):
    heating (t_on <= t < t_off):  dT = dT_inf * (1 - exp(-(t - t_on) / tau))
    cooling (t >= t_off):         dT = dT(t_off) * exp(-(t - t_off) / tau_cool)

Derived parameters for the heating power P:
    R_th = dT_inf / P      [K/W]
    C_th = tau / R_th      [J/K]
    E_inf = C_th * dT_inf  [J]   (stored thermal energy at steady state)

Usage:
    python thermal_identification.py [data_file] [--power 2.4]
"""

import argparse
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit

HERE = Path(__file__).resolve().parent
DEFAULT_DATA = HERE.parent / "data" / "10OhmResistor_temperatureRise_5V_2p4W.txt"
PLOT_DIR = HERE.parent / "plots"


def load_log(path):
    """Read the logger CSV (time_s, r_ntc_kohm, temp_c); header/garbage lines are skipped."""
    rows = []
    for line in Path(path).read_text().splitlines():
        parts = line.strip().split(",")
        if len(parts) != 3:
            continue
        try:
            rows.append([float(p) for p in parts])
        except ValueError:
            continue
    data = np.array(rows)
    return data[:, 0], data[:, 1], data[:, 2]


def step_model(t, t_amb, dt_inf, tau, t_on, t_off, tau_cool):
    """Ambient -> PT1 heating from t_on -> PT1 cooling from t_off."""
    heat = lambda x: dt_inf * (1.0 - np.exp(-np.clip(x - t_on, 0.0, None) / tau))
    dt = heat(t)
    after = t >= t_off
    dt[after] = heat(t_off) * np.exp(-(t[after] - t_off) / tau_cool)
    return t_amb + dt


def initial_guess(t, temp):
    t_amb0 = np.median(temp[: max(10, len(temp) // 20)])
    t_on0 = t[np.argmax(temp > t_amb0 + 1.5)] - 5.0
    smooth = np.convolve(temp, np.ones(9) / 9, mode="same")
    t_off0 = t[len(smooth) - 5 - np.argmax(smooth[::-1][4:] >= smooth[5:-5].max() - 0.3)]
    return [t_amb0, temp.max() - t_amb0, 200.0, t_on0, t_off0, 200.0]


def identify(t, temp, power_w):
    p0 = initial_guess(t, temp)
    popt, pcov = curve_fit(step_model, t, temp, p0=p0, maxfev=20000)
    perr = np.sqrt(np.diag(pcov))
    names = ["t_amb_C", "dT_inf_K", "tau_s", "t_on_s", "t_off_s", "tau_cool_s"]
    fit = {n: float(v) for n, v in zip(names, popt)}
    fit.update({n + "_std": float(e) for n, e in zip(names, perr)})

    r_th = fit["dT_inf_K"] / power_w
    c_th = fit["tau_s"] / r_th
    fit.update(
        power_W=power_w,
        R_th_K_per_W=r_th,
        C_th_J_per_K=c_th,
        E_inf_J=c_th * fit["dT_inf_K"],
        T_inf_C=fit["t_amb_C"] + fit["dT_inf_K"],
        residual_rms_K=float(np.sqrt(np.mean((temp - step_model(t, *popt)) ** 2))),
    )
    return fit, popt


def plot_fit(t, temp, popt, fit, out_dir):
    model = step_model(t, *popt)
    fig, (ax, axr) = plt.subplots(2, 1, figsize=(10, 7), sharex=True,
                                  gridspec_kw={"height_ratios": [3, 1]})
    ax.plot(t, temp, ".", ms=3, color="0.55", label="measured (NTC on case)")
    ax.plot(t, model, "-", lw=2, color="C3", label="PT1 fit")
    ax.axvline(fit["t_on_s"], ls="--", color="C2", lw=1)
    ax.axvline(fit["t_off_s"], ls="--", color="C0", lw=1)
    ax.axhline(fit["T_inf_C"], ls=":", color="C3", lw=1)
    ax.text(fit["t_on_s"] + 5, fit["t_amb_C"] + 2, "power on", color="C2")
    ax.text(fit["t_off_s"] + 5, fit["t_amb_C"] + 2, "power off", color="C0")
    ax.set_ylabel("case temperature [°C]")
    ax.set_title(
        f"5 W / 10 Ω aluminum resistor, {fit['power_W']:.1f} W step, open air\n"
        f"τ = {fit['tau_s']:.0f} s, ΔT∞ = {fit['dT_inf_K']:.1f} K, "
        f"R_th = {fit['R_th_K_per_W']:.1f} K/W, C_th = {fit['C_th_J_per_K']:.2f} J/K, "
        f"τ_cool = {fit['tau_cool_s']:.0f} s"
    )
    ax.legend(loc="center right")
    ax.grid(alpha=0.3)
    axr.plot(t, temp - model, ".", ms=3, color="0.4")
    axr.axhline(0, color="k", lw=0.8)
    axr.set_ylabel("residual [K]")
    axr.set_xlabel("time [s]")
    axr.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_dir / "01_step_response_fit.png", dpi=130)
    plt.close(fig)


def plot_log_linear(t, temp, fit, out_dir):
    """A first-order system gives a straight line in ln(dT_inf - dT) over time (slope -1/tau)."""
    heating = (t > fit["t_on_s"] + 5) & (t < fit["t_off_s"])
    remaining = fit["dT_inf_K"] - (temp[heating] - fit["t_amb_C"])
    valid = remaining > 1.0  # the log amplifies sensor noise near steady state
    th = t[heating][valid] - fit["t_on_s"]
    y = np.log(remaining[valid])
    slope, intercept = np.polyfit(th, y, 1)

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(th, y, ".", ms=3, color="0.55", label="measured (heating phase)")
    ax.plot(th, intercept + slope * th, "-", color="C3",
            label=f"linear fit: τ = {-1.0 / slope:.0f} s")
    ax.plot(th, np.log(fit["dT_inf_K"]) - th / fit["tau_s"], "--", color="C0",
            label=f"PT1 fit: τ = {fit['tau_s']:.0f} s")
    ax.set_xlabel("time since power on [s]")
    ax.set_ylabel("ln(ΔT∞ − ΔT)  [ln K]")
    ax.set_title("First-order check: remaining temperature rise decays exponentially")
    ax.legend()
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_dir / "02_first_order_check.png", dpi=130)
    plt.close(fig)
    return -1.0 / slope


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("data", nargs="?", default=DEFAULT_DATA)
    ap.add_argument("--power", type=float, default=2.4, help="heating power during the step [W]")
    args = ap.parse_args()

    t, _, temp = load_log(args.data)
    fit, popt = identify(t, temp, args.power)
    PLOT_DIR.mkdir(exist_ok=True)
    plot_fit(t, temp, popt, fit, PLOT_DIR)
    fit["tau_log_linear_s"] = plot_log_linear(t, temp, fit, PLOT_DIR)

    (PLOT_DIR / "identified_parameters.json").write_text(json.dumps(fit, indent=2))
    print(f"ambient            T_amb  = {fit['t_amb_C']:.2f} +- {fit['t_amb_C_std']:.2f} degC")
    print(f"power on / off     t      = {fit['t_on_s']:.1f} s / {fit['t_off_s']:.1f} s")
    print(f"steady-state rise  dT_inf = {fit['dT_inf_K']:.1f} +- {fit['dT_inf_K_std']:.1f} K"
          f"  (T_inf = {fit['T_inf_C']:.1f} degC)")
    print(f"time constant      tau    = {fit['tau_s']:.1f} +- {fit['tau_s_std']:.1f} s (heating)")
    print(f"                   tau    = {fit['tau_log_linear_s']:.1f} s (log-linear check)")
    print(f"                   tau    = {fit['tau_cool_s']:.1f} +- {fit['tau_cool_s_std']:.1f} s (cooling)")
    print(f"thermal resistance R_th   = {fit['R_th_K_per_W']:.1f} K/W")
    print(f"thermal capacity   C_th   = {fit['C_th_J_per_K']:.2f} J/K")
    print(f"stored energy      E_inf  = {fit['E_inf_J']:.0f} J")
    print(f"fit residual RMS          = {fit['residual_rms_K']:.2f} K")


if __name__ == "__main__":
    main()
