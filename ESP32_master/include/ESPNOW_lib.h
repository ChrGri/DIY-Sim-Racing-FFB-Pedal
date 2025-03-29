#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "esp_now.h"
#include "ESPNowW.h"
#include "Main.h"

//#define ESPNow_debug
uint8_t esp_master[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
uint8_t Clu_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x32};
uint8_t Gas_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33};
uint8_t Brk_mac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x34};
uint8_t broadcast_mac[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t esp_Host[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t esp_Mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t* Recv_mac;
uint16_t ESPNow_send=0;
uint16_t ESPNow_recieve=0;
int rssi_display;
//bool MAC_get=false;
bool ESPNOW_status =false;
bool ESPNow_initial_status=false;
bool ESPNow_update= false;
bool ESPNow_no_device=false;
bool update_basic_state=false;
bool update_extend_state=false;
bool pedal_OTA_action_b=false;
uint16_t Joystick_value[]={0,0,0};
bool ESPNow_request_config_b[3]={false,false,false};
bool ESPNow_error_b=false;
uint16_t pedal_throttle_value=0;
uint16_t pedal_brake_value=0;
uint16_t pedal_cluth_value=0;
uint16_t pedal_brake_rudder_value=0;
uint16_t pedal_throttle_rudder_value=0;
uint8_t pedal_status=0;
bool ESPNow_Pairing_status = false;
bool UpdatePairingToEeprom = false;
bool ESPNow_pairing_action_b = false;
bool software_pairing_action_b = false;

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
struct ESPNow_Send_Struct
{ 
  uint16_t pedal_position;
  float pedal_position_ratio;
};

typedef struct ESP_pairing_reg
{
  uint8_t Pair_status[4];
  uint8_t Pair_mac[4][6];
} ESP_pairing_reg;

typedef struct DAP_Joystick_Message {
  uint8_t payloadtype;
  uint64_t cycleCnt_u64;
  int64_t timeSinceBoot_i64;
	int32_t controllerValue_i32;
  int8_t pedal_status; //0=default, 1=rudder, 2=rudder brake
} DAP_Joystick_Message;

// Create a struct_message called myData
DAP_Joystick_Message Joystick_Data;
DAP_Joystick_Message broadcast_incoming;
ESPNow_Send_Struct _ESPNow_Recv;
ESPNow_Send_Struct _ESPNow_Send;
ESP_pairing_reg _ESP_pairing_reg;

bool sendMessageToMaster(int32_t controllerValue)
{
  Joystick_Data.payloadtype=DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK;
  Joystick_Data.cycleCnt_u64++;
  Joystick_Data.timeSinceBoot_i64 = esp_timer_get_time() / 1000;
  Joystick_Data.controllerValue_i32 = controllerValue;
  if(dap_calculationVariables_st.Rudder_status)
  {
    if(dap_calculationVariables_st.rudder_brake_status)
    {
      Joystick_Data.pedal_status=2;
    }
    else
    {
      Joystick_Data.pedal_status=1;
    }
  }
  else
  {
    Joystick_Data.pedal_status=0;
  }
  esp_now_send(broadcast_mac, (uint8_t *) &Joystick_Data, sizeof(Joystick_Data));
  return true;
  
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
    //bridge and analog device
    if(dap_esppairing_st.payloadESPNowInfo_._deviceID==99||dap_esppairing_st.payloadESPNowInfo_._deviceID==98)
    {
      memcpy(&_ESP_pairing_reg.Pair_mac[3], mac_addr , 6);
      _ESP_pairing_reg.Pair_status[3]=1;
      UpdatePairingToEeprom = true;
    }
  }


}
//void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
void onRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) 
{
  //only get mac in pairing
  if(ESPNow_pairing_action_b)
  {
    ESPNow_Pairing_callback(mac_addr, data, data_len);
  }
  //only recieve the package from registed mac address
  if(MacCheck((uint8_t*)mac_addr, Clu_mac)||MacCheck((uint8_t*)mac_addr, Brk_mac)||MacCheck((uint8_t*)mac_addr, Gas_mac))
  {
    if(data_len==sizeof(Joystick_Data))
    {

      
      memcpy(&Joystick_Data, data, sizeof(DAP_Joystick_Message));
      if(Joystick_Data.payloadtype==DAP_PAYLOAD_TYPE_ESPNOW_JOYSTICK)
      {
        //#ifdef ACTIVATE_JOYSTICK_OUTPUT
        // normalize controller output
        int32_t joystickNormalizedToInt32 = NormalizeControllerOutputValue(Joystick_Data.controllerValue_i32, 0, 10000, 100); 
        //if(esp_now_info->src_addr[5]==Clu_mac[5])
        if(mac_addr[5]==Clu_mac[5])
        {
          pedal_cluth_value=joystickNormalizedToInt32;
          Joystick_value[0]=Joystick_Data.controllerValue_i32;
          //joystick_update=true;
        }
        if(mac_addr[5]==Brk_mac[5])
        {
          pedal_brake_value=joystickNormalizedToInt32;
          Joystick_value[1]=Joystick_Data.controllerValue_i32;
          
          //joystick_update=true;
        }
        if(mac_addr[5]==Gas_mac[5])
        {
          pedal_throttle_value=joystickNormalizedToInt32;
          Joystick_value[2]=Joystick_Data.controllerValue_i32;
          pedal_status=Joystick_Data.pedal_status;//control pedal status only by Throttle
          //joystick_update=true;
        }
      }

      #ifdef ESPNow_debug
      Serial.print("Bytes received: ");
      Serial.println(len);
      Serial.print("CycleCnt: ");
      Serial.println(myData.cycleCnt_u64);
      Serial.print("TimeSinceBoot in ms (shared): ");
      Serial.println(myData.timeSinceBoot_i64);
      Serial.print("controllerValue_i32: ");
      Serial.println(myData.controllerValue_i32);	
      Serial.println();
      #endif
    }
    
    if(data_len==sizeof(dap_state_basic_st))
    {
      memcpy(&dap_state_basic_st, data, sizeof(dap_state_basic_st));
      //Joystick_value[dap_state_basic_st.payLoadHeader_.PedalTag]=dap_state_basic_st.payloadPedalState_Basic_.joystickOutput_u16;
      update_basic_state=true;
      if(dap_state_basic_st.payloadPedalState_Basic_.error_code_u8!=0)
      {
        ESPNow_error_b=true;
      }
    }

    if(data_len==sizeof(dap_state_extended_st))
    {
      memcpy(&dap_state_extended_st, data, sizeof(dap_state_extended_st));
      update_extend_state=true;
    }

    if(data_len==sizeof(DAP_config_st))
    {
      memcpy(&dap_config_st_Temp, data, sizeof(DAP_config_st));
      ESPNow_request_config_b[dap_config_st_Temp.payLoadPedalConfig_.pedal_type]=true;
      if(dap_config_st_Temp.payLoadPedalConfig_.pedal_type==0)
      {
        memcpy(&dap_config_st_Clu, &dap_config_st_Temp, sizeof(DAP_config_st));
      }
      if(dap_config_st_Temp.payLoadPedalConfig_.pedal_type==1)
      {
        memcpy(&dap_config_st_Brk, &dap_config_st_Temp, sizeof(DAP_config_st));
      }
      if(dap_config_st_Temp.payLoadPedalConfig_.pedal_type==2)
      {
        memcpy(&dap_config_st_Gas, &dap_config_st_Temp, sizeof(DAP_config_st));
      }
      
    }
  }
  


}
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status)
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

  int rssi = ppkt->rx_ctrl.rssi;
  rssi_display = rssi;
}

