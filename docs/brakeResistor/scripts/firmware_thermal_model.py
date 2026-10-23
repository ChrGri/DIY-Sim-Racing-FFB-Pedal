# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "matplotlib"]
# ///
"""Simulate the firmware's PT1 thermal model (PredictiveBrakeController.h) against the
identified physical resistor model.

The firmware constants are parsed from ESP32/include/PredictiveBrakeController.h, the
physical parameters are taken from plots/identified_parameters.json (written by
thermal_identification.py) or fall back to the measured defaults.

Both models integrate the stored thermal energy E = C_th * dT:
    dE/dt = P_in - E / tau
The firmware uses a larger tau than measured (slower assumed cooling), so its energy
estimate is always >= the real one for the same input power.

Usage:
    python firmware_thermal_model.py
"""

import json
import re
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
HEADER = REPO / "ESP32" / "include" / "PredictiveBrakeController.h"
PLOT_DIR = HERE.parent / "plots"
PARAMS = PLOT_DIR / "identified_parameters.json"

DT_S = 0.01  # simulation step (firmware runs at ~1 kHz, the thermal dynamics are >100 s)


def parse_firmware_constants(path=HEADER):
    text = path.read_text()

    def const(name):
        m = re.search(rf"{name}\s*=\s*([0-9.]+)f?", text)
        if not m:
            raise ValueError(f"{name} not found in {path}")
        return float(m.group(1))

    return {
        "tau_s": const("THERMAL_TAU_S_FL32"),
        "e_max_j": const("MAX_ENERGY_JOULES_FL32"),
        "e_recovery_j": const("RECOVERY_ENERGY_JOULES_FL32"),
        "ramp_start_01": const("AVAILABILITY_RAMP_START_01"),
        "cooldown_s": const("THERMAL_COOLDOWN_TIME_US") * 1e-6,
    }


def load_physical_params():
    if PARAMS.exists():
        p = json.loads(PARAMS.read_text())
        return {"tau_s": p["tau_s"], "c_th": p["C_th_J_per_K"], "r_th": p["R_th_K_per_W"],
                "t_on_s": p["t_on_s"], "t_off_s": p["t_off_s"], "power_w": p["power_W"]}
    # measured defaults (5W-10R-LVKE, open air, 2.4 W step)
    return {"tau_s": 201.0, "c_th": 7.65, "r_th": 26.3, "t_on_s": 93.0, "t_off_s": 837.0,
            "power_w": 2.4}


class FirmwareThermalModel:
    """Mirror of the energy model, lockout and availableFraction() of the firmware."""

    def __init__(self, fw):
        self.fw = fw
        self.energy_j = 0.0
        self.lockout = False
        self.lockout_start_s = 0.0

    def step(self, t_s, power_request_w, dt_s):
        fw = self.fw
        power_w = 0.0 if self.lockout else power_request_w
        cooling_w = self.energy_j / fw["tau_s"]
        self.energy_j = max(0.0, self.energy_j + (power_w - cooling_w) * dt_s)
        if self.energy_j >= fw["e_max_j"] and not self.lockout:
            self.lockout = True
            self.lockout_start_s = t_s
        if self.lockout:
            if t_s - self.lockout_start_s > fw["cooldown_s"] and self.energy_j <= fw["e_recovery_j"]:
                self.lockout = False
        return power_w

    def available_fraction(self):
        if self.lockout:
            return 0.0
        used = self.energy_j / self.fw["e_max_j"]
        return float(np.clip((1.0 - used) / (1.0 - self.fw["ramp_start_01"]), 0.0, 1.0))


def simulate(fw, phys, power_profile_w, t, tau_real_list):
    """Run the firmware model; feed the power it actually allows into physical PT1 models."""
    model = FirmwareThermalModel(fw)
    out = {"energy_fw": np.zeros_like(t), "avail": np.zeros_like(t), "lockout": np.zeros_like(t),
           "power": np.zeros_like(t)}
    real = {tau: np.zeros_like(t) for tau in tau_real_list}
    e_real = {tau: 0.0 for tau in tau_real_list}
    for i, ti in enumerate(t):
        p = model.step(ti, power_profile_w[i], DT_S)
        out["power"][i] = p
        out["energy_fw"][i] = model.energy_j
        out["avail"][i] = model.available_fraction()
        out["lockout"][i] = model.lockout
        for tau in tau_real_list:
            e_real[tau] += (p - e_real[tau] / tau) * DT_S
            real[tau][i] = e_real[tau] / phys["c_th"]  # temperature rise [K]
    return out, real


