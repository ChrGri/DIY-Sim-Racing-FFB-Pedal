#pragma once

#include "DiyActivePedal_types.h"

// The number of segments, which are defined for the spline
//#define NUMBER_OF_SPLINE_SEGMENTS 5

class ForceCurveInterpolated {

public:
  float EvalForceCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32);
  float EvalForceGradientCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32, bool normalized_b);
  float EvalJoystickCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32);
};
