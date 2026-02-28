#include "SignalFilter_2nd_order.h"

// Define constants from the original code
static const float KF_MODEL_NOISE_FORCE_JERK = 1.0f * 1e-5;

// Constructor
KalmanFilter2ndOrder::KalmanFilter2ndOrder(float varianceEstimate)
  : _timeLastObservation(micros()), _R(varianceEstimate)
{
  // Initialize state
  _x[0] = _x[1] = _x[2] = 0.0f;

  // Initialize error covariance with high uncertainty
  // Set diagonal elements to large values
  _P_cov[0][0] = 1000.0f; // Large uncertainty in initial position
  _P_cov[1][1] = 1000.0f; // Large uncertainty in initial velocity
  _P_cov[2][2] = 1000.0f; // Large uncertainty in initial acceleration

  // Set off-diagonal elements to zero (assuming initial errors are uncorrelated)
  _P_cov[0][1] = _P_cov[0][2] = 0.0f;
  _P_cov[1][0] = _P_cov[1][2] = 0.0f;
  _P_cov[2][0] = _P_cov[2][1] = 0.0f;

  // Measurement matrix H, relating state to measurement (only position)
  // H = 1x3 = [1, 0, 0]
  _H[0][0] = 1.0f;
  _H[0][1] = 0.0f;
  _H[0][2] = 0.0f;

  // Measurement noise covariance R = 1x1 (scalar)
  _R = varianceEstimate;
  // convert to grams
  _R *= 1000.0f * 1000.0f;
}

