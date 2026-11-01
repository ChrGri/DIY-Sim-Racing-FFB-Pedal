# IMM Kalman Filter for the Pedal Force (HOLD / MOVE)

Implementation: [`ESP32/src/SignalFilter_IMM.cpp`](../../ESP32/src/SignalFilter_IMM.cpp), class `KalmanFilterIMM`.
Selected by `kfModelOrder_u8 = 4` ("IMM adaptive (hold/move)" in the SimHub plugin's load cell filter dropdown).

Experimental extensions that couple the filter to the admittance model (plate inertia compensation, foot reaction
removal, HOLD damping, startup plate identification) are implemented but **disabled by default**. See section 11.

---

## 1. Purpose

The admittance controller (`MoveByAdmittanceStrategy`) turns the measured foot force into pedal motion. Any noise
left in the force signal moves the virtual pedal. For slow noise the displacement is roughly

$$\Delta x \approx \frac{\Delta F_{\text{noise}}}{k_{\text{local}}}$$

This is largest at light force, where the force curve is flattest. The foot yields to the motion, the load cell
reading changes, and the loop closes. The result is a subtle vibration while holding the pedal still.

A fixed linear filter can only trade noise against lag, and lag inside the admittance loop makes the pedal feel
heavy or unstable. The IMM (Interacting Multiple Model) filter instead runs two hypotheses about the foot force
and blends them by how well each explains the incoming samples:

| Model | Assumption | Behaviour |
|---|---|---|
| **HOLD** (index 0) | intent force is constant (random walk) | strong smoothing, ≈ 1.5 Hz |
| **MOVE** (index 1) | intent force changes at a rate (constant rate) | low lag, 5–60 Hz (slider) |

The admittance model's own states are known exactly (they are our integrator), so they enter the filter as
**known inputs**, not as estimated states.

---

## 2. Position in the signal chain

```text
load cell task ──queue──► loadcellReading [kg]
                            │  convertToPedalForce(·, sledPosition)   (linear: F = g(x_sled) · F_lc)
                            ▼
                     pedalForce_fl32 [kg]
                            │  KalmanFilterIMM::filteredValue(z, newSample, Δt, slider, x_model, a_model)
                            ▼                                         ▲ previous cycle's admittanceStates_st
                     filteredReading [kg] ──► MoveByAdmittanceStrategy(..., μ_H)
                     changeVelocity  [kg/s]
```

- `newSample` is true only in cycles where the load cell queue delivered a fresh value. In the other cycles the
  filter only predicts; held values are never used as new measurements.
- $x_{\text{model}}$ and $a_{\text{model}}$ are the admittance model's position (`physicalPos_m`, task space, m) and
  acceleration (`virtualAcc_mps2`) from the previous cycle.
- $\mu_H$ (HOLD probability) is passed to the admittance strategy for the optional HOLD damping (section 11.3).

---

## 3. Measurement model with known inputs

The load cell measures more than the foot's intent:

$$z = F_{\text{intent}} - k_{\text{foot}}\,\Delta x + c_m\,a_{\text{plate}} + v,\qquad v \sim \mathcal{N}(0, R)$$

| Term | Meaning | Source |
|---|---|---|
| $F_{\text{intent}}$ | force the foot wants to apply | estimated |
| $k_{\text{foot}}\,\Delta x$ | passive reaction of the compliant foot to pedal micro-motion | $k_{\text{foot}}$ estimated, $\Delta x$ known |
| $c_m\,a_{\text{plate}}$ | plate inertia seen by the load cell | $c_m$ identified at startup, $a$ known |

### 3.1 Delay alignment

The model states lead the load cell by the servo and ADC delay $\tau$. A 64-entry ring buffer delays them by

$$d = \operatorname{round}(\tau / \Delta t)\ \text{cycles},\qquad x_d = x_{\text{model}}(t - d\,\Delta t),\quad a_d = a_{\text{model}}(t - d\,\Delta t)$$

$\tau$ defaults to 2 ms, or comes from the startup identification when it is enabled (section 11.1).

### 3.2 Pedal micro-motion $\Delta x$

$\Delta x$ must contain only the small movement around the current hold point, not the travel of a press.
Otherwise a finished press would leave a large $\Delta x$ that the model reads as foot reaction. A foot reference
position $x_{\text{ref}}$ therefore follows the pedal immediately in MOVE and slowly in HOLD:

$$\alpha_{\text{ref}} = \mu^M + \mu^H\left(1 - e^{-\Delta t/\tau_{\text{ref}}}\right),\qquad
x_{\text{ref}} \leftarrow x_{\text{ref}} + \alpha_{\text{ref}}\,(x_d - x_{\text{ref}}),\qquad \tau_{\text{ref}} = 0.5\ \text{s}$$

$$\Delta x = \operatorname{clamp}\left(x_d - x_{\text{ref}},\ \pm 1\ \text{mm}\right)$$

### 3.3 Inertia-compensated measurement

$$z' = z - \gamma\,c_m\,a_d$$

$\gamma$ = `s_inertiaCompensationFraction_fl32`, **0 by default**, so $z' = z$.

### 3.4 Linear, time-varying measurement matrix

Because $\Delta x$ is a known input, the measurement is linear in the state:

$$z' = \mathbf{H}_k\,\mathbf{x}_k + v_k,\qquad \mathbf{H}_k = \begin{bmatrix} 1 & 0 & -\Delta x_k \end{bmatrix}$$

The filter stays a linear Kalman filter; no EKF is needed.

---

## 4. State and models

Both models share the state vector, so their estimates can be mixed:

$$\mathbf{x} = \begin{bmatrix} F \\ \dot F \\ k \end{bmatrix} \qquad [\text{kg},\ \text{kg/s},\ \text{kg/m}]$$

$F$ is the intent force, $k$ the foot stiffness.

### 4.1 HOLD model: random walk on the force

$$\mathbf{F}_H = \begin{bmatrix} 1 & 0 & 0 \\ 0 & 0 & 0 \\ 0 & 0 & 1 \end{bmatrix},\qquad
\mathbf{Q}_H = \begin{bmatrix} q_H\,\Delta t & 0 & 0 \\ 0 & \varepsilon & 0 \\ 0 & 0 & q_k\,\Delta t \end{bmatrix}$$

The force rate is forced to zero. $\varepsilon = 10^{-9}$ only keeps $\mathbf{P}$ positive definite.

### 4.2 MOVE model: constant force rate (white noise on $\dot F$)

$$\mathbf{F}_M = \begin{bmatrix} 1 & \Delta t & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1 \end{bmatrix},\qquad
\mathbf{Q}_M = q_M \begin{bmatrix} \tfrac{\Delta t^3}{3} & \tfrac{\Delta t^2}{2} & 0 \\[2pt] \tfrac{\Delta t^2}{2} & \Delta t & 0 \\ 0 & 0 & 0 \end{bmatrix}
+ \begin{bmatrix} 0 & 0 & 0 \\ 0 & 0 & 0 \\ 0 & 0 & q_k\,\Delta t \end{bmatrix}$$

The upper block is the exact discretization of continuous white noise with spectral density $q_M$ acting on $\dot F$.

### 4.3 Foot stiffness

$k$ is a slow random walk in both models, $q_k = (200\ \text{kg/m})^2/\text{s}$. The prior is $k_0 = 0$ with
standard deviation 1000 kg/m.

---

## 5. Noise parameters

### 5.1 Measurement noise $R$

At startup, `loadcell->estimateBiasAndVariance()` measures the load cell variance $\sigma^2_{lc}$ in kg². The
conversion to pedal force is linear in the load cell force, with a gain that depends on the sled position:

$$g(x_{\text{sled}}) = \texttt{convertToPedalForce}(1,\ x_{\text{sled}})$$

$R$ is updated every cycle to that gain:

$$R = \sigma^2_{lc}\; g(x_{\text{sled}})^2$$

### 5.2 Process noise from target bandwidths

For a Kalman filter at steady state, measurement noise with variance $R$ sampled every $T$ seconds is equivalent to
a continuous noise density

$$r = R\,T$$

where $T$ is the averaged load cell sample interval (section 7.4). For the continuous-time (Kalman–Bucy) steady
state of each model:

- **Random walk (HOLD):** first-order low pass with $\omega_H = \sqrt{q_H / r}$, so

$$q_H = R\,T\,\omega_H^2,\qquad \omega_H = 2\pi f_H,\quad f_H = 1.5\ \text{Hz}$$

- **Constant rate (MOVE):** second-order low pass with $\omega_n = (q_M / r)^{1/4}$ and damping $\zeta = 1/\sqrt 2$, so

$$q_M = R\,T\,\omega_M^4,\qquad \omega_M = 2\pi f_M$$

Specifying the noise as bandwidths keeps the filter's behaviour the same regardless of load cell noise level and
sample rate.

### 5.3 Slider mapping

The existing "Loadcell denoising" slider $s \in [1, 255]$ (`kfModelNoise_u8`) selects the MOVE bandwidth on a
logarithmic scale:

$$f_M = f_{\min}\left(\frac{f_{\max}}{f_{\min}}\right)^{\frac{s-1}{254}},\qquad f_{\min} = 5\ \text{Hz},\ f_{\max} = 60\ \text{Hz}$$

---

## 6. Mode switching (Markov chain)

The mode is modelled as a Markov chain with an expected dwell time $\tau_j$ in each mode:

$$p_{jj} = e^{-\Delta t / \tau_j},\qquad p_{ij} = 1 - p_{ii}\ \ (i \neq j)$$

$$\boldsymbol{\Pi} = \begin{bmatrix} p_{HH} & 1 - p_{HH} \\ 1 - p_{MM} & p_{MM} \end{bmatrix},\qquad \tau_H = 0.3\ \text{s},\ \ \tau_M = 0.15\ \text{s}$$

$p_{ij}$ is the probability of being in model $j$ now, given model $i$ in the previous cycle.

---

## 7. One IMM cycle

Inputs: the previous estimates $\hat{\mathbf{x}}^i,\ \mathbf{P}^i$ and mode probabilities $\mu^i$ for
$i \in \{H, M\}$, the measurement $z'$ and the matrix $\mathbf{H}_k$ from section 3.

### 7.1 Interaction (mixing)

Predicted mode probabilities:

$$\bar c_j = \sum_i p_{ij}\,\mu^i$$

Mixing weights (probability that the system was in model $i$, given that it is in model $j$ now):

$$\mu^{i|j} = \frac{p_{ij}\,\mu^i}{\bar c_j}$$

Mixed initial state and covariance for each model $j$:

$$\hat{\mathbf{x}}^{0j} = \sum_i \mu^{i|j}\,\hat{\mathbf{x}}^i$$

$$\mathbf{P}^{0j} = \sum_i \mu^{i|j}\left[\mathbf{P}^i + \left(\hat{\mathbf{x}}^i - \hat{\mathbf{x}}^{0j}\right)\left(\hat{\mathbf{x}}^i - \hat{\mathbf{x}}^{0j}\right)^{\!\top}\right]$$

The spread term adds uncertainty when the two models disagree.

### 7.2 Prediction (per model)

$$\hat{\mathbf{x}}^j_{-} = \mathbf{F}_j\,\hat{\mathbf{x}}^{0j},\qquad
\mathbf{P}^j_{-} = \mathbf{F}_j\,\mathbf{P}^{0j}\,\mathbf{F}_j^\top + \mathbf{Q}_j$$

Written out for MOVE (as in the code):

$$\begin{aligned}
P_{00} &= P^0_{00} + \Delta t\,(P^0_{01} + P^0_{10}) + \Delta t^2 P^0_{11} + q_M \tfrac{\Delta t^3}{3} \\
P_{01} &= P_{10} = P^0_{01} + \Delta t\,P^0_{11} + q_M \tfrac{\Delta t^2}{2} \\
P_{02} &= P_{20} = P^0_{02} + \Delta t\,P^0_{12} \\
P_{11} &= P^0_{11} + q_M\,\Delta t \\
P_{12} &= P_{21} = P^0_{12} \\
P_{22} &= P^0_{22} + q_k\,\Delta t
\end{aligned}$$

### 7.3 Update (per model, only if a fresh sample arrived)

Innovation and its covariance:

$$y_j = z' - \mathbf{H}_k\,\hat{\mathbf{x}}^j_{-},\qquad S_j = \mathbf{H}_k\,\mathbf{P}^j_{-}\,\mathbf{H}_k^\top + R$$

Kalman gain:

$$\mathbf{K}_j = \frac{\mathbf{P}^j_{-}\,\mathbf{H}_k^\top}{S_j}$$

**Foot stiffness is learned only in HOLD.** In MOVE, $k$ is a *consider* parameter (Schmidt–Kalman filter). It is
used in the prediction, but its gain is set to zero ($K_{M,2} = 0$), so the correlation between a press and the
pedal travel cannot corrupt it.

State update:

$$\hat{\mathbf{x}}^j = \hat{\mathbf{x}}^j_{-} + \mathbf{K}_j\,y_j$$

Covariance update in Joseph form, which stays valid for the partially zeroed Schmidt gain:

$$\mathbf{P}^j = (\mathbf{I} - \mathbf{K}_j\mathbf{H}_k)\,\mathbf{P}^j_{-}\,(\mathbf{I} - \mathbf{K}_j\mathbf{H}_k)^\top + \mathbf{K}_j\,R\,\mathbf{K}_j^\top$$

The diagonal terms are floored at $10^{-9}$ and $S_j$ at $10^{-12}$.

Gaussian likelihood of the innovation, evaluated in the log domain. The constant $-\tfrac12\ln 2\pi$ is dropped
because it cancels in the normalization:

$$\Lambda_j = \frac{1}{\sqrt{2\pi S_j}}\exp\!\left(-\frac{y_j^2}{2 S_j}\right)
\quad\Rightarrow\quad
\ell_j = -\tfrac12\left(\frac{y_j^2}{S_j} + \ln S_j\right)$$

Mode probability update. Subtracting $\ell_{\max} = \max_j \ell_j$ avoids float underflow:

$$\mu^j = \frac{e^{\ell_j - \ell_{\max}}\,\bar c_j}{\sum_i e^{\ell_i - \ell_{\max}}\,\bar c_i}$$

**Without a fresh sample**, only the Markov prior applies: $\mu^j = \bar c_j$.

### 7.4 Sample interval estimate

The time since the last fresh sample, $t_s$, is accumulated each cycle. On every fresh sample it updates the
averaged interval $T$ used in section 5.2:

$$T \leftarrow T + \alpha\,(t_s - T),\qquad \alpha = 0.05$$

### 7.5 Constraints

- **Foot stiffness:** physically non-negative and bounded, $k \in [0,\ 5000\ \text{kg/m}]$ (≈ 49 N/mm), with
  $P_{22} \le 1000^2$. The lower bound also separates intent from reaction. A press moves the pedal *with* the
  force change (positive correlation between $\Delta x$ and $z$), while a passive foot reaction moves it *against*
  it. With $k \ge 0$, HOLD cannot explain a press as foot reaction.
- **Mode probabilities:** each is kept at or above $\mu_{\min} = 10^{-3}$ and then renormalized, so the inactive
  model can win back within a few samples:

$$\mu^j \leftarrow \frac{\max(\mu^j, \mu_{\min})}{\sum_i \max(\mu^i, \mu_{\min})}$$

### 7.6 Output

Probability-weighted estimates:

$$\hat F = \sum_j \mu^j\,\hat F^j,\qquad \hat k = \sum_j \mu^j\,\hat k^j,\qquad
\hat{\dot F} = \sum_j \mu^j\,\hat{\dot F}{}^j \quad(\texttt{changeVelocity})$$

The force passed to the admittance model is the denoised measured force, plus an optional removal of the
self-caused foot reaction while holding:

$$F_{\text{out}} = \hat F - \hat k\,\Delta x + \operatorname{clamp}\!\left(\beta\,\mu^H\,\hat k\,\Delta x,\ \pm 0.5\ \text{kg}\right)
\quad(\texttt{filteredReading})$$

$\beta$ = `s_footReactionRemovalHold_fl32`, **0 by default**, so $F_{\text{out}} = \hat F - \hat k\,\Delta x$.

---

## 8. Why it separates noise from real presses

- **Holding still:** both models see zero-mean innovations. HOLD predicts more tightly ($S_H < S_M$), so every
  sample adds about $\tfrac12 \ln(S_M / S_H)$ to the log-likelihood ratio in favour of HOLD. $\mu^M$ decays toward
  $\mu_{\min}$, and the output follows the 1.5 Hz HOLD estimate.
- **Pressing:** a force ramp at rate $\dot F$ builds a lag in HOLD of about $\dot F / \omega_H$. Once $|y_H|$
  exceeds a few $\sqrt{S_H}$, the model with the larger $S$ (MOVE) explains the samples better. $\mu^M \to 1$
  within a few samples, and the output switches to the low-lag MOVE estimate.

---

## 9. Parameters

| Constant (in `SignalFilter_IMM.cpp`) | Value | Effect |
|---|---|---|
| `s_holdBandwidthHz_fl32` | 1.5 Hz | smoothing while holding; higher = less "sticky" |
| `s_moveBandwidthMinHz_fl32` / `MaxHz` | 5 / 60 Hz | MOVE bandwidth range covered by the slider |
| `s_holdDwellTime_s_fl32` | 0.3 s | expected HOLD duration; longer = fewer switches |
| `s_moveDwellTime_s_fl32` | 0.15 s | expected MOVE duration |
| `s_modeProbabilityMin_fl32` | 1e-3 | floor that keeps both models alive |
| `s_sampleIntervalFilterAlpha_fl32` | 0.05 | smoothing of the measured sample interval |
| `s_footStiffnessMax_kgPerM_fl32` | 5000 kg/m | upper bound of $k$ |
| `s_footStiffnessInitialStd_kgPerM_fl32` | 1000 kg/m | prior uncertainty of $k$ |
| `s_footStiffnessRandomWalk_fl32` | 200² (kg/m)²/s | how fast $k$ may change |
| `s_footReferenceTimeConstant_s_fl32` | 0.5 s | $\tau_{\text{ref}}$ of the foot reference in HOLD |
| `s_footMicroMotionMax_m_fl32` | 1 mm | clamp of $\Delta x$ |
| `s_inertiaDelayDefault_s_fl32` | 2 ms | $\tau$ without identification |
| `s_inertiaCompensationFraction_fl32` | **0** (experimental) | $\gamma$, section 11.1 |
| `s_footReactionRemovalHold_fl32` | **0** (experimental) | $\beta$, section 11.2 |
| `s_footReactionCorrectionMax_kg_fl32` | 0.5 kg | clamp of the $\beta$ correction |

`HOLD_DAMPING_RATIO` in `StepperMovementStrategy.h` is **0** (experimental, section 11.3).

---

## 10. Telemetry and tuning

While mode 4 is active, the extended pedal state field `oscillationMonitorValue_u8` carries $255\,\mu^M$. It
appears as the "Oscillation Monitor" signal in the live plot: 0 means HOLD, 255 means MOVE.

With the debug flag `DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT_U8` set, the firmware prints once per second:

```text
[IMM] k_foot=<k̂> kg/m, mu_hold=<μ_H>
```

| Symptom | Adjustment |
|---|---|
| Residual vibration while holding | lower `s_holdBandwidthHz_fl32` |
| Holding feels sticky or delayed on slow presses | raise `s_holdBandwidthHz_fl32` |
| Monitor flickers during slow, steady presses | lengthen both dwell times |
| Presses feel laggy | raise the slider (higher MOVE bandwidth) |

---

## 11. Experimental coupling to the admittance model (disabled)

### 11.1 Plate inertia compensation and startup identification

**Idea.** Remove the plate inertia force from the measurement ($\gamma > 0$ in section 3.3), which breaks the path
noise → motion → inertia force → motion.

**Identification** ([`PlateInertiaIdentification.cpp`](../../ESP32/src/PlateInertiaIdentification.cpp)). This runs
only if $\gamma > 0$, in mode 4, outside rudder mode, once after each homing, with no foot on the pedal. It takes
about 3.5 s. The pedal follows a two-tone trajectory on the pedal arc:

$$x(t) = A\,e(t)\,\bigl(1 - \cos\omega t\bigr),\qquad A = 3\ \text{mm}$$

$e(t)$ ramps in over the first period and out over the last. The tones are $f_1 = 3$ Hz (4 evaluated periods) and
$f_2 = 7$ Hz (8 evaluated periods). The arc offset is converted to sled travel with the local kinematic gain at
the rest position, $\partial s_{\text{arc}}/\partial x_{\text{sled}}$ from `pedalInclineAngleDeg`.

Lock-in demodulation over the evaluated periods, with $N$ fresh samples:

$$I = \frac{2}{N}\sum F\cos\omega t,\qquad Q = \frac{2}{N}\sum F\sin\omega t,\qquad
H(\omega) = \frac{I + jQ}{A\,\omega^2}$$

The model includes a gravity and linkage stiffness term $k_g$:

$$H(\omega) = \left(c_m - \frac{k_g}{\omega^2}\right)e^{\,j\omega\tau}$$

**Delay.** Taken from the higher tone, where inertia dominates. The sign $\sigma = \operatorname{sign}(\operatorname{Re}H_2)$
keeps $c_m$ signed, because the sign depends on the load cell mounting:

$$\tau = \frac{\operatorname{atan2}(\sigma\,\operatorname{Im}H_2,\ \sigma\,\operatorname{Re}H_2)}{\omega_2}$$

**Coefficient.** The signed apparent coefficient per tone is the projection onto the delay phasor. Two tones then
give two equations for $c_m$ and $k_g$:

$$m(\omega) = \operatorname{Re}\!\left(H\,e^{-j\omega\tau}\right) = c_m - \frac{k_g}{\omega^2}$$

$$c_m = \frac{m_2\,\omega_2^2 - m_1\,\omega_1^2}{\omega_2^2 - \omega_1^2},\qquad
k_g = \frac{m_2 - m_1}{1/\omega_1^2 - 1/\omega_2^2}$$

Here $c_m$ is in kg (force) per m/s², and the equivalent mass is $9.81\,c_m$.

**Plausibility checks** (otherwise the previous model is kept):
- at least 50 samples per tone
- $|\bar F| < 1$ kg (larger means a foot is on the pedal)
- residual variance $< 100\,R$
- $0.002 \le |c_m| \le 0.3$
- $0 \le \tau \le 8$ ms

**Simulation finding.** With $\gamma = 1$ and the plate mass *not* added to the virtual mass, the closed loop
diverged. The plate mass physically stabilizes the loop, and removing it leaves only the small virtual mass against
a stiff foot plus 2 ms of delay. With the plate mass moved into the virtual mass, hold jitter still got *worse*
(7.1 vs 2.7 µm at a 10 N/mm foot). The likely cause is that the model acceleration is derived from the noisy force,
so $c_m\,a_{\text{model}}$ injects noise the real, servo-filtered plate never produces. A usable version would at
least need (a) the removed mass added to the virtual mass and (b) the acceleration passed through a model of the
servo response.

### 11.2 Foot reaction removal ($\beta$)

**Idea.** While holding, remove part of the estimated passive foot reaction $\hat k\,\Delta x$ from the force
passed to the admittance model (section 7.6), so the pedal ignores the force changes it causes itself.

**Simulation finding.** $\beta = 0.5$ gave no measurable hold improvement. $\beta = 1.0$ made the loop unstable with
a 10 N/mm foot. The estimation of $\hat k$ itself runs regardless of $\beta$ and can be observed via the debug print.

### 11.3 HOLD damping ("statistical stiction")

**Idea.** In `MoveByAdmittanceStrategy`, add damping near standstill, weighted by the HOLD probability:

$$c_{\text{hold}} = \mu^H\,\kappa\,2\sqrt{m_v\,k_{\text{local}}}\;\max\!\left(0,\ 1 - \frac{|v|}{v_{\text{band}}}\right),\qquad v_{\text{band}} = 20\ \text{mm/s}$$

$\kappa$ = `HOLD_DAMPING_RATIO`, **0 by default**. It enters the Tustin integrator like all other damping.

**Simulation finding.** Mixed. Hold jitter improved slightly with a soft foot (3 N/mm) and not with a stiff one
(10 N/mm). Jitter after a press increased 3–8×.

### 11.4 Enabling for hardware experiments

Raise the constants one at a time, and watch the Oscillation Monitor and the `[IMM]` debug prints:

1. `HOLD_DAMPING_RATIO` (lowest risk)
2. `s_footReactionRemovalHold_fl32`, staying at or below 0.5
3. `s_inertiaCompensationFraction_fl32`, only together with adding the identified mass $9.81\,|c_m|$ to the
   virtual mass. That step is not implemented yet.

---

## 12. Simulation checks

These are simulation results from a Python port of the filter, not measurements on hardware.

**Open loop (filter only)**, from the 2-state version of the filter. Synthetic force, σ = 8 g load cell noise,
2 kHz load cell, 4 kHz loop:

| Slider | Holding still: output noise σ | MOVE detected after a 200 kg/s press starts | Slow ramp (4 kg/s): bias |
|---|---|---|---|
| 1 | 0.4–0.8 g | 1.0 ms | < 1 g |
| 128 | 0.3–1.2 g | 0.5 ms | < 1 g |
| 255 | 0.5–1.0 g | 0.5 ms | < 1 g |

**Closed loop**, 3-state version with defaults ($\gamma = \beta = \kappa = 0$). The plant has:
- an admittance model with $m_v$ = 0.3 kg, $k$ = 1500 N/m, ζ = 0.7
- a servo with 1.5 ms delay followed by a 40 Hz second-order response
- a plate with 0.5 kg equivalent mass
- a foot modelled as a spring-damper that adapts slowly

Metric: RMS pedal jitter after removing an 80 ms moving average, while holding 3 kg.

| Foot stiffness | 30 Hz low-pass (baseline) | IMM |
|---|---|---|
| 3 N/mm | 2.1 µm | 0.8 µm |
| 10 N/mm | 4.1 µm | 2.7 µm |

**Closed loop, plant calibrated to a real trace** (`DiyFfbPedalStateLog_Throttle_Wired20260929_055517`, 5 throttle
presses). Identified from the trace:

| Plant parameter | Value | Source |
|---|---|---|
| Servo position loop / velocity feed-forward / velocity loop | 40 /s, 0.40, 400 /s | fit of logged servo error (95 % of its variance) |
| Servo following lag | 14–16 ms | error ÷ target speed |
| Servo state over Modbus | 93 Hz, ~4 ms latency | log |
| Fresh load cell samples | ~556 /s, σ = 10 g | log |
| Force curve | 5.8 N + 1348 N/m · x, 17.1 mm travel | inferred from the logged model states |
| Virtual mass / base damping | 0.57 kg / 43.45 Ns/m | log |

Validation: driven with the logged force, the simulated admittance model and servo reproduce the logged pedal
position within 0.65 mm RMS (3.8 % of travel) and the logged damping within 9 %.

Jitter while holding 1.5 kg at mid-travel (µm RMS, high-passed). The baseline low-pass is set to 80 Hz, which
matches the noise reduction of the logged filter (10 g → 5.7 g):

| Foot | Servo velocity FF | No filter | Baseline | IMM |
|---|---|---|---|---|
| 3 N/mm | 0.40 (current) | 2.7 | 2.8 | 0.6 |
| 3 N/mm | 0.70 | 2.9 | 3.1 | 0.7 |
| 3 N/mm | 0.90 | 3.1 | 3.2 | 0.8 |
| 10 N/mm | 0.40 (current) | 3.2 | 3.6 | 4.3 |
| 10 N/mm | 0.70 | 3.3 | 3.8 | 1.4 |
| 10 N/mm | 0.90 | 3.4 | 4.0 | 1.3 |

With the current 14–16 ms servo lag and a stiff foot, the IMM is slightly worse than the baseline. Once the servo
follows faster, it is about 3× better. The servo retune and the IMM filter complement each other.

**Limits of this simulation.** The foot model is crude. It does not reproduce the vibration felt on the real
pedal, and its press response is dominated by the foot's own adaptation, so rise-time numbers are not meaningful.
Treat the results as a check of direction and stability, not of feel.

---

## References

- H. A. P. Blom, Y. Bar-Shalom, *The Interacting Multiple Model Algorithm for Systems with Markovian Switching
  Coefficients*, IEEE Transactions on Automatic Control, 33(8), 1988.
- Y. Bar-Shalom, X. R. Li, T. Kirubarajan, *Estimation with Applications to Tracking and Navigation*, Wiley, 2001
  (chapter 11: IMM; chapter 6: discretized white-noise models).
- S. F. Schmidt, *Application of State-Space Methods to Navigation Problems*, Advances in Control Systems, 3, 1966
  (consider parameters / Schmidt–Kalman filter).
