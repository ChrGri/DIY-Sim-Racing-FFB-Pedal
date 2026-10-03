#include "PlateInertiaIdentification.h"

// =========================================================
// IDENTIFICATION SETTINGS
// =========================================================
// Excitation amplitude on the pedal arc (m). x(t) = A * (1 - cos(w t)) stays on the pressed side of the rest position.
static const float s_excitationAmplitude_m_fl32 = 0.003f;

// Two tones: frequency and number of evaluated periods. Each tone adds one ramp-in and one ramp-out period.
static const float s_toneFrequencyHz_afl32[2] = {3.0f, 7.0f};
static const uint8_t s_toneMeasuredPeriods_au8[2] = {4, 8};

// Plausibility limits
static const uint32_t s_minSamplesPerTone_u32 = 50;
static const float s_maxMeanForce_kg_fl32 = 1.0f;          // larger: foot on the pedal
static const float s_maxResidualVarianceFactor_fl32 = 100.0f; // residual variance vs. loadcell noise variance
static const float s_minAbsInertiaCoefficient_fl32 = 0.002f;
static const float s_maxAbsInertiaCoefficient_fl32 = 0.3f;
static const float s_maxDelay_s_fl32 = 0.008f;

static const float s_twoPi_fl32 = 6.283185307f;

PlateInertiaIdentification::PlateInertiaIdentification()
  : isActive_b(false), isResultValid_b(false), toneIdx_u8(0), toneTime_s_fl32(0.0f),
    measurementVariance_fl32(1e-6f),
    inertiaCoefficient_kgPerMps2_fl32(0.0f), delay_s_fl32(0.0f), gravityStiffness_kgPerM_fl32(0.0f)
{
  for (uint8_t toneIdx = 0; toneIdx < NMB_TONES; toneIdx++) {
    sumCos_afl32[toneIdx] = 0.0f;
    sumSin_afl32[toneIdx] = 0.0f;
    sumForce_afl32[toneIdx] = 0.0f;
    sumForceSq_afl32[toneIdx] = 0.0f;
    sampleCount_au32[toneIdx] = 0;
  }
}

void PlateInertiaIdentification::start(float measurementVariance_fl32) {
  for (uint8_t toneIdx = 0; toneIdx < NMB_TONES; toneIdx++) {
    sumCos_afl32[toneIdx] = 0.0f;
    sumSin_afl32[toneIdx] = 0.0f;
    sumForce_afl32[toneIdx] = 0.0f;
    sumForceSq_afl32[toneIdx] = 0.0f;
    sampleCount_au32[toneIdx] = 0;
  }
  this->measurementVariance_fl32 = (measurementVariance_fl32 > 1e-9f) ? measurementVariance_fl32 : 1e-9f;
  toneIdx_u8 = 0;
  toneTime_s_fl32 = 0.0f;
  isResultValid_b = false;
  isActive_b = true;
}

float PlateInertiaIdentification::update(float deltaTime_s_fl32, bool newSample_b, float pedalForce_kg_fl32) {
  if (!isActive_b) {
    return 0.0f;
  }

  toneTime_s_fl32 += deltaTime_s_fl32;

  float omega_fl32 = s_twoPi_fl32 * s_toneFrequencyHz_afl32[toneIdx_u8];
  float periodPosition_fl32 = toneTime_s_fl32 * s_toneFrequencyHz_afl32[toneIdx_u8];
  float measuredPeriods_fl32 = (float)s_toneMeasuredPeriods_au8[toneIdx_u8];

  // Amplitude envelope: ramp in over the first period, ramp out over the last period
  float envelope_fl32;
  if (periodPosition_fl32 < 1.0f) {
    envelope_fl32 = periodPosition_fl32;
  } else if (periodPosition_fl32 < 1.0f + measuredPeriods_fl32) {
    envelope_fl32 = 1.0f;
  } else if (periodPosition_fl32 < 2.0f + measuredPeriods_fl32) {
    envelope_fl32 = 2.0f + measuredPeriods_fl32 - periodPosition_fl32;
  } else {
    // tone finished, continue with the next one
    toneIdx_u8++;
    toneTime_s_fl32 = 0.0f;
    if (toneIdx_u8 >= NMB_TONES) {
      finish();
    }
    return 0.0f;
  }

  float phase_fl32 = omega_fl32 * toneTime_s_fl32;
  float cosPhase_fl32 = cosf(phase_fl32);

  // Lock-in accumulation over the full-amplitude periods only
  if (newSample_b && (periodPosition_fl32 >= 1.0f) && (periodPosition_fl32 < 1.0f + measuredPeriods_fl32)) {
    sumCos_afl32[toneIdx_u8] += pedalForce_kg_fl32 * cosPhase_fl32;
    sumSin_afl32[toneIdx_u8] += pedalForce_kg_fl32 * sinf(phase_fl32);
    sumForce_afl32[toneIdx_u8] += pedalForce_kg_fl32;
    sumForceSq_afl32[toneIdx_u8] += pedalForce_kg_fl32 * pedalForce_kg_fl32;
    sampleCount_au32[toneIdx_u8]++;
  }

  return s_excitationAmplitude_m_fl32 * envelope_fl32 * (1.0f - cosPhase_fl32);
}

void PlateInertiaIdentification::finish() {
  isActive_b = false;
  isResultValid_b = false;

  float responseReal_afl32[NMB_TONES];
  float responseImag_afl32[NMB_TONES];
  float omega_afl32[NMB_TONES];

  for (uint8_t toneIdx = 0; toneIdx < NMB_TONES; toneIdx++) {
    uint32_t sampleCount_u32 = sampleCount_au32[toneIdx];
    if (sampleCount_u32 < s_minSamplesPerTone_u32) {
      return;
    }
    float invCount_fl32 = 1.0f / (float)sampleCount_u32;

    // In-phase / quadrature amplitude of the force relative to cos(w t)
    float inPhase_fl32 = 2.0f * sumCos_afl32[toneIdx] * invCount_fl32;
    float quadrature_fl32 = 2.0f * sumSin_afl32[toneIdx] * invCount_fl32;

    // Foot on the pedal or disturbed motion: large mean force or large residual variance
    float meanForce_fl32 = sumForce_afl32[toneIdx] * invCount_fl32;
    float forceVariance_fl32 = sumForceSq_afl32[toneIdx] * invCount_fl32 - meanForce_fl32 * meanForce_fl32;
    float residualVariance_fl32 = forceVariance_fl32 - 0.5f * (inPhase_fl32 * inPhase_fl32 + quadrature_fl32 * quadrature_fl32);
    if ((fabsf(meanForce_fl32) > s_maxMeanForce_kg_fl32) ||
        (residualVariance_fl32 > s_maxResidualVarianceFactor_fl32 * measurementVariance_fl32)) {
      return;
    }

    // Transfer function acceleration -> force: H = (I + jQ) / (A w^2)
    omega_afl32[toneIdx] = s_twoPi_fl32 * s_toneFrequencyHz_afl32[toneIdx];
    float accelerationAmplitude_fl32 = s_excitationAmplitude_m_fl32 * omega_afl32[toneIdx] * omega_afl32[toneIdx];
    responseReal_afl32[toneIdx] = inPhase_fl32 / accelerationAmplitude_fl32;
    responseImag_afl32[toneIdx] = quadrature_fl32 / accelerationAmplitude_fl32;
  }

  // Delay from the phase at the higher frequency (inertia dominates there).
  // The coefficient is signed: flip by 180 deg if the response points backwards.
  float sign_fl32 = (responseReal_afl32[1] >= 0.0f) ? 1.0f : -1.0f;
  float delay_s_fl32_lcl = atan2f(sign_fl32 * responseImag_afl32[1], sign_fl32 * responseReal_afl32[1]) / omega_afl32[1];

  // Signed apparent coefficient per tone: projection of H onto the delay phasor
  // m(w) = Re(H * exp(-j w tau)) = c_m - k_g / w^2
  float apparent_afl32[NMB_TONES];
  for (uint8_t toneIdx = 0; toneIdx < NMB_TONES; toneIdx++) {
    float delayPhase_fl32 = omega_afl32[toneIdx] * delay_s_fl32_lcl;
    apparent_afl32[toneIdx] = responseReal_afl32[toneIdx] * cosf(delayPhase_fl32) + responseImag_afl32[toneIdx] * sinf(delayPhase_fl32);
  }

  // Solve m(w1), m(w2) for c_m and k_g
  float omegaSq1_fl32 = omega_afl32[0] * omega_afl32[0];
  float omegaSq2_fl32 = omega_afl32[1] * omega_afl32[1];
  float coefficient_fl32 = (apparent_afl32[1] * omegaSq2_fl32 - apparent_afl32[0] * omegaSq1_fl32) / (omegaSq2_fl32 - omegaSq1_fl32);
  float gravityStiffness_fl32 = (apparent_afl32[1] - apparent_afl32[0]) / (1.0f / omegaSq1_fl32 - 1.0f / omegaSq2_fl32);

  if ((fabsf(coefficient_fl32) < s_minAbsInertiaCoefficient_fl32) ||
      (fabsf(coefficient_fl32) > s_maxAbsInertiaCoefficient_fl32) ||
      (delay_s_fl32_lcl < 0.0f) || (delay_s_fl32_lcl > s_maxDelay_s_fl32)) {
    return;
  }

  inertiaCoefficient_kgPerMps2_fl32 = coefficient_fl32;
  delay_s_fl32 = delay_s_fl32_lcl;
  gravityStiffness_kgPerM_fl32 = gravityStiffness_fl32;
  isResultValid_b = true;
}
