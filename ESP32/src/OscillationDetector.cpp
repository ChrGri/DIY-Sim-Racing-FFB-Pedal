#include "OscillationDetector.h"
#include <Arduino.h>     // Required for constrain()
#include "esp_timer.h"   // Required for esp_timer_get_time()

OscillationDetector::OscillationDetector(float threshold, float attack, float decay)
    : m_lastLoadCell_kg(0.0f),
      m_oscillationIntensity_01(0.0f),
      m_signFlipEnergy(0.0f),
      m_lastSign(0),
      m_lastTimeUs(0),
      m_threshold(threshold),
      m_attackRate(attack),
      m_decayRate(decay)
{
}

void OscillationDetector::reset() {
    m_oscillationIntensity_01 = 0.0f;
    m_signFlipEnergy = 0.0f;
    m_lastSign = 0;
    m_lastTimeUs = 0;
}

void OscillationDetector::setParameters(float threshold, float attack, float decay) {
    m_threshold = threshold;
    m_attackRate = attack;
    m_decayRate = decay;
}

float OscillationDetector::getIntensity() const {
    return m_oscillationIntensity_01;
}

float OscillationDetector::getSignFlipEnergy() const {
    return m_signFlipEnergy;
}

uint8_t OscillationDetector::getMonitorValue() const {
    return (uint8_t)(m_signFlipEnergy * 255.0f);
}

// Combined logic using Force Rate and Sign Flips (Zero-Crossings)
float OscillationDetector::processIntensity(float forceRate_kgs, float dt_s) {
    
    // 1. Sign-Flip Detection (Frequency Monitor)
    // Determine movement direction: 1 (pushing), -1 (pulling), 0 (stable)
    int8_t currentSign = (forceRate_kgs > SIGN_SENSITIVITY) ? 1 : 
                         (forceRate_kgs < -SIGN_SENSITIVITY ? -1 : 0);
    
    if (currentSign != 0 && currentSign != m_lastSign) {
        // Significant direction change detected: increase frequency energy
        m_signFlipEnergy += ENERGY_INC;
        m_lastSign = currentSign;
    }

    // Leaky integrator for energy: constant decay over time
    m_signFlipEnergy -= ENERGY_DECAY * dt_s;
    m_signFlipEnergy = constrain(m_signFlipEnergy, 0.0f, 1.0f);

    // 2. Combined Logic for Attack/Decay
    // We only ramp up intensity if magnitude is high AND energy indicates a recurring oscillation.
    // Human kicks are fast but mostly unidirectional (energy stays low).
    // Mechanical oscillations alternate sign rapidly (energy stays high).
    float activity = abs(forceRate_kgs);
    
    if (activity > m_threshold && m_signFlipEnergy > ENERGY_THRESHOLD) {
        // High frequency chatter detected: increase damping intensity
        m_oscillationIntensity_01 += m_attackRate * dt_s;
    } else {
        // System is stable or unidirectional: decay intensity
        m_oscillationIntensity_01 -= m_decayRate * dt_s;
    }

    // Keep the intensity safely clamped within [0.0, 1.0]
    m_oscillationIntensity_01 = constrain(m_oscillationIntensity_01, 0.0f, 1.0f);

    return m_oscillationIntensity_01;
}

float OscillationDetector::update(float loadCellReadingKg_fl32) {
    uint64_t currentTimeUs = esp_timer_get_time();

    if (m_lastTimeUs == 0) {
        m_lastTimeUs = currentTimeUs;
        m_lastLoadCell_kg = loadCellReadingKg_fl32;
        return m_oscillationIntensity_01;
    }

    float dt_s = (float)(currentTimeUs - m_lastTimeUs) * 1e-6f;
    m_lastTimeUs = currentTimeUs;

    // Guard against extreme time leaps or zero division
    if (dt_s <= 0.0f || dt_s > 0.1f) return m_oscillationIntensity_01;

    float currentForceRate_kgs = (loadCellReadingKg_fl32 - m_lastLoadCell_kg) / dt_s;
    m_lastLoadCell_kg = loadCellReadingKg_fl32;

    return processIntensity(currentForceRate_kgs, dt_s);
}

float OscillationDetector::updateWithExternalRate(float forceRate_kgs) {
    uint64_t currentTimeUs = esp_timer_get_time();

    if (m_lastTimeUs == 0) {
        m_lastTimeUs = currentTimeUs;
        return m_oscillationIntensity_01;
    }

    float dt_s = (float)(currentTimeUs - m_lastTimeUs) * 1e-6f;
    m_lastTimeUs = currentTimeUs;

    if (dt_s <= 0.0f || dt_s > 0.1f) return m_oscillationIntensity_01;

    return processIntensity(forceRate_kgs, dt_s);
}