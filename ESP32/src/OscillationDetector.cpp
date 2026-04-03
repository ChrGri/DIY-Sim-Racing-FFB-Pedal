#include "OscillationDetector.h"
#include <Arduino.h>     // Required for abs() and constrain()
#include "esp_timer.h"   // Required for esp_timer_get_time()

OscillationDetector::OscillationDetector(float threshold, float attack, float decay)
    : m_lastLoadCell_kg(0.0f),
      m_oscillationIntensity_01(0.0f),
      m_lastTimeUs(0),
      m_threshold(threshold),
      m_attackRate(attack),
      m_decayRate(decay)
{
}

// Internal helper to apply the Attack/Decay math
float OscillationDetector::processIntensity(float activity, float dt_s) {
    if (activity > m_threshold) {
        // High frequency chatter detected: ramp up oscillation intensity quickly
        m_oscillationIntensity_01 += m_attackRate * dt_s;
    } else {
        // Stable signal: allow intensity to decay gradually
        m_oscillationIntensity_01 -= m_decayRate * dt_s;
    }

    // Keep the output safely clamped within [0.0, 1.0]
    m_oscillationIntensity_01 = constrain(m_oscillationIntensity_01, 0.0f, 1.0f);

    return m_oscillationIntensity_01;
}

// Option 1: Calculate the derivative internally
float OscillationDetector::update(float loadCellReadingKg_fl32) {
    uint64_t currentTimeUs = esp_timer_get_time();

    if (m_lastTimeUs == 0) {
        m_lastTimeUs = currentTimeUs;
        m_lastLoadCell_kg = loadCellReadingKg_fl32;
        return m_oscillationIntensity_01;
    }

    float dt_s = (float)(currentTimeUs - m_lastTimeUs) * 1e-6f;
    m_lastTimeUs = currentTimeUs;

    // Protect against zero division or extreme time leaps
    if (dt_s <= 0.0f || dt_s > 0.1f) return m_oscillationIntensity_01;

    // Calculate the rate of force change (dF/dt)
    float currentForceRate_kgs = (loadCellReadingKg_fl32 - m_lastLoadCell_kg) / dt_s;
    m_lastLoadCell_kg = loadCellReadingKg_fl32;

    return processIntensity(abs(currentForceRate_kgs), dt_s);
}

// Option 2: Use the externally provided force velocity
float OscillationDetector::updateWithExternalRate(float forceRate_kgs) {
    uint64_t currentTimeUs = esp_timer_get_time();

    if (m_lastTimeUs == 0) {
        m_lastTimeUs = currentTimeUs;
        return m_oscillationIntensity_01;
    }

    float dt_s = (float)(currentTimeUs - m_lastTimeUs) * 1e-6f;
    m_lastTimeUs = currentTimeUs;

    if (dt_s <= 0.0f || dt_s > 0.1f) return m_oscillationIntensity_01;

    // Bypass the derivative calculation and directly process the intensity
    return processIntensity(abs(forceRate_kgs), dt_s);
}

float OscillationDetector::getIntensity() const {
    return m_oscillationIntensity_01;
}

void OscillationDetector::reset() {
    m_lastLoadCell_kg = 0.0f;
    m_oscillationIntensity_01 = 0.0f;
    m_lastTimeUs = 0; 
}

void OscillationDetector::setParameters(float threshold, float attack, float decay) {
    m_threshold = threshold;
    m_attackRate = attack;
    m_decayRate = decay;
}