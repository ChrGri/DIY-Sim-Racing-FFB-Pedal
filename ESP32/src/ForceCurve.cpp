#include "ForceCurve.h"
//#include "InterpolationLib.h"
#include "Arduino.h"



/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: force computation                            */
/*                                                                                            */
/**********************************************************************************************/

// see https://swharden.com/blog/2022-01-22-spline-interpolation/
float ForceCurve_Interpolated::EvalForceCubicSpline(const DAP_config_st* config_st, const DAP_calculationVariables_st* calc_st, float fractionalPos)
{

  float fractionalPos_lcl = constrain(fractionalPos, 0, 1);

  float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > (NUMBER_OF_SPLINE_SEGMENTS-1) ){splineSegment_u8 = NUMBER_OF_SPLINE_SEGMENTS-1;}
  float a = config_st->payLoadPedalConfig_.cubic_spline_param_a_array[splineSegment_u8];
  float b = config_st->payLoadPedalConfig_.cubic_spline_param_b_array[splineSegment_u8];

  float yOrig[ NUMBER_OF_SPLINE_SEGMENTS + 1 ];
  yOrig[0] = config_st->payLoadPedalConfig_.relativeForce_p000;
  yOrig[1] = config_st->payLoadPedalConfig_.relativeForce_p020;
  yOrig[2] = config_st->payLoadPedalConfig_.relativeForce_p040;
  yOrig[3] = config_st->payLoadPedalConfig_.relativeForce_p060;
  yOrig[4] = config_st->payLoadPedalConfig_.relativeForce_p080;
  yOrig[5] = config_st->payLoadPedalConfig_.relativeForce_p100;

  //double dx = 1.0f;
  float t = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
  float y = (1.0f - t) * yOrig[splineSegment_u8] + t * yOrig[splineSegment_u8 + 1] + t * (1.0f - t) * (a * (1.0f - t) + b * t);
  
  if (calc_st->Force_Range> 0)
  {
      y = calc_st->Force_Min + y / 100.0f * calc_st->Force_Range;
  }
  else
  {
    y = calc_st->Force_Min;
  }


  return y;
}


/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: gradient computation                         */
/*                                                                                            */
/**********************************************************************************************/

float ForceCurve_Interpolated::EvalForceGradientCubicSpline(const DAP_config_st* config_st, const DAP_calculationVariables_st* calc_st, float fractionalPos, bool normalized_b)
{

  float fractionalPos_lcl = constrain(fractionalPos, 0, 1);

  float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > 4){splineSegment_u8 = 4;}
  float a = config_st->payLoadPedalConfig_.cubic_spline_param_a_array[splineSegment_u8];
  float b = config_st->payLoadPedalConfig_.cubic_spline_param_b_array[splineSegment_u8];

  float yOrig[NUMBER_OF_SPLINE_SEGMENTS + 1];
  yOrig[0] = config_st->payLoadPedalConfig_.relativeForce_p000;
  yOrig[1] = config_st->payLoadPedalConfig_.relativeForce_p020;
  yOrig[2] = config_st->payLoadPedalConfig_.relativeForce_p040;
  yOrig[3] = config_st->payLoadPedalConfig_.relativeForce_p060;
  yOrig[4] = config_st->payLoadPedalConfig_.relativeForce_p080;
  yOrig[5] = config_st->payLoadPedalConfig_.relativeForce_p100;



  float Delta_x_orig = 100.0f; // total horizontal range [0,100]
  float dx = Delta_x_orig / NUMBER_OF_SPLINE_SEGMENTS; // spline segment horizontal range
  float t = (splineSegment_fl32 - (float)splineSegment_u8); // relative position in spline segment [0, 1]
  float dy = yOrig[splineSegment_u8 + 1] - yOrig[splineSegment_u8]; // spline segment vertical range
  float y_prime = 0.0f;
  if (fabsf(dx) > 0)
  {
      y_prime = dy / dx + (1.0f - 2.0f * t) * (a * (1.0f - t) + b * t) / dx + t * (1.0f - t) * (b - a) / dx;
  }
  // when the spline was identified, x and y were givin in the unit of percent --> 0-100
  // --> conversion of the gradient to the proper axis scaling is performed
  if (normalized_b == false)
  {
    float d_y_scale = calc_st->Force_Range / 100.0f;
    float d_x_scale=0.0f;
    if (fabs(calc_st->stepperPosRange) > 0.01f)
    {
        d_x_scale = 100.0f / calc_st->stepperPosRange;
    }
    
    y_prime *= d_x_scale * d_y_scale;
  }

  return y_prime;
}


