#pragma once
#include <Arduino.h>

class KalmanFilter {
private:
  unsigned long _timeLastObservation;

public:
  KalmanFilter(float varianceEstimate);

  float filteredValue(float observation, float command, uint8_t modelNoiseScaling_u8);
  float changeVelocity();
};
