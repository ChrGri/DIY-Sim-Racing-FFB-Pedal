#pragma once

#include <stdint.h>
#include "Main.h"

class LoadCell_ADS1256 {
private:
  float _zeroPoint = 0.0;
  float _varianceEstimate = 0.0;
  float _standardDeviationEstimate = 0.0;

public:
  LoadCell_ADS1256(uint8_t channel0=0, uint8_t channel1=1);
  float getReadingKg() const;
  // float getAngleMeasurement() const;
  void setLoadcellRating(uint8_t loadcellRating_u8) const;
  
public:
  void setZeroPoint();
  void estimateVariance();

public:
  float getVarianceEstimate() const { return _varianceEstimate; }
};
