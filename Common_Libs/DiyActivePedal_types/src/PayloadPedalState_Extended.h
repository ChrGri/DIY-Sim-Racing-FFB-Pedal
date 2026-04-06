#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadPedalStateExtended
{
  // register values from servo
  uint32_t servoStateCycleCount_u32;
  int32_t servoPositionTarget_i32;
  int32_t servoPositionFeedback_i32;
  int16_t servoPositionError_i16;
  int16_t servoVoltage0p1V_i16;
  int16_t servoCurrentPercent_i16;
  

  // values from ESP
  uint32_t timeInUs_u32;
  uint32_t cycleCount_u32;
  float pedalForceRaw_fl32;
  float pedalForceFiltered_fl32;
  float forceVelEst_fl32;
  int32_t targetPosition_i32;
  int32_t currentSpeedInHz_i32;
  uint8_t brakeResistorState_b;
  uint8_t oscillationMonitorValue_u8;

  float admittance_expectedForce_N;
  uint8_t admittance_isOscillating;
  float admittance_admittancePsi_N;
  float admittance_virtualMass_kg;
  float admittance_virtualDamping_Ns_m;

} PayloadPedalStateExtended_t;