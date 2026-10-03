#include "SignalFilter_IMM.h"

// =========================================================
// TUNING CONSTANTS
// =========================================================
// Closed-loop bandwidth of the HOLD model. Only slow drift (e.g. the foot settling) passes.
static const float s_holdBandwidthHz_fl32 = 1.5f;

// Closed-loop bandwidth range of the MOVE model, selected with the "Loadcell denoising" slider (1..255).
static const float s_moveBandwidthMinHz_fl32 = 5.0f;
static const float s_moveBandwidthMaxHz_fl32 = 60.0f;

// Expected dwell time in each mode (Markov switching). Longer = fewer mode changes.
static const float s_holdDwellTime_s_fl32 = 0.3f;
static const float s_moveDwellTime_s_fl32 = 0.15f;

// Lower bound of each mode probability, so a model can always win back quickly.
static const float s_modeProbabilityMin_fl32 = 1e-3f;

// Plate inertia: fraction of the predicted inertia force removed from the measurement (1 = full compensation).
// EXPERIMENTAL, off by default: in closed-loop simulation, removing the plate mass without adding it to the
// virtual mass made the loop unstable, and compensating with the (noisy) model acceleration added noise.
static const float s_inertiaCompensationFraction_fl32 = 0.0f;
// Defaults until the startup identification delivers values (0 = no compensation)
static const float s_inertiaCoefficientDefault_kgPerMps2_fl32 = 0.0f;
static const float s_inertiaDelayDefault_s_fl32 = 0.002f;

// Foot stiffness (kg force per m of pedal micro-motion): prior, bounds and how fast it may change
static const float s_footStiffnessMax_kgPerM_fl32 = 5000.0f;         // ~49 N/mm
static const float s_footStiffnessInitialStd_kgPerM_fl32 = 1000.0f;
static const float s_footStiffnessRandomWalk_fl32 = 200.0f * 200.0f; // (kg/m)^2 per s

// Foot reference position: follows the pedal immediately in MOVE, slowly in HOLD
static const float s_footReferenceTimeConstant_s_fl32 = 0.5f;
// Micro-motion window: larger deviations are not treated as foot reaction
static const float s_footMicroMotionMax_m_fl32 = 0.001f;

// Fraction of the self-caused foot reaction removed while holding (0 = off, 1 = full)
// EXPERIMENTAL, off by default: no benefit at 0.5 and unstable at 1.0 with a stiff foot in simulation.
static const float s_footReactionRemovalHold_fl32 = 0.0f;
// Upper bound of that correction force (kg)
static const float s_footReactionCorrectionMax_kg_fl32 = 0.5f;

// Initial and minimum values
static const float s_initialSampleInterval_s_fl32 = 1e-3f;
static const float s_sampleIntervalFilterAlpha_fl32 = 0.05f;
static const float s_measurementNoiseMin_fl32 = 1e-9f;
static const float s_innovationCovarianceMin_fl32 = 1e-12f;

static const float s_twoPi_fl32 = 6.283185307f;

// Constructor
KalmanFilterIMM::KalmanFilterIMM(float varianceEstimate_fl32)
  : outputForce_fl32(0.0f), outputForceRate_fl32(0.0f), outputFootStiffness_fl32(0.0f),
    measurementNoiseR_fl32(varianceEstimate_fl32),
    sampleInterval_s_fl32(s_initialSampleInterval_s_fl32), timeSinceLastSample_s_fl32(0.0f),
    inertiaCoefficient_kgPerMps2_fl32(s_inertiaCoefficientDefault_kgPerMps2_fl32),
    inertiaDelay_s_fl32(s_inertiaDelayDefault_s_fl32),
    delayBufferIdx_u8(0), isDelayBufferPrimed_b(false),
    footReferencePos_m_fl32(0.0f),
    isInitialized_b(false)
{
  setMeasurementVariance(varianceEstimate_fl32);
  reset(0.0f);
  isInitialized_b = false; // initialize on the first real measurement
}

void KalmanFilterIMM::setMeasurementVariance(float varianceEstimate_fl32) {
  measurementNoiseR_fl32 = (varianceEstimate_fl32 > s_measurementNoiseMin_fl32) ? varianceEstimate_fl32 : s_measurementNoiseMin_fl32;
}

