#ifndef PLATE_INERTIA_IDENTIFICATION_H
#define PLATE_INERTIA_IDENTIFICATION_H

#include "Arduino.h"
#include <stdint.h>
#include "Main.h"

// Identifies the pedal plate inertia as seen by the loadcell, right after homing (no foot on the pedal).
//
// The pedal is moved along x(t) = A * (1 - cos(w t)) in task space (pedal arc) at two frequencies.
// Lock-in demodulation of the loadcell force against the commanded acceleration a(t) = A w^2 cos(w t) gives
//   H(w) = (c_m - k_g / w^2) * exp(j w tau)
// c_m:  inertia coefficient (kg force per m/s^2, signed), k_g: gravity / linkage stiffness, tau: servo + ADC delay.
// Two frequencies separate c_m from k_g; the phase at the higher frequency gives tau.
class PlateInertiaIdentification {
public:
  PlateInertiaIdentification();

  // Start the identification
  // measurementVariance_fl32: loadcell noise variance in pedal force units (kg^2), used to detect a foot on the pedal
  void start(float measurementVariance_fl32);

  bool isActive() const { return isActive_b; }

  // Advance by one control cycle. Returns the commanded pedal arc offset from the rest position in meters.
  // pedalForce_kg_fl32 is only evaluated when newSample_b is true.
  float IRAM_ATTR_FLAG update(float deltaTime_s_fl32, bool newSample_b, float pedalForce_kg_fl32);

  // Results, valid after isActive() turned false
  bool isResultValid() const { return isResultValid_b; }
  float getInertiaCoefficient_kgPerMps2() const { return inertiaCoefficient_kgPerMps2_fl32; }
  float getDelay_s() const { return delay_s_fl32; }
  float getGravityStiffness_kgPerM() const { return gravityStiffness_kgPerM_fl32; }

private:
  static const uint8_t NMB_TONES = 2;

  void finish();

  bool isActive_b;
  bool isResultValid_b;

  uint8_t toneIdx_u8;
  float toneTime_s_fl32;

  float measurementVariance_fl32;

  // Lock-in accumulators per tone (only over the measured periods)
  float sumCos_afl32[NMB_TONES];
  float sumSin_afl32[NMB_TONES];
  float sumForce_afl32[NMB_TONES];
  float sumForceSq_afl32[NMB_TONES];
  uint32_t sampleCount_au32[NMB_TONES];

  float inertiaCoefficient_kgPerMps2_fl32;
  float delay_s_fl32;
  float gravityStiffness_kgPerM_fl32;
};

#endif // PLATE_INERTIA_IDENTIFICATION_H
