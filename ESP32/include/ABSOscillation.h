#pragma once

#include "DiyActivePedal_types.h"
#include <MovingAverageFilter.h>
#include "FastTrig.h"
#include "esp_timer.h"

static const long s_absActiveTimePerTriggerMillis_i32 = 100;
static const long s_rpmActiveTimePerTriggerMillis_i32 = 100;
static const long s_bpActiveTimePerTriggerMillis_i32 = 100;
static const long s_wsActiveTimePerTriggerMillis_i32 = 100;
static const long s_cvActiveTimePerTriggerMillis_i32 = 100;

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
// ABS fade times: a short fade-in keeps the onset crisp without a force step,
// a longer fade-out ends the vibration smoothly instead of cutting it mid-cycle.
static const float s_absFadeInTime_s_fl32 = 0.005f;
static const float s_absFadeOutTime_s_fl32 = 0.015f;

// absForceOrTarvelBit_u8 bits: bit 0 selects force (0) or travel (1) modulation,
// bit 1 scales the amplitude with the current pedal force (like the brake pressure),
// bits 2-7 hold the amplitude at zero force for that scaling, in 2 % steps of the max
// amplitude (0..50 = 0..100 %).
// Older plugins only send 0 or 1, so their behaviour is unchanged.
#define ABS_MODULATE_TRAVEL_BIT_U8 0x01
#define ABS_SCALE_WITH_FORCE_BIT_U8 0x02
#define ABS_MIN_AMPLITUDE_SHIFT_U8 2
#define ABS_MIN_AMPLITUDE_MASK_U8 0x3F
#define ABS_MIN_AMPLITUDE_STEPS_U8 50
// Low-pass of the force used for the scaling, slower than the ABS vibration so the
// vibration itself does not modulate its own amplitude.
static const float s_absForceScaleTau_s_fl32 = 0.1f;
// Pulse waveform: share of the period used by the fast kick back
static const float s_absPulseRiseShare_01_fl32 = 0.12f;

class ABSOscillation {
private:
  volatile long timeLastTriggerMillis_i32;  // written Core0 (trigger), read Core1 (forceOffset)
  int64_t lastCallTimeUs_i64 = 0;
  // Phase in periods [0, 1). Integrated from the frequency, so a frequency change
  // (track condition) or a retrigger never makes the waveform jump.
  float phase_01_fl32 = 0.0f;
  // Fade envelope [0, 1] applied to the waveform
  float envelope_01_fl32 = 0.0f;
  // low-pass filtered force scale [min amplitude ratio, 1]
  float forceScale_01_fl32 = 1.0f;

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
  
  // Forced inline into the caller (the pedal task) instead of IRAM_ATTR_FLAG: a separate IRAM
  // copy of this header-defined function fails to link on the ESP32-S3
  // ("dangerous relocation: l32r: literal placed after use").
  inline __attribute__((always_inline)) void forceOffset(DapCalculationVariables_t* calcVars_st, uint8_t absPattern_u8, uint8_t absForceOrTarvelBit_u8)
  {
    // Microsecond timing: the pedal loop runs every ~600 us, so a millis() based
    // waveform was a 1 ms staircase, sampled unevenly (1 or 2 loops per step),
    // which added a buzz on top of the ABS vibration.
    int64_t timeNowUs_i64 = esp_timer_get_time();
    float dt_s_fl32 = (lastCallTimeUs_i64 > 0) ? (float)(timeNowUs_i64 - lastCallTimeUs_i64) * 1e-6f : 0.0f;
    dt_s_fl32 = constrain(dt_s_fl32, 0.0f, 0.01f);
    lastCallTimeUs_i64 = timeNowUs_i64;

    long timeSinceTriggerMillis_i32 = millis() - timeLastTriggerMillis_i32;
    bool absActive_b = (timeSinceTriggerMillis_i32 <= s_absActiveTimePerTriggerMillis_i32);

    // fade envelope
    if (absActive_b)
    {
      envelope_01_fl32 += dt_s_fl32 / s_absFadeInTime_s_fl32;
    }
    else
    {
      envelope_01_fl32 -= dt_s_fl32 / s_absFadeOutTime_s_fl32;
    }
    envelope_01_fl32 = constrain(envelope_01_fl32, 0.0f, 1.0f);

    if (envelope_01_fl32 <= 0.0f)
    {
      // idle: restart the next ABS event at the beginning of a period
      phase_01_fl32 = 0.0f;
      absOscillationForceOffset_fl32 = 0.0f;
      absOscillationPositionOffset_fl32 = 0.0f;
      return;
    }

    // frequency depends on the road condition
    float absFreq_fl32 = calcVars_st->absFrequency_fl32;
    absFreq_fl32 = absFreq_fl32 * (1.0f + ((float)calcVars_st->trackCondition_u8) * 0.1f);
    absFreq_fl32 = constrain(absFreq_fl32, 0.0f, 50.0f);

    // phase integration (keeps running during the fade-out)
    phase_01_fl32 += absFreq_fl32 * dt_s_fl32;
    phase_01_fl32 -= floorf(phase_01_fl32);

    // abs amplitude, given in [0, 1] of the force / travel range
    float absAmp_fl32 = calcVars_st->absAmplitude_fl32 * envelope_01_fl32;

    // optional: amplitude following the pedal force (brake pressure), linear from the min
    // amplitude at zero force to the max amplitude (the amplitude setting) at max force
    if (absForceOrTarvelBit_u8 & ABS_SCALE_WITH_FORCE_BIT_U8)
    {
      uint8_t minSteps_u8 = (absForceOrTarvelBit_u8 >> ABS_MIN_AMPLITUDE_SHIFT_U8) & ABS_MIN_AMPLITUDE_MASK_U8;
      float minRatio_01_fl32 = (float)min(minSteps_u8, (uint8_t)ABS_MIN_AMPLITUDE_STEPS_U8) / (float)ABS_MIN_AMPLITUDE_STEPS_U8;
      float forceRatio_01_fl32 = 1.0f;
      if (calcVars_st->forceMax_fl32 > 0.1f)
      {
        forceRatio_01_fl32 = constrain(calcVars_st->currentForceReading_fl32 / calcVars_st->forceMax_fl32, 0.0f, 1.0f);
      }
      float forceScaleTarget_01_fl32 = minRatio_01_fl32 + (1.0f - minRatio_01_fl32) * forceRatio_01_fl32;
      float alpha_fl32 = dt_s_fl32 / (s_absForceScaleTau_s_fl32 + dt_s_fl32);
      forceScale_01_fl32 += alpha_fl32 * (forceScaleTarget_01_fl32 - forceScale_01_fl32);
      absAmp_fl32 *= forceScale_01_fl32;
    }
    float absAmplForce_fl32 = absAmp_fl32 * calcVars_st->forceRange_fl32;
    float absAmplPosition_fl32 = absAmp_fl32 * calcVars_st->stepperPosRange_fl32;

    // waveform in [-1, 1] (sine), [-0.5, 0.5] (sawtooth) or [-1, 0] (pulse)
    float waveform_fl32 = 0.0f;
    switch (absPattern_u8)
    {
      case 0:
        // sine wave pattern
        waveform_fl32 = isin(360.0f * phase_01_fl32);
        break;
      case 1:
        // sawtooth pattern, symmetrical around 0
        if (absFreq_fl32 > 0.0f)
        {
          waveform_fl32 = phase_01_fl32 - 0.5f;
        }
        break;
      case 2:
        // pulse pattern, like an ABS pump: a fast kick back against the foot, then a
        // smooth return. Continuous at the period boundary (starts and ends at 0 with
        // zero slope), so only the kick itself is sharp.
        if (phase_01_fl32 < s_absPulseRiseShare_01_fl32)
        {
          waveform_fl32 = -isin(90.0f * phase_01_fl32 / s_absPulseRiseShare_01_fl32);
        }
        else
        {
          float decay_01_fl32 = 1.0f - (phase_01_fl32 - s_absPulseRiseShare_01_fl32) / (1.0f - s_absPulseRiseShare_01_fl32);
          waveform_fl32 = -decay_01_fl32 * decay_01_fl32;
        }
        break;
      default:
        break;
    }

    // apply the oscillation to either force or position based on the configuration bit
    switch (absForceOrTarvelBit_u8 & ABS_MODULATE_TRAVEL_BIT_U8)
    {
      case 0:
        absOscillationForceOffset_fl32 = absAmplForce_fl32 * waveform_fl32;
        absOscillationPositionOffset_fl32 = 0.0f;
        break;
      case 1:
        absOscillationForceOffset_fl32 = 0.0f;
        absOscillationPositionOffset_fl32 = absAmplPosition_fl32 * waveform_fl32;
        break;
      default:
        absOscillationForceOffset_fl32 = 0.0f;
        absOscillationPositionOffset_fl32 = 0.0f;
        break;
    }
  }
};