void KalmanFilterIMM::setInertiaModel(float coefficient_kgPerMps2_fl32, float delay_s_fl32) {
  // signed: the sign depends on the loadcell mounting and sign convention
  inertiaCoefficient_kgPerMps2_fl32 = coefficient_kgPerMps2_fl32;
  inertiaDelay_s_fl32 = (delay_s_fl32 > 0.0f) ? delay_s_fl32 : 0.0f;
}

bool KalmanFilterIMM::isInertiaCompensationEnabled() const {
  return s_inertiaCompensationFraction_fl32 > 0.0f;
}

void KalmanFilterIMM::reset(float measurement_fl32) {
  for (uint8_t modelIdx = 0; modelIdx < NMB_MODELS; modelIdx++) {
    stateX_aafl32[modelIdx][STATE_FORCE] = measurement_fl32;
    stateX_aafl32[modelIdx][STATE_FORCE_RATE] = 0.0f;
    stateX_aafl32[modelIdx][STATE_FOOT_STIFFNESS] = 0.0f;

    for (uint8_t i = 0; i < NMB_STATES; i++) {
      for (uint8_t j = 0; j < NMB_STATES; j++) {
        stateCovarianceP_aaafl32[modelIdx][i][j] = 0.0f;
      }
    }
    stateCovarianceP_aaafl32[modelIdx][STATE_FORCE][STATE_FORCE] = measurementNoiseR_fl32;
    stateCovarianceP_aaafl32[modelIdx][STATE_FORCE_RATE][STATE_FORCE_RATE] = measurementNoiseR_fl32 * 1e4f;
    stateCovarianceP_aaafl32[modelIdx][STATE_FOOT_STIFFNESS][STATE_FOOT_STIFFNESS] = s_footStiffnessInitialStd_kgPerM_fl32 * s_footStiffnessInitialStd_kgPerM_fl32;
  }

  modeProbability_afl32[MODEL_HOLD] = 0.9f;
  modeProbability_afl32[MODEL_MOVE] = 0.1f;

  outputForce_fl32 = measurement_fl32;
  outputForceRate_fl32 = 0.0f;
  outputFootStiffness_fl32 = 0.0f;
  timeSinceLastSample_s_fl32 = 0.0f;
  isDelayBufferPrimed_b = false;
  isInitialized_b = true;
}

