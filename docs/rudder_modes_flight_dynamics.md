# Flight Rudder & Anti-Torque Pedal Dynamics: Real-World Aviation Principles & FFB Implementation Guide

This document provides a comprehensive theoretical and physical breakdown of rudder and anti-torque pedal systems in real-world aircraft (fixed-wing airplanes and rotary-wing helicopters). It establishes the physical and mathematical foundations required to implement realistic Force Feedback (FFB) modes, admittance control algorithms, and dual-pedal synchronization over ESP-NOW for the DIY Active Pedal.

---

## 1. Executive Summary & Fundamental Kinematics

### 1.1 The Mechanical Push-Pull Constraint
In virtually all real-world manned aircraft, the left and right rudder/anti-torque pedals are **rigidly interconnected** through a mechanical linkage (push-pull tubes, cable-and-pulley networks, or a central pivoting teeter-totter bar).

```text
                      [ Aircraft Yaw Axis / Rudder Surface ]
                                     ▲
                                     │  (Push-Pull Linkage / Cable / Fly-By-Wire)
                                     │
           [ Left Pedal ] ───► ───[ Pivot ]─── ◄─── [ Right Pedal ]
             (Push Forward)                         (Retracts Aft)
```

* **Anti-Symmetric Displacement**: Pushing the **left pedal forward** forces the **right pedal backward** by an equal and opposite displacement:
  $$\Delta x_{\text{right}} = -\Delta x_{\text{left}}$$
* **Dual-Pedal Resistance (Fighting Oneself)**: If a pilot pushes both feet forward simultaneously with equal force ($F_L = F_R$), the mechanical linkage prevents any net travel ($\Delta x = 0$). The pilot simply feels the rigid structural stiffness of the pedal chassis.
* **Single Degree of Freedom (Yaw)**: In contrast to sim racing (where throttle, brake, and clutch operate as independent 1-DOF mechanisms), aviation rudder pedals act as **one shared differential degree of freedom** distributed across two foot-plates.

---

### 1.2 Mechanical Linkage Emulation & Mutual Lock
In a real aircraft cockpit, rudder and anti-torque pedals are mechanically slaved together via pushrods, torque tubes, or cables:
* **Anti-Symmetric Push-Pull Motion**: Pushing the **left pedal forward** forces the **right pedal backward** by the exact same distance ($x_R = 1.0 - x_L$).
* **Rigid Mutual Lock**: Attempting to push **both pedals forward simultaneously** is blocked by a solid 1200 N virtual barrier, exactly replicating a rigid mechanical teeter-totter bar.
* **Pure Rudder Axis**: The pedals act as a single, differential yaw-control degree of freedom distributed across two footplates.

---

## 2. Fixed-Wing Aircraft Rudder Systems (Planes)

Fixed-wing rudder control dynamics differ fundamentally depending on the flight control architecture: **Mechanical Reversible Controls**, **Hydraulically Boosted / Irreversible Controls**, and **Fly-By-Wire (FBW)**.

```mermaid
graph TD
    A["Fixed-Wing Rudder Architectures"] --> B["1. Reversible / Mechanical<br/>(General Aviation / Light Aircraft)"]
    A --> C["2. Irreversible / Hydromechanical<br/>(Transports & Commercial Jets)"]
    A --> D["3. Fly-By-Wire (FBW)<br/>(Airbus, Modern Fighters, B777/787)"]

    B --> B1["Aerodynamic Hinge Moments Felt Directly<br/>Q-Feel: Force scales with Airspeed² (½ρV²)<br/>Zero-speed floppiness on ground<br/>Mechanical trim tab shifts neutral angle"]
    
    C --> C1["Hydraulic PCUs drive rudder<br/>Artificial Feel Unit (AFU) generates force<br/>Dynamic Q-Feel scheduling<br/>Variable Rudder Travel Limiters at high IAS"]
    
    D --> D1["Electronic Flight Control System (EFCS)<br/>Spring-damper feel unit with cross-cockpit link<br/>Auto-yaw coordination & Dutch roll damping<br/>Pedals used mainly for crosswind decrab & engine-out"]
```

---

### 2.1 Reversible / Mechanical Control Systems (General Aviation)
*Found in: Cessna 172/182, Piper PA-28, Beechcraft Bonanza, aerobatic aircraft, gliders.*

In mechanical systems, the pilot's feet are connected directly to the vertical rudder surface via stainless steel control cables or aluminum push-pull torque tubes.

#### Physical Characteristics:
1. **Dynamic Pressure Scaling ($Q$-Feel)**:
   The aerodynamic resistance felt at the pedals is directly proportional to the dynamic pressure $q = \frac{1}{2} \rho V^2$:
   $$F_{\text{aero}} \propto q \cdot S_{\text{rudder}} \cdot c_{\text{chord}} \cdot C_{h}(\delta_r, \beta)$$
   * **At 0 knots (on the ramp / hangar)**: The pedals feel very light, loose, and floppy with almost no centering force (only light internal return bungees or linkage friction).
   * **At high airspeed ($V_{\text{IAS}}$)**: The aerodynamic forces push strongly against deflection. Deflecting the rudder to maximum angle requires high foot effort (e.g., 200–500 N).
2. **Natural Aerodynamic Self-Centering**:
   When the pilot removes foot pressure in flight, the airflow over the vertical stabilizer naturally blows the rudder surface back into the slipstream ($C_{h\alpha} \cdot \beta + C_{h\delta} \cdot \delta_r = 0$), forcing the pedals back to the aerodynamic neutral point.
3. **Breakout Force & Friction**:
   Cable guide pulleys, fairleads, and hinge bearings introduce a baseline static breakout friction ($\approx 10\text{--}25\text{ N}$) that prevents small control flutter around zero deflection.
4. **Mechanical Trim Tabs**:
   A small movable tab at the trailing edge of the rudder surface deflects into the airflow, creating an aerodynamic moment that holds the entire rudder at a trimmed angle $\delta_{\text{trim}}$. This physically shifts the zero-force resting position of the pedals.
5. **Slipstream & P-Factor Asymmetry**:
   In propeller-driven aircraft, the clockwise-rotating prop creates a helical slipstream swirling around the fuselage and striking the left side of the vertical fin. During high-power / low-speed climb, the pilot must maintain continuous **right rudder pressure** unless trimmed out.

---

### 2.2 Hydraulically Boosted / Irreversible Systems
*Found in: Boeing 737 / 747 / 767, Bombardier CRJ, classic jet transports.*

In irreversible systems, hydraulic Power Control Units (PCUs) move the rudder surface. Because hydraulic actuators isolate the pilot from aerodynamic surface forces, an **Artificial Feel Unit (AFU)** must synthesize pedal feedback.

#### Physical Characteristics:
1. **Artificial Feel Unit (AFU) / Q-Feel Computer**:
   The AFU uses centering springs, cam profiles, and hydraulic/pneumatic pistons fed by pitot dynamic pressure ($q$) to simulate airspeed-dependent stiffness.
2. **Rudder Travel Limiter / Rudder Ratio Changer**:
   * At low speeds (takeoff/landing, $< 150\text{ kts}$), full rudder travel ($\pm 25^\circ\text{ to }\pm 30^\circ$) is available for crosswind control and engine failure yaw authority.
   * At high speeds ($> 250\text{ kts}$), aerodynamic loads would snap the vertical tail off if full deflection were applied. A mechanical limiter or hydraulic ratio changer restricts maximum pedal travel to as little as $\pm 2^\circ\text{ to }\pm 4^\circ$, creating **dynamically moving hard endstops**.
3. **Yaw Damper Interaction**:
   A yaw damper servo automatically injects rudder corrections to suppress Dutch roll oscillations. In some aircraft (e.g. B737), yaw damper inputs move the rudder surface without moving the pedals (series yaw damper); in others, pedals may slightly backdrive.
4. **Pedestal Rudder Trim**:
   A rotary trim knob on the center pedestal drives an electric trim actuator that physically shifts the neutral center of the AFU spring mechanism, moving both pedals to a new hands/feet-off equilibrium position.

---

### 2.3 Fly-By-Wire (FBW) Aircraft
*Found in: Airbus A320 / A330 / A350, Boeing 777 / 787, F-16, F/A-18, Rafale.*

In modern FBW aircraft, pedal positions are read by Rotary Variable Differential Transformers (RVDTs) and processed by Flight Control Computers (FCCs).

#### Physical Characteristics:
1. **Spring-Loaded Centering Gradient**:
   The pedals use a dual-rate or non-linear progressive spring feel unit. Centering force is constant or scheduled by flight mode:
   $$F_{\text{pedal}}(x) = F_{\text{breakout}} \cdot \text{sgn}(x) + K_1 \cdot x + K_2 \cdot x^3 + D \cdot \dot{x}$$
2. **Cross-Cockpit Mechanical Interconnect**:
   In Airbus aircraft, although the sidesticks are not mechanically linked between Captain and First Officer, the **rudder pedals ARE mechanically interconnected** across the cockpit via push-pull rods. If the Captain pushes left, the First Officer's left pedal moves forward and right pedal moves back.
3. **Flight Control Laws (Auto-Yaw Coordination)**:
   During normal flight, the fly-by-wire system automatically handles turn coordination and sideslip compensation. Pilots keep their feet flat on the floor during cruise. Rudder pedals are actively used only for:
   * Crosswind runway align / decrab during flare.
   * Engine failure (asymmetric thrust compensation).
   * Nosewheel steering during low-speed ground taxi ($< 20\text{ kts}$ via pedal steering ratio).

---

## 3. Rotary-Wing Aircraft Anti-Torque Systems (Helicopters)

Helicopter pedals control the pitch of the tail rotor blades (or Fenestron / NOTAR airflow) to counteract main rotor torque and provide yaw heading control.

> [!IMPORTANT]
> **The Fundamental Helicopter Difference: Non-Centering & Continuous Offset.**
> Unlike an airplane rudder (which naturally wants to streamline to aerodynamic center), a helicopter tail rotor requires **continuous non-zero thrust** to prevent the fuselage from spinning uncontrollably due to main rotor torque. 
> Therefore, helicopter pedals **do not snap back to a mechanical center** during normal operation.

```mermaid
graph TD
    A["Helicopter Anti-Torque Architectures"] --> B["1. Unaugmented / Light Helicopters<br/>(Robinson R22/R44, Bell 206, Cabri G2)"]
    A --> C["2. Force Trim / SAS Helicopters<br/>(UH-60 Blackhawk, EC135/H145, AW139)"]

    B --> B1["No return-to-center spring<br/>Adjustable friction damper holds position<br/>Pedal stays wherever the pilot places it<br/>Pilot constantly modulates torque offset"]

    C --> C1["Force Trim System with Magnetic Clutch<br/>Spring centering around a dynamic trim anchor<br/>Force Trim Release (FTR) button decouples spring<br/>Beep Trim hat switch slews neutral anchor point"]
```

---

### 3.1 Light / Unaugmented Helicopters (Friction-Held)
*Found in: Robinson R22 / R44, Bell 206 JetRanger, Schweizer 300, Guimbal Cabri G2.*

#### Physical Characteristics:
1. **Zero Centering Spring (Pure Friction / Damping)**:
   There is **no centering spring** pulling the pedals toward 50% travel. If the pilot takes their feet off the pedals, the pedals stay exactly where they were left (or drift slowly due to tail rotor aerodynamic feedback).
2. **Mechanical Friction Adjuster**:
   A mechanical friction knob on the cockpit floor applies a clamping friction force:
   $$F_{\text{resist}} = F_{\text{friction}} \cdot \text{sgn}(\dot{x}) + D \cdot \dot{x}$$
   This damps out foot tremors and prevents blade pitch forces from back-driving the pedals.
3. **Hover vs. Cruise Trim Positions**:
   * **In Hover**: Counteracting main rotor torque requires high tail rotor pitch. In counter-clockwise rotor systems (American helicopters like Robinson, Bell, Sikorsky), **significant left pedal** is held continuously.
   * **In High-Speed Forward Cruise**: The vertical tail fin unloads the tail rotor aerodynamically; the pedal position moves closer to neutral or slightly right.
   * **During Autorotation (Engine Failure)**: Rotor torque disappears instantly. The pilot must immediately stomp **full right pedal** to prevent violent left yaw.

---

### 3.2 Medium / Heavy Turbine Helicopters (Force Trim & SAS)
*Found in: Sikorsky UH-60 Black Hawk / S-70, Airbus Helicopters H135 / H145 / H225, Leonardo AW139, Boeing CH-47 Chinook.*

Advanced helicopters use hydraulic boosters and an electro-mechanical **Force Trim System** to provide artificial centering feel without fatiguing the pilot.

```text
       [ Spring Feel Unit ] ──► [ Magnetic Brake / Electro-Clutch ] ──► (Chassis Frame)
               │                                ▲
               ▼                                │  (Engaged = Rigid Anchor)
         [ Left Pedal ]                         │  (FTR Pressed = Free Float)
                                                ▼
                                    [ Pilot Cyclic/Pedal FTR Switch ]
```

