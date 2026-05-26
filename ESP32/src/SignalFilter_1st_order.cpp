#include "SignalFilter_1st_order.h"

// Define a new tuning constant for the random acceleration model
static const float s_kfModelNoiseAcceleration_fl32 = 1.0f * 1e-2; // Tune this value

// Constructor
KalmanFilter1stOrder::KalmanFilter1stOrder(float varianceEstimate_fl32)
  : timeLastObservation_u32(micros()), measurementNoiseR_fl32(varianceEstimate_fl32)
{
  // Initialize 2D state [position, velocity]
  stateX_afl32[0] = stateX_afl32[1] = 0.0f;

  // Initialize 2x2 error covariance with high uncertainty
  stateCovarianceP_aafl32[0][0] = 1000.0f; // Large uncertainty in initial position
  stateCovarianceP_aafl32[1][1] = 1000.0f; // Large uncertainty in initial velocity
  stateCovarianceP_aafl32[0][1] = stateCovarianceP_aafl32[1][0] = 0.0f;

  // Measurement matrix H, relating state to measurement (only position)
  // H = 1x2 = [1, 0]
  measurementMatrixH_aafl32[0][0] = 1.0f;
  measurementMatrixH_aafl32[0][1] = 0.0f;

  // Measurement noise covariance R = 1x1 (scalar)
  measurementNoiseR_fl32 = varianceEstimate_fl32;
  // convert to grams
  measurementNoiseR_fl32 *= 1000.0f * 1000.0f;
}

float IRAM_ATTR_FLAG KalmanFilter1stOrder::filteredValue(float measurement_fl32, float command_fl32, uint8_t modelNoiseScaling_u8) {
  // Obtain time (this part is unchanged)
  unsigned long currentTime_u32 = micros();
  unsigned long elapsedTime_u32 = currentTime_u32 - timeLastObservation_u32;
  timeLastObservation_u32 = currentTime_u32;

  float measurementGram_fl32 = measurement_fl32 * 1000.0f;

  float modelNoiseScaling_fl32 = modelNoiseScaling_u8 / 255.0f;
  if (modelNoiseScaling_fl32 < 0.001f) modelNoiseScaling_fl32 = 0.001f;
  if (elapsedTime_u32 < 1) elapsedTime_u32 = 1;
  if (elapsedTime_u32 > 5000) elapsedTime_u32 = 5000;

  float deltaTimeMs_fl32 = elapsedTime_u32 / 1000.0f;
  float deltaTimeMsPow2_fl32 = deltaTimeMs_fl32 * deltaTimeMs_fl32;
  float deltaTimeMsPow3_fl32 = deltaTimeMsPow2_fl32 * deltaTimeMs_fl32;
  float deltaTimeMsPow4_fl32 = deltaTimeMsPow3_fl32 * deltaTimeMs_fl32;

  // Update State Transition Matrix F for a constant velocity model
  // F = [1, dt; 0, 1]
	(void)command_fl32;
  stateTransitionF_aafl32[0][0] = 1.0f; stateTransitionF_aafl32[0][1] = deltaTimeMs_fl32;
  stateTransitionF_aafl32[1][0] = 0.0f; stateTransitionF_aafl32[1][1] = 1.0f;
	
  // Update Process Noise Covariance Matrix Q for a random acceleration model
  // Q = [1/4*dt^4, 1/2*dt^3; 1/2*dt^3, dt^2] * variance_acceleration
  float accelerationVariance_fl32 = modelNoiseScaling_fl32 * s_kfModelNoiseAcceleration_fl32;
  processNoiseQ_aafl32[0][0] = accelerationVariance_fl32 * deltaTimeMsPow4_fl32 / 4.0f;
  processNoiseQ_aafl32[0][1] = accelerationVariance_fl32 * deltaTimeMsPow3_fl32 / 2.0f;
  processNoiseQ_aafl32[1][0] = processNoiseQ_aafl32[0][1];
  processNoiseQ_aafl32[1][1] = accelerationVariance_fl32 * deltaTimeMsPow2_fl32;

  // --- Predict Step ---
  // Predicted state estimate: x_pred = F * x
  float xPred_afl32[2];
  xPred_afl32[0] = stateX_afl32[0] + deltaTimeMs_fl32 * stateX_afl32[1];
  xPred_afl32[1] = stateX_afl32[1];
  
  // Predicted error covariance: P_pred = F * P * F' + Q
  float pPred_aafl32[2][2];
  pPred_aafl32[0][0] = stateCovarianceP_aafl32[0][0] + deltaTimeMs_fl32 * (stateCovarianceP_aafl32[1][0] + stateCovarianceP_aafl32[0][1] + deltaTimeMs_fl32 * stateCovarianceP_aafl32[1][1]) + processNoiseQ_aafl32[0][0];
  pPred_aafl32[0][1] = stateCovarianceP_aafl32[0][1] + deltaTimeMs_fl32 * stateCovarianceP_aafl32[1][1] + processNoiseQ_aafl32[0][1];
  pPred_aafl32[1][0] = stateCovarianceP_aafl32[1][0] + deltaTimeMs_fl32 * stateCovarianceP_aafl32[1][1] + processNoiseQ_aafl32[1][0];
  pPred_aafl32[1][1] = stateCovarianceP_aafl32[1][1] + processNoiseQ_aafl32[1][1];
  
  // --- Update Step ---
  // Measurement residual: y = z - H * x_pred
  float measurementResidual_fl32 = measurementGram_fl32 - xPred_afl32[0];
  
  // S = H * P_pred * H' + R (simplifies to P_pred[0][0] + R)
  float sCov_fl32 = pPred_aafl32[0][0] + measurementNoiseR_fl32;
  
  if (fabsf(sCov_fl32) > 0.000001f) {
	// inv(S_cov)
	float invSCov_fl32 = 1.0f / sCov_fl32;
	
	// Kalman Gain: K = P_pred * H' * inv(S)
    // K becomes a 2x1 vector
    kalmanGainK_afl32[0] = pPred_aafl32[0][0] * invSCov_fl32;
    kalmanGainK_afl32[1] = pPred_aafl32[1][0] * invSCov_fl32;

    // Updated state estimate: x = x_pred + K * y
    stateX_afl32[0] = xPred_afl32[0] + kalmanGainK_afl32[0] * measurementResidual_fl32;
    stateX_afl32[1] = xPred_afl32[1] + kalmanGainK_afl32[1] * measurementResidual_fl32;

    // Updated error covariance (Joseph Form for 2x2 system)
    // P = (I - K*H) * P_pred * (I - K*H)' + K*R*K'
    
    // (I - K*H) matrix for 2x2 system
    float iMinusKh_aafl32[2][2] = {
      {1.0f - kalmanGainK_afl32[0], 0.0f},
      {-kalmanGainK_afl32[1]      , 1.0f}
    };

    // Calculate the first term: (I-KH) * P_pred * (I-KH)'
    // Manual multiplication for efficiency
    float term1Final_aafl32[2][2];
    float pPredTranspose_aafl32[2][2] = {{pPred_aafl32[0][0], pPred_aafl32[1][0]}, {pPred_aafl32[0][1], pPred_aafl32[1][1]}}; // Transpose P_pred for simpler calculation
    term1Final_aafl32[0][0] = iMinusKh_aafl32[0][0] * (iMinusKh_aafl32[0][0] * pPred_aafl32[0][0] + iMinusKh_aafl32[0][1] * pPred_aafl32[1][0]);
    term1Final_aafl32[0][1] = iMinusKh_aafl32[0][0] * (iMinusKh_aafl32[0][0] * pPred_aafl32[0][1] + iMinusKh_aafl32[0][1] * pPred_aafl32[1][1]);
    term1Final_aafl32[1][0] = iMinusKh_aafl32[1][0] * (iMinusKh_aafl32[0][0] * pPred_aafl32[0][0] + iMinusKh_aafl32[0][1] * pPred_aafl32[1][0]) + iMinusKh_aafl32[1][1] * (iMinusKh_aafl32[1][0] * pPredTranspose_aafl32[0][0] + iMinusKh_aafl32[1][1] * pPredTranspose_aafl32[1][0]);
    term1Final_aafl32[1][1] = iMinusKh_aafl32[1][0] * (iMinusKh_aafl32[0][0] * pPred_aafl32[0][1] + iMinusKh_aafl32[0][1] * pPred_aafl32[1][1]) + iMinusKh_aafl32[1][1] * (iMinusKh_aafl32[1][0] * pPredTranspose_aafl32[0][1] + iMinusKh_aafl32[1][1] * pPredTranspose_aafl32[1][1]);
    
    // Symmetrize the result (Joseph form should guarantee symmetry, but this corrects minor float errors)
    term1Final_aafl32[0][1] = term1Final_aafl32[1][0] = (term1Final_aafl32[0][1] + term1Final_aafl32[1][0]) / 2.0f;

    // Second term: K * R * K'
    float tempKrkt_aafl32[2][2];
    tempKrkt_aafl32[0][0] = kalmanGainK_afl32[0] * measurementNoiseR_fl32 * kalmanGainK_afl32[0];
    tempKrkt_aafl32[0][1] = kalmanGainK_afl32[0] * measurementNoiseR_fl32 * kalmanGainK_afl32[1];
    tempKrkt_aafl32[1][0] = kalmanGainK_afl32[1] * measurementNoiseR_fl32 * kalmanGainK_afl32[0];
    tempKrkt_aafl32[1][1] = kalmanGainK_afl32[1] * measurementNoiseR_fl32 * kalmanGainK_afl32[1];

    // Final P update
    stateCovarianceP_aafl32[0][0] = term1Final_aafl32[0][0] + tempKrkt_aafl32[0][0];
    stateCovarianceP_aafl32[0][1] = term1Final_aafl32[0][1] + tempKrkt_aafl32[0][1];
    stateCovarianceP_aafl32[1][0] = term1Final_aafl32[1][0] + tempKrkt_aafl32[1][0];
    stateCovarianceP_aafl32[1][1] = term1Final_aafl32[1][1] + tempKrkt_aafl32[1][1];
	
  } else {
    // S is zero, reset P_cov to avoid issues
    stateCovarianceP_aafl32[0][0] = stateCovarianceP_aafl32[0][1] = stateCovarianceP_aafl32[1][0] = stateCovarianceP_aafl32[1][1] = 0.0f;
  }

  return stateX_afl32[0] / 1000.0f; // conversion g --> kg
}

float IRAM_ATTR_FLAG KalmanFilter1stOrder::changeVelocity() {
  return stateX_afl32[1]; // conversion g/ms --> kg/s
}