#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadPedalConfig {
  // configure pedal start and endpoint
  // In percent
  uint8_t pedalStartPosition;
  uint8_t pedalEndPosition;

  // configure pedal forces
  float maxForce;
  float preloadForce;
  
  // design force vs travel curve
  uint8_t quantityOfControl;
  uint8_t relativeForce00;
  uint8_t relativeForce01;
  uint8_t relativeForce02;
  uint8_t relativeForce03;
  uint8_t relativeForce04;
  uint8_t relativeForce05;
  uint8_t relativeForce06;
  uint8_t relativeForce07;
  uint8_t relativeForce08;
  uint8_t relativeForce09;
  uint8_t relativeForce10;
  uint8_t relativeTravel00;
  uint8_t relativeTravel01;
  uint8_t relativeTravel02;
  uint8_t relativeTravel03;
  uint8_t relativeTravel04;
  uint8_t relativeTravel05;
  uint8_t relativeTravel06;
  uint8_t relativeTravel07;
  uint8_t relativeTravel08;
  uint8_t relativeTravel09;
  uint8_t relativeTravel10;

  uint8_t numOfJoystickMapControl;
  uint8_t joystickMapOrig00;
  uint8_t joystickMapOrig01;
  uint8_t joystickMapOrig02;
  uint8_t joystickMapOrig03;
  uint8_t joystickMapOrig04;
  uint8_t joystickMapOrig05;
  uint8_t joystickMapOrig06;
  uint8_t joystickMapOrig07;
  uint8_t joystickMapOrig08;
  uint8_t joystickMapOrig09;
  uint8_t joystickMapOrig10;
  uint8_t joystickMapMapped00;
  uint8_t joystickMapMapped01;
  uint8_t joystickMapMapped02;
  uint8_t joystickMapMapped03;
  uint8_t joystickMapMapped04;
  uint8_t joystickMapMapped05;
  uint8_t joystickMapMapped06;
  uint8_t joystickMapMapped07;
  uint8_t joystickMapMapped08;
  uint8_t joystickMapMapped09;
  uint8_t joystickMapMapped10;
  // parameter to configure damping
  uint8_t dampingPress;
  uint8_t dampingPull;

  // configure ABS effect 
  uint8_t absFrequency; // In Hz
  uint8_t absAmplitude; // In kg/20
  uint8_t absPattern; // 0: sinewave, 1: sawtooth
  uint8_t absForceOrTarvelBit; // 0: Force, 1: travel


  // geometric properties of the pedal
  // in mm
  int16_t lengthPedal_a;
  int16_t lengthPedal_b;
  int16_t lengthPedal_d;
  int16_t lengthPedal_c_horizontal;
  int16_t lengthPedal_c_vertical;
  int16_t lengthPedal_travel;
  

  //Simulate ABS trigger
  uint8_t Simulate_ABS_trigger;
  uint8_t Simulate_ABS_value;
  // configure for RPM effect
  uint8_t RPM_max_freq; //In HZ
  uint8_t RPM_min_freq; //In HZ
  uint8_t RPM_AMP; //In Kg

  //configure for bite point
  uint8_t BP_trigger_value;
  uint8_t BP_amp;
  uint8_t BP_freq;
  uint8_t BP_trigger;
  //G force effect
  uint8_t G_multi;
  uint8_t G_window;
  //wheel slip
  uint8_t WS_amp;
  uint8_t WS_freq;
  //Road impact effect
  uint8_t Road_multi;
  uint8_t Road_window;
  //Custom Vibration 1
  uint8_t CV_amp_1;
  uint8_t CV_freq_1;
  //Custom Vibration 2
  uint8_t CV_amp_2;
  uint8_t CV_freq_2;

  // controller settings
  uint8_t maxGameOutput;

  // Kalman filter model noise
  uint8_t kf_modelNoise;
  uint8_t kf_modelOrder;

  // debug flags, sued to enable debug output
  uint8_t debug_flags_0;

  // loadcell rating in kg / 2 --> to get value in kg, muiltiply by 2
  uint8_t loadcell_rating;

  // use loadcell or travel as joystick output
  uint8_t travelAsJoystickOutput_u8;

  // invert loadcell sign
  uint8_t invertLoadcellReading_u8;

  // invert motor direction
  uint8_t invertMotorDirection_u8;

  // spindle pitch in mm/rev
  uint8_t spindlePitch_mmPerRev_u8;

  //pedal type, 0= clutch, 1= brake, 2= gas
  uint8_t pedal_type;
  uint8_t stepLossFunctionFlags_u8;
  uint8_t kf_Joystick_u8;
  uint8_t kf_modelNoise_joystick;
  uint8_t servoIdleTimeout;
  uint8_t positionSmoothingFactor_u8;
  uint8_t minForceForEffects_u8;
  uint8_t servoRatioOfInertia_u8;
  uint32_t configHash;

};