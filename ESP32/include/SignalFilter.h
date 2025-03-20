#pragma once
#include <Arduino.h>

class KalmanFilter {
private:
  unsigned long _timeLastObservation;  // Last observation timestamp

  float _position;  // Estimated position
  float _velocity;  // Estimated velocity

  float _P_cov[2][2];   // Error covariance
  float _F[2][2];   // State transition matrix
  float _Q[2][2];   // Process noise covariance
  float _H[1][2];   // Measurement matrix
  float _R;         // Measurement noise covariance
  float _K[2];      // Kalman Gain
  float _z;         // Measurement (position)
  float _y;         // Measurement residual
  float _S_cov;         // Residual covariance

  // Model noise constant (higher value = faster pedal response)
  static constexpr float KF_MODEL_NOISE_FORCE_ACCELERATION = 20000.0f;

  void multiplyMatrices(float mat1[2][2], float mat2[2][2], float result[2][2]);

public:
  KalmanFilter(float varianceEstimate);
  float filteredValue(float observation, float command, uint8_t modelNoiseScaling_u8);
  float changeVelocity();
};
