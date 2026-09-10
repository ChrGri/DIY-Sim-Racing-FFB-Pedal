#pragma once

#include <Arduino.h>
#include <math.h>

/**
 * @brief Predictive Brake Resistor Controller V2 (Catch-Up & Tracking-Error EMF Observer)
 * 
 * Specifically tuned for:
 *  - Meanwell LRS-350-36 (36V 350W Power Supply)
 *  - StepperOnline / Leadshine iSV57 130W Integrated Servo
 *  - FR120N MOSFET module (Optocoupler-isolated -> Strictly NO high-frequency PWM)
 *  - 10W Braking Resistor (5 Ohm, low thermal mass)
 * 
 * Breakthrough Finding:
 *  Voltage spikes to 66V occur whenever a large servo tracking error (e.g. -2500 to -3000 steps)
 *  is rapidly collapsed back to zero (Catch-Up Deceleration). When the rotor catches up to the commanded
 *  target at up to 200,000 steps/s (~7500 rpm), the internal servo PI loop violently counter-brakes to prevent 
 *  overshoot. This counter-braking acts as a massive generator, dumping kinetic rotor energy into the DC bus!
 * 
 * Core Control Strategy:
 *  1. Tracking Error Time-To-Zero (TTZ) Predictor:
 *     Monitors the rate of error collapse: d(Error)/dt. When error * d(Error)/dt < 0 and TTZ < 30 ms,
 *     it predicts the impending kinetic deceleration spike 15-25 ms BEFORE it dumps into the bus.
 *  2. Zero Firing at Resting Voltage: Never fire while bus voltage is at nominal PSU resting level (<=36.5V).
 *  3. Reactive Hard Clamp (>= 38.5V): If telemetry reports >= 38.5V, fire immediately to guarantee 
 *     voltage can never climb towards 45V or 60V.
 *  4. FR120N MOSFET Safe Pulsing: Discrete single-shot pulses (600 µs min ON, 1000 µs min OFF blanking)
 *     to prevent optocoupler / linear-region burnout.
 *  5. 10W Resistor Thermal Model: Cumulative I^2*t tracking with 12J max budget and automatic cooldown.
 */
class PredictiveBrakeControllerV2 {
public:
  // --- Hardware Configuration Constants ---
  static constexpr float DEFAULT_PSU_VOLTAGE_V = 36.0f;
  // Trigger threshold: 2.0V above PSU voltage (e.g. 38.0V)
  static constexpr float VOLTAGE_TRIGGER_OFFSET_V = 2.0f;
  // Hard clamp threshold: if telemetry sees this, fire immediately
  static constexpr float VOLTAGE_HARD_CLAMP_OFFSET_V = 2.5f; // e.g. 38.5V
  // Servo hardware overvoltage fault boundary
  static constexpr float SERVO_OVERVOLTAGE_TRIP_V = 45.0f;

  // 10W Braking Resistor parameters
  static constexpr float RESISTOR_OHMS = 5.0f;
  static constexpr float RESISTOR_RATED_POWER_W = 10.0f;
  static constexpr float RESISTOR_COOLING_POWER_W = 6.0f;     // Passive dissipation in Watt
  static constexpr float RESISTOR_MAX_ENERGY_JOULES = 12.0f;  // Max thermal capacity before trip
  static constexpr float RESISTOR_RECOVERY_ENERGY_JOULES = 3.0f; // Re-enable threshold

  // FR120N Optocoupler / Gate Protection Constants
  static constexpr uint32_t MIN_ON_PULSE_US = 600;       // Ensure full saturation through PC817
  static constexpr uint32_t MIN_OFF_BLANKING_US = 1000;   // Minimum recovery time between pulses
  static constexpr uint32_t MAX_BURST_TIME_US = 6000;     // Max single continuous burst (6 ms = ~2J)
  static constexpr uint32_t THERMAL_COOLDOWN_TIME_US = 1500000; // 1.5s lockout on thermal overload

