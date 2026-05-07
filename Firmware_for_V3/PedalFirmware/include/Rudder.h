#pragma once
#define Rudder_timeout 1500
#include "DiyActivePedal_types.h"
#include "MovingAverageFilter.h"
#include "SignalFilter.h"
MovingAverageFilter averagefilter_rudder(200);
MovingAverageFilter averagefilter_rudder_force(50);
class Rudder{
  public:
  int32_t Center_offset;
  int32_t offset_raw;
  int32_t offset_filter;
  int32_t stepper_range;
  int32_t dead_zone_upper;
  int32_t dead_zone_lower;
  int32_t dead_zone;
  int32_t syncPedalPosition_i32;
  int32_t currentPedalPosition_i32;
  float endpos_travel;
  float forceRange_f;
  float force_offset_raw;
  float force_offset_filter;
  float force_center_offset;
  float position_ratio_sync;
  float position_ratio_current;
  int debug_count=0;
  KalmanFilter* kalman_rudder = NULL;
  //bool IsReady = false;
  Rudder()
  {
    kalman_rudder=new KalmanFilter(3.0f);
  }
  void offset_calculate(DAP_calculationVariables_st* calcVars_st)
  {
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    position_ratio_sync=calcVars_st->syncPedalPositionRatio_f;
    endpos_travel=(float)calcVars_st->stepperPosRange;
    position_ratio_current=((float)(currentPedalPosition_i32-calcVars_st->stepperPosMin))/endpos_travel;    
    dead_zone=20;
    Center_offset=calcVars_st->stepperPosMin+ calcVars_st->stepperPosRange/2.0f;
    float center_deadzone = 0.51f;
    if(calcVars_st->rudderStatus_b)
    {
      if(position_ratio_sync>center_deadzone)
      {
        offset_raw=(int32_t)(-1*(position_ratio_sync-0.50f)*endpos_travel);
          
      }
      else
      {
        offset_raw=0;
      }
      if(calcVars_st->rudderBrakeStatus_b)
      {
        offset_raw=0;
      }
      offset_filter=(int32_t)kalman_rudder->filteredValue(offset_raw+Center_offset,0.0f,1);
      //offset_filter=averagefilter_rudder.process(offset_raw+Center_offset);
      //cap offset filter to prevent over the endstop value
      offset_filter=constrain(offset_filter,calcVars_st->stepperPosMinDefault,calcVars_st->stepperPosMaxDefault);
    }
    else
    {
      offset_filter=calcVars_st->stepperPosMin;
    }

  }
  void force_offset_calculate(DAP_calculationVariables_st* calcVars_st)
  {
    dead_zone=20;
    Center_offset=calcVars_st->stepperPosRange/2.0f;
    dead_zone_upper=Center_offset+dead_zone/2.0f;
    dead_zone_lower=Center_offset-dead_zone/2.0f;
    syncPedalPosition_i32=calcVars_st->syncPedalPosition_u32;
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    stepper_range=calcVars_st->stepperPosRange;
    forceRange_f = calcVars_st->forceRange_f;
    force_center_offset = forceRange_f / 2 + calcVars_st->forceMin_f;
    endpos_travel=(float)calcVars_st->stepperPosRange;
    //endpos_travel=((float)(calcVars_st->currentPedalPosition_u32-calcVars_st->stepperPosMin))/((float)calcVars_st->stepperPosRange);
    position_ratio_sync=calcVars_st->syncPedalPositionRatio_f;
    position_ratio_current=((float)(currentPedalPosition_i32-calcVars_st->stepperPosMin))/endpos_travel;
    

    float center_deadzone = 0.51f;
    if(calcVars_st->rudderStatus_b)
    {
      
        
        if(position_ratio_sync>center_deadzone)
        {
          force_offset_raw = (float)(-1.0f * (position_ratio_sync - 0.50f) * forceRange_f);
          
        }
        else
        {
          force_offset_raw=0.0f;
        }
        if(calcVars_st->rudderBrakeStatus_b)
        {
          force_offset_raw=0.0f;
        }
     
      force_offset_filter=averagefilter_rudder_force.process(force_offset_raw+force_center_offset);
    }
    else
    {
      force_offset_filter=0;
    }
  }
};
//Rudder impact
MovingAverageFilter Averagefilter_Rudder_G_Offset(50);
class Rudder_G_Force{
  public:
  int32_t offset_raw;
  int32_t offset_filter;
  int32_t stepper_range;
  uint8_t G_value;
  long stepperPosMax;
  void offset_calculate(DAP_calculationVariables_st* calcVars_st)
  {
    stepperPosMax=(float)calcVars_st->stepperPosMax;
    stepper_range=(float)calcVars_st->stepperPosRange;
    float Amp_max=0.3*stepper_range;
    if(calcVars_st->rudderStatus_b)
    {
      float offset= Amp_max*((float)G_value)/100.0f;
      //offset=constrain(offset,0,Amp_max);
      offset_filter=Averagefilter_Rudder_G_Offset.process((stepperPosMax-offset));
    }
    else
    {
      offset_filter=calcVars_st->stepperPosMax;
    }

  }
};
MovingAverageFilter averagefilter_helirudder(200);
class helicoptersRudder{
  public:
  int32_t Center_offset;
  int32_t offset_raw;
  int32_t offset_filter;
  int32_t stepper_range;
  int32_t dead_zone_upper;
  int32_t dead_zone_lower;
  int32_t dead_zone;
  int32_t syncPedalPosition_i32;
  int32_t currentPedalPosition_i32;
  float endpos_travel;
  float forceRange_f;
  float force_offset_raw;
  float force_offset_filter;
  float force_center_offset;
  float position_ratio_sync;
  float position_ratio_current;
  float currentForceReading;
  float pedalPreload;
  float offsetLast;
  int debug_count=0;
  float deadzoneTolerance = 0.01f;
  float position_ratio_last;
  unsigned long debugPrintLast=0;
  KalmanFilter* kalman_rudder = NULL;
  //bool IsReady = false;
  helicoptersRudder()
  {
    kalman_rudder=new KalmanFilter(3.0f);
  }
  void offset_calculate(DAP_calculationVariables_st* calcVars_st)
  {
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    position_ratio_sync=calcVars_st->syncPedalPositionRatio_f;
    endpos_travel=(float)calcVars_st->stepperPosRange;
    currentForceReading=(float)calcVars_st->currentForceReading;
    pedalPreload=(float)calcVars_st->forceMin_f;
    position_ratio_current=((float)(currentPedalPosition_i32-calcVars_st->stepperPosMin))/endpos_travel;    
    dead_zone=20;
    Center_offset=calcVars_st->stepperPosMin+ calcVars_st->stepperPosRange/2.0f;
    float center_deadzone = 0.51f;
    if(calcVars_st->helicopterRudderStatus_b)
    {
      //no press status
      if(currentForceReading<pedalPreload)
      {
        if(calcVars_st->isHelicopterRudderInitialized_b)
        {
          if(abs((1-position_ratio_sync)-position_ratio_last)>deadzoneTolerance)
          {
            offset_raw=(int32_t)(-1*(position_ratio_sync-0.50f)*endpos_travel);
            offsetLast=offset_raw;
            position_ratio_last=constrain((1-position_ratio_sync),0,1);
          }
          else
          {
            offset_raw=offsetLast;
          }  
        }
        else
        {
          offset_raw=0;
        }

      }
      else
      {
        if((position_ratio_current-position_ratio_last)>deadzoneTolerance)
        {
          position_ratio_last=position_ratio_current;
          offset_raw=0;
          offsetLast=(int32_t)((position_ratio_current-0.5f)*endpos_travel);
        }
        else
        {
          offset_raw=(int32_t)((position_ratio_last-0.5f)*endpos_travel);
          offsetLast=offset_raw;
        }


      }
      //offset_filter=(int32_t)kalman_rudder->filteredValue(offset_raw+Center_offset,0.0f,1);
      offset_filter=averagefilter_helirudder.process(offset_raw+Center_offset);
      //cap offset filter to prevent over the endstop value
      offset_filter=constrain(offset_filter,calcVars_st->stepperPosMinDefault,calcVars_st->stepperPosMaxDefault);
    }
    else
    {
      offset_filter=calcVars_st->stepperPosMin;
    }
    /*
    if(debugPrintLast-millis()>200)
    {
      Serial.print("Current Force=");
      Serial.println(currentForceReading);
      Serial.print("offset_filter=");
      Serial.println(offset_filter);
      Serial.print("offset_Last=");
      Serial.println(offsetLast);
      Serial.print("position_ratio_Last=");
      Serial.println(position_ratio_last);
      Serial.print("position_ratio_sync=");
      Serial.println(position_ratio_sync);     
    }
    */
  }
  
};