#include "SignalFilter.h"

KalmanFilter::KalmanFilter(float varianceEstimate)
  : _timeLastObservation(micros()), _position(0.0f), _velocity(0.0f), _R(varianceEstimate)
{
  // Initialize error covariance
  _P_cov[0][0] = _P_cov[0][1] = _P_cov[1][0] = _P_cov[1][1] = 0.0f;

  // State transition matrix
  _F[0][0] = 1.0f; _F[0][1] = 0.1f;
  _F[1][0] = 0.0f; _F[1][1] = 1.0f;

  // Process noise covariance
  _Q[0][0] = 0.1f; _Q[0][1] = 0.0f;
  _Q[1][0] = 0.0f; _Q[1][1] = 0.1f;

  // Measurement matrix
  _H[0][0] = 1.0f;
  _H[0][1] = 0.0f;
}

// Matrix multiplication function
void KalmanFilter::multiplyMatrices(float mat1[2][2], float mat2[2][2], float result[2][2]) {
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) {
      result[i][j] = 0.0f;
      for (int k = 0; k < 2; ++k)
        result[i][j] += mat1[i][k] * mat2[k][j];
    }
}

float KalmanFilter::filteredValue(float observation, float command, uint8_t modelNoiseScaling_u8) {
  // Obtain time
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - _timeLastObservation;
  _timeLastObservation = currentTime;

  float modelNoiseScaling = modelNoiseScaling_u8 / 255.0f;
  float modelNoiseLowerThreshold = 1.0f / 255.0f;
  if (modelNoiseScaling < modelNoiseLowerThreshold) modelNoiseScaling = modelNoiseLowerThreshold;
  if (elapsedTime < 1) elapsedTime = 1;
  if (elapsedTime > 5000) elapsedTime = 5000;

  float delta_t = elapsedTime / 1000000.0f;  // Convert to seconds
  float delta_t_pow2 = delta_t * delta_t;
  float delta_t_pow3 = delta_t_pow2 * delta_t;
  float delta_t_pow4 = delta_t_pow2 * delta_t_pow2;

  // Update transition matrix
  // F = [1, T; 0, 1]
  _F[0][0] = 1.0f;
  _F[0][1] = delta_t;
  _F[1][0] = 0.0f;
  _F[1][1] = 1.0f;

  // Process noise covariance
  float a_var = modelNoiseScaling * KF_MODEL_NOISE_FORCE_ACCELERATION;
  float K_Q_11 = a_var * 0.5f * delta_t_pow3;
  _Q[0][0] = a_var * 0.25f * delta_t_pow4;
  _Q[0][1] = K_Q_11;
  _Q[1][0] = K_Q_11;
  _Q[1][1] = a_var * delta_t_pow2;

  // Predict Step
  // x_pred = x + T*v
  // v_pred = v
  float x_pred[2] = {
    _position + delta_t * _velocity,
    _velocity
  };

  // Transpose of F
  float Ftrans[2][2] = {
    { _F[0][0], _F[1][0] },
    { _F[0][1], _F[1][1] }
  };

  float FP[2][2];
  float FPFtrans[2][2];
  multiplyMatrices(_F, _P_cov, FP);
  multiplyMatrices(FP, Ftrans, FPFtrans);
  float P_pred[2][2] = {
    { FPFtrans[0][0] + _Q[0][0], FPFtrans[0][1] + _Q[0][1] },
    { FPFtrans[1][0] + _Q[1][0], FPFtrans[1][1] + _Q[1][1] }
  };

  // Update Step
  _z = observation;  // Measurement

  // y = z - H*x_pred
  _y = _z - x_pred[0];  // Residual

  // S = (H * P * H' + R)
  // S = P + R, since H = [1, 0]
  _S_cov = P_pred[0][0] + _R;  // Residual covariance

  if (fabsf(_S) > 0.000001f) {
    // K = P_pred * H' * inv(S)
    _K[0] = P_pred[0][0] / _S_cov;  // Kalman Gain
    _K[1] = P_pred[1][0] / _S_cov;

    // Update state estimate
    _position = x_pred[0] + _K[0] * _y;
    _velocity = x_pred[1] + _K[1] * _y;

    // Update error covariance
    // P = (I - K*H)*P_pred
    float p_arg[2][2] = {
      { (1.0f - _K[0]), 0.0f },
      { -_K[1], 1.0f }
    };

    multiplyMatrices(p_arg, P_pred, _P_cov);
  } else {
    _P_cov[0][0] = _P_cov[0][1] = _P_cov[1][0] = _P_cov[1][1] = 0.0f;
  }

  return _position;
}

float KalmanFilter::changeVelocity() {
  return _velocity;
}
