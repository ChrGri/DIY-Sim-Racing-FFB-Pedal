#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadPedalStateExtended
{
  uint32_t timeInUs_u32;
  uint32_t cycleCount_u32;
  //unsigned long timeInUsFromSerialTask_u32;
  float pedalForceRaw_fl32;
  float pedalForceFiltered_fl32;
  float forceVelEst_fl32;

  // register values from servo
  int32_t servoPosition_i32;
  int32_t servoPositionTarget_i32;
  int16_t servoPositionEstimated_i16;
  int32_t targetPosition_i32;
  int32_t currentSpeedInMilliHz_i32;
  //int16_t servoPositionEstimated_stepperPos_i16;
  int16_t servoPositionError_i16;
  uint16_t angleSensorOutput_u16;
  int16_t servoVoltage0p1V_i16;
  int16_t servoCurrentPercent_i16;
  uint8_t brakeResistorState_b;
} PayloadPedalStateExtended_t;