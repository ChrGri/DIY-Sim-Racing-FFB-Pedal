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
#define ESPNOW_LOG_MAGIC_KEY_U8 0x99
#define ESPNOW_LOG_MAGIC_KEY_2_U8 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY_U8 0x99
#define MAX_CAPACITY_OF_SCAN_PEDAL_U8 3
#define TIMEOUT_OF_UNASSIGNED_SCAN_U32 1000
uint8_t g_espMaster_au8[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x31};
uint8_t g_pedalMac_aau8[3][6] = {
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x32},
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x34},
    {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}
};
uint8_t g_broadcastMac_au8[]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t g_espHost_au8[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x35};
uint8_t g_espMac_au8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t* g_recvMac_pu8;
uint16_t g_espNowSend_u16=0;
uint16_t g_espNowReceive_u16=0;
int g_rssiDisplay_i32;
int32_t g_rssi_ai32[3]={0,0,0};
//bool MAC_get=false;
bool g_espNowStatus_b =false;
bool g_espNowInitialStatus_b=false;
bool g_espNowUpdate_b= false;
bool g_espNowNoDevice_b=false;
bool g_updateBasicState_ab[3]={false,false,false};
bool g_updateExtendState_ab[3]={false,false,false};
bool g_sendAssignment_ab[3] = {false, false, false};
bool g_pedalOtaAction_b=false;
uint16_t g_joystickValue_au16[]={0,0,0};
uint16_t g_joystickThrottleValueFromPedal_u16=0;
uint16_t g_joystickValueOriginal_au16[]={0,0,0};
unsigned long g_pedalLastUpdate_au32[3]={1,1,1};
bool g_espNowRequestConfig_ab[3]={false,false,false};
bool g_espNowError_ab[3]={false,false,false};
uint16_t g_pedalThrottleValue_u16=0;
uint16_t g_pedalBrakeValue_u16=0;
uint16_t g_pedalClutchValue_u16=0;
uint16_t g_pedalBrakeRudderValue_u16=0;
uint16_t g_pedalThrottleRudderValue_u16=0;
uint8_t g_pedalStatus_u8=0;
bool g_espNowPairingStatus_b = false;
bool g_updatePairingToEeprom_b = false;
bool g_espNowPairingAction_b = false;
bool g_softwarePairingAction_b = false;
bool g_newUnassignedPedalDetected_ab[3]={false,false,false};
QueueHandle_t g_messageQueueHandle_pv;

extern DAP_servo_config_st_t dap_servo_config_response_st[3];
extern bool send_servo_config_to_host[3];

int16_t uint16ToInt16Convertor(uint16_t unsignedValue)
{
  const uint16_t OFFSET = 0x8000;
  int16_t tmp = int16_t(unsignedValue-OFFSET);
  return tmp;
}

bool macCheck(uint8_t* Mac_A, uint8_t* Mac_B)
{
  return memcmp(Mac_A, Mac_B, 6) == 0;
}


typedef struct EspPairingReg_t

{
  uint8_t pairStatus_au8[4];
  uint8_t pairMac_aau8[4][6];
} EspPairingReg_t;

struct UnassignedPeer_t 

{
  uint8_t mac[6];
  unsigned long lastSeen; 
  bool peerAdded;
};

typedef struct EspNowMessage_t
{
  char text_ac[240];
} EspNowMessage_t;

EspPairingReg_t g_espPairingReg_st;
std::list<UnassignedPeer_t> g_unassignedPeersList;

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
    //bridge and analog device
    if(dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==99||dap_esppairing_st.payloadEspnowInfo_st.deviceId_u8==98)
    {
      memcpy(&g_espPairingReg_st.pairMac_aau8[3], mac_addr , 6);
      g_espPairingReg_st.pairStatus_au8[3]=1;
      g_updatePairingToEeprom_b = true;
    }
  }


}

void onRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len)
{
  //only get mac in pairing
  if(g_espNowPairingAction_b)
  {
    espNowPairingCallback(esp_now_info->src_addr, data, data_len);
  }

  //assignment request handling
  if(data_len==sizeof(DapAssignmentBroadcast_t) && 
  memcmp(esp_now_info->src_addr,g_pedalMac_aau8[0],6)!=0 &&
  memcmp(esp_now_info->src_addr,g_pedalMac_aau8[1],6)!=0 &&
  memcmp(esp_now_info->src_addr,g_pedalMac_aau8[2],6)!=0)
  {
    DapAssignmentBroadcast_t dap_assignmentboardcast_st_lcl;
    memcpy(&dap_assignmentboardcast_st_lcl, data, sizeof(DapAssignmentBroadcast_t));
    bool structChecker=true;
    if(dap_assignmentboardcast_st_lcl.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
    if(dap_assignmentboardcast_st_lcl.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_ASSIGNMENT_U8) structChecker=false;
    uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_assignmentboardcast_st_lcl.payloadHeader_st)), sizeof(dap_assignmentboardcast_st_lcl.payloadHeader_st) + sizeof(dap_assignmentboardcast_st_lcl.payloadAssignmentRequest_st));
    if(crcChecker!=dap_assignmentboardcast_st_lcl.payloadFooter_st.checkSum_u16) structChecker=false;
    if(structChecker)
    
{
      int connectedPedalNumber=dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[0]+dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[1]+dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[2];
      int maxScanAllowance=MAX_CAPACITY_OF_SCAN_PEDAL_U8-connectedPedalNumber;

      bool found = false;
      for (UnassignedPeer_t &peer : g_unassignedPeersList) 
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
        if (g_unassignedPeersList.size() < maxScanAllowance) 
        {
          UnassignedPeer_t newPeer;
          memcpy(newPeer.mac, esp_now_info->src_addr, 6);
          newPeer.lastSeen = millis();
          newPeer.peerAdded = false;
          g_unassignedPeersList.push_back(newPeer);
          //ESPNow.add_peer(esp_now_info->src_addr);
        }

      }
    }

  }
  //only recieve the package from registed mac address
  if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[0])||macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[1])||macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[2]))
  {
    if(esp_now_info->rx_ctrl != NULL)
 {
      if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[0]))
 {
        g_rssi_ai32[0] = esp_now_info->rx_ctrl->rssi;
        g_rssiDisplay_i32 = g_rssi_ai32[0];
      } else if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[1]))
 {
        g_rssi_ai32[1] = esp_now_info->rx_ctrl->rssi;
        g_rssiDisplay_i32 = g_rssi_ai32[1];
      } else if(macCheck((uint8_t*)esp_now_info->src_addr, g_pedalMac_aau8[2]))
 {
        g_rssi_ai32[2] = esp_now_info->rx_ctrl->rssi;
        g_rssiDisplay_i32 = g_rssi_ai32[2];
      }
    }
    if(data[0]==DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8 && data[1]==ESPNOW_LOG_MAGIC_KEY_U8 && data[2]==ESPNOW_LOG_MAGIC_KEY_2_U8)
    {

      PayloadHidMessage_t receivedMsg;
      //getESPNOWLog_b = true;
      int copyLen = data[3];
      if (copyLen >= sizeof(receivedMsg.text_ac)) copyLen = sizeof(receivedMsg.text_ac) - 1;
      if (copyLen > 0)
      {

        memset(receivedMsg.text_ac, 0, sizeof(receivedMsg.text_ac));
        receivedMsg.payloadType_u8=DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
        receivedMsg.magicKey1_u8 = ESPNOW_LOG_MAGIC_KEY_U8;
        receivedMsg.magicKey2_u8=ESPNOW_LOG_MAGIC_KEY_2_U8;
        receivedMsg.length_u8= copyLen;
        memcpy(receivedMsg.text_ac, &data[4], copyLen);
        receivedMsg.text_ac[copyLen] = '\0';
        xQueueSend(g_messageQueueHandle_pv, &receivedMsg, 0);
      }
    }
    if(data_len==sizeof(DapStateBasic_t))
    {
      
      //g_joystickValue_au16[dap_state_basic_st.payloadHeader_st.pedalTag_u8]=dap_state_basic_st.payloadPedalStateBasic_st.joystickOutput_u16;
      DapStateBasic_t dap_state_basic_st_lcl;
      memcpy(&dap_state_basic_st_lcl, data, sizeof(DapStateBasic_t));
      bool structChecker=true;
      if(dap_state_basic_st_lcl.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(dap_state_basic_st_lcl.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_STATE_BASIC_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl.payloadHeader_st)), sizeof(dap_state_basic_st_lcl.payloadHeader_st) + sizeof(dap_state_basic_st_lcl.payloadPedalStateBasic_st));
      if(crcChecker!=dap_state_basic_st_lcl.payloadFooter_st.checkSum_u16) structChecker=false;
      
      //fill the joystick value
      if(structChecker)
      