  // iSV57 130W Motor Constants (from isv57_tunedParameters.h: Pr7.04-Pr7.14)
  static constexpr float MOTOR_KE_VS_RAD = 0.0436f; // Back-EMF constant (5.6 Vrms/krpm)
  static constexpr float MOTOR_KT_NM_A = 0.080f;    // Torque constant (0.08 Nm/A)
  static constexpr float MOTOR_R_PHASE_OHM = 0.82f; // Phase resistance (0.82 Ohm)
  static constexpr float MOTOR_J_ROTOR_KGM2 = 4.0e-5f; // Rotor inertia (0.40 kg*cm^2)
  static constexpr float STEPS_PER_REV = 1600.0f;   // Standard iSV57 microstepping
  static constexpr float BUS_CAPACITANCE_F = 0.0010f; // 1000 µF DC bus capacitance
  static constexpr float QUIESCENT_POWER_W = 3.0f;  // Standby logic power

  // Tracking Error Dynamics Thresholds
  static constexpr int32_t LARGE_ERROR_THRESHOLD_STEPS = 300;     // Error must be > 300 steps
  static constexpr float MIN_ERROR_COLLAPSE_RATE_STEPS_S = 15000.0f; // Must be closing > 15k steps/s
  static constexpr float TTZ_TRIGGER_WINDOW_S = 0.035f;            // Time-to-zero < 35 ms

private:
  // Voltages
  float voltagePsu_V = DEFAULT_PSU_VOLTAGE_V;
  float voltageBusEstimated_V = DEFAULT_PSU_VOLTAGE_V;
  bool isBaselineInitialized_b = false;

  // Tracking Error history
  float prevError_fl32 = 0.0f;
  uint32_t prevTimeUs_u32 = 0;
  bool isInitialized_b = false;

  // Pulse timing state (FR120N safe pulse generator)
  bool isPulseActive_b = false;
  uint32_t pulseStartTimeUs_u32 = 0;
  uint32_t plannedPulseDurationUs_u32 = 0;
  uint32_t lastPulseEndTimeUs_u32 = 0;

  // Thermal accumulator state (10W resistor I^2*t model)
  float accumulatedEnergyJoules_fl32 = 0.0f;
  bool isInThermalLockout_b = false;
  uint32_t lockoutStartTimeUs_u32 = 0;

  // Telemetry delay compensation ring buffer
  static constexpr uint8_t TELEMETRY_HISTORY_SIZE = 128;
  float busVoltageHistory_fl32[TELEMETRY_HISTORY_SIZE];
  uint8_t historyWriteIdx_u8 = 0;

  /**
   * @brief Updates the 10W resistor thermal energy accumulator
   */
  void updateThermalModel(bool isMosfetActive, float dt_s) {
    if (isMosfetActive) {
      float powerIn_W = (voltageBusEstimated_V * voltageBusEstimated_V) / RESISTOR_OHMS;
      accumulatedEnergyJoules_fl32 += (powerIn_W - RESISTOR_COOLING_POWER_W) * dt_s;
    } else {
      accumulatedEnergyJoules_fl32 -= RESISTOR_COOLING_POWER_W * dt_s;
    }

    if (accumulatedEnergyJoules_fl32 < 0.0f) {
      accumulatedEnergyJoules_fl32 = 0.0f;
    }

    // Check for thermal trip
    if (accumulatedEnergyJoules_fl32 >= RESISTOR_MAX_ENERGY_JOULES && !isInThermalLockout_b) {
      isInThermalLockout_b = true;
      lockoutStartTimeUs_u32 = micros();
      isPulseActive_b = false;
    }

    // Check for recovery from thermal lockout
    if (isInThermalLockout_b) {
      uint32_t now = micros();
      bool timeCooled = (now - lockoutStartTimeUs_u32) > THERMAL_COOLDOWN_TIME_US;
      bool energyCooled = (accumulatedEnergyJoules_fl32 <= RESISTOR_RECOVERY_ENERGY_JOULES);
      if (timeCooled && energyCooled) {
        isInThermalLockout_b = false;
      }
    }
  }

public:
  PredictiveBrakeControllerV2() {
    Reset();
  }

  void Reset() {
    isInitialized_b = false;
    isPulseActive_b = false;
    pulseStartTimeUs_u32 = 0;
    plannedPulseDurationUs_u32 = 0;
    lastPulseEndTimeUs_u32 = 0;
    accumulatedEnergyJoules_fl32 = 0.0f;
    isInThermalLockout_b = false;
    lockoutStartTimeUs_u32 = 0;
    voltagePsu_V = DEFAULT_PSU_VOLTAGE_V;
    voltageBusEstimated_V = DEFAULT_PSU_VOLTAGE_V;
    historyWriteIdx_u8 = 0;
    prevError_fl32 = 0.0f;
    for (uint8_t i = 0; i < TELEMETRY_HISTORY_SIZE; i++) {
      busVoltageHistory_fl32[i] = DEFAULT_PSU_VOLTAGE_V;
    }
  }

  void setVoltageThreshold(float threshold_V) {
    if (threshold_V >= 16.0f && threshold_V <= 65.0f) {
      voltagePsu_V = threshold_V;
      voltageBusEstimated_V = threshold_V;
      isBaselineInitialized_b = true;
    }
  }

  /**
   * @brief Low-frequency telemetry synchronization (called from Modbus)
   */
  void updateTelemetryVoltage(float telemetryVoltage_fl32, int32_t currentSpeedHz_i32) {
    if (telemetryVoltage_fl32 < 16.0f || telemetryVoltage_fl32 > 65.0f) {
      return;
    }

    // When pedal is at rest, smoothly track the nominal resting PSU voltage
    if (abs(currentSpeedHz_i32) < 500 && !isPulseActive_b) {
      if (!isBaselineInitialized_b) {
        voltagePsu_V = telemetryVoltage_fl32;
        voltageBusEstimated_V = telemetryVoltage_fl32;
        isBaselineInitialized_b = true;
      } else {
        if (telemetryVoltage_fl32 < voltagePsu_V) {
          voltagePsu_V = 0.95f * voltagePsu_V + 0.05f * telemetryVoltage_fl32;
        } else {
          voltagePsu_V = 0.999f * voltagePsu_V + 0.001f * telemetryVoltage_fl32;
        }
      }
    }

    // Smith Observer delay compensation
    uint8_t delayedIdx = (historyWriteIdx_u8 - 80) & (TELEMETRY_HISTORY_SIZE - 1);
    float historicalEstimatedVoltage = busVoltageHistory_fl32[delayedIdx];
    float observationError = telemetryVoltage_fl32 - historicalEstimatedVoltage;

    voltageBusEstimated_V += 0.05f * observationError;
    if (voltageBusEstimated_V < voltagePsu_V) {
      voltageBusEstimated_V = voltagePsu_V;
    }

    // Reality check: If telemetry reports higher voltage, pull model up
    if (telemetryVoltage_fl32 > voltageBusEstimated_V) {
      voltageBusEstimated_V = telemetryVoltage_fl32;
    }
  }

  /**
   * @brief Fallback voltage check (reactive mode)
   */
  bool simpleVoltageCheck(float servoVoltage_fl32,
                          uint32_t currentTimeUs_u32 = 0,
                          int32_t currentSpeedInHz_i32 = 0) {
    if (currentTimeUs_u32 == 0) {
      currentTimeUs_u32 = micros();
    }

    float dt_s = 0.001f;
    if (prevTimeUs_u32 != 0) {
      uint32_t dt_us = currentTimeUs_u32 - prevTimeUs_u32;
      if (dt_us > 0 && dt_us < 100000) {
        dt_s = (float)dt_us * 1e-6f;
      }
    }
    prevTimeUs_u32 = currentTimeUs_u32;

    updateTelemetryVoltage(servoVoltage_fl32, currentSpeedInHz_i32);
    updateThermalModel(isPulseActive_b, dt_s);

    if (isInThermalLockout_b) {
      isPulseActive_b = false;
      return false;
    }

    float triggerLimit_V = voltagePsu_V + VOLTAGE_HARD_CLAMP_OFFSET_V; // 38.5V
    float cutoffLimit_V = voltagePsu_V + 1.2f;

    if (servoVoltage_fl32 >= triggerLimit_V && !isPulseActive_b) {
      if ((currentTimeUs_u32 - lastPulseEndTimeUs_u32) >= MIN_OFF_BLANKING_US) {
        isPulseActive_b = true;
        pulseStartTimeUs_u32 = currentTimeUs_u32;
        plannedPulseDurationUs_u32 = 2000; // 2.0 ms pulse
      }
    }

    if (isPulseActive_b) {
      uint32_t onDurationUs = currentTimeUs_u32 - pulseStartTimeUs_u32;
      if (onDurationUs >= plannedPulseDurationUs_u32 || 
          (servoVoltage_fl32 <= cutoffLimit_V && onDurationUs >= MIN_ON_PULSE_US)) {
        isPulseActive_b = false;
        lastPulseEndTimeUs_u32 = currentTimeUs_u32;
      }
    }

    return isPulseActive_b;
  }

  /**
   * @brief Fast 4000 Hz Predictive & Reactive Hybrid Controller (Called from Main.cpp)
   * 
   * @param servoPositionError_i32 Current servo tracking error in steps
   * @param servoPositionErrorChangeRateInStepsPerSecond_fl32 Rate of error change (dError/dt)
   * @param forceVelEst_fl32 Force velocity (dF/dt) from load cell Kalman filter
   * @param currentSpeedInHz_i32 Motor step frequency from FastAccelStepper
   * @param servoVoltage_fl32 Bus voltage from cyclic telemetry
   * @param currentTimeUs_u32 Current timestamp in microseconds
   * @param pedalForceKg_fl32 Applied pedal force in kg (optional)
   * @return true if braking resistor MOSFET must be ON, false if OFF
   */
  bool Update(int32_t servoPositionError_i32,
              float servoPositionErrorChangeRateInStepsPerSecond_fl32,
              float forceVelEst_fl32, 
              int32_t currentSpeedInHz_i32,
              float servoVoltage_fl32, 
              uint32_t currentTimeUs_u32,
              float pedalForceKg_fl32 = 0.0f) {
    if (!isInitialized_b) {
      prevError_fl32 = (float)servoPositionError_i32;
      prevTimeUs_u32 = currentTimeUs_u32;
      voltagePsu_V = (servoVoltage_fl32 >= 16.0f && servoVoltage_fl32 <= 65.0f) 
                     ? servoVoltage_fl32 : DEFAULT_PSU_VOLTAGE_V;
      voltageBusEstimated_V = voltagePsu_V;
      isInitialized_b = true;
      return false;
    }

    uint32_t dt_us = currentTimeUs_u32 - prevTimeUs_u32;
    if (dt_us == 0 || dt_us > 10000) {
      dt_us = 250; // Fallback 4000 Hz
    }
    float dt_s = (float)dt_us * 1e-6f;
    prevTimeUs_u32 = currentTimeUs_u32;

    // 1. Sync Modbus telemetry
    updateTelemetryVoltage(servoVoltage_fl32, currentSpeedInHz_i32);

    // 2. Tracking Error Dynamics & Time-To-Zero (TTZ) Analysis
    float currentError_fl32 = (float)servoPositionError_i32;
    float dError_fl32 = servoPositionErrorChangeRateInStepsPerSecond_fl32;

    // Calculate Time-To-Zero (TTZ): Only valid when error is moving TOWARDS zero!
    // Condition: error * dError < 0 means the error magnitude is actively collapsing
    float ttz_s = 999.0f;
    bool isErrorCollapsing_b = false;
    if ((currentError_fl32 * dError_fl32) < 0.0f) {
      isErrorCollapsing_b = true;
      ttz_s = fabsf(currentError_fl32 / dError_fl32);
    }

    // Rate of collapse in steps/s
    float collapseRateStepsPerS = fabsf(dError_fl32);
    bool wasLargeError_b = (fabsf(currentError_fl32) > (float)LARGE_ERROR_THRESHOLD_STEPS) ||
                           (fabsf(prevError_fl32) > (float)LARGE_ERROR_THRESHOLD_STEPS);
    prevError_fl32 = currentError_fl32;

    // 3. Catch-Up Kinetic Braking Power Prediction
    // When rotor catches up at high step rate, rotor angular speed is omega = 2*pi*f/N_spr
    float catchupOmega_rad_s = (2.0f * (float)M_PI / STEPS_PER_REV) * collapseRateStepsPerS;
    
    // Kinetic deceleration power dumped when arriving at zero: P = J * omega * (omega / dt_stop)
    float P_catchup_regen_W = 0.0f;
    if (isErrorCollapsing_b && wasLargeError_b && (collapseRateStepsPerS > MIN_ERROR_COLLAPSE_RATE_STEPS_S)) {
      // Rotor kinetic energy stored during catchup: E = 0.5 * J * omega^2
      float E_catchup_J = 0.5f * MOTOR_J_ROTOR_KGM2 * (catchupOmega_rad_s * catchupOmega_rad_s);
      // Deceleration happens over ~15 ms when approaching target
      P_catchup_regen_W = (E_catchup_J / 0.015f) * 0.90f; // Generator conversion efficiency
    }

    // Motor Back-EMF at catchup speed
    float U_emf_catchup_LL_V = 1.732f * MOTOR_KE_VS_RAD * catchupOmega_rad_s;
    if (U_emf_catchup_LL_V > (voltageBusEstimated_V + 1.2f)) {
      float excessV = U_emf_catchup_LL_V - (voltageBusEstimated_V + 1.2f);
      P_catchup_regen_W += (excessV * excessV) / (2.0f * MOTOR_R_PHASE_OHM);
    }

    // 4. Bus Voltage State Prediction
    float dU_pred_free_V = ((P_catchup_regen_W - QUIESCENT_POWER_W) / (BUS_CAPACITANCE_F * voltageBusEstimated_V)) * dt_s;
    float U_pred_free_V = voltageBusEstimated_V + dU_pred_free_V;
    if (U_pred_free_V < voltagePsu_V) {
      U_pred_free_V = voltagePsu_V;
    }

    // Voltage thresholds
    float U_trigger_V = voltagePsu_V + VOLTAGE_TRIGGER_OFFSET_V;       // 38.0V
    float U_hard_clamp_V = voltagePsu_V + VOLTAGE_HARD_CLAMP_OFFSET_V; // 38.5V

    // 5. Thermal Accumulator Update
    updateThermalModel(isPulseActive_b, dt_s);

    // 6. Dual-Trigger Firing Logic
    if (isInThermalLockout_b) {
      isPulseActive_b = false;
    } else {
      bool blankingElapsed_b = (currentTimeUs_u32 - lastPulseEndTimeUs_u32) >= MIN_OFF_BLANKING_US;

      if (!isPulseActive_b) {
        // TRIGGER 1: Predictive Catch-Up Deceleration Trigger!
        // Fires precisely when a large error is collapsing into zero within 35 ms AND regen power > 15W
        bool predictiveCatchupTrigger_b = isErrorCollapsing_b && wasLargeError_b &&
                                          (ttz_s < TTZ_TRIGGER_WINDOW_S) &&
                                          (P_catchup_regen_W > 15.0f);

        // TRIGGER 2: Voltage Prediction Threshold
        bool voltagePredictiveTrigger_b = (U_pred_free_V > U_trigger_V) && (P_catchup_regen_W > 10.0f);

        // TRIGGER 3: Reactive Hard Clamp (Safety Net)
        // If telemetry voltage is >= 38.5V, CLAMP IMMEDIATELY!
        bool reactiveClampTrigger_b = (servoVoltage_fl32 >= U_hard_clamp_V) || 
                                      (voltageBusEstimated_V >= U_hard_clamp_V);

        // ABSOLUTE PROTECTION: Never fire if voltage is sitting at or below nominal PSU level (36.0V)!
        // (Allows predictive firing if TTZ is critically close < 15 ms to preempt the capacitor surge)
        bool voltageEligible_b = (voltageBusEstimated_V >= (voltagePsu_V + 0.8f)) || 
                                 (servoVoltage_fl32 >= (voltagePsu_V + 1.2f)) ||
                                 (predictiveCatchupTrigger_b && (ttz_s < 0.015f));

        if ((predictiveCatchupTrigger_b || voltagePredictiveTrigger_b || reactiveClampTrigger_b) &&
            voltageEligible_b && blankingElapsed_b) {
          
          // Calculate energy to absorb
          float effectiveVoltage = fmaxf(voltageBusEstimated_V, servoVoltage_fl32);
          float E_excess_J = 0.5f * BUS_CAPACITANCE_F * 
                             (effectiveVoltage * effectiveVoltage - U_trigger_V * U_trigger_V);
          if (E_excess_J < 0.15f) {
            E_excess_J = 0.15f; // Minimum 150 mJ pulse
          }

          // Calculate pulse duration
          float P_resistor_W = (effectiveVoltage * effectiveVoltage) / RESISTOR_OHMS;
          uint32_t requiredPulseUs = (uint32_t)((E_excess_J / P_resistor_W) * 1e6f);

          // For fast catchup collapse, provide solid 2-3 ms pulse
          if (predictiveCatchupTrigger_b || effectiveVoltage > U_hard_clamp_V) {
            requiredPulseUs = fmaxf(requiredPulseUs, 2000); // At least 2.0 ms
          }

          // Clamp pulse for FR120N and 10W resistor safety
          if (requiredPulseUs < MIN_ON_PULSE_US) {
            requiredPulseUs = MIN_ON_PULSE_US;
          }
          if (requiredPulseUs > MAX_BURST_TIME_US) {
            requiredPulseUs = MAX_BURST_TIME_US;
          }

          // Activate single-shot pulse!
          isPulseActive_b = true;
          pulseStartTimeUs_u32 = currentTimeUs_u32;
          plannedPulseDurationUs_u32 = requiredPulseUs;
        }
      } else {
        // Pulse is currently ACTIVE: Check completion criteria
        uint32_t elapsedUs = currentTimeUs_u32 - pulseStartTimeUs_u32;

        bool durationExpired = (elapsedUs >= plannedPulseDurationUs_u32);
        bool voltageNormalized = (voltageBusEstimated_V <= (voltagePsu_V + 1.0f)) && 
                                 (servoVoltage_fl32 <= (voltagePsu_V + 1.2f)) && 
                                 (elapsedUs >= MIN_ON_PULSE_US);

        if (durationExpired || voltageNormalized) {
          isPulseActive_b = false;
          lastPulseEndTimeUs_u32 = currentTimeUs_u32;
        }
      }
    }

    // 7. Physical Bus Voltage State Integration
    float P_resistor_actual_W = isPulseActive_b 
                                ? ((voltageBusEstimated_V * voltageBusEstimated_V) / RESISTOR_OHMS) 
                                : 0.0f;
    float dU_actual_V = ((P_catchup_regen_W - P_resistor_actual_W - QUIESCENT_POWER_W) / 
                        (BUS_CAPACITANCE_F * voltageBusEstimated_V)) * dt_s;
    voltageBusEstimated_V += dU_actual_V;
    if (voltageBusEstimated_V < voltagePsu_V) {
      voltageBusEstimated_V = voltagePsu_V;
    }

    // Store in ring buffer
    busVoltageHistory_fl32[historyWriteIdx_u8] = voltageBusEstimated_V;
    historyWriteIdx_u8 = (historyWriteIdx_u8 + 1) & (TELEMETRY_HISTORY_SIZE - 1);

    return isPulseActive_b;
  }

  // Diagnostic / Telemetry Getters
  float getEstimatedBusVoltage() const { return voltageBusEstimated_V; }
  float getPsuVoltage() const { return voltagePsu_V; }
  float getAccumulatedJoules() const { return accumulatedEnergyJoules_fl32; }
  bool isThermalLockout() const { return isInThermalLockout_b; }
};
