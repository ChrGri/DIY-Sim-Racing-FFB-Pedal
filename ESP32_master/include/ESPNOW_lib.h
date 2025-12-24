#pragma once
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "esp_now.h"
#include "ESPNowW.h"
#include "Main.h"
#include <list>
#include <iterator>

//#define ESPNow_debug
#define ESPNOW_LOG_MAGIC_KEY 0x99
#define ESPNOW_LOG_MAGIC_KEY_2 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY 0x99
#define MAX_CAPACITY_OF_SCAN_PEDAL 3
#define TIMEOUT_OF_UNASSIGNED_SCAN 1000
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
int32_t rssi[3]={0,0,0};
//bool MAC_get=false;
bool ESPNOW_status =false;
bool ESPNow_initial_status=false;
bool ESPNow_update= false;
bool ESPNow_no_device=false;
bool update_basic_state[3]={false,false,false};
bool update_extend_state[3]={false,false,false};
bool sendAssignment_b[3] = {false, false, false};
bool pedal_OTA_action_b=false;
uint16_t Joystick_value[]={0,0,0};
uint16_t Joystick_throttle_value_from_pedal=0;
uint16_t Joystick_value_original[]={0,0,0};
bool ESPNow_request_config_b[3]={false,false,false};
bool ESPNow_error_b[3]={false,false,false};
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
bool newUnassignedPedalDetected[3]={false,false,false};
QueueHandle_t messageQueueHandle;

int16_t Uint16ToInt16Cnvertor(uint16_t unsignedValue)
{
  const uint16_t OFFSET = 0x8000;
  int16_t tmp = int16_t(unsignedValue-OFFSET);
  return tmp;
}

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


typedef struct ESP_pairing_reg
{
  uint8_t Pair_status[4];
  uint8_t Pair_mac[4][6];
} ESP_pairing_reg;

struct UnassignedPeer 
{
  uint8_t mac[6];
  unsigned long lastSeen; 
  bool peerAdded;
};

typedef struct ESPNOW_Message{
  char text[240];
} ESPNOW_Message;

ESP_pairing_reg _ESP_pairing_reg;
std::list<UnassignedPeer> unassignedPeersList;

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