{
        uint8_t pedalTag=dap_state_basic_st_lcl.payloadHeader_st.pedalTag_u8;
        if(pedalTag < 3)
 {
          memcpy(&dap_state_basic_st[pedalTag], data, sizeof(DapStateBasic_t));
          g_updateBasicState_ab[pedalTag]=true;
          g_pedalLastUpdate_au32[pedalTag]=millis();
          if(dap_state_basic_st_lcl.payloadPedalStateBasic_st.errorCode_u8!=0) g_espNowError_ab[pedalTag]=true;
          float joystickData_u32= dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16/32767.0f*10000.0f;
          uint16_t joystickNormalizedToInt16 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16; 
          switch (pedalTag)
          {
            case PEDAL_ID_CLUTCH:
              g_pedalClutchValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[0]=joystickData_u32;
              g_joystickValueOriginal_au16[0] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              break;
            case PEDAL_ID_BRAKE:
              g_pedalBrakeValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[1]=joystickData_u32;
              g_joystickValueOriginal_au16[1] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              break;
            case PEDAL_ID_THROTTLE:
              g_pedalThrottleValue_u16=joystickNormalizedToInt16;
              g_joystickValue_au16[2]=joystickData_u32;
              g_joystickValueOriginal_au16[2] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
              g_pedalStatus_u8=dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.pedalStatus_u8;//control pedal status only by Throttle
              g_joystickThrottleValueFromPedal_u16=dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
            break;
            default:
            break;
          }
        }
      }
    }

    if(data_len==sizeof(DapStateExtended_t))
    {
      DapStateExtended_t dap_state_extend_st_lcl;
      memcpy(&dap_state_extend_st_lcl, data, sizeof(DapStateExtended_t));
      bool structChecker=true;      
      uint8_t pedalTag=dap_state_extend_st_lcl.payloadHeader_st.pedalTag_u8;
      if(dap_state_extend_st_lcl.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(dap_state_extend_st_lcl.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_STATE_EXTENDED_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(dap_state_extend_st_lcl.payloadHeader_st)), sizeof(dap_state_extend_st_lcl.payloadHeader_st) + sizeof(dap_state_extend_st_lcl.payloadPedalStateExtended_st));
      if(crcChecker!=dap_state_extend_st_lcl.payloadFooter_st.checkSum_u16) structChecker=false;
      if(structChecker)
      
{
        if(pedalTag < 3)
 {
          memcpy(&dap_state_extended_st[pedalTag], data, sizeof(DapStateExtended_t));
          g_updateExtendState_ab[pedalTag]=true;
        }
      }

    }

    if(data_len==sizeof(DapConfig_t))
    {
      memcpy(&dap_config_st_Temp, data, sizeof(DapConfig_t));
      if(dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8 < 3)
 {
        g_espNowRequestConfig_ab[dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8]=true;
        if(dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8==0)
        {
          memcpy(&dap_config_st_Clu, &dap_config_st_Temp, sizeof(DapConfig_t));
        }
        if(dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8==1)
        {
          memcpy(&dap_config_st_Brk, &dap_config_st_Temp, sizeof(DapConfig_t));
        }
        if(dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8==2)
        {
          memcpy(&dap_config_st_Gas, &dap_config_st_Temp, sizeof(DapConfig_t));
        }
      }
    }

    if(data_len==sizeof(DAP_servo_config_st_t))
    {
      DAP_servo_config_st_t received_servo_config;
      memcpy(&received_servo_config, data, sizeof(DAP_servo_config_st_t));
      bool structChecker=true;
      if(received_servo_config.payloadHeader_st.version_u8!=DAP_VERSION_CONFIG_U8) structChecker=false;
      if(received_servo_config.payloadHeader_st.payloadType_u8!=DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8) structChecker=false;
      uint16_t crcChecker = checksumCalculator((uint8_t*)(&(received_servo_config.payloadHeader_st)), sizeof(received_servo_config.payloadHeader_st) + sizeof(received_servo_config.payloadServoConfig_st));
      if(crcChecker!=received_servo_config.payloadFooter_st.checkSum_u16) structChecker=false;
      
      if(structChecker)
      
{
        uint8_t pedalTag = received_servo_config.payloadHeader_st.pedalTag_u8;
        if(pedalTag < 3)
 {
          memcpy(&dap_servo_config_response_st[pedalTag], &received_servo_config, sizeof(DAP_servo_config_st_t));
          send_servo_config_to_host[pedalTag] = true;
        }
      }
    }
  }
  


}
void onSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{

}

