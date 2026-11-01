#ifndef SIGNAL_FILTER_IMM_H
#define SIGNAL_FILTER_IMM_H

#include "Arduino.h"
#include <stdint.h>
#include "Main.h"

// Interacting Multiple Model (IMM) Kalman filter for the pedal force, coupled to the admittance model.
//
// The loadcell does not only measure the foot's intent:
//   z = F_intent - k_foot * dx + c_m * a_pedal + noise
//  - c_m * a_pedal: inertia of the pedal plate, known from the admittance model's acceleration
//  - k_foot * dx:   passive reaction of the compliant foot to pedal micro-motion dx (k_foot estimated online)
//
// Two hypotheses about F_intent run in parallel and are blended by how well each explains the samples:
//  - HOLD: intent is (almost) constant  --> strong smoothing, learns k_foot
//  - MOVE: intent changes at a rate     --> low-lag tracking of real presses
// Both models share the state [F_intent; dF_intent/dt; k_foot] so they can be mixed.
class KalmanFilterIMM {
public:
  // Constructor
  KalmanFilterIMM(float varianceEstimate_fl32);

  // Main filter function
  // measurement_fl32:     force measurement at the foot point (kg)
  // newSample_b:          true if the measurement is a fresh loadcell sample, false if it is a held value
  // deltaTime_s_fl32:     time since the last call in seconds
  // modelNoiseScaling_u8: 1..255, bandwidth of the MOVE model (slow to fast)
  // pedalPos_m_fl32:      admittance model position in task space (m), previous cycle
  // pedalAcc_mps2_fl32:   admittance model acceleration in task space (m/s^2), previous cycle
  float IRAM_ATTR_FLAG filteredValue(float measurement_fl32, bool newSample_b, float deltaTime_s_fl32, uint8_t modelNoiseScaling_u8,
                                     float pedalPos_m_fl32, float pedalAcc_mps2_fl32);

  // Getters for the state variables
  float IRAM_ATTR_FLAG changeVelocity() const;
  float IRAM_ATTR_FLAG moveProbability() const;
  float IRAM_ATTR_FLAG holdProbability() const;
  float footStiffness_kgPerM() const;

  // Measurement noise variance in (measurement unit)^2
  void IRAM_ATTR_FLAG setMeasurementVariance(float varianceEstimate_fl32);

  // Plate inertia model: force = coefficient * acceleration(t - delay)
  // coefficient in kg (force) per m/s^2 (signed), delay in seconds (servo + ADC)
  void setInertiaModel(float coefficient_kgPerMps2_fl32, float delay_s_fl32);

  // True if the plate inertia compensation is enabled (only then the startup identification is needed)
  bool isInertiaCompensationEnabled() const;

  // Re-initialize all states at the given force (e.g. after switching filters)
  void reset(float measurement_fl32);

private:
  static const uint8_t NMB_MODELS = 2;
  static const uint8_t NMB_STATES = 3;
  static const uint8_t MODEL_HOLD = 0;
  static const uint8_t MODEL_MOVE = 1;
  static const uint8_t STATE_FORCE = 0;
  static const uint8_t STATE_FORCE_RATE = 1;
  static const uint8_t STATE_FOOT_STIFFNESS = 2;
  static const uint8_t DELAY_BUFFER_LENGTH = 64;

  // Per model state vector [force; force rate; foot stiffness] and covariance (3x3)
  float stateX_aafl32[NMB_MODELS][NMB_STATES];
  float stateCovarianceP_aaafl32[NMB_MODELS][NMB_STATES][NMB_STATES];

  // Mode probabilities [HOLD, MOVE]
  float modeProbability_afl32[NMB_MODELS];

  // Blended output
  float outputForce_fl32;
  float outputForceRate_fl32;
  float outputFootStiffness_fl32;

  // Measurement noise covariance R (scalar)
  float measurementNoiseR_fl32;

  // Averaged time between fresh samples, used to derive the process noise from a target bandwidth
  float sampleInterval_s_fl32;
  float timeSinceLastSample_s_fl32;

  // Plate inertia model
  float inertiaCoefficient_kgPerMps2_fl32;
  float inertiaDelay_s_fl32;

  // Delay line for the admittance model states, aligns them with the loadcell reading
  float pedalPosBuffer_afl32[DELAY_BUFFER_LENGTH];
  float pedalAccBuffer_afl32[DELAY_BUFFER_LENGTH];
  uint8_t delayBufferIdx_u8;
  bool isDelayBufferPrimed_b;

  // Reference position of the foot: the pedal micro-motion dx is measured relative to it
  float footReferencePos_m_fl32;

  bool isInitialized_b;
};

#endif // SIGNAL_FILTER_IMM_H
