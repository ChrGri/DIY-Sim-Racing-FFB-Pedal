#pragma once

#include <stdint.h>
#include "Arduino.h"
#include "CubicInterpolatorFloat.h"
// define the payload revision
#define DAP_VERSION_CONFIG 152

// define the payload types
#define DAP_PAYLOAD_TYPE_CONFIG 100
#define DAP_PAYLOAD_TYPE_ACTION 110
#define DAP_PAYLOAD_TYPE_STATE_BASIC 120
#define DAP_PAYLOAD_TYPE_STATE_EXTENDED 130
#define DAP_PAYLOAD_TYPE_ESPNOW_PAIRING 140
#define DAP_PAYLOAD_TYPE_ESPNOW_RUDDER 150
#define DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK 160
#define DAP_PAYLOAD_TYPE_BRIDGE_STATE 210
#define DAP_PAYLOAD_TYPE_ESPNOW_LOG 225

struct payloadHeader {
  
  // structure identification via payload
  uint8_t payloadType;

  // variable to check if structure at receiver matched version from transmitter
  uint8_t version;

  // store to EEPROM flag
  uint8_t storeToEeprom;

  //pedal tag
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
  uint8_t erroe_code_u8;
  uint8_t pedalFirmwareVersion_u8[3];
  uint8_t servoStatus;
};

struct payloadPedalState_Extended {

  unsigned long timeInUs_u32;
  float pedalForce_raw_fl32;
  float pedalForce_filtered_fl32;
  float forceVel_est_fl32;

  // register values from servo
  int16_t servoPosition_i16;
  int16_t servoPositionTarget_i16;
  int16_t servoPositionEstimated_i16;
  //int16_t servoPositionEstimated_stepperPos_i16;
  int16_t servo_position_error_i16;
  uint16_t angleSensorOutput_ui16;
  int16_t servo_voltage_0p1V;
  int16_t servo_current_percent_i16;
  uint8_t brakeResistorState_b;
};

struct payloadBridgeState {
  uint8_t Pedal_RSSI;
  uint8_t Pedal_availability[3];
  uint8_t Bridge_action;//0=none, 1=enable pairing

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
  // In percent
  /*
  uint8_t relativeForce_p000; 
  uint8_t relativeForce_p020;
  uint8_t relativeForce_p040;
  uint8_t relativeForce_p060;
  uint8_t relativeForce_p080;
  uint8_t relativeForce_p100;
  */

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
  // cubic spline parameters
  /*
  float cubic_spline_param_a_array[5];
  float cubic_spline_param_b_array[5];
  */
  // PID parameters
  float PID_p_gain;
  float PID_i_gain;
  float PID_d_gain;
  float PID_velocity_feedforward_gain;

  // MPC settings
  float MPC_0th_order_gain;
  float MPC_1st_order_gain;
  float MPC_2nd_order_gain;

  uint8_t control_strategy_b;

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
  //OTA flag
  //uint8_t OTA_flag;

  uint8_t stepLossFunctionFlags_u8;
  //joystick out flag
  //uint8_t Joystick_ESPsync_to_ESP;

  uint8_t kf_Joystick_u8;
  uint8_t kf_modelNoise_joystick;
  uint8_t servoIdleTimeout;
  

};

struct payloadESPNowInfo{
  //uint8_t macAddr[6];
  uint8_t _deviceID;
  uint8_t occupy;
  uint8_t occupy2;

};
struct payloadFooter {
  // To check if structure is valid
  uint16_t checkSum;
};

typedef payloadHeader PayloadHeader_t;
typedef payloadPedalAction PayloadPedalAction_t;
typedef payloadPedalState_Basic PayloadPedalStateBasic_t;
typedef payloadPedalState_Extended PayloadPedalStateExtended_t;
typedef payloadBridgeState PayloadBridgeState_t;
typedef payloadRudderState PayloadRudderState_t;
typedef payloadPedalConfig PayloadPedalConfig_t;
typedef payloadESPNowInfo PayloadEspnowInfo_t;
typedef payloadFooter PayloadFooter_t;


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

struct DAP_config_st {

  payloadHeader payLoadHeader_;
  payloadPedalConfig payLoadPedalConfig_;
  payloadFooter payloadFooter_; 
  
  void initializeDefaults();
  void loadConfigFromEeprom(DAP_config_st& config_st);
  void storeConfigToEeprom(DAP_config_st& config_st);
};

struct DAP_ESPPairing_st {
  payloadHeader payLoadHeader_;
  payloadESPNowInfo payloadESPNowInfo_;
  payloadFooter payloadFooter_; 
};
struct DAP_Rudder_st {
  payloadHeader payLoadHeader_;
  payloadRudderState payloadRudderState_;
  payloadFooter payloadFooter_; 
};

struct DAP_calculationVariables_st
{
  float springStiffnesss;
  float springStiffnesssInv;
  float forceMin_f;
  float forceMax_f;
  float forceRange_f;
  long stepperPosMinEndstop;
  long stepperPosMaxEndstop;
  long stepperPosEndstopRange;
  float rpmMaxFreq_f;
  float rpmMinFreq_f;
  float rpmAmp_f;
  long stepperPosMin;
  long stepperPosMax;
  float stepperPosRange;
  float startPosRel;
  float endPosRel;
  float absFrequency;
  float absAmplitude;
  float rpmValue_f;
  float bpTriggerValue_f;
  float bpAmp_f;
  float bpFreq_f;
  float dampingPress;
  float forceMaxDefault_f;
  float wsAmp_f;
  float wsFreq_f;
  bool rudderStatus_b;
  bool isRudderInitialized_b=false;
  bool helicopterRudderStatus_b;
  bool isHelicopterRudderInitialized_b=false;
  uint8_t pedalType_u8;
  uint32_t syncPedalPosition_u32;
  uint32_t currentPedalPosition_u32;
  float currentPedalPositionRatio_f;
  float syncPedalPositionRatio_f;
  bool rudderBrakeStatus_b;
  long stepperPosMinDefault;
  long stepperPosMaxDefault;
  float stepperPosRangeDefault;
  uint32_t stepsPerMotorRevolution;
  uint8_t trackCondition_u8;
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
  void dynamicUpdate();
  void resetMaxForce();
  void stepperPosSetback();
  void setDefaultPos();
  void updateStepperMinPos(long newMinstop);
  void updateStepperMaxPos(long newMaxstop);
};

enum class PedalSystemAction
{
  NONE,
  RESET_PEDAL_POSITION,//not in use
  PEDAL_RESTART,
  ENABLE_OTA,//not in use
  ENABLE_PAIRING,//not in use
  ESP_BOOT_INTO_DOWNLOAD_MODE,
  PRINT_PEDAL_INFO
};
enum class RudderAction
{
  NONE,
  RUDDER_THROTTLE_AND_BRAKE,
  RUDDER_CLEAR_RUDDER_STATUS,
  RUDDER_THROTTLE_AND_CLUTCH,
  HELIRUDDER_THROTTLE_AND_BRAKE,
  HELIRUDDER_THROTTLE_AND_CLUTCH
};

class DAP_config_class {
public:
  // Konstruktor
  DAP_config_class();

  // Methode zum sicheren Abrufen der Konfiguration
  DAP_config_st getConfig();

  // Methode zum sicheren Setzen der Konfiguration
  void setConfig(DAP_config_st tmp);

  // Methode zum Laden der Konfiguration aus dem EEPROM
  void loadConfigFromEeprom();

  // Methode zum Speichern der Konfiguration im EEPROM
  void storeConfigToEeprom();

  //initialized config if needed
  void initializedConfig();

private:
  SemaphoreHandle_t mutex;
  DAP_config_st _config_st;
};