float KalmanFilterIMM::filteredValue(float measurement_fl32, bool newSample_b, float deltaTime_s_fl32, uint8_t modelNoiseScaling_u8,
                                     float pedalPos_m_fl32, float pedalAcc_mps2_fl32) {
  if (!isInitialized_b) {
    reset(measurement_fl32);
  }

  float dt_fl32 = constrain(deltaTime_s_fl32, 1e-5f, 5e-3f);

  // =========================================================
  // KNOWN INPUTS FROM THE ADMITTANCE MODEL (delay aligned)
  // =========================================================
  if (!isDelayBufferPrimed_b) {
    for (uint8_t i = 0; i < DELAY_BUFFER_LENGTH; i++) {
      pedalPosBuffer_afl32[i] = pedalPos_m_fl32;
      pedalAccBuffer_afl32[i] = 0.0f;
    }
    footReferencePos_m_fl32 = pedalPos_m_fl32;
    isDelayBufferPrimed_b = true;
  }
  delayBufferIdx_u8 = (delayBufferIdx_u8 + 1) % DELAY_BUFFER_LENGTH;
  pedalPosBuffer_afl32[delayBufferIdx_u8] = pedalPos_m_fl32;
  pedalAccBuffer_afl32[delayBufferIdx_u8] = pedalAcc_mps2_fl32;

  int32_t delayCycles_i32 = (int32_t)(inertiaDelay_s_fl32 / dt_fl32 + 0.5f);
  delayCycles_i32 = constrain(delayCycles_i32, 0, DELAY_BUFFER_LENGTH - 1);
  uint8_t delayedIdx_u8 = (uint8_t)((delayBufferIdx_u8 + DELAY_BUFFER_LENGTH - delayCycles_i32) % DELAY_BUFFER_LENGTH);
  float delayedPedalPos_m_fl32 = pedalPosBuffer_afl32[delayedIdx_u8];
  float delayedPedalAcc_mps2_fl32 = pedalAccBuffer_afl32[delayedIdx_u8];

  // Foot reference: jumps along with the pedal in MOVE, drifts slowly in HOLD,
  // so dx only contains the micro-motion around the current hold point.
  float referenceAlpha_fl32 = modeProbability_afl32[MODEL_MOVE]
                            + modeProbability_afl32[MODEL_HOLD] * (1.0f - expf(-dt_fl32 / s_footReferenceTimeConstant_s_fl32));
  footReferencePos_m_fl32 += referenceAlpha_fl32 * (delayedPedalPos_m_fl32 - footReferencePos_m_fl32);
  float footMicroMotion_m_fl32 = constrain(delayedPedalPos_m_fl32 - footReferencePos_m_fl32, -s_footMicroMotionMax_m_fl32, s_footMicroMotionMax_m_fl32);

  // Remove the predicted plate inertia force from the measurement
  float compensatedMeasurement_fl32 = measurement_fl32
                                    - s_inertiaCompensationFraction_fl32 * inertiaCoefficient_kgPerMps2_fl32 * delayedPedalAcc_mps2_fl32;

  // Measurement matrix H = [1, 0, -dx]
  float measurementH_afl32[NMB_STATES] = {1.0f, 0.0f, -footMicroMotion_m_fl32};

  // =========================================================
  // PROCESS NOISE FROM TARGET BANDWIDTHS
  // =========================================================
  // With continuous-equivalent measurement noise density r = R * T (T = sample interval):
  //  - random walk (HOLD):           bandwidth w = sqrt(q / r)   --> q = r * w^2
  //  - constant rate (MOVE):         bandwidth w = (q / r)^(1/4) --> q = r * w^4
  timeSinceLastSample_s_fl32 += dt_fl32;
  float noiseDensity_fl32 = measurementNoiseR_fl32 * sampleInterval_s_fl32;

  float holdOmega_fl32 = s_twoPi_fl32 * s_holdBandwidthHz_fl32;
  float qHold_fl32 = noiseDensity_fl32 * holdOmega_fl32 * holdOmega_fl32;

  float sliderValue01_fl32 = constrain(((float)modelNoiseScaling_u8 - 1.0f) / 254.0f, 0.0f, 1.0f);
  float moveBandwidthHz_fl32 = s_moveBandwidthMinHz_fl32 * powf(s_moveBandwidthMaxHz_fl32 / s_moveBandwidthMinHz_fl32, sliderValue01_fl32);
  float moveOmega_fl32 = s_twoPi_fl32 * moveBandwidthHz_fl32;
  float moveOmegaPow2_fl32 = moveOmega_fl32 * moveOmega_fl32;
  float qMove_fl32 = noiseDensity_fl32 * moveOmegaPow2_fl32 * moveOmegaPow2_fl32;

  float qFootStiffness_fl32 = s_footStiffnessRandomWalk_fl32 * dt_fl32;

  // =========================================================
  // 1. MIXING (interaction step)
  // =========================================================
  // Markov transition probabilities p[i][j] = P(model j now | model i before)
  float pStayHold_fl32 = expf(-dt_fl32 / s_holdDwellTime_s_fl32);
  float pStayMove_fl32 = expf(-dt_fl32 / s_moveDwellTime_s_fl32);
  float transition_aafl32[NMB_MODELS][NMB_MODELS] = {
    {pStayHold_fl32, 1.0f - pStayHold_fl32},
    {1.0f - pStayMove_fl32, pStayMove_fl32}
  };

  // Predicted mode probabilities c_j = sum_i p[i][j] * mu_i
  float predictedModeProbability_afl32[NMB_MODELS];
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    predictedModeProbability_afl32[j] = 0.0f;
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      predictedModeProbability_afl32[j] += transition_aafl32[i][j] * modeProbability_afl32[i];
    }
  }

  // Mixed initial state and covariance per model
  float mixedX_aafl32[NMB_MODELS][NMB_STATES];
  float mixedP_aaafl32[NMB_MODELS][NMB_STATES][NMB_STATES];
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    float mixingWeight_afl32[NMB_MODELS];
    for (uint8_t i = 0; i < NMB_MODELS; i++) {
      mixingWeight_afl32[i] = transition_aafl32[i][j] * modeProbability_afl32[i] / predictedModeProbability_afl32[j];
    }

    for (uint8_t s = 0; s < NMB_STATES; s++) {
      mixedX_aafl32[j][s] = 0.0f;
      for (uint8_t i = 0; i < NMB_MODELS; i++) {
        mixedX_aafl32[j][s] += mixingWeight_afl32[i] * stateX_aafl32[i][s];
      }
    }

    for (uint8_t r = 0; r < NMB_STATES; r++) {
      for (uint8_t c = r; c < NMB_STATES; c++) {
        float sum_fl32 = 0.0f;
        for (uint8_t i = 0; i < NMB_MODELS; i++) {
          float dxR_fl32 = stateX_aafl32[i][r] - mixedX_aafl32[j][r];
          float dxC_fl32 = stateX_aafl32[i][c] - mixedX_aafl32[j][c];
          sum_fl32 += mixingWeight_afl32[i] * (stateCovarianceP_aaafl32[i][r][c] + dxR_fl32 * dxC_fl32);
        }
        mixedP_aaafl32[j][r][c] = sum_fl32;
        mixedP_aaafl32[j][c][r] = sum_fl32;
      }
    }
  }

  // =========================================================
  // 2. PREDICT per model
  // =========================================================
  // HOLD: F = [1, 0, 0; 0, 0, 0; 0, 0, 1], force random walk, rate forced to zero
  {
    float* x = stateX_aafl32[MODEL_HOLD];
    float (*P)[NMB_STATES] = stateCovarianceP_aaafl32[MODEL_HOLD];
    float (*P0)[NMB_STATES] = mixedP_aaafl32[MODEL_HOLD];
    x[STATE_FORCE] = mixedX_aafl32[MODEL_HOLD][STATE_FORCE];
    x[STATE_FORCE_RATE] = 0.0f;
    x[STATE_FOOT_STIFFNESS] = mixedX_aafl32[MODEL_HOLD][STATE_FOOT_STIFFNESS];

    P[0][0] = P0[0][0] + qHold_fl32 * dt_fl32;
    P[0][1] = P[1][0] = 0.0f;
    P[0][2] = P[2][0] = P0[0][2];
    P[1][1] = s_measurementNoiseMin_fl32; // keep P positive definite
    P[1][2] = P[2][1] = 0.0f;
    P[2][2] = P0[2][2] + qFootStiffness_fl32;
  }

  // MOVE: F = [1, dt, 0; 0, 1, 0; 0, 0, 1], white noise on the force rate
  // Q = q * [dt^3/3, dt^2/2, 0; dt^2/2, dt, 0; 0, 0, 0] + diag(0, 0, q_k * dt)
  {
    float* x = stateX_aafl32[MODEL_MOVE];
    float (*P)[NMB_STATES] = stateCovarianceP_aaafl32[MODEL_MOVE];
    float (*P0)[NMB_STATES] = mixedP_aaafl32[MODEL_MOVE];
    float dtPow2_fl32 = dt_fl32 * dt_fl32;
    float dtPow3_fl32 = dtPow2_fl32 * dt_fl32;

    x[STATE_FORCE] = mixedX_aafl32[MODEL_MOVE][STATE_FORCE] + dt_fl32 * mixedX_aafl32[MODEL_MOVE][STATE_FORCE_RATE];
    x[STATE_FORCE_RATE] = mixedX_aafl32[MODEL_MOVE][STATE_FORCE_RATE];
    x[STATE_FOOT_STIFFNESS] = mixedX_aafl32[MODEL_MOVE][STATE_FOOT_STIFFNESS];

    P[0][0] = P0[0][0] + dt_fl32 * (P0[0][1] + P0[1][0]) + dtPow2_fl32 * P0[1][1] + qMove_fl32 * dtPow3_fl32 / 3.0f;
    P[0][1] = P[1][0] = P0[0][1] + dt_fl32 * P0[1][1] + qMove_fl32 * dtPow2_fl32 / 2.0f;
    P[0][2] = P[2][0] = P0[0][2] + dt_fl32 * P0[1][2];
    P[1][1] = P0[1][1] + qMove_fl32 * dt_fl32;
    P[1][2] = P[2][1] = P0[1][2];
    P[2][2] = P0[2][2] + qFootStiffness_fl32;
  }

  // =========================================================
  // 3. UPDATE per model and mode probabilities
  // =========================================================
  if (newSample_b) {
    // Averaged sample interval for the process noise scaling
    sampleInterval_s_fl32 += s_sampleIntervalFilterAlpha_fl32 * (timeSinceLastSample_s_fl32 - sampleInterval_s_fl32);
    timeSinceLastSample_s_fl32 = 0.0f;

    float logLikelihood_afl32[NMB_MODELS];
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      float* x = stateX_aafl32[j];
      float (*P)[NMB_STATES] = stateCovarianceP_aaafl32[j];

      // P * H'
      float pHt_afl32[NMB_STATES];
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        pHt_afl32[r] = 0.0f;
        for (uint8_t c = 0; c < NMB_STATES; c++) {
          pHt_afl32[r] += P[r][c] * measurementH_afl32[c];
        }
      }

      // Innovation and its covariance S = H * P * H' + R
      float predictedMeasurement_fl32 = 0.0f;
      float innovationCov_fl32 = measurementNoiseR_fl32;
      for (uint8_t c = 0; c < NMB_STATES; c++) {
        predictedMeasurement_fl32 += measurementH_afl32[c] * x[c];
        innovationCov_fl32 += measurementH_afl32[c] * pHt_afl32[c];
      }
      if (innovationCov_fl32 < s_innovationCovarianceMin_fl32) innovationCov_fl32 = s_innovationCovarianceMin_fl32;
      float invInnovationCov_fl32 = 1.0f / innovationCov_fl32;
      float innovation_fl32 = compensatedMeasurement_fl32 - predictedMeasurement_fl32;

      // Kalman gain K = P * H' / S
      // The foot stiffness is only learned in HOLD. In MOVE it is a "consider" parameter
      // (Schmidt-Kalman): it is used in the prediction but not updated, so presses cannot corrupt it.
      float gain_afl32[NMB_STATES];
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        gain_afl32[r] = pHt_afl32[r] * invInnovationCov_fl32;
      }
      if (j == MODEL_MOVE) {
        gain_afl32[STATE_FOOT_STIFFNESS] = 0.0f;
      }

      for (uint8_t r = 0; r < NMB_STATES; r++) {
        x[r] += gain_afl32[r] * innovation_fl32;
      }

      // Joseph form, valid for the (partially zeroed) Schmidt gain:
      // P = (I - K*H) * P * (I - K*H)' + K * R * K'
      float iMinusKh_aafl32[NMB_STATES][NMB_STATES];
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        for (uint8_t c = 0; c < NMB_STATES; c++) {
          iMinusKh_aafl32[r][c] = ((r == c) ? 1.0f : 0.0f) - gain_afl32[r] * measurementH_afl32[c];
        }
      }
      float temp_aafl32[NMB_STATES][NMB_STATES];
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        for (uint8_t c = 0; c < NMB_STATES; c++) {
          float sum_fl32 = 0.0f;
          for (uint8_t k = 0; k < NMB_STATES; k++) {
            sum_fl32 += iMinusKh_aafl32[r][k] * P[k][c];
          }
          temp_aafl32[r][c] = sum_fl32;
        }
      }
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        for (uint8_t c = r; c < NMB_STATES; c++) {
          float sum_fl32 = measurementNoiseR_fl32 * gain_afl32[r] * gain_afl32[c];
          for (uint8_t k = 0; k < NMB_STATES; k++) {
            sum_fl32 += temp_aafl32[r][k] * iMinusKh_aafl32[c][k];
          }
          P[r][c] = sum_fl32;
          P[c][r] = sum_fl32;
        }
      }
      for (uint8_t r = 0; r < NMB_STATES; r++) {
        if (P[r][r] < s_measurementNoiseMin_fl32) P[r][r] = s_measurementNoiseMin_fl32;
      }

      // Gaussian log likelihood (constant term dropped, it cancels in the normalization)
      logLikelihood_afl32[j] = -0.5f * (innovation_fl32 * innovation_fl32 * invInnovationCov_fl32 + logf(innovationCov_fl32));
    }

    // mu_j ~ Lambda_j * c_j, evaluated relative to the max log likelihood to avoid underflow
    float maxLogLikelihood_fl32 = max(logLikelihood_afl32[MODEL_HOLD], logLikelihood_afl32[MODEL_MOVE]);
    float normalization_fl32 = 0.0f;
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] = expf(logLikelihood_afl32[j] - maxLogLikelihood_fl32) * predictedModeProbability_afl32[j];
      normalization_fl32 += modeProbability_afl32[j];
    }
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] /= normalization_fl32;
    }
  } else {
    // No fresh measurement: only the Markov prior changes the mode probabilities
    for (uint8_t j = 0; j < NMB_MODELS; j++) {
      modeProbability_afl32[j] = predictedModeProbability_afl32[j];
    }
  }

  // Foot stiffness is physically non-negative and bounded; its uncertainty never exceeds the prior
  const float footStiffnessVarianceMax_fl32 = s_footStiffnessInitialStd_kgPerM_fl32 * s_footStiffnessInitialStd_kgPerM_fl32;
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    stateX_aafl32[j][STATE_FOOT_STIFFNESS] = constrain(stateX_aafl32[j][STATE_FOOT_STIFFNESS], 0.0f, s_footStiffnessMax_kgPerM_fl32);
    if (stateCovarianceP_aaafl32[j][2][2] > footStiffnessVarianceMax_fl32) {
      stateCovarianceP_aaafl32[j][2][2] = footStiffnessVarianceMax_fl32;
    }
  }

  // Keep every model alive so it can win back quickly
  if (modeProbability_afl32[MODEL_HOLD] < s_modeProbabilityMin_fl32) modeProbability_afl32[MODEL_HOLD] = s_modeProbabilityMin_fl32;
  if (modeProbability_afl32[MODEL_MOVE] < s_modeProbabilityMin_fl32) modeProbability_afl32[MODEL_MOVE] = s_modeProbabilityMin_fl32;
  float probabilitySum_fl32 = modeProbability_afl32[MODEL_HOLD] + modeProbability_afl32[MODEL_MOVE];
  modeProbability_afl32[MODEL_HOLD] /= probabilitySum_fl32;
  modeProbability_afl32[MODEL_MOVE] /= probabilitySum_fl32;

  // =========================================================
  // 4. OUTPUT: probability weighted combination
  // =========================================================
  float intentForce_fl32 = 0.0f;
  outputForceRate_fl32 = 0.0f;
  outputFootStiffness_fl32 = 0.0f;
  for (uint8_t j = 0; j < NMB_MODELS; j++) {
    intentForce_fl32 += modeProbability_afl32[j] * stateX_aafl32[j][STATE_FORCE];
    outputForceRate_fl32 += modeProbability_afl32[j] * stateX_aafl32[j][STATE_FORCE_RATE];
    outputFootStiffness_fl32 += modeProbability_afl32[j] * stateX_aafl32[j][STATE_FOOT_STIFFNESS];
  }

  // Denoised foot force = intent - foot reaction. While holding, part of the self-caused
  // foot reaction is removed, which breaks the noise -> motion -> foot reaction loop.
  float footReaction_fl32 = outputFootStiffness_fl32 * footMicroMotion_m_fl32;
  float footReactionCorrection_fl32 = constrain(s_footReactionRemovalHold_fl32 * modeProbability_afl32[MODEL_HOLD] * footReaction_fl32,
                                                -s_footReactionCorrectionMax_kg_fl32, s_footReactionCorrectionMax_kg_fl32);
  outputForce_fl32 = intentForce_fl32 - footReaction_fl32 + footReactionCorrection_fl32;

  return outputForce_fl32;
}

float KalmanFilterIMM::changeVelocity() const {
  return outputForceRate_fl32;
}

float KalmanFilterIMM::moveProbability() const {
  return modeProbability_afl32[MODEL_MOVE];
}

float KalmanFilterIMM::holdProbability() const {
  return modeProbability_afl32[MODEL_HOLD];
}

float KalmanFilterIMM::footStiffness_kgPerM() const {
  return outputFootStiffness_fl32;
}
