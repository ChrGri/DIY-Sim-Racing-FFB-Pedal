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
  float fractionalPos_float=fractionalPos_lcl*100.0f;
  //float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  uint32_t numberOfPoints_u32 = config_st->payLoadPedalConfig_.quantityOfControl;
  float numberOfSplineSegments = (config_st->payLoadPedalConfig_.quantityOfControl-1); // quantityOfControl is number of points
  float splineSegment_fl32 = 0; // initialize to 0, because (fractionalPos_float > calc_st->travel[i]) wont fin it otherwise

  for(int i=0; i < numberOfPoints_u32; i++)
  {
    if(fractionalPos_float > calc_st->travel[i])
    {
      if(i== (numberOfSplineSegments) )
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff= (fractionalPos_float-(float)calc_st->travel[i])/(float)(calc_st->travel[i+1]-calc_st->travel[i]);
        splineSegment_fl32=(float)i+diff;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > (numberOfSplineSegments) )
  {
    splineSegment_u8 = numberOfSplineSegments;
  }
  float a = calc_st->interpolatorA[splineSegment_u8];
  float b = calc_st->interpolatorB[splineSegment_u8];

  float yOrig[numberOfPoints_u32];

  for(int i=0; i<numberOfPoints_u32; i++)
  {
    yOrig[i]=calc_st->force[i];
  }

  //double dx = 1.0f;
  float t = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
  float y=0.0f;

  if(splineSegment_u8 >= numberOfSplineSegments)
  {
    y = yOrig[splineSegment_u8];
  }
  else
  {
    y = (1.0f - t) * yOrig[splineSegment_u8] + t * yOrig[splineSegment_u8 + 1] + t * (1.0f - t) * (a * (1.0f - t) + b * t);
  }
  
  
  if (calc_st->forceRange_f> 0)
  {
      y = calc_st->forceMin_f + y / 100.0f * calc_st->forceRange_f;
  }
  else
  {
    y = calc_st->forceMin_f;
  }
  /*
  if(fractionalPos>0.9)
  {
    Serial.print("force y=");
    Serial.print(y);
    Serial.print(", splineSegment_fl32=");
    Serial.print(splineSegment_fl32);
    Serial.print(", splineSegment_u8=");
    Serial.println(splineSegment_u8);    
    Serial.print("numberOfPoints_u32=");
    Serial.print(numberOfPoints_u32);    
    Serial.print(", fractionalPos_float=");
    Serial.print(fractionalPos_float);    
    Serial.print(", interpolar a=");
    Serial.print(a); 
    Serial.print(", interpolar b=");
    Serial.println(b);     
  }
  */
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
  float fractionalPos_float=fractionalPos_lcl*100.0f;
  
  float numberOfSplineSegments = (config_st->payLoadPedalConfig_.quantityOfControl-1); // quantityOfControl is number of points
  uint32_t numberOfPoints_u32 = config_st->payLoadPedalConfig_.quantityOfControl;
  float splineSegment_fl32 = 0; // initialize to 0, because (fractionalPos_float > calc_st->travel[i]) wont fin it otherwise

  for(int i=0; i < numberOfPoints_u32; i++)
  {
    if(fractionalPos_float > calc_st->travel[i])
    {
      if(i== numberOfSplineSegments )
      {
        splineSegment_fl32=(float)i;
      }
      else
      {
        float diff= (fractionalPos_float-(float)calc_st->travel[i])/(float)(calc_st->travel[i+1]-calc_st->travel[i]);
        splineSegment_fl32=(float)i+diff;
      }  
    }
    else
    {
      break;
    }
  }
  uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
  
  if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
  if (splineSegment_u8 > (numberOfSplineSegments) )
  {
    splineSegment_u8 = numberOfSplineSegments;
  }
  float a = calc_st->interpolatorA[splineSegment_u8];
  float b = calc_st->interpolatorB[splineSegment_u8];

  float yOrig[numberOfPoints_u32];
  for(int i=0; i<numberOfPoints_u32; i++)
  {
    yOrig[i]=calc_st->force[i];
  }



  float Delta_x_orig = 100.0f; // total horizontal range [0,100]
  float dx = Delta_x_orig / numberOfSplineSegments; // spline segment horizontal range
  float t = (splineSegment_fl32 - (float)splineSegment_u8); // relative position in spline segment [0, 1]
  float dy =0.0f;
  if(splineSegment_u8 >= numberOfSplineSegments)
  {
    dy=0;
  }
  else
  {
    dy = yOrig[splineSegment_u8 + 1] - yOrig[splineSegment_u8]; // spline segment vertical range
  }
  
  float y_prime = 0.0f;
  if (fabsf(dx) > 0)
  {
      y_prime = dy / dx + (1.0f - 2.0f * t) * (a * (1.0f - t) + b * t) / dx + t * (1.0f - t) * (b - a) / dx;
  }
  // when the spline was identified, x and y were givin in the unit of percent --> 0-100
  // --> conversion of the gradient to the proper axis scaling is performed
  if (normalized_b == false)
  {
    float d_y_scale = calc_st->forceRange_f / 100.0f;
    float d_x_scale=0.0f;
    if (fabs(calc_st->stepperPosRange) > 0.01f)
    {
        d_x_scale = 100.0f / calc_st->stepperPosRange;
    }
    
    y_prime *= d_x_scale * d_y_scale;
  }
  /*
  if(fractionalPos>0.9)
  {
    Serial.print("forcegradient y_prime=");
    Serial.print(y_prime);
    Serial.print(", splineSegment_fl32=");
    Serial.print(splineSegment_fl32);
    Serial.print(", splineSegment_u8=");
    Serial.println(splineSegment_u8);    
    Serial.print("numberOfPoints_u32=");
    Serial.print(numberOfPoints_u32);    
    Serial.print(", fractionalPos_float=");
    Serial.print(fractionalPos_float);    
    Serial.print(", interpolar a=");
    Serial.print(a); 
    Serial.print(", interpolar b=");
    Serial.println(b);
    Serial.print("dx=");
    Serial.print(dx);     
    Serial.print(", t=");
    Serial.print(t);
    Serial.print(", dy=");
    Serial.print(dy);
    if(splineSegment_u8==numberOfSplineSegments)
    {
      Serial.print(", yOrig[splineSegment_u8]=");
      Serial.print(yOrig[splineSegment_u8 ]);
      Serial.print(", yOrig[splineSegment_u8-1]=");
      Serial.println(yOrig[splineSegment_u8-1]);
    }
    else
    {
      Serial.print(", yOrig[splineSegment_u8 + 1]=");
      Serial.print(yOrig[splineSegment_u8 + 1]);
      Serial.print(", yOrig[splineSegment_u8]=");
      Serial.println(yOrig[splineSegment_u8]);
    }

  }
  */
  return y_prime;
}