void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
  //only get mac in pairing
  if(ESPNow_pairing_action_b)
  {
    ESPNow_Pairing_callback(esp_now_info->src_addr, data, data_len);
  }

  //assignment request handling
  if(data_len==sizeof(DAP_AssignmentBoardcast_st) && 
  memcmp(esp_now_info->src_addr,Clu_mac,6)!=0 &&
  memcmp(esp_now_info->src_addr,Brk_mac,6)!=0 &&
  memcmp(esp_now_info->src_addr,Gas_mac,6)!=0)
  {
    DAP_AssignmentBoardcast_st dap_assignmentboardcast_st_lcl;
    memcpy(&dap_assignmentboardcast_st_lcl, data, sizeof(DAP_AssignmentBoardcast_st));
    bool structChecker=true;
    if(dap_assignmentboardcast_st_lcl.payLoadHeader_.version!=DAP_VERSION_CONFIG) structChecker=false;
    if(dap_assignmentboardcast_st_lcl.payLoadHeader_.payloadType!=DAP_PAYLOAD_TYPE_ASSIGNMENT) structChecker=false;
    uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_assignmentboardcast_st_lcl.payLoadHeader_)), sizeof(dap_assignmentboardcast_st_lcl.payLoadHeader_) + sizeof(dap_assignmentboardcast_st_lcl.payloadAssignmentRequest_));
    if(crcChecker!=dap_assignmentboardcast_st_lcl.payloadFooter_.checkSum) structChecker=false;
    if(structChecker)
    {
      int connectedPedalNumber=dap_bridge_state_st.payloadBridgeState_.Pedal_availability[0]+dap_bridge_state_st.payloadBridgeState_.Pedal_availability[1]+dap_bridge_state_st.payloadBridgeState_.Pedal_availability[2];
      int maxScanAllowance=MAX_CAPACITY_OF_SCAN_PEDAL-connectedPedalNumber;

      bool found = false;
      for (UnassignedPeer &peer : unassignedPeersList) 
      {
        if (memcmp(peer.mac, esp_now_info->src_addr, 6) == 0) 
        {
          peer.lastSeen = millis();
          found = true;
          break;
        }
      }
      if (!found) 
      {
        //ActiveSerial->println("[L]get assignment request");
        if (unassignedPeersList.size() < maxScanAllowance) 
        {
          UnassignedPeer newPeer;
          memcpy(newPeer.mac, esp_now_info->src_addr, 6);
          newPeer.lastSeen = millis();
          newPeer.peerAdded = false;
          unassignedPeersList.push_back(newPeer);
          //ESPNow.add_peer(esp_now_info->src_addr);
        }

      }
    }

  }
  //only recieve the package from registed mac address
  if(MacCheck((uint8_t*)esp_now_info->src_addr, Clu_mac)||MacCheck((uint8_t*)esp_now_info->src_addr, Brk_mac)||MacCheck((uint8_t*)esp_now_info->src_addr, Gas_mac))
  {
    if(data[0]==DAP_PAYLOAD_TYPE_ESPNOW_LOG && data[1]==ESPNOW_LOG_MAGIC_KEY && data[2]==ESPNOW_LOG_MAGIC_KEY_2)
    {

      Dap_hidmessage_st receivedMsg;
      //getESPNOWLog_b = true;
      int copyLen = data[3];
      if (copyLen> sizeof(receivedMsg.text)) copyLen = sizeof(receivedMsg.text);
      if (copyLen > 0)
      {

        memset(receivedMsg.text, 0, sizeof(receivedMsg.text));
        receivedMsg.payloadType=DAP_PAYLOAD_TYPE_ESPNOW_LOG;
        receivedMsg.magicKey1 = ESPNOW_LOG_MAGIC_KEY;
        receivedMsg.magicKey2=ESPNOW_LOG_MAGIC_KEY_2;
        receivedMsg.length= copyLen;
        memcpy(receivedMsg.text, &data[4], copyLen);
        receivedMsg.text[copyLen] = '\0';
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(messageQueueHandle, &receivedMsg, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken)
        {
          portYIELD_FROM_ISR();
        }
      }
    }
    if(data_len==sizeof(DAP_state_basic_st))
    {
      
      //Joystick_value[dap_state_basic_st.payLoadHeader_.PedalTag]=dap_state_basic_st.payloadPedalState_Basic_.joystickOutput_u16;
      DAP_state_basic_st dap_state_basic_st_lcl;
      memcpy(&dap_state_basic_st_lcl, data, sizeof(DAP_state_basic_st));
      bool structChecker=true;
      if(dap_state_basic_st_lcl.payLoadHeader_.version!=DAP_VERSION_CONFIG) structChecker=false;
      if(dap_state_basic_st_lcl.payLoadHeader_.payloadType!=DAP_PAYLOAD_TYPE_STATE_BASIC) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl.payLoadHeader_)), sizeof(dap_state_basic_st_lcl.payLoadHeader_) + sizeof(dap_state_basic_st_lcl.payloadPedalState_Basic_));
      if(crcChecker!=dap_state_basic_st_lcl.payloadFooter_.checkSum) structChecker=false;
      
      //fill the joystick value
      if(structChecker)
      {
        uint8_t pedalTag=dap_state_basic_st_lcl.payLoadHeader_.PedalTag;
        memcpy(&dap_state_basic_st[pedalTag], data, sizeof(DAP_state_basic_st));
        update_basic_state[pedalTag]=true;
        if(dap_state_basic_st_lcl.payloadPedalState_Basic_.error_code_u8!=0) ESPNow_error_b[pedalTag]=true;
        float joystickData_u32= dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16/32767.0f*10000.0f;
        uint16_t joystickNormalizedToInt16 = dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16; 
        switch (pedalTag)
        {
          case PEDAL_ID_CLUTCH:
            pedal_cluth_value=joystickNormalizedToInt16;
            Joystick_value[0]=joystickData_u32;
            Joystick_value_original[0] = dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16;
            break;
          case PEDAL_ID_BRAKE:
            pedal_brake_value=joystickNormalizedToInt16;
            Joystick_value[1]=joystickData_u32;
            Joystick_value_original[1] = dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16;
            break;
          case PEDAL_ID_THROTTLE:
            pedal_throttle_value=joystickNormalizedToInt16;
            Joystick_value[2]=joystickData_u32;
            Joystick_value_original[2] = dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16;
            pedal_status=dap_state_basic_st[pedalTag].payloadPedalState_Basic_.pedalStatus;//control pedal status only by Throttle
            Joystick_throttle_value_from_pedal=dap_state_basic_st[pedalTag].payloadPedalState_Basic_.joystickOutput_u16;
          break;
          default:
          break;
        }

      }
    }

    if(data_len==sizeof(DAP_state_extended_st))
    {
      DAP_state_extended_st dap_state_extend_st_lcl;
      memcpy(&dap_state_extend_st_lcl, data, sizeof(DAP_state_extended_st));
      bool structChecker=true;      
      uint8_t pedalTag=dap_state_extend_st_lcl.payLoadHeader_.PedalTag;
      if(dap_state_extend_st_lcl.payLoadHeader_.version!=DAP_VERSION_CONFIG) structChecker=false;
      if(dap_state_extend_st_lcl.payLoadHeader_.payloadType!=DAP_PAYLOAD_TYPE_STATE_EXTENDED) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_extend_st_lcl.payLoadHeader_)), sizeof(dap_state_extend_st_lcl.payLoadHeader_) + sizeof(dap_state_extend_st_lcl.payloadPedalState_Extended_));
      if(crcChecker!=dap_state_extend_st_lcl.payloadFooter_.checkSum) structChecker=false;
      if(structChecker)
      {
        memcpy(&dap_state_extended_st[pedalTag], data, sizeof(DAP_state_extended_st));
        update_extend_state[pedalTag]=true;
      }

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
    const uint8_t *addr_SOURCE = payload + 10;  // 傳送端 MAC
    uint8_t addr_package[6];
    memcpy(addr_package, addr_SOURCE, 6);
    if(MacCheck(addr_package, Clu_mac))
    {
      rssi[0]=ppkt->rx_ctrl.rssi;
      rssi_display=rssi[0];
    }
    if(MacCheck(addr_package, Brk_mac))
    {
      rssi[1]=ppkt->rx_ctrl.rssi;
      rssi_display=rssi[1];
    }
    if(MacCheck(addr_package, Gas_mac))
    {
      rssi[2]=ppkt->rx_ctrl.rssi;
      rssi_display=rssi[2];
    }
  }
  
  //int rssi = ppkt->rx_ctrl.rssi;
  //rssi_display = rssi;
  
}

