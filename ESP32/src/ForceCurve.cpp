#include "ForceCurve.h"
#include "Arduino.h"

/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: force computation                            */
/*                                                                                            */
/**********************************************************************************************/
// see https://swharden.com/blog/2022-01-22-spline-interpolation/
float ForceCurveInterpolated::EvalForceCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
{
  const uint32_t num = config_st->payloadPedalConfig_st.quantityOfControl_u8;
  const float frac = constrain(fractionalPos_fl32, 0.0f, 1.0f) * 100.0f;

  // FIX: Changed <= to >= to correctly locate the segment index
  int i = 0; while (i < num && frac >= calc_st->travel_afl32[i]) i++;
  if (i) i--;
  
  float splineSegment_fl32 = (float)i;
  if (i != num - 1) {
      splineSegment_fl32 += (frac - calc_st->travel_afl32[i]) / (calc_st->travel_afl32[i+1] - calc_st->travel_afl32[i]);
  }

  // Safe clamping without external library dependencies
  const uint8_t maxSegment_u8 = (uint8_t)(num - 2);
  const uint8_t splineSegment_u8 = ((uint8_t)splineSegment_fl32 < maxSegment_u8) ? (uint8_t)splineSegment_fl32 : maxSegment_u8;

  const float f0 = calc_st->force_afl32[splineSegment_u8];
  const float f1 = calc_st->force_afl32[splineSegment_u8 + 1];
  const float a = calc_st->interpolatorA_pfl32[splineSegment_u8];
  const float b = calc_st->interpolatorB_pfl32[splineSegment_u8];

  const float t = (splineSegment_fl32 - (float)splineSegment_u8);
  float y_fl32 = f0 + (f1 - f0 + (a + (b - a) * t) * (1.0f - t)) * t;
  
  return calc_st->forceMin_fl32 + y_fl32 * 0.01f * fmaxf(calc_st->forceRange_fl32, 0.0f);
}


/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: gradient computation                         */
/*                                                                                            */
/**********************************************************************************************/

