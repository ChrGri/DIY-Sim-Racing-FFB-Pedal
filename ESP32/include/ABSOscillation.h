#pragma once

#include "DiyActivePedal_types.h"
#include <MovingAverageFilter.h>
#include "FastTrig.h"

static const long ABS_ACTIVE_TIME_PER_TRIGGER_MILLIS = 100;
static const long RPM_ACTIVE_TIME_PER_TRIGGER_MILLIS = 100;
static const long BP_ACTIVE_TIME_PER_TRIGGER_MILLIS = 100;
static const long WS_ACTIVE_TIME_PER_TRIGGER_MILLIS = 100;
static const long CV_ACTIVE_TIME_PER_TRIGGER_MILLIS = 100;
static float RPM_VALUE_LAST = 0.0f;

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
  long _timeLastTriggerMillis;
  long _absTimeMillis;
  long _lastCallTimeMillis = 0;

public:
  float absOscillation_Force_offset = 0.0f;
  float absOscillation_Position_offset = 0.0f;
  ABSOscillation()
    : _timeLastTriggerMillis(0)
  {}

public:
  void IRAM_ATTR_FLAG trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t absPattern, uint8_t absForceOrTarvelBit) {

    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    
    

    if (timeSinceTrigger > ABS_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _absTimeMillis = 0;
      absOscillation_Force_offset = 0.0f;
      absOscillation_Position_offset = 0.0f;
    }
    else
    {
      //frequency depend on road condition
      float absFreq = calcVars_st->absFrequency;
      absFreq = absFreq*(1.0f + ((float)calcVars_st->TrackCondition) * 0.1f );
      absFreq = constrain(absFreq, 0.0f, 50.0f);
      
      float absForceOffset_local = 0.0f;
      _absTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float absTimeSeconds = _absTimeMillis * 0.001f;

      // abs amplitude
      float absAmp_fl32 = 0.0f;
      switch (absForceOrTarvelBit) {
        case 0:
          absAmp_fl32 = calcVars_st->absAmplitude* calcVars_st->Force_Range; 
          break;
        case 1:
          absAmp_fl32 = calcVars_st->stepperPosRange * calcVars_st->absAmplitude; // since absAmplitude is given in percent, needs to be scaled to from intervall [0%, 100%] to intervall [0, 1]
          break;
        default:
          break;
      }


      float sineArgInDeg_fl32;
      switch (absPattern) {
        case 0:
          // sine wave pattern
          sineArgInDeg_fl32 = 360.0f * absFreq * absTimeSeconds;
          absForceOffset_local =  isin(sineArgInDeg_fl32);
          break;
        case 1:
          // sawtooth pattern
          if (calcVars_st->absFrequency > 0.0f)
          {
            absForceOffset_local = fmodf(absTimeSeconds, 1.0f / (float)absFreq) * (float)absFreq;
            absForceOffset_local -= 0.5f; // make it symmetrical around 0
          }
          break;
        default:
          break;
      }
      
      switch (absForceOrTarvelBit) {
        case 0:
          absOscillation_Force_offset = absAmp_fl32 * absForceOffset_local;
          absOscillation_Position_offset = 0.0f;
          break;
        case 1:
          absOscillation_Force_offset = 0.0f;
          absOscillation_Position_offset = absAmp_fl32 * absForceOffset_local;
          break;
        default:
          break;
      }
      
    }

    _lastCallTimeMillis = timeNowMillis;

    // ActiveSerial->printf("%0.3f, %0.3f\n", absForceOffset, absPosOffset);

    return;
    
  }
};

class RPMOscillation {
private:
  long _timeLastTriggerMillis;
  long _RPMTimeMillis;
  long _lastCallTimeMillis = 0;
  

public:
  RPMOscillation()
    : _timeLastTriggerMillis(0)
  {}
  float RPM_value = 0.0f;
  int32_t RPM_position_offset = 0;
public:
  void IRAM_ATTR_FLAG trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st) {

    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float RPMForceOffset = 0.0f;
    
    if (timeSinceTrigger > RPM_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _RPMTimeMillis = 0;
      RPMForceOffset = RPM_VALUE_LAST;
    }
    else
    {
      float RPM_max_freq = calcVars_st->RPM_max_freq;
      float RPM_min_freq = calcVars_st->RPM_min_freq;
      float RPM_amp_base = calcVars_st->RPM_AMP;
      float stepperPosRange = calcVars_st->stepperPosRange;
      float RPM_amp = 0.0f; 

      if(RPM_value == 0.0f)
      {
        RPM_min_freq = 0.0f;
        RPM_amp_base = 0.0f;
        RPMForceOffset = 0.0f;
      }
      else
      {
          RPM_amp = RPM_amp_base * (1.0f + 0.3f * RPM_value * 0.01f);
        float RPM_freq = constrain(RPM_value*(RPM_max_freq-RPM_min_freq)* 0.01f, RPM_min_freq, RPM_max_freq);
        _RPMTimeMillis += timeNowMillis - _lastCallTimeMillis;
        float RPMTimeSeconds = _RPMTimeMillis * 0.001f;
        RPMForceOffset = stepperPosRange* RPM_amp * isin( 360.0f * RPM_freq* RPMTimeSeconds ); 
      }
      
    }

    _lastCallTimeMillis = timeNowMillis;
    RPM_VALUE_LAST = RPMForceOffset;
    if (calcVars_st->Force_Range > 0.0f)
    {
        RPM_position_offset = RPMForceOffset;
    }
  }
};