void ESPNow_initialize()
{

    WiFi.mode(WIFI_MODE_STA);
    ActiveSerial->println("[L]Initializing Wifi."); 
    delay(1000);
    WiFi.macAddress(esp_Mac); 
    ActiveSerial->printf("[L]Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Mac[0], esp_Mac[1], esp_Mac[2], esp_Mac[3], esp_Mac[4], esp_Mac[5]);
    
    //ActiveSerial->print("Current MAC Address:  ");  
    //ActiveSerial->println(WiFi.macAddress());
    #ifndef ESPNow_Pairing_function
      ActiveSerial->println("Overwriting Mac address");
      esp_wifi_set_mac(WIFI_IF_STA, &esp_Host[0]);
      delay(300);
      ActiveSerial->print("[L]Modified MAC Address:  ");  
      ActiveSerial->println(WiFi.macAddress());
    #endif
    ActiveSerial->println("[L]Initializing ESP-NOW");
    ESPNow.init();
    delay(3000);
    #ifdef Using_Board_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_MCS0_LGI);
    #endif
    
    #ifdef Using_Board_ESP32S3
    //esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);
    //esp_wifi_config_espnow_rate(WIFI_IF_STA, 	WIFI_PHY_RATE_11M_L);
      #ifdef LOW_TX_POWER
      esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
      ActiveSerial->println("[L]Setting Wifi strength to 8.5dbm ");
      #endif
    #endif
    //reading from eeprom
    #ifdef ESPNow_Pairing_function
    ESP_pairing_reg ESP_pairing_reg_local;
    EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
    memcpy(&_ESP_pairing_reg, &ESP_pairing_reg_local,sizeof(ESP_pairing_reg));
    //_ESP_pairing_reg=ESP_pairing_reg_local;
    //EEPROM.get(EEPROM_offset, _ESP_pairing_reg);
    ActiveSerial->print("[L]");
    for(int i=0;i<4;i++)
    { 
      if(_ESP_pairing_reg.Pair_status[i]==1)
      {
        ActiveSerial->print("Paired Device #");
        ActiveSerial->print(i);
        //ActiveSerial->print(" Pair: ");
        //ActiveSerial->print(_ESP_pairing_reg.Pair_status[i]);
        ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", _ESP_pairing_reg.Pair_mac[i][0], _ESP_pairing_reg.Pair_mac[i][1], _ESP_pairing_reg.Pair_mac[i][2], _ESP_pairing_reg.Pair_mac[i][3], _ESP_pairing_reg.Pair_mac[i][4], _ESP_pairing_reg.Pair_mac[i][5]);
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
            ActiveSerial->println("[L]Clutch mac address is same with others, no clutch reading will apply");
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
            ActiveSerial->println("[L]Throttle mac address is same with Brake, no Throttle reading will apply");
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
    bool addPeerCHecker= true;
    //ActiveSerial->printf("[L]BRK Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Brk_mac[0], Brk_mac[1], Brk_mac[2], Brk_mac[3], Brk_mac[4], Brk_mac[5]);
    if(ESPNow.add_peer(Brk_mac)!= ESP_OK) addPeerCHecker=false;
    //ActiveSerial->printf("[L]GAS Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Gas_mac[0], Gas_mac[1], Gas_mac[2], Gas_mac[3], Gas_mac[4], Gas_mac[5]);
    if(ESPNow.add_peer(Gas_mac)!= ESP_OK) addPeerCHecker=false;
    //ActiveSerial->printf("[L]CLU Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Clu_mac[0], Clu_mac[1], Clu_mac[2], Clu_mac[3], Clu_mac[4], Clu_mac[5]);
    if(ESPNow.add_peer(Clu_mac)!= ESP_OK) addPeerCHecker=false;    
    //ActiveSerial->printf("[L]HOST Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", esp_Host[0], esp_Host[1], esp_Host[2], esp_Host[3], esp_Host[4], esp_Host[5]); 
    if(ESPNow.add_peer(esp_Host)!= ESP_OK) addPeerCHecker=false;
    if(ESPNow.add_peer(broadcast_mac)!= ESP_OK) addPeerCHecker=false;
    if(addPeerCHecker) ActiveSerial->println("[L]Peers added successfully.");
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(OnSent);
    //rssi calculate
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    ESPNow_initial_status=true;
    ESPNOW_status=true;
    ActiveSerial->println("[L]ESPNow Initialized");
  
}
void print_struct_hex(DAP_bridge_state_st* s) {
    const uint8_t* p = (const uint8_t*)s;
    for (size_t i = 0; i < sizeof(DAP_bridge_state_st); i++) 
    {
      ActiveSerial->print("0x");  
      if (p[i] < 16) ActiveSerial->print('0');
      ActiveSerial->print(p[i], HEX);
      ActiveSerial->print("-");
    }
    ActiveSerial->println("");
}


void checkAndRemoveTimeoutUnssignedPedal() 
{
  unsigned long currentTime = millis();
  auto it = unassignedPeersList.begin();
  while (it != unassignedPeersList.end())
  { 
    if (currentTime - it->lastSeen > TIMEOUT_OF_UNASSIGNED_SCAN) 
    {
      ActiveSerial->println("[L]Unassigned pedal timeout and removed");
      uint8_t mac[6]={0};
      memcpy(mac, it->mac, 6);
      it = unassignedPeersList.erase(it);
      ActiveSerial->print("[L]List size AFTER removal: ");
      ActiveSerial->println(unassignedPeersList.size());
      esp_err_t result = esp_now_del_peer(mac);
      if (result == ESP_OK) 
      {
        ActiveSerial->println("[L]ESPNow peer removed successfully.");
      } 
      else 
      {
        ActiveSerial->println("[L]Failed to remove ESPNow peer.");
      }
    } 
    else ++it;
  }
}