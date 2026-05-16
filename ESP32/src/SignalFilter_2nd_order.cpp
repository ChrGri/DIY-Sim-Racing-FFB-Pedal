#include "SignalFilter_2nd_order.h"

// Define constants from the original code
static const float s_kfModelNoiseForceJerk_fl32 = 1.0f * 1e-5;

// Constructor
KalmanFilter2ndOrder::KalmanFilter2ndOrder(float varianceEstimate_fl32)
  : timeLastObservation_u32(micros()), measurementNoiseR_fl32(varianceEstimate_fl32)
{
  // Initialize state
  stateX_afl32[0] = stateX_afl32[1] = stateX_afl32[2] = 0.0f;

  // Initialize error covariance with high uncertainty
  // Set diagonal elements to large values
  stateCovarianceP_aafl32[0][0] = 1000.0f; // Large uncertainty in initial position
  stateCovarianceP_aafl32[1][1] = 1000.0f; // Large uncertainty in initial velocity
  stateCovarianceP_aafl32[2][2] = 1000.0f; // Large uncertainty in initial acceleration

  // Set off-diagonal elements to zero (assuming initial errors are uncorrelated)
  stateCovarianceP_aafl32[0][1] = stateCovarianceP_aafl32[0][2] = 0.0f;
  stateCovarianceP_aafl32[1][0] = stateCovarianceP_aafl32[1][2] = 0.0f;
  stateCovarianceP_aafl32[2][0] = stateCovarianceP_aafl32[2][1] = 0.0f;

  // Measurement matrix H, relating state to measurement (only position)
  // H = 1x3 = [1, 0, 0]
  measurementMatrixH_aafl32[0][0] = 1.0f;
  measurementMatrixH_aafl32[0][1] = 0.0f;
  measurementMatrixH_aafl32[0][2] = 0.0f;

  // Measurement noise covariance R = 1x1 (scalar)
  measurementNoiseR_fl32 = varianceEstimate_fl32;
  // convert to grams
  measurementNoiseR_fl32 *= 1000.0f * 1000.0f;
}

float IRAM_ATTR_FLAG KalmanFilter2ndOrder::filteredValue(float measurement_fl32, float command_fl32, uint8_t modelNoiseScaling_u8) {
  // Obtain time
  unsigned long currentTime_u32 = micros();
  unsigned long elapsedTime_u32 = currentTime_u32 - timeLastObservation_u32;
  timeLastObservation_u32 = currentTime_u32;
  float measurementGram_fl32 = measurement_fl32 * 1000.0f;

  float modelNoiseScaling_fl32 = modelNoiseScaling_u8 / 255.0f;
  if (modelNoiseScaling_fl32 < 0.001f) modelNoiseScaling_fl32 = 0.001f;
  if (elapsedTime_u32 < 1) elapsedTime_u32 = 1;
  if (elapsedTime_u32 > 5000) elapsedTime_u32 = 5000;

  // delta_t is now in milliseconds
  float deltaTimeMs_fl32 = elapsedTime_u32 / 1000.0f;
  float deltaTimeMsPow2_fl32 = deltaTimeMs_fl32 * deltaTimeMs_fl32;
  float deltaTimeMsPow3_fl32 = deltaTimeMsPow2_fl32 * deltaTimeMs_fl32;
  float deltaTimeMsPow4_fl32 = deltaTimeMsPow2_fl32 * deltaTimeMsPow2_fl32;
  float deltaTimeMsPow5_fl32 = deltaTimeMsPow4_fl32 * deltaTimeMs_fl32;
  float deltaTimeMsPow6_fl32 = deltaTimeMsPow5_fl32 * deltaTimeMs_fl32;

  // Update State Transition Matrix F
  // F = [1, dt, 0.5*dt^2; 0, 1, dt; 0, 0, 1]
	(void)command_fl32;
  stateTransitionF_aafl32[0][0] = 1.0f; stateTransitionF_aafl32[0][1] = deltaTimeMs_fl32; stateTransitionF_aafl32[0][2] = 0.5f * deltaTimeMsPow2_fl32;
  stateTransitionF_aafl32[1][0] = 0.0f; stateTransitionF_aafl32[1][1] = 1.0f; stateTransitionF_aafl32[1][2] = deltaTimeMs_fl32;
  stateTransitionF_aafl32[2][0] = 0.0f; stateTransitionF_aafl32[2][1] = 0.0f; stateTransitionF_aafl32[2][2] = 1.0f;
	
  // Update Process Noise Covariance Matrix Q based on jerk model
  // Q = r * r' * a_var_jerk, where r = [1/6*dt^3, 1/2*dt^2, dt]'
  //   = [1/36*delta_t^6, 1/12*delta_t^5, 1/6*delta_t^4;
  //   =  1/12*delta_t^5, 1/4*delta_t^4, 1/2*delta_t^3;
  //   =  1/6*delta_t^4, 1/2*delta_t^3, delta_t^2]
  float jerkVariance_fl32 = modelNoiseScaling_fl32 * s_kfModelNoiseForceJerk_fl32;
  processNoiseQ_aafl32[0][0] = jerkVariance_fl32 * deltaTimeMsPow6_fl32 / 36.0f;
  processNoiseQ_aafl32[0][1] = jerkVariance_fl32 * deltaTimeMsPow5_fl32 / 12.0f;
  processNoiseQ_aafl32[0][2] = jerkVariance_fl32 * deltaTimeMsPow4_fl32 / 6.0f;
  processNoiseQ_aafl32[1][0] = processNoiseQ_aafl32[0][1];
  processNoiseQ_aafl32[1][1] = jerkVariance_fl32 * deltaTimeMsPow4_fl32 / 4.0f;
  processNoiseQ_aafl32[1][2] = jerkVariance_fl32 * deltaTimeMsPow3_fl32 / 2.0f;
  processNoiseQ_aafl32[2][0] = processNoiseQ_aafl32[0][2];
  processNoiseQ_aafl32[2][1] = processNoiseQ_aafl32[1][2];
  processNoiseQ_aafl32[2][2] = jerkVariance_fl32 * deltaTimeMsPow2_fl32;

  // Predict Step
  // Predicted state estimate: x_pred = F * x
  float xPred_afl32[3];
  xPred_afl32[0] = stateX_afl32[0] + stateTransitionF_aafl32[0][1] * stateX_afl32[1] + stateTransitionF_aafl32[0][2] * stateX_afl32[2];
  xPred_afl32[1] = stateX_afl32[1] + stateTransitionF_aafl32[1][2] * stateX_afl32[2];
  xPred_afl32[2] = stateX_afl32[2];
  
  // Predicted error covariance: P_pred = F * P * F' + Q
  //
  // The full matrix multiplication for P_pred is:
  // [ P_pred_00, P_pred_01, P_pred_02 ]   [ F_00, F_01, F_02 ] [ P_00, P_01, P_02 ] [ F_00, F_10, F_20 ]   [ Q_00, Q_01, Q_02 ]
  // [ P_pred_10, P_pred_11, P_pred_12 ] = [ F_10, F_11, F_12 ] [ P_10, P_11, P_12 ] [ F_01, F_11, F_21 ] + [ Q_10, Q_11, Q_12 ]
  // [ P_pred_20, P_pred_21, P_pred_22 ]   [ F_20, F_21, F_22 ] [ P_20, P_21, P_22 ] [ F_02, F_12, F_22 ]   [ Q_20, Q_21, Q_22 ]
  //
  // Since F is an upper triangular matrix with F_10, F_20, and F_21 being 0, and P is symmetric,
  // the calculation is simplified to a series of dot products. Your code manually performs these
  // dot products, which is more efficient than a generic matrix multiplication function.
  //
  // For example, the calculation for P_pred_00:
  // (F * P)_row0 = [F_00*P_00 + F_01*P_10 + F_02*P_20, F_00*P_01 + F_01*P_11 + F_02*P_21, F_00*P_02 + F_01*P_12 + F_02*P_22]
  //
  // P_pred_00 = (F * P)_row0 . (F')_col0 + Q_00
  //           = (F * P)_row0 . [F_00, F_01, F_02]' + Q_00
  //           = (F_00*P_00 + F_01*P_10 + F_02*P_20)*F_00 + (F_00*P_01 + F_01*P_11 + F_02*P_21)*F_01 + (F_00*P_02 + F_01*P_12 + F_02*P_22)*F_02 + Q_00
  //
  // This is what the following code implements in a single line for each element of P_pred,
  // taking advantage of the zeros in F and the symmetry of P.
  float pPred_aafl32[3][3];
  pPred_aafl32[0][0] = (stateCovarianceP_aafl32[0][0] + stateTransitionF_aafl32[0][1] * stateCovarianceP_aafl32[1][0] + stateTransitionF_aafl32[0][2] * stateCovarianceP_aafl32[2][0]) +
                 stateTransitionF_aafl32[0][1] * (stateCovarianceP_aafl32[0][1] + stateTransitionF_aafl32[0][1] * stateCovarianceP_aafl32[1][1] + stateTransitionF_aafl32[0][2] * stateCovarianceP_aafl32[2][1]) +
                 stateTransitionF_aafl32[0][2] * (stateCovarianceP_aafl32[0][2] + stateTransitionF_aafl32[0][1] * stateCovarianceP_aafl32[1][2] + stateTransitionF_aafl32[0][2] * stateCovarianceP_aafl32[2][2]) + processNoiseQ_aafl32[0][0];
                 
  pPred_aafl32[0][1] = (stateCovarianceP_aafl32[1][0] + stateTransitionF_aafl32[1][2] * stateCovarianceP_aafl32[2][0]) +
                 stateTransitionF_aafl32[0][1] * (stateCovarianceP_aafl32[1][1] + stateTransitionF_aafl32[1][2] * stateCovarianceP_aafl32[2][1]) +
                 stateTransitionF_aafl32[0][2] * (stateCovarianceP_aafl32[1][2] + stateTransitionF_aafl32[1][2] * stateCovarianceP_aafl32[2][2]) + processNoiseQ_aafl32[0][1];
                 
  pPred_aafl32[0][2] = (stateCovarianceP_aafl32[2][0]) +
                 stateTransitionF_aafl32[0][1] * (stateCovarianceP_aafl32[2][1]) +
                 stateTransitionF_aafl32[0][2] * (stateCovarianceP_aafl32[2][2]) + processNoiseQ_aafl32[0][2];
				 
  pPred_aafl32[1][0] = pPred_aafl32[0][1];
  pPred_aafl32[1][1] = (stateCovarianceP_aafl32[1][1] + stateTransitionF_aafl32[1][2] * stateCovarianceP_aafl32[2][1]) + 
                 stateTransitionF_aafl32[1][2] * (stateCovarianceP_aafl32[1][2] + stateTransitionF_aafl32[1][2] * stateCovarianceP_aafl32[2][2]) + processNoiseQ_aafl32[1][1];
  pPred_aafl32[1][2] = (stateCovarianceP_aafl32[2][1]) +
                 stateTransitionF_aafl32[1][2] * (stateCovarianceP_aafl32[2][2]) + processNoiseQ_aafl32[1][2];

  pPred_aafl32[2][0] = pPred_aafl32[0][2];
  pPred_aafl32[2][1] = pPred_aafl32[1][2];
  pPred_aafl32[2][2] = stateCovarianceP_aafl32[2][2] + processNoiseQ_aafl32[2][2];
  
    
  // Update Step
  // Measurement residual: y = z - H * x_pred
  // Since only position is measured, measurement residual is of type position only
  float measurementResidual_fl32 = measurementGram_fl32 -  xPred_afl32[0];
  
  // S = H * P_pred * H' + R
  // H = [1; 0; 0]
  // P = 3x3
  // H * P_pred * H' simplifies to P_pred[0][0]
  // (H * P) = 1x3 = [1, 0, 0] * [P_00, P_01, P_02; P_10, P_11, P_12; P_20, P_21, P_22] = [P_00, P_01, P_02]
  // (H * P) * H' = 1x3 * 3x1 = 1x1 = [P_00, P_01, P_02] * [1; 0; 0] = P_00
  // S_cov = [P_00, 0, 0; 0, 0, 0; 0, 0, 0] + R
  float sCov_fl32 = pPred_aafl32[0][0] + measurementNoiseR_fl32;
  
  if (fabsf(sCov_fl32) > 0.000001f) {
	  
	// inv(S_cov) = 1 / (P_00 + R)
	float invSCov_fl32 = 1.0f / sCov_fl32;
	
	// Kalman Gain: K = P_pred * H' * inv(S)
    // K = P_pred * H' * inv(S)
	// K = 3x1 * 1x1 = 3x1
    kalmanGainK_afl32[0] = pPred_aafl32[0][0] * invSCov_fl32;
    kalmanGainK_afl32[1] = pPred_aafl32[1][0] * invSCov_fl32;
    kalmanGainK_afl32[2] = pPred_aafl32[2][0] * invSCov_fl32;

    // Updated state estimate: x = x_pred + K * y
    stateX_afl32[0] = xPred_afl32[0] + kalmanGainK_afl32[0] * measurementResidual_fl32;
    stateX_afl32[1] = xPred_afl32[1] + kalmanGainK_afl32[1] * measurementResidual_fl32;
    stateX_afl32[2] = xPred_afl32[2] + kalmanGainK_afl32[2] * measurementResidual_fl32;

    // P_pred * H' = 3x3 * (1x3)' = 3x1


    // Updated error covariance (Joseph Form)
    // P = (I - K*H) * P_pred * (I - K*H)' + K*R*K'
    //
    // This is the Joseph form of the covariance update equation, which ensures the resulting
    // covariance matrix remains symmetric and positive semi-definite, improving numerical stability.
    //
    // The multiplication is simplified because H = [1, 0, 0].
    //
    // First term: (I - K*H) * P_pred
    // (I - K*H) = [ 1 - K_0, 0, 0 ]
    //             [  -K_1,  1, 0  ]
    //             [  -K_2,  0, 1 ]
    //
    // This part of the code calculates the product of this matrix with P_pred.
    // temp_P = (I - K*H) * P_pred
    //
    // temp_P[0][0] = (1.0f - _K[0]) * P_pred[0][0];
    // temp_P[0][1] = (1.0f - _K[0]) * P_pred[0][1];
    // ...
    //
    // Second term: K*R*K'
    // This term is an outer product of the Kalman gain vector.
    // K*R*K' = R * [ K_0*K_0, K_0*K_1, K_0*K_2 ]
    //                [ K_1*K_0, K_1*K_1, K_1*K_2 ]
    //                [ K_2*K_0, K_2*K_1, K_2*K_2 ]
    //
    // The code calculates this second term in the nested for loops.
    //
    // Final P update: _P_cov = temp_P * (I - K*H)' + K*R*K'
    // This final step is simplified by the fact that the first term is symmetric
    // due to the form (I - KH) * P_pred * (I-KH)'. The code calculates this and
    // then adds the second term.
    float tempP_aafl32[3][3];
    tempP_aafl32[0][0] = (1.0f - kalmanGainK_afl32[0]) * pPred_aafl32[0][0];
    tempP_aafl32[0][1] = (1.0f - kalmanGainK_afl32[0]) * pPred_aafl32[0][1];
    tempP_aafl32[0][2] = (1.0f - kalmanGainK_afl32[0]) * pPred_aafl32[0][2];

    tempP_aafl32[1][0] = -kalmanGainK_afl32[1] * pPred_aafl32[0][0] + pPred_aafl32[1][0];
    tempP_aafl32[1][1] = -kalmanGainK_afl32[1] * pPred_aafl32[0][1] + pPred_aafl32[1][1];
    tempP_aafl32[1][2] = -kalmanGainK_afl32[1] * pPred_aafl32[0][2] + pPred_aafl32[1][2];

    tempP_aafl32[2][0] = -kalmanGainK_afl32[2] * pPred_aafl32[0][0] + pPred_aafl32[2][0];
    tempP_aafl32[2][1] = -kalmanGainK_afl32[2] * pPred_aafl32[0][1] + pPred_aafl32[2][1];
    tempP_aafl32[2][2] = -kalmanGainK_afl32[2] * pPred_aafl32[0][2] + pPred_aafl32[2][2];

    // Second term: K * R * K'
    float tempKrkt_aafl32[3][3];
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        tempKrkt_aafl32[i][j] = kalmanGainK_afl32[i] * measurementNoiseR_fl32 * kalmanGainK_afl32[j];
      }
    }

    // Final P update
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        stateCovarianceP_aafl32[i][j] = tempP_aafl32[i][j] + tempKrkt_aafl32[i][j];
      }
    }
	

  } else {
    // S is zero, reset P_cov to avoid issues
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        stateCovarianceP_aafl32[i][j] = 0.0f;
      }
    }
  }

  return stateX_afl32[0] / 1000.0f;
}

float IRAM_ATTR_FLAG KalmanFilter2ndOrder::changeVelocity() {
  return stateX_afl32[1]; // conversion g/ms --> kg/s
}

float IRAM_ATTR_FLAG KalmanFilter2ndOrder::changeAccel() {
  return stateX_afl32[2] * 1000.0f; // conversion g/ms^2 --> kg/s^2
}