def plot_vs_measurement(fw, phys):
    """Measured 2.4 W step fed into the firmware model: when would it trip?"""
    data = HERE.parent / "data" / "10OhmResistor_temperatureRise_5V_2p4W.txt"
    rows = [l.split(",") for l in data.read_text().splitlines() if l.count(",") == 2]
    meas = np.array([[float(a), float(c)] for a, _, c in rows if a[0].isdigit()])
    t_amb = np.median(meas[meas[:, 0] < phys["t_on_s"] - 5, 1])

    t = np.arange(0.0, meas[-1, 0], DT_S)
    power = np.where((t >= phys["t_on_s"]) & (t < phys["t_off_s"]), phys["power_w"], 0.0)

    # firmware model without lockout effect on the input (the bench supply kept heating)
    e_fw = np.zeros_like(t)
    e = 0.0
    for i in range(len(t)):
        e = max(0.0, e + (power[i] - e / fw["tau_s"]) * DT_S)
        e_fw[i] = e
    t_trip = t[np.argmax(e_fw >= fw["e_max_j"])]

    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.plot(meas[:, 0], phys["c_th"] * (meas[:, 1] - t_amb), ".", ms=3, color="0.55",
            label=f"measured: C_th·ΔT (C_th = {phys['c_th']:.2f} J/K)")
    ax.plot(t, e_fw, color="C3", lw=2, label=f"firmware model (τ = {fw['tau_s']:.0f} s)")
    ax.axhline(fw["e_max_j"], color="C3", ls="--", lw=1)
    ax.axhline(fw["e_recovery_j"], color="C2", ls="--", lw=1)
    ax.axhline(fw["ramp_start_01"] * fw["e_max_j"], color="C1", ls=":", lw=1)
    ax.text(5, fw["e_max_j"] + 6, f"trip {fw['e_max_j']:.0f} J", color="C3")
    ax.text(5, fw["e_recovery_j"] + 6, f"recovery {fw['e_recovery_j']:.0f} J", color="C2")
    ax.text(5, fw["ramp_start_01"] * fw["e_max_j"] - 18,
            f"availability ramp {fw['ramp_start_01'] * fw['e_max_j']:.0f} J", color="C1")
    ax.axvline(t_trip, color="C3", lw=1)
    e_real_trip = np.interp(t_trip, meas[:, 0], phys["c_th"] * (meas[:, 1] - t_amb))
    ax.annotate(f"firmware trips after {t_trip - phys['t_on_s']:.0f} s\n"
                f"real case: {e_real_trip:.0f} J ≈ +{e_real_trip / phys['c_th']:.0f} K",
                xy=(t_trip, fw["e_max_j"]), xytext=(t_trip + 60, 330),
                arrowprops={"arrowstyle": "->"})
    ax.set_xlabel("time [s]")
    ax.set_ylabel("stored thermal energy [J]")
    ax.set_title(f"{phys['power_w']:.1f} W step: firmware energy model vs. measured resistor")
    ax.legend(loc="lower right")
    ax.grid(alpha=0.3)
    sec = ax.secondary_yaxis("right", functions=(lambda e: e / phys["c_th"],
                                                 lambda d: d * phys["c_th"]))
    sec.set_ylabel("temperature rise ΔT [K]")
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "03_firmware_model_vs_measurement.png", dpi=130)
    plt.close(fig)
    return t_trip - phys["t_on_s"]


def plot_trip_time(fw, phys):
    """Time from cold until lockout at constant average power, new PT1 vs. old leaky bucket."""
    p = np.logspace(np.log10(0.5), np.log10(200.0), 400)
    sustain_w = fw["e_max_j"] / fw["tau_s"]
    with np.errstate(invalid="ignore", divide="ignore"):
        t_pt1 = np.where(p > sustain_w, -fw["tau_s"] * np.log(1.0 - fw["e_max_j"] / (p * fw["tau_s"])), np.nan)
        t_old = np.where(p > 3.0, 60.0 / (p - 3.0), np.nan)  # previous model: 60 J, 3 W cooling
        dt_real = p * phys["r_th"] * (1.0 - np.exp(-t_pt1 / phys["tau_s"]))

    fig, (ax, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True)
    ax.loglog(p, t_pt1, color="C3", lw=2, label=f"PT1: {fw['e_max_j']:.0f} J, τ = {fw['tau_s']:.0f} s")
    ax.loglog(p, t_old, color="0.5", ls="--", label="previous leaky bucket: 60 J, 3 W cooling")
    ax.axvline(sustain_w, color="C3", ls=":", lw=1)
    ax.text(sustain_w * 1.05, 2, f"never trips below {sustain_w:.1f} W", color="C3", rotation=90)
    ax.set_ylabel("time until thermal lockout [s]")
    ax.set_title("Thermal budget: time from cold to lockout at constant average power")
    ax.legend()
    ax.grid(alpha=0.3, which="both")

    ax2.semilogx(p, dt_real, color="C0", lw=2,
                 label=f"open air (τ = {phys['tau_s']:.0f} s, C_th = {phys['c_th']:.2f} J/K)")
    ax2.axhline(fw["e_max_j"] / phys["c_th"], color="C3", ls="--", lw=1,
                label=f"E_max / C_th = {fw['e_max_j'] / phys['c_th']:.1f} K")
    ax2.set_xlabel("average resistor power [W]")
    ax2.set_ylabel("real case ΔT at trip [K]")
    ax2.legend()
    ax2.grid(alpha=0.3, which="both")
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "04_trip_time_vs_power.png", dpi=130)
    plt.close(fig)