float ForceCurve_Interpolated::EvalJoystickCubicSpline(const DAP_config_st* config_st, const DAP_calculationVariables_st* calc_st, float fractionalPos)
{

  float fractionalPos_lcl = constrain(fractionalPos, 0, 1);
  float fractionalPos_float=fractionalPos_lcl*100.0f;
  //float splineSegment_fl32 = fractionalPos_lcl * 5.0f;
  uint32_t numberOfPoints_u32 = calc_st->numOfJoystickControl;
  float numberOfSplineSegments = calc_st->numOfJoystickControl-1; // quantityOfControl is number of points
  float splineSegment_fl32 = 0; // initialize to 0, because (fractionalPos_float > calc_st->travel[i]) wont fin it otherwise
  float y=0.0f;
  if(fractionalPos_float < calc_st->joystickOrig[0])
  {
    y=0.0f;
  }
  if(fractionalPos_float >= calc_st->joystickOrig[0] && fractionalPos_float < calc_st->joystickOrig[(int)numberOfSplineSegments])
  {
    for(int i=0; i < numberOfPoints_u32; i++)
    {
      if(fractionalPos_float > calc_st->joystickOrig[i])
      {
        if(i== (numberOfSplineSegments) )
        {
          splineSegment_fl32=(float)i;
        }
        else
        {
          float diff= (fractionalPos_float-(float)calc_st->joystickOrig[i])/(float)(calc_st->joystickOrig[i+1]-calc_st->joystickOrig[i]);
          splineSegment_fl32=(float)i+diff;
        }  
      }
      else
      {
        break;
      }
    }
    uint8_t splineSegment_u8 = (uint8_t)floor(splineSegment_fl32);
    
    if (splineSegment_u8 < 0){splineSegment_u8 = 0;}
    if (splineSegment_u8 > (numberOfSplineSegments) )
    {
      splineSegment_u8 = numberOfSplineSegments;
    }
    float a = calc_st->joystickInterpolarter._result.a[splineSegment_u8];
    float b = calc_st->joystickInterpolarter._result.b[splineSegment_u8];

    float yOrig[numberOfPoints_u32];

    for(int i=0; i<numberOfPoints_u32; i++)
    {
      yOrig[i]=calc_st->joystickMapping[i];
    }

    //double dx = 1.0f;
    float t = (splineSegment_fl32 - (float)splineSegment_u8);// / dx;
    

    if(splineSegment_u8 >= numberOfSplineSegments)
    {
      y = yOrig[splineSegment_u8];
    }
    else
    {
      y = (1.0f - t) * yOrig[splineSegment_u8] + t * yOrig[splineSegment_u8 + 1] + t * (1.0f - t) * (a * (1.0f - t) + b * t);
    }
    
    float joystickMappingRange=calc_st->joystickMapping[(int)numberOfSplineSegments]-calc_st->joystickMapping[0];
    if (joystickMappingRange> 0)
    {
        y =  y / 100.0f * joystickMappingRange;
    }
    else
    {
      y = 0.0f;
    }
    //debug
    /*
    Serial.print("joystick y=");
    Serial.print(y);
    Serial.print(", splineSegment_fl32=");
    Serial.print(splineSegment_fl32);
    Serial.print(", splineSegment_u8=");
    Serial.println(splineSegment_u8);    
    Serial.print("numberOfPoints_u32=");
    Serial.print(numberOfPoints_u32);    
    Serial.print(", fractionalPos_float=");
    Serial.print(fractionalPos_float);    
    Serial.print(", interpolar a=");
    Serial.print(a); 
    Serial.print(", interpolar b=");
    Serial.println(b);
    */  
  }
  if (fractionalPos_float>= calc_st->joystickOrig[(int)numberOfSplineSegments])
  {
    /* code */
    y=100.0f;
  }

  return y;
  
}