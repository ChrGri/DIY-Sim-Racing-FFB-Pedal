#pragma once

#include <stdint.h>
#include "Arduino.h"
#include "CubicInterpolatorFloat.h"
#include "PedalEnum.h"
#include "PedalDefine.h"
#include "PayloadHeader.h"
#include "PayloadAction.h"
#include "PayloadPedalState_basic.h"
#include "PayloadPedalState_Extended.h"
#include "PayloadBridgeState.h"
#include "PayloadEspnowInfo.h"
#include "PayloadOtaInfo.h"
#include "PayloadRudderState.h"
#include "PayloadAssignmentRequest.h"
#include "PayloadPedalConfig.h"
#include "PayloadFooter.h"
#include "PayloadHidMessage.h"

// define the payload revision
struct __attribute__((packed)) DAP_actions_st {
  payloadHeader payLoadHeader_;
  payloadPedalAction payloadPedalAction_;
  payloadFooter payloadFooter_; 
};

struct __attribute__((packed)) DAP_state_basic_st {
  payloadHeader payLoadHeader_;
  payloadPedalState_Basic payloadPedalState_Basic_;
  payloadFooter payloadFooter_; 
};

struct __attribute__((packed)) DAP_state_extended_st {
  payloadHeader payLoadHeader_;
  payloadPedalState_Extended payloadPedalState_Extended_;
  payloadFooter payloadFooter_; 
};
struct __attribute__((packed)) DAP_bridge_state_st {
  payloadHeader payLoadHeader_;
  payloadBridgeState payloadBridgeState_;
  payloadFooter payloadFooter_; 
};

struct __attribute__((packed)) DAP_action_ota_st {
  payloadHeader payLoadHeader_;
  payloadOtaInfo payloadOtaInfo_;
  payloadFooter payloadFooter_; 
};

struct __attribute__((packed)) DAP_config_st {

  payloadHeader payLoadHeader_;
  payloadPedalConfig payLoadPedalConfig_;
  payloadFooter payloadFooter_; 
  
  void initialiseDefaults();
  void loadConfigFromEprom(DAP_config_st& config_st);
  void storeConfigToEprom(DAP_config_st& config_st);
};

struct __attribute__((packed)) DAP_ESPPairing_st {
  payloadHeader payLoadHeader_;
  payloadESPNowInfo payloadESPNowInfo_;
  payloadFooter payloadFooter_; 
};

struct __attribute__((packed)) DAP_AssignmentBoardcast_st {
  payloadHeader payLoadHeader_;
  payloadAssignmentRequest payloadAssignmentRequest_;
  payloadFooter payloadFooter_; 
};
struct __attribute__((packed)) DAP_Rudder_st {
  payloadHeader payLoadHeader_;
  payloadRudderState payloadRudderState_;
  payloadFooter payloadFooter_; 
};

struct DAP_Assignement_reg
{
  uint8_t payloadType;
  uint8_t magicKey;
  uint8_t isAdvancedPaired;
  uint8_t deviceID;
  uint8_t pairstatus[4];
  uint8_t pairedMac[4][6];
  uint16_t crc;
};

struct DAP_calculationVariables_st
{
  float springStiffnesss;
  float springStiffnesssInv;
  float Force_Min;
  float Force_Max;
  float Force_Range;
  long stepperPosMinEndstop;
  long stepperPosMaxEndstop;
  long stepperPosEndstopRange;
  float RPM_max_freq;
  float RPM_min_freq;
  float RPM_AMP;
  long stepperPosMin;
  long stepperPosMax;
  float stepperPosRange;
  float startPosRel;
  float endPosRel;
  float absFrequency;
  float absAmplitude;
  float rpm_value;
  float BP_trigger_value;
  float BP_amp;
  float BP_freq;
  float dampingPress;
  float Force_Max_default;
  float WS_amp;
  float WS_freq;
  bool Rudder_status;
  bool isRudderInitialized=false;
  bool helicopterRudderStatus;
  bool isHelicopterRudderInitialized=false;
  uint8_t pedal_type;
  uint32_t sync_pedal_position;
  uint32_t current_pedal_position;
  float current_pedal_position_ratio;
  float Sync_pedal_position_ratio;
  bool rudder_brake_status;
  long stepperPosMin_default;
  long stepperPosMax_default;
  float stepperPosRange_default;
  uint32_t stepsPerMotorRevolution;
  uint8_t TrackCondition;
  float currentForceReading;
  float force[11];
  float travel[11]; 
  float *interpolatorA= nullptr;
  float *interpolatorB = nullptr;
  float *joystickInterpolatorA= nullptr;
  float *joystickInterpolatorB = nullptr;
  float joystickOrig[11];
  float joystickMapping[11];
  uint8_t numOfJoystickControl;
  Cubic _cubic;
  Cubic joystickInterpolarter;
  void updateFromConfig(DAP_config_st& config_st);
  void updateEndstops(long newMinEndstop, long newMaxEndstop);
  void updateStiffness();
  void dynamic_update();
  void reset_maxforce();
  void StepperPos_setback();
  void Default_pos();
  void update_stepperMinpos(long newMinstop);
  void update_stepperMaxpos(long newMaxstop);
};

class DAP_config_class {
public:
  // Konstruktor
  DAP_config_class();

  // Methode zum sicheren Abrufen der Konfiguration
  bool getConfig(DAP_config_st * dapConfigIn_pst, uint16_t timeoutInMs_u16);

  // Methode zum sicheren Setzen der Konfiguration
  void setConfig(DAP_config_st tmp);

  // Methode zum Laden der Konfiguration aus dem EEPROM
  void loadConfigFromEprom();

  // Methode zum Speichern der Konfiguration im EEPROM
  void storeConfigToEprom();

  //initialized config if needed
  void initializedConfig();

private:
  SemaphoreHandle_t mutex;
  DAP_config_st _config_st;
  uint16_t checksumCalculator(uint8_t * data, uint16_t length);
};
