#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadPedalState_Extended {
  unsigned long timeInUs_u32;
  uint32_t cycleCount_u32;
  //unsigned long timeInUsFromSerialTask_u32;
  float pedalForce_raw_fl32;
  float pedalForce_filtered_fl32;
  float forceVel_est_fl32;

  // register values from servo
  int32_t servoPosition_i32;
  int32_t servoPositionTarget_i32;
  int16_t servoPositionEstimated_i16;
  int32_t targetPosition_i32;
  int32_t currentSpeedInMilliHz_i32;
  //int16_t servoPositionEstimated_stepperPos_i16;
  int16_t servo_position_error_i16;
  uint16_t angleSensorOutput_ui16;
  int16_t servo_voltage_0p1V;
  int16_t servo_current_percent_i16;
  uint8_t brakeResistorState_b;
};