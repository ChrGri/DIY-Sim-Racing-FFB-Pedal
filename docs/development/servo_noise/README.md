# Servo noise measurement

[servo_noise.py](servo_noise.py) records the servo noise with a microphone, so that servo parameter sets
can be compared by numbers instead of by ear.

```
python servo_noise.py devices
python servo_noise.py record <label>
python servo_noise.py compare <reference dir> <dir> [<dir> ...]
python servo_noise.py plot <dir>
```

Needs `numpy` and `sounddevice`; `plot` also needs `matplotlib`. Recordings are saved to
`~/Downloads/PedalNoise/<timestamp>_<label>/`: one WAV per microphone, plus `result.json` and `spectra.png`.

## Procedure

1. Put the microphone near the pedal. Don't move it, and don't change its Windows input level, for the rest
   of the series. Otherwise the levels aren't comparable.
2. Record a reference with the servo **off**, e.g. `record servo_off`. Every result is compared against it.
3. For each parameter set, change **one** register in the plugin's Servo tab and run `record <label>`.
   - Beeps on the default output device announce the phases:

     | Signal | Phase | What to do |
     |---|---|---|
     | 1 beep | rest (6 s) | foot off the pedal |
     | 2 beeps | hold (6 s) | foot resting on the pedal, ~3 kg, not moving |
     | 3 beeps | press (12 s) | about one press per second, the same in every run |
     | long beep | end | |

   - The first 1.5 s after each beep are skipped.
4. Run `compare <reference> <runs...>`, and `plot` to look at the spectra.

## Metrics

| Metric | Calculation | Use |
|---|---|---|
| broadband 500–2000 Hz | Median over time per frequency bin, then a running median over frequency, integrated over the band | The servo's broadband noise. Robust against clicks and ambient tones (fans, mains). |
| servo only 500–2000 Hz | broadband level minus the reference power | Estimated level of the servo noise alone. Useful when the servo is only a few dB above the room. |
| tone 12 kHz | Plain power in 11.9–12.1 kHz | The servo whine (see below). The broadband metric removes tones, so tones need their own window. |
| tonal peaks | Spectral maxima more than 8 dB above the running median | Candidates for a resonance or notch filter. |

Repeatability with the same setting: about ±0.5 dB for the broadband level and ±1.5 dB for "servo only".

## Microphones

- **Avoid Windows audio effects.** Shared-mode capture with Windows effects (noise suppression, AGC) gated the
  signal and removed stationary noise. `servo_noise.py` therefore opens microphones in **WASAPI exclusive
  mode** or via **WDM-KS**. The list is `MICROPHONES` at the top of the script.
- **Webcam (Razer Kiyo Pro):** usable, 48 kHz.
- **Analog headset input:** too insensitive. It drifted by 1.2 dB with the servo off and did not resolve the
  servo noise.
- **Distance:** moving the microphone from about 50 cm to 5–10 cm from the motor did not raise the servo level.
  The noise is radiated by the pedal frame and the table (structure-borne), not by the motor as a point source.

## Results (2026-10-11, throttle, iSV57)

"Servo only" level in 500–2000 Hz, in dBFS:

| Setting | Rest | Hold | Press |
|---|---|---|---|
| Servo NVM state before the session | −66.4 | −65.5 | – |
| `isv57_tunedParameters.h` flashed | −72.2 | −69.3 | −66.6 |
| Pr6.23 = 0 (instead of 30) | −72.8 | −70.1 | −66.5 |
| Pr1.04 = 50 (instead of 20) | −74.1 | −70.6 | −67.8 |
| Pr1.10 = 300 (instead of 35) | −71.3 | −70.5 | −66.0 |
| Notches Pr2.01/2.04 = 5000 (off) | −67.4 | −69.3 | −64.7 |

- **Flashing `isv57_tunedParameters.h` cut the broadband servo noise by about 4 dB.** The servo's NVM had
  deviated from the table.
- **Pr6.23 and Pr1.10 made no measurable difference.**
- **Pr1.04 = 50** was 1–2 dB quieter, but the pedal felt rougher. Pr1.10 = 300 also felt rougher.
- **Turning off the notches (50 / 90 Hz)** made the servo louder at rest (+4.8 dB). A hump at 75–90 Hz
  appeared, so the notches are active on the iSV57. Keep them.
- **The audible whine is a pure tone at ~11.99 kHz**, 16–22 dB above the room in its window.
  - It is mainly present while the pedal moves, as a short burst during every stroke.
  - None of the loop parameters changed it, and it was there with the old NVM state as well.
  - Most likely it is the drive's PWM carrier: current ripple at the switching frequency.
  - No register for the PWM frequency is known. Mechanical measures are the remaining options: decoupling the
    frame, or an absorbing cover around the motor. 12 kHz is easy to absorb.