float ForceCurveInterpolated::EvalForceGradientCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32, bool normalized_b)
{
  const uint32_t num = config_st->payloadPedalConfig_st.quantityOfControl_u8;
  const float frac = constrain(fractionalPos_fl32, 0.0f, 1.0f) * 100.0f;
  
  int i = 0; while (i < num && frac >= calc_st->travel_afl32[i]) i++;
  if (i) i--;
  
  float splineSegment_fl32 = (float)i;
  if (i != num - 1) {
      splineSegment_fl32 += (frac - calc_st->travel_afl32[i]) / (calc_st->travel_afl32[i+1] - calc_st->travel_afl32[i]);
  }

  const uint8_t maxSegment_u8 = (uint8_t)(num - 2);
  const uint8_t splineSegment_u8 = ((uint8_t)splineSegment_fl32 < maxSegment_u8) ? (uint8_t)splineSegment_fl32 : maxSegment_u8;

  const float a = calc_st->interpolatorA_pfl32[splineSegment_u8];
  const float b = calc_st->interpolatorB_pfl32[splineSegment_u8];
  const float dx = calc_st->travel_afl32[splineSegment_u8 + 1] - calc_st->travel_afl32[splineSegment_u8]; 
  const float t = (splineSegment_fl32 - (float)splineSegment_u8); 
  const float dy_fl32 = calc_st->force_afl32[splineSegment_u8 + 1] - calc_st->force_afl32[splineSegment_u8]; 
  
  float yPrime_fl32 = 0.0f;
  if (fabsf(dx) > 0.0f)
  {
    /**********************************************************************************************
     * MATHEMATICAL DERIVATION: SPLINE GRADIENT CALCULATION
     * * 1. The Local Polynomial (Segment Equation)
     * The cubic spline interpolates a segment between points (x0, y0) and (x1, y1)
     * using a normalized local parameter 't', where t is in the range [0, 1].
     * * y(t) = (1 - t)*y0 + t*y1 + t*(1 - t) * [a*(1 - t) + b*t]
     * * 2. The Derivative with respect to 't' (Product Rule)
     * We need the rate of change with respect to t (dy/dt). We apply the product 
     * rule (u'v + uv') to the last term of the equation:
     * * Let u = t*(1 - t) = (t - t^2)       =>  u' = (1 - 2t)
     * Let v = [a*(1 - t) + b*t]           =>  v' = (b - a)
     * * dy/dt = -y0 + y1 + u'v + uv'
     * dy/dt = (y1 - y0) + (1 - 2t)*[a*(1 - t) + b*t] + t*(1 - t)*(b - a)
     * * 3. The Derivative with respect to 'x' (Chain Rule)
     * We want the physical gradient dy/dx, not dy/dt. We find this using the 
     * chain rule: dy/dx = (dy/dt) * (dt/dx).
     * * Since t is the fractional position within the segment:
     * t = (x - x0) / (x1 - x0) = (x - x0) / dx
     * Therefore, dt/dx = 1 / dx
     * * Giving us our final gradient equation for the segment:
     * dy/dx = [ (y1 - y0) + (1 - 2t)*[a*(1 - t) + b*t] + t*(1 - t)*(b - a) ] / dx
     * * 4. Normalization to Physical Axis Scaling
     * The spline mathematical evaluation operates strictly in percentages [0, 100].
     * To convert the resulting gradient from (dY% / dX%) to (dForce / dPos), 
     * we scale by the physical ranges of those axes:
     * * dForce/dPos = (dy% / dx%) * (Force_Range / Pos_Range)
     **********************************************************************************************/
    yPrime_fl32 = ((3.0f * (a - b) * t + 2.0f * b - 4.0f * a) * t + dy_fl32 + a) / dx;
  }
  
  if (normalized_b) return yPrime_fl32;

  if (fabsf(calc_st->stepperPosRange_fl32) <= 0.01f) return 0.0f;
  return yPrime_fl32 * calc_st->forceRange_fl32 / calc_st->stepperPosRange_fl32;
}



float ForceCurveInterpolated::EvalJoystickCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
{
  const uint32_t num = calc_st->numOfJoystickControl_u8;
  const float frac = constrain(fractionalPos_fl32, 0.0f, 1.0f) * 100.0f;

  if (frac < calc_st->joystickOrig_afl32[0]) return 0.0f;
  if (frac >= calc_st->joystickOrig_afl32[(int)num - 1]) return 100.0f;

  // FIX: Changed <= to >= to correctly locate the segment index
  int i = 0; while (i < num && frac >= calc_st->joystickOrig_afl32[i]) i++;
  if (i) i--;
  
  float splineSegment_fl32 = (float)i;
  if (i != num - 1) {
      splineSegment_fl32 += (frac - calc_st->joystickOrig_afl32[i]) / (calc_st->joystickOrig_afl32[i+1] - calc_st->joystickOrig_afl32[i]);
  }

  const uint8_t maxSegment_u8 = (uint8_t)(num - 2);
  const uint8_t splineSegment_u8 = ((uint8_t)splineSegment_fl32 < maxSegment_u8) ? (uint8_t)splineSegment_fl32 : maxSegment_u8;

  const float j0 = calc_st->joystickMapping_afl32[splineSegment_u8];
  const float j1 = calc_st->joystickMapping_afl32[splineSegment_u8 + 1];
  const float a = calc_st->joystickInterpolator_st.result_st.a_afl32[splineSegment_u8];
  const float b = calc_st->joystickInterpolator_st.result_st.b_afl32[splineSegment_u8];
  
  const float t = (splineSegment_fl32 - (float)splineSegment_u8);
  const float y_fl32 = j0 + t * (j1 - j0 + (a + (b - a) * t) * (1.0f - t));
  
  return y_fl32 * 0.01f * fmaxf(calc_st->joystickMapping_afl32[num - 1] - calc_st->joystickMapping_afl32[0], 0.0f);
}