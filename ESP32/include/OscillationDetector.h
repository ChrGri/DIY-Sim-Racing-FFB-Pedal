#pragma once
#include <stdint.h> // Required for uint64_t

/**
 * @brief Frequency-aware Oscillation Detector for Admittance Control.
 * * This detector uses a combination of Force Rate Magnitude (dF/dt) and 
 * Sign-Flip Energy (Zero-Crossing frequency) to distinguish between 
 * intentional fast human inputs and mechanical limit cycle oscillations.
 */
class OscillationDetector {
public:
    // Constructor with default values tuned for stability based on offline simulation
    OscillationDetector(float threshold = 400.0f, float attack = 30.0f, float decay = 5.0f);

    // Calculates the force rate (derivative) internally and updates state
    float update(float loadCellReadingKg_fl32);

    // Uses an externally provided force rate (e.g., forceVelEst_fl32)
    float updateWithExternalRate(float forceRate_kgs);

    // Getters for telemetry
    float getIntensity() const;
    float getSignFlipEnergy() const;
    uint8_t getMonitorValue() const; // Returns energy mapped to 0-255

    // Resets the internal state
    void reset();

    // Setter to adjust parameters at runtime
    void setParameters(float threshold, float attack, float decay);

private:
    // State variables
    float m_lastLoadCell_kg;
    float m_oscillationIntensity_01;
    float m_signFlipEnergy;
    int8_t m_lastSign;
    uint64_t m_lastTimeUs;

    // Parameters
    float m_threshold;
    float m_attackRate;
    float m_decayRate;

    // Internal constants for sign-flip logic
    const float ENERGY_INC = 0.4f;       // Energy added per sign flip
    const float ENERGY_DECAY = 40.0f;    // Energy decay rate per second
    const float ENERGY_THRESHOLD = 0.3f;  // Minimum energy to allow AOM activation
    const float SIGN_SENSITIVITY = 50.0f; // Minimum rate [kg/s] to count as a sign flip

    // Internal helper function to calculate the combined logic
    float processIntensity(float forceRate_kgs, float dt_s);
};