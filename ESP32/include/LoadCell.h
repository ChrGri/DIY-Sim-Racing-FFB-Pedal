#pragma once

#include "Main.h"
#include <stdint.h>

#ifndef USES_ADS1220



class LoadCellAds1256 {
private:
  float _zeroPoint = 0.0;
  float _varianceEstimate = 0.0;
  float _standardDeviationEstimate = 0.0;

public:
  LoadCellAds1256(uint8_t channel0=0, uint8_t channel1=1);
  float getReadingKg() const;
  // float getAngleMeasurement() const;
  void setLoadcellRating(uint8_t loadcellRating_u8) const;
  void estimateBiasAndVariance();
  float getVarianceEstimate() const { return _varianceEstimate; }
  float getShiftingEstimate() const { return _zeroPoint; }
  float getSTDEstimate() const { return _standardDeviationEstimate; }
};


#endif