// The callback that does the magic
void promiscuousRxCb(void *buf, wifi_promiscuous_pkt_type_t type)
 {
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
    const uint8_t *addr_SOURCE = payload + 10;  // å‚³é€ ç«¯ MAC
    uint8_t addr_package[6];
    memcpy(addr_package, addr_SOURCE, 6);
    if (macCheck(addr_package, g_pedalMac_aau8[0]))
    {
      g_rssi_ai32[0]=ppkt->rx_ctrl.rssi;
      g_rssiDisplay_i32=g_rssi_ai32[0];
    }
    if (macCheck(addr_package, g_pedalMac_aau8[1]))
    {
      g_rssi_ai32[1]=ppkt->rx_ctrl.rssi;
      g_rssiDisplay_i32=g_rssi_ai32[1];
    }
    if (macCheck(addr_package, g_pedalMac_aau8[2]))
    {
      g_rssi_ai32[2]=ppkt->rx_ctrl.rssi;
      g_rssiDisplay_i32=g_rssi_ai32[2];
    }
  }
  
  //int g_rssi_ai32 = ppkt->rx_ctrl.g_rssi_ai32;
  //g_rssiDisplay_i32 = g_rssi_ai32;
  
}

void espNowInitialize()
{

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(true, true);
    WiFi.setSleep(false);
    ActiveSerial->println("[L]Initializing Wifi."); 
    delay(1000);
    WiFi.macAddress(g_espMac_au8); 
    ActiveSerial->printf("[L]Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espMac_au8[0], g_espMac_au8[1], g_espMac_au8[2], g_espMac_au8[3], g_espMac_au8[4], g_espMac_au8[5]);
    
    //ActiveSerial->print("Current MAC Address:  ");  
    //ActiveSerial->println(WiFi.macAddress());
    #ifndef ESPNow_Pairing_function
      ActiveSerial->println("Overwriting Mac address");
      esp_wifi_set_mac(WIFI_IF_STA, &g_espHost_au8[0]);
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
    EspPairingReg_t ESP_pairing_reg_local;
    EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
    memcpy(&g_espPairingReg_st, &ESP_pairing_reg_local,sizeof(EspPairingReg_t));
    //g_espPairingReg_st=ESP_pairing_reg_local;
    //EEPROM.get(EEPROM_offset, g_espPairingReg_st);
    ActiveSerial->print("[L]");
    for(int i=0;i<4;i++)
    { 
      if(g_espPairingReg_st.pairStatus_au8[i]==1)
      {
        ActiveSerial->print("Paired Device #");
        ActiveSerial->print(i);
        //ActiveSerial->print(" Pair: ");
        //ActiveSerial->print(g_espPairingReg_st.pairStatus_au8[i]);
        ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espPairingReg_st.pairMac_aau8[i][0], g_espPairingReg_st.pairMac_aau8[i][1], g_espPairingReg_st.pairMac_aau8[i][2], g_espPairingReg_st.pairMac_aau8[i][3], g_espPairingReg_st.pairMac_aau8[i][4], g_espPairingReg_st.pairMac_aau8[i][5]);
      }           
    }
    
    for(int i=0; i<4;i++)
    {
      if(g_espPairingReg_st.pairStatus_au8[i]==1)
      {
        if(i==0)
        {
          if(macCheck(g_espPairingReg_st.pairMac_aau8[0],g_espPairingReg_st.pairMac_aau8[1])||macCheck(g_espPairingReg_st.pairMac_aau8[0],g_espPairingReg_st.pairMac_aau8[2]))
          {
            ActiveSerial->println("[L]Clutch mac address is same with others, no clutch reading will apply");
          }
          else
          {
            memcpy(&g_pedalMac_aau8[0],&g_espPairingReg_st.pairMac_aau8[i],6);
          }
          
        }
        if(i==1)
        {
          memcpy(&g_pedalMac_aau8[1],&g_espPairingReg_st.pairMac_aau8[i],6);          
        }
        if(i==2)
        {
          if(macCheck(g_espPairingReg_st.pairMac_aau8[1],g_espPairingReg_st.pairMac_aau8[2]))
          {
            ActiveSerial->println("[L]Throttle mac address is same with Brake, no Throttle reading will apply");
          }
          else
          {
            memcpy(&g_pedalMac_aau8[2],&g_espPairingReg_st.pairMac_aau8[i],6);
          }          
        }        
        if(i==3)
        {
          memcpy(&g_espHost_au8,&g_espPairingReg_st.pairMac_aau8[i],6);
        }        
      }
    }
    #endif
    bool addPeerCHecker= true;
    //ActiveSerial->printf("[L]BRK Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_pedalMac_aau8[1][0], g_pedalMac_aau8[1][1], g_pedalMac_aau8[1][2], g_pedalMac_aau8[1][3], g_pedalMac_aau8[1][4], g_pedalMac_aau8[1][5]);
    if(ESPNow.add_peer(g_pedalMac_aau8[1])!= ESP_OK) addPeerCHecker=false;
    //ActiveSerial->printf("[L]GAS Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_pedalMac_aau8[2][0], g_pedalMac_aau8[2][1], g_pedalMac_aau8[2][2], g_pedalMac_aau8[2][3], g_pedalMac_aau8[2][4], g_pedalMac_aau8[2][5]);
    if(ESPNow.add_peer(g_pedalMac_aau8[2])!= ESP_OK) addPeerCHecker=false;
    //ActiveSerial->printf("[L]CLU Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_pedalMac_aau8[0][0], g_pedalMac_aau8[0][1], g_pedalMac_aau8[0][2], g_pedalMac_aau8[0][3], g_pedalMac_aau8[0][4], g_pedalMac_aau8[0][5]);
    if(ESPNow.add_peer(g_pedalMac_aau8[0])!= ESP_OK) addPeerCHecker=false;    
    //ActiveSerial->printf("[L]HOST Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_espHost_au8[0], g_espHost_au8[1], g_espHost_au8[2], g_espHost_au8[3], g_espHost_au8[4], g_espHost_au8[5]); 
    if(ESPNow.add_peer(g_espHost_au8)!= ESP_OK) addPeerCHecker=false;
    if(ESPNow.add_peer(g_broadcastMac_au8)!= ESP_OK) addPeerCHecker=false;
    if(addPeerCHecker) ActiveSerial->println("[L]Peers added successfully.");
    ESPNow.reg_recv_cb(onRecv);
    ESPNow.reg_send_cb(onSent);
    //set wifi channel
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    //g_rssi_ai32 calculate
    // esp_wifi_set_promiscuous(true);
    // esp_wifi_set_promiscuous_rx_cb(&promiscuousRxCb);
    g_espNowInitialStatus_b=true;
    g_espNowStatus_b=true;
    ActiveSerial->println("[L]ESPNow Initialized");
  
}
void printStructHex(DapBridgeState_t* s)
 {
    const uint8_t* p = (const uint8_t*)s;
    for (size_t i = 0; i < sizeof(DapBridgeState_t); i++) 
    {
      ActiveSerial->print("0x");  
      if (p[i] < 16) ActiveSerial->print('0');
      ActiveSerial->print(p[i], HEX);
      ActiveSerial->print("-");
    }
    ActiveSerial->println("");
}


void checkAndRemoveTimeoutUnassignedPedal() 
{
  unsigned long currentTime = millis();
  auto it = g_unassignedPeersList.begin();
  while (it != g_unassignedPeersList.end())
  { 
    if (currentTime - it->lastSeen > TIMEOUT_OF_UNASSIGNED_SCAN_U32) 
    {
      ActiveSerial->println("[L]Unassigned pedal timeout and removed");
      uint8_t mac[6]={0};
      memcpy(mac, it->mac, 6);
      it = g_unassignedPeersList.erase(it);
      ActiveSerial->print("[L]List size AFTER removal: ");
      ActiveSerial->println(g_unassignedPeersList.size());
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
