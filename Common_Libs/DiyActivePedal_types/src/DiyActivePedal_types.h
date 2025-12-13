#pragma once

#include <stdint.h>
#include "Arduino.h"
#include "CubicInterpolatorFloat.h"
// define the payload revision
#define DAP_VERSION_CONFIG 163

// define the payload types
#define DAP_PAYLOAD_TYPE_CONFIG 100
#define DAP_PAYLOAD_TYPE_ACTION 110
#define DAP_PAYLOAD_TYPE_STATE_BASIC 120
#define DAP_PAYLOAD_TYPE_STATE_EXTENDED 130
#define DAP_PAYLOAD_TYPE_ESPNOW_PAIRING 140
#define DAP_PAYLOAD_TYPE_ESPNOW_RUDDER 150
#define DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK 160
#define DAP_PAYLOAD_TYPE_BRIDGE_STATE 210
#define DAP_PAYLOAD_TYPE_ACTION_OTA 220
#define DAP_PAYLOAD_TYPE_ESPNOW_LOG 225
#define DAP_PAYLOAD_TYPE_ASSIGNMENT 230



#define SOF_BYTE_0 0xAA
#define SOF_BYTE_1 0x55
#define EOF_BYTE_0 0xAA
#define EOF_BYTE_1 0x56

#define ASSIGNMENT_EEPROM_OFFSET 9

enum pedalStatus{
  PEDAL_STATUS_NORMAL,
  PEDAL_STATUS_RUDDER,
  PEDAL_STATUS_RUDDERBRAKE
};
enum pedalID{
  PEDAL_ID_CLUTCH,
  PEDAL_ID_BRAKE,
  PEDAL_ID_THROTTLE,
  PEDAL_ID_ASSIGNMENT_ERROR,
  PEDAL_ID_UNKNOWN,
  PEDAL_ID_TEMP_1,
  PEDAL_ID_TEMP_2,
  PEDAL_ID_TEMP_3
};
enum class PedalSystemAction{
  NONE,
  RESET_PEDAL_POSITION, // not in use
  PEDAL_RESTART,
  ENABLE_OTA,     // not in use
  ENABLE_PAIRING, // not in use
  ESP_BOOT_INTO_DOWNLOAD_MODE,
  PRINT_PEDAL_INFO,
  SET_ASSIGNMENT_0,
  SET_ASSIGNMENT_1,
  SET_ASSIGNMENT_2,
  CLEAR_ASSIGNMENT,
  ASSIGNMENT_CHECK_BEEP
};
enum class RudderAction{
  NONE,
  RUDDER_THROTTLE_AND_BRAKE,
  RUDDER_CLEAR_RUDDER_STATUS,
  RUDDER_THROTTLE_AND_CLUTCH,
  HELIRUDDER_THROTTLE_AND_BRAKE,
  HELIRUDDER_THROTTLE_AND_CLUTCH
};

enum bridgeAction{
  BRIDGE_ACTION_NONE,
  BRIDGE_ACTION_ENABLE_PAIRING,
  BRIDGE_ACTION_RESTART,
  BRIDGE_ACTION_DOWNLOAD_MODE,
  BRIDGE_ACTION_DEBUG,
  BRIDGE_ACTION_JOYSTICK_FLASHING_MODE,
  BRIDGE_ACTION_JOYSTICK_DEBUG 
};

enum otaAction{
  OTA_ACTION_NORMAL,
  OTA_ACTION_FORCE_UPDATE,
  OTA_ACTION_PLATFORMIO_DIRECT_UPLOAD
};
struct payloadHeader{
  // start of frame indicator
  uint8_t startOfFrame0_u8;
  uint8_t startOfFrame1_u8;

  // structure identification via payload
  uint8_t payloadType;

  // variable to check if structure at receiver matched version from transmitter
  uint8_t version;

  // store to EEPROM flag
  uint8_t storeToEeprom;

  // pedal tag
  uint8_t PedalTag;
};

struct payloadPedalAction {
  uint8_t triggerAbs_u8;
  //uint8_t resetPedalPos_u8; //1=reset position, 2=restart ESP
  uint8_t system_action_u8; //1=reset position, 2=restart ESP, 3=OTA Enable, 4=enable pairing
  uint8_t startSystemIdentification_u8;
  uint8_t returnPedalConfig_u8;
  uint8_t RPM_u8;
  uint8_t G_value;
  uint8_t WS_u8;
  uint8_t impact_value_u8;
  uint8_t Trigger_CV_1;
  uint8_t Trigger_CV_2;
  uint8_t Rudder_action;
  uint8_t Rudder_brake_action;
};


struct payloadPedalState_Basic {
  uint16_t pedalPosition_u16;
  uint16_t pedalForce_u16;
  uint16_t joystickOutput_u16;
  uint8_t error_code_u8;
  uint8_t pedalFirmwareVersion_u8[3];
  uint8_t servoStatus;
  uint8_t pedalStatus;
  uint8_t pedalContrlBoardType;
};

struct payloadPedalState_Extended {
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

struct payloadBridgeState{
  uint8_t unassignedPedalCount;
  uint8_t Pedal_availability[3];
  uint8_t Bridge_action; // 0=none, 1=enable pairing 2=Restart 3=download mode
  uint8_t Bridge_firmware_version_u8[3];
  int32_t Pedal_RSSI_Realtime[3];
  uint8_t macAddressDetected[18];
};

struct payloadRudderState {
  uint16_t pedal_position;
  float pedal_position_ratio;
};
struct payloadPedalConfig {
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

struct payloadESPNowInfo{
  uint8_t _deviceID;
  uint8_t occupy;
  uint8_t occupy2;

};

struct payloadOtaInfo{
    uint8_t device_ID;
    uint8_t ota_action;
    uint8_t mode_select;
    uint8_t SSID_Length;
    uint8_t PASS_Length;
    uint8_t WIFI_SSID[64];
    uint8_t WIFI_PASS[64];
};

struct payloadAssignmentRequest{
  uint8_t assignmentAction;
  uint8_t assignmentState;
  uint8_t macAddress[6];
};

struct payloadFooter {
  // To check if structure is valid
  uint16_t checkSum;

  // end of frame bytes
  uint8_t enfOfFrame0_u8;
  uint8_t enfOfFrame1_u8;
};


struct DAP_actions_st {
  payloadHeader payLoadHeader_;
  payloadPedalAction payloadPedalAction_;
  payloadFooter payloadFooter_; 
};

struct DAP_state_basic_st {
  payloadHeader payLoadHeader_;
  payloadPedalState_Basic payloadPedalState_Basic_;
  payloadFooter payloadFooter_; 
};

struct DAP_state_extended_st {
  payloadHeader payLoadHeader_;
  payloadPedalState_Extended payloadPedalState_Extended_;
  payloadFooter payloadFooter_; 
};
struct DAP_bridge_state_st {
  payloadHeader payLoadHeader_;
  payloadBridgeState payloadBridgeState_;
  payloadFooter payloadFooter_; 
};

struct DAP_action_ota_st {
  payloadHeader payLoadHeader_;
  payloadOtaInfo payloadOtaInfo_;
  payloadFooter payloadFooter_; 
};

struct DAP_config_st {

  payloadHeader payLoadHeader_;
  payloadPedalConfig payLoadPedalConfig_;
  payloadFooter payloadFooter_; 
  
  void initialiseDefaults();
  void loadConfigFromEprom(DAP_config_st& config_st);
  void storeConfigToEprom(DAP_config_st& config_st);
};

struct DAP_ESPPairing_st {
  payloadHeader payLoadHeader_;
  payloadESPNowInfo payloadESPNowInfo_;
  payloadFooter payloadFooter_; 
};

struct DAP_AssignmentBoardcast_st {
  payloadHeader payLoadHeader_;
  payloadAssignmentRequest payloadAssignmentRequest_;
  payloadFooter payloadFooter_; 
};
struct DAP_Rudder_st {
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