void ESPNow_initialize()
{

    WiFi.mode(WIFI_MODE_STA);
    Serial.println("[L]Initializing ESPNow, please wait"); 
    WiFi.macAddress(esp_Mac); 
    Serial.printf("[L]Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Mac[0], esp_Mac[1], esp_Mac[2], esp_Mac[3], esp_Mac[4], esp_Mac[5]);
    
    //Serial.print("Current MAC Address:  ");  
    //Serial.println(WiFi.macAddress());
    #ifndef ESPNow_Pairing_function
      Serial.println("Overwriting Mac address.......");
      esp_wifi_set_mac(WIFI_IF_STA, &esp_Host[0]);
      delay(300);
      Serial.print("[L]Modified MAC Address:  ");  
      Serial.println(WiFi.macAddress());
    #endif
    ESPNow.init();
    Serial.println("[L]Waiting for ESPNOW");
    delay(3000);
    #ifdef Using_Board_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_LGI);
    #endif
    
    #ifdef Using_Board_ESP32S3
    //esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);
    esp_wifi_config_espnow_rate(WIFI_IF_STA, 	WIFI_PHY_RATE_11M_L);
    esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    #endif
    //reading from eeprom
    #ifdef ESPNow_Pairing_function
    ESP_pairing_reg ESP_pairing_reg_local;
    EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
    memcpy(&_ESP_pairing_reg, &ESP_pairing_reg_local,sizeof(ESP_pairing_reg));
    //_ESP_pairing_reg=ESP_pairing_reg_local;
    //EEPROM.get(EEPROM_offset, _ESP_pairing_reg);
    Serial.print("[L]");
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
          if(MacCheck(_ESP_pairing_reg.Pair_mac[0],_ESP_pairing_reg.Pair_mac[1])||MacCheck(_ESP_pairing_reg.Pair_mac[0],_ESP_pairing_reg.Pair_mac[2]))
          {
            Serial.println("[L]Clutch mac address is same with others, no clutch reading will apply");
          }
          else
          {
            memcpy(&Clu_mac,&_ESP_pairing_reg.Pair_mac[i],6);
          }
          
        }
        if(i==1)
        {
          memcpy(&Brk_mac,&_ESP_pairing_reg.Pair_mac[i],6);          
        }
        if(i==2)
        {
          if(MacCheck(_ESP_pairing_reg.Pair_mac[1],_ESP_pairing_reg.Pair_mac[2]))
          {
            Serial.println("[L]Throttle mac address is same with Brake, no Throttle reading will apply");
          }
          else
          {
            memcpy(&Gas_mac,&_ESP_pairing_reg.Pair_mac[i],6);
          }          
        }        
        if(i==3)
        {
          memcpy(&esp_Host,&_ESP_pairing_reg.Pair_mac[i],6);
        }        
      }
    }
    #endif
    
    Serial.printf("[L]BRK Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Brk_mac[0], Brk_mac[1], Brk_mac[2], Brk_mac[3], Brk_mac[4], Brk_mac[5]);
    if(ESPNow.add_peer(Brk_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add BRK Mac");
    }
    Serial.printf("[L]GAS Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Gas_mac[0], Gas_mac[1], Gas_mac[2], Gas_mac[3], Gas_mac[4], Gas_mac[5]);
    if(ESPNow.add_peer(Gas_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add Throttle Mac");
    }
    Serial.printf("[L]CLU Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Clu_mac[0], Clu_mac[1], Clu_mac[2], Clu_mac[3], Clu_mac[4], Clu_mac[5]);
    if(ESPNow.add_peer(Clu_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add Clutch Mac");
    }     
    Serial.printf("[L]HOST Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Host[0], esp_Host[1], esp_Host[2], esp_Host[3], esp_Host[4], esp_Host[5]); 
    if(ESPNow.add_peer(esp_Host)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("Sucess to add host peer");
    }

    if(ESPNow.add_peer(broadcast_mac)== ESP_OK)
    {
      ESPNOW_status=true;
      Serial.println("[L]Sucess to add broadcast peer");
    }
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    //rssi calculate
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    ESPNow_initial_status=true;
    Serial.println("[L]ESPNow Initialized");
  
}
