#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "ESPNowW.h"
#include "DiyActivePedal_types.h"
//#define ESPNow_debug
uint8_t esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
//uint8_t esp_master[] = {0xdc, 0xda, 0x0c, 0x22, 0x8f, 0xd8}; // S3
//uint8_t esp_master[] = {0x48, 0x27, 0xe2, 0x59, 0x48, 0xc0}; // S2 mini
uint8_t Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};
uint8_t broadcast_mac[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t esp_Host[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t esp_Mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t* Recv_mac;
uint16_t ESPNow_send=0;
uint16_t ESPNow_recieve=0;
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
bool ESPNOW_BootIntoDownloadMode = false;
bool Get_Rudder_action_b=false;
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
    //Serial.println("Failed send data to ESP_Master");
  }
  else
  {
    ESPNow_no_device=false;
  }
  */
  
  /*if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
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
void onRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) 
{
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
    ESPNow_Pairing_callback(mac_addr, data, data_len);
  }
  if(ESPNOW_status)
  {
    //rudder message
    if(MacCheck(Recv_mac,(uint8_t *)mac_addr))
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
    if(MacCheck(esp_Host,(uint8_t *)mac_addr))
    {

      if(data_len==sizeof(dap_config_st))
      {

        if(mac_addr[5]==esp_Host[5])
        {
          //Serial.println("dap_config_st ESPNow recieved");
          if(semaphore_updateConfig!=NULL)
          {
            if(xSemaphoreTake(semaphore_updateConfig, (TickType_t)1)==pdTRUE)
            {
              bool structChecker = true;
              uint16_t crc;
              DAP_config_st * dap_config_st_local_ptr;
              dap_config_st_local_ptr = &dap_config_st_local;
              //Serial.readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
              memcpy(dap_config_st_local_ptr, data, sizeof(DAP_config_st));
        
    

              // check if data is plausible
              if ( dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG )
              { 
                structChecker = false;
                ESPNow_error_code=101;

              }
              if ( dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG )
              { 
                structChecker = false;
                if(ESPNow_error_code==0)
                {
                  ESPNow_error_code=102;
                }
                
              }
                      // checksum validation
              crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
              if (crc != dap_config_st_local.payloadFooter_.checkSum)
              { 
                structChecker = false;
                if(ESPNow_error_code==0)
                {
                  ESPNow_error_code=103;
                }

              }


                      // if checks are successfull, overwrite global configuration struct
              if (structChecker == true)
              {
                //Serial.println("Updating pedal config");
                configUpdateAvailable = true;   
                Config_update_b=true;       
              }
                xSemaphoreGive(semaphore_updateConfig);
            }
          }
        }  
        
      }

      DAP_actions_st dap_actions_st;
      if(data_len==sizeof(dap_actions_st))
      {
              
              memcpy(&dap_actions_st, data, sizeof(dap_actions_st));
              //Serial.readBytes((char*)&dap_actions_st, sizeof(DAP_actions_st));
              if(dap_actions_st.payLoadHeader_.PedalTag==dap_config_st.payLoadPedalConfig_.pedal_type)
              {
                bool structChecker = true;
                uint16_t crc;
                if ( dap_actions_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION )
                { 
                  structChecker = false;
                  if(ESPNow_error_code==0)
                  {
                    ESPNow_error_code=111;
                  }

                }
                if ( dap_actions_st.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
                  structChecker = false;
                  if(ESPNow_error_code==0)
                  {
                    ESPNow_error_code=112;
                  }

                }
                crc = checksumCalculator((uint8_t*)(&(dap_actions_st.payLoadHeader_)), sizeof(dap_actions_st.payLoadHeader_) + sizeof(dap_actions_st.payloadPedalAction_));
                if (crc != dap_actions_st.payloadFooter_.checkSum){ 
                  structChecker = false;
                  if(ESPNow_error_code==0)
                  {
                    ESPNow_error_code=113;
                  }

                }


                if (structChecker == true)
                {

                  
                  //2= restart pedal
                  if (dap_actions_st.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::PEDAL_RESTART)
                  {
                    ESPNow_restart = true;
                  }
                  //3= Wifi OTA
                  if (dap_actions_st.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::ENABLE_OTA)
                  {
                    ESPNow_OTA_enable = true;
                  }
                  //5= Boot into download mode
                  if (dap_actions_st.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
                  {
                    ESPNOW_BootIntoDownloadMode = true;
                  }
                  // trigger ABS effect
                  if (dap_actions_st.payloadPedalAction_.triggerAbs_u8>0)
                  {
                    absOscillation.trigger();
                    if(dap_actions_st.payloadPedalAction_.triggerAbs_u8>1)
                    {
                      dap_calculationVariables_st.TrackCondition=dap_actions_st.payloadPedalAction_.triggerAbs_u8-1;
                    }
                    else
                    {
                      dap_calculationVariables_st.TrackCondition=dap_actions_st.payloadPedalAction_.triggerAbs_u8=0;
                    }
                    
                  }
                  //RPM effect
                  _RPMOscillation.RPM_value=dap_actions_st.payloadPedalAction_.RPM_u8;
                  //G force effect
                  _G_force_effect.G_value=dap_actions_st.payloadPedalAction_.G_value-128;       
                  //wheel slip
                  if (dap_actions_st.payloadPedalAction_.WS_u8)
                  {
                    _WSOscillation.trigger();
                  }     
                  //Road impact && Rudder G impact
                  if(dap_calculationVariables_st.Rudder_status==false)
                  {
                    _Road_impact_effect.Road_Impact_value=dap_actions_st.payloadPedalAction_.impact_value_u8;
                  }
                  else
                  {
                    _rudder_g_force.G_value=dap_actions_st.payloadPedalAction_.impact_value_u8;
                  }
                  // trigger system identification
                  if (dap_actions_st.payloadPedalAction_.startSystemIdentification_u8)
                  {
                    systemIdentificationMode_b = true;
                  }
                  // trigger Custom effect effect 1
                  if (dap_actions_st.payloadPedalAction_.Trigger_CV_1)
                  {
                    CV1.trigger();
                  }
                  // trigger Custom effect effect 2
                  if (dap_actions_st.payloadPedalAction_.Trigger_CV_2)
                  {
                    CV2.trigger();
                  }
                  // trigger return pedal position
                  if (dap_actions_st.payloadPedalAction_.returnPedalConfig_u8)
                  {
                    ESPNow_config_request=true;
                    /*
                    DAP_config_st * dap_config_st_local_ptr;
                    dap_config_st_local_ptr = &dap_config_st;
                    //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                    crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                    dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
                    Serial.write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
                    Serial.print("\r\n");
                    */
                  }
                  if(dap_actions_st.payloadPedalAction_.Rudder_action==1 || dap_actions_st.payloadPedalAction_.Rudder_action==3)
                  {
                    Get_Rudder_action_b=true;
                    if(dap_actions_st.payloadPedalAction_.Rudder_action==3)
                    {
                      if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
                      {
                        Recv_mac=Clu_mac;
                        //ESPNow.add_peer(Recv_mac);
                      }
                    }
                    if(dap_calculationVariables_st.Rudder_status==false)
                    {
                      dap_calculationVariables_st.Rudder_status=true;
                      Rudder_initializing=true;
                      //Serial.println("Rudder on");
                      moveSlowlyToPosition_b=true;
                      //Serial.print("status:");
                      //Serial.println(dap_calculationVariables_st.Rudder_status);
                    }
                    else
                    {
                      dap_calculationVariables_st.Rudder_status=false;
                      //Serial.println("Rudder off");
                      Rudder_deinitializing=true;
                      moveSlowlyToPosition_b=true;
                      //Serial.print("status:");
                      //Serial.println(dap_calculationVariables_st.Rudder_status);
                    }
                  }
                  if(dap_actions_st.payloadPedalAction_.Rudder_brake_action==1)
                  {
                    Get_Rudder_action_b=true;
                    if(dap_calculationVariables_st.rudder_brake_status==false&&dap_calculationVariables_st.Rudder_status==true)
                    {
                      dap_calculationVariables_st.rudder_brake_status=true;
                      //Serial.println("Rudder brake on");
                      //Serial.print("status:");
                      //Serial.println(dap_calculationVariables_st.Rudder_status);
                    }
                    else
                    {
                      dap_calculationVariables_st.rudder_brake_status=false;
                      //Serial.println("Rudder brake off");
                      //Serial.print("status:");
                      //Serial.println(dap_calculationVariables_st.Rudder_status);
                    }
                  }
                  //clear rudder status
                  if(dap_actions_st.payloadPedalAction_.Rudder_action==2)
                  {
                    dap_calculationVariables_st.Rudder_status=false;
                    dap_calculationVariables_st.rudder_brake_status=false;
                    //Serial.println("Rudder Status Clear");
                    Rudder_deinitializing=true;
                    moveSlowlyToPosition_b=true;
                  }
                }
              }

              

            
              
      }
      if(data_len==sizeof(Basic_WIfi_info))
      {        
        memcpy(&_basic_wifi_info, data, sizeof(Basic_WIfi_info));
        OTA_update_action_b=true;
      }
      

    }

    

  }

}
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{

}
void ESPNow_initialize()
{

    WiFi.mode(WIFI_MODE_STA);
    Serial.println("Initializing ESPNow, please wait"); 
    //Serial.print("Current MAC Address:  ");  
    //Serial.println(WiFi.macAddress());
    WiFi.macAddress(esp_Mac); 
    Serial.printf("Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Mac[0], esp_Mac[1], esp_Mac[2], esp_Mac[3], esp_Mac[4], esp_Mac[5]);
    #ifndef ESPNow_Pairing_function
      if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
      {
        esp_wifi_set_mac(WIFI_IF_STA, &Clu_mac[0]);
      }
      if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
      {
        esp_wifi_set_mac(WIFI_IF_STA, &Brk_mac[0]);
      }
      if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
      {
        esp_wifi_set_mac(WIFI_IF_STA, &Gas_mac[0]);
      }
      delay(300);
      Serial.print("Modified MAC Address:  ");  
      Serial.println(WiFi.macAddress());
    #endif


    ESPNow.init();
    Serial.println("Wait for ESPNOW");
    delay(3000);
    #ifdef ESPNow_S3
      //esp_wifi_config_espnow_rate(WIFI_IF_STA, 	WIFI_PHY_RATE_54M);
      esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
      esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    #endif
    #ifdef ESPNow_ESP32
      esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_LGI);
      //esp_wifi_config_espnow_rate(WIFI_IF_STA, 	WIFI_PHY_RATE_54M);
    #endif
    #ifdef ESPNow_Pairing_function
      ESP_pairing_reg ESP_pairing_reg_local;
      EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
      memcpy(&_ESP_pairing_reg, &ESP_pairing_reg_local,sizeof(ESP_pairing_reg));
      //_ESP_pairing_reg=ESP_pairing_reg_local;
      //EEPROM.get(EEPROM_offset, _ESP_pairing_reg);
      for(int i=0;i<4;i++)
      { 
        if(_ESP_pairing_reg.Pair_status[i]==1)
        {
          Serial.print("Paired Device #");
          Serial.print(i);
          //Serial.print(" Pair: ");
          //Serial.print(_ESP_pairing_reg.Pair_status[i]);
          Serial.printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", _ESP_pairing_reg.Pair_mac[i][0], _ESP_pairing_reg.Pair_mac[i][1], _ESP_pairing_reg.Pair_mac[i][2], _ESP_pairing_reg.Pair_mac[i][3], _ESP_pairing_reg.Pair_mac[i][4], _ESP_pairing_reg.Pair_mac[i][5]);
        }           
      }
      for(int i=0; i<4;i++)
      {
        if(_ESP_pairing_reg.Pair_status[i]==1)
        {
          if(i==0)
          {
            memcpy(&Clu_mac,&_ESP_pairing_reg.Pair_mac[i],6);
          }
          if(i==1)
          {
            memcpy(&Brk_mac,&_ESP_pairing_reg.Pair_mac[i],6);
          }
          if(i==2)
          {
            memcpy(&Gas_mac,&_ESP_pairing_reg.Pair_mac[i],6);
          }        
          if(i==3)
          {
            memcpy(&esp_Host,&_ESP_pairing_reg.Pair_mac[i],6);
          }        
        }
      }
    #endif
       
    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      Recv_mac=Gas_mac;
      ESPNow.add_peer(Recv_mac);    
    }

    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      Recv_mac=Brk_mac;
      ESPNow.add_peer(Brk_mac);
      ESPNow.add_peer(Clu_mac);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
    {
      Recv_mac=Gas_mac;
      ESPNow.add_peer(Recv_mac);
    }
    


    
    if(ESPNow.add_peer(esp_master)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add joystick peer");
    }
    if(ESPNow.add_peer(esp_Host)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add host peer");
    }
    if(ESPNow.add_peer(broadcast_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add broadcast peer");
    }
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    ESPNow_initial_status=true;
    ESPNOW_status=true;
    Serial.println("ESPNow Initialized");
  
}
