#pragma once

#include "DiyActivePedal_types.h"
#include <MovingAverageFilter.h>
#include "FastTrig.h"

static const long s_absActiveTimePerTriggerMillis_i32 = 100;
static const long s_rpmActiveTimePerTriggerMillis_i32 = 100;
static const long s_bpActiveTimePerTriggerMillis_i32 = 100;
static const long s_wsActiveTimePerTriggerMillis_i32 = 100;
static const long s_cvActiveTimePerTriggerMillis_i32 = 100;
static float s_rpmValueLast_fl32 = 0.0f;

enum class TrackCondition
{
  Dry,
  MostlyDry,
  VeryLightWet,
  LightWet,
  ModeratelyWet,
  VeryWet,
  ExtremelyWet
};
class ABSOscillation {
private:
  long timeLastTriggerMillis_i32;
  long absTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;

public:
  float absOscillationForceOffset_fl32 = 0.0f;
  float absOscillationPositionOffset_fl32 = 0.0f;
  ABSOscillation()
    : timeLastTriggerMillis_i32(0)
  {}

public:
  void IRAM_ATTR_FLAG trigger()
  {
    timeLastTriggerMillis_i32 = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st, uint8_t absPattern_u8, uint8_t absForceOrTarvelBit_u8)
  {

    long timeNowMillis = millis();
    float timeSinceTrigger_fl32 = (timeNowMillis - timeLastTriggerMillis_i32);
    
    

    if (timeSinceTrigger_fl32 > s_absActiveTimePerTriggerMillis_i32)
    {
      absTimeMillis_i32 = 0;
      absOscillationForceOffset_fl32 = 0.0f;
      absOscillationPositionOffset_fl32 = 0.0f;
    }
    else
    {
      //frequency depend on road condition
      float absFreq_fl32 = calcVars_st->absFrequency_fl32;
      absFreq_fl32 = absFreq_fl32*(1.0f + ((float)calcVars_st->trackCondition_u8) * 0.1f );
      absFreq_fl32 = constrain(absFreq_fl32, 0.0f, 50.0f);
      
      float absForceOffsetLocal_fl32 = 0.0f;
      absTimeMillis_i32 += timeNowMillis - lastCallTimeMillis_i32;
      float absTimeSeconds_fl32 = absTimeMillis_i32 * 0.001f;

      // abs amplitude
      float absAmp_fl32 = 0.0f;
      switch (absForceOrTarvelBit_u8)
      {
        case 0:
          absAmp_fl32 = calcVars_st->absAmplitude_fl32* calcVars_st->forceRange_fl32; 
          break;
        case 1:
          absAmp_fl32 = calcVars_st->stepperPosRange_fl32 * calcVars_st->absAmplitude_fl32; // since absAmplitude_u8 is given in percent, needs to be scaled to from intervall [0%, 100%] to intervall [0, 1]
          break;
        default:
          break;
      }


      float sineArgInDeg_fl32;
      switch (absPattern_u8)
      {
        case 0:
          // sine wave pattern
          sineArgInDeg_fl32 = 360.0f * absFreq_fl32 * absTimeSeconds_fl32;
          absForceOffsetLocal_fl32 =  isin(sineArgInDeg_fl32);
          break;
        case 1:
          // sawtooth pattern
          if (calcVars_st->absFrequency_fl32 > 0.0f)
          {
            absForceOffsetLocal_fl32 = fmodf(absTimeSeconds_fl32, 1.0f / (float)absFreq_fl32) * (float)absFreq_fl32;
            absForceOffsetLocal_fl32 -= 0.5f; // make it symmetrical around 0
          }
          break;
        default:
          break;
      }
      
      switch (absForceOrTarvelBit_u8)
      {
        case 0:
          absOscillationForceOffset_fl32 = absAmp_fl32 * absForceOffsetLocal_fl32;
          absOscillationPositionOffset_fl32 = 0.0f;
          break;
        case 1:
          absOscillationForceOffset_fl32 = 0.0f;
          absOscillationPositionOffset_fl32 = absAmp_fl32 * absForceOffsetLocal_fl32;
          break;
        default:
          break;
      }
      
    }

    lastCallTimeMillis_i32 = timeNowMillis;

    // ActiveSerial->printf("%0.3f, %0.3f\n", absForceOffset, absPosOffset);

    return;
    
  }
};

