#pragma once

#include <Arduino.h>
#include <math.h>

/**
 * @brief Predictive brake resistor controller (Time-To-Impact & Foot Dynamics Observer)
 * 100% timer-rollover safe (Unsigned Delta Logic), thermal lockout, leaky-bucket energy accumulator,
 * dynamic voltage auto-baselining & Schmitt hysteresis.
 */
class PredictiveBrakeController {
private:
    // --- 1. Tuning Parameters (Prediction) ---
    const float FOOT_ESCAPE_RATE_KG_S = -100.0f; 
    const float TTZ_WARNING_S = 0.04f; 
    const int32_t MIN_SPEED_HZ = 30000; 
    const uint32_t HOLD_TIME_US = 30000; // 30 ms predictive burst
    const int32_t ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32 = -100; 

    // --- 2. Hysteresis Parameters (Reactive Overvoltage Protection) ---
    const float BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE = 4.0f;
    const float BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE = 1.5f;
    
    // Baseline voltage (auto-learned from idle bus voltage or set via setVoltageThreshold)
    float voltageThreshold_V_fl32 = 38.0f; 
    bool is_baseline_initialized_b = false;

    // --- 3. Safety Parameters (Thermal Protection) ---
    // Reduced from 500 ms to 80 ms to prevent power supply contention and burnout
    const uint32_t MAX_CONTINUOUS_ON_TIME_US = 80000; 
    // Increased from 1.0 s to 3.0 s to allow physical thermal dissipation
    const uint32_t THERMAL_COOLDOWN_TIME_US = 3000000; 

    // --- 4. Leaky-Bucket Energy Accumulator (I^2*t Thermal Model) ---
    const float RESISTOR_OHMS_FL32 = 5.0f;            // Typical brake resistor resistance
    const float COOLING_POWER_W_FL32 = 20.0f;         // Passive continuous dissipation capacity
    const float MAX_ENERGY_JOULES_FL32 = 300.0f;      // Max energy capacity before thermal trip (~10-15°C core rise)
    const float RECOVERY_ENERGY_JOULES_FL32 = 50.0f;  // Energy threshold to clear thermal lockout
    float accumulated_energy_j_fl32 = 0.0f;

    // --- Internal State Variables (Prediction & Hysteresis) ---
    float prev_error_fl32 = 0.0f;
    uint32_t prev_time_us_u32 = 0;
    bool is_initialized_b = false;
    
    // Rollover-safe timer variables
    bool is_timer_active_b = false;
    uint32_t timer_start_time_us_u32 = 0;
    
    bool is_voltage_fallback_active_b = false; 

    // --- Internal State Variables (Thermal Protection) ---
    bool is_hardware_active_b = false;
    uint32_t active_start_time_us_u32 = 0;
    bool is_in_lockout_b = false;
    uint32_t lockout_start_time_us_u32 = 0;

    /**
     * @brief Automatically baseline the resting/idle DC bus voltage.
     * Prevents false triggers when operating on 48V (or custom voltage) power supplies.
     */
    void updateVoltageBaseline(float servoVoltage_fl32, bool isHardwareActive_b, int32_t currentSpeedInHz_i32 = 0) {
        if (!isHardwareActive_b && abs(currentSpeedInHz_i32) < 1000 && servoVoltage_fl32 >= 16.0f && servoVoltage_fl32 <= 65.0f) {
            if (!is_baseline_initialized_b) {
                voltageThreshold_V_fl32 = servoVoltage_fl32;
                is_baseline_initialized_b = true;
            } else if (servoVoltage_fl32 < voltageThreshold_V_fl32) {
                // Quickly track downwards toward true nominal resting voltage
                voltageThreshold_V_fl32 = 0.95f * voltageThreshold_V_fl32 + 0.05f * servoVoltage_fl32;
            } else {
                // Very slowly track upwards (e.g. if user adjusts power supply trim pot)
                voltageThreshold_V_fl32 = 0.9999f * voltageThreshold_V_fl32 + 0.0001f * servoVoltage_fl32;
            }
        }
    }

