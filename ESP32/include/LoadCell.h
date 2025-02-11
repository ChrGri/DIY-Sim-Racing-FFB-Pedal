#pragma once

#include <stdint.h>
#include "Main.h"

#define DIFFERENTIAL_0_1 0b00000001
#define DIFFERENTIAL_2_3 0b00100011
#define DIFFERENTIAL_4_5 0b01000101
#define DIFFERENTIAL_6_7 0b01100111

class LoadCell_ADS1256 {
private:
  float _zeroPoint = 0.0;
  float _varianceEstimate = 0.0;
  float _standardDeviationEstimate = 0.0;
  float _conversionFactor = 0.0;
  uint8_t _pedalType=PEDAL_TYPE_BRAKE;

public:
  LoadCell_ADS1256(uint8_t pdType=PEDAL_TYPE_BRAKE, uint8_t differential=DIFFERENTIAL_0_1);
  float getReadingKg(uint8_t differential=DIFFERENTIAL_0_1) const;
  void setLoadcellRating(uint8_t loadcellRating_u8);
  void clearADCBuffer(uint8_t differential=DIFFERENTIAL_0_1);

public:
void setZeroPoint(uint8_t differential=DIFFERENTIAL_0_1);
void estimateVariance(uint8_t differential=DIFFERENTIAL_0_1);
  
public:
  float getVarianceEstimate() const { return _varianceEstimate; }
};