class RPMOscillation {
private:
  long timeLastTriggerMillis_i32;
  long rpmTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  

public:
  RPMOscillation()
    : timeLastTriggerMillis_i32(0)
  {}
  float rpmValue_fl32 = 0.0f;
  int32_t rpmPositionOffset_i32 = 0;
public:
  void IRAM_ATTR_FLAG trigger()
  {
    timeLastTriggerMillis_i32 = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st)
  {

    long timeNowMillis = millis();
    float timeSinceTrigger_fl32 = (timeNowMillis - timeLastTriggerMillis_i32);
    float rpmForceOffset_fl32 = 0.0f;
    
    if (timeSinceTrigger_fl32 > s_rpmActiveTimePerTriggerMillis_i32)
    {
      rpmTimeMillis_i32 = 0;
      rpmForceOffset_fl32 = s_rpmValueLast_fl32;
    }
    else
    {
      float rpmMaxFreq_fl32 = calcVars_st->rpmMaxFreq_fl32;
      float rpmMinFreq_fl32 = calcVars_st->rpmMinFreq_fl32;
      float rpmAmpBase_fl32 = calcVars_st->rpmAmp_fl32;
      float stepperPosRange_fl32 = calcVars_st->stepperPosRange_fl32;
      float rpmAmp_fl32 = 0.0f; 

      if (rpmValue_fl32 == 0.0f)
      {
        rpmMinFreq_fl32 = 0.0f;
        rpmAmpBase_fl32 = 0.0f;
        rpmForceOffset_fl32 = 0.0f;
      }
      else
      {
          rpmAmp_fl32 = rpmAmpBase_fl32 * (1.0f + 0.3f * rpmValue_fl32 * 0.01f);
        float rpmFreq_fl32 = constrain(rpmValue_fl32*(rpmMaxFreq_fl32-rpmMinFreq_fl32)* 0.01f, rpmMinFreq_fl32, rpmMaxFreq_fl32);
        rpmTimeMillis_i32 += timeNowMillis - lastCallTimeMillis_i32;
        float rpmTimeSeconds_fl32 = rpmTimeMillis_i32 * 0.001f;
        rpmForceOffset_fl32 = stepperPosRange_fl32 * rpmAmp_fl32 * isin( 360.0f * rpmFreq_fl32 * rpmTimeSeconds_fl32 ); 
      }
      
    }

    lastCallTimeMillis_i32 = timeNowMillis;
    s_rpmValueLast_fl32 = rpmForceOffset_fl32;
    if (calcVars_st->forceRange_fl32 > 0.0f)
    {
      rpmPositionOffset_i32 = rpmForceOffset_fl32;
    }
  }
};

class BitePointOscillation {
private:
  long timeLastTriggerMillis_i32;
  long biteTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  

public:
  BitePointOscillation()
    : timeLastTriggerMillis_i32(0)
  {}
  //float RPM_value =0;
  float bitePointOffset_fl32 = 0.0f;
public:
  void IRAM_ATTR_FLAG  trigger()
  {
    timeLastTriggerMillis_i32 = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st)
  {

    long timeNowMillis = millis();
    float timeSinceTrigger_fl32 = (timeNowMillis - timeLastTriggerMillis_i32);
    float bitePointOffsetLocal_fl32 = 0.0f;
    

    if (timeSinceTrigger_fl32 > s_bpActiveTimePerTriggerMillis_i32)
    {
      biteTimeMillis_i32 = 0;
      bitePointOffsetLocal_fl32 = 0.0f;
    }
    else
    {
      float bpFreq_fl32 = calcVars_st->bpFreq_fl32;
      float bpAmp_fl32 = calcVars_st->bpAmp_fl32;
      float stepperPosRange_fl32 = calcVars_st->stepperPosRange_fl32;
      biteTimeMillis_i32 += timeNowMillis - lastCallTimeMillis_i32;
      float bpTimeSeconds_fl32 = biteTimeMillis_i32 * 0.001f;

      //RPMForceOffset = calcVars_st->absAmplitude_fl32 * sin(calcVars_st->absFrequency_fl32 * RPMTimeSeconds);
      bitePointOffsetLocal_fl32 = stepperPosRange_fl32 * bpAmp_fl32 * isin( 360.0f * bpFreq_fl32 * bpTimeSeconds_fl32);
    }

    bitePointOffset_fl32 = bitePointOffsetLocal_fl32;
    lastCallTimeMillis_i32 = timeNowMillis;

  }
};


