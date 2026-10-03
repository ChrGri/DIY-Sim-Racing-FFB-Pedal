#pragma once

#include <Arduino.h>
#include <math.h>

// TESTING ONLY: disables the energy-based thermal lockout of the brake resistor and
// the reduction of its regen budget share (availableFraction() stays 1), to check
// whether the thermal governor causes inconsistent pedal speed. The energy is still
// integrated; the dead-man timer and the 80 ms full-on limit stay active.
// Remove for release: a 10 Ohm / 5 W resistor relies on this protection.
// #define BRAKE_RESISTOR_THERMAL_LOCKOUT_DISABLED_FOR_TESTING

/**
 * @brief Brake resistor controller: PWM duty from the regen governor's feedforward
 * (admittance strategy) or a reactive voltage check (rudder), reactive backstop with
 * Schmitt hysteresis, dynamic voltage auto-baselining, thermal lockout and a
 * leaky-bucket energy accumulator. Timer-rollover safe (unsigned deltas).
 */
class PredictiveBrakeController {
private:
  // --- 1. Hysteresis Parameters (Reactive Overvoltage Protection) ---
  // simpleVoltageCheck() (rudder)
  const float BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE = 4.0f;
  const float BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE = 1.5f;
  // Backstop of updateDuty() (feedforward mode): above the servo bleeder level
  // (~rest + 7 V measured), so it only engages when the feedforward falls short
  const float BACKSTOP_UPPER_THRESHOLD_VOLTAGE = 10.0f;
  const float BACKSTOP_LOWER_THRESHOLD_VOLTAGE = 7.0f;
  // Minimum on-time of the PWM (FR120N module: PC817 + 4.7 kOhm gate resistors switch
  // in ~20-45 us). Shorter pulses would spend most of their time in the MOSFET's
  // linear region without dissipating much in the resistor; duties below this are
  // dropped (the servo bleeder takes that small power). 10 % = 100 us at 1 kHz.
  const float MIN_PWM_DUTY_01 = 0.10f;
  float appliedDuty_01_fl32 = 0.0f;

  // Baseline voltage (auto-learned from idle bus voltage or set via
  // setVoltageThreshold)
  float voltageThreshold_V_fl32 = 38.0f;
  bool is_baseline_initialized_b = false;

  // --- 2. Safety Parameters (Thermal Protection) ---
  // Reduced from 500 ms to 80 ms to prevent power supply contention and burnout
  const uint32_t MAX_CONTINUOUS_ON_TIME_US = 80000;
  // Increased from 1.0 s to 3.0 s to allow physical thermal dissipation
  const uint32_t THERMAL_COOLDOWN_TIME_US = 3000000;

  // --- 3. Leaky-Bucket Energy Accumulator (I^2*t Thermal Model) ---
  // brake resistor resistance, set from the config (setResistanceOhm)
  float RESISTOR_OHMS_FL32 = 10.0f;
  const float COOLING_POWER_W_FL32 =
      3.0f; // Passive continuous dissipation capacity
  // 10 Ohm / 5 W cement resistor (~19 mm): typical short-time overload 5x rated
  // power for 5 s (~125 J); half of it as burst budget = 2-3 hard stomps in a row.
  // The 3 W cooling keeps the long-term average at ~60 % of the rating.
  const float MAX_ENERGY_JOULES_FL32 =
      60.0f; // Max energy capacity before thermal trip
  // resistor share of the regen budget starts to shrink at this fraction of the budget
  const float AVAILABILITY_RAMP_START_01 = 0.7f;
  const float RECOVERY_ENERGY_JOULES_FL32 =
      50.0f; // Energy threshold to clear thermal lockout
  float accumulated_energy_j_fl32 = 0.0f;

  // --- Internal State Variables (Hysteresis) ---
  uint32_t prev_time_us_u32 = 0;
  bool is_voltage_fallback_active_b = false;

  // --- Internal State Variables (Thermal Protection) ---
  bool is_hardware_active_b = false;
  uint32_t active_start_time_us_u32 = 0;
  bool is_in_lockout_b = false;
  uint32_t lockout_start_time_us_u32 = 0;

  /**
   * @brief Automatically baseline the resting/idle DC bus voltage.
   * Prevents false triggers when operating on 48V (or custom voltage) power
   * supplies.
   */
  void updateVoltageBaseline(float servoVoltage_fl32, bool isHardwareActive_b,
                             int32_t currentSpeedInHz_i32 = 0) {
    if (!isHardwareActive_b && abs(currentSpeedInHz_i32) < 1000 &&
        servoVoltage_fl32 >= 16.0f && servoVoltage_fl32 <= 65.0f) {
      if (!is_baseline_initialized_b) {
        voltageThreshold_V_fl32 = servoVoltage_fl32;
        is_baseline_initialized_b = true;
      } else if (servoVoltage_fl32 < voltageThreshold_V_fl32) {
        // Quickly track downwards toward true nominal resting voltage
        voltageThreshold_V_fl32 =
            0.95f * voltageThreshold_V_fl32 + 0.05f * servoVoltage_fl32;
      } else {
        // Very slowly track upwards (e.g. if user adjusts power supply trim
        // pot)
        voltageThreshold_V_fl32 =
            0.9999f * voltageThreshold_V_fl32 + 0.0001f * servoVoltage_fl32;
      }
    }
  }

  /**
   * @brief Thermal safety governor of the on/off voltage check (rudder): enforces
   * max continuous on-time, thermal cooldown lockout, and cumulative I^2*t energy
   * tracking. updateDuty() has its own duty-based equivalent.
   */
  bool applyThermalSafetyGovernor(bool logical_activate_b,
                                  float servoVoltage_fl32,
                                  uint32_t currentTimeUs_u32, float dt_s_fl32,
                                  int32_t currentSpeedInHz_i32 = 0) {
    // 1. Update dynamic bus baseline voltage
    updateVoltageBaseline(servoVoltage_fl32, is_hardware_active_b,
                          currentSpeedInHz_i32);

    // 2. Update thermal energy accumulator
    if (is_hardware_active_b) {
      float power_in_w =
          (servoVoltage_fl32 * servoVoltage_fl32) / RESISTOR_OHMS_FL32;
      accumulated_energy_j_fl32 +=
          (power_in_w - COOLING_POWER_W_FL32) * dt_s_fl32;
    } else {
      accumulated_energy_j_fl32 -= COOLING_POWER_W_FL32 * dt_s_fl32;
    }
    if (accumulated_energy_j_fl32 < 0.0f) {
      accumulated_energy_j_fl32 = 0.0f;
    }

    // 3. Trip thermal lockout if cumulative energy exceeds thermal budget
    if (accumulated_energy_j_fl32 >= MAX_ENERGY_JOULES_FL32 &&
        !is_in_lockout_b) {
      is_in_lockout_b = true;
      lockout_start_time_us_u32 = currentTimeUs_u32;
      is_hardware_active_b = false;
      is_voltage_fallback_active_b = false;
    }

    // 4. Evaluate lockout recovery
    if (is_in_lockout_b) {
      bool time_cooled_b = (currentTimeUs_u32 - lockout_start_time_us_u32) >
                           THERMAL_COOLDOWN_TIME_US;
      bool energy_cooled_b =
          accumulated_energy_j_fl32 <= RECOVERY_ENERGY_JOULES_FL32;
      if (time_cooled_b && energy_cooled_b) {
        is_in_lockout_b = false;
      } else {
        logical_activate_b = false;
      }
    }

    // 5. Hardware on-time monitoring & hard cutoff
    if (logical_activate_b) {
      if (!is_hardware_active_b) {
        is_hardware_active_b = true;
        active_start_time_us_u32 = currentTimeUs_u32;
      } else {
        if ((currentTimeUs_u32 - active_start_time_us_u32) >
            MAX_CONTINUOUS_ON_TIME_US) {
          // EMERGENCY SHUTDOWN: continuous on-time exceeded
          is_hardware_active_b = false;
          logical_activate_b = false;
          is_in_lockout_b = true;
          lockout_start_time_us_u32 = currentTimeUs_u32;
          is_voltage_fallback_active_b = false;
        }
      }
    } else {
      is_hardware_active_b = false;
    }

    return logical_activate_b;
  }

public:
  PredictiveBrakeController() {}

  void setVoltageThreshold(float threshold_V) {
    if (threshold_V > 16.0f && threshold_V < 80.0f) {
      voltageThreshold_V_fl32 = threshold_V;
      is_baseline_initialized_b = true;
    }
  }

  // resistance of the fitted brake resistor (pedal config); used by the thermal model
  void setResistanceOhm(float resistance_Ohm) {
    if (resistance_Ohm >= 1.0f) {
      RESISTOR_OHMS_FL32 = resistance_Ohm;
    }
  }

  /**
   * @brief Share of the brake resistor that the regen power budget may count on:
   * 1 while cool, ramping to 0 between 70 % and 100 % of the thermal energy budget,
   * 0 during a thermal lockout. Lets the admittance strategy fall back to the servo's
   * own regen budget before the resistor cuts out.
   */
  float availableFraction() const {
    if (is_in_lockout_b) {
      return 0.0f;
    }
#ifdef BRAKE_RESISTOR_THERMAL_LOCKOUT_DISABLED_FOR_TESTING
    return 1.0f;
#else
    float usedFraction_fl32 = accumulated_energy_j_fl32 / MAX_ENERGY_JOULES_FL32;
    return constrain((1.0f - usedFraction_fl32) / (1.0f - AVAILABILITY_RAMP_START_01), 0.0f, 1.0f);
#endif
  }

  /**
   * @brief PWM duty for the brake resistor (admittance strategy).
   *
   * Feedforward: the strategy requests the duty that dissipates the estimated regen
   * power above the servo's own share. A reactive backstop (full on) only engages well
   * above the servo's bleeder level, so it no longer fires on every press. The thermal
   * model integrates the actually dissipated power duty * V^2 / R.
   *
   * @return duty [0, 1] to apply
   */
  float updateDuty(float feedforwardDuty_01, float servoVoltage_fl32,
                   uint32_t currentTimeUs_u32, int32_t currentSpeedInHz_i32) {
    float dt_s_fl32 = 0.001f;
    if (prev_time_us_u32 != 0) {
      uint32_t dt_us = currentTimeUs_u32 - prev_time_us_u32;
      if (dt_us > 0 && dt_us < 100000) {
        dt_s_fl32 = (float)dt_us * 1e-6f;
      }
    }
    prev_time_us_u32 = currentTimeUs_u32;

    updateVoltageBaseline(servoVoltage_fl32, appliedDuty_01_fl32 > 0.0f,
                          currentSpeedInHz_i32);

    // reactive backstop with hysteresis
    if (servoVoltage_fl32 >= voltageThreshold_V_fl32 + BACKSTOP_UPPER_THRESHOLD_VOLTAGE) {
      is_voltage_fallback_active_b = true;
    } else if (servoVoltage_fl32 <= voltageThreshold_V_fl32 + BACKSTOP_LOWER_THRESHOLD_VOLTAGE) {
      is_voltage_fallback_active_b = false;
    }
    float duty_01 = constrain(feedforwardDuty_01, 0.0f, 1.0f);
    if (duty_01 < MIN_PWM_DUTY_01) {
      duty_01 = 0.0f;
    }
    if (is_voltage_fallback_active_b) {
      duty_01 = 1.0f;
    }

    // thermal model: dissipated energy minus passive cooling
    float fullOnPower_W = (servoVoltage_fl32 * servoVoltage_fl32) / RESISTOR_OHMS_FL32;
    accumulated_energy_j_fl32 +=
        (appliedDuty_01_fl32 * fullOnPower_W - COOLING_POWER_W_FL32) * dt_s_fl32;
    if (accumulated_energy_j_fl32 < 0.0f) {
      accumulated_energy_j_fl32 = 0.0f;
    }
#ifndef BRAKE_RESISTOR_THERMAL_LOCKOUT_DISABLED_FOR_TESTING
    if (accumulated_energy_j_fl32 >= MAX_ENERGY_JOULES_FL32 && !is_in_lockout_b) {
      is_in_lockout_b = true;
      lockout_start_time_us_u32 = currentTimeUs_u32;
    }
#endif

    // continuous full-on limit (the feedforward stays far below 100 %)
    if (duty_01 >= 0.99f) {
      if (!is_hardware_active_b) {
        is_hardware_active_b = true;
        active_start_time_us_u32 = currentTimeUs_u32;
      } else if ((currentTimeUs_u32 - active_start_time_us_u32) > MAX_CONTINUOUS_ON_TIME_US) {
        is_in_lockout_b = true;
        lockout_start_time_us_u32 = currentTimeUs_u32;
      }
    } else {
      is_hardware_active_b = false;
    }

    if (is_in_lockout_b) {
      bool time_cooled_b = (currentTimeUs_u32 - lockout_start_time_us_u32) >
                           THERMAL_COOLDOWN_TIME_US;
      bool energy_cooled_b = accumulated_energy_j_fl32 <= RECOVERY_ENERGY_JOULES_FL32;
      if (time_cooled_b && energy_cooled_b) {
        is_in_lockout_b = false;
      } else {
        duty_01 = 0.0f;
        is_hardware_active_b = false;
        is_voltage_fallback_active_b = false;
      }
    }

    appliedDuty_01_fl32 = duty_01;
    return duty_01;
  }

  // On/off reactive voltage check with the thermal governor (rudder mode)
  bool simpleVoltageCheck(float servoVoltage_fl32,
                          uint32_t currentTimeUs_u32 = 0,
                          int32_t currentSpeedInHz_i32 = 0) {
    if (currentTimeUs_u32 == 0) {
      currentTimeUs_u32 = micros();
    }

    float dt_s_fl32 = 0.001f;
    if (prev_time_us_u32 != 0) {
      uint32_t dt_us = currentTimeUs_u32 - prev_time_us_u32;
      if (dt_us > 0 && dt_us < 100000) {
        dt_s_fl32 = (float)dt_us * 1e-6f;
      }
    }
    prev_time_us_u32 = currentTimeUs_u32;

    float upperLimit_V =
        voltageThreshold_V_fl32 + BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE;
    float lowerLimit_V =
        voltageThreshold_V_fl32 + BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE;

    if (servoVoltage_fl32 >= upperLimit_V) {
      is_voltage_fallback_active_b = true;
    } else if (servoVoltage_fl32 <= lowerLimit_V) {
      is_voltage_fallback_active_b = false;
    }

    return applyThermalSafetyGovernor(is_voltage_fallback_active_b,
                                      servoVoltage_fl32, currentTimeUs_u32,
                                      dt_s_fl32, currentSpeedInHz_i32);
  }
};