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
#include "PayloadServoConfig.h"

// define the payload revision
typedef struct __attribute__((packed)) DapActions
{
  PayloadHeader_t payloadHeader_st;
  PayloadPedalAction_t payloadPedalAction_st;
  PayloadFooter_t payloadFooter_st;
} DapActions_t;

typedef struct __attribute__((packed)) DapStateBasic
{
  PayloadHeader_t payloadHeader_st;
  PayloadPedalStateBasic_t payloadPedalStateBasic_st;
  PayloadFooter_t payloadFooter_st;
} DapStateBasic_t;

typedef struct __attribute__((packed)) DapStateExtended
{
  PayloadHeader_t payloadHeader_st;
  PayloadPedalStateExtended_t payloadPedalStateExtended_st;
  PayloadFooter_t payloadFooter_st;
} DapStateExtended_t;

typedef struct __attribute__((packed)) DapBridgeState
{
  PayloadHeader_t payloadHeader_st;
  PayloadBridgeState_t payloadBridgeState_st;
  PayloadFooter_t payloadFooter_st;
} DapBridgeState_t;

typedef struct __attribute__((packed)) DapActionOta
{
  PayloadHeader_t payloadHeader_st;
  PayloadOtaInfo_t payloadOtaInfo_st;
  PayloadFooter_t payloadFooter_st;
} DapActionOta_t;

typedef struct __attribute__((packed)) DapConfig
{

  PayloadHeader_t payloadHeader_st;
  PayloadPedalConfig_t payloadPedalConfig_st;
  PayloadFooter_t payloadFooter_st;
  
  void initializeDefaults();
  void loadConfigFromEeprom(DapConfig& config_st);
  void storeConfigToEeprom(DapConfig& config_st);
} DapConfig_t;

typedef struct __attribute__((packed)) DapEspPairing
{
  PayloadHeader_t payloadHeader_st;
  PayloadEspnowInfo_t payloadEspnowInfo_st;
  PayloadFooter_t payloadFooter_st;
} DapEspPairing_t;

typedef struct __attribute__((packed)) DapAssignmentBroadcast
{
  PayloadHeader_t payloadHeader_st;
  PayloadAssignmentRequest_t payloadAssignmentRequest_st;
  PayloadFooter_t payloadFooter_st;
} DapAssignmentBroadcast_t;

typedef struct __attribute__((packed)) DapRudder
{
  PayloadHeader_t payloadHeader_st;
  PayloadRudderState_t payloadRudderState_st;
  PayloadFooter_t payloadFooter_st;
} DapRudder_t;

typedef struct DapAssignmentReg
{
  uint8_t payloadType_u8;
  uint8_t magicKey_u8;
  uint8_t isAdvancedPaired_u8;
  uint8_t deviceId_u8;
  uint8_t pairStatus_au8[4];
  uint8_t pairedMac_aau8[4][6];
  uint16_t crc_u16;
} DapAssignmentReg_t;

typedef struct __attribute__((packed)) DAP_servo_config_st {
  PayloadHeader_t payloadHeader_st;
  payloadServoConfig_t payloadServoConfig_st;
  PayloadFooter_t payloadFooter_st;
} DAP_servo_config_st_t;

typedef struct DapCalculationVariables
{
  float springStiffnesss_fl32;
  float springStiffnesssInv_fl32;
  float forceMin_fl32;
  float forceMax_fl32;
  float forceRange_fl32;
  int32_t stepperPosMinEndstop_i32;
  int32_t stepperPosMaxEndstop_i32;
  int32_t stepperPosEndstopRange_i32;
  float rpmMaxFreq_fl32;
  float rpmMinFreq_fl32;
  float rpmAmp_fl32;
  int32_t softEndstopMinStepperPos_i32; // soft endstop min position in steps
  int32_t softEndstopMaxStepperPos_i32; // soft endstop max position in steps
  //int32_t hardEndstopMinStepperPos_i32; // hard endstop min position in steps. This should not be exceeded under any circumstances to avoid hardware damage
  //int32_t hardEndstopMaxStepperPos_i32; // hard endstop max position in steps. This should not be exceeded under any circumstances to avoid hardware damage
  float stepperPosRange_fl32;
  float startPosRel_fl32;
  float endPosRel_fl32;
  float absFrequency_fl32;
  float absAmplitude_fl32;
  float rpmValue_fl32;
  float bpTriggerValue_fl32;
  float bpAmp_fl32;
  float bpFreq_fl32;
  float dampingPress_fl32;
  float forceMaxDefault_fl32;
  float wsAmp_fl32;
  float wsFreq_fl32;
  volatile bool rudderStatus_b;
  bool isRudderInitialized_b = false;
  volatile bool helicopterRudderStatus_b;
  bool isHelicopterRudderInitialized_b = false;
  uint8_t pedalType_u8;
  uint32_t syncPedalPosition_u32;
  uint32_t currentPedalPosition_u32;
  float currentPedalPositionRatio_fl32;
  float syncPedalPositionRatio_fl32;
  volatile bool rudderBrakeStatus_b;
  int32_t stepperPosMinDefault_i32;
  int32_t stepperPosMaxDefault_i32;
  float stepperPosRangeDefault_fl32;
  uint32_t stepsPerMotorRevolution_u32;
  volatile uint8_t trackCondition_u8;
  float currentForceReading_fl32;
  float force_afl32[11];
  float travel_afl32[11]; 
  float *interpolatorA_pfl32 = nullptr;
  float *interpolatorB_pfl32 = nullptr;
  float *joystickInterpolatorA_pfl32 = nullptr;
  float *joystickInterpolatorB_pfl32 = nullptr;
  float joystickOrig_afl32[11];
  float joystickMapping_afl32[11];
  uint8_t numOfJoystickControl_u8;
  Cubic cubic_st;
  Cubic joystickInterpolator_st;
  void updateFromConfig(DapConfig_t& config_st);
  void updateEndstops(int32_t newMinEndstop_i32, int32_t newMaxEndstop_i32);
  void updateStiffness();
  void dynamicUpdate();
  void resetMaxForce();
  void stepperPosSetback();
  void setDefaultPos();
  void updateStepperMinPos(int32_t newMinstop_i32);
  void updateStepperMaxPos(int32_t newMaxstop_i32);
} DapCalculationVariables_t;

class DapConfigClass
{
public:
  // Konstruktor
  DapConfigClass();

  // Methode zum sicheren Abrufen der Konfiguration
  bool getConfig(DapConfig_t *dapConfigIn_pst, uint16_t timeoutInMs_u16);

  // Methode zum sicheren Setzen der Konfiguration
  void setConfig(DapConfig_t config_st);

  // Methode zum Laden der Konfiguration aus dem EEPROM
  void loadConfigFromEeprom();

  // Methode zum Speichern der Konfiguration im EEPROM
  void storeConfigToEeprom();

  //initialized config if needed
  void initializedConfig();

private:
  SemaphoreHandle_t mutex_sh;
  DapConfig_t config_st;
  uint16_t checksumCalculator_u16(uint8_t *data_pu8, uint16_t length_u16);
};