def plot_braking_sequence(fw, phys):
    """Illustrative hard-braking session: lockout, availability ramp and recovery."""
    stomp_energy_j, stomp_len_s, period_s, session_s = 20.0, 0.4, 5.0, 500.0
    t = np.arange(0.0, 900.0, DT_S)
    in_stomp = (np.mod(t, period_s) < stomp_len_s) & (t < session_s)
    power = np.where(in_stomp, stomp_energy_j / stomp_len_s, 0.0)
    tau_encl = phys["tau_s"] * 1.25
    out, real = simulate(fw, phys, power, t, [phys["tau_s"], tau_encl])

    fig, (ax, ax2) = plt.subplots(2, 1, figsize=(10, 8), sharex=True,
                                  gridspec_kw={"height_ratios": [3, 1]})
    # drawn wide underneath: with tau_fw = 1.25 * tau_measured it coincides with the
    # enclosure curve
    ax.plot(t, out["energy_fw"], color="C3", lw=4, alpha=0.5, label="firmware energy model")
    ax.axhline(fw["e_max_j"], color="C3", ls="--", lw=1)
    ax.axhline(fw["e_recovery_j"], color="C2", ls="--", lw=1)
    ax.axhline(fw["ramp_start_01"] * fw["e_max_j"], color="C1", ls=":", lw=1)
    ax.plot(t, real[phys["tau_s"]] * phys["c_th"], color="C0", lw=1,
            label=f"real resistor, open air (τ = {phys['tau_s']:.0f} s)")
    ax.plot(t, real[tau_encl] * phys["c_th"], color="C9", lw=1,
            label=f"real resistor, enclosure assumption (τ = {tau_encl:.0f} s)")
    ax.fill_between(t, 0, 1, where=out["lockout"] > 0, color="C3", alpha=0.12,
                    transform=ax.get_xaxis_transform(), label="thermal lockout")
    ax.axvline(session_s, color="0.3", lw=1)
    ax.text(session_s + 5, 20, "braking stops", color="0.3")
    ax.set_ylabel("stored thermal energy [J]")
    ax.set_title(f"Illustrative session: {stomp_energy_j:.0f} J brake event every {period_s:.0f} s "
                 f"({stomp_energy_j / period_s:.0f} W average)")
    ax.legend(loc="upper right", fontsize=8)
    ax.grid(alpha=0.3)
    sec = ax.secondary_yaxis("right", functions=(lambda e: e / phys["c_th"],
                                                 lambda d: d * phys["c_th"]))
    sec.set_ylabel("temperature rise ΔT [K]")

    ax2.plot(t, out["avail"], color="C1")
    ax2.set_ylabel("availableFraction()")
    ax2.set_xlabel("time [s]")
    ax2.set_ylim(-0.05, 1.05)
    ax2.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(PLOT_DIR / "05_braking_session.png", dpi=130)
    plt.close(fig)

    lockout_edges = np.flatnonzero(np.diff(out["lockout"]) != 0) * DT_S
    return lockout_edges, max(real[tau_encl]) , max(real[phys["tau_s"]])


def main():
    fw = parse_firmware_constants()
    phys = load_physical_params()
    PLOT_DIR.mkdir(exist_ok=True)

    t_trip = plot_vs_measurement(fw, phys)
    plot_trip_time(fw, phys)
    edges, dt_encl_max, dt_open_max = plot_braking_sequence(fw, phys)

    recovery_s = fw["tau_s"] * np.log(fw["e_max_j"] / fw["e_recovery_j"])
    print(f"firmware: tau = {fw['tau_s']:.0f} s, E_max = {fw['e_max_j']:.0f} J, "
          f"E_recovery = {fw['e_recovery_j']:.0f} J, ramp from {fw['ramp_start_01'] * fw['e_max_j']:.0f} J")
    print(f"max. sustained power without trip   = {fw['e_max_j'] / fw['tau_s']:.2f} W")
    print(f"modelled dT at trip (C_th measured) = {fw['e_max_j'] / phys['c_th']:.1f} K")
    print(f"lockout duration (no heating)       = {recovery_s:.0f} s")
    print(f"{phys['power_w']:.1f} W step trips after             = {t_trip:.0f} s")
    print(f"braking session lockout edges [s]   = {np.round(edges, 1).tolist()}")
    print(f"braking session max real dT         = {dt_open_max:.1f} K open air, "
          f"{dt_encl_max:.1f} K enclosure assumption")


if __name__ == "__main__":
    main()