    /**
     * @brief Universal thermal safety governor: enforces max continuous on-time,
     * thermal cooldown lockout, and cumulative I^2*t energy tracking.
     * Protects brake resistor across BOTH predictive mode and non-predictive voltage check.
     */
    bool applyThermalSafetyGovernor(bool logical_activate_b, float servoVoltage_fl32, uint32_t currentTimeUs_u32, float dt_s_fl32, int32_t currentSpeedInHz_i32 = 0) {
        // 1. Update dynamic bus baseline voltage
        updateVoltageBaseline(servoVoltage_fl32, is_hardware_active_b, currentSpeedInHz_i32);

        // 2. Update thermal energy accumulator
        if (is_hardware_active_b) {
            float power_in_w = (servoVoltage_fl32 * servoVoltage_fl32) / RESISTOR_OHMS_FL32;
            accumulated_energy_j_fl32 += (power_in_w - COOLING_POWER_W_FL32) * dt_s_fl32;
        } else {
            accumulated_energy_j_fl32 -= COOLING_POWER_W_FL32 * dt_s_fl32;
        }
        if (accumulated_energy_j_fl32 < 0.0f) {
            accumulated_energy_j_fl32 = 0.0f;
        }

        // 3. Trip thermal lockout if cumulative energy exceeds thermal budget
        if (accumulated_energy_j_fl32 >= MAX_ENERGY_JOULES_FL32 && !is_in_lockout_b) {
            is_in_lockout_b = true;
            lockout_start_time_us_u32 = currentTimeUs_u32;
            is_hardware_active_b = false;
            is_voltage_fallback_active_b = false;
            is_timer_active_b = false;
        }

        // 4. Evaluate lockout recovery
        if (is_in_lockout_b) {
            bool time_cooled_b = (currentTimeUs_u32 - lockout_start_time_us_u32) > THERMAL_COOLDOWN_TIME_US;
            bool energy_cooled_b = accumulated_energy_j_fl32 <= RECOVERY_ENERGY_JOULES_FL32;
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
                if ((currentTimeUs_u32 - active_start_time_us_u32) > MAX_CONTINUOUS_ON_TIME_US) {
                    // EMERGENCY SHUTDOWN: continuous on-time exceeded
                    is_hardware_active_b = false;
                    logical_activate_b = false;
                    is_in_lockout_b = true;
                    lockout_start_time_us_u32 = currentTimeUs_u32;
                    is_voltage_fallback_active_b = false;
                    is_timer_active_b = false;
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

    void Reset() {
        is_initialized_b = false;
        is_timer_active_b = false;
        is_voltage_fallback_active_b = false;
        is_hardware_active_b = false;
        is_in_lockout_b = false;
        accumulated_energy_j_fl32 = 0.0f;
    }

    bool simpleVoltageCheck(float servoVoltage_fl32, uint32_t currentTimeUs_u32 = 0, int32_t currentSpeedInHz_i32 = 0) {
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

        float upperLimit_V = voltageThreshold_V_fl32 + BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE;
        float lowerLimit_V = voltageThreshold_V_fl32 + BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE;

        if (servoVoltage_fl32 >= upperLimit_V) {
            is_voltage_fallback_active_b = true; 
        } else if (servoVoltage_fl32 <= lowerLimit_V) {
            is_voltage_fallback_active_b = false;
        }

        return applyThermalSafetyGovernor(is_voltage_fallback_active_b, servoVoltage_fl32, currentTimeUs_u32, dt_s_fl32, currentSpeedInHz_i32);
    }

    bool Update(int32_t servoPositionError_i32
                , float servoPositionErrorChangeRateInStepsPerSecond_fl32
                , float forceVelEst_fl32
                , int32_t currentSpeedInHz_i32
                , float servoVoltage_fl32
                , uint32_t currentTimeUs_u32) 
    {
        if (!is_initialized_b) {
            prev_error_fl32 = (float)servoPositionError_i32;
            prev_time_us_u32 = currentTimeUs_u32;
            is_initialized_b = true;
            updateVoltageBaseline(servoVoltage_fl32, false, currentSpeedInHz_i32);
            return false;
        }

        uint32_t dt_us_u32 = currentTimeUs_u32 - prev_time_us_u32;
        if (dt_us_u32 == 0) dt_us_u32 = 250; // Default 4000 Hz interval
        float dt_s_fl32 = (float)dt_us_u32 * 1e-6f;
        float current_error_fl32 = (float)servoPositionError_i32;

        // --- 1. Time-To-Zero (TTZ) Calculation ---
        float d_error_fl32 = servoPositionErrorChangeRateInStepsPerSecond_fl32;
        float ttz_s_fl32 = 999.0f;
        if ((current_error_fl32 * d_error_fl32) < 0.0f) {
            ttz_s_fl32 = fabsf(current_error_fl32 / d_error_fl32);
        }

        // --- 2. Foot Dynamics & Kinetic Checks ---
        bool high_kinetic_energy_b = (abs(currentSpeedInHz_i32) > MIN_SPEED_HZ) || (fabsf(d_error_fl32) > (float)MIN_SPEED_HZ);
        bool foot_is_dynamic_b = ( fabsf(forceVelEst_fl32) > fabsf(FOOT_ESCAPE_RATE_KG_S) );
        bool errorWasLarge_b = (prev_error_fl32 < ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32) || (current_error_fl32 < ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32);

        // --- 3. The Precision Trigger ("Sniper Trigger") ---
        bool trigger_b = foot_is_dynamic_b && high_kinetic_energy_b && (ttz_s_fl32 < TTZ_WARNING_S) && errorWasLarge_b;

        // --- 4. Rollover-Safe Timer Logic ---
        if (trigger_b && !is_timer_active_b) {
            is_timer_active_b = true;
            timer_start_time_us_u32 = currentTimeUs_u32;
        }

        prev_error_fl32 = current_error_fl32;
        prev_time_us_u32 = currentTimeUs_u32;

        // --- 5. Logical Evaluation ---
        bool logical_activate_b = false;
        
        // 5a. Evaluate predictive timer
        if (is_timer_active_b) {
            if ((currentTimeUs_u32 - timer_start_time_us_u32) <= HOLD_TIME_US) {
                logical_activate_b = true;
            } else {
                is_timer_active_b = false;
            }
        }
        
        // 5b. Reactive Hysteresis
        float upperLimit_V = voltageThreshold_V_fl32 + BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE;
        float lowerLimit_V = voltageThreshold_V_fl32 + BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE;

        if (servoVoltage_fl32 >= upperLimit_V) {
            is_voltage_fallback_active_b = true; 
        } else if (servoVoltage_fl32 <= lowerLimit_V) {
            is_voltage_fallback_active_b = false;
        }

        if (is_voltage_fallback_active_b) {
            logical_activate_b = true;
        }

        // 6. Universal Thermal Safety Governor & Energy Accumulator
        return applyThermalSafetyGovernor(logical_activate_b, servoVoltage_fl32, currentTimeUs_u32, dt_s_fl32, currentSpeedInHz_i32);
    }
};