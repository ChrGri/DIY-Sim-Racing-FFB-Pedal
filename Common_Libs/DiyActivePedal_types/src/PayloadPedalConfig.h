#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadPedalConfig
{
  // configure pedal start and endpoint
  // In percent
  uint8_t pedalStartPosition_u8;
  uint8_t pedalEndPosition_u8;

  // configure pedal forces
  float maxForce_fl32;
  float preloadForce_fl32;
  
  // design force vs travel curve
  uint8_t quantityOfControl_u8;
  uint8_t relativeForce00_u8;
  uint8_t relativeForce01_u8;
  uint8_t relativeForce02_u8;
  uint8_t relativeForce03_u8;
  uint8_t relativeForce04_u8;
  uint8_t relativeForce05_u8;
  uint8_t relativeForce06_u8;
  uint8_t relativeForce07_u8;
  uint8_t relativeForce08_u8;
  uint8_t relativeForce09_u8;
  uint8_t relativeForce10_u8;
  uint8_t relativeTravel00_u8;
  uint8_t relativeTravel01_u8;
  uint8_t relativeTravel02_u8;
  uint8_t relativeTravel03_u8;
  uint8_t relativeTravel04_u8;
  uint8_t relativeTravel05_u8;
  uint8_t relativeTravel06_u8;
  uint8_t relativeTravel07_u8;
  uint8_t relativeTravel08_u8;
  uint8_t relativeTravel09_u8;
  uint8_t relativeTravel10_u8;

  uint8_t numOfJoystickMapControl_u8;
  uint8_t joystickMapOrig00_u8;
  uint8_t joystickMapOrig01_u8;
  uint8_t joystickMapOrig02_u8;
  uint8_t joystickMapOrig03_u8;
  uint8_t joystickMapOrig04_u8;
  uint8_t joystickMapOrig05_u8;
  uint8_t joystickMapOrig06_u8;
  uint8_t joystickMapOrig07_u8;
  uint8_t joystickMapOrig08_u8;
  uint8_t joystickMapOrig09_u8;
  uint8_t joystickMapOrig10_u8;
  uint8_t joystickMapMapped00_u8;
  uint8_t joystickMapMapped01_u8;
  uint8_t joystickMapMapped02_u8;
  uint8_t joystickMapMapped03_u8;
  uint8_t joystickMapMapped04_u8;
  uint8_t joystickMapMapped05_u8;
  uint8_t joystickMapMapped06_u8;
  uint8_t joystickMapMapped07_u8;
  uint8_t joystickMapMapped08_u8;
  uint8_t joystickMapMapped09_u8;
  uint8_t joystickMapMapped10_u8;

  // configure ABS effect 
  uint8_t absFrequency_u8; // In Hz
  uint8_t absAmplitude_u8; // In kg/20
  uint8_t absPattern_u8; // 0: sinewave, 1: sawtooth
  uint8_t absForceOrTarvelBit_u8; // 0: Force, 1: travel


  // geometric properties of the pedal
  // in mm
  int16_t lengthPedalA_i16;
  int16_t lengthPedalB_i16;
  int16_t lengthPedalD_i16;
  int16_t lengthPedalCHorizontal_i16;
  int16_t lengthPedalCVertical_i16;
  int16_t lengthPedalTravel_i16;
  

  //Simulate ABS trigger
  uint8_t simulateAbsTrigger_u8;
  uint8_t simulateAbsValue_u8;
  // configure for RPM effect
  uint8_t rpmMaxFreq_u8; //In HZ
  uint8_t rpmMinFreq_u8; //In HZ
  uint8_t rpmAmp_u8; //In Kg

  //configure for bite point
  uint8_t bpTriggerValue_u8;
  uint8_t bpAmp_u8;
  uint8_t bpFreq_u8;
  uint8_t bpTrigger_u8;
  //G force effect
  uint8_t gMulti_u8;
  uint8_t gWindow_u8;
  //wheel slip
  uint8_t wsAmp_u8;
  uint8_t wsFreq_u8;
  //Road impact effect
  uint8_t roadMulti_u8;
  uint8_t roadWindow_u8;
  //Custom Vibration 1
  uint8_t cvAmp1_u8;
  uint8_t cvFreq1_u8;
  //Custom Vibration 2
  uint8_t cvAmp2_u8;
  uint8_t cvFreq2_u8;
  //Custom Vibration 3
  uint8_t cvAmp3_u8;
  uint8_t cvFreq3_u8;
  //Custom Vibration 4
  uint8_t cvAmp4_u8;
  uint8_t cvFreq4_u8;
  // controller settings
  uint8_t maxGameOutput_u8;

  // Kalman filter model noise
  uint8_t kfModelNoise_u8;
  uint8_t kfModelOrder_u8;

  // debug flags, sued to enable debug output
  uint8_t debugFlags0_u8;

  // loadcell rating in kg / 2 --> to get value in kg, muiltiply by 2
  uint8_t loadcellRating_u8;

  // use loadcell or travel as joystick output
  uint8_t travelAsJoystickOutput_u8;

  // invert loadcell sign
  uint8_t invertLoadcellReading_u8;

  // invert motor direction
  uint8_t invertMotorDirection_u8;

  // spindle pitch in mm/rev
  uint8_t spindlePitch_mmPerRev_u8;

  //pedal type, 0= clutch, 1= brake, 2= gas
  uint8_t pedalType_u8;
  uint8_t stepLossFunctionFlags_u8;
  uint8_t kfJoystick_u8;
  uint8_t kfModelNoiseJoystick_u8;
  uint8_t servoIdleTimeout_u8;
  uint8_t positionSmoothingFactor_u8;
  uint8_t minForceForEffects_u8;
  uint8_t servoRatioOfInertia_u8;
  uint32_t configHash_u32;
  uint8_t endstopDetectionThreshold_u8;

  uint8_t virtualPedalMass_u8;
  uint8_t virtualPedalDamping_u8;

} PayloadPedalConfig_t;