#include "SignalFilter_1st_order.h"

// Define a new tuning constant for the random acceleration model
static const float KF_MODEL_NOISE_ACCELERATION = 1.0f * 1e2; // Tune this value

// Constructor
KalmanFilter1stOrder::KalmanFilter1stOrder(float varianceEstimate)
  : _timeLastObservation(micros()), _R(varianceEstimate)
{
  // Initialize 2D state [position, velocity]
  _x[0] = _x[1] = 0.0f;

  // Initialize 2x2 error covariance with high uncertainty
  _P_cov[0][0] = 1000.0f; // Large uncertainty in initial position
  _P_cov[1][1] = 1000.0f; // Large uncertainty in initial velocity
  _P_cov[0][1] = _P_cov[1][0] = 0.0f;

  // Measurement matrix H, relating state to measurement (only position)
  // H = 1x2 = [1, 0]
  _H[0][0] = 1.0f;
  _H[0][1] = 0.0f;

  // Measurement noise covariance R = 1x1 (scalar)
  _R = varianceEstimate;
  // convert to grams
  _R *= 1000.0f * 1000.0f;
}

float IRAM_ATTR_FLAG KalmanFilter1stOrder::filteredValue(float measurement, float command, uint8_t modelNoiseScaling_u8) {
  // Obtain time (this part is unchanged)
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - _timeLastObservation;
  _timeLastObservation = currentTime;

  float measurement_gram_fl32 = measurement * 1000.0f;

  float modelNoiseScaling = modelNoiseScaling_u8 / 255.0f;
  if (modelNoiseScaling < 0.001f) modelNoiseScaling = 0.001f;
  if (elapsedTime < 1) elapsedTime = 1;
  if (elapsedTime > 5000) elapsedTime = 5000;

  float delta_t_ms = elapsedTime / 1000.0f;
  float delta_t_ms_pow2 = delta_t_ms * delta_t_ms;
  float delta_t_ms_pow3 = delta_t_ms_pow2 * delta_t_ms;
  float delta_t_ms_pow4 = delta_t_ms_pow3 * delta_t_ms;

  // Update State Transition Matrix F for a constant velocity model
  // F = [1, dt; 0, 1]
  _F[0][0] = 1.0f; _F[0][1] = delta_t_ms;
  _F[1][0] = 0.0f; _F[1][1] = 1.0f;
	
  // Update Process Noise Covariance Matrix Q for a random acceleration model
  // Q = [1/4*dt^4, 1/2*dt^3; 1/2*dt^3, dt^2] * variance_acceleration
  float a_var_accel = modelNoiseScaling * KF_MODEL_NOISE_ACCELERATION;
  _Q[0][0] = a_var_accel * delta_t_ms_pow4 / 4.0f;
  _Q[0][1] = a_var_accel * delta_t_ms_pow3 / 2.0f;
  _Q[1][0] = _Q[0][1];
  _Q[1][1] = a_var_accel * delta_t_ms_pow2;

  // --- Predict Step ---
  // Predicted state estimate: x_pred = F * x
  float x_pred[2];
  x_pred[0] = _x[0] + delta_t_ms * _x[1];
  x_pred[1] = _x[1];
  
  // Predicted error covariance: P_pred = F * P * F' + Q
  float P_pred[2][2];
  P_pred[0][0] = _P_cov[0][0] + delta_t_ms * (_P_cov[1][0] + _P_cov[0][1] + delta_t_ms * _P_cov[1][1]) + _Q[0][0];
  P_pred[0][1] = _P_cov[0][1] + delta_t_ms * _P_cov[1][1] + _Q[0][1];
  P_pred[1][0] = _P_cov[1][0] + delta_t_ms * _P_cov[1][1] + _Q[1][0];
  P_pred[1][1] = _P_cov[1][1] + _Q[1][1];
  
  // --- Update Step ---
  // Measurement residual: y = z - H * x_pred
  float y = measurement_gram_fl32 - x_pred[0];
  
  // S = H * P_pred * H' + R (simplifies to P_pred[0][0] + R)
  float S_cov = P_pred[0][0] + _R;
  
  if (fabsf(S_cov) > 0.000001f) {
	// inv(S_cov)
	float inv_S_cov = 1.0f / S_cov;
	
	// Kalman Gain: K = P_pred * H' * inv(S)
    // K becomes a 2x1 vector
    _K[0] = P_pred[0][0] * inv_S_cov;
    _K[1] = P_pred[1][0] * inv_S_cov;

    // Updated state estimate: x = x_pred + K * y
    _x[0] = x_pred[0] + _K[0] * y;
    _x[1] = x_pred[1] + _K[1] * y;

    // Updated error covariance (Joseph Form for 2x2 system)
    // P = (I - K*H) * P_pred * (I - K*H)' + K*R*K'
    
    // (I - K*H) matrix for 2x2 system
    float I_minus_KH[2][2] = {
        {1.0f - _K[0], 0.0f},
        {-_K[1]      , 1.0f}
    };

    // Calculate the first term: (I-KH) * P_pred * (I-KH)'
    // Manual multiplication for efficiency
    float term1_final[2][2];
    float P_pred_T[2][2] = {{P_pred[0][0], P_pred[1][0]}, {P_pred[0][1], P_pred[1][1]}}; // Transpose P_pred for simpler calculation
    term1_final[0][0] = I_minus_KH[0][0] * (I_minus_KH[0][0] * P_pred[0][0] + I_minus_KH[0][1] * P_pred[1][0]);
    term1_final[0][1] = I_minus_KH[0][0] * (I_minus_KH[0][0] * P_pred[0][1] + I_minus_KH[0][1] * P_pred[1][1]);
    term1_final[1][0] = I_minus_KH[1][0] * (I_minus_KH[0][0] * P_pred[0][0] + I_minus_KH[0][1] * P_pred[1][0]) + I_minus_KH[1][1] * (I_minus_KH[1][0] * P_pred_T[0][0] + I_minus_KH[1][1] * P_pred_T[1][0]);
    term1_final[1][1] = I_minus_KH[1][0] * (I_minus_KH[0][0] * P_pred[0][1] + I_minus_KH[0][1] * P_pred[1][1]) + I_minus_KH[1][1] * (I_minus_KH[1][0] * P_pred_T[0][1] + I_minus_KH[1][1] * P_pred_T[1][1]);
    
    // Symmetrize the result (Joseph form should guarantee symmetry, but this corrects minor float errors)
    term1_final[0][1] = term1_final[1][0] = (term1_final[0][1] + term1_final[1][0]) / 2.0f;

    // Second term: K * R * K'
    float temp_KRKT[2][2];
    temp_KRKT[0][0] = _K[0] * _R * _K[0];
    temp_KRKT[0][1] = _K[0] * _R * _K[1];
    temp_KRKT[1][0] = _K[1] * _R * _K[0];
    temp_KRKT[1][1] = _K[1] * _R * _K[1];

    // Final P update
    _P_cov[0][0] = term1_final[0][0] + temp_KRKT[0][0];
    _P_cov[0][1] = term1_final[0][1] + temp_KRKT[0][1];
    _P_cov[1][0] = term1_final[1][0] + temp_KRKT[1][0];
    _P_cov[1][1] = term1_final[1][1] + temp_KRKT[1][1];
	
  } else {
    // S is zero, reset P_cov to avoid issues
    _P_cov[0][0] = _P_cov[0][1] = _P_cov[1][0] = _P_cov[1][1] = 0.0f;
  }

  return _x[0] / 1000.0f; // conversion g --> kg
}

float IRAM_ATTR_FLAG KalmanFilter1stOrder::changeVelocity() {
  return _x[1]; // conversion g/ms --> kg/s
}