"""Acoustic measurement of the servo noise, to compare servo parameter sets objectively.

Records all configured microphones at the same time while the user follows a fixed protocol,
announced by beeps on the default output device:

    1 beep  -> rest:  foot off the pedal
    2 beeps -> hold:  foot resting on the pedal (~3 kg), not moving
    3 beeps -> press: normal presses
    long beep -> end

For every phase and microphone it reports band levels (dBFS, from a Welch spectrum) and the
strongest tonal peaks (candidates for a notch filter). The first 1.5 s after each beep are
skipped. Results go to <out>/<timestamp>_<label>/ (WAV per microphone + result.json).

Usage:
    python servo_noise.py devices
    python servo_noise.py record <label> [--segments rest:6,hold:6,press:12] [--no-beep] [--out DIR]
    python servo_noise.py compare <result dir> [<result dir> ...]
    python servo_noise.py plot <result dir>     (spectrograms + spectrum per phase, needs matplotlib)

Record one run with the servo disabled (label e.g. "servo_off") as the ambient floor.
Microphones are opened so that Windows audio effects (noise suppression, AGC) are bypassed:
WASAPI exclusive mode or WDM-KS. Needs numpy and sounddevice.
"""
import datetime
import json
import os
import sys
import threading
import time
import wave

import numpy as np
import sounddevice as sd

# (key, device name substring, host API, extra settings factory). The analog headset input
# ("HD Audio Microphone", WDM-KS) was too insensitive: it drifted by 1.2 dB with the servo
# off and did not resolve the servo noise.
MICROPHONES = [
    ("razer", "Razer Kiyo Pro", "Windows WASAPI", lambda: sd.WasapiSettings(exclusive=True)),
]
DEFAULT_SEGMENTS = "rest:6,hold:6,press:12"
DEFAULT_OUT = os.path.join(os.path.expanduser("~"), "Downloads", "PedalNoise")
# BANDS_HZ[1] is the main metric: the servo's standstill noise (iSV57, 2026-10-11) fills
# ~500-1900 Hz with a sharp upper edge; below 500 Hz the room dominates
BANDS_HZ = [(50, 270), (500, 2000), (270, 2000), (2000, 8000), (50, 8000)]
# Narrow tones, measured as plain power in the window (the broadband metric removes tones).
# The servo whine is a pure tone at ~11.99 kHz, 33 dB above the room, independent of the loop
# parameters (likely the drive's PWM carrier).
TONES_HZ = {"tone 12 kHz": (11900, 12100)}
# beeps take up to 0.8 s plus the output latency of the speakers
SKIP_AFTER_BEEP_S = 1.5
BEEP_HZ = 1000.0


def find_device(name, hostapi):
    for i, d in enumerate(sd.query_devices()):
        if (d["max_input_channels"] > 0 and name.lower() in d["name"].lower()
                and sd.query_hostapis(d["hostapi"])["name"] == hostapi):
            return i, d
    return None, None


def beep(count, long_beep=False):
    fs = 48000
    tone = np.sin(2 * np.pi * BEEP_HZ * np.arange(int(fs * (0.8 if long_beep else 0.15))) / fs) * 0.3
    gap = np.zeros(int(fs * 0.12))
    sig = np.concatenate([np.concatenate([tone, gap]) for _ in range(count)]).astype(np.float32)
    sd.play(sig, fs)


def welch(x, fs, nfft=8192):
    win = np.hanning(nfft)
    psd = np.zeros(nfft // 2 + 1)
    n = 0
    for i in range(0, len(x) - nfft, nfft // 2):
        psd += np.abs(np.fft.rfft(x[i:i + nfft] * win)) ** 2
        n += 1
    psd /= max(n, 1) * np.sum(win ** 2) * fs / 2   # one-sided power spectral density (FS^2/Hz)
    return np.fft.rfftfreq(nfft, 1 / fs), psd


def broadband_psd(x, fs, nfft=8192, smooth_bins=31):
    """PSD robust against short events and narrow ambient tones: median over time per bin,
    then a running median over frequency. Broadband servo noise passes, a fan line or a
    click does not. Scaled so that stationary noise gives the same level as welch()."""
    win = np.hanning(nfft)
    frames = np.array([np.abs(np.fft.rfft(x[i:i + nfft] * win)) ** 2
                       for i in range(0, len(x) - nfft, nfft // 2)])
    # the median of an exponentially distributed periodogram bin is ln(2) x its mean
    psd = np.median(frames, axis=0) / np.log(2) / (np.sum(win ** 2) * fs / 2)
    h = smooth_bins // 2
    psd = np.array([np.median(psd[max(0, i - h):i + h + 1]) for i in range(len(psd))])
    return np.fft.rfftfreq(nfft, 1 / fs), psd


def analyze_segment(x, fs):
    f, psd = welch(x, fs)
    df = f[1] - f[0]
    bands = {f"{lo}-{hi}": 10 * np.log10(np.sum(psd[(f >= lo) & (f < hi)]) * df + 1e-20)
             for lo, hi in BANDS_HZ}
    fb, psd_bb = broadband_psd(x, fs)
    broadband = {f"{lo}-{hi}": 10 * np.log10(np.sum(psd_bb[(fb >= lo) & (fb < hi)]) * df + 1e-20)
                 for lo, hi in BANDS_HZ}
    # tonal peaks: local maxima 100-4000 Hz more than 8 dB above the running median
    sel = (f >= 100) & (f <= 4000)
    db = 10 * np.log10(psd + 1e-30)
    k = 31
    med = np.array([np.median(db[max(0, i - k):i + k]) for i in range(len(db))])
    idx = [i for i in np.nonzero(sel)[0]
           if db[i] == db[max(0, i - 3):i + 4].max() and db[i] - med[i] > 8.0]
    idx = sorted(idx, key=lambda i: db[i] - med[i], reverse=True)[:5]
    peaks = [(round(float(f[i]), 1), round(float(db[i] - med[i]), 1)) for i in idx]
    tones = {name: 10 * np.log10(np.sum(psd[(f >= lo) & (f < hi)]) * df + 1e-20)
             for name, (lo, hi) in TONES_HZ.items()}
    return bands, broadband, peaks, tones


def record(label, segments, use_beep, out_root):
    streams, buffers, rates = [], {}, {}
    for key, name, api, extra in MICROPHONES:
        dev, info = find_device(name, api)
        if dev is None:
            print(f"microphone '{key}' ({name}, {api}) not found, skipped")
            continue
        rates[key] = int(info["default_samplerate"])
        buffers[key] = []
        def callback(indata, frames, t, status, key=key):
            buffers[key].append(indata[:, 0].copy())
        streams.append(sd.InputStream(device=dev, channels=info["max_input_channels"],
                                      samplerate=rates[key], dtype="float32",
                                      extra_settings=extra(), callback=callback))
    if not streams:
        sys.exit("no microphone available")

    for s in streams:
        s.start()
    t0 = time.monotonic()
    start_unix_s = time.time()   # to align with the SimHub state log (column hostTimeUnixMs)
    marks = []
    for n, (name, dur) in enumerate(segments):
        marks.append((name, time.monotonic() - t0, dur))
        print(f"{name} for {dur:.0f} s", flush=True)
        if use_beep:
            threading.Thread(target=beep, args=(n + 1,)).start()
        time.sleep(dur)
    if use_beep:
        beep(1, long_beep=True)
        time.sleep(0.9)
    for s in streams:
        s.stop()
        s.close()

    out_dir = os.path.join(out_root, datetime.datetime.now().strftime("%Y%m%d_%H%M%S") + "_" + label)
    os.makedirs(out_dir, exist_ok=True)
    result = {"label": label, "microphones": list(buffers), "start_unix_s": start_unix_s,
              "segments": [{"name": name, "start_s": round(start, 2), "duration_s": dur}
                           for name, start, dur in marks]}
    for key, chunks in buffers.items():
        x = np.concatenate(chunks)
        with wave.open(os.path.join(out_dir, f"{key}.wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(rates[key])
            w.writeframes((np.clip(x, -1, 1) * 32767).astype(np.int16).tobytes())
    with open(os.path.join(out_dir, "result.json"), "w") as fjson:
        json.dump(result, fjson, indent=2)
    result = analyze_dir(out_dir)
    print_result(result)
    print(f"\nsaved to {out_dir}")


def analyze_dir(result_dir):
    """(Re)computes the metrics of a recording from its WAV files and updates result.json."""
    with open(os.path.join(result_dir, "result.json")) as fjson:
        result = json.load(fjson)
    for key in result["microphones"]:
        x, fs = load_wav(os.path.join(result_dir, f"{key}.wav"))
        for seg in result["segments"]:
            a = int((seg["start_s"] + SKIP_AFTER_BEEP_S) * fs)
            b = int((seg["start_s"] + seg["duration_s"]) * fs)
            bands, broadband, peaks, tones = analyze_segment(x[a:b], fs)
            seg.setdefault("microphones", {})[key] = {
                "bands_dbfs": bands, "broadband_dbfs": broadband, "tones_dbfs": tones,
                "tonal_peaks_hz_db": peaks}
    with open(os.path.join(result_dir, "result.json"), "w") as fjson:
        json.dump(result, fjson, indent=2)
    return result


def print_result(result):
    print(f"\n== {result['label']} (broadband level, robust against ambient tones and clicks)")
    for seg in result["segments"]:
        for key, m in seg["microphones"].items():
            bands = "  ".join(f"{b} Hz {v:6.1f}" for b, v in m["broadband_dbfs"].items())
            tones = "  ".join(f"{n} {v:6.1f}" for n, v in m["tones_dbfs"].items())
            peaks = ", ".join(f"{fq:.0f} Hz (+{d:.0f} dB)" for fq, d in m["tonal_peaks_hz_db"])
            print(f"{seg['name']:6s} {key:6s} {bands}  {tones}   peaks: {peaks or '-'}")


def compare(dirs):
    """The first directory is the reference, ideally a run with the servo off: the 'servo only'
    column subtracts its power, i.e. estimates the level of the servo noise alone."""
    results = [analyze_dir(d) for d in dirs]
    ref = results[0]
    band = f"{BANDS_HZ[1][0]}-{BANDS_HZ[1][1]}"
    print(f"dBFS, difference to '{ref['label']}' in brackets. broadband = robust against ambient "
          f"tones and clicks, raw = plain band level, servo only = broadband minus the reference "
          f"power (n/a: within 0.5 dB of the reference)")
    tone = next(iter(TONES_HZ))
    print(f"{'':27s}{'broadband ' + band:>22s}{'servo only ' + band:>24s}"
          f"{tone:>22s}{'broadband 50-8000':>22s}")
    for seg in ref["segments"]:
        for key in ref["microphones"]:
            print(f"{seg['name']} / {key}")
            m0 = seg["microphones"][key]
            for r in results:
                s = next((x for x in r["segments"] if x["name"] == seg["name"]), None)
                if s is None or key not in s.get("microphones", {}):
                    continue
                m = s["microphones"][key]
                level, level0 = m["broadband_dbfs"][band], m0["broadband_dbfs"][band]
                if level - level0 > 0.5:
                    servo_only = f"{10 * np.log10(10 ** (level / 10) - 10 ** (level0 / 10)):24.1f}"
                else:
                    servo_only = f"{'n/a':>24s}"
                cols = [(m["tones_dbfs"][tone], m0["tones_dbfs"][tone]),
                        (m["broadband_dbfs"]["50-8000"], m0["broadband_dbfs"]["50-8000"])]
                print(f"   {r['label']:24s}{level:13.1f} ({level - level0:+5.1f}){servo_only}"
                      + "".join(f"{v:13.1f} ({v - v0:+5.1f})" for v, v0 in cols))


def load_wav(path):
    with wave.open(path, "rb") as w:
        fs = w.getframerate()
        x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(np.float64) / 32767
    return x - np.mean(x), fs


def plot(result_dir, f_max_hz=16000.0):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.colors import LinearSegmentedColormap

    surface, ink, ink2, grid = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"
    phase_colors = ["#2a78d6", "#eb6834", "#1baf7a"]   # categorical slots 1-3
    ramp = LinearSegmentedColormap.from_list(
        "blue", [surface, "#cde2fb", "#86b6ef", "#3987e5", "#1c5cab", "#0d366b"])
    plt.rcParams.update({
        "figure.facecolor": surface, "axes.facecolor": surface, "savefig.facecolor": surface,
        "axes.edgecolor": grid, "axes.labelcolor": ink2, "xtick.color": ink2, "ytick.color": ink2,
        "text.color": ink, "axes.spines.top": False, "axes.spines.right": False, "font.size": 10,
        "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
        "legend.frameon": False, "lines.linewidth": 1.6,
    })
    with open(os.path.join(result_dir, "result.json")) as fjson:
        result = json.load(fjson)
    mics = result["microphones"]
    fig = plt.figure(figsize=(13, 3.2 * len(mics) + 3.8))
    gs = fig.add_gridspec(len(mics) + 1, len(mics), height_ratios=[1] * len(mics) + [1.15])
    for row, key in enumerate(mics):
        x, fs = load_wav(os.path.join(result_dir, f"{key}.wav"))
        nfft, hop = 4096, 1024
        win = np.hanning(nfft)
        frames = np.array([np.abs(np.fft.rfft(x[i:i + nfft] * win)) ** 2
                           for i in range(0, len(x) - nfft, hop)])
        f = np.fft.rfftfreq(nfft, 1 / fs)
        t = (np.arange(len(frames)) * hop + nfft / 2) / fs
        db = 10 * np.log10(frames / (np.sum(win ** 2) * fs / 2) + 1e-20).T
        keep = f <= f_max_hz
        vmax = np.percentile(db[keep], 99.5)
        ax = fig.add_subplot(gs[row, :])
        im = ax.pcolormesh(t, f[keep], db[keep], cmap=ramp, vmin=vmax - 50, vmax=vmax,
                           shading="auto", rasterized=True)
        for n, seg in enumerate(result["segments"]):
            ax.axvline(seg["start_s"], color=ink2, lw=0.8, ls=":")
            ax.axvspan(seg["start_s"], seg["start_s"] + SKIP_AFTER_BEEP_S, color=grid, alpha=0.5, lw=0)
            ax.text(seg["start_s"] + SKIP_AFTER_BEEP_S + 0.1, f_max_hz * 0.97, seg["name"],
                    va="top", color=ink, fontsize=9)
        ax.set_ylabel("frequency [Hz]")
        ax.set_title(f"{result['label']}: spectrogram, microphone '{key}'")
        cb = fig.colorbar(im, ax=ax, pad=0.01)
        cb.set_label("dBFS/Hz", color=ink2)
        cb.outline.set_visible(False)
        if row == len(mics) - 1:
            ax.set_xlabel("time [s] (grey: skipped after the beep)")

        ax2 = fig.add_subplot(gs[len(mics), row])
        for n, seg in enumerate(result["segments"]):
            a = int((seg["start_s"] + SKIP_AFTER_BEEP_S) * fs)
            b = int((seg["start_s"] + seg["duration_s"]) * fs)
            fw, psd = welch(x[a:b], fs)
            sel = (fw >= 30) & (fw <= f_max_hz)
            ax2.plot(fw[sel], 10 * np.log10(psd[sel] + 1e-20), color=phase_colors[n % 3],
                     lw=1.2, label=seg["name"])
        for lo, hi in [(270, 2000)]:
            ax2.axvspan(lo, hi, color=grid, alpha=0.6, lw=0, zorder=0)
        ax2.set_xscale("log")
        ax2.grid(True, color=grid, lw=0.8)
        ax2.set_xlabel("frequency [Hz] (shaded: 270-2000 Hz)")
        ax2.set_ylabel("dBFS/Hz")
        ax2.set_title(f"Spectrum per phase, '{key}'")
        ax2.legend(loc="upper right")
    fig.tight_layout()
    out = os.path.join(result_dir, "spectra.png")
    fig.savefig(out, dpi=110)
    plt.close(fig)
    print(f"plot written to {out}")


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)
    cmd = args.pop(0)
    if cmd == "devices":
        for key, name, api, _ in MICROPHONES:
            dev, info = find_device(name, api)
            print(f"{key:6s} -> {dev}: {info['name'] if info else 'NOT FOUND'} ({api})")
    elif cmd == "record":
        label = args.pop(0) if args and not args[0].startswith("--") else "run"
        segments, use_beep, out_root = DEFAULT_SEGMENTS, True, DEFAULT_OUT
        while args:
            opt = args.pop(0)
            if opt == "--segments":
                segments = args.pop(0)
            elif opt == "--no-beep":
                use_beep = False
            elif opt == "--out":
                out_root = args.pop(0)
        segs = [(s.split(":")[0], float(s.split(":")[1])) for s in segments.split(",")]
        record(label, segs, use_beep, out_root)
    elif cmd == "compare":
        compare(args)
    elif cmd == "plot":
        for d in args:
            plot(d)
    else:
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
