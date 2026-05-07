#pragma once

#include <stdint.h>
#include "Main.h"


#ifdef USES_ADS1220


/*  Uses ADS1256 */
class LoadCellAds1220 
{
private:
  float zeroPoint_fl32 = 0.0f;
  float varianceEstimate_fl32 = 0.0f;
  float standardDeviationEstimate_fl32 = 0.0f;

public:
  LoadCellAds1220();
  float readLoadcellWeightInKg() const;
  void setLoadcellRating(uint8_t loadcellRating_u8) const;
  void estimateBiasAndVariance();
  
  float getVarianceEstimate() const 
  { 
      return varianceEstimate_fl32; 
  }
  
  float getBiasEstimate() const 
  { 
      return zeroPoint_fl32; 
  }

  float getStandardDeviationEstimate() const 
  { 
      return standardDeviationEstimate_fl32; 
  }

};

#endif