#### Physical Characteristics:
1. **Magnetic Brake & Spring Gradient**:
   The pedal is connected to a centering spring pack whose base is anchored to an electro-magnetic clutch (magnetic brake). When the brake is energized, the spring provides a centering force around the current anchor point $x_{\text{trim}}$:
   $$F_{\text{pedal}} = -K_{\text{trim}} \cdot (x - x_{\text{trim}}) - D \cdot \dot{x}$$
2. **Force Trim Release (FTR) Workflow**:
   * In flight, the pilot wants to establish a new hover heading or airspeed.
   * The pilot presses and holds the **FTR (Force Trim Release)** button on the cyclic grip or pedal.
   * The magnetic brake de-energizes instantly $\rightarrow$ Spring base floats freely $\rightarrow$ Centering resistance drops to zero ($K_{\text{trim}} \to 0$).
   * The pilot repositions the pedals with light effort to the desired heading.
   * The pilot releases the FTR button $\rightarrow$ Magnetic brake locks instantly $\rightarrow$ The **new pedal position becomes the new zero-force center** ($x_{\text{trim}} \leftarrow x_{\text{current}}$).
3. **Beep Trim (4-Way Hat Switch)**:
   For fine adjustments in cruise, the pilot pushes a 4-way beep trim switch on the cyclic. A small electric trim actuator slowly slews the magnetic brake anchor position at a constant velocity:
   $$\frac{dx_{\text{trim}}}{dt} = \pm v_{\text{slew}} \quad (\approx 1\text{ to }3\% \text{ travel / sec})$$
4. **Yaw Stability Augmentation System (Yaw SAS)**:
   High-bandwidth hydraulic smart actuators inject micro-corrections ($\pm 5\text{ to }10\%$ authority) to stabilize yaw rate without back-driving the pilot's pedals.

---

## 4. Comprehensive Comparison: Airplanes vs. Helicopters

| Parameter | GA Mechanical Plane | FBW / Commercial Airliner | Light Helicopter (R22/R44) | Turbine Helicopter (Force Trim) |
|---|---|---|---|---|
| **Dual-Pedal Linkage** | Rigid Push-Pull ($x_R = -x_L$) | Rigid Push-Pull ($x_R = -x_L$) | Rigid Push-Pull ($x_R = -x_L$) | Rigid Push-Pull ($x_R = -x_L$) |
| **Centering Behavior** | Aerodynamic self-centering ($q \propto V^2$) | Artificial spring centering | **No centering** (stays where placed) | Centered around **dynamic magnetic brake anchor** |
| **Airspeed ($V_{\text{IAS}}$) Influence** | High (floppy at $0\text{ kts}$, stiff at $V_{\text{ne}}$) | High (Q-feel + travel limiters) | Negligible on pedal stiffness | Negligible on pedal stiffness |
| **Breakout Force** | Low to moderate ($10\text{--}25\text{ N}$) | High, distinct detent ($30\text{--}60\text{ N}$) | Near zero ($0\text{--}5\text{ N}$) | Moderate around trim point ($15\text{--}30\text{ N}$) |
| **Trim Mechanism** | Aerodynamic tab (shifts aero neutral) | Pedestal trim wheel (shifts spring) | Hand friction nut (holds pedal) | **FTR Button** (instant snap-anchor) + Beep Trim |
| **Toe Brakes** | Yes (differential main wheels) | Yes (differential main wheels) | **None** | **None** (or parking brake on wheeled helis) |
| **Engine-Out Behavior** | Large asymmetric yaw toward dead engine | Asymmetric yaw; trim required | Total loss of torque (stomp opposite pedal) | Total loss of torque (stomp opposite pedal) |

---

## 5. Mathematical Control Models for Active FFB Pedals

To translate real-world aviation physics into firmware, we define four core operational control modes built upon the 4 kHz Admittance Physics Engine.

```text
                      ┌──────────────────────────────────────────────┐
                      │            Active Pedal Firmware             │
                      │       4 kHz Admittance Controller           │
                      └──────────────────────┬───────────────────────┘
                                             │
             ┌───────────────────────────────┼───────────────────────────────┐
             ▼                               ▼                               ▼
    [ Fixed-Wing Plane ]            [ Light Helicopter ]            [ Turbine Helicopter ]
    ┌──────────────────────┐        ┌──────────────────────┐        ┌──────────────────────┐
    │ Mode 1: Aero Q-Feel  │        │ Mode 2: Pure Friction│        │ Mode 3: Force Trim   │
    │ Mode 4: FBW Constant │        │ (Non-Centering Damped│        │ (FTR + Dynamic Trim  │
    │ (Airspeed Scheduling)│        │  Position Hold)      │        │  Magnetic Brake)     │
    └──────────────────────┘        └──────────────────────┘        └──────────────────────┘
```

---

### 5.1 Dual-Pedal Admittance Synchronization (Coupled Equations of Motion)

When two DIY Active Pedals are linked over ESP-NOW, they form a single distributed virtual push-pull mechanism.

Let $x_L \in [0.0, 1.0]$ be the normalized displacement of the Left Pedal ($0.5 = \text{Center}$, $1.0 = \text{Full Forward}$, $0.0 = \text{Full Aft}$).
Let $x_R \in [0.0, 1.0]$ be the normalized displacement of the Right Pedal.

#### Ideal Kinematic Anti-Symmetry:
$$x_R(t) = 1.0 - x_L(t)$$

#### Coupled Admittance Dynamics:
For the Left Pedal, the net virtual acceleration $\ddot{x}_L$ is calculated as:
$$M_{\text{v}} \ddot{x}_L + C_{\text{v}} \dot{x}_L + K_{\text{centering}}(x_L - x_{\text{trim}}) = F_{\text{pilot}, L} - F_{\text{sync}, R} + F_{\text{opposing}} + F_{\text{aero}} + F_{\text{effects}}$$

Where:
* $F_{\text{pilot}, L}$: Force measured by the local load cell (in Newtons).
* $F_{\text{sync}, R}$: Force measured by the opposite pedal received over ESP-NOW.
* $F_{\text{opposing}}$: Virtual rigid-link reaction force when the pilot presses both pedals simultaneously:
  $$\Delta x_{\text{error}} = (x_L + x_R) - 1.0$$
  $$F_{\text{opposing}} = -K_{\text{link}} \cdot \Delta x_{\text{error}} - D_{\text{link}} \cdot (\dot{x}_L + \dot{x}_R)$$
  *(When both feet press forward, $\Delta x_{\text{error}} > 0$, generating an immense restoring force that stops forward travel immediately).*

---

### 5.2 Mode 1: Fixed-Wing Reversible (Aerodynamic Q-Feel)

Simulates General Aviation aircraft where stiffness and travel limits scale dynamically with flight telemetry ($V_{\text{IAS}}$).

```text
                      Force (N)
                         ▲
                         │              High Airspeed (V = 160 kts)
                         │                 /
                         │                /  Medium Airspeed (V = 80 kts)
                         │               /  /
                         │              /  /   Zero Speed / Ramp (V = 0 kts)
                         │             /  /   /
                         │            /  /  _───
                         ├───────────/──/───    ───► Pedal Travel (x)
                        -x_max      0 (Center)   +x_max
```

#### Mathematical Formulation:
1. **Dynamic Centering Stiffness**:
   $$K(V_{\text{IAS}}) = K_{\text{min}} + K_{\text{aero}} \cdot \left( \frac{V_{\text{IAS}}}{V_{\text{ref}}} \right)^2$$
   * At $V_{\text{IAS}} = 0$: $K = K_{\text{min}}$ (light mechanical centering spring).
   * At high airspeed: $K$ increases quadratically up to $K_{\text{max}}$.
2. **Dynamic Travel Limiting (Variable Hard Endstops)**:
   $$x_{\text{travel, max}}(V_{\text{IAS}}) = \begin{cases} 
   x_{\text{full}}, & V_{\text{IAS}} \le V_{\text{corner}} \\
   x_{\text{full}} \cdot \left( \frac{V_{\text{corner}}}{V_{\text{IAS}}} \right), & V_{\text{IAS}} > V_{\text{corner}}
   \end{cases}$$
3. **Aerodynamic Trim Offset**:
   $$x_{\text{neutral}} = 0.5 + x_{\text{trim\_offset}}$$
   $$F_{\text{centering}} = -K(V_{\text{IAS}}) \cdot (x_L - x_{\text{neutral}})$$

---

### 5.3 Mode 2: Fixed-Wing FBW / Constant Spring Gradient

Simulates modern airliners and jet transports with constant mechanical spring feel units, high breakout force, and electronic damping.

#### Mathematical Formulation:
$$F_{\text{feel}}(x) = -\left[ F_{\text{breakout}} \cdot \text{sgn}(x - x_{\text{trim}}) + K_1 (x - x_{\text{trim}}) + K_3 (x - x_{\text{trim}})^3 \right] - D_{\text{viscous}} \cdot \dot{x}$$

* **Breakout Detent**: Requires an initial $20\text{--}40\text{ N}$ to leave center, preventing unintended rudder inputs during manual flight.
* **Cubic Stiffness ($K_3$)**: Progressive force ramp-up near end of travel.

---

### 5.4 Mode 3: Helicopter Pure Friction (Non-Centering Position Hold)

Simulates light helicopters (e.g. R22/R44) with zero return-to-center spring and adjustable mechanical friction clamping.

```text
                      Force (N)
                         ▲
                         │     ┌─────────────────── (+F_friction when moving forward)
                         │     │
                         ├─────┼───────────────────► Velocity (v)
                         │     │
      (-F_friction when ─┴─────┘
       moving aft)
```

#### Mathematical Formulation:
1. **Zero Centering Stiffness**:
   $$K_{\text{centering}} = 0.0$$
2. **Coulomb Friction & Viscous Damping**:
   $$F_{\text{damping}} = -D_{\text{heli}} \cdot \dot{x}$$
   $$F_{\text{friction}} = -F_{\text{coulomb}} \cdot \tanh\left( \frac{\dot{x}}{v_{\epsilon}} \right)$$
3. **Result**: When the pilot moves the pedal to 35% travel and lets go ($\dot{x} = 0$, $F_{\text{pilot}} = 0$), the net acceleration is zero ($\ddot{x} = 0$). The pedal **remains locked at 35% travel indefinitely**.

---

### 5.5 Mode 4: Helicopter Force Trim (Magnetic Brake & FTR)

Simulates turbine helicopters (e.g. UH-60, H145) with electro-magnetic clutch trim release.

```mermaid
stateDiagram-v2
    [*] --> Trimmed_Hold: System Active
    
    Trimmed_Hold --> FTR_Free_Float: Pilot Holds FTR Switch (Button Down)
    FTR_Free_Float --> Trimmed_Hold: Pilot Releases FTR Switch (Button Up)
    
    state Trimmed_Hold {
        direction LR
        Magnetic_Brake: LOCKED
        Centering_Spring: Active around x_trim
        Feel: Spring Resistance
    }
    
    state FTR_Free_Float {
        direction LR
        Magnetic_Brake: RELEASED
        Centering_Spring: ZERO (x_trim tracks current pos)
        Feel: Pure Light Damping
    }
```

#### Mathematical Formulation:
Let $S_{\text{FTR}} \in \{0, 1\}$ be the state of the Force Trim Release button:

$$\begin{cases}
\text{If } S_{\text{FTR}} = 1 \text{ (Pressed)}: & K_{\text{trim}} = 0.0, \quad x_{\text{trim}}(t) = x_{\text{current}}(t) \\
\text{If } S_{\text{FTR}} = 0 \text{ (Released)}: & K_{\text{trim}} = K_{\text{nominal}}, \quad x_{\text{trim}} = \text{const} \quad (\text{locked at moment of release})
\end{cases}$$

#### Beep Trim Slew Rate:
When the pilot pulses the Beep Trim hat switch:
$$x_{\text{trim}}(t + \Delta t) = \text{constrain}\left( x_{\text{trim}}(t) + v_{\text{beep}} \cdot \Delta t \cdot \text{Dir}_{\text{hat}}, \; 0.05, \; 0.95 \right)$$

---

## 6. Flight Simulation Tactile & Environmental Effects

Beyond kinematic centering and friction, the active FFB pedal injects tactile cueing from flight simulator telemetry (MSFS, DCS, X-Plane, IL-2 via SimHub):

```text
       [ Flight Sim Telemetry ] ──► [ SimHub DAP Plugin ] ──► [ ESP32 4 kHz Admittance Engine ]
          ├─ Airspeed (IAS)            └─ Compute Effects        └─ High-Frequency Haptic Layer:
          ├─ Angle of Attack (AoA)                                    ├─ Stall Buffet Vibration
          ├─ Sideslip (Beta)                                          ├─ Ground Roll Rumble
          ├─ Engine RPM / Torque                                      ├─ Asymmetric Engine Kick
          └─ Wheel Speed (Gnd)                                        └─ Weapon Recoil Jolt
```

| Effect | Physical Cause | Waveform / Synthesis | Frequency |
|---|---|---|---|
| **Stall / Rudder Buffet** | Separated turbulent airflow from wing root or high AoA striking the vertical fin | Band-limited pink noise / harmonic oscillation modulated by $(\text{AoA} - \text{AoA}_{\text{crit}})$ | $12\text{--}28\text{ Hz}$ |
| **Ground Roll Rumble** | Runway tarmac texture and centerline expansion joints felt through nosewheel linkage | Amplitude-modulated white noise scaling with ground speed $V_{\text{ground}}$ | $20\text{--}65\text{ Hz}$ |
| **Engine Failure Kick** | Sudden loss of thrust on one wing creating immediate asymmetric yaw moment | Instantaneous step force impulse ($F_{\text{kick}} \propto \text{Thrust}_{\text{dead}}$) | Single impulse + DC offset |
| **Helicopter ETL Shudder** | Effective Translational Lift transition ($16\text{--}24\text{ kts}$) rotor wake boundary | Low-frequency sinusoidal beat frequency matching rotor blade passage ($N \times \Omega$) | $14\text{--}22\text{ Hz}$ |
| **Gunfire Recoil Jolt** | Nose-mounted cannon firing (e.g. A-10 GAU-8, F-16 M61) producing severe airframe shudder | High-amplitude pulse train matching gun rate-of-fire | $50\text{--}70\text{ Hz}$ |

---

## 7. Flight Rudder User Interface (GUI) & Parameter Reference

The SimHub Flight Rudder interface provides dynamic, mode-adaptive control over the dual-pedal bilateral admittance system. The UI automatically reconfigures its force-deflection curve, parameter sliders, and visual telemetry depending on whether **Airplane** or **Helicopter** mode is selected.

---

### 7.1 Airplane (Fixed-Wing) Mode Interface

In Airplane mode, the interface displays the full bipolar centering spring curve with dynamic Q-feel scaling, deadzones, and aerodynamic trim offsets.

![SimHub Plugin Rudder Interface - Airplane Mode](media/images/plugin_rudder_mode_airplane_0.png)

#### Airplane Mode Parameter Breakdown:

| Parameter | UI Control | Range / Units | Firmware Register / Variable | Aviation Physics & Implementation Function |
|---|---|---|---|---|
| **Rudder Mode Selector** | Dropdown | `Airplane` / `Helicopter` | `rudderOffsets_st.rudderMode_u8 = 0` | Activates the aerodynamic self-centering admittance strategy with linear/progressive spring return and speed-dependent force scaling. |
| **Max Centering Force** | Vertical Slider (Right) | $0\text{--}25\text{ kg}$ ($0\text{--}245\text{ N}$) | `config_st->payloadPedalConfig_st.maxForce` | Peak aerodynamic resistance at full deflection ($\pm 50\%$). Emulates rudder surface hinge moments at cruising airspeed. |
| **Center Breakout Force** | Vertical Slider (Lower) | $0\text{--}5\text{ kg}$ ($0\text{--}49\text{ N}$) | `config_st->payloadPedalConfig_st.preloadForce` | Preload / breakout force required to move pedals off aerodynamic neutral ($0\%$). Simulates control cable tension and aerodynamic detent feel. |
| **Centering Profile** | Button Switcher | `Linear` / `Progressive` / `S-Curve` | Spline interpolation in `ForceCurve.cpp` | **Linear**: Constant aerodynamic spring rate (General Aviation).<br/>**Progressive**: Force increases quadratically near end of stroke (Jet / High-Speed).<br/>**S-Curve**: Soft breakout with progressive high-speed cushioning. |
| **Centering Deadzone** | Horizontal Slider | $0\text{--}15\text{ \%}$ | `rudderOffsets_st.deadzone_01` | Center deadband where aerodynamic spring force is zero ($0\text{ N}$). Prevents unintentional rudder inputs from pilot feet resting on the pedals. |
| **Rudder Trim Offset** | Horizontal Slider | $-25\text{\%}\text{ to }+25\text{\%}$ | `rudderOffsets_st.trimOffset_01` | Shifts the physical zero-force aerodynamic neutral point without moving hardware stops. Emulates cockpit rudder trim wheel or yaw trim tab. |
| **Airspeed Q-Feel Gain** | Horizontal Slider | $0\text{--}100\text{ \%}$ | Dynamic scaling factor in SimHub | Sensitivity factor for aerodynamic dynamic pressure $q = \frac{1}{2}\rho V^2$. Determines how aggressively pedal stiffness increases with simulator airspeed ($V_{\text{IAS}}$). |
| **Enable Dynamic Q-Feel** | Checkbox | `Enabled` / `Disabled` | Real-time payload force scaling | When enabled, automatically stiffens pedals as airspeed increases and softens them during slow taxi ($V_{\text{IAS}} \approx 0$). |
| **Travel Limits (MIN / MAX)**| Dual Slider | $5\%\text{--}95\%$ Travel | `softEndstopMinStepperPos_i32` / `Max` | Calibrated software travel limits preventing mechanical collisions with physical endstops. |

---

### 7.2 Helicopter (Anti-Torque) Mode Interface

In Helicopter mode, the aerodynamic centering spring is completely disabled ($K_{\text{spring}} = 0\text{ N}$). The pedal functions as a **Free-Floating, Continuous Position-Hold** anti-torque pedal driven purely by viscous damping and mechanical clamping friction.

![SimHub Plugin Rudder Interface - Helicopter Mode](media/images/plugin_rudder_mode_helicopter_0.png)

#### Helicopter Mode Parameter Breakdown:

| Parameter | UI Control | Range / Units | Firmware Register / Variable | Aviation Physics & Implementation Function |
|---|---|---|---|---|
| **Free-Floating Status Badge** | Top Banner | Readout | `RUDDER_MODE_HELICOPTER (1)` | Visually confirms that the return spring is inactive ($0\text{ N}$). Moving the pedal into any position and releasing the foot leaves it locked at that exact position. |
| **Hydraulic Viscous Damping** | Horizontal Slider | $0\text{--}100\text{ \%}$ | `virtualPedalDampingInPercent_u8` | Simulates the hydraulic damper attached to helicopter anti-torque pedals. Governs $D_{\text{heli}} \cdot \dot{x}$ ($30\text{--}110\text{ N}\cdot\text{s/m}$), preventing erratic pedal flutter during high-rate yaw inputs. |
| **Mechanical Friction** | Horizontal Slider | $0.5\text{--}5.0\text{ N}$ | `coulombFrictionIn0p1N_u8` | Coulomb clamping friction ($F_{\text{coulomb}}$). Provides static holding force so pedals do not sag or shift under cable tension or the passive weight of pilot feet. |
| **Hover Bias / Anti-Torque Trim** | Horizontal Slider | $-25\text{\%}\text{ to }+25\text{\%}$ | `rudderOffsets_st.trimOffset_01` | Pre-positions the pedals for trimmed hover (e.g. left pedal forward for American counter-clockwise rotors; right pedal forward for European clockwise rotors), avoiding pilot leg fatigue. |

---

### 7.3 Shared Bilateral Coupling & Admittance Parameters

Both Airplane and Helicopter modes share the underlying 4 kHz bilateral admittance mechanics and push-pull interconnect:

| Parameter | UI Control | Range / Units | Firmware Implementation | Mechanical & Simulation Function |
|---|---|---|---|---|
| **Bilateral Push-Pull Sync** | Slider | $20\text{--}400\text{ N}$ | `trackingGain_N = 250.0f` | Rigid virtual linkage gain ($x_{\text{remote}} = 1.0 - x_{\text{local}}$). Replicates a solid mechanical pushrod connecting left and right pedals. |
| **Hard Common-Mode Lock** | Firmware Active | $1200\text{ N}$ barrier | `K_COMMON_LOCK_N = 1200.0f` | Solid mechanical barrier that instantly stops dual forward travel if the pilot presses both pedals simultaneously ($(x_L + x_R) > 1.0$). |
| **Virtual Pedal Mass** | Slider | $0.5\text{--}5.0\text{ kg}$ | `virtualMass_kg` | Virtual inertia $M$ in the admittance differential equation ($M\ddot{x} + C\dot{x} + Kx = F$). Sets the tactile heft and physical momentum of the pedal assembly. |
| **Soft Endstop Travel** | Slider | $0\text{--}10\text{ mm}$ | `penetration_m` | Elastic deceleration zone before hard physical limits. |
| **Soft Endstop Stiffness** | Slider | $5\text{--}25\text{ kg/mm}$ | `stiffnessAtMaxTravel_Npermm` | Progressive spring rate at the end of travel, ensuring smooth cushioning without mechanical clatter. |

---

### 7.4 ESP-NOW Wireless Telemetry & Diagnostics

The bottom panel provides real-time latency and packet transmission diagnostics for the peer-to-peer wireless link:

* **Delay (ms)**: Round-trip handshake latency (optimized to $\le 2\text{ ms}$ via high-speed ESP-NOW streaming).
* **Rate (Hz)**: Live packet exchange rate (boosted to $350\text{--}500\text{ Hz}$ with $2\text{ ms}$ update intervals).
* **Jitter (ms)**: Packet arrival timing variance (typically $\le \pm 1\text{ ms}$).
* **Real-time Latency Scope**: Live rolling waveform visualizing jitter stability and signal integrity.