class RpmOscillation_t {
private:
  uint32_t timeLastTriggerMillis_u32;
  uint32_t lastCallTimeMillis_u32;
  float phase_fl32;
  float lastRpmForceOffset_fl32;

public:
  RpmOscillation_t()
    : timeLastTriggerMillis_u32(0),
      lastCallTimeMillis_u32(0),
      phase_fl32(0.0f),
      lastRpmForceOffset_fl32(0.0f)
  {}
  
  volatile float rpmValue_fl32 = 0.0f;  // written Core0 (serialCommunicationTaskRx), read Core1 (forceOffset)
  int32_t rpmPositionOffset_i32 = 0;

public:
  void IRAM_ATTR_FLAG trigger()
  {
    timeLastTriggerMillis_u32 = millis();
  }
  
  void IRAM_ATTR_FLAG forceOffset(DapCalculationVariables_t* calcVars_st)
  {
    uint32_t timeNowMillis_u32 = millis();
    uint32_t timeSinceTrigger_u32 = timeNowMillis_u32 - timeLastTriggerMillis_u32;
    float rpmForceOffset_fl32 = 0.0f;
    
    if (timeSinceTrigger_u32 > (uint32_t)s_rpmActiveTimePerTriggerMillis_i32)
    {
      phase_fl32 = 0.0f;
      rpmForceOffset_fl32 = lastRpmForceOffset_fl32;
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
        phase_fl32 = 0.0f;
      }
      else
      {
        rpmAmp_fl32 = rpmAmpBase_fl32 * (1.0f + 0.3f * rpmValue_fl32 * 0.01f);
        float rpmFreq_fl32 = constrain(rpmMinFreq_fl32 + rpmValue_fl32 * 0.01f * (rpmMaxFreq_fl32 - rpmMinFreq_fl32), rpmMinFreq_fl32, rpmMaxFreq_fl32);
        
        uint32_t dt_u32 = 0;
        if (lastCallTimeMillis_u32 != 0) {
          dt_u32 = timeNowMillis_u32 - lastCallTimeMillis_u32;
        }
        float dtSeconds_fl32 = dt_u32 * 0.001f;
        
        phase_fl32 += 360.0f * rpmFreq_fl32 * dtSeconds_fl32;
        while (phase_fl32 >= 360.0f) {
          phase_fl32 -= 360.0f;
        }
        
        rpmForceOffset_fl32 = stepperPosRange_fl32 * rpmAmp_fl32 * isin(phase_fl32); 
      }
    }

    lastCallTimeMillis_u32 = timeNowMillis_u32;
    lastRpmForceOffset_fl32 = rpmForceOffset_fl32;
    
    if (calcVars_st->forceRange_fl32 > 0.0f)
    {
      rpmPositionOffset_i32 = rpmForceOffset_fl32;
    }
  }
};

class BitePointOscillation_t {
private:
  long timeLastTriggerMillis_i32;
  long biteTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  

public:
  BitePointOscillation_t()
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
class GForceEffect_t
{
  public:
  volatile float gValue_fl32 = 0.0f;  // written Core0 (serialCommunicationTaskRx), read Core1 (forceOffset)
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
class WsOscillation_t {
private:
  volatile long timeLastTriggerMillis_i32;  // written Core0 (trigger), read Core1 (forceOffset)
  long wsTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  

public:
  WsOscillation_t()
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
class RoadImpactEffect_t
{
  public:
  float roadImpactForce_fl32 = 0;
  float roadImpactForceRaw_fl32 = 0;
  volatile uint8_t roadImpactValue_u8 = 0;  // written Core0 (serialCommunicationTaskRx), read Core1 (forceOffset)

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
class CustomVibration_t {
private:
  volatile long timeLastTriggerMillis_i32;  // written Core0 (trigger), read Core1 (forceOffset)
  long cvTimeMillis_i32;
  long lastCallTimeMillis_i32 = 0;
  float cvAmp_fl32 = 0.0f;


public:
  CustomVibration_t()
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
