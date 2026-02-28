#pragma once

#include "DiyActivePedal_types.h"
#include "MovingAverageFilter.h"
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
  ABSOscillation()
    : _timeLastTriggerMillis(0)
  {}

public:
  void trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t absPattern, uint8_t absForceOrTarvelBit, float * absForceOffset, float * absPosOffset) {

    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    
    

    if (timeSinceTrigger > ABS_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _absTimeMillis = 0;
      *absForceOffset = 0.0f;
      *absPosOffset = 0.0f;
    }
    else
    {
      //frequency depend on road condition
      float absFreq = calcVars_st->absFrequency;
      absFreq = absFreq*(1.0f + ((float)calcVars_st->trackCondition_u8) * 0.1f );
      absFreq = constrain(absFreq, 0, 50.0f);
      
      float absForceOffset_local = 0.0f;
      _absTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float absTimeSeconds = _absTimeMillis * 0.001f;

      // abs amplitude
      float absAmp_fl32 = 0.0f;
      switch (absForceOrTarvelBit) {
        case 0:
          absAmp_fl32 = calcVars_st->absAmplitude; 
          break;
        case 1:
          absAmp_fl32 = calcVars_st->stepperPosRange * calcVars_st->absAmplitude * 0.01f;
          break;
        default:
          break;
      }


      
      switch (absPattern) {
        case 0:
          // sine wave pattern
          absForceOffset_local =  absAmp_fl32 * isin(2.0f * PI * absFreq * absTimeSeconds * RAD_TO_DEG);
          break;
        case 1:
          // sawtooth pattern
          if (calcVars_st->absFrequency > 0.0f)
          {
            absForceOffset_local = absAmp_fl32 * fmod(absTimeSeconds, 1.0f / (float)absFreq) * (float)absFreq;
            absForceOffset_local -= absAmp_fl32 * 0.5f; // make it symmetrical around 0
          }
          break;
        default:
          break;
      }
      
      switch (absForceOrTarvelBit) {
        case 0:
          *absForceOffset = absForceOffset_local;
          *absPosOffset = 0.0f;
          break;
        case 1:
          *absForceOffset = 0.0f;
          *absPosOffset = absForceOffset_local;
          break;
        default:
          break;
      }
      
    }

    _lastCallTimeMillis = timeNowMillis;

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
  void trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(DAP_calculationVariables_st* calcVars_st) {

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
      float RPM_max_freq = calcVars_st->rpmMaxFreq_f;
      float RPM_min_freq = calcVars_st->rpmMinFreq_f;
      if(RPM_value == 0.0f)
      {
        RPM_min_freq = 0.0f;
      }

      float RPM_amp_base = calcVars_st->rpmAmp_f;
      float RPM_amp = 0.0f; 

      RPM_amp = RPM_amp_base*(1.0f+ 0.3f * RPM_value * 0.01f);
      float RPM_freq = constrain(RPM_value*(RPM_max_freq-RPM_min_freq)* 0.01f, RPM_min_freq, RPM_max_freq);
      _RPMTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float RPMTimeSeconds = _RPMTimeMillis * 0.001f;
      RPMForceOffset = RPM_amp * isin( 2.0f*PI* RPM_freq* RPMTimeSeconds * RAD_TO_DEG); 
    }

    _lastCallTimeMillis = timeNowMillis;
    RPM_VALUE_LAST = RPMForceOffset;
    if (calcVars_st->forceRange_f > 0.0f)
    {
        RPM_position_offset = calcVars_st->stepperPosRange* ( RPMForceOffset / calcVars_st->forceRange_f );
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
  float BitePoint_Force_offset = 0;
public:
  void trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(DAP_calculationVariables_st* calcVars_st) {

    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float BitePointForceOffset = 0.0f;
    

    if (timeSinceTrigger > BP_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _BiteTimeMillis = 0;
      BitePointForceOffset = 0.0f;
    }
    else
    {
      float BP_freq = calcVars_st->bpFreq_f;
      float BP_amp = calcVars_st->bpAmp_f;

      _BiteTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float BPTimeSeconds = _BiteTimeMillis * 0.001f;

      //RPMForceOffset = calcVars_st->absAmplitude * sin(calcVars_st->absFrequency * RPMTimeSeconds);
      BitePointForceOffset = BP_amp * isin( 2.0f*PI* BP_freq* BPTimeSeconds * RAD_TO_DEG);
    }

    BitePoint_Force_offset = BitePointForceOffset;
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
  

  void forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t G_multi)
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
  float WS_Force_offset = 0.0f;
public:
  void trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(DAP_calculationVariables_st* calcVars_st) {


    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float WSForceOffset = 0.0f;
    
    float WS_amp = calcVars_st->wsAmp_f;

    if (timeSinceTrigger > WS_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _WSTimeMillis = 0;
    }
    else
    {
      float WS_freq = calcVars_st->wsFreq_f;
      _WSTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float WSTimeSeconds = _WSTimeMillis * 0.001f;
      WSForceOffset = WS_amp * isin( 2.0f*PI* WS_freq* WSTimeSeconds * RAD_TO_DEG);   
    }

    WS_Force_offset = WSForceOffset;
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

  void forceOffset(DAP_calculationVariables_st* calcVars_st, uint8_t Road_impact_multi)
  {
    uint32_t forceRange_u32;
    float Road_multiplier = ((float)Road_impact_multi)* 0.01f;
    forceRange_u32 = calcVars_st->forceRange_f;
    Road_Impact_force_raw = 0.3f*Road_multiplier*((float)forceRange_u32)*((float)Road_Impact_value)* 0.01f;

    //apply filter
    Road_Impact_force = movingAverageFilter_roadimpact.process(Road_Impact_force_raw);

  }
};
//Wheel slip
class Custom_vibration {
private:
  long _timeLastTriggerMillis;
  long _CVTimeMillis;
  long _lastCallTimeMillis = 0;
  

public:
  Custom_vibration()
    : _timeLastTriggerMillis(0)
  {}
  //float RPM_value =0;
  float CV_Force_offset = 0.0f;
public:
  void trigger() {
    _timeLastTriggerMillis = millis();
  }
  
  void forceOffset(float CV_freq, float CV_amp) {


    long timeNowMillis = millis();
    float timeSinceTrigger = (timeNowMillis - _timeLastTriggerMillis);
    float CVForceOffset = 0.0f;


    if (timeSinceTrigger > CV_ACTIVE_TIME_PER_TRIGGER_MILLIS)
    {
      _CVTimeMillis = 0;
      CVForceOffset = 0.0f;
    }
    else
    {
      _CVTimeMillis += timeNowMillis - _lastCallTimeMillis;
      float CVTimeSeconds = _CVTimeMillis * 0.001f;
      CVForceOffset = 0.05f * CV_amp * isin( 2.0f*PI* CV_freq* CVTimeSeconds * RAD_TO_DEG);  
    }

    CV_Force_offset = CVForceOffset;
    _lastCallTimeMillis = timeNowMillis;
  }
};
