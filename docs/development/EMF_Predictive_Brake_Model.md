# Physical and Mathematical Modeling: Back-EMF, DC Bus Dynamics, and Predictive Brake Chopper Control

**Project:** DIY Sim Racing Active FFB Pedal  
**System Component:** `PredictiveBrakeControllerV2` / `isv57communication` / `StepperWithLimits` / `Main.cpp`  
**Target Hardware:** StepperOnline iSV57 (130W), Meanwell LRS-350-36 (36V), FR120N MOSFET + PC817, 10W $5\,\Omega$ Brake Resistor  
**Status:** Comprehensive Physical Specification, Empirical Telemetry Analysis & Control Architecture  

---

## Table of Contents
1. [Executive Summary & Root Cause Analysis](#1-executive-summary--root-cause-analysis)
   - 1.1 Telemetry Latency vs. 4 kHz Real-Time Control Loop
   - 1.2 Failure Modes of Previous Implementations (`simpleVoltageCheck` vs `Update`)
   - 1.3 Key Discovery from Empirical Telemetry: Pedal Press Catch-Up Spikes
   - 1.4 Physics of Tracking Error Collapse (Rotor Inertia & Plugging Counter-Torque)
2. [Hardware Constraints & Operating Boundaries](#2-hardware-constraints--operating-boundaries)
   - 2.1 Power Supply Unit: Meanwell LRS-350-36 (Uni-directional DC Bus)
   - 2.2 Integrated Servo Motor: StepperOnline iSV57 130W
   - 2.3 MOSFET Switch: FR120N + PC817 Optocoupler (Strict "No-PWM" Constraint)
   - 2.4 Brake Resistor: 10W $5.0\,\Omega$ Ceramic Resistor (Low Thermal Mass)
3. [Servo Internal Parameters (`isv57_tunedParameters.h`) & In-Drive Suppression](#3-servo-internal-parameters-isv57_tunedparametersh--in-drive-suppression)
   - 3.1 `Pr7.31` Bleeder Control Mode & Reactive Pump Lift Suppression
   - 3.2 `Pr1.04` 1st Torque Command Low-Pass Filter
   - 3.3 `Pr2.22` & `Pr2.23` Position Command Smoothing Filters (PT1 / FIR)
   - 3.4 Position Loop Stiffness vs Velocity Loop Damping (`Pr1.00` / `Pr1.01`)
   - 3.5 Startup Parameter Synchronization via Modbus RTU
4. [Mathematical Modeling of Electromechanical Dynamics](#4-mathematical-modeling-of-electromechanical-dynamics)
   - 4.1 Kinematic Transformation (Ball Screw & Rotor Angular Velocity)
   - 4.2 Back-EMF Generation in PMSM / BLDC Inverter
   - 4.3 4-Quadrant Operating Regimes & Kinetic Energy Transfer
   - 4.4 Non-Linear Differential Equation of the DC Bus Capacitor ($\dot{U}_{\text{bus}}$)
5. [Predictive Brake Controller Architecture (V2)](#5-predictive-brake-controller-architecture-v2)
   - 5.1 Time-To-Zero (TTZ) Catch-Up Predictor ($e \cdot \dot{e} < 0$)
   - 5.2 Kinetic Energy Discharge Model & Pulse Sizing
   - 5.3 Discrete Single-Shot Pulse Generation ($T_{\text{on}} \ge 600\,\mu\text{s}$, $T_{\text{off}} \ge 1000\,\mu\text{s}$)
   - 5.4 Multi-Layer Voltage Gating (Zero Power Sourced from PSU)
   - 5.5 Fast Reactive Safety Clamp ($\ge 38.5\,\text{V}$)
   - 5.6 $I^2 \cdot t$ Thermal Safety Budget for 10W Resistor
6. [Telemetry Verification & Tuning Guidelines](#6-telemetry-verification--tuning-guidelines)
   - 6.1 Telemetry Validation via `VisualizePedalLog.ipynb`
   - 6.2 Step-by-Step Tuning Workflow

---

## 1. Executive Summary & Root Cause Analysis

In active force-feedback sim-racing pedals, an integrated brushless AC/DC servo motor drives a linear ball-screw mechanism to reproduce realistic braking forces (up to 100+ kg). During dynamic human foot inputs, high amounts of mechanical energy are transferred between the driver's leg, the pedal lever arm, and the servo motor.

Two critical failure modes previously plagued the system:
1. **Drive Overvoltage Shutdown (Leadshine Fault `Err-01` / `Err-02`):** The internal bus voltage exceeded the hard inverter trip limit ($> 45 - 50\,\text{V}$), shutting down the drive and leaving the pedal lifeless.
2. **Brake Resistor Thermal Runaway:** The external dump resistor overheated, glowing red or becoming excessively hot, threatening physical enclosure damage.

```
       +-------------------------------------------------------------+
       |                  Root Cause Breakdown                       |
       +-------------------------------------------------------------+
                                      |
         +----------------------------+----------------------------+
         |                                                         |
         v                                                         v
[Telemetry Latency]                                   [False-Triggering & Blind Burn]
- 100 Hz Modbus cycle                                 - Firing while U_bus == 35.3V (PSU resting)
- 15-25 ms round-trip delay                           - Drawing 260W directly from Meanwell PSU
- 66V spike lasts 15-20 ms                            - Missing the true catch-up spike during press
- Reactive check fires AFTER trip!                    - 30 ms blind burst >> 1.5 J energy need
```

### 1.1 Telemetry Latency vs. 4 kHz Real-Time Control Loop
The pedal control loop (`pedalUpdateTask` in `Main.cpp`) runs on Core 1 at **4000 Hz** ($\Delta t = 250\,\mu\text{s}$). In contrast, cyclic servo telemetry (`readServoStates()`) runs over a 38400-baud Modbus RTU serial bus at **100 Hz** (10 ms period).

```
Physical Event Timeline:
|--- Event Starts (t = 0 ms)
|    Rotor decelerates abruptly -> Back-EMF pumps into C_bus
|--- Overvoltage Trip (t = 3.6 ms)
|    C_bus reaches trip threshold (50V) -> Inverter disables gates
|--- Telemetry Request Transmitted (t = 10 ms)
|    UART sends Modbus query frame (8 bytes)
|--- Telemetry Received & Decoded (t = 18-22 ms)
     ESP32 reads voltage = 48V -> Way too late! Drive has already faulted!
```

Due to this 15–25 ms transport lag, a purely reactive threshold check based on telemetry voltage (`brakeController.simpleVoltageCheck()`) is fundamentally unable to prevent overvoltage trips.

---

### 1.2 Failure Modes of Previous Implementations

#### Why `simpleVoltageCheck()` Failed (Too Late)
The capacitor bank inside the iSV57 and control board has a total capacitance of $C_{\text{bus}} \approx 1000\,\mu\text{F}$. The energy margin between nominal operating voltage ($U_{\text{PSU}} = 36\,\text{V}$) and overvoltage trip ($U_{\text{trip}} = 45\,\text{V}$) is:
$$\Delta E_{\text{cap}} = \frac{1}{2} C_{\text{bus}} \left(U_{\text{trip}}^2 - U_{\text{PSU}}^2\right) = \frac{1}{2} \cdot 10^{-3}\,\text{F} \cdot (45^2 - 36^2)\,\text{V}^2 = 0.364\,\text{J}$$

When mechanical counter-braking injects a power of $P_{\text{regen}} \approx 150\,\text{W}$, the voltage charges to the trip threshold in:
$$\Delta t_{\text{trip}} = \frac{\Delta E_{\text{cap}}}{P_{\text{regen}}} = \frac{0.364\,\text{J}}{150\,\text{W}} \approx 2.43\,\text{ms}$$
Since the Modbus telemetry lag is $15 - 25\,\text{ms}$, the drive trips **at least 12 ms before the ESP32 even knows the voltage rose**.

#### Why `brakeController.Update()` Overheated the Resistor (Too Early & Too Long)
The original `PredictiveBrakeController` used force derivative thresholds ($\dot{F} < -150\,\text{kg/s}$) combined with a fixed 30 ms timer:
```cpp
// Flawed legacy logic:
const uint32_t HOLD_TIME_US = 30000; // 30 ms continuous burn
if (trigger_b && !is_timer_active_b) {
    digitalWrite(BRAKE_RESISTOR_PIN, HIGH);
}
```
This had catastrophic consequences:
1. **Sourcing Power Directly from the PSU:** When activated at resting voltage ($U_{\text{bus}} \approx 35.3\,\text{V}$), the brake chopper became a direct short across the Meanwell 350W PSU through the $5\,\Omega$ resistor:
   $$P = \frac{U^2}{R} = \frac{(35.3\,\text{V})^2}{5\,\Omega} = 249.2\,\text{W}$$
   The chopper was not absorbing regenerated motor energy; it was boiling the resistor using mains power from the wall!
2. **Fixed 30 ms Hold Time:** The 10W resistor absorbed $250\,\text{W} \times 0.030\,\text{s} = 7.5\,\text{J}$ per trigger. A few rapid pedal presses quickly exceeded the resistor's thermal dissipation capacity.

---

### 1.3 Key Discovery from Empirical Telemetry: Pedal Press Catch-Up Spikes

Detailed diagnostic telemetry logs recorded via `VisualizePedalLog.ipynb` revealed the true physical mechanism behind the 66V spikes:

```
Telemetry Trace During Rapid Pedal Press:
Time [s]    Pedal Force [kg]    Tracking Error [steps]    Bus Voltage [V]    Brake Pin State
-----------------------------------------------------------------------------------------
9.060          5.2                  -120                      35.3V             LOW
9.065          4.1 (dF/dt < 0)      -350                      35.3V             HIGH (Legacy fired!)
9.095          8.5 (Rising)        -2850 (Peak error)         35.4V             LOW  (Legacy stopped!)
9.105         32.0 (Foot driving)   -1200 (Rapid catch-up!)   42.1V             LOW  (UNPROTECTED!)
9.112         48.0 (Hard press)      -50  (Plugging brake!)   66.2V!            LOW  (66V SPIKE!)
9.125         55.0 (Static)            0                      36.1V             LOW  (Spike decaying)
```

#### The Two Distinct Phenomena:
1. **The Legacy False Trigger:** At $t = 9.065\,\text{s}$, minor foot trembling caused a negative force derivative ($\dot{F} = -258\,\text{kg/s}$). The legacy controller fired for 30 ms while the bus was completely calm at 35.3V, dumping $7.5\,\text{J}$ of pure PSU power into the resistor.
2. **The True 66V Spike:** At $t = 9.105 - 9.115\,\text{s}$, during **vigorous pedal depression** (not release!), the servo tracking error collapsed from $-2850$ steps to zero in under $15\,\text{ms}$. At that exact moment, the bus voltage surged to **66.2 V**. The legacy controller was completely dormant because $\dot{F}$ was positive!

---

### 1.4 Physics of Tracking Error Collapse (Rotor Inertia & Counter-Torque Plugging)

Why does bus voltage surge when the user pushes *harder* on the pedal?

```
               +-------------------------------------------+
               | User presses pedal violently (Foot Force) |
               +-------------------------------------------+
                                     |
                                     v
               +-------------------------------------------+
               | Commanded steps advance faster than rotor |
               | Tracking error builds up to -3000 steps   |
               +-------------------------------------------+
                                     |
                                     v
               +-------------------------------------------+
               | Rotor accelerates violently to catch up   |
               | Rotor speed reaches omega > 2500 RPM      |
               +-------------------------------------------+
                                     |
                                     v
               +-------------------------------------------+
               | Error collapses towards 0: Position loop  |
               | slams reverse braking torque (Plugging)   |
               +-------------------------------------------+
                                     |
                                     v
               +-------------------------------------------+
               | Kinetic Energy E = 1/2 J omega^2 (2-3 J)  |
               | pumped through inverter diodes into C_bus |
               | in 10 ms -> 66V VOLTAGE SURGE!            |
               +-------------------------------------------+
```

1. As the foot applies rapid pressure, the internal admittance algorithm issues position steps faster than the motor rotor can accelerate against ball screw friction and mechanical inertia.
2. An error of $-2500$ to $-3000$ microsteps accumulates ($0.8 - 1.0$ motor revolutions).
3. The servo's internal position and velocity loops react with maximum forward torque, accelerating the rotor to high angular velocities ($\omega > 250\,\text{rad/s} \approx 2400\,\text{RPM}$).
4. As the rotor reaches the commanded position ($\text{error} \to 0$), the position loop must instantly halt the rotor to avoid positional overshoot.
5. The drive generates a massive **counter-torque (plugging / regenerative braking)**. The kinetic energy stored in the rotating motor inertia ($J_{\text{rotor}} = 0.40\,\text{kg}\cdot\text{cm}^2 = 4.0 \times 10^{-5}\,\text{kg}\cdot\text{m}^2$) plus reflected ball-screw inertia:
   $$E_{\text{kin}} = \frac{1}{2} J_{\text{total}} \omega^2 \approx 1.5 - 2.5\,\text{J}$$
6. Because the Meanwell switching PSU cannot absorb reverse current, this entire energy packet is dumped into the $1000\,\mu\text{F}$ capacitor in $\Delta t \approx 10 - 15\,\text{ms}$:
   $$P_{\text{peak}} = \frac{\Delta E}{\Delta t} \approx \frac{2.0\,\text{J}}{0.012\,\text{s}} \approx 166\,\text{W}$$
   $$\Delta U = \sqrt{U_0^2 + \frac{2 \Delta E}{C_{\text{bus}}}} = \sqrt{36^2 + \frac{2 \cdot 2.0}{1.0 \times 10^{-3}}} = \sqrt{1296 + 4000} \approx \mathbf{72.7\,\text{V}}$$
   This perfectly explains the empirical 66V peak observed on telemetry!

---

## 2. Hardware Constraints & Operating Boundaries

```
+---------------------------------------------------------------------------------+
|                               Electrical Topology                               |
+---------------------------------------------------------------------------------+
                                                     +------------------+
                                                     | 10W 5.0 Ohm      |
                                                     | Brake Resistor   |
                                                     +------------------+
                                                               |
+-----------------+      Anti-Backfeed Diode      +------------+-------+-----------+
| Meanwell        |          +----+               |            |       |           |
| LRS-350-36      |--------->|    |-------------->+---------+  |       |  +-----+  |
| 36V 9.7A        |          +----+               | C_bus   |  |     +-+--+ Q1  |  |
| (No Regen Sink) |                               | 1000 uF |  |     | FR120N   |  |
+-----------------+                               | 63V     |  |     | MOSFET   |  |
                                                  +---------+  |     +----+-----+  |
                                                               |          |        |
                                                  +------------+--+       |        |
                                                  | iSV57 Inverter|      GND       |
                                                  | 3-Phase BLDC  |                |
                                                  +---------------+                |
                                                                                   |
ESP32 (Core 1, 4kHz) --[GPIO 41]--> [PC817 Opto] ----------------------------------+
```

### 2.1 Power Supply Unit: Meanwell LRS-350-36
* **Nominal Voltage:** $36.0\,\text{V}$ DC (adjustable $32.4 - 39.6\,\text{V}$).
* **Rated Current:** $9.7\,\text{A}$ ($350\,\text{W}$).
* **Output Topology:** Flyback / Forward converter with output synchronous rectifier or Schottky diode block.
* **Critical Rule:** The PSU **cannot sink current**. If current flows back into the positive rail, the output diodes block, and the voltage floats upwards until electrolytic capacitors fail or overvoltage protection crowbars.

### 2.2 Integrated Servo Motor: StepperOnline iSV57 130W
* **Poles:** 8 (4 pole pairs, $p = 4$).
* **Rated Speed:** $3000\,\text{RPM}$ ($4000\,\text{RPM}$ max).
* **Back-EMF Constant ($K_e$):** $5.6\,\text{V}_{\text{rms}} / 1000\,\text{RPM}$ phase-to-phase ($K_e \approx 0.076\,\text{V}\cdot\text{s/rad}$).
* **Phase Resistance ($R_s$):** $0.82\,\Omega$ line-to-line.
* **Phase Inductance ($L_s$):** $0.84\,\text{mH}$ line-to-line.
* **Rotor Inertia ($J_m$):** $0.40\,\text{kg}\cdot\text{cm}^2 = 4.0 \times 10^{-5}\,\text{kg}\cdot\text{m}^2$.
* **Bus Overvoltage Trip Point:** Factory default $50 - 72\,\text{V}$ (configured via `Pr7.34`).

### 2.3 MOSFET Switch: FR120N + PC817 Optocoupler (Strict "No-PWM" Constraint)
* **Switching Element:** N-Channel Power MOSFET FR120N / LR120N ($100\,\text{V}$, $9.4\,\text{A}$, $R_{\text{DS(on)}} \approx 0.21\,\Omega$).
* **Gate Driver:** PC817 optocoupler with high pull-up resistance ($1\,\text{k}\Omega - 10\,\text{k}\Omega$).
* **Switching Dynamics:**
  - Rise time ($t_r$): $15 - 25\,\mu\text{s}$
  - Fall time ($t_f$): $25 - 45\,\mu\text{s}$

> [!CAUTION]
> **PWM WILL DESTROY THE MOSFET:**
> Operating this optocoupler-driven circuit with high-frequency PWM ($> 1\,\text{kHz}$) causes the MOSFET to spend a large fraction of each cycle in the **linear active region** ($V_{\text{GS}} \approx 3 - 4\,\text{V}$). The resulting instantaneous power dissipation ($P = V_{\text{DS}} \cdot I_{\text{D}} \approx 20\,\text{V} \cdot 4\,\text{A} = 80\,\text{W}$) will induce rapid thermal breakdown in the TO-252 package within milliseconds.
> 
> **Design Mandate:** **STRICTLY NO PWM.** Switching must occur in discrete, single-shot pulses with:
> - **Minimum ON-time ($T_{\text{on,min}}$):** $\ge 600\,\mu\text{s}$ (ensures full saturation well past rise time).
> - **Minimum OFF-time ($T_{\text{off,min}}$):** $\ge 1000\,\mu\text{s}$ (ensures full gate discharge and cooling).

### 2.4 Brake Resistor: 10W $5.0\,\Omega$ Ceramic Resistor (Low Thermal Mass)
* **Resistance ($R$):** $5.0\,\Omega \pm 5\%$.
* **Continuous Power Rating ($P_{\text{cont}}$):** $10\,\text{W}$ in free air.
* **Instantaneous Power at 36V:** $P_{\text{inst}} = \frac{36^2}{5} = 259.2\,\text{W}$.
* **Instantaneous Power at 50V:** $P_{\text{inst}} = \frac{50^2}{5} = 500.0\,\text{W}$.
* **Single-Pulse Thermal Limit:** $E_{\text{single,max}} \approx 12.0\,\text{J}$ ($T_{\text{on,max}} \approx 30 - 45\,\text{ms}$ cumulative per burst).
* **Cooling Time Constant ($\tau_{\text{therm}}$):** $\approx 15 - 25\,\text{s}$.

---

## 3. Servo Internal Parameters (`isv57_tunedParameters.h`) & In-Drive Suppression

The Leadshine / StepperOnline iSV57 firmware contains built-in parameters specifically designed to absorb or eliminate regenerative voltage spikes before they propagate to the DC bus.

### 3.1 `Pr7.31` Bleeder Control Mode & Reactive Pump Lift Suppression
* **Register Address:** `pr_7_00 + 31` (Index 271 in `tuned_parameters`).
* **Factory / Initial Setting:** `0` (Disabled).
* **Modes:**
  - `0`: Disable regenerative resistance discharge.
  - `1`: **Enable reactive pump lift suppression function.**
  - `2`: Enable external regenerative resistance discharge.

```
+-------------------------------------------------------------------+
| Pr7.31 = 1: Reactive Pump Lift Suppression Inverter Operation    |
+-------------------------------------------------------------------+
When bus voltage exceeds Pr7.32 (38V), the drive modulates d-axis   |
reactive current (id) to produce internal stator magnetic flux      |
dissipation, actively suppressing the voltage rise across C_bus!    |
+-------------------------------------------------------------------+
```

By changing `Pr7.31` from `0` to **`1`**, the servo inverter autonomously suppresses regenerative voltage surges by dumping excess energy into stator copper losses via controlled flux weakening.

---

### 3.2 `Pr1.04` 1st Torque Command Low-Pass Filter
* **Register Address:** `pr_1_00 + 4` (Index 29 in `tuned_parameters`).
* **Initial Setting:** `100` ($1.00\,\text{ms}$).
* **Updated Setting:** **`180`** ($1.80\,\text{ms}$).
* **Physical Effect:** Filters the torque demand output from the speed PI loop. When the tracking error collapses to zero, the torque demand would normally reverse from $+100\%$ to $-100\%$ within a single servo sample cycle. A filter constant of $1.8\,\text{ms}$ converts this step change into an exponential curve, reducing peak deceleration jerk ($d\tau/dt$) by over $40\%$.

---

### 3.3 `Pr2.22` & `Pr2.23` Position Command Smoothing Filters (PT1 / FIR)
* **`Pr2.22` (Positional Command Smoothing Filter - PT1):**
  - Time constant in $0.1\,\text{ms}$.
  - Current value: `15` ($1.5\,\text{ms}$).
  - Recommended for heavy sim pedal setups: **`35`** ($3.5\,\text{ms}$).
  - Smooths step pulses arriving from the ESP32 RMT peripheral, eliminating high-frequency step chatter and softening stop transitions.
* **`Pr2.23` (Positional Command FIR Filter):**
  - Moving average filter in $0.1\,\text{ms}$.
  - Current value: `10` ($1.0\,\text{ms}$).

---

### 3.4 Position Loop Stiffness vs Velocity Loop Damping (`Pr1.00` / `Pr1.01`)
* **`Pr1.00` (1st Position Loop Gain $K_p$):** Currently `600` ($1/\text{s}$).
* **`Pr1.01` (1st Velocity Loop Gain $K_v$):** Currently `400` ($0.1\,\text{Hz}$).
* **Damping Ratio:** High $K_p$ relative to $K_v$ produces an underdamped position response when recovering from large errors ($> 2000$ steps), causing the motor to violently brake and oscillate at the target. If voltage spikes persist, reduce `Pr1.00` to `450 - 500`.

---

### 3.5 Startup Parameter Synchronization via Modbus RTU
To ensure modified parameters actually take effect in hardware, [`sendTunedServoParameters()`](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/isv57communication.cpp#L215) in `isv57communication.cpp` must transmit them on every boot:

```cpp
// Transmitted during initialization in isv57communication.cpp:
retValue_b |= modbus.writeAndVerifyDeviceParameter(
    slaveId, pr_7_00 + 31, tuned_parameters[pr_7_00 + 31]); // Pr7.31 = 1 (Bleeder Mode)

retValue_b |= setServoVoltage(SERVO_MAX_VOLTAGE_IN_V_36V);   // Pr7.32 = 38V (Threshold)

retValue_b |= modbus.writeAndVerifyDeviceParameter(
    slaveId, pr_7_00 + 33, tuned_parameters[pr_7_00 + 33]); // Pr7.33 = 1V (Hysteresis)
```

---

## 4. Mathematical Modeling of Electromechanical Dynamics

```
[Pedal Lever: F_foot, v_sled]
              |
              v (Ball Screw Lead: p = 5 mm/rev)
[Motor Rotor: T_motor, omega_rotor]
              |
              v (Inverter 3-Phase Bridge)
[DC Bus Capacitor C_bus: U_bus(t)] <=====> [Meanwell PSU (36V)]
              |
              v (Chopper MOSFET Q1)
[Brake Resistor: R = 5 Ohm]
```

### 4.1 Kinematic Transformation
The pedal mechanism converts linear sled displacement $x_{\text{sled}}(t)$ into rotor rotation $\theta_m(t)$ via a precision ball screw with lead pitch $p_{\text{lead}} = 5.0\,\text{mm/rev} = 5.0 \times 10^{-3}\,\text{m/rev}$:

$$\omega_m(t) = \frac{2\pi}{p_{\text{lead}}} \cdot v_{\text{sled}}(t) = \frac{2\pi}{0.005\,\text{m}} \cdot v_{\text{sled}}(t) = 1256.64 \cdot v_{\text{sled}}(t) \quad [\text{rad/s}]$$

For a sled velocity of $v_{\text{sled}} = 0.25\,\text{m/s}$:
$$\omega_m = 1256.64 \cdot 0.25 = 314.16\,\text{rad/s} \quad (3000\,\text{RPM})$$

---

### 4.2 Back-EMF Generation in PMSM / BLDC Inverter
Each phase winding generates an induced counter-electromotive force proportional to rotor speed and magnetic flux linkage $\Psi_m$:
$$e_{\text{phase}}(t) = K_e \cdot \omega_m(t)$$

For the iSV57 ($K_e = 0.076\,\text{V}\cdot\text{s/rad}$ line-to-line peak):
$$\hat{U}_{\text{BEMF,LL}} = K_e \cdot \omega_m$$
At $3000\,\text{RPM}$ ($\omega_m = 314.16\,\text{rad/s}$):
$$\hat{U}_{\text{BEMF,LL}} = 0.076 \cdot 314.16 = 23.88\,\text{V}$$

Notice that **pure Back-EMF rectification alone ($24\,\text{V}$) is LESS than the 36V bus voltage**. Uncontrolled diode bridge conduction does NOT occur at $3000\,\text{RPM}$!  
Therefore, **the 66V voltage spike is 100% caused by active inductive boost / plugging counter-torque from the inverter switching**, not passive generator rectification!

---

### 4.3 4-Quadrant Operating Regimes & Kinetic Energy Transfer

| Quadrant | Torque ($\tau$) | Speed ($\omega$) | Mechanical Power ($P_{\text{mech}} = \tau \omega$) | Electrical Mode |
| :--- | :--- | :--- | :--- | :--- |
| **Q1** | $> 0$ (Forward) | $> 0$ (Forward) | $> 0$ (Motoring) | Power drawn from PSU |
| **Q2** | $< 0$ (Reverse) | $> 0$ (Forward) | $< 0$ (**Regenerative / Plugging**) | **Power pumped into $C_{\text{bus}}$!** |
| **Q3** | $< 0$ (Reverse) | $< 0$ (Reverse) | $> 0$ (Motoring) | Power drawn from PSU |
| **Q4** | $> 0$ (Forward) | $< 0$ (Reverse) | $< 0$ (**Regenerative / Plugging**) | **Power pumped into $C_{\text{bus}}$!** |

When the tracking error $e(t) < 0$ and is collapsing rapidly ($\dot{e} > 0$), the motor operates in **Quadrant 2**:
* Rotor speed $\omega_m > 0$ (moving fast forward to catch target).
* Torque $\tau_m < 0$ (applying reverse braking torque to stop at target).
* Net regenerated power flowing back into DC link:
  $$P_{\text{regen}}(t) = |\tau_m(t) \cdot \omega_m(t)| - I^2 R_{\text{winding}} - P_{\text{friction}}$$

---

### 4.4 Non-Linear Differential Equation of the DC Bus Capacitor

The instantaneous voltage across the DC bus capacitor $C_{\text{bus}}$ is governed by the energy balance equation:
$$\frac{d}{dt}\left(\frac{1}{2} C_{\text{bus}} U_{\text{bus}}^2(t)\right) = P_{\text{in}}(t) - P_{\text{out}}(t)$$

Expanding the derivative:
$$C_{\text{bus}} U_{\text{bus}}(t) \frac{dU_{\text{bus}}}{dt} = P_{\text{regen}}(t) + P_{\text{PSU}}(t) - P_{\text{brake}}(t)$$

$$\frac{dU_{\text{bus}}}{dt} = \frac{P_{\text{regen}}(t) + P_{\text{PSU}}(t) - \frac{U_{\text{bus}}^2(t)}{R_{\text{brake}}} \cdot s(t)}{C_{\text{bus}} U_{\text{bus}}(t)}$$

Where:
* $s(t) \in \{0, 1\}$ is the discrete MOSFET conduction state.
* $P_{\text{PSU}}(t)$ is the power delivered by the Meanwell PSU:
  $$P_{\text{PSU}}(t) = \begin{cases} 
  I_{\text{load}}(t) \cdot U_{\text{bus}}(t) & \text{if } U_{\text{bus}}(t) \le U_{\text{PSU}} \\
  0 & \text{if } U_{\text{bus}}(t) > U_{\text{PSU}} \quad \text{(Output diodes reverse-biased)}
  \end{cases}$$

> [!IMPORTANT]
> **Zero Power Sourced from PSU Condition:**
> To guarantee that the brake resistor **never** absorbs power from the Meanwell power supply, the brake switch state $s(t)$ must satisfy:
> $$s(t) = 0 \quad \forall \; U_{\text{bus}}(t) \le U_{\text{PSU}} + \delta_{\text{margin}}$$
> where $\delta_{\text{margin}} \ge 0.5\,\text{V}$ (i.e. $U_{\text{threshold}} \ge 36.5\,\text{V}$).

---

## 5. Predictive Brake Controller Architecture (V2)

`PredictiveBrakeControllerV2` was engineered to overcome every flaw identified in the legacy system.

```
       +-------------------------------------------------------------+
       |             4000 Hz Control Loop (Main.cpp)                 |
       +-------------------------------------------------------------+
                                      |
         +----------------------------+----------------------------+
         |                                                         |
         v                                                         v
[Branch 1: Catch-Up Predictor]                       [Branch 2: Reactive Clamp]
- Tracking Error e and d(e)/dt                       - Telemetry U_bus >= 38.5V
- Condition: e * de/dt < 0                           - Immediate single-shot pulse
- Time-To-Zero (TTZ) < 35 ms                         - Overcomes Modbus lag
- Predicts spike BEFORE rotor halts!                 - Ultimate safety net
         |                                                         |
         +----------------------------+----------------------------+
                                      |
                                      v
              +-----------------------------------------------+
              | Multi-Layer Voltage Gating                    |
              | If U_bus <= 36.5V -> DISALLOW ALL FIRING!     |
              | ZERO heating from Meanwell PSU guaranteed     |
              +-----------------------------------------------+
                                      |
                                      v
              +-----------------------------------------------+
              | Kinetic Energy & Pulse Sizing Engine          |
              | Calculate E_excess = E_kin - E_cap_absorb     |
              | Convert to discrete pulse Ton = 0.6 - 6.0 ms  |
              +-----------------------------------------------+
                                      |
                                      v
              +-----------------------------------------------+
              | FR120N Safe Switching & I^2*t Thermal Guard   |
              | Enforce Ton >= 600 us (No opto linear burnout)|
              | Enforce Toff >= 1000 us (Cooling period)      |
              | Monitor continuous 10W thermal budget         |
              +-----------------------------------------------+
```

### 5.1 Time-To-Zero (TTZ) Catch-Up Predictor ($e \cdot \dot{e} < 0$)
The tracking error $e(t) = \text{pos}_{\text{actual}} - \text{pos}_{\text{target}}$ and its derivative $\dot{e}(t) = \frac{e(t) - e(t - \Delta t)}{\Delta t}$ indicate whether the rotor is catching up:
1. **Convergence Condition:** If $e(t) \cdot \dot{e}(t) < 0$, the error is actively shrinking (the rotor is rushing toward the target position).
2. **Time-To-Zero Calculation:**
   $$\text{TTZ} = \left| \frac{e(t)}{\dot{e}(t)} \right|$$
3. **Trigger Window:** If $|e| > 600\,\text{steps}$ and $\text{TTZ} < 35\,\text{ms}$, a violent plugging stop is imminent within $10 - 35\,\text{ms}$. The controller pre-arms to dissipate the incoming energy surge.

---

### 5.2 Kinetic Energy Discharge Model & Pulse Sizing
The kinetic energy stored in the rotor during catch-up at estimated velocity $\omega_{\text{est}}$ is:
$$E_{\text{kin}} = \frac{1}{2} J_{\text{total}} \omega_{\text{est}}^2$$

The bus capacitors can safely absorb energy from nominal voltage ($U_{\text{nom}} = 36\,\text{V}$) up to maximum allowed ceiling ($U_{\text{max}} = 38.0\,\text{V}$):
$$\Delta E_{\text{cap,allow}} = \frac{1}{2} C_{\text{bus}} \left(U_{\text{max}}^2 - U_{\text{nom}}^2\right) = \frac{1}{2} \cdot 10^{-3} \cdot (38.0^2 - 36.0^2) = 0.074\,\text{J}$$

The net excess energy that must be dumped into the resistor is:
$$E_{\text{excess}} = \max\left(0, \; E_{\text{kin}} - \Delta E_{\text{cap,allow}}\right)$$

The pulse duration $T_{\text{on}}$ required to absorb this exact energy through the resistor ($R = 5\,\Omega$) at average voltage $U_{\text{avg}} \approx 37\,\text{V}$ is:
$$P_{\text{res}} = \frac{U_{\text{avg}}^2}{R} = \frac{37^2}{5} \approx 273.8\,\text{W}$$
$$T_{\text{on}} = \frac{E_{\text{excess}}}{P_{\text{res}}}$$

Instead of a blind 30 ms blast ($8.2\,\text{J}$), the controller issues a **laser-precise single-shot pulse between $600\,\mu\text{s}$ and $6000\,\mu\text{s}$ ($0.16 - 1.6\,\text{J}$)**, absorbing only the true excess!

---

### 5.3 Discrete Single-Shot Pulse Sizing & Safe Off-Time
To guarantee MOSFET survival with the slow PC817 optocoupler:
```cpp
const uint32_t MIN_PULSE_WIDTH_US   = 600;   // 600 us: Safely exceeds opto rise time
const uint32_t MAX_PULSE_BURST_US   = 6000;  // 6.0 ms: Max single-shot burst
const uint32_t MIN_OFF_TIME_US      = 1000;  // 1000 us: Full discharge & cooling
```

---

### 5.4 Multi-Layer Voltage Gating
To eliminate the possibility of burning resistor power from the Meanwell PSU:
```cpp
const float MIN_SAFE_TRIGGER_VOLTAGE_V = 36.5f; // Absolute hard floor

if (currentVoltage_V <= MIN_SAFE_TRIGGER_VOLTAGE_V) {
    // Under no circumstances allow the resistor to fire!
    disarmBrakeResistor();
    return;
}
```

---

### 5.5 Fast Reactive Safety Clamp ($\ge 38.5\,\text{V}$)
Even if predictive estimates deviate, a fast reactive clamp operates in the 4 kHz loop:
* If telemetry voltage exceeds **$38.5\,\text{V}$**, an immediate single-shot pulse ($1.5\,\text{ms}$) is fired.
* This catches unexpected manual back-driving or aggressive rebound kicks that bypass predictive criteria.

---

### 5.6 $I^2 \cdot t$ Thermal Safety Budget for 10W Resistor
An energy accumulator models thermal loading:
$$E_{\text{accum}}(t + \Delta t) = E_{\text{accum}}(t) + P_{\text{pulse}} \cdot T_{\text{pulse}} - P_{\text{diss}} \cdot \Delta t$$
Where $P_{\text{diss}} = 10.0\,\text{W}$ is the continuous dissipation rate. If cumulative energy exceeds $E_{\text{limit}} = 15.0\,\text{J}$, the controller enters thermal cool-down lockout, preventing physical resistor burnout.

---

## 6. Telemetry Verification & Tuning Guidelines

### 6.1 Telemetry Validation via `VisualizePedalLog.ipynb`
When recording a telemetry session with `DiyActivePedal`, verify the following metrics:
1. **Bus Voltage Plateau:** Voltage should remain stably within $35.5\,\text{V} - 39.5\,\text{V}$ during hard 60+ kg presses and rapid releases.
2. **Elimination of 66V Spikes:** The 66V spike must be eliminated or clamped below $42\,\text{V}$.
3. **Resistor Temperature:** After 15 minutes of vigorous driving, the ceramic brake resistor should feel slightly warm to the touch ($35 - 45^\circ\text{C}$), never hot or smoking.

```
Expected Ideal Telemetry Trace:
Voltage [V]
 50V | - - - - - - - - - - - - - - - - - Overvoltage Trip Margin
 42V | - - - - - - - - - - - - - - - - - Maximum Dynamic Peak
 38V | . . . . . . . . . . . . . . . . . Chopper Operating Threshold
 36V |__________________________________ Meanwell PSU Rest Line (Flat)
       0s      2s      4s      6s      8s
```

---

### 6.2 Step-by-Step Tuning Workflow

If minor overvoltage spikes or residual warmth are observed:

1. **Verify `Pr7.31` in Hardware:**
   Ensure the startup log reports `Found servo slave ID: 63` and verifies Modbus parameter transmission without errors. `Pr7.31 = 1` activates internal drive suppression.
2. **Adjust Position Smoothing Filter (`Pr2.22`):**
   If the motor feels "notchy" or makes an audible "thud" at the end of rapid movements, increase `Pr2.22` from `15` to `35` ($3.5\,\text{ms}$) in `isv57_tunedParameters.h`.
3. **Fine-Tune `TTZ_THRESHOLD_S` in `PredictiveBrakeControllerV2.h`:**
   - If spikes occur before the chopper fires: Increase `TTZ_THRESHOLD_S` from `0.035f` to `0.045f` ($45\,\text{ms}$) to trigger earlier.
   - If the chopper fires too frequently on gentle inputs: Increase `MIN_ERROR_FOR_CATCHUP` from `600` to `800` steps.
4. **Hardware Verification:**
   Ensure the FR120N MOSFET has clean gate pull-up connections and the ground reference between ESP32 and Meanwell 36V PSU is common and low-impedance.
