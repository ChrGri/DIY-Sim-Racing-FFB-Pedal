#pragma once
#ifdef ESPNOW_Enable
static const bool IS_ESPNOW_ENABLED = true;
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "ESPNowW.h"
#include "DiyActivePedal_types.h"
#include "StepperMovementStrategy_Rudder.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

//#define ESPNow_debugg_rudder_st
//#define ESPNow_debug
#define ESPNOW_LOG_MAGIC_KEY_U8 0x99
#define ESPNOW_LOG_MAGIC_KEY_2_U8 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY_U8 0x99

uint8_t g_espMaster_au8[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
//uint8_t esp_master[] = {0xdc, 0xda, 0x0c, 0x22, 0x8f, 0xd8}; // S3
//uint8_t esp_master[] = {0x48, 0x27, 0xe2, 0x59, 0x48, 0xc0}; // S2 mini
uint8_t g_pedalMac_aau8[3][6] = {
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x32},
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x34},
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}
};
uint8_t g_broadcastMac_au8[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t g_espHost_au8[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t g_espMac_au8[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t g_recvMac_au8[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint16_t g_espNowSend_u16=0;
uint16_t g_espNowReceive_u16=0;
int32_t g_rssi_ai32[4]={0,0,0,0};//clutch, brake,throttle,bridge
//bool MAC_get=false;
bool g_espNowStatus_b =false;
bool g_espNowInitialStatus_b=false;
bool g_espNowRudderUpdate_b= false;
bool g_espNowNoDevice_b=false;
bool g_espNowConfigRequest_b=false;
bool g_espNowRestart_b=false;
bool g_espNowOtaEnable_b=false;
uint8_t g_espNowErrorCode_u8=0;
bool g_espNowPairingStatus_b = false;
bool g_updatePairingToEeprom_b = false;
bool g_espNowPairingAction_b = false;
bool g_softwarePairingAction_b = false;
bool g_hardwarePairingAction_b = false;
bool g_otaUpdateAction_b=false;
bool g_configUpdate_b=false;
volatile bool g_rudderInitializing_b = false;
volatile bool g_rudderDeinitializing_b = false;
volatile bool g_heliRudderInitializing_b = false;
volatile bool g_heliRudderDeinitializing_b = false;
bool g_espNowBootIntoDownloadMode_b = false;
bool g_getRudderAction_b=false;
bool g_getHeliRudderAction_b=false;
bool g_printPedalInfo_b=false;
bool g_configUpdateBuzzer_b = false;
bool g_assignmentUpdateBuzzer_b = false;
bool g_assignmentUpdate_b = false;
bool g_assignmentClear_b = false;
bool g_deviceIdStructChecker_b = false;
unsigned long g_rudderInitializedTime_u32=0;
DapAssignmentReg_t g_dapAssignmentReg_st;
DapRudder_t g_dapRudderReceiving_st;
DapRudder_t g_dapRudderSending_st;
extern QueueHandle_t s_servoConfigRxQueue;

volatile uint32_t g_lastEspnowRecvTime_u32 = 0;
volatile bool g_isEspnowConnected_b = false;
volatile uint32_t g_lastEspnowSendTime_u32 = 0;
volatile uint32_t g_lastEspnowOnSentTime_u32 = 0;
volatile uint32_t g_lastPartnerTimestamp_ms = 0;
volatile uint8_t g_currentSyncDelay_ms = 0;

/*
struct ESPNow_Send_Struct
{
  uint16_t pedal_position;
  float pedal_position_ratio;
};
*/

typedef struct DAP_Joystick_Message 
{
  uint8_t payloadtype;
  uint64_t cycleCnt_u64;
  int64_t timeSinceBoot_i64;
	int32_t controllerValue_i32;
  int8_t pedal_status; //0=default, 1=rudder, 2=rudder brake
} DAP_Joystick_Message;

typedef struct EspPairingReg_t
{
  uint8_t pairStatus_au8[4];
  uint8_t pairMac_aau8[4][6];
} EspPairingReg_t;
// Create a struct_message called myData
DAP_Joystick_Message _dap_joystick_message;

//ESPNow_Send_Struct _ESPNow_Recv;
//ESPNow_Send_Struct _ESPNow_Send;
EspPairingReg_t g_espPairingReg_st;

inline bool macCheck(const uint8_t* Mac_A, const uint8_t* Mac_B)
{
  return memcmp(Mac_A, Mac_B, 6) == 0;
}


void ESPNow_Joystick_Broadcast(int32_t controllerValue)
{
  _dap_joystick_message.payloadtype=DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK_U8;
  _dap_joystick_message.cycleCnt_u64++;
  _dap_joystick_message.timeSinceBoot_i64 = esp_timer_get_time() / 1000;
  _dap_joystick_message.controllerValue_i32 = controllerValue;
  if(dap_calculationVariables_st.rudderStatus_b)
  {
    if(dap_calculationVariables_st.rudderBrakeStatus_b)
    {
      _dap_joystick_message.pedal_status=2;
    }
    else
    {
      _dap_joystick_message.pedal_status=1;
    }
  }
  else
  {
    _dap_joystick_message.pedal_status=0;
  }
  g_lastEspnowSendTime_u32 = millis();
  esp_now_send(g_broadcastMac_au8, (uint8_t *) &_dap_joystick_message, sizeof(_dap_joystick_message));

  
  
  //esp_now_send(esp_master, (uint8_t *) &myData, sizeof(myData));
  /*
  if (result != ESP_OK) 
  {
    g_espNowNoDevice_b=true;
    //ActiveSerial->println("Failed send data to ESP_Master");
  }
  else
  {
    g_espNowNoDevice_b=false;
  }
  */
  
  /*if (result == ESP_OK)
  {
    ActiveSerial->println("Sent with success");
  }
  else
  {
    ActiveSerial->println("Error sending the data");
  }*/
}
void espNowPairingCallback(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{

  if(data_len==sizeof(DapEspPairing_t))
  {
    memcpy(&dap_esppairing_st, data , sizeof(DapEspPairing_t));
    //pedal reg
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==0||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==1||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==2)
    {
      memcpy(&g_espPairingReg_st.pairMac_aau8[dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8], mac_addr , 6);
      g_espPairingReg_st.pairStatus_au8[dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8]=1;
      g_updatePairingToEeprom_b = true;
    }
    //bridge and analog device, for pedal, only save for bridge
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==99/*||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==98*/)
    {
      memcpy(&g_espPairingReg_st.pairMac_aau8[3], mac_addr , 6);
      g_espPairingReg_st.pairStatus_au8[3]=1;
      g_updatePairingToEeprom_b = true;
    }
  }


}

void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) 
{
  if(esp_now_info->src_addr==NULL || data==NULL || data_len<=0)
  {
    return;
  }
  
  if (esp_now_info->rx_ctrl != NULL)
  {
    for (int i = 0; i < 3; i++)
    {
      if (macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[i]))
      {
        g_rssi_ai32[i] = esp_now_info->rx_ctrl->rssi;
        break; // Match found, exit loop
      }
    }
    // Also check host
    if (macCheck((uint8_t*)esp_now_info->src_addr, g_espHost_au8))
    {
      g_rssi_ai32[3] = esp_now_info->rx_ctrl->rssi;
    }
  }

  g_lastEspnowRecvTime_u32 = millis();
  g_isEspnowConnected_b = true;
  //uint8_t mac_addr[6]={0};
  DapConfig_t dap_config_espnow_recv_st;
  
  global_dap_config_class.getConfig(&dap_config_espnow_recv_st, 500);

  /*
  if(g_espNowStatus_b)
  {
    memcpy(&g_espNowReceive_u16, data, sizeof(g_espNowReceive_u16));
    ESPNow_update=true;
  }
  */
  //only get mac in pairing
  if(g_espNowPairingAction_b)
  {
    espNowPairingCallback(esp_now_info->src_addr, data, data_len);
  }
  if(g_espNowStatus_b)
  {
    //rudder message
    bool isRudderSender = macCheck(g_recvMac_au8, (uint8_t *)esp_now_info->src_addr) ||
                          macCheck(g_pedalMac_aau8[0], (uint8_t *)esp_now_info->src_addr) ||
                          macCheck(g_pedalMac_aau8[1], (uint8_t *)esp_now_info->src_addr) ||
                          macCheck(g_pedalMac_aau8[2], (uint8_t *)esp_now_info->src_addr);
    if(isRudderSender)
    {
      if(data_len==sizeof(DapRudder_t))
      {

        bool structChecker = true;
        uint16_t crc;
        DapRudder_t dapg_rudder_st_st_local;
        memcpy(&dapg_rudder_st_st_local, data, sizeof(DapRudder_t));
        // check if data is plausible  
        if ( dapg_rudder_st_st_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ESPNOW_RUDDER_U8 )
        {
          structChecker = false;
        }  
        if ( dapg_rudder_st_st_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8 )
        {
          structChecker = false;
        }
        // checksum validation
        crc = checksumCalculator_u16((uint8_t*)(&(dapg_rudder_st_st_local.payloadHeader_st)), sizeof(dapg_rudder_st_st_local.payloadHeader_st) + sizeof(dapg_rudder_st_st_local.payloadRudderState_st));
        if (crc != dapg_rudder_st_st_local.payloadFooter_st.checkSum_u16)
        {
          structChecker = false;
        }
        // if checks are successfull, overwrite global configuration struct
        if (structChecker == true)
        {
          memcpy(&g_dapRudderReceiving_st, data, sizeof(DapRudder_t));
          g_espNowRudderUpdate_b=true;

          // Lock onto partner pedal's MAC for unicast
          memcpy(g_recvMac_au8, esp_now_info->src_addr, 6);
          if (!esp_now_is_peer_exist(g_recvMac_au8)) {
            ESPNow.add_peer(g_recvMac_au8);
          }

          // 1. Immediate zero-latency update to calculation variables for 4000 Hz physics loop
          dap_calculationVariables_st.syncPedalPosition_u32 = dapg_rudder_st_st_local.payloadRudderState_st.pedalPosition_u16;
          dap_calculationVariables_st.syncPedalPositionRatio_fl32 = dapg_rudder_st_st_local.payloadRudderState_st.pedalPositionRatio_fl32;
          dap_calculationVariables_st.syncPedalForce_N_fl32 = dapg_rudder_st_st_local.payloadRudderState_st.pedalForce_N_fl32;

          // 2. RTT and Latency computation
          uint32_t incomingSendTime = dapg_rudder_st_st_local.payloadRudderState_st.sendTimestamp_ms;
          uint32_t incomingEchoTime = dapg_rudder_st_st_local.payloadRudderState_st.echoTimestamp_ms;
          g_lastPartnerTimestamp_ms = incomingSendTime;

          if (incomingEchoTime > 0) {
            uint32_t now_ms = millis();
            if (now_ms >= incomingEchoTime) {
              uint32_t rtt = now_ms - incomingEchoTime;
              if (rtt < 255) {
                g_currentSyncDelay_ms = (uint8_t)(rtt / 2); // One-way wireless delay in ms
              }
            }
          }
        }

      }
    }
    if(macCheck(g_espHost_au8,(uint8_t *)esp_now_info->src_addr))
    {
      
      if (data_len == sizeof(DapConfig_t))
      {
        if (esp_now_info->src_addr[5] == g_espHost_au8[5])
        {
          // ActiveSerial->println("dap_config_st ESPNow recieved");

          bool structChecker = true;
          uint16_t crc;
          DapConfig_t *dap_config_st_local_ptr;
          dap_config_st_local_ptr = &dap_config_espnow_recv_st;
          // ActiveSerial->readBytes((char*)dap_config_st_local_ptr, sizeof(DapConfig_t));
          memcpy(dap_config_st_local_ptr, data, sizeof(DapConfig_t));

          // check if data is plausible
          if (dap_config_espnow_recv_st.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_CONFIG_U8)
          {
            structChecker = false;
            g_espNowErrorCode_u8 = 101;
          }
          if (dap_config_espnow_recv_st.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
          {
            structChecker = false;
            if (g_espNowErrorCode_u8 == 0)
            {
              g_espNowErrorCode_u8 = 102;
            }
          }
          // checksum validation
          crc = checksumCalculator_u16((uint8_t *)(&(dap_config_espnow_recv_st.payloadHeader_st)), sizeof(dap_config_espnow_recv_st.payloadHeader_st) + sizeof(dap_config_espnow_recv_st.payloadPedalConfig_st));
          if (crc != dap_config_espnow_recv_st.payloadFooter_st.checkSum_u16)
          {
            structChecker = false;
            if (g_espNowErrorCode_u8 == 0)
            {
              g_espNowErrorCode_u8 = 103;
            }
          }

          // if checks are successfull, overwrite global configuration struct
          if (structChecker == true)
          {
            // ActiveSerial->println("Updating pedal config");
            configDataPackage_t configPackage_st;
            configPackage_st.config_st = dap_config_espnow_recv_st;
            xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
            //global_dap_config_class.setConfig(dap_config_espnow_recv_st);
            if(dap_config_espnow_recv_st.payloadHeader_st.storeToEeprom_u8==1)
            {
              g_configUpdateBuzzer_b = true;
            }            

          }
        }
      }

      DapActions_t dap_actions_st;
      if(data_len==sizeof(dap_actions_st))
      {
        //ActiveSerial->print(" get action");
        memcpy(&dap_actions_st, data, sizeof(DapActions_t));
        // ActiveSerial->readBytes((char*)&dap_actions_st, sizeof(DapActions_t));
        bool commandForAssignment_b = false;
        if(dap_actions_st.payloadHeader_st.pedalTag_u8 == PEDAL_ID_TEMP_1 || dap_actions_st.payloadHeader_st.pedalTag_u8 == PEDAL_ID_TEMP_2 ||dap_actions_st.payloadHeader_st.pedalTag_u8 == PEDAL_ID_TEMP_3)
        {
          commandForAssignment_b = true;
        }

        if (dap_actions_st.payloadHeader_st.pedalTag_u8 == dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 || commandForAssignment_b)
        {
          bool structChecker = true;
          uint16_t crc;
          if (dap_actions_st.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ACTION_U8)
          {
            structChecker = false;
            if (g_espNowErrorCode_u8 == 0)
            {
              g_espNowErrorCode_u8 = 111;
            }
          }
          if (dap_actions_st.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
          {
            structChecker = false;
            if (g_espNowErrorCode_u8 == 0)
            {
              g_espNowErrorCode_u8 = 112;
            }
          }
          crc = checksumCalculator_u16((uint8_t *)(&(dap_actions_st.payloadHeader_st)), sizeof(dap_actions_st.payloadHeader_st) + sizeof(dap_actions_st.payloadPedalAction_st));
          if (crc != dap_actions_st.payloadFooter_st.checkSum_u16)
          {
            structChecker = false;
            if (g_espNowErrorCode_u8 == 0)
            {
              g_espNowErrorCode_u8 = 113;
            }
          }

          if (structChecker == true)
          {

            // 2= restart pedal
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::PEDAL_RESTART)
            {
              g_espNowRestart_b = true;
            }
            // 3= Wifi OTA
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ENABLE_OTA)
            {
              g_espNowOtaEnable_b = true;
            }
            // 5= Boot into download mode
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
            {
              g_espNowBootIntoDownloadMode_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
            {
              g_printPedalInfo_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::WAKEUP_PEDAL)
            {
              if (g_pedalOperationalState_u8 == (uint8_t)PEDAL_STATE_STANDBY_WAITING_FOR_WAKEUP_E)
              {
                g_pedalOperationalState_u8 = (uint8_t)PEDAL_STATE_HOMING_E;
              }
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0 && commandForAssignment_b)
            {
              g_dapAssignmentReg_st.deviceId_u8 = PEDAL_ID_CLUTCH;
              g_assignmentUpdate_b = true;
              g_assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1 && commandForAssignment_b)
            {
              g_dapAssignmentReg_st.deviceId_u8 = PEDAL_ID_BRAKE;
              g_assignmentUpdate_b = true;
              g_assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2 && commandForAssignment_b)
            {
              g_dapAssignmentReg_st.deviceId_u8 = PEDAL_ID_THROTTLE;
              g_assignmentUpdate_b = true;
              g_assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP)
            {
              g_assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT && !commandForAssignment_b)
            {
              g_assignmentClear_b = true;
            }
            // trigger ABS effect
            if (dap_actions_st.payloadPedalAction_st.triggerAbs_u8 > 0)
            {
              absOscillation.trigger();
              if (dap_actions_st.payloadPedalAction_st.triggerAbs_u8 > 1)
              {
                dap_calculationVariables_st.trackCondition_u8 = dap_actions_st.payloadPedalAction_st.triggerAbs_u8 - 1;
              }
              else
              {
                dap_calculationVariables_st.trackCondition_u8 = dap_actions_st.payloadPedalAction_st.triggerAbs_u8 = 0;
              }
            }
            // RPM effect
              g_rpmOscillation_st.rpmValue_fl32 = dap_actions_st.payloadPedalAction_st.rpm_u8;
            // G force effect
            g_gForceEffect_st.gValue_fl32 = dap_actions_st.payloadPedalAction_st.gValue_u8 - 128;
            // wheel slip
            if (dap_actions_st.payloadPedalAction_st.wheelSlip_u8)
            {
              g_wsOscillation_st.trigger();
            }
            // Road impact && Rudder_t G impact
            if (dap_calculationVariables_st.rudderStatus_b == false)
            {
              g_roadImpactEffect_st.roadImpactValue_u8 = dap_actions_st.payloadPedalAction_st.impactValue_u8;
            }
            else
            {
              g_rudderGForce_st.gValue_u8 = dap_actions_st.payloadPedalAction_st.impactValue_u8;
            }
            // trigger system identification
            // if (dap_actions_st.payloadPedalAction_st.startSystemIdentification_u8)
            // {
            //   systemIdentificationMode_b = true;
            // }
            // trigger Custom effect effect 1
            if (dap_actions_st.payloadPedalAction_st.triggerCv1_u8) g_customVibration1_st.trigger();
            // trigger Custom effect effect 2
            if (dap_actions_st.payloadPedalAction_st.triggerCv2_u8) g_customVibration2_st.trigger();
            // trigger Custom effect effect 3
            if (dap_actions_st.payloadPedalAction_st.triggerCv3_u8) g_customVibration3_st.trigger();
            // trigger Custom effect effect 4
            if (dap_actions_st.payloadPedalAction_st.triggerCv4_u8) g_customVibration4_st.trigger();
            // trigger return pedal position
            if (dap_actions_st.payloadPedalAction_st.returnPedalConfig_u8)
            {
              g_espNowConfigRequest_b = true;
              /*
              DapConfig_t * dap_config_st_local_ptr;
              dap_config_st_local_ptr = &dap_config_st;
              //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payloadHeader_st)), sizeof(dap_config_st.payloadHeader_st) + sizeof(dap_config_st.payloadPedalConfig_st));
              crc = checksumCalculator((uint8_t*)(&(dap_config_st.payloadHeader_st)), sizeof(dap_config_st.payloadHeader_st) + sizeof(dap_config_st.payloadPedalConfig_st));
              dap_config_st_local_ptr->payloadFooter_st.checkSum_u16 = crc;
              ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DapConfig_t));
              ActiveSerial->print("\r\n");
              */
            }
            uint8_t rudderAct = dap_actions_st.payloadPedalAction_st.rudderAction_u8;
            if (rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_BRAKE || 
                rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
            {
              g_getRudderAction_b = true;
              if (rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 == 2)
                {
                  // Recv_mac=Clu_mac;
                  memcpy(g_recvMac_au8, g_pedalMac_aau8[0], 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.rudderStatus_b == false)
              {
                dap_calculationVariables_st.rudderStatus_b = true;
                dap_calculationVariables_st.helicopterRudderStatus_b = false;
                // ActiveSerial->println("Rudder_t on");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.rudderStatus_b = false;
                dap_calculationVariables_st.helicopterRudderStatus_b = false;
                moveSlowlyToPosition_b = true;
                ResetRudderStrategyState();
                // ActiveSerial->println("Rudder_t off");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
            else if (rudderAct == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_BRAKE || 
                     rudderAct == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
            {
              g_getHeliRudderAction_b = true;
              if (rudderAct == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 == 2)
                {
                  memcpy(g_recvMac_au8, g_pedalMac_aau8[0], 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.helicopterRudderStatus_b == false)
              {
                dap_calculationVariables_st.helicopterRudderStatus_b = true;
                dap_calculationVariables_st.rudderStatus_b = false;
                // ActiveSerial->println("Rudder_t on");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.helicopterRudderStatus_b = false;
                dap_calculationVariables_st.rudderStatus_b = false;
                moveSlowlyToPosition_b = true;
                ResetRudderStrategyState();
                // ActiveSerial->println("Rudder_t off");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
            else if (rudderAct == (uint8_t)RudderAction::RUDDER_CLEAR_RUDDER_STATUS)
            {
              dap_calculationVariables_st.rudderStatus_b = false;
              dap_calculationVariables_st.helicopterRudderStatus_b = false;
              dap_calculationVariables_st.rudderBrakeStatus_b = false;
              moveSlowlyToPosition_b = true;
              ResetRudderStrategyState();
              // ActiveSerial->println("Rudder_t Status Clear");
            }

            if (dap_actions_st.payloadPedalAction_st.rudderBrakeAction_u8 == 1)
            {
              g_getRudderAction_b = true;
              if (dap_calculationVariables_st.rudderBrakeStatus_b == false && 
                  (dap_calculationVariables_st.rudderStatus_b == true || dap_calculationVariables_st.helicopterRudderStatus_b == true))
              {
                dap_calculationVariables_st.rudderBrakeStatus_b = true;
                // ActiveSerial->println("Rudder_t brake on");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.rudderBrakeStatus_b = false;
                // ActiveSerial->println("Rudder_t brake off");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
          }
        }
      }
      if(data_len==sizeof(DapActionOta_t))
      {
        memcpy(&dap_action_ota_st, data, sizeof(DapActionOta_t));
        g_otaUpdateAction_b=true;
      }
      
      if(data_len==sizeof(DAP_servo_config_st))
      {
        DAP_servo_config_st received_servo_config;
        memcpy(&received_servo_config, data, sizeof(DAP_servo_config_st));
        
        bool structChecker = true;
        if (received_servo_config.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8) structChecker = false;
        if (received_servo_config.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8) structChecker = false;
        
        uint16_t crc = checksumCalculator_u16((uint8_t *)(&(received_servo_config.payloadHeader_st)), sizeof(received_servo_config.payloadHeader_st) + sizeof(received_servo_config.payloadServoConfig_st));
        if (crc != received_servo_config.payloadFooter_st.checkSum_u16) structChecker = false;
        
        if (structChecker == true)
        {
          if (s_servoConfigRxQueue != NULL)
          {
            xQueueSend(s_servoConfigRxQueue, &received_servo_config, (TickType_t)0);
          }
        }
      }

    }

    

  }

}
void onSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    g_lastEspnowOnSentTime_u32 = millis();
}

inline bool isEspnowBusy()
{
    uint32_t latency = millis() - g_lastEspnowSendTime_u32;
    uint32_t onSentAgo = millis() - g_lastEspnowOnSentTime_u32;
    // If last send is more recent than last onSent
    if (latency < onSentAgo)
    {
        // 50ms timeout to prevent permanent lockup
        if (latency < 50) return true;
    }
    return false;
}

inline uint32_t getEspnowSendLatency()
{
    uint32_t latency = millis() - g_lastEspnowSendTime_u32;
    uint32_t onSentAgo = millis() - g_lastEspnowOnSentTime_u32;
    if (latency < onSentAgo)
    {
        return latency;
    }
    return 0;
}

inline bool checkEspnowConnection()
{
    if (millis() - g_lastEspnowRecvTime_u32 > 1000)
    {
        g_isEspnowConnected_b = false;
    }
    return g_isEspnowConnected_b;
}


void espNowInitialize()
{
  DapConfig_t dap_config_espnow_init_st;
  global_dap_config_class.getConfig(&dap_config_espnow_init_st, 500);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setSleep(false);
  delay(1000);
  ActiveSerial->println("Initializing Wifi, please wait");
  // ActiveSerial->print("Current MAC Address:  ");
  // ActiveSerial->println(WiFi.macAddress());
  WiFi.macAddress(g_espMac_au8);
  ActiveSerial->printf("Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espMac_au8[0], g_espMac_au8[1], g_espMac_au8[2], g_espMac_au8[3], g_espMac_au8[4], g_espMac_au8[5]);
  #ifndef ESPNow_Pairing_function
    switch (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8)
    {
    case PEDAL_ID_CLUTCH:
      esp_wifi_set_mac(WIFI_IF_STA, &g_pedalMac_aau8[0][0]);
      break;
    case PEDAL_ID_BRAKE:
      esp_wifi_set_mac(WIFI_IF_STA, &g_pedalMac_aau8[1][0]);
      break;  
    case PEDAL_ID_THROTTLE:
      esp_wifi_set_mac(WIFI_IF_STA, &g_pedalMac_aau8[2][0]);
      break;         
    default:
      ActiveSerial->println("Mac address overwrite failed, no pedal role assignment.");
      break;
    }
    delay(300);
    ActiveSerial->print("Overwrite MAC Address:  ");
    ActiveSerial->println(WiFi.macAddress());
  #endif
  ActiveSerial->println("Initializing ESP-NOW");
  ESPNow.init();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  delay(3000);
  #ifdef ESPNow_S3
    #ifdef LOWER_WIFI_TRANSMISSION_POWER
      esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
      esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    #endif
  #endif
  #ifdef ESPNow_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_LGI);
    // esp_wifi_config_espnow_rate(WIFI_IF_STA, 	WIFI_PHY_RATE_54M);
  #endif
  #ifdef ESPNow_Pairing_function
    EspPairingReg_t ESP_pairing_reg_local;
    EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
    memcpy(&g_espPairingReg_st, &ESP_pairing_reg_local, sizeof(EspPairingReg_t));
    //g_espPairingReg_st=ESP_pairing_reg_local;
    // EEPROM.get(EEPROM_offset, g_espPairingReg_st);
    for (int i = 0; i < 4; i++)
    {
      if (g_espPairingReg_st.pairStatus_au8[i] == 1)
      {
        ActiveSerial->print("Paired Device #");
        ActiveSerial->print(i);
        // ActiveSerial->print(" Pair: ");
        // ActiveSerial->print(g_espPairingReg_st.pairStatus_au8[i]);
        ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espPairingReg_st.pairMac_aau8[i][0], g_espPairingReg_st.pairMac_aau8[i][1], g_espPairingReg_st.pairMac_aau8[i][2], g_espPairingReg_st.pairMac_aau8[i][3], g_espPairingReg_st.pairMac_aau8[i][4], g_espPairingReg_st.pairMac_aau8[i][5]);
      }
    }
    for (int i = 0; i < 4; i++)
    {
      if (g_espPairingReg_st.pairStatus_au8[i] == 1)
      {
        if (i == 0)
        {
          memcpy(&g_pedalMac_aau8[0], &g_espPairingReg_st.pairMac_aau8[i], 6);
        }
        if (i == 1)
        {
          memcpy(&g_pedalMac_aau8[1], &g_espPairingReg_st.pairMac_aau8[i], 6);
        }
        if (i == 2)
        {
          memcpy(&g_pedalMac_aau8[2], &g_espPairingReg_st.pairMac_aau8[i], 6);
        }
        if (i == 3)
        {
          memcpy(&g_espHost_au8, &g_espPairingReg_st.pairMac_aau8[i], 6);
        }
      }
    }
    #endif

    if (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_BRAKE || dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_CLUTCH)
    {
      memcpy(g_recvMac_au8, g_pedalMac_aau8[2], 6);
      ESPNow.add_peer(g_recvMac_au8);
    }

    if (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_THROTTLE)
    {
      memcpy(g_recvMac_au8, g_pedalMac_aau8[1], 6);
      ESPNow.add_peer(g_pedalMac_aau8[1]);
      ESPNow.add_peer(g_pedalMac_aau8[0]);
    }
    bool peerAddingChecker=true;
    if(ESPNow.add_peer(g_espMaster_au8)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(g_broadcastMac_au8)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(g_espHost_au8)!= ESP_OK) peerAddingChecker=false;
    if(peerAddingChecker) ActiveSerial->println("Sucess to add peers");

    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(onSent);
    //rssi calculate
    g_espNowInitialStatus_b=true;
    g_espNowStatus_b=true;
    ActiveSerial->println("ESPNow Initialized");
  
}

void sendESPNOWLog(const char *log,...)
{
  uint8_t buffer[250];
  uint8_t payloadType = DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
  //uint8_t logLen = strlen(log); 
  va_list args;
  char* result = NULL;
  int needed_size;
  va_start(args, log); // initialized va_list
  needed_size = vsnprintf(NULL, 0, log, args);
  va_end(args); 
  if (needed_size < 0) return;
  result = (char*)malloc(needed_size + 1);
  // malloc error
  if (result == NULL) return;
  va_start(args, log); 
  vsnprintf(result, needed_size + 1, log, args);
  va_end(args); 
  int logLen=strlen(result);
  if (logLen > 240) logLen = 240;
  buffer[0] = payloadType;
  buffer[1] = ESPNOW_LOG_MAGIC_KEY_U8;
  buffer[2] = ESPNOW_LOG_MAGIC_KEY_2_U8;
  buffer[3] = logLen;
  memcpy(&buffer[4], result, logLen);
  g_lastEspnowSendTime_u32 = millis();
  ESPNow.send_message(g_broadcastMac_au8, (uint8_t *)buffer, 4 + logLen);
  free(result);
}

void softwareAssignmentInitialize()
{
  DapAssignmentReg_t dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg_local);
  bool structChecker= true;
  uint16_t crc = checksumCalculator_u16((uint8_t *)(&dap_assignement_reg_local), sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
  if(dap_assignement_reg_local.payloadType_u8 != DAP_PAYLOAD_TYPE_ASSIGNMENT_U8) structChecker = false;
  if(dap_assignement_reg_local.magicKey_u8 != ESPNOW_ASSIGNMENT_MAGIC_KEY_U8) structChecker = false;
  if(crc != dap_assignement_reg_local.crc_u16) structChecker = false;
  if(dap_assignement_reg_local.crc_u16 != crc) structChecker = false;
  DapConfig_t tmp;
  global_dap_config_class.getConfig(&tmp, 500);
  if(structChecker) 
  {
    memcpy(&g_dapAssignmentReg_st, &dap_assignement_reg_local, sizeof(DapAssignmentReg_t));
    g_deviceIdStructChecker_b = true;
    ActiveSerial->print("Overwritting pedal assignment: ");
    ActiveSerial->println(dap_assignement_reg_local.deviceId_u8);

    if (g_dapAssignmentReg_st.deviceId_u8 == PEDAL_ID_CLUTCH || g_dapAssignmentReg_st.deviceId_u8 == PEDAL_ID_BRAKE || g_dapAssignmentReg_st.deviceId_u8 == PEDAL_ID_THROTTLE)
    {
      tmp.payloadPedalConfig_st.pedalType_u8 = g_dapAssignmentReg_st.deviceId_u8;
    }
    else
    {
      tmp.payloadPedalConfig_st.pedalType_u8 = PEDAL_ID_UNKNOWN;
    }
      
  }
  else
  {
    tmp.payloadPedalConfig_st.pedalType_u8 = PEDAL_ID_UNKNOWN;
    ActiveSerial->println("Assignment error:");
    ActiveSerial->print("Payload type expect:");
    ActiveSerial->print(DAP_PAYLOAD_TYPE_ASSIGNMENT_U8);
    ActiveSerial->print(" Payload type get:");
    ActiveSerial->println(dap_assignement_reg_local.payloadType_u8);
    ActiveSerial->print("Magic key expect:");
    ActiveSerial->print(ESPNOW_ASSIGNMENT_MAGIC_KEY_U8);
    ActiveSerial->print(" Magic key get:");
    ActiveSerial->println(dap_assignement_reg_local.magicKey_u8);
    ActiveSerial->print("crc expect:");
    ActiveSerial->print(crc);
    ActiveSerial->print(" crc get:");
    ActiveSerial->println(dap_assignement_reg_local.crc_u16);
    ActiveSerial->print("Pedal ID get:");
    ActiveSerial->println(dap_assignement_reg_local.deviceId_u8);
  }
  configDataPackage_t configPackage_st;
  configPackage_st.config_st = tmp;
  xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
  delay(1000); // delay for writting config into global
}

void writeAssignmentToEeprom()
{
  ActiveSerial->println("Writting assignment to eeprom.");
  g_dapAssignmentReg_st.magicKey_u8 = ESPNOW_ASSIGNMENT_MAGIC_KEY_U8;
  g_dapAssignmentReg_st.payloadType_u8 = DAP_PAYLOAD_TYPE_ASSIGNMENT_U8;
  //refill the crc
  g_dapAssignmentReg_st.crc_u16 = checksumCalculator_u16((uint8_t *)(&g_dapAssignmentReg_st), sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET_U32, g_dapAssignmentReg_st);
  EEPROM.commit();
  delay(1000);
  //check the data inside of eeprom
  DapAssignmentReg_t dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg_local);
  //list those assignment
  ActiveSerial->println("check the assignment in eeprom");
  ActiveSerial->print("Assignment expected:");
  ActiveSerial->print(g_dapAssignmentReg_st.deviceId_u8);
  ActiveSerial->print(" Assignment get:");
  ActiveSerial->println(dap_assignement_reg_local.deviceId_u8);
  ActiveSerial->print("crc expected:");
  ActiveSerial->print(g_dapAssignmentReg_st.crc_u16);
  ActiveSerial->print(" crc get:");
  ActiveSerial->println(dap_assignement_reg_local.crc_u16);
  
}
void clearAssignmentToEeprom()
{
  ActiveSerial->println("clear assignment from eeprom.");
  g_dapAssignmentReg_st.magicKey_u8 = 0;
  g_dapAssignmentReg_st.payloadType_u8 = 0;
  g_dapAssignmentReg_st.deviceId_u8 = 99;
  // refill the crc
  g_dapAssignmentReg_st.crc_u16 = 0;
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET_U32, g_dapAssignmentReg_st);
  EEPROM.commit();
  delay(1000);
}
#else
static const bool IS_ESPNOW_ENABLED = false;
static uint8_t g_espNowErrorCode_u8 = 0;
static bool g_espNowPairingAction_b = false;
static bool g_updatePairingToEeprom_b = false;
static bool g_hardwarePairingAction_b = false;
static bool g_espNowRudderUpdate_b = false;
static bool g_espNowRestart_b = false;
static bool g_espNowOtaEnable_b = false;
static bool g_printPedalInfo_b = false;
static bool g_assignmentUpdate_b = false;
static bool g_assignmentClear_b = false;
static bool g_configUpdateBuzzer_b = false;
static bool g_assignmentUpdateBuzzer_b = false;
static bool g_deviceIdStructChecker_b = false;
static unsigned long g_rudderInitializedTime_u32 = 0;
static bool g_isEspnowConnected_b = false;
static uint32_t g_lastEspnowRecvTime_u32 = 0;
static uint32_t g_lastEspnowSendTime_u32 = 0;
static uint32_t g_lastEspnowOnSentTime_u32 = 0;
static volatile bool g_rudderInitializing_b = false;
static volatile bool g_rudderDeinitializing_b = false;
static volatile bool g_heliRudderInitializing_b = false;
static volatile bool g_heliRudderDeinitializing_b = false;
static bool g_espNowBootIntoDownloadMode_b = false;
static bool g_getRudderAction_b = false;
static bool g_getHeliRudderAction_b = false;

typedef struct EspPairingReg_t {
  uint8_t pairStatus_au8[4];
  uint8_t pairMac_aau8[4][6];
} EspPairingReg_t;
static EspPairingReg_t g_espPairingReg_st;

inline void espNowInitialize() {}
inline void sendESPNOWLog(const char *log,...) {}
inline void ESPNow_Joystick_Broadcast(int32_t controllerValue) {}
inline void writeAssignmentToEeprom() {}
inline void clearAssignmentToEeprom() {}
inline void softwareAssignmentInitialize() {}
#endif