MovingAverageFilter g_movingAverageFilter_st(100);
// G force effect
class GForceEffect
{
  public:
  float gValue_fl32 = 0.0f;
  float gForceRaw_fl32 = 0.0f;
  float gForce_fl32 = 0.0f;
  float gNormInverse_fl32 = 0.10193679918450560652395514780836f; //1 / 9.81f;
  

  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st, uint8_t gMulti_u8)
  {
    
    if (gValue_fl32 == -128.0f)
    {
      gForceRaw_fl32 = 0.0f;
    }
    else
    {
      float gMultiplier_fl32 = ((float)gMulti_u8) * 0.01f;
      gForceRaw_fl32 = 10.0f * gValue_fl32 * gMultiplier_fl32 * gNormInverse_fl32;
    }

    //apply filter
    gForce_fl32 = g_movingAverageFilter_st.process(gForceRaw_fl32);
    //gForce_fl32 = gForceRaw_fl32;
    
  }
};
//Wheel slip
class WSOscillation {
private:
  long timeLastTriggerMillis_i32;
  long wsTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  

public:
  WSOscillation()
    : timeLastTriggerMillis_i32(0)
  {}
  //float RPM_value =0;
  float wheelSlipOffset_fl32 = 0.0f;
public:
  void IRAM_ATTR_FLAG trigger()
  {
    timeLastTriggerMillis_i32 = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st)
  {


    long timeNowMillis = millis();
    float timeSinceTrigger_fl32 = (timeNowMillis - timeLastTriggerMillis_i32);
    float wsOffset_fl32 = 0.0f;
    
    float wsAmp_fl32 = calcVars_st->wsAmp_fl32;

    if (timeSinceTrigger_fl32 > s_wsActiveTimePerTriggerMillis_i32)
    {
      wsTimeMillis_i32 = 0;
    }
    else
    {
      float stepperPosRange_fl32 = calcVars_st->stepperPosRange_fl32;
      float wsFreq_fl32 = calcVars_st->wsFreq_fl32;
      wsTimeMillis_i32 += timeNowMillis - lastCallTimeMillis_i32;
      float wsTimeSeconds_fl32 = wsTimeMillis_i32 * 0.001f;
      wsOffset_fl32 = stepperPosRange_fl32 * wsAmp_fl32 * isin( 360.0f * wsFreq_fl32 * wsTimeSeconds_fl32 );   
    }

    wheelSlipOffset_fl32 = wsOffset_fl32;
    lastCallTimeMillis_i32 = timeNowMillis;

  }
};
//Road impact
MovingAverageFilter g_movingAverageFilterRoadImpact_st(100);
class RoadImpactEffect
{
  public:
  float roadImpactForce_fl32 = 0;
  float roadImpactForceRaw_fl32 = 0;
  uint8_t roadImpactValue_u8 = 0;

  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st, uint8_t roadImpactMulti_u8)
  {
    float forceRange_fl32;
    float roadMultiplier_fl32 = ((float)roadImpactMulti_u8) * 0.01f;
    forceRange_fl32 = calcVars_st->forceRange_fl32;
    roadImpactForceRaw_fl32 = 0.3f * roadMultiplier_fl32 * forceRange_fl32 * ((float)roadImpactValue_u8) * 0.01f;

    //apply filter
    roadImpactForce_fl32 = g_movingAverageFilterRoadImpact_st.process(roadImpactForceRaw_fl32);

  }
};
//Custom effects
class CustomVibration {
private:
  long timeLastTriggerMillis_i32;
  long cvTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  float cvAmp_fl32 = 0.0f;


public:
  CustomVibration()
    : timeLastTriggerMillis_i32(0)
  {}
  //float RPM_value =0;
  float customVibrationOffset_fl32 = 0.0f;
public:
  void trigger()
  {
    timeLastTriggerMillis_i32 = millis();
  }
  
  void forceOffset(float cvFreq_fl32, float cvAmpInPercent_fl32, float travelRange_fl32)
  {
    long timeNowMillis = millis();
    float timeSinceTrigger_fl32 = (timeNowMillis - timeLastTriggerMillis_i32);
    float customVibrationOffsetLocal_fl32 = 0.0f;
    cvAmp_fl32 = cvAmpInPercent_fl32 * 0.001f * travelRange_fl32;

    if (timeSinceTrigger_fl32 > s_cvActiveTimePerTriggerMillis_i32)
    {
      cvTimeMillis_i32 = 0;
      customVibrationOffsetLocal_fl32 = 0.0f;
    }
    else
    {
      cvTimeMillis_i32 += timeNowMillis - lastCallTimeMillis_i32;
      float cvTimeSeconds_fl32 = cvTimeMillis_i32 * 0.001f;
      customVibrationOffsetLocal_fl32 = cvAmp_fl32 * isin( 360.0f * cvFreq_fl32 * cvTimeSeconds_fl32 );  
    }

    customVibrationOffset_fl32 = customVibrationOffsetLocal_fl32;
    lastCallTimeMillis_i32 = timeNowMillis;
  }
};
