#include "ForceCurve.h"
//#include "InterpolationLib.h"
#include "Arduino.h"



/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: force computation                            */
/*                                                                                            */
/**********************************************************************************************/
// see https://swharden.com/blog/2022-01-22-spline-interpolation/
float IRAM_ATTR_FLAG ForceCurveInterpolated::EvalForceCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
{

  float fractionalPosLcl_fl32 = constrain(fractionalPos_fl32, 0, 1);
  float fractionalPosPercent_fl32 = fractionalPosLcl_fl32 * 100.0f;
  //float splineSegment_fl32 = fractionalPosLcl_fl32 * 5.0f;
  uint32_t numberOfPoints_u32 = config_st->payloadPedalConfig_st.quantityOfControl_u8;
  float numberOfSplineSegments_fl32 = (config_st->payloadPedalConfig_st.quantityOfControl_u8-1); // quantityOfControl_u8 is number of points
  float splineSegment_fl32 = 0; // initialize to 0, because (fractionalPos_float > calc_st->travel_afl32[i]) wont fin it otherwise

  for(int i=0; i < numberOfPoints_u32; i++)
  {
    if(fractionalPosPercent_fl32 > calc_st->travel_afl32[i])
    {
      if(i== (numberOfSplineSegments_fl32) )
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff_fl32 = (fractionalPosPercent_fl32-(float)calc_st->travel_afl32[i])/(float)(calc_st->travel_afl32[i+1]-calc_st->travel_afl32[i]);
        splineSegment_fl32=(float)i+diff_fl32;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  // if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  uint8_t maxSegmentIndex_u8 = (uint8_t)(numberOfSplineSegments_fl32 - 1);
  if (splineSegment_u8 > maxSegmentIndex_u8)
  {
    splineSegment_u8 = maxSegmentIndex_u8;
  }
  float a_fl32 = calc_st->interpolatorA_pfl32[splineSegment_u8];
  float b_fl32 = calc_st->interpolatorB_pfl32[splineSegment_u8];

  float yOrig[numberOfPoints_u32];

  for(int i=0; i<numberOfPoints_u32; i++)
  {
    yOrig[i]=calc_st->force_afl32[i];
  }

  //double dx = 1.0f;
  float t_fl32 = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
  float y_fl32=0.0f;

  y_fl32 = (1.0f - t_fl32) * yOrig[splineSegment_u8] + t_fl32 * yOrig[splineSegment_u8 + 1] + t_fl32 * (1.0f - t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32);
  
  
  if (calc_st->forceRange_fl32> 0)
  {
      y_fl32 = calc_st->forceMin_fl32 + y_fl32 / 100.0f * calc_st->forceRange_fl32;
  }
  else
  {
    y_fl32 = calc_st->forceMin_fl32;
  }
  /*
  if(fractionalPos>0.9)
  {
    ActiveSerial->print("force y=");
    ActiveSerial->print(y);
    ActiveSerial->print(", splineSegment_fl32=");
    ActiveSerial->print(splineSegment_fl32);
    ActiveSerial->print(", splineSegment_u8=");
    ActiveSerial->println(splineSegment_u8);    
    ActiveSerial->print("numberOfPoints_u32=");
    ActiveSerial->print(numberOfPoints_u32);    
    ActiveSerial->print(", fractionalPos_float=");
    ActiveSerial->print(fractionalPos_float);    
    ActiveSerial->print(", interpolar a=");
    ActiveSerial->print(a); 
    ActiveSerial->print(", interpolar b=");
    ActiveSerial->println(b);     
  }
  */
  return y_fl32;
  
}


/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: gradient computation                         */
/*                                                                                            */
/**********************************************************************************************/

float IRAM_ATTR_FLAG ForceCurveInterpolated::EvalForceGradientCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32, bool normalized_b)
{
  float fractionalPosLcl_fl32 = constrain(fractionalPos_fl32, 0, 1);
  float fractionalPosPercent_fl32 = fractionalPosLcl_fl32*100.0f;
  
  float numberOfSplineSegments_fl32 = (config_st->payloadPedalConfig_st.quantityOfControl_u8-1); // quantityOfControl_u8 is number of points
  uint32_t numberOfPoints_u32 = config_st->payloadPedalConfig_st.quantityOfControl_u8;
  float splineSegment_fl32 = 0.0f; // initialize to 0, because (fractionalPos_float > calc_st->travel_afl32[i]) wont fin it otherwise

  for(int i=0; i < numberOfPoints_u32; i++)
  {
    if(fractionalPosPercent_fl32 >= calc_st->travel_afl32[i])
    {
      if(i== numberOfSplineSegments_fl32 )
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff_fl32 = (fractionalPosPercent_fl32-(float)calc_st->travel_afl32[i])/(float)(calc_st->travel_afl32[i+1]-calc_st->travel_afl32[i]);
        splineSegment_fl32=(float)i+diff_fl32;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  // if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  uint8_t maxSegmentIndex_u8 = (uint8_t)(numberOfSplineSegments_fl32 - 1);
  if (splineSegment_u8 > maxSegmentIndex_u8)
  {
    splineSegment_u8 = maxSegmentIndex_u8;
  }
  float a_fl32 = calc_st->interpolatorA_pfl32[splineSegment_u8];
  float b_fl32 = calc_st->interpolatorB_pfl32[splineSegment_u8];

  float yOrig[numberOfPoints_u32];
  for(int i=0; i<numberOfPoints_u32; i++)
  {
    yOrig[i]=calc_st->force_afl32[i];
  }

  
  // Calculate true dynamic dx_fl32 based on the actual spacing of the control points
  float dx_fl32 = calc_st->travel_afl32[splineSegment_u8 + 1] - calc_st->travel_afl32[splineSegment_u8];
  
  float t_fl32 = (splineSegment_fl32 - (float)splineSegment_u8); // relative position in spline segment [0, 1]
  float dy_fl32 =0.0f;
  
  dy_fl32 = yOrig[splineSegment_u8 + 1] - yOrig[splineSegment_u8]; // spline segment vertical range
  
  float yPrime_fl32 = 0.0f;
  if (fabsf(dx_fl32) > 0)
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
      yPrime_fl32 = dy_fl32 / dx_fl32 + (1.0f - 2.0f * t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32) / dx_fl32 + t_fl32 * (1.0f - t_fl32) * (b_fl32 - a_fl32) / dx_fl32;
  }
  // when the spline was identified, x and y were givin in the unit of percent --> 0-100
  // --> conversion of the gradient to the proper axis scaling is performed
  if (normalized_b == false)
  {
    float dYScale_fl32 = calc_st->forceRange_fl32 / 100.0f;
    float dXScale_fl32=0.0f;
    if (fabsf(calc_st->stepperPosRange_fl32) > 0.01f)
    {
      dXScale_fl32 = 100.0f / calc_st->stepperPosRange_fl32;
    }
    
    yPrime_fl32 *= dXScale_fl32 * dYScale_fl32;
  }
  /*
  if(fractionalPos>0.9)
  {
    ActiveSerial->print("forcegradient y_prime=");
    ActiveSerial->print(y_prime);
    ActiveSerial->print(", splineSegment_fl32=");
    ActiveSerial->print(splineSegment_fl32);
    ActiveSerial->print(", splineSegment_u8=");
    ActiveSerial->println(splineSegment_u8);    
    ActiveSerial->print("numberOfPoints_u32=");
    ActiveSerial->print(numberOfPoints_u32);    
    ActiveSerial->print(", fractionalPos_float=");
    ActiveSerial->print(fractionalPos_float);    
    ActiveSerial->print(", interpolar a=");
    ActiveSerial->print(a); 
    ActiveSerial->print(", interpolar b=");
    ActiveSerial->println(b);
    ActiveSerial->print("dx=");
    ActiveSerial->print(dx);     
    ActiveSerial->print(", t=");
    ActiveSerial->print(t);
    ActiveSerial->print(", dy=");
    ActiveSerial->print(dy);
    if(splineSegment_u8==numberOfSplineSegments)
    {
      ActiveSerial->print(", yOrig[splineSegment_u8]=");
      ActiveSerial->print(yOrig[splineSegment_u8 ]);
      ActiveSerial->print(", yOrig[splineSegment_u8-1]=");
      ActiveSerial->println(yOrig[splineSegment_u8-1]);
    }
    else
    {
      ActiveSerial->print(", yOrig[splineSegment_u8 + 1]=");
      ActiveSerial->print(yOrig[splineSegment_u8 + 1]);
      ActiveSerial->print(", yOrig[splineSegment_u8]=");
      ActiveSerial->println(yOrig[splineSegment_u8]);
    }

  }
  */
  return yPrime_fl32;
}



float IRAM_ATTR_FLAG ForceCurveInterpolated::EvalJoystickCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
{

  float fractionalPosLcl_fl32 = constrain(fractionalPos_fl32, 0, 1);
  float fractionalPosPercent_fl32 = fractionalPosLcl_fl32*100.0f;
  //float splineSegment_fl32 = fractionalPosLcl_fl32 * 5.0f;
  uint32_t numberOfPoints_u32 = calc_st->numOfJoystickControl_u8;
  float numberOfSplineSegments_fl32 = calc_st->numOfJoystickControl_u8-1; // quantityOfControl_u8 is number of points
  float splineSegment_fl32 = 0; // initialize to 0, because (fractionalPos_float > calc_st->travel_afl32[i]) wont fin it otherwise
  float y_fl32=0.0f;
  if(fractionalPosPercent_fl32 < calc_st->joystickOrig_afl32[0])
  {
    y_fl32=0.0f;
  }
  if(fractionalPosPercent_fl32 >= calc_st->joystickOrig_afl32[0] && fractionalPosPercent_fl32 < calc_st->joystickOrig_afl32[(int)numberOfSplineSegments_fl32])
  {
    for(int i=0; i < numberOfPoints_u32; i++)
    {
      if(fractionalPosPercent_fl32 > calc_st->joystickOrig_afl32[i])
      {
        if(i== (numberOfSplineSegments_fl32) )
        {
          splineSegment_fl32=(float)i;
        }
        else
        {
          float diff_fl32 = (fractionalPosPercent_fl32-(float)calc_st->joystickOrig_afl32[i])/(float)(calc_st->joystickOrig_afl32[i+1]-calc_st->joystickOrig_afl32[i]);
          splineSegment_fl32=(float)i+diff_fl32;
        }  
      }
      else
      {
        break;
      }
    }
    uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
    
    // if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
    uint8_t maxSegmentIndex_u8 = (uint8_t)(numberOfSplineSegments_fl32 - 1);
    if (splineSegment_u8 > maxSegmentIndex_u8)
    {
      splineSegment_u8 = maxSegmentIndex_u8;
    }
    float a_fl32 = calc_st->joystickInterpolator_st.result_st.a_afl32[splineSegment_u8];
    float b_fl32 = calc_st->joystickInterpolator_st.result_st.b_afl32[splineSegment_u8];

    float yOrig[numberOfPoints_u32];

    for(int i=0; i<numberOfPoints_u32; i++)
    {
      yOrig[i]=calc_st->joystickMapping_afl32[i];
    }

    //double dx = 1.0f;
    float t_fl32 = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
    

    y_fl32 = (1.0f - t_fl32) * yOrig[splineSegment_u8] + t_fl32 * yOrig[splineSegment_u8 + 1] + t_fl32 * (1.0f - t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32);
    
    float joystickMappingRange_fl32 = calc_st->joystickMapping_afl32[(int)numberOfSplineSegments_fl32]-calc_st->joystickMapping_afl32[0];
    if (joystickMappingRange_fl32> 0)
    {
        y_fl32 =  y_fl32 / 100.0f * joystickMappingRange_fl32;
    }
    else
    {
      y_fl32 = 0.0f;
    }
    //debug
    /*
    ActiveSerial->print("joystick y=");
    ActiveSerial->print(y);
    ActiveSerial->print(", splineSegment_fl32=");
    ActiveSerial->print(splineSegment_fl32);
    ActiveSerial->print(", splineSegment_u8=");
    ActiveSerial->println(splineSegment_u8);    
    ActiveSerial->print("numberOfPoints_u32=");
    ActiveSerial->print(numberOfPoints_u32);    
    ActiveSerial->print(", fractionalPos_float=");
    ActiveSerial->print(fractionalPos_float);    
    ActiveSerial->print(", interpolar a=");
    ActiveSerial->print(a); 
    ActiveSerial->print(", interpolar b=");
    ActiveSerial->println(b);
    */  
  }
  if (fractionalPosPercent_fl32>= calc_st->joystickOrig_afl32[(int)numberOfSplineSegments_fl32])
  {
    /* code */
    y_fl32=100.0f;
  }

  return y_fl32;
  
}
