#pragma once
#include <stdint.h> // Required for uint64_t

class OscillationDetector {
public:
    // Constructor with default values tuned for stability
    OscillationDetector(float threshold = 300.0f, float attack = 20.0f, float decay = 3.0f);

    // Main function called in every control loop cycle
    float update(float loadCellReadingKg_fl32);

    // Getter for the current oscillation intensity without recalculating
    float getIntensity() const;

    // Resets the internal state (useful for system resets/restarts)
    void reset();

    // Setter to adjust parameters at runtime (e.g., via a web interface)
    void setParameters(float threshold, float attack, float decay);

private:
    // State variables
    float m_lastLoadCell_kg;
    float m_oscillationIntensity_01;
    uint64_t m_lastTimeUs; // Stores the timestamp of the last update

    // Parameters
    float m_threshold;
    float m_attackRate;
    float m_decayRate;
};