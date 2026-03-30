#pragma once
#define Rudder_timeout 3000
#include "DiyActivePedal_types.h"
#include <MovingAverageFilter.h>
#include "SignalFilter_1st_order.h"
MovingAverageFilter averagefilter_rudder(200);
MovingAverageFilter averagefilter_rudder_force(50);
class Rudder{
  public:
  int32_t centerOffset_i32;
  int32_t offsetRaw_i32;
  int32_t offsetFilter_i32;
  int32_t stepperRange_i32;
  int32_t deadZoneUpper_i32;
  int32_t deadZoneLower_i32;
  int32_t deadZone_i32;
  int32_t syncPedalPosition_i32;
  int32_t currentPedalPosition_i32;
  float endPosTravel_fl32;
  float forceRange_fl32;
  float forceOffsetRaw_fl32;
  float forceOffsetFilter_fl32;
  float forceCenterOffset_fl32;
  float positionRatioSync_fl32;
  float positionRatioCurrent_fl32;
  int debugCount_i32=0;
  KalmanFilter1stOrder* kalman_rudder = NULL;
  //bool IsReady = false;
  Rudder()
  {
    kalman_rudder=new KalmanFilter1stOrder(3.0f);
  }
  void offsetCalculate(DapCalculationVariables_t* calcVars_st)
  {
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    positionRatioSync_fl32=calcVars_st->syncPedalPositionRatio_fl32;
    endPosTravel_fl32=(float)calcVars_st->stepperPosRange_fl32;
    positionRatioCurrent_fl32=((float)(currentPedalPosition_i32-calcVars_st->softEndstopMinStepperPos_i32))/endPosTravel_fl32;
    deadZone_i32=20;
    centerOffset_i32=calcVars_st->softEndstopMinStepperPos_i32+ calcVars_st->stepperPosRange_fl32/2.0f;
    float centerDeadzone_fl32 = 0.51f;
    if(calcVars_st->rudderStatus_b)
    {
      if(positionRatioSync_fl32>centerDeadzone_fl32)
      {
        offsetRaw_i32=(int32_t)(-1*(positionRatioSync_fl32-0.50f)*endPosTravel_fl32);
          
      }
      else
      {
        offsetRaw_i32=0;
      }
      if(calcVars_st->rudderBrakeStatus_b)
      {
        offsetRaw_i32=0;
      }
      offsetFilter_i32=(int32_t)kalman_rudder->filteredValue(offsetRaw_i32+centerOffset_i32,0.0f,1);
      //offset_filter=averagefilter_rudder.process(offset_raw+Center_offset);
      //cap offset filter to prevent over the endstop value
      offsetFilter_i32=constrain(offsetFilter_i32,calcVars_st->stepperPosMinDefault_i32,calcVars_st->stepperPosMaxDefault_i32);
    }
    else
    {
      offsetFilter_i32=calcVars_st->softEndstopMinStepperPos_i32;
    }

  }
  void forceOffsetCalculate(DapCalculationVariables_t* calcVars_st)
  {
    deadZone_i32=20;
    centerOffset_i32=calcVars_st->stepperPosRange_fl32/2.0f;
    deadZoneUpper_i32=centerOffset_i32+deadZone_i32/2.0f;
    deadZoneLower_i32=centerOffset_i32-deadZone_i32/2.0f;
    syncPedalPosition_i32=calcVars_st->syncPedalPosition_u32;
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    stepperRange_i32=calcVars_st->stepperPosRange_fl32;
    forceRange_fl32 = calcVars_st->forceRange_fl32;
    forceCenterOffset_fl32 = forceRange_fl32 / 2 + calcVars_st->forceMin_fl32;
    endPosTravel_fl32=(float)calcVars_st->stepperPosRange_fl32;
    //endpos_travel=((float)(calcVars_st->currentPedalPosition_u32-calcVars_st->softEndstopMinStepperPos_i32))/((float)calcVars_st->stepperPosRange_fl32);
    positionRatioSync_fl32=calcVars_st->syncPedalPositionRatio_fl32;
    positionRatioCurrent_fl32=((float)(currentPedalPosition_i32-calcVars_st->softEndstopMinStepperPos_i32))/endPosTravel_fl32;
    

    float centerDeadzone_fl32 = 0.51f;
    if(calcVars_st->rudderStatus_b)
    {
      
        
        if(positionRatioSync_fl32>centerDeadzone_fl32)
        {
          forceOffsetRaw_fl32 = (float)(-1.0f * (positionRatioSync_fl32 - 0.50f) * forceRange_fl32);
          
        }
        else
        {
          forceOffsetRaw_fl32=0.0f;
        }
        if(calcVars_st->rudderBrakeStatus_b)
        {
          forceOffsetRaw_fl32=0.0f;
        }
     
      forceOffsetFilter_fl32=averagefilter_rudder_force.process(forceOffsetRaw_fl32+forceCenterOffset_fl32);
    }
    else
    {
      forceOffsetFilter_fl32=0;
    }
  }
};
//Rudder impact
MovingAverageFilter Averagefilter_Rudder_G_Offset(50);
class RudderGForce{
  public:
  int32_t offsetRaw_i32;
  long offsetFilter_l;
  float stepperRange_fl32;
  uint8_t gValue_u8;
  long stepperPosMax_l;
  void offsetCalculate(DapCalculationVariables_t* calcVars_st)
  {
    stepperPosMax_l=(float)calcVars_st->softEndstopMaxStepperPos_i32;
    stepperRange_fl32=(float)calcVars_st->stepperPosRange_fl32;
    float ampMax_fl32=0.3f*stepperRange_fl32;
    if(calcVars_st->rudderStatus_b)
    {
      float offset_fl32= ampMax_fl32*((float)gValue_u8)/100.0f;
      //offset=constrain(offset,0,Amp_max);
      offsetFilter_l=Averagefilter_Rudder_G_Offset.process((stepperPosMax_l-(int32_t)offset_fl32));
    }
    else
    {
      offsetFilter_l=calcVars_st->softEndstopMaxStepperPos_i32;
    }

  }
};
MovingAverageFilter averagefilter_helirudder(200);
class HelicoptersRudder{
  public:
  int32_t centerOffset_i32;
  int32_t offsetRaw_i32;
  int32_t offsetFilter_i32;
  int32_t stepperRange_i32;
  int32_t deadZoneUpper_i32;
  int32_t deadZoneLower_i32;
  int32_t deadZone_i32;
  int32_t syncPedalPosition_i32;
  int32_t currentPedalPosition_i32;
  float endPosTravel_fl32;
  float forceRange_fl32;
  float forceOffsetRaw_fl32;
  float forceOffsetFilter_fl32;
  float forceCenterOffset_fl32;
  float positionRatioSync_fl32;
  float positionRatioCurrent_fl32;
  float currentForceReading_fl32;
  float pedalPreload_fl32;
  float offsetLast_fl32;
  int debugCount_i32=0;
  float deadzoneTolerance_fl32=0.01f;
  float positionRatioLast_fl32;
  unsigned long debugPrintLast=0;
  KalmanFilter1stOrder* kalman_rudder = NULL;
  //bool IsReady = false;
  HelicoptersRudder()
  {
    kalman_rudder=new KalmanFilter1stOrder(3.0f);
  }
  void offsetCalculate(DapCalculationVariables_t* calcVars_st)
  {
    currentPedalPosition_i32=calcVars_st->currentPedalPosition_u32;
    positionRatioSync_fl32=calcVars_st->syncPedalPositionRatio_fl32;
    endPosTravel_fl32=(float)calcVars_st->stepperPosRange_fl32;
    currentForceReading_fl32=(float)calcVars_st->currentForceReading_fl32;
    pedalPreload_fl32=(float)calcVars_st->forceMin_fl32;
    positionRatioCurrent_fl32=((float)(currentPedalPosition_i32-calcVars_st->softEndstopMinStepperPos_i32))/endPosTravel_fl32;
    deadZone_i32=20;
    centerOffset_i32=calcVars_st->softEndstopMinStepperPos_i32+ calcVars_st->stepperPosRange_fl32/2.0f;
    float centerDeadzone_fl32 = 0.51f;
    if(calcVars_st->helicopterRudderStatus_b)
    {
      //no press status
      if(currentForceReading_fl32<pedalPreload_fl32)
      {
        if(calcVars_st->isHelicopterRudderInitialized_b)
        {
          if(abs((1-positionRatioSync_fl32)-positionRatioLast_fl32)>deadzoneTolerance_fl32)
          {
            offsetRaw_i32=(int32_t)(-1*(positionRatioSync_fl32-0.50f)*endPosTravel_fl32);
            offsetLast_fl32=offsetRaw_i32;
            positionRatioLast_fl32=constrain((1-positionRatioSync_fl32),0,1);
          }
          else
          {
            offsetRaw_i32=offsetLast_fl32;
          }  
        }
        else
        {
          offsetRaw_i32=0;
        }

      }
      else
      {
        if((positionRatioCurrent_fl32-positionRatioLast_fl32)>deadzoneTolerance_fl32)
        {
          positionRatioLast_fl32=positionRatioCurrent_fl32;
          offsetRaw_i32=0;
          offsetLast_fl32=(int32_t)((positionRatioCurrent_fl32-0.5f)*endPosTravel_fl32);
        }
        else
        {
          offsetRaw_i32=(int32_t)((positionRatioLast_fl32-0.5f)*endPosTravel_fl32);
          offsetLast_fl32=offsetRaw_i32;
        }


      }
      //offset_filter=(int32_t)kalman_rudder->filteredValue(offset_raw+Center_offset,0.0f,1);
      offsetFilter_i32=averagefilter_helirudder.process(offsetRaw_i32+centerOffset_i32);
      //cap offset filter to prevent over the endstop value
      offsetFilter_i32=constrain(offsetFilter_i32,calcVars_st->stepperPosMinDefault_i32,calcVars_st->stepperPosMaxDefault_i32);
    }
    else
    {
      offsetFilter_i32=calcVars_st->softEndstopMinStepperPos_i32;
    }
    /*
    if(debugPrintLast-millis()>200)
    {
      ActiveSerial->print("Current Force=");
      ActiveSerial->println(currentForceReading_fl32);
      ActiveSerial->print("offset_filter=");
      ActiveSerial->println(offset_filter);
      ActiveSerial->print("offset_Last=");
      ActiveSerial->println(offsetLast_fl32);
      ActiveSerial->print("position_ratio_Last=");
      ActiveSerial->println(position_ratio_last);
      ActiveSerial->print("position_ratio_sync=");
      ActiveSerial->println(position_ratio_sync);     
    }
    */
  }
  
};