float KalmanFilter2ndOrder::filteredValue(float measurement, float command, uint8_t modelNoiseScaling_u8) {
  // Obtain time
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - _timeLastObservation;
  _timeLastObservation = currentTime;
  float measurement_gram_fl32 = measurement * 1000.0f;

  float modelNoiseScaling = modelNoiseScaling_u8 / 255.0f;
  if (modelNoiseScaling < 0.001f) modelNoiseScaling = 0.001f;
  if (elapsedTime < 1) elapsedTime = 1;
  if (elapsedTime > 5000) elapsedTime = 5000;

  // delta_t is now in milliseconds
  float delta_t_ms = elapsedTime / 1000.0f;
  float delta_t_ms_pow2 = delta_t_ms * delta_t_ms;
  float delta_t_ms_pow3 = delta_t_ms_pow2 * delta_t_ms;
  float delta_t_ms_pow4 = delta_t_ms_pow2 * delta_t_ms_pow2;
  float delta_t_ms_pow5 = delta_t_ms_pow4 * delta_t_ms;
  float delta_t_ms_pow6 = delta_t_ms_pow5 * delta_t_ms;

  // Update State Transition Matrix F
  // F = [1, dt, 0.5*dt^2; 0, 1, dt; 0, 0, 1]
  _F[0][0] = 1.0f; _F[0][1] = delta_t_ms; _F[0][2] = 0.5f * delta_t_ms_pow2;
  _F[1][0] = 0.0f; _F[1][1] = 1.0f; _F[1][2] = delta_t_ms;
  _F[2][0] = 0.0f; _F[2][1] = 0.0f; _F[2][2] = 1.0f;
	
  // Update Process Noise Covariance Matrix Q based on jerk model
  // Q = r * r' * a_var_jerk, where r = [1/6*dt^3, 1/2*dt^2, dt]'
  //   = [1/36*delta_t^6, 1/12*delta_t^5, 1/6*delta_t^4;
  //   =  1/12*delta_t^5, 1/4*delta_t^4, 1/2*delta_t^3;
  //   =  1/6*delta_t^4, 1/2*delta_t^3, delta_t^2]
  float a_var_jerk = modelNoiseScaling * KF_MODEL_NOISE_FORCE_JERK;
  _Q[0][0] = a_var_jerk * delta_t_ms_pow6 / 36.0f;
  _Q[0][1] = a_var_jerk * delta_t_ms_pow5 / 12.0f;
  _Q[0][2] = a_var_jerk * delta_t_ms_pow4 / 6.0f;
  _Q[1][0] = _Q[0][1];
  _Q[1][1] = a_var_jerk * delta_t_ms_pow4 / 4.0f;
  _Q[1][2] = a_var_jerk * delta_t_ms_pow3 / 2.0f;
  _Q[2][0] = _Q[0][2];
  _Q[2][1] = _Q[1][2];
  _Q[2][2] = a_var_jerk * delta_t_ms_pow2;

  // Predict Step
  // Predicted state estimate: x_pred = F * x
  float x_pred[3];
  x_pred[0] = _x[0] + _F[0][1] * _x[1] + _F[0][2] * _x[2];
  x_pred[1] = _x[1] + _F[1][2] * _x[2];
  x_pred[2] = _x[2];
  
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
  float P_pred[3][3];
  P_pred[0][0] = (_P_cov[0][0] + _F[0][1] * _P_cov[1][0] + _F[0][2] * _P_cov[2][0]) +
                 _F[0][1] * (_P_cov[0][1] + _F[0][1] * _P_cov[1][1] + _F[0][2] * _P_cov[2][1]) +
                 _F[0][2] * (_P_cov[0][2] + _F[0][1] * _P_cov[1][2] + _F[0][2] * _P_cov[2][2]) + _Q[0][0];
                 
  P_pred[0][1] = (_P_cov[1][0] + _F[1][2] * _P_cov[2][0]) +
                 _F[0][1] * (_P_cov[1][1] + _F[1][2] * _P_cov[2][1]) +
                 _F[0][2] * (_P_cov[1][2] + _F[1][2] * _P_cov[2][2]) + _Q[0][1];
                 
  P_pred[0][2] = (_P_cov[2][0]) +
                 _F[0][1] * (_P_cov[2][1]) +
                 _F[0][2] * (_P_cov[2][2]) + _Q[0][2];
				 
  P_pred[1][0] = P_pred[0][1];
  P_pred[1][1] = (_P_cov[1][1] + _F[1][2] * _P_cov[2][1]) + 
                 _F[1][2] * (_P_cov[1][2] + _F[1][2] * _P_cov[2][2]) + _Q[1][1];
  P_pred[1][2] = (_P_cov[2][1]) +
                 _F[1][2] * (_P_cov[2][2]) + _Q[1][2];

  P_pred[2][0] = P_pred[0][2];
  P_pred[2][1] = P_pred[1][2];
  P_pred[2][2] = _P_cov[2][2] + _Q[2][2];
  
    
  // Update Step
  // Measurement residual: y = z - H * x_pred
  // Since only position is measured, measurement residual is of type position only
  float y = measurement_gram_fl32 -  x_pred[0];
  
  // S = H * P_pred * H' + R
  // H = [1; 0; 0]
  // P = 3x3
  // H * P_pred * H' simplifies to P_pred[0][0]
  // (H * P) = 1x3 = [1, 0, 0] * [P_00, P_01, P_02; P_10, P_11, P_12; P_20, P_21, P_22] = [P_00, P_01, P_02]
  // (H * P) * H' = 1x3 * 3x1 = 1x1 = [P_00, P_01, P_02] * [1; 0; 0] = P_00
  // S_cov = [P_00, 0, 0; 0, 0, 0; 0, 0, 0] + R
  float S_cov = P_pred[0][0] + _R;
  
  if (fabsf(S_cov) > 0.000001f) {
	  
	// inv(S_cov) = 1 / (P_00 + R)
	float inv_S_cov = 1.0f / S_cov;
	
	// Kalman Gain: K = P_pred * H' * inv(S)
    // K = P_pred * H' * inv(S)
	// K = 3x1 * 1x1 = 3x1
    _K[0] = P_pred[0][0] * inv_S_cov;
    _K[1] = P_pred[1][0] * inv_S_cov;
    _K[2] = P_pred[2][0] * inv_S_cov;

    // Updated state estimate: x = x_pred + K * y
    _x[0] = x_pred[0] + _K[0] * y;
    _x[1] = x_pred[1] + _K[1] * y;
    _x[2] = x_pred[2] + _K[2] * y;

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
    float temp_P[3][3];
    temp_P[0][0] = (1.0f - _K[0]) * P_pred[0][0];
    temp_P[0][1] = (1.0f - _K[0]) * P_pred[0][1];
    temp_P[0][2] = (1.0f - _K[0]) * P_pred[0][2];

    temp_P[1][0] = -_K[1] * P_pred[0][0] + P_pred[1][0];
    temp_P[1][1] = -_K[1] * P_pred[0][1] + P_pred[1][1];
    temp_P[1][2] = -_K[1] * P_pred[0][2] + P_pred[1][2];

    temp_P[2][0] = -_K[2] * P_pred[0][0] + P_pred[2][0];
    temp_P[2][1] = -_K[2] * P_pred[0][1] + P_pred[2][1];
    temp_P[2][2] = -_K[2] * P_pred[0][2] + P_pred[2][2];

    // Second term: K * R * K'
    float temp_KRKT[3][3];
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        temp_KRKT[i][j] = _K[i] * _R * _K[j];
      }
    }

    // Final P update
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        _P_cov[i][j] = temp_P[i][j] + temp_KRKT[i][j];
      }
    }
	

  } else {
    // S is zero, reset P_cov to avoid issues
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        _P_cov[i][j] = 0.0f;
      }
    }
  }

  return _x[0] / 1000.0f;
}

float KalmanFilter2ndOrder::changeVelocity() {
  return _x[1]; // conversion g/ms --> kg/s
}

float KalmanFilter2ndOrder::changeAccel() {
  return _x[2] * 1000.0f; // conversion g/ms^2 --> kg/s^2
}