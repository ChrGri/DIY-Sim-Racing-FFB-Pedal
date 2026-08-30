# Pedal Operational States, Startup Architecture & Standby Mode

This document details the startup sequence, standby mode architecture, state machine transitions, and communication protocols for the DIY Sim Racing Active FFB Pedal.

---

## 1. State Machine Overview

The pedal firmware operates as a FreeRTOS-driven state machine managed by `g_pedalOperationalState_u8` with the following primary operational states:

| State | Enum Value | Description | Motor State | LED Indication |
|---|---|---|---|---|
| **Standby** | `PEDAL_STATE_STANDBY_WAITING_FOR_WAKEUP_E` (0) | Silent standby; waiting for SimHub connection or foot press | Unenergized / Disabled (`SERVO_IDLE_NOT_CONNECTED`) | Soft Blue (`#002080`) |
| **Homing** | `PEDAL_STATE_HOMING_E` (1) | Sensorless min/max calibration & position alignment | Energized (`SERVO_CONNECTED`) | Cyan (`#00FFFF`) → Purple (`#800080`) |
| **Active** | `PEDAL_STATE_ACTIVE_E` (2) | 4 kHz closed-loop FFB calculation & USB Joystick output | Active Closed Loop | Green (`#00FF00`) / Profile Color |
| **Idle / Sleep** | N/A (Sub-state of Active) | Inactivity timeout reached (`servoIdleTimeout_u8`) | Unenergized / Disabled (`SERVO_IDLE_NOT_CONNECTED`) | Red (`#FF0000`) |

---

## 2. State Transition Diagram

```mermaid
stateDiagram-v2
    [*] --> Boot_Initialization: Power On / Reset

    state Boot_Initialization {
        [*] --> Hardware_Init: Clamp GPIOs & Read EEPROM
        Hardware_Init --> Loadcell_Calibration: Silent Bias & Variance Estimate
        Loadcell_Calibration --> Tasks_Spawned: Start FreeRTOS Tasks (USB Rx/Tx, Telemetry, ESP-NOW)
    }

    Tasks_Spawned --> Standby_Waiting: wakeOnPluginOnly == 1
    Tasks_Spawned --> Homing_Sequence: wakeOnPluginOnly == 0

    state Standby_Waiting {
        description: LED = Soft Blue | Motor = Disabled | USB/ESP-NOW = Online
    }

    Standby_Waiting --> Homing_Sequence: SimHub connected (WAKEUP_PEDAL)
    Standby_Waiting --> Homing_Sequence: Foot pressure > 1.0 kg (500ms delay)

    state Homing_Sequence {
        [*] --> Double_Beep: Play 770 Hz tone
        Double_Beep --> Servo_Wake: Core 0 enables iSV57 axis
        Servo_Wake --> Find_Limits: Sensorless Min/Max search
        Find_Limits --> Move_To_Zero: Set zero & move to soft min
    }

    Homing_Sequence --> Active_Operation: Homing Complete (LED = Green)

    state Active_Operation {
        description: 4 kHz Closed-Loop FFB | USB Joystick Output
    }

    Active_Operation --> Idle_Sleep: Inactivity > servoIdleTimeout
    
    state Idle_Sleep {
        description: LED = Red | Motor = Disabled | USB/Joystick = Online (No Disconnect)
    }

    Idle_Sleep --> Homing_Sequence: Foot pressure > 1.0 kg / SimHub wake
```

### ASCII State Flow
```text
[ Power On / Reset ]
         │
         ▼
[ Boot & Init ] ──► (Clamp GPIOs, Loadcell Bias/Variance Calibration, Start FreeRTOS Comms)
         │
         ├─── if (wakeOnPluginOnly == 1) ──► [ STANDBY MODE ] (LED Soft Blue, Motor Off, USB Online)
         │                                         │
         │                                         ├── SimHub WAKEUP_PEDAL
         │                                         └── Foot Press > 1.0 kg
         │                                                 │
         │◄── if (wakeOnPluginOnly == 0) ──────────────────┘
         │
         ▼
[ HOMING SEQUENCE ]
  ├─ Double-beep (770 Hz)
  ├─ Core 0 enables servo axis (servoWakeAction)
  ├─ Sensorless Min/Max Search
  └─ Move to soft min pos
         │
         ▼
[ ACTIVE OPERATION ] (4 kHz FFB Control & USB Joystick)
         │
         │ (Inactivity > idle timeout)
         ▼
[ IDLE / SLEEP ] (LED Red, Motor Off, USB & Game bindings stay CONNECTED)
         │
         └── Foot Press > 1.0 kg ──► [ HOMING SEQUENCE ] (Seamless wake, no MCU reboot)
```

