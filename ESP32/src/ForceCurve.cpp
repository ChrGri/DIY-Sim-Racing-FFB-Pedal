#include "ForceCurve.h"
//#include "InterpolationLib.h"
#include "Arduino.h"



/**********************************************************************************************/
/*                                                                                            */
/*                         Spline interpolation: force computation                            */
/*                                                                                            */
/**********************************************************************************************/
// see https://swharden.com/blog/2022-01-22-spline-interpolation/
float ForceCurveInterpolated::EvalForceCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
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
  if (splineSegment_u8 > (numberOfSplineSegments_fl32) )
  {
    splineSegment_u8 = numberOfSplineSegments_fl32;
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

  if(splineSegment_u8 >= numberOfSplineSegments_fl32)
  {
    y_fl32 = yOrig[splineSegment_u8];
  }
  else
  {
    y_fl32 = (1.0f - t_fl32) * yOrig[splineSegment_u8] + t_fl32 * yOrig[splineSegment_u8 + 1] + t_fl32 * (1.0f - t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32);
  }
  
  
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

float ForceCurveInterpolated::EvalForceGradientCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32, bool normalized_b)
{
  float fractionalPosLcl_fl32 = constrain(fractionalPos_fl32, 0, 1);
  float fractionalPosPercent_fl32 = fractionalPosLcl_fl32*100.0f;
  
  float numberOfSplineSegments_fl32 = (config_st->payloadPedalConfig_st.quantityOfControl_u8-1); // quantityOfControl_u8 is number of points
  uint32_t numberOfPoints_u32 = config_st->payloadPedalConfig_st.quantityOfControl_u8;
  float splineSegment_fl32 = 0.0f; // initialize to 0, because (fractionalPos_float > calc_st->travel_afl32[i]) wont fin it otherwise

  for(int i=0; i < numberOfPoints_u32; i++)
  {
    if(fractionalPosPercent_fl32 > calc_st->travel_afl32[i])
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
  if (splineSegment_u8 > (numberOfSplineSegments_fl32) )
  {
    splineSegment_u8 = numberOfSplineSegments_fl32;
  }
  float a_fl32 = calc_st->interpolatorA_pfl32[splineSegment_u8];
  float b_fl32 = calc_st->interpolatorB_pfl32[splineSegment_u8];

  float yOrig[numberOfPoints_u32];
  for(int i=0; i<numberOfPoints_u32; i++)
  {
    yOrig[i]=calc_st->force_afl32[i];
  }



  float deltaXOrig_fl32 = 100.0f; // total horizontal range [0,100]
  float dx_fl32 = deltaXOrig_fl32 / numberOfSplineSegments_fl32; // spline segment horizontal range
  float t_fl32 = (splineSegment_fl32 - (float)splineSegment_u8); // relative position in spline segment [0, 1]
  float dy_fl32 =0.0f;
  if(splineSegment_u8 >= numberOfSplineSegments_fl32)
  {
    dy_fl32=0;
  }
  else
  {
    dy_fl32 = yOrig[splineSegment_u8 + 1] - yOrig[splineSegment_u8]; // spline segment vertical range
  }
  
  float yPrime_fl32 = 0.0f;
  if (fabsf(dx_fl32) > 0)
  {
      yPrime_fl32 = dy_fl32 / dx_fl32 + (1.0f - 2.0f * t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32) / dx_fl32 + t_fl32 * (1.0f - t_fl32) * (b_fl32 - a_fl32) / dx_fl32;
  }
  // when the spline was identified, x and y were givin in the unit of percent --> 0-100
  // --> conversion of the gradient to the proper axis scaling is performed
  if (normalized_b == false)
  {
    float dYScale_fl32 = calc_st->forceRange_fl32 / 100.0f;
    float dXScale_fl32=0.0f;
    if (fabs(calc_st->stepperPosRange_fl32) > 0.01f)
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



float ForceCurveInterpolated::EvalJoystickCubicSpline(const DapConfig_t* config_st, const DapCalculationVariables_t* calc_st, float fractionalPos_fl32)
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
    if (splineSegment_u8 > (numberOfSplineSegments_fl32) )
    {
      splineSegment_u8 = numberOfSplineSegments_fl32;
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
    

    if(splineSegment_u8 >= numberOfSplineSegments_fl32)
    {
      y_fl32 = yOrig[splineSegment_u8];
    }
    else
    {
      y_fl32 = (1.0f - t_fl32) * yOrig[splineSegment_u8] + t_fl32 * yOrig[splineSegment_u8 + 1] + t_fl32 * (1.0f - t_fl32) * (a_fl32 * (1.0f - t_fl32) + b_fl32 * t_fl32);
    }
    
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