class BitePointOscillation {
private:
  long _timeLastTriggerMillis;
  long _BiteTimeMillis;
  long _lastCallTimeMillis = 0;
  

public:
  BitePointOscillation()
    : _timeLastTriggerMillis(0)
  {}
  //float RPM_value =0;
  float BitePointOffset = 0.0f;
public:
  void IRAM_ATTR_FLAG  trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st) {

    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float bitePointOffset = 0.0f;
    

    if (timeSinceTrigger > BP_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _BiteTimeMillis = 0;
      bitePointOffset = 0.0f;
    }
    else
    {
      float BP_freq = calcVars_st->BP_freq;
      float BP_amp = calcVars_st->BP_amp;
      float stepperPosRange= calcVars_st->stepperPosRange;
      _BiteTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float BPTimeSeconds = _BiteTimeMillis * 0.001f;

      //RPMForceOffset = calcVars_st->absAmplitude * sin(calcVars_st->absFrequency * RPMTimeSeconds);
      bitePointOffset = stepperPosRange*BP_amp * isin( 360.0f * BP_freq * BPTimeSeconds);
    }

    BitePointOffset = bitePointOffset;
    _lastCallTimeMillis = timeNowMillis;

  }
};


MovingAverageFilter movingAverageFilter(100);
// G force effect
class G_force_effect
{
  public:
  float G_value = 0;
  float G_force_raw = 0;
  float G_force = 0;
  float g_norm_inverse = 0.10193679918450560652395514780836f; //1 / 9.81f;
  

  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t G_multi)
  {
    
    if(G_value == -128.0f)
    {
      G_force_raw = 0.0f;
    }
    else
    {
      float G_multiplier = ((float)G_multi) * 0.01f;
      G_force_raw = 10.0f*(G_value)*G_multiplier * g_norm_inverse;
    }

    //apply filter
    G_force = movingAverageFilter.process(G_force_raw);
    //G_force=G_force_raw;
    
  }
};
//Wheel slip
class WSOscillation {
private:
  long _timeLastTriggerMillis;
  long _WSTimeMillis;
  long _lastCallTimeMillis = 0;
  

public:
  WSOscillation()
    : _timeLastTriggerMillis(0)
  {}
  //float RPM_value =0;
  float WheelSlipOffset = 0.0f;
public:
  void IRAM_ATTR_FLAG trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st) {


    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float wsOffset = 0.0f;
    
    float WS_amp = calcVars_st->WS_amp;

    if (timeSinceTrigger > WS_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _WSTimeMillis = 0;
    }
    else
    {
      float stepperPosRange= calcVars_st->stepperPosRange;
      float WS_freq = calcVars_st->WS_freq;
      _WSTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float WSTimeSeconds = _WSTimeMillis * 0.001f;
      wsOffset = stepperPosRange* WS_amp * isin( 360.0f * WS_freq* WSTimeSeconds );   
    }

    WheelSlipOffset = wsOffset;
    _lastCallTimeMillis = timeNowMillis;

  }
};
//Road impact
MovingAverageFilter movingAverageFilter_roadimpact(100);
class Road_impact_effect
{
  public:
  float Road_Impact_force=0;
  float Road_Impact_force_raw=0;
  uint8_t Road_Impact_value=0;

  void IRAM_ATTR_FLAG forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t Road_impact_multi)
  {
    uint32_t Force_Range;
    float Road_multiplier = ((float)Road_impact_multi)* 0.01f;
    Force_Range = calcVars_st->Force_Range;
    Road_Impact_force_raw = 0.3f*Road_multiplier*((float)Force_Range)*((float)Road_Impact_value)* 0.01f;

    //apply filter
    Road_Impact_force = movingAverageFilter_roadimpact.process(Road_Impact_force_raw);

  }
};
//Custom effects
class Custom_vibration {
private:
  long _timeLastTriggerMillis;
  long _CVTimeMillis;
  long _lastCallTimeMillis = 0;
  float CV_amp = 0.0f;


public:
  Custom_vibration()
    : _timeLastTriggerMillis(0)
  {}
  //float RPM_value =0;
  float CustomVibrationOffset = 0.0f;
public:
  void IRAM_ATTR_FLAG trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(float CV_freq, float CV_amp_in_percent, float travelRange) 
  {
    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float customVibrationOffset = 0.0f;
    CV_amp = CV_amp_in_percent * 0.001f *travelRange;

    if (timeSinceTrigger > CV_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _CVTimeMillis = 0;
      customVibrationOffset = 0.0f;
    }
    else
    {
      _CVTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float CVTimeSeconds = _CVTimeMillis * 0.001f;
      customVibrationOffset = CV_amp * isin( 360.0f * CV_freq* CVTimeSeconds );  
    }

    CustomVibrationOffset = customVibrationOffset;
    _lastCallTimeMillis = timeNowMillis;
  }
};