---

## 3. Detailed Startup & Setup Routine

### Non-Blocking Boot Architecture
In earlier revisions, the setup routine executed a blocking homing sweep during `setup()` before FreeRTOS communication tasks started. The refactored startup architecture separates initialization into discrete phases:

1. **Hardware Pin Clamping & EEPROM Loading**:
   - GPIO lines (UART, RS485/Modbus, LED, Buzzer) are immediately initialized to safe idle states.
   - Pedal configuration struct (`DapConfig_t`) is loaded from EEPROM.

2. **Zero-Vibration Loadcell Calibration**:
   - `loadcell->estimateBiasAndVariance()` and 2nd-order Kalman filter allocations execute during boot/standby while the motor is silent and motionless.

3. **Instant FreeRTOS Task Dispatch**:
   - `serialCommunicationTaskRx` (Core 0), `serialCommunicationTaskTx` (Core 0), `servoCommunicationTask` (Core 0), and `pedalUpdateTask` (Core 1) are spawned immediately.
   - USB CDC serial and wireless ESP-NOW links are alive and listening within milliseconds of power-on.

4. **Standby Gating (`wakeOnPluginOnly_u8`)**:
   - If `wakeOnPluginOnly_u8 == 1`: The servo enters unpowered idle (`SERVO_IDLE_NOT_CONNECTED`), the LED turns Soft Blue, and `g_pedalOperationalState_u8` is set to `PEDAL_STATE_STANDBY_WAITING_FOR_WAKEUP_E`.
   - If `wakeOnPluginOnly_u8 == 0` (Legacy / Auto-boot): `performPedalHomingSequence()` runs immediately, entering `PEDAL_STATE_ACTIVE_E`.

---

## 4. Thread-Safe Servo Wakeup (`servoWakeAction`)

To prevent Modbus bus contention and corrupted telemetry packets:
* **All Modbus register operations** with the integrated servo (iSV57) are serialized exclusively within `servoCommunicationTask` on Core 0.
* When a wake request is initiated (from `pedalUpdateTask` on Core 1 or from USB serial / ESP-NOW), `stepper->servoWakeAction()` sets a thread-safe flag `setServoToWake_b = true`.
* `servoCommunicationTask` on Core 0 picks up `setServoToWake_b`, calls `isv57.enableAxis()`, transitions `servoStatus` to `SERVO_CONNECTED`, and clears the flag.
* This eliminates UART collisions and avoids false `"Servo communication lost!"` alarm triggers.

---

## 5. Seamless Wakeup from Idle Timeout (No MCU Reset)

* When the configured `servoIdleTimeout_u8` expires, the motor enters unenergized sleep (`SERVO_IDLE_NOT_CONNECTED`) with a Red LED.
* When the user presses the pedal (> 1.0 kg), `pedalUpdateTask` detects the force, pauses for 500 ms (allowing the user to release foot pressure), plays a 770 Hz double-beep, and executes `performPedalHomingSequence()`.
* **Zero Disconnects**: Unlike older implementations that called `ESP.restart()`, this approach preserves USB CDC serial streams, HID joystick descriptor endpoints, and game input bindings without interruption.

---

## 6. SimHub Plugin & ESP-NOW Bridge Protocol

* **Payload Version**: Synchronized at `DAP_VERSION_CONFIG_U8 = 173`.
* **Immediate Wakeup Command**: SimHub automatically sets `systemAction_u8 = PedalSystemAction::WAKEUP_PEDAL` on initial connection / config query.
* **0 ms Connection Dispatch**: Serial and wireless connections dispatch `Reading_config_auto` immediately upon port open / packet discovery rather than waiting for background polling timers.
* **ESP-NOW Bridge (`ESP32_master`)**: Action packets containing `WAKEUP_PEDAL` are forwarded over ESP-NOW to wireless pedals, which transition from `PEDAL_STATE_STANDBY_WAITING_FOR_WAKEUP_E` to `PEDAL_STATE_HOMING_E` upon receipt.
