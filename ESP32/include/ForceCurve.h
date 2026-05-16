#pragma once

#include "Main.h"
#include "DiyActivePedal_types.h"

// The number of segments, which are defined for the spline
//#define NUMBER_OF_SPLINE_SEGMENTS 5

class ForceCurveInterpolated {

public:
  float IRAM_ATTR_FLAG EvalForceCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32);
  float IRAM_ATTR_FLAG EvalForceGradientCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32, bool normalized_b);
  float IRAM_ATTR_FLAG EvalJoystickCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32);
};
