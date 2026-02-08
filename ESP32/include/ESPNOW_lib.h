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

uint8_t esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
//uint8_t esp_master[] = {0xdc, 0xda, 0x0c, 0x22, 0x8f, 0xd8}; // S3
//uint8_t esp_master[] = {0x48, 0x27, 0xe2, 0x59, 0x48, 0xc0}; // S2 mini
uint8_t Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};
uint8_t broadcast_mac[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t esp_Host[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t esp_Mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t Recv_mac[]={0};
uint16_t ESPNow_send=0;
uint16_t ESPNow_recieve=0;
int32_t rssi[4]={0,0,0,0};//clutch, brake,throttle,bridge
//bool MAC_get=false;
bool ESPNOW_status =false;
bool ESPNow_initial_status=false;
bool ESPNow_Rudder_Update= false;
bool ESPNow_no_device=false;
bool ESPNow_config_request=false;
bool ESPNow_restart=false;
bool ESPNow_OTA_enable=false;
uint8_t ESPNow_error_code=0;
bool ESPNow_Pairing_status = false;
bool UpdatePairingToEeprom = false;
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
DAP_Assignement_reg dap_assignement_reg;
DAP_Rudder_st dap_rudder_receiving;
DAP_Rudder_st dap_rudder_sending;
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
  _dap_joystick_message.payloadtype=DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK;
  _dap_joystick_message.cycleCnt_u64++;
  _dap_joystick_message.timeSinceBoot_i64 = esp_timer_get_time() / 1000;
  _dap_joystick_message.controllerValue_i32 = controllerValue;
  if(dap_calculationVariables_st.Rudder_status)
  {
    if(dap_calculationVariables_st.rudder_brake_status)
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
  esp_now_send(broadcast_mac, (uint8_t *) &_dap_joystick_message, sizeof(_dap_joystick_message));

  
  
  //esp_now_send(esp_master, (uint8_t *) &myData, sizeof(myData));
  /*
  if (result != ESP_OK) 
  {
    ESPNow_no_device=true;
    //ActiveSerial->println("Failed send data to ESP_Master");
  }
  else
  {
    ESPNow_no_device=false;
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

  if(data_len==sizeof(DAP_ESPPairing_st))
  {
    memcpy(&dap_esppairing_st, data , sizeof(DAP_ESPPairing_st));
    //pedal reg
    if(dap_esppairing_st.payloadESPNowInfo_._deviceID==0||dap_esppairing_st.payloadESPNowInfo_._deviceID==1||dap_esppairing_st.payloadESPNowInfo_._deviceID==2)
    {
      memcpy(&_ESP_pairing_reg.Pair_mac[dap_esppairing_st.payloadESPNowInfo_._deviceID], mac_addr , 6);
      _ESP_pairing_reg.Pair_status[dap_esppairing_st.payloadESPNowInfo_._deviceID]=1;
      UpdatePairingToEeprom = true;
    }
    //bridge and analog device, for pedal, only save for bridge
    if(dap_esppairing_st.payloadESPNowInfo_._deviceID==99/*||dap_esppairing_st.payloadESPNowInfo_._deviceID==98*/)
    {
      memcpy(&_ESP_pairing_reg.Pair_mac[3], mac_addr , 6);
      _ESP_pairing_reg.Pair_status[3]=1;
      UpdatePairingToEeprom = true;
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
  DAP_config_st dap_config_espnow_recv_st;
  
  global_dap_config_class.getConfig(&dap_config_espnow_recv_st, 500);

  /*
  if(ESPNOW_status)
  {
    memcpy(&ESPNow_recieve, data, sizeof(ESPNow_recieve));
    ESPNow_update=true;
  }
  */
  //only get mac in pairing
  if(ESPNow_pairing_action_b)
  {
    ESPNow_Pairing_callback(esp_now_info->src_addr, data, data_len);
  }
  if(ESPNOW_status)
  {
    //rudder message
    if(MacCheck(Recv_mac,(uint8_t *)esp_now_info->src_addr))
    {
      if(data_len==sizeof(DAP_Rudder_st))
      {

        bool structChecker = true;
        uint16_t crc;
        DAP_Rudder_st dap_rudder_st_local;
        memcpy(&dap_rudder_st_local, data, sizeof(DAP_Rudder_st));
        // check if data is plausible  
        if ( dap_rudder_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ESPNOW_RUDDER )
        {
          structChecker = false;
        }  
        if ( dap_rudder_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG )
        {
          structChecker = false;
        }
        // checksum validation
        crc = checksumCalculator((uint8_t*)(&(dap_rudder_st_local.payLoadHeader_)), sizeof(dap_rudder_st_local.payLoadHeader_) + sizeof(dap_rudder_st_local.payloadRudderState_));
        if (crc != dap_rudder_st_local.payloadFooter_.checkSum)
        {
          structChecker = false;
        }
        // if checks are successfull, overwrite global configuration struct
        if (structChecker == true)
        {
          memcpy(&dap_rudder_receiving, data, sizeof(DAP_Rudder_st));
          ESPNow_Rudder_Update=true;
        }

      }
    }
    if(MacCheck(esp_Host,(uint8_t *)esp_now_info->src_addr))
    {
      
      if (data_len == sizeof(DAP_config_st))
      {
        if (esp_now_info->src_addr[5] == esp_Host[5])
        {
          // ActiveSerial->println("dap_config_st ESPNow recieved");

          bool structChecker = true;
          uint16_t crc;
          DAP_config_st *dap_config_st_local_ptr;
          dap_config_st_local_ptr = &dap_config_espnow_recv_st;
          // ActiveSerial->readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
          memcpy(dap_config_st_local_ptr, data, sizeof(DAP_config_st));

          // check if data is plausible
          if (dap_config_espnow_recv_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG)
          {
            structChecker = false;
            ESPNow_error_code = 101;
          }
          if (dap_config_espnow_recv_st.payLoadHeader_.version != DAP_VERSION_CONFIG)
          {
            structChecker = false;
            if (ESPNow_error_code == 0)
            {
              ESPNow_error_code = 102;
            }
          }
          // checksum validation
          crc = checksumCalculator((uint8_t *)(&(dap_config_espnow_recv_st.payLoadHeader_)), sizeof(dap_config_espnow_recv_st.payLoadHeader_) + sizeof(dap_config_espnow_recv_st.payLoadPedalConfig_));
          if (crc != dap_config_espnow_recv_st.payloadFooter_.checkSum)
          {
            structChecker = false;
            if (ESPNow_error_code == 0)
            {
              ESPNow_error_code = 103;
            }
          }

          // if checks are successfull, overwrite global configuration struct
          if (structChecker == true)
          {
            // ActiveSerial->println("Updating pedal config");
            configDataPackage_t configPackage_st;
            configPackage_st.config_st = dap_config_espnow_recv_st;
            xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
            //global_dap_config_class.setConfig(dap_config_espnow_recv_st);
            if(dap_config_espnow_recv_st.payLoadHeader_.storeToEeprom==1)
            {
              Config_update_Buzzer_b = true;
            }            

          }
        }
      }

      DAP_actions_st dap_actions_st;
      if(data_len==sizeof(dap_actions_st))
      {
        //ActiveSerial->print(" get action");
        memcpy(&dap_actions_st, data, sizeof(DAP_actions_st));
        // ActiveSerial->readBytes((char*)&dap_actions_st, sizeof(DAP_actions_st));
        bool commandForAssignment_b = false;
        if(dap_actions_st.payLoadHeader_.PedalTag == PEDAL_ID_TEMP_1 || dap_actions_st.payLoadHeader_.PedalTag == PEDAL_ID_TEMP_2 ||dap_actions_st.payLoadHeader_.PedalTag == PEDAL_ID_TEMP_3)
        {
          commandForAssignment_b = true;
        }

        if (dap_actions_st.payLoadHeader_.PedalTag == dap_config_espnow_recv_st.payLoadPedalConfig_.pedal_type || commandForAssignment_b)
        {
          bool structChecker = true;
          uint16_t crc;
          if (dap_actions_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION)
          {
            structChecker = false;
            if (ESPNow_error_code == 0)
            {
              ESPNow_error_code = 111;
            }
          }
          if (dap_actions_st.payLoadHeader_.version != DAP_VERSION_CONFIG)
          {
            structChecker = false;
            if (ESPNow_error_code == 0)
            {
              ESPNow_error_code = 112;
            }
          }
          crc = checksumCalculator((uint8_t *)(&(dap_actions_st.payLoadHeader_)), sizeof(dap_actions_st.payLoadHeader_) + sizeof(dap_actions_st.payloadPedalAction_));
          if (crc != dap_actions_st.payloadFooter_.checkSum)
          {
            structChecker = false;
            if (ESPNow_error_code == 0)
            {
              ESPNow_error_code = 113;
            }
          }

          if (structChecker == true)
          {

            // 2= restart pedal
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::PEDAL_RESTART)
            {
              ESPNow_restart = true;
            }
            // 3= Wifi OTA
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::ENABLE_OTA)
            {
              ESPNow_OTA_enable = true;
            }
            // 5= Boot into download mode
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
            {
              ESPNOW_BootIntoDownloadMode = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
            {
              printPedalInfo_b = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceID=PEDAL_ID_CLUTCH;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceID = PEDAL_ID_BRAKE;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2 && commandForAssignment_b)
            {
              dap_assignement_reg.deviceID = PEDAL_ID_THROTTLE;
              assignmentUpdate_b = true;
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP)
            {
              assignmentUpdateBuzzer_b = true;
            }
            if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT && !commandForAssignment_b)
            {
              assignmentClear_b = true;
            }
            // trigger ABS effect
            if (dap_actions_st.payloadPedalAction_.triggerAbs_u8 > 0)
            {
              absOscillation.trigger();
              if (dap_actions_st.payloadPedalAction_.triggerAbs_u8 > 1)
              {
                dap_calculationVariables_st.TrackCondition = dap_actions_st.payloadPedalAction_.triggerAbs_u8 - 1;
              }
              else
              {
                dap_calculationVariables_st.TrackCondition = dap_actions_st.payloadPedalAction_.triggerAbs_u8 = 0;
              }
            }
            // RPM effect
            _RPMOscillation.RPM_value = dap_actions_st.payloadPedalAction_.RPM_u8;
            // G force effect
            _G_force_effect.G_value = dap_actions_st.payloadPedalAction_.G_value - 128;
            // wheel slip
            if (dap_actions_st.payloadPedalAction_.WS_u8)
            {
              _WSOscillation.trigger();
            }
            // Road impact && Rudder G impact
            if (dap_calculationVariables_st.Rudder_status == false)
            {
              _Road_impact_effect.Road_Impact_value = dap_actions_st.payloadPedalAction_.impact_value_u8;
            }
            else
            {
              _rudder_g_force.G_value = dap_actions_st.payloadPedalAction_.impact_value_u8;
            }
            // trigger system identification
            if (dap_actions_st.payloadPedalAction_.startSystemIdentification_u8)
            {
              systemIdentificationMode_b = true;
            }
            // trigger Custom effect effect 1
            if (dap_actions_st.payloadPedalAction_.Trigger_CV_1) CV1.trigger();
            // trigger Custom effect effect 2
            if (dap_actions_st.payloadPedalAction_.Trigger_CV_2) CV2.trigger();
            // trigger Custom effect effect 3
            if (dap_actions_st.payloadPedalAction_.Trigger_CV_3) CV3.trigger();
            // trigger Custom effect effect 4
            if (dap_actions_st.payloadPedalAction_.Trigger_CV_4) CV4.trigger();
            // trigger return pedal position
            if (dap_actions_st.payloadPedalAction_.returnPedalConfig_u8)
            {
              ESPNow_config_request = true;
              /*
              DAP_config_st * dap_config_st_local_ptr;
              dap_config_st_local_ptr = &dap_config_st;
              //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
              crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
              dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
              ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
              ActiveSerial->print("\r\n");
              */
            }
            if (dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_BRAKE || dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
            {
              Get_Rudder_action_b = true;
              if (dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payLoadPedalConfig_.pedal_type == 2)
                {
                  // Recv_mac=Clu_mac;
                  memcpy(Recv_mac, Clu_mac, 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.Rudder_status == false)
              {
                dap_calculationVariables_st.Rudder_status = true;
                Rudder_initializing = true;
                // ActiveSerial->println("Rudder on");
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
              else
              {
                dap_calculationVariables_st.Rudder_status = false;
                // ActiveSerial->println("Rudder off");
                Rudder_deinitializing = true;
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
            }
            if (dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_BRAKE || dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
            {
              Get_HeliRudder_action_b = true;
              if (dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH)
              {
                if (dap_config_espnow_recv_st.payLoadPedalConfig_.pedal_type == 2)
                {
                  memcpy(Recv_mac, Clu_mac, 6);
                  // ESPNow.add_peer(Recv_mac);
                }
              }
              if (dap_calculationVariables_st.helicopterRudderStatus == false)
              {
                dap_calculationVariables_st.helicopterRudderStatus = true;
                HeliRudder_initializing = true;
                // ActiveSerial->println("Rudder on");
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
              else
              {
                dap_calculationVariables_st.helicopterRudderStatus = false;
                // ActiveSerial->println("Rudder off");
                HeliRudder_deinitializing = true;
                moveSlowlyToPosition_b = true;
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
            }
            if (dap_actions_st.payloadPedalAction_.Rudder_brake_action == 1)
            {
              Get_Rudder_action_b = true;
              if (dap_calculationVariables_st.rudder_brake_status == false && dap_calculationVariables_st.Rudder_status == true)
              {
                dap_calculationVariables_st.rudder_brake_status = true;
                // ActiveSerial->println("Rudder brake on");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
              else
              {
                dap_calculationVariables_st.rudder_brake_status = false;
                // ActiveSerial->println("Rudder brake off");
                // ActiveSerial->print("status:");
                // ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
              }
            }
            // clear rudder status
            if (dap_actions_st.payloadPedalAction_.Rudder_action == (uint8_t)RudderAction::RUDDER_CLEAR_RUDDER_STATUS)
            {
              dap_calculationVariables_st.Rudder_status = false;
              dap_calculationVariables_st.helicopterRudderStatus = false;
              dap_calculationVariables_st.rudder_brake_status = false;
              // ActiveSerial->println("Rudder Status Clear");
              Rudder_deinitializing = true;
              HeliRudder_deinitializing = true;
              moveSlowlyToPosition_b = true;
            }
          }
        }
      }
      if(data_len==sizeof(DAP_action_ota_st))
      {        
        memcpy(&dap_action_ota_st, data, sizeof(DAP_action_ota_st));
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
    if(MacCheck(addr_package, Clu_mac))
    {
      rssi[0]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, Brk_mac))
    {
      rssi[1]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, Gas_mac))
    {
      rssi[2]=ppkt->rx_ctrl.rssi;
    }
    if(MacCheck(addr_package, esp_Host))
    {
      rssi[3]=ppkt->rx_ctrl.rssi;
    }
  }
  
  //int rssi = ppkt->rx_ctrl.rssi;
  //rssi_display = rssi;
  
}
void ESPNow_initialize()
{
  DAP_config_st dap_config_espnow_init_st;
  global_dap_config_class.getConfig(&dap_config_espnow_init_st, 500);
  WiFi.mode(WIFI_MODE_STA);
  delay(1000);
  ActiveSerial->println("Initializing Wifi, please wait");
  // ActiveSerial->print("Current MAC Address:  ");
  // ActiveSerial->println(WiFi.macAddress());
  WiFi.macAddress(esp_Mac);
  ActiveSerial->printf("Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Mac[0], esp_Mac[1], esp_Mac[2], esp_Mac[3], esp_Mac[4], esp_Mac[5]);
  #ifndef ESPNow_Pairing_function
    switch (dap_config_espnow_init_st.payLoadPedalConfig_.pedal_type)
    {
    case PEDAL_ID_CLUTCH:
      esp_wifi_set_mac(WIFI_IF_STA, &Clu_mac[0]);
      break;
    case PEDAL_ID_BRAKE:
      esp_wifi_set_mac(WIFI_IF_STA, &Brk_mac[0]);
      break;  
    case PEDAL_ID_THROTTLE:
      esp_wifi_set_mac(WIFI_IF_STA, &Gas_mac[0]);
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
    #if PCB_VERSION == 7
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
          memcpy(&Clu_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 1)
        {
          memcpy(&Brk_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 2)
        {
          memcpy(&Gas_mac, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
        if (i == 3)
        {
          memcpy(&esp_Host, &_ESP_pairing_reg.Pair_mac[i], 6);
        }
      }
    }
    #endif

    if (dap_config_espnow_init_st.payLoadPedalConfig_.pedal_type == PEDAL_ID_BRAKE || dap_config_espnow_init_st.payLoadPedalConfig_.pedal_type == PEDAL_ID_CLUTCH)
    {
      memcpy(Recv_mac, Gas_mac, 6);
      ESPNow.add_peer(Recv_mac);
    }

    if (dap_config_espnow_init_st.payLoadPedalConfig_.pedal_type == PEDAL_ID_THROTTLE)
    {
      memcpy(Recv_mac, Brk_mac, 6);
      ESPNow.add_peer(Brk_mac);
      ESPNow.add_peer(Clu_mac);
    }
    bool peerAddingChecker=true;
    if(ESPNow.add_peer(esp_master)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(broadcast_mac)!= ESP_OK) peerAddingChecker=false;
    if(ESPNow.add_peer(esp_Host)!= ESP_OK) peerAddingChecker=false;
    if(peerAddingChecker) ActiveSerial->println("Sucess to add peers");

    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    //rssi calculate
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    ESPNow_initial_status=true;
    ESPNOW_status=true;
    ActiveSerial->println("ESPNow Initialized");
  
}

void sendESPNOWLog(const char *log,...)
{
  uint8_t buffer[250];
  uint8_t payloadType = DAP_PAYLOAD_TYPE_ESPNOW_LOG;
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
  ESPNow.send_message(broadcast_mac, (uint8_t *)buffer, 4 + logLen);
  free(result);
}

void softwareAssignmentInitialize()
{
  DAP_Assignement_reg dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET, dap_assignement_reg_local);
  bool structChecker= true;
  uint16_t crc = checksumCalculator((uint8_t *)(&dap_assignement_reg_local), sizeof(DAP_Assignement_reg) - sizeof(uint16_t));
  if(dap_assignement_reg_local.payloadType!= DAP_PAYLOAD_TYPE_ASSIGNMENT) structChecker = false;
  if(dap_assignement_reg_local.magicKey!= ESPNOW_ASSIGNMENT_MAGIC_KEY) structChecker = false;
  if(crc!=dap_assignement_reg_local.crc) structChecker = false;
  if(dap_assignement_reg_local.crc != crc) structChecker = false;
  DAP_config_st tmp;
  global_dap_config_class.getConfig(&tmp, 500);
  if(structChecker) 
  {
    memcpy(&dap_assignement_reg, &dap_assignement_reg_local, sizeof(DAP_Assignement_reg));
    deviceIdStructChecker = true;
    ActiveSerial->print("Overwritting pedal assignment: ");
    ActiveSerial->println(dap_assignement_reg_local.deviceID);

    if (dap_assignement_reg.deviceID == PEDAL_ID_CLUTCH || dap_assignement_reg.deviceID == PEDAL_ID_BRAKE || dap_assignement_reg.deviceID == PEDAL_ID_THROTTLE)
    {
      tmp.payLoadPedalConfig_.pedal_type = dap_assignement_reg.deviceID;
    }
    else
    {
      tmp.payLoadPedalConfig_.pedal_type = PEDAL_ID_UNKNOWN;
    }
      
  }
  else
  {
    tmp.payLoadPedalConfig_.pedal_type = PEDAL_ID_UNKNOWN;
    ActiveSerial->println("Assignment error:");
    ActiveSerial->print("Payload type expect:");
    ActiveSerial->print(DAP_PAYLOAD_TYPE_ASSIGNMENT);
    ActiveSerial->print(" Payload type get:");
    ActiveSerial->println(dap_assignement_reg_local.payloadType);
    ActiveSerial->print("Magic key expect:");
    ActiveSerial->print(ESPNOW_ASSIGNMENT_MAGIC_KEY);
    ActiveSerial->print(" Magic key get:");
    ActiveSerial->println(dap_assignement_reg_local.magicKey);
    ActiveSerial->print("crc expect:");
    ActiveSerial->print(crc);
    ActiveSerial->print(" crc get:");
    ActiveSerial->println(dap_assignement_reg_local.crc);
    ActiveSerial->print("Pedal ID get:");
    ActiveSerial->println(dap_assignement_reg_local.deviceID);
  }
  configDataPackage_t configPackage_st;
  configPackage_st.config_st = tmp;
  xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
  delay(1000); // delay for writting config into global
}

void writeAssignmentToEeprom()
{
  ActiveSerial->println("Writting assignment to eeprom.");
  dap_assignement_reg.magicKey=ESPNOW_ASSIGNMENT_MAGIC_KEY;
  dap_assignement_reg.payloadType=DAP_PAYLOAD_TYPE_ASSIGNMENT;
  //refill the crc
  dap_assignement_reg.crc = checksumCalculator((uint8_t *)(&dap_assignement_reg), sizeof(DAP_Assignement_reg) - sizeof(uint16_t));
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET, dap_assignement_reg);
  EEPROM.commit();
  delay(1000);
  //check the data inside of eeprom
  DAP_Assignement_reg dap_assignement_reg_local;
  EEPROM.get(ASSIGNMENT_EEPROM_OFFSET, dap_assignement_reg_local);
  //list those assignment
  ActiveSerial->println("check the assignment in eeprom");
  ActiveSerial->print("Assignment expected:");
  ActiveSerial->print(dap_assignement_reg.deviceID);
  ActiveSerial->print(" Assignment get:");
  ActiveSerial->println(dap_assignement_reg_local.deviceID);
  ActiveSerial->print("crc expected:");
  ActiveSerial->print(dap_assignement_reg.crc);
  ActiveSerial->print(" crc get:");
  ActiveSerial->println(dap_assignement_reg_local.crc);
  
}
void clearAssignmentToEeprom()
{
  ActiveSerial->println("clear assignment from eeprom.");
  dap_assignement_reg.magicKey = 0;
  dap_assignement_reg.payloadType = 0;
  dap_assignement_reg.deviceID = 99;
  // refill the crc
  dap_assignement_reg.crc = 0;
  // write assignment to eeprom
  EEPROM.put(ASSIGNMENT_EEPROM_OFFSET, dap_assignement_reg);
  EEPROM.commit();
  delay(1000);
}