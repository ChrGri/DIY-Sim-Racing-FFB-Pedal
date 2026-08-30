# iSV57 Servo Simultaneous Power-On Fix & Standby Power-Cycle Recovery

## 1. Overview & Problem Description

When the **ESP32** and the **iSV57 integrated closed-loop servo** are powered on simultaneously from the same DC bus power supply (e.g., 36V or 48V rail), or when the servo is power-cycled dynamically via hardware switching (`SERVO_POWER_PIN`), the iSV57 can enter an **alarm/error state** (transient undervoltage, communication timeout, position deviation, or drive disabled) and lose its configured internal volatile registers.

This document details the startup safeguards, the 3.5-second discovery and stabilization window, transient alarm wiping, and the **full register re-configuration pipeline** executed after every cold boot or dynamic hardware power cycle.

---

## 2. Hardware Power Pin (`SERVO_POWER_PIN`) vs. Software Standby

The firmware supports two distinct standby / sleep architectures across different PCB revisions:

| Architecture | PCB Variants | Standby Mechanism (`servoIdleAction`) | Wakeup Mechanism (`servoWakeAction`) |
| :--- | :--- | :--- | :--- |
| **Software Modbus Standby** | `ControlBoard_V7` (PCB 14)<br>`ControlBoard_V6` (PCB 13)<br>`ControlBoard_V5` (PCB 12) | Axis disabled via Modbus (`isv57.disableAxis()`). Motor unenergized, but servo DSP and RS485 communication remain powered and alive. | Axis enabled via Modbus (`isv57.enableAxis()`). Telemetry and registers remain persistent; instant wake. |
| **Hardware Power Switching** | `ControlBoard_PCBA_V2X` (PCB 9)<br>(Boards with `SERVO_POWER_PIN` defined) | Cuts physical power to servo via GPIO (`gpio_set_level(SERVO_POWER_PIN, 0)`). Standby state set to `SERVO_IDLE_NOT_CONNECTED`. | Re-energizes `SERVO_POWER_PIN`, waits for DSP boot & SRDY, clears boot alarms, and re-programs all registers via `configureServoRegistersAfterPowerOn()`. |

---

## 3. Root Cause Analysis & Fix Matrix

| Aspect | Previous State / Issue | Resolution & Architecture |
| :--- | :--- | :--- |
| **Power-On DSP Stabilization** | ESP32 booted faster than iSV57 internal DSP/rails, causing immediate Modbus timeout and infinite reboot loop. | Added a **3.5-second discovery retry window** (`isv57.findServosSlaveId()`) in `StepperWithLimits` and `servoWakeAction()` to allow power rails to charge and DSP firmware to stabilize. |
| **Transient Boot Alarms** | iSV57 frequently booted with transient line-noise or undervoltage alarms that blocked step execution. | Added double `isv57.clearServoAlarms()` (writing `0x1111` to `0x019A` / `Pr0.25`) immediately upon discovery. |
| **Volatile Register Loss on Power Cycle** | On boards with `SERVO_POWER_PIN`, turning power off cleared the servo's RAM registers (telemetry addresses, microsteps, gains, voltage limits). | Implemented [`configureServoRegistersAfterPowerOn()`](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/StepperWithLimits.cpp#L904-L939) which runs after **every** hardware power cycle: <br>1. Wipes startup alarms (`clearServoAlarms()`)<br>2. Reads alarm history (`readAlarmHistory()`)<br>3. Configures cyclic telemetry (`setupServoStateReading()` → `0x0191`–`0x0194`)<br>4. Re-programs tuned parameters (`sendTunedServoParameters()`)<br>5. Sets braking resistor voltage threshold (`setServoVoltage()`)<br>6. Re-establishes lifeline & enables axis (`enableAxis()`). |
| **UART RX Floating Noise** | Floating RX lines during boot injected false start bits / framing errors into the ESP32 UART buffer. | Pre-clamped `ISV57_RXPIN` with `pinMode(ISV57_RXPIN, INPUT_PULLUP)` and asserted `ISV57_TXPIN` HIGH at earliest boot in `setup()` and constructor. |
| **Standby Communication Collisions** | While the servo was unpowered (`SERVO_IDLE_NOT_CONNECTED`), background tasks flooded UART with pings and false alarms. | Gated [`updateLifeline()`](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/StepperWithLimits.cpp#L641-L649) and [`handleConnectionLoss()`](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/StepperWithLimits.cpp#L651-L668) to bypass execution while in `SERVO_IDLE_NOT_CONNECTED`. |
| **DC Voltage Verification** | Power supply voltage check allowed only 1.0s before aborting. | Extended loop to 30 iterations (3.0s) to support slow-ramping / soft-start power supplies. |

---

## 4. Key Implementation References

### Firmware Source Files
- [StepperWithLimits.h](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/include/StepperWithLimits.h#L99): Declaration of `configureServoRegistersAfterPowerOn()`.
- [StepperWithLimits.cpp](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/StepperWithLimits.cpp#L904-L1012):
  - `configureServoRegistersAfterPowerOn()`: Complete register setup pipeline.
  - `servoWakeAction()`: Power-pin activation, 3.5s discovery retry, alarm clearing, and register initialization.
  - `servoIdleAction()`: Power-pin shutdown and quiet standby state transition.
  - `updateLifeline()` & `handleConnectionLoss()`: Standby-aware connection validation.
- [Main.cpp](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/Main.cpp#L687-L725):
  - Early GPIO clamping in `setup()`.
  - `performPedalHomingSequence()` executing `servoWakeAction()` before homing sweep.
- [isv57communication.cpp](file:///c:/Users/chris/OneDrive/Desktop/GIT/DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7/ESP32/src/isv57communication.cpp):
  - `clearServoAlarms()` (Register `0x019A = 0x1111`).
  - `setupServoStateReading()` (Registers `0x0191`–`0x0194`).
  - `sendTunedServoParameters()`.
