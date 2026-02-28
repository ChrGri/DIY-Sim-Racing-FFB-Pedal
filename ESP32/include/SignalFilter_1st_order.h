#ifndef SIGNAL_FILTER_1ST_ORDER_H
#define SIGNAL_FILTER_1ST_ORDER_H

#include "Arduino.h"
#include <stdint.h>
#include "Main.h"

class KalmanFilter1stOrder {
public:
  // Constructor
  KalmanFilter1stOrder(float varianceEstimate_fl32);

  // Main filter function
  float IRAM_ATTR_FLAG filteredValue(float measurement_fl32, float command_fl32, uint8_t modelNoiseScaling_u8);

  // Getters for the state variables
  float IRAM_ATTR_FLAG changeVelocity();
  
private:
  // State vector [position; velocity]
  float stateX_afl32[2];
  
  // State covariance matrix P (2x2)
  float stateCovarianceP_aafl32[2][2];
  
  // State transition matrix F (2x2)
  float stateTransitionF_aafl32[2][2];

  // Process noise covariance matrix Q (2x2)
  float processNoiseQ_aafl32[2][2];
  
  // Measurement matrix H (1x2)
  float measurementMatrixH_aafl32[1][2];
  
  // Kalman gain K (2x1)
  float kalmanGainK_afl32[2];
  
  // Measurement noise covariance R (scalar)
  float measurementNoiseR_fl32;
  
  // Timestamp of the last observation
  unsigned long timeLastObservation_u32;
};

#endif // SIGNAL_FILTER_1ST_ORDER_H