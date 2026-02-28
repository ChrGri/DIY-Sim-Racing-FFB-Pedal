#ifndef SIGNAL_FILTER_1ST_ORDER_H
#define SIGNAL_FILTER_1ST_ORDER_H

#include "Arduino.h"
#include <stdint.h>
#include "Main.h"

class KalmanFilter1stOrder {
public:
  // Constructor
  KalmanFilter1stOrder(float varianceEstimate);

  // Main filter function
  float IRAM_ATTR_FLAG filteredValue(float measurement, float command, uint8_t modelNoiseScaling_u8);

  // Getters for the state variables
  float IRAM_ATTR_FLAG changeVelocity();
  
private:
  // State vector [position; velocity]
  float _x[2];
  
  // State covariance matrix P (2x2)
  float _P_cov[2][2];
  
  // State transition matrix F (2x2)
  float _F[2][2];

  // Process noise covariance matrix Q (2x2)
  float _Q[2][2];
  
  // Measurement matrix H (1x2)
  float _H[1][2];
  
  // Kalman gain K (2x1)
  float _K[2];
  
  // Measurement noise covariance R (scalar)
  float _R;
  
  // Timestamp of the last observation
  unsigned long _timeLastObservation;
};

#endif // SIGNAL_FILTER_1ST_ORDER_H