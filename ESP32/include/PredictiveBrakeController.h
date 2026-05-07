#pragma once

#include <Arduino.h>
#include <math.h>

/**
 * @brief Predictive brake resistor controller (Time-To-Impact & Foot Dynamics Observer)
 * 100% timer-rollover safe (Unsigned Delta Logic), thermal lockout & Schmitt hysteresis.
 */
class PredictiveBrakeController {
private:
    // --- 1. Tuning Parameters (Prediction) ---
    const float FOOT_ESCAPE_RATE_KG_S = -100.0f; 
    const float TTZ_WARNING_S = 0.04f; 
    const int32_t MIN_SPEED_HZ = 30000; 
    const uint32_t HOLD_TIME_US = 30000; 
    const int32_t ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32 = -100; 

    // --- 2. Hysteresis Parameters (Reactive Overvoltage Protection) ---
    const float BRAKE_RESISTOR_UPPER_THRESHOLD_VOLTAGE = 4.0f;
    const float BRAKE_RESISTOR_LOWER_THRESHOLD_VOLTAGE = 1.0f;
    
    // NEW: No longer 'const', allowing the value to be set externally.
    // Default value is 38.0V (for a 36V power supply).
    float voltageThreshold_V_fl32 = 38.0f; 

    // --- 3. Safety Parameters (Thermal Protection) ---
    const uint32_t MAX_CONTINUOUS_ON_TIME_US = 500000; 
    const uint32_t THERMAL_COOLDOWN_TIME_US = 1000000; 

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

public:
    PredictiveBrakeController() {}

    // =========================================================
    // Setter function for the voltage limit
    // =========================================================
    /**
     * @brief Sets the reactive overvoltage threshold base value.
     * @param threshold_V The new threshold in Volts (e.g., 38.0f for 36V power supplies)
     */
    void setVoltageThreshold(float threshold_V) {
        // Optional: A sanity check to prevent accidentally setting it to 5V or 1000V.
        if (threshold_V > 37.0f && threshold_V < 80.0f) {
            voltageThreshold_V_fl32 = threshold_V; 
        }
    }

    void Reset() {
        is_initialized_b = false;
        is_timer_active_b = false;
        is_voltage_fallback_active_b = false;
        is_hardware_active_b = false;
        is_in_lockout_b = false;
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
            return false;
        }

        uint32_t dt_us_u32 = currentTimeUs_u32 - prev_time_us_u32;
        if (dt_us_u32 == 0) dt_us_u32 = 1000; 
        float dt_s_fl32 = (float)dt_us_u32 * 1e-6f;
        float current_error_fl32 = (float)servoPositionError_i32;

        // --- 1. Time-To-Zero (TTZ) Calculation ---
        //float d_error_fl32 = (current_error_fl32 - prev_error_fl32) / dt_s_fl32;
        float d_error_fl32 = servoPositionErrorChangeRateInStepsPerSecond_fl32;
        float ttz_s_fl32 = 999.0f;
        
        if ((current_error_fl32 * d_error_fl32) < 0.0f) {
            ttz_s_fl32 = fabsf(current_error_fl32 / d_error_fl32);
        }

        // --- 2. Foot Dynamics & Kinetic Checks ---
        //bool foot_is_escaping_b = (forceVelEst_fl32 < FOOT_ESCAPE_RATE_KG_S);
        bool high_kinetic_energy_b = (abs(currentSpeedInHz_i32) > MIN_SPEED_HZ);

        // overwerite foot escaping value. Sometimes positive values have been seen here too. 
        bool foot_is_dynamic_b = ( fabsf(forceVelEst_fl32) > fabsf(FOOT_ESCAPE_RATE_KG_S) );

        bool errorWasLarge_b = (prev_error_fl32 < ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32) || (current_error_fl32 < ERROR_WAS_LARGE_TRHESHOLD_STEPS_I32);

        // --- 3. The Precision Trigger ("Sniper Trigger") ---
        bool trigger_b = foot_is_dynamic_b && high_kinetic_energy_b && (ttz_s_fl32 < TTZ_WARNING_S) && errorWasLarge_b;

        // --- 4. Rollover-Safe Timer Logic ---
        if (trigger_b) {
            is_timer_active_b = true;
            timer_start_time_us_u32 = currentTimeUs_u32; // Simply reset the start time
        }

        prev_error_fl32 = current_error_fl32;
        prev_time_us_u32 = currentTimeUs_u32;

        // --- 5. Logical Evaluation ---
        bool logical_activate_b = false;
        
        // 5a. Evaluate predictive timer (100% rollover-safe)
        if (is_timer_active_b) {
            if ((currentTimeUs_u32 - timer_start_time_us_u32) <= HOLD_TIME_US) {
                logical_activate_b = true;
            } else {
                is_timer_active_b = false; // Timer has expired
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

        // =========================================================
        // 6. THERMAL OVERLOAD PROTECTION (Also rollover-safe)
        // =========================================================
        if (is_in_lockout_b) {
            if ((currentTimeUs_u32 - lockout_start_time_us_u32) > THERMAL_COOLDOWN_TIME_US) {
                is_in_lockout_b = false; 
            } else {
                logical_activate_b = false; 
            }
        }

        if (logical_activate_b) {
            if (!is_hardware_active_b) {
                is_hardware_active_b = true;
                active_start_time_us_u32 = currentTimeUs_u32;
            } else {
                if ((currentTimeUs_u32 - active_start_time_us_u32) > MAX_CONTINUOUS_ON_TIME_US) {
                    // EMERGENCY SHUTDOWN!
                    is_hardware_active_b = false;
                    logical_activate_b = false;       
                    
                    is_in_lockout_b = true;           
                    lockout_start_time_us_u32 = currentTimeUs_u32;
                    
                    // Hard reset of all internal triggers!
                    is_voltage_fallback_active_b = false; 
                    is_timer_active_b = false; 
                }
            }
        } else {
            is_hardware_active_b = false;
        }

        return logical_activate_b;
    }
};