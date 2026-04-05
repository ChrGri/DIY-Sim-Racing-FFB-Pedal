#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "ESPNowW.h"
#include "DiyActivePedal_types.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

//#define ESPNow_debug_rudder
//#define ESPNow_debug
#define ESPNOW_LOG_MAGIC_KEY 0x99
#define ESPNOW_LOG_MAGIC_KEY_2 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY 0x99

uint8_t g_esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
//uint8_t esp_master[] = {0xdc, 0xda, 0x0c, 0x22, 0x8f, 0xd8}; // S3
//uint8_t esp_master[] = {0x48, 0x27, 0xe2, 0x59, 0x48, 0xc0}; // S2 mini
uint8_t g_Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t g_Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t g_Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};
uint8_t g_broadcast_mac[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t g_esp_Host[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t g_esp_Mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t g_Recv_mac[]={0};
uint16_t g_ESPNow_send=0;
uint16_t g_ESPNow_recieve=0;
int32_t g_rssi[4]={0,0,0,0};//clutch, brake,throttle,bridge
//bool MAC_get=false;
bool g_ESPNOW_status =false;
bool g_ESPNow_initial_status=false;
bool g_ESPNow_Rudder_Update= false;
bool g_ESPNow_no_device=false;
bool g_ESPNow_config_request=false;
bool g_ESPNow_restart=false;
bool g_ESPNow_OTA_enable=false;
uint8_t g_ESPNow_error_code=0;
bool g_ESPNow_Pairing_status = false;
bool g_UpdatePairingToEeprom = false;
bool ESPNow_pairing_action_b = false;
bool software_pairing_action_b = false;
bool hardware_pairing_action_b = false;
bool OTA_update_action_b=false;
bool Config_update_b=false;
bool Rudder_initializing = false;
bool Rudder_deinitializing = false;
bool HeliRudder_initializing = false;
bool HeliRudder_deinitializing = false;
bool ESPNOW_BootIntoDownloadMode = false;
bool Get_Rudder_action_b=false;
bool Get_HeliRudder_action_b=false;
bool printPedalInfo_b=false;
bool Config_update_Buzzer_b = false;
bool assignmentUpdateBuzzer_b = false;
bool assignmentUpdate_b = false;
bool assignmentClear_b = false;
bool deviceIdStructChecker = false;
unsigned long Rudder_initialized_time=0;
DapAssignmentReg_t dap_assignement_reg;
DapRudder_t dap_rudder_receiving;
DapRudder_t dap_rudder_sending;
/*
struct ESPNow_Send_Struct
{ 
  uint16_t pedal_position;
  float pedal_position_ratio;
};
*/

typedef struct DAP_Joystick_Message {
  uint8_t payloadtype;
  uint64_t cycleCnt_u64;
  int64_t timeSinceBoot_i64;
	int32_t controllerValue_i32;
  int8_t pedal_status; //0=default, 1=rudder, 2=rudder brake
} DAP_Joystick_Message;

typedef struct ESP_pairing_reg
{
  uint8_t Pair_status[4];
  uint8_t Pair_mac[4][6];
} ESP_pairing_reg;
// Create a struct_message called myData
DAP_Joystick_Message _dap_joystick_message;

//ESPNow_Send_Struct _ESPNow_Recv;
//ESPNow_Send_Struct _ESPNow_Send;
ESP_pairing_reg _ESP_pairing_reg;

bool MacCheck(uint8_t* Mac_A, uint8_t*  Mac_B)
{
  uint8_t mac_i=0;
  for(mac_i=0;mac_i<6;mac_i++)
  {
    if(Mac_A[mac_i]!=Mac_B[mac_i])
    {      
      break;
    }
    else
    {
      if(mac_i==5)
      {
        return true;
      }
    }
  }
  return false;   
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
  esp_now_send(g_broadcast_mac, (uint8_t *) &_dap_joystick_message, sizeof(_dap_joystick_message));

  
  
  //esp_now_send(esp_master, (uint8_t *) &myData, sizeof(myData));
  /*
  if (result != ESP_OK) 
  {
    g_ESPNow_no_device=true;
    //ActiveSerial->println("Failed send data to ESP_Master");
  }
  else
  {
    g_ESPNow_no_device=false;
  }
  */
  
  /*if (result == ESP_OK) {
    ActiveSerial->println("Sent with success");
  }
  else {
    ActiveSerial->println("Error sending the data");
  }*/
}
void ESPNow_Pairing_callback(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{

  if(data_len==sizeof(DapEspPairing_t))
  {
    memcpy(&dap_esppairing_st, data , sizeof(DapEspPairing_t));
    //pedal reg
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==0||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==1||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==2)
    {
      memcpy(&_ESP_pairing_reg.Pair_mac[dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8], mac_addr , 6);
      _ESP_pairing_reg.Pair_status[dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8]=1;
      g_UpdatePairingToEeprom = true;
    }
    //bridge and analog device, for pedal, only save for bridge
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==99/*||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==98*/)
    {
      memcpy(&_ESP_pairing_reg.Pair_mac[3], mac_addr , 6);
      _ESP_pairing_reg.Pair_status[3]=1;
      g_UpdatePairingToEeprom = true;
    }
  }


}

void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) 
{
  if(esp_now_info->src_addr==NULL || data==NULL || data_len<=0)
  {
    return;
  }
  //uint8_t mac_addr[6]={0};
  DapConfig_t dap_config_espnow_recv_st;
  
  global_dap_config_class.getConfig(&dap_config_espnow_recv_st, 500);

  /*
  if(g_ESPNOW_status)
  {
    memcpy(&g_ESPNow_recieve, data, sizeof(g_ESPNow_recieve));
    ESPNow_update=true;
  }
  */
  //only get mac in pairing
  if(ESPNow_pairing_action_b)
  {
    ESPNow_Pairing_callback(esp_now_info->src_addr, data, data_len);
  }
  if(g_ESPNOW_status)
  {
    //rudder message
    if(MacCheck(g_Recv_mac,(uint8_t *)esp_now_info->src_addr))
    {
      if(data_len==sizeof(DapRudder_t))
      {

        bool structChecker = true;
        uint16_t crc;
        DapRudder_t dap_rudder_st_local;
        memcpy(&dap_rudder_st_local, data, sizeof(DapRudder_t));
        // check if data is plausible  
        if ( dap_rudder_st_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ESPNOW_RUDDER_U8 )
        {
          structChecker = false;
        }  
        if ( dap_rudder_st_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8 )
        {
          structChecker = false;
        }
        // checksum validation
        crc = checksumCalculator_u16((uint8_t*)(&(dap_rudder_st_local.payloadHeader_st)), sizeof(dap_rudder_st_local.payloadHeader_st) + sizeof(dap_rudder_st_local.payloadRudderState_st));
        if (crc != dap_rudder_st_local.payloadFooter_st.checkSum_u16)
        {
          structChecker = false;
        }
        // if checks are successfull, overwrite global configuration struct
        if (structChecker == true)
        {
          memcpy(&dap_rudder_receiving, data, sizeof(DapRudder_t));
          g_ESPNow_Rudder_Update=true;
        }

      }
    }
    if(MacCheck(g_esp_Host,(uint8_t *)esp_now_info->src_addr))
    {
      
      if (data_len == sizeof(DapConfig_t))
      {
        if (esp_now_info->src_addr[5] == g_esp_Host[5])
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
            g_ESPNow_error_code = 101;
          }
          if (dap_config_espnow_recv_st.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
          {
            structChecker = false;
            if (g_ESPNow_error_code == 0)
            {
              g_ESPNow_error_code = 102;
            }
          }
          // checksum validation
          crc = checksumCalculator_u16((uint8_t *)(&(dap_config_espnow_recv_st.payloadHeader_st)), sizeof(dap_config_espnow_recv_st.payloadHeader_st) + sizeof(dap_config_espnow_recv_st.payloadPedalConfig_st));
          if (crc != dap_config_espnow_recv_st.payloadFooter_st.checkSum_u16)
          {
            structChecker = false;
            if (g_ESPNow_error_code == 0)
            {
              g_ESPNow_error_code = 103;
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
              Config_update_Buzzer_b = true;
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
            if (g_ESPNow_error_code == 0)
            {
              g_ESPNow_error_code = 111;
            }
          }
          if (dap_actions_st.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
          {
            structChecker = false;
            if (g_ESPNow_error_code == 0)
            {
              g_ESPNow_error_code = 112;
            }
          }
          crc = checksumCalculator_u16((uint8_t *)(&(dap_actions_st.payloadHeader_st)), sizeof(dap_actions_st.payloadHeader_st) + sizeof(dap_actions_st.payloadPedalAction_st));
          if (crc != dap_actions_st.payloadFooter_st.checkSum_u16)
          {
            structChecker = false;
            if (g_ESPNow_error_code == 0)
            {
              g_ESPNow_error_code = 113;
            }
          }

          if (structChecker == true)
          {

            // 2= restart pedal
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::PEDAL_RESTART)
            {
              g_ESPNow_restart = true;
            }
            // 3= Wifi OTA
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ENABLE_OTA)
            {
              g_ESPNow_OTA_enable = true;
            }
            // 5= Boot into download mode
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
            {
              ESPNOW_BootIntoDownloadMode = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
            {
              printPedalInfo_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceId_u8 = PEDAL_ID_CLUTCH;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceId_u8 = PEDAL_ID_BRAKE;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceId_u8 = PEDAL_ID_THROTTLE;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP)
            {
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT && !commandForAssignment_b)
            {
              assignmentClear_b = true;
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
              _RPMOscillation.rpmValue_fl32 = dap_actions_st.payloadPedalAction_st.rpm_u8;
            // G force effect
            gForceEffect_.gValue_fl32 = dap_actions_st.payloadPedalAction_st.gValue_u8 - 128;
            // wheel slip
            if (dap_actions_st.payloadPedalAction_st.wheelSlip_u8)
            {
              _WSOscillation.trigger();
            }
            // Road impact && Rudder G impact
            if (dap_calculationVariables_st.rudderStatus_b == false)
            {
              roadImpactEffect_.roadImpactValue_u8 = dap_actions_st.payloadPedalAction_st.impactValue_u8;
            }
            else
            {
              rudderGForce_.gValue_u8 = dap_actions_st.payloadPedalAction_st.impactValue_u8;
            }
            // trigger system identification
            if (dap_actions_st.payloadPedalAction_st.startSystemIdentification_u8)
            {
              systemIdentificationMode_b = true;
            }
            // trigger Custom effect effect 1
            if (dap_actions_st.payloadPedalAction_st.triggerCv1_u8) customVibration1_.trigger();
            // trigger Custom effect effect 2
            if (dap_actions_st.payloadPedalAction_st.triggerCv2_u8) customVibration2_.trigger();
            // trigger Custom effect effect 3
            if (dap_actions_st.payloadPedalAction_st.triggerCv3_u8) customVibration3_.trigger();
            // trigger Custom effect effect 4
            if (dap_actions_st.payloadPedalAction_st.triggerCv4_u8) customVibration4_.trigger();
            // trigger return pedal position
            if (dap_actions_st.payloadPedalAction_st.returnPedalConfig_u8)
            {
              g_ESPNow_config_request = true;
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
            if (dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_BRAKE || dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
            {
              Get_Rudder_action_b = true;
              if (dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 == 2)
                {
                  // Recv_mac=Clu_mac;
                  memcpy(g_Recv_mac, g_Clu_mac, 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.rudderStatus_b == false)
              {
                dap_calculationVariables_st.rudderStatus_b = true;
                Rudder_initializing = true;
                // ActiveSerial->println("Rudder on");
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.rudderStatus_b = false;
                // ActiveSerial->println("Rudder off");
                Rudder_deinitializing = true;
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
            if (dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_BRAKE || dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
            {
              Get_HeliRudder_action_b = true;
              if (dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 == 2)
                {
                  memcpy(g_Recv_mac, g_Clu_mac, 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.helicopterRudderStatus_b == false)
              {
                dap_calculationVariables_st.helicopterRudderStatus_b = true;
                HeliRudder_initializing = true;
                // ActiveSerial->println("Rudder on");
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.helicopterRudderStatus_b = false;
                // ActiveSerial->println("Rudder off");
                HeliRudder_deinitializing = true;
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
            if (dap_actions_st.payloadPedalAction_st.rudderBrakeAction_u8 == 1)
            {
              Get_Rudder_action_b = true;
              if (dap_calculationVariables_st.rudderBrakeStatus_b == false && dap_calculationVariables_st.rudderStatus_b == true)
              {
                dap_calculationVariables_st.rudderBrakeStatus_b = true;
                // ActiveSerial->println("Rudder brake on");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
              else
              {
                dap_calculationVariables_st.rudderBrakeStatus_b = false;
                // ActiveSerial->println("Rudder brake off");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
              }
            }
            // clear rudder status
            if (dap_actions_st.payloadPedalAction_st.rudderAction_u8 == (uint8_t)RudderAction::RUDDER_CLEAR_RUDDER_STATUS)
            {
              dap_calculationVariables_st.rudderStatus_b = false;
              dap_calculationVariables_st.helicopterRudderStatus_b = false;
              dap_calculationVariables_st.rudderBrakeStatus_b = false;
              // ActiveSerial->println("Rudder Status Clear");
              Rudder_deinitializing = true;
              HeliRudder_deinitializing = true;
              moveSlowlyToPosition_b = true;
            }
          }
        }
      }
      if(data_len==sizeof(DapActionOta_t))
      {        
        memcpy(&dap_action_ota_st, data, sizeof(DapActionOta_t));
        OTA_update_action_b=true;
      }
      

    }

    

  }

}
void OnSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{

}

// The callback that does the magic
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  //const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  //const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t* payload = ppkt->payload;
  if (ppkt->rx_ctrl.sig_len > 24)
  {
    const uint8_t *addr_DESTINATION = payload + 4;   
    const uint8_t *addr_SOURCE = payload + 10;  
    uint8_t addr_package[6];
    memcpy(addr_package, addr_SOURCE, 6);
    if(MacCheck(addr_package, g_Clu_mac))
    {
      g_rssi[0]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, g_Brk_mac))
    {
      g_rssi[1]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, g_Gas_mac))
    {
      g_rssi[2]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, g_esp_Host))
    {
      g_rssi[3]=ppkt->rx_ctrl.rssi;
    }
  }
  
  //int rssi = ppkt->rx_ctrl.rssi;
  //rssi_display = rssi;
  
}
void ESPNow_initialize()
{
  DapConfig_t dap_config_espnow_init_st;
  global_dap_config_class.getConfig(&dap_config_espnow_init_st, 500);
  WiFi.mode(WIFI_MODE_STA);
  delay(1000);
  ActiveSerial->println("Initializing Wifi, please wait");
  // ActiveSerial->print("Current MAC Address:  ");
  // ActiveSerial->println(WiFi.macAddress());
  WiFi.macAddress(g_esp_Mac);
  ActiveSerial->printf("Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_esp_Mac[0], g_esp_Mac[1], g_esp_Mac[2], g_esp_Mac[3], g_esp_Mac[4], g_esp_Mac[5]);
  #ifndef ESPNow_Pairing_function
    switch (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8)
    {
    case PEDAL_ID_CLUTCH:
      esp_wifi_set_mac(WIFI_IF_STA, &g_Clu_mac[0]);
      break;
    case PEDAL_ID_BRAKE:
      esp_wifi_set_mac(WIFI_IF_STA, &g_Brk_mac[0]);
      break;  
    case PEDAL_ID_THROTTLE:
      esp_wifi_set_mac(WIFI_IF_STA, &g_Gas_mac[0]);
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
    ESP_pairing_reg ESP_pairing_reg_local;
    EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
    memcpy(&_ESP_pairing_reg, &ESP_pairing_reg_local, sizeof(ESP_pairing_reg));
    //_ESP_pairing_reg=ESP_pairing_reg_local;
    // EEPROM.get(EEPROM_offset, _ESP_pairing_reg);
    for (int i = 0; i < 4; i++)
    {
      if (_ESP_pairing_reg.Pair_status[i] == 1)
      {
        ActiveSerial->print("Paired Device #");
        ActiveSerial->print(i);
        // ActiveSerial->print(" Pair: ");
        // ActiveSerial->print(_ESP_pairing_reg.Pair_status[i]);
        ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", _ESP_pairing_reg.Pair_mac[i][0], _ESP_pairing_reg.Pair_mac[i][1], _ESP_pairing_reg.Pair_mac[i][2], _ESP_pairing_reg.Pair_mac[i][3], _ESP_pairing_reg.Pair_mac[i][4], _ESP_pairing_reg.Pair_mac[i][5]);
      }
    }
    for (int i = 0; i < 4; i++)
    {
      if (_ESP_pairing_reg.Pair_status[i] == 1)
      {
        if (i == 0)
        {
          memcpy(&g_Clu_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 1)
        {
          memcpy(&g_Brk_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 2)
        {
          memcpy(&g_Gas_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 3)
        {
          memcpy(&g_esp_Host, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
      }
    }
    #endif

    if (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_BRAKE || dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_CLUTCH)
    {
      memcpy(g_Recv_mac, g_Gas_mac, 6);
      ESPNow.add_peer(g_Recv_mac);
    }

    if (dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8 == PEDAL_ID_THROTTLE)
    {
      memcpy(g_Recv_mac, g_Brk_mac, 6);
      ESPNow.add_peer(g_Brk_mac);
      ESPNow.add_peer(g_Clu_mac);
    }
    bool peerAddingChecker=true;
    if(ESPNow.add_peer(g_esp_master)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(g_broadcast_mac)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(g_esp_Host)!= ESP_OK) peerAddingChecker=false;
    if(peerAddingChecker) ActiveSerial->println("Sucess to add peers");

    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    //rssi calculate
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    g_ESPNow_initial_status=true;
    g_ESPNOW_status=true;
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
  buffer[1] = ESPNOW_LOG_MAGIC_KEY;
  buffer[2] = ESPNOW_LOG_MAGIC_KEY_2;
  buffer[3] = logLen;
  memcpy(&buffer[4], result, logLen);
  ESPNow.send_message(g_broadcast_mac, (uint8_t *)buffer, 4 + logLen);
  free(result);
}

void softwareAssignmentInitialize()
{
  DapAssignmentReg_t dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg_local);
  bool structChecker= true;
  uint16_t crc = checksumCalculator_u16((uint8_t *)(&dap_assignement_reg_local), sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
  if(dap_assignement_reg_local.payloadType_u8 != DAP_PAYLOAD_TYPE_ASSIGNMENT_U8) structChecker = false;
  if(dap_assignement_reg_local.magicKey_u8 != ESPNOW_ASSIGNMENT_MAGIC_KEY) structChecker = false;
  if(crc != dap_assignement_reg_local.crc_u16) structChecker = false;
  if(dap_assignement_reg_local.crc_u16 != crc) structChecker = false;
  DapConfig_t tmp;
  global_dap_config_class.getConfig(&tmp, 500);
  if(structChecker) 
  {
    memcpy(&dap_assignement_reg, &dap_assignement_reg_local, sizeof(DapAssignmentReg_t));
    deviceIdStructChecker = true;
    ActiveSerial->print("Overwritting pedal assignment: ");
    ActiveSerial->println(dap_assignement_reg_local.deviceId_u8);

    if (dap_assignement_reg.deviceId_u8 == PEDAL_ID_CLUTCH || dap_assignement_reg.deviceId_u8 == PEDAL_ID_BRAKE || dap_assignement_reg.deviceId_u8 == PEDAL_ID_THROTTLE)
    {
      tmp.payloadPedalConfig_st.pedalType_u8 = dap_assignement_reg.deviceId_u8;
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
    ActiveSerial->print(ESPNOW_ASSIGNMENT_MAGIC_KEY);
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
  dap_assignement_reg.magicKey_u8 = ESPNOW_ASSIGNMENT_MAGIC_KEY;
  dap_assignement_reg.payloadType_u8 = DAP_PAYLOAD_TYPE_ASSIGNMENT_U8;
  //refill the crc
  dap_assignement_reg.crc_u16 = checksumCalculator_u16((uint8_t *)(&dap_assignement_reg), sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg);
  EEPROM.commit();
  delay(1000);
  //check the data inside of eeprom
  DapAssignmentReg_t dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg_local);
  //list those assignment
  ActiveSerial->println("check the assignment in eeprom");
  ActiveSerial->print("Assignment expected:");
  ActiveSerial->print(dap_assignement_reg.deviceId_u8);
  ActiveSerial->print(" Assignment get:");
  ActiveSerial->println(dap_assignement_reg_local.deviceId_u8);
  ActiveSerial->print("crc expected:");
  ActiveSerial->print(dap_assignement_reg.crc_u16);
  ActiveSerial->print(" crc get:");
  ActiveSerial->println(dap_assignement_reg_local.crc_u16);
  
}
void clearAssignmentToEeprom()
{
  ActiveSerial->println("clear assignment from eeprom.");
  dap_assignement_reg.magicKey_u8 = 0;
  dap_assignement_reg.payloadType_u8 = 0;
  dap_assignement_reg.deviceId_u8 = 99;
  // refill the crc
  dap_assignement_reg.crc_u16 = 0;
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET_U32, dap_assignement_reg);
  EEPROM.commit();
  delay(1000);
}
