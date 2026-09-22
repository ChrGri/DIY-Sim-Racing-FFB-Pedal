
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779

#define ESTIMATE_LOADCELL_VARIANCE
#define ISV_COMMUNICATION
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER 1
#define DEBUG_INFO_0_STEPPER_POS 2
#define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS 8
#define DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS 16
#define DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT 32
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT 64
#define DEBUG_INFO_0_CONTROL_LOOP_ALGO 128

#define BAUD3M 3000000
#define PI 3.14159267
#define DEG_TO_RAD PI / 180

#include "Arduino.h"
#include "Main.h"
#include "esp_system.h"
#ifndef CONFIG_IDF_TARGET_ESP32C6
  #include "soc/rtc_cntl_reg.h"
#endif
// alias to serial stream, thus it can dynamically switch depending on board
Stream *ActiveSerial = nullptr;
//Stream *ActiveSerial = nullptr;
#include "FanatecInterface.h"
#include "OTA_Pull.h"
#include "Version_Board.h"
#include <EEPROM.h>
#include <DiyActivePedal_types.h>
#include "Wire.h"
#include "SPI.h"
#include "JoystickController.h"

#include <MovingAverageFilter.h>
#include <TaskScheduler.h>


// https://www.tutorialspoint.com/cyclic-redundancy-check-crc-in-arduino
uint16_t checksumCalculator(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}

DapConfig_t dap_config_st[3];
DapCalculationVariables_t dap_calculationVariables_st;
DapStateBasic_t dap_state_basic_st[3];
DapStateExtended_t dap_state_extended_st[3];
DapActions_t dap_actions_st[3];
DapActions_t dap_actionassignment_st[3];
DapBridgeState_t dap_bridge_state_st;
DapConfig_t dap_config_st_Clu;
DapConfig_t dap_config_st_Brk;
DapConfig_t dap_config_st_Gas;
DapConfig_t dap_config_st_Temp;
DapEspPairing_t dap_esppairing_st;//saving
DapEspPairing_t dap_esppairing_lcl;//sending
//DapConfig_t dap_config_st_store[3];
DapBridgeState_t dap_bridge_state_lcl;//
DapActionOta_t dap_action_ota_st;
#include "WirelessCommunication_bridge.h"

// global variable for servo config routing ---
DAP_servo_config_st_t dap_servo_config_st[3];          // packets from host to servo
DAP_servo_config_st_t dap_servo_config_response_st[3]; // packets from servo to host (responses)
bool update_servo_config[3] = {false, false, false};
bool send_servo_config_to_host[3] = {false, false, false}; // Muss in ESPNOW_lib.cpp bei RX gesetzt werden
bool firstDebugMessage_b = false;


#define EEPROM_offset 15

bool isSerialConfigGet[3]={false, false, false};

#ifndef CONFIG_IDF_TARGET_ESP32S3
  #include <rtc_wdt.h>
#endif
#ifdef USING_LED
  #include "soc/soc_caps.h"
  #include <Adafruit_NeoPixel.h>
  #ifdef LED_ENABLE_WAVESHARE
    #define LEDS_COUNT 1
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO, NEO_RGB + NEO_KHZ800);
  #endif
  #ifdef LED_ENABLE_DONGLE
    #define LEDS_COUNT 3
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO, NEO_GRB + NEO_KHZ800);
  #endif
  #define CHANNEL 0
  #define LED_BRIGHT 30
  /*
  static const crgb_t L_RED = 0xff0000;
  static const crgb_t L_GREEN = 0x00ff00;
  static const crgb_t L_BLUE = 0x0000ff;
  static const crgb_t L_WHITE = 0xe0e0e0;
  static const crgb_t L_YELLOW = 0xffde21;
  static const crgb_t L_ORANGE = 0xffa500;
  static const crgb_t L_CYAN = 0x00ffff;
  static const crgb_t L_PURPLE = 0x800080;
  */

#endif

#ifdef Fanatec_comunication
  FanatecInterface fanatec(Fanatec_serial_RX, Fanatec_serial_TX, Fanatec_plug); // RX: GPIO18, TX: GPIO17, PLUG: GPIO16
  bool Fanatec_Mode=false;
#endif

#ifdef OTA_Update
  void otaUpdateTask(void *pvParameters);
#endif


#ifdef External_RP2040
  #include "RP2040PicoUART.h"
  RP2040PicoUART *_rp2040picoUART;
  DAP_JoystickUART_State dap_joystickUART_state_lcl;
#endif

#ifdef Using_MCP4728
  #include <Adafruit_MCP4728.h>
  Adafruit_MCP4728 mcp;
  TwoWire MCP4728_I2C= TwoWire(1);
  bool MCP_status =false;
#endif

//global variables
bool configUpdateAvailable[3] = {false, false, false};                              
  //DapConfig_t dap_config_st_local;
int32_t joystickNormalizedToInt32 = 0;                           
bool resetPedalPosition = false;
bool dap_action_update[3]= {false,false,false};
MovingAverageFilter rssi_filter(30);
int32_t joystickNormalizedToInt32_local = 0;

uint8_t pedal_avaliable[3]={0,0,0};
uint8_t LED_Status=0; //0=normal 1= pairing
TaskScheduler taskScheduler;
//task define here
void espNowCommunicationTxTask( void * pvParameters);
void joystickUpdateTask(void *pvParameters);
void ledUpdateTask( void * pvParameters);
void serialCommunicationRxTask( void * pvParameters);
void serialCommunicationTxTask( void * pvParameters);
void ledUpdateDongleTask( void * pvParameters);
void fanatecUpdateTask(void *pvParameters);
void miscTask(void *pvParameters);
void hidCommunicaitonRxTask(void *pvParameters);
void hidCommunicaitonTxTask(void *pvParameters);
void setup()
{
  #ifdef USB_JOYSTICK
    Serial1.begin(BAUD3M, SERIAL_8N1, 44, 43);
    // ActiveSerial->begin(BAUD3M, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial1;
    
  #else
    Serial.begin(BAUD3M, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial;
  #endif

  #ifdef External_RP2040
        _rp2040picoUART = new RP2040PicoUART(RP2040rxPin, RP2040txPin, handshakeGPIO, RP2040baudrate);
  #endif
  
  #if PCB_VERSION == 5 || PCB_VERSION == 6 || PCB_VERSION == 7 || PCB_VERSION == 8 || PCB_VERSION == 9
    // ActiveActiveSerial->setTxTimeoutMs(0);
    //ActiveSerial->setRxBufferSize(1024);
    //ActiveSerial->setTimeout(5);
    //ActiveSerial->begin(3000000);
  #else
      ActiveSerial->setRxBufferSize(1024);
    ActiveSerial->begin(921600);
    ActiveSerial->setTimeout(5);

  #endif
  
  // setup serial


   ActiveSerial->println(" ");
   ActiveSerial->println(" ");
   ActiveSerial->println(" ");

   ActiveSerial->println("[L]This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.");
   ActiveSerial->println("[L]Please check github repo for more detail: https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal");
  #ifdef OTA_Update
    ActiveSerial->print("[L]Board:");
    ActiveSerial->println(BRIDGE_BOARD);
    ActiveSerial->print("[L]Version:");
    ActiveSerial->println(BRIDGE_FIRMWARE_VERSION);
  #endif
  parse_version(BRIDGE_FIRMWARE_VERSION,&versionMajor,&versionMinor,&versionPatch);
  delay(5);
  #ifdef USB_JOYSTICK
    ActiveSerial->println("[L]Setup controller");
    SetupController();
    delay(500);
    tinyusbJoystick_.printf("This work is licensed under a CC-BY-NC-SA License.\nPlease check github repo for more detail: https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal\nBridge: Board:%s, Version:%s.", BRIDGE_BOARD, BRIDGE_FIRMWARE_VERSION);
    delay(500);
    taskScheduler.addScheduledTask(hidCommunicaitonRxTask, "Hid Rx", REPETITION_INTERVAL_HID_RX_TASK, TASK_PRIORITY_HID_RX_TASK, CORE_ID_HID_RX_TASK, STACK_SIZE_HID_RX_TASK);
    taskScheduler.addScheduledTask(hidCommunicaitonTxTask, "Hid Tx", REPETITION_INTERVAL_HID_TX_TASK, TASK_PRIORITY_HID_TX_TASK, CORE_ID_HID_TX_TASK, STACK_SIZE_HID_TX_TASK);
    /*
    ActiveSerial->print("[L]HID descriptor Size:");
    ActiveSerial->println(reportSize);
    ActiveSerial->print("[L]HID descriptor:");
    for(int i =0; i<reportSize;i++)
    {
      ActiveSerial->print("0x");  
      if (hidDescriptorBufferForCheck[i] < 16) ActiveSerial->print('0');
      ActiveSerial->print(hidDescriptorBufferForCheck[i], HEX);
      ActiveSerial->print("-");

    }
    ActiveSerial->println("");
    */
  #endif
  //create message queue
  g_messageQueueHandle_pv = xQueueCreate(10, sizeof(PayloadHidMessage_t));
  if (g_messageQueueHandle_pv == NULL)
  {
    ActiveSerial->println("[L]Error during xqueue creation.");
    ESP.restart();
  }
  EEPROM.begin(512);
  #ifdef ESPNow_Pairing_function
    //button read setup
    pinMode(Pairing_GPIO, INPUT_PULLUP);
  #endif
  #if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
    disableCore0WDT();
    disableCore1WDT();
  #endif
  //enable ESP-NOW
  wirelessComm.begin(loadMacAddressesFromEeprom());
  
  #ifdef Using_MCP4728
    MCP4728_I2C.begin(MCP_SDA,MCP_SCL,400000);
    uint8_t i2c_address[8]={0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67};
    int index_address=0;
    int found_address=0;
    int error;
    for(index_address=0;index_address<8;index_address++)
    {
      MCP4728_I2C.beginTransmission(i2c_address[index_address]);
      error = MCP4728_I2C.endTransmission();
      if (error == 0)
      {
        ActiveSerial->print("I2C device found at address");
        ActiveSerial->print(i2c_address[index_address]);
        ActiveSerial->println("  !");
        found_address=index_address;
        break;
        
      }
      else
      {
        ActiveSerial->print("try address");
        ActiveSerial->println(i2c_address[index_address]);
      }
    }
    
    if(mcp.begin(i2c_address[found_address], &MCP4728_I2C)==false)
    {
      ActiveSerial->println("Couldn't find MCP4728, will not have analog output");
      MCP_status=false;
    }
    else
    {
      ActiveSerial->println("MCP4728 founded");
      MCP_status=true;
      //MCP.begin();
    }
    
  #endif
        
  //initialize time_record
  uint8_t pedalIDX;
  for(pedalIDX=0;pedalIDX<3;pedalIDX++)
  {
    g_pedalLastUpdate_au32[pedalIDX]=millis();
  }
  #ifdef LED_ENABLE_WAVESHARE
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0xff,0xff);
    pixels.show();
    /*
    xTaskCreatePinnedToCore(
                          ledUpdateTask,   
                          "LED_update_Task", 
                          3000,  
                          //STACK_SIZE_FOR_TASK_2,    
                          NULL,      
                          1,         
                          &Task3,    
                          0);     
                          */
    taskScheduler.addScheduledTask(ledUpdateTask, "LED Update", REPETITION_INTERVAL_LED_UPDATE_TASK, TASK_PRIORITY_LED_UPDATE_TASK, CORE_ID_LED_UPDATE_TASK, STACK_SIZE_LED_UPDATE_TASK);
    delay(500);
  #endif
  #ifdef LED_ENABLE_DONGLE
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0xff,0xff);
    pixels.show();
    /*
    xTaskCreatePinnedToCore(
                          LED_Task_Dongle,   
                          "LED_update_Task", 
                          3000,  
                          //STACK_SIZE_FOR_TASK_2,    
                          NULL,      
                          1,         
                          &Task3,    
                          0);
    */
    taskScheduler.addScheduledTask(ledUpdateDongleTask, "LED Update for Dongle", REPETITION_INTERVAL_LED_UPDATE_TASK, TASK_PRIORITY_LED_UPDATE_TASK, CORE_ID_LED_UPDATE_TASK, STACK_SIZE_LED_UPDATE_TASK);
    delay(500);
  #endif
  #ifdef Fanatec_comunication
    // Initialize FanatecInterface
    fanatec.begin();

    // Set connection callback
    fanatec.onConnected([](bool connected) {        
      if (connected) {
        ActiveSerial->println("[L] FANATEC Connected to Wheelbase.");
      } else {
        ActiveSerial->println("[L] FANATEC Disconnected from Wheelbase.");
      }
    });
    delay(2000);
    taskScheduler.addScheduledTask(fanatecUpdateTask, "FANATEC Update", REPETITION_INTERVAL_FANATEC_UPDATE_TASK,TASK_PRIORITY_FANATEC_UPDATE_TASK ,CORE_ID_FANATEC_UPDATE_TASK , STACK_SIZE_FANATEC_UPDATE_TASK);
    delay(500);
  #endif

  #ifdef OTA_Update
    taskScheduler.addScheduledTask(otaUpdateTask, "OTA Update", REPETITION_INTERVAL_OTA_UPDATE_TASK, TASK_PRIORITY_OTA_UPDATE_TASK, CORE_ID_OTA_UPDATE_TASK, STACK_SIZE_OTA_UPDATE_TASK);
  #endif
  
  //initialize wifi 
  for(uint i=0;i<30;i++)
  {
    dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8[i]=0;
    dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8[i]=0;
  }
  //task scheduler adding task
  ActiveSerial->println("[L]Initializing task scheduler...");
  
  taskScheduler.addScheduledTask(serialCommunicationRxTask, "Seria RX", REPETITION_INTERVAL_SERIAL_RX_TASK, TASK_PRIORITY_SERIAL_RX_TASK, CORE_ID_SERIAL_RX_TASK, STACK_SIZE_SERIAL_RX_TASK);
  taskScheduler.addScheduledTask(serialCommunicationTxTask, "Seria TX", REPETITION_INTERVAL_SERIAL_TX_TASK, TASK_PRIORITY_SERIAL_TX_TASK, CORE_ID_SERIAL_TX_TASK, STACK_SIZE_SERIAL_TX_TASK);
  taskScheduler.addScheduledTask(espNowCommunicationTxTask, "Espnow tx", REPETITION_INTERVAL_ESPNOW_TX_TASK, TASK_PRIORITY_ESPNOW_TX_TASK, CORE_ID_ESPNOW_TX_TASK, STACK_SIZE_ESPNOW_TX_TASK);
  taskScheduler.addScheduledTask(joystickUpdateTask, "Joystick Update", REPETITION_INTERVAL_JOYSTICK_UPDATE_TASK, TASK_PRIORITY_JOYSTICK_UPDATE_TASK, CORE_ID_JOYSTICK_UPDATE_TASK, STACK_SIZE_JOYSTICK_UPDATE_TASK);
  taskScheduler.addScheduledTask(miscTask, "MISC", REPETITION_INTERVAL_MISC_TASK, TASK_PRIORITY_MISC_TASK, CORE_ID_MISC_TASK, STACK_SIZE_MISC_TASK);
  
  delay(100);
  taskScheduler.begin();

  ActiveSerial->println("[L]Setup end");
  #ifdef USB_JOYSTICK
    delay(2000);
    tinyusbJoystick_.printf("Bridge setup end.");
  #endif
  
  
}

//espnow communication task
uint32_t loop_count=0;
bool basic_rssi_update=false;
unsigned long bridge_state_last_update=millis();
unsigned long Pairing_state_start;
unsigned long Pairing_state_last_sending;
uint8_t press_count=0;
uint Pairing_timeout=20000;
bool Pairing_timeout_status=false;
bool building_dap_esppairing_lcl =false;
void loop() 
{
  taskYIELD();
  delay(10000);
}

inline void syncPairingTableToPedals()
{
  DapAssignmentReg_t pairSync = {};
  pairSync.payloadType_u8 = DAP_PAYLOAD_TYPE_ASSIGNMENT_U8;
  pairSync.magicKey_u8 = ESPNOW_ASSIGNMENT_MAGIC_KEY_U8;
  pairSync.isAdvancedPaired_u8 = 1;
  for (int p = 0; p < 3; p++) {
    bool hasMac = false;
    for (int b = 0; b < 6; b++) {
      if (wirelessComm.getPedalMac(p)[b] != 0) {
        hasMac = true;
        break;
      }
    }
    if (hasMac) {
      pairSync.pairStatus_au8[p] = 1;
      memcpy(pairSync.pairedMac_aau8[p], wirelessComm.getPedalMac(p), 6);
    } else {
      pairSync.pairStatus_au8[p] = 0;
      memset(pairSync.pairedMac_aau8[p], 0, 6);
    }
  }
  pairSync.pairStatus_au8[3] = 1;
  memcpy(pairSync.pairedMac_aau8[3], wirelessComm.getOwnMac(), 6);

  for (int p = 0; p < 3; p++) {
    if (pairSync.pairStatus_au8[p] == 1) {
      pairSync.deviceId_u8 = p;
      pairSync.crc_u16 = checksumCalculator((uint8_t*)(&pairSync), sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
      wirelessComm.sendAssignmentSync(pairSync);
    }
  }
}

void espNowCommunicationTxTask( void * pvParameters )
{
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) 
    {
      #ifdef ESPNow_Pairing_function
        if(digitalRead(Pairing_GPIO)==LOW||g_softwarePairingAction_b)
        {
          ActiveSerial->println("[L]Bridge Pairing.....");
          delay(1000);
          Pairing_state_start=millis();
          Pairing_state_last_sending=millis();
          g_espNowPairingAction_b=true;
          building_dap_esppairing_lcl=true;
          LED_Status=1;
          g_softwarePairingAction_b=false;
        }
        if(g_espNowPairingAction_b)
        {
          unsigned long now=millis();
          //sending package
          if(building_dap_esppairing_lcl)
          {
            uint16_t crc=0;          
            building_dap_esppairing_lcl=false;
            dap_esppairing_lcl.payloadEspnowInfo_st.deviceId_u8=deviceID;
            dap_esppairing_lcl.payloadHeader_st.payloadType=DAP_PAYLOAD_TYPE_ESPNOW_PAIRING;
            dap_esppairing_lcl.payloadHeader_st.pedalTag_u8=deviceID;
            dap_esppairing_lcl.payloadHeader_st.version=DAP_VERSION_CONFIG;
            crc = checksumCalculator((uint8_t*)(&(dap_esppairing_lcl.payloadHeader_st)), sizeof(dap_esppairing_lcl.payloadHeader_st) + sizeof(dap_esppairing_lcl.payloadEspnowInfo_st));
            dap_esppairing_lcl.payloadFooter_st.checkSum_u16=crc;
          }
          if(now-Pairing_state_last_sending>400)
          {
            Pairing_state_last_sending=now;
            ESPNow.send_message(g_broadcastMac_au8,(uint8_t *) &dap_esppairing_lcl, sizeof(dap_esppairing_lcl));
          }
          

          //timeout check
          if(now-Pairing_state_start>Pairing_timeout)
          {
            ActiveSerial->println("[L]Bridge Pairing timeout!");
            g_espNowPairingAction_b=false;
            LED_Status=0;
            if(g_updatePairingToEeprom_b)
            {
              EEPROM.put(EEPROM_offset,g_espPairingReg_st);
              EEPROM.commit();
              g_updatePairingToEeprom_b=false;
              
              //list eeprom
              EspPairingReg_t ESP_pairing_reg_local;
              EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
              for(int i=0;i<4;i++)
              {
                if(ESP_pairing_reg_local.pairStatus_au8[i]==1)
                {                
                  ActiveSerial->print("[L]#");
                  ActiveSerial->print(i);
                  ActiveSerial->print("Pair: ");
                  ActiveSerial->print(ESP_pairing_reg_local.pairStatus_au8[i]);
                  ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X", ESP_pairing_reg_local.pairMac_aau8[i][0], ESP_pairing_reg_local.pairMac_aau8[i][1], ESP_pairing_reg_local.pairMac_aau8[i][2], ESP_pairing_reg_local.pairMac_aau8[i][3], ESP_pairing_reg_local.pairMac_aau8[i][4], ESP_pairing_reg_local.pairMac_aau8[i][5]);
                  ActiveSerial->println("");
                }
                ActiveSerial->println("");
              }
              ActiveSerial->println("");
              //adding peer
              /*
              for(int i=0; i<4;i++)
              {
                if(g_espPairingReg_st.pairStatus_au8[i]==1)
                {
                  if(i==0)
                  {
                    ESPNow.remove_peer(g_pedalMac_aau8[0]);
                    memcpy(&g_pedalMac_aau8[0],&g_espPairingReg_st.pairMac_aau8[i],6);
                    delay(300);
                    ESPNow.add_peer(g_pedalMac_aau8[0]);
                    
                  }
                  if(i==1)
                  {
                    ESPNow.remove_peer(g_pedalMac_aau8[1]);
                    memcpy(&g_pedalMac_aau8[1],&g_espPairingReg_st.pairMac_aau8[i],6);
                    delay(300);
                    ESPNow.add_peer(g_pedalMac_aau8[1]);
                    //ActiveSerial->printf("[L]Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", g_pedalMac_aau8[1][0], g_pedalMac_aau8[1][1], g_pedalMac_aau8[1][2], g_pedalMac_aau8[1][3], g_pedalMac_aau8[1][4], g_pedalMac_aau8[1][5]);

                  }
                  if(i==2)
                  {
                    ESPNow.remove_peer(g_pedalMac_aau8[2]);
                    memcpy(&g_pedalMac_aau8[2],&g_espPairingReg_st.pairMac_aau8[i],6);
                    delay(300);
                    ESPNow.add_peer(g_pedalMac_aau8[2]);
                  }        
                  if(i==3)
                  {
                    ESPNow.remove_peer(g_espHost_au8);
                    memcpy(&g_espHost_au8,&g_espPairingReg_st.pairMac_aau8[i],6);
                    delay(300);
                    ESPNow.add_peer(g_espHost_au8);                
                  }        
                }
              }
              */
            }
            
          }
        }
      #endif

      // Pace at most one send attempt per 2ms wake across all the
      // categories below (config/action/servo-config/OTA) - unlike the
      // pedal side's existing packetSentThisCycle discipline, nothing paced
      // this loop before, and unicast's higher per-frame cost (hardware
      // ACK+retry) makes back-to-back sends here a real risk of exhausting
      // the driver's TX descriptor pool (see the history note on
      // WirelessCommunicationBridge::sendTo()). sendTo() itself also
      // self-limits (busy/backoff), so this is a belt-and-suspenders cap,
      // not the only thing preventing overload.
      bool sentThisCycle = false;

      for(int i=0;i<3 && !sentThisCycle;i++)
      {
        if(configUpdateAvailable[i])
        {
          configUpdateAvailable[i] = false;
          if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[i]==1)
          {
            esp_err_t err;
            ActiveSerial->printf("[L]Sending Pedal #%d config, Result:", i);
            err = wirelessComm.sendConfigToPedal(i, dap_config_st[i]);
            ActiveSerial->println(esp_err_to_name(err));
            #ifdef USB_JOYSTICK
              tinyusbJoystick_.printf("Forward config to Pedal: %d, result:%s", i, esp_err_to_name(err));
            #endif
            sentThisCycle = true;
          }
        }
      }


      for(int i=0; i<3 && !sentThisCycle; i++)
      {
        if(dap_action_update[i] )
        {
          dap_action_update[i]=false;
          // TEMP DIAGNOSTIC (unconditional, low-volume - only fires for the
          // rare returnPedalConfig_u8 action) - tracking down why the
          // wireless config-echo request never completes. Remove once the
          // root cause is confirmed.
          bool isConfigRequest = dap_actions_st[i].payloadPedalAction_st.returnPedalConfig_u8 != 0;
          if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[i]==1)
          {
            // The actual action send goes out FIRST, then the (lower-
            // priority) pairing-table refresh below - not the other way
            // around. syncPairingTableToPedals() fires up to 3 back-to-back
            // unicast sends with no delay between them; under unicast's
            // shared in-flight/backoff state (see the history note on
            // WirelessCommunicationBridge::sendTo()), that burst was eating
            // the one "send slot" this 2ms wake before the actual rudder
            // action below ever got a chance to go out - the action send
            // would be silently busy-skipped with no retry, since
            // dap_action_update[i] is already cleared above. Reordering
            // costs nothing here: the pairing table already exists in RAM
            // from boot/last sync, this call just refreshes it.
            wirelessComm.sendActionToPedal(i, dap_actions_st[i]);
            sentThisCycle = true;
            if (i == PEDAL_ID_BRAKE &&
                dap_actions_st[i].payloadPedalAction_st.rudderAction_u8 != 0 &&
                dap_actions_st[i].payloadPedalAction_st.systemAction_u8 == 0) {
              syncPairingTableToPedals();
            }
            if (isConfigRequest) {
              ActiveSerial->printf("[L][DIAG] ConfigReq action forwarded to Pedal #%d\n", i);
              #ifdef USB_JOYSTICK
              tinyusbJoystick_.printf("[DIAG] ConfigReq action forwarded to Pedal #%d", i);
              #endif
            }
          }
          else if (isConfigRequest)
          {
            ActiveSerial->printf("[L][DIAG] ConfigReq action for Pedal #%d DROPPED - pedalAvailability=0\n", i);
            #ifdef USB_JOYSTICK
            tinyusbJoystick_.printf("[DIAG] ConfigReq action for Pedal #%d DROPPED - pedalAvailability=0", i);
            #endif
          }
        }

        // --- ADDED: Forward Servo Config to Pedals ---
        for(int s=0; s<3 && !sentThisCycle; s++)
        {
          if(update_servo_config[s])
          {
            update_servo_config[s] = false;
            // TEMP DIAGNOSTIC (unconditional - "Load From Servo" is rare/
            // user-triggered, not continuous). Tracking down why servo
            // register exchange doesn't complete over wireless. Remove
            // once confirmed fixed.
            if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[s]==1)
            {
              wirelessComm.sendServoConfigToPedal(s, dap_servo_config_st[s]);
              sentThisCycle = true;
              ActiveSerial->printf("[L][DIAG] ServoConfig forwarded to Pedal #%d\n", s);
              #ifdef USB_JOYSTICK
              tinyusbJoystick_.printf("[DIAG] ServoConfig forwarded to Pedal #%d", s);
              #endif
            }
            else
            {
              ActiveSerial->printf("[L][DIAG] ServoConfig for Pedal #%d DROPPED - pedalAvailability=0\n", s);
              #ifdef USB_JOYSTICK
              tinyusbJoystick_.printf("[DIAG] ServoConfig for Pedal #%d DROPPED - pedalAvailability=0", s);
              #endif
            }
          }
        }

      }



      //forward the basic wifi info for pedals
      if(g_pedalOtaAction_b && !sentThisCycle)
      {
        if (dap_action_ota_st.payloadOtaInfo_st.deviceId_u8 < 3) {
          wirelessComm.sendOtaToPedal(dap_action_ota_st.payloadOtaInfo_st.deviceId_u8, dap_action_ota_st);
          ActiveSerial->printf("[L]Forward OTA command to Pedal #%d\n", dap_action_ota_st.payloadOtaInfo_st.deviceId_u8);
        }
        g_pedalOtaAction_b=false;
      }
    }
  }
}


void clearPedalAssignmentAction(uint8_t targetIdx, const DapActions_t &action)
{
  if (targetIdx > 2) return;
  uint8_t targetMac[6] = {0};
  bool hasMac = false;
  for (int b = 0; b < 6; b++) {
    if (wirelessComm.getPedalMac(targetIdx)[b] != 0) {
      hasMac = true;
      break;
    }
  }
  if (!hasMac) {
    for (int b = 0; b < 6; b++) {
      if (g_espPairingReg_st.pairMac_aau8[targetIdx][b] != 0) {
        hasMac = true;
        break;
      }
    }
    if (hasMac) {
      memcpy(targetMac, g_espPairingReg_st.pairMac_aau8[targetIdx], 6);
    }
  } else {
    memcpy(targetMac, wirelessComm.getPedalMac(targetIdx), 6);
  }

  if (hasMac) {
    wirelessComm.sendUnicastRetry(targetMac, (const uint8_t*)&action, sizeof(DapActions_t), 5, 20);
  }

  // Clear pairing in EEPROM & RAM
  g_espPairingReg_st.pairStatus_au8[targetIdx] = 0;
  memset(g_espPairingReg_st.pairMac_aau8[targetIdx], 0, 6);
  EEPROM.put(EEPROM_offset, g_espPairingReg_st);
  EEPROM.commit();
  uint8_t zeroMac[6] = {0};
  wirelessComm.setPedalMac(targetIdx, zeroMac);
  dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[targetIdx] = 0;

  ActiveSerial->printf("[L]Cleared assignment for pedal %d and sent CLEAR_ASSIGNMENT action\n", targetIdx);
  syncPairingTableToPedals();
}

void pushPedalAssignmentAction(uint8_t sourceTag, uint8_t newRole, const DapActions_t &action)
{
  if (newRole > 2) {
    ActiveSerial->printf("[L]pushPedalAssignmentAction: invalid target role %d\n", newRole);
    return;
  }
  if (sourceTag > 7) {
    ActiveSerial->printf("[L]pushPedalAssignmentAction: invalid source tag %d\n", sourceTag);
    return;
  }

  uint8_t targetMac[6] = {0};
  bool foundMac = false;

  // Prüfe ob Quell-MAC bereits bekannt ist
  if (sourceTag < 3) {
    for (int b = 0; b < 6; b++) {
      if (wirelessComm.getPedalMac(sourceTag)[b] != 0) {
        foundMac = true;
        break;
      }
    }
    if (foundMac) {
      memcpy(targetMac, wirelessComm.getPedalMac(sourceTag), 6);
    }
  }
  if (!foundMac) {
    ActiveSerial->printf("[L]pushPedalAssignmentAction: No MAC known for tag %d\n", sourceTag);
    return;
  }

  // Unicast a few times for reliability - the target MAC is already known
  // (resolved above), so send directly to it instead of broadcasting.
  wirelessComm.sendUnicastRetry(targetMac, (const uint8_t*)&action, sizeof(DapActions_t), 3, 20);
  ActiveSerial->printf("[L]Assignment action sent to pedal: %02X:%02X:%02X:%02X:%02X:%02X, new role: %d\n",
                       targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5], newRole);

  // Bridge merkt sich die MAC-Adresse im RAM & EEPROM
  if (sourceTag != newRole) {
    wirelessComm.setPedalMac(newRole, targetMac);
    if (sourceTag >= 3) {
      uint8_t zeroMac[6] = {0};
      wirelessComm.setPedalMac(sourceTag, zeroMac);
    }

    memcpy(g_espPairingReg_st.pairMac_aau8[newRole], targetMac, 6);
    g_espPairingReg_st.pairStatus_au8[newRole] = 1;
    EEPROM.put(EEPROM_offset, g_espPairingReg_st);
    EEPROM.commit();

    syncPairingTableToPedals();
  }
}

//serial communication recieve task
bool PedalUpdateIntervalPrint_b=false;
unsigned long PedalUpdateLast=0;
unsigned long UARTJoystickUpdateLast=0;
bool isBridgeInDebugMode_b=false;
bool UARTJoystickUpdate_b=false;
int joystick_fake_value=0;

typedef struct {
    uint16_t startBytePos_u16;
    uint16_t endBytePos_u16;
    uint16_t payloadType_u16;
    bool validFlag_b;
} structChecker_st;

// Helper function to determine expected packet size from payload type
// Returns 0 if the payload type is unknown.
static inline size_t getExpectedPacketSize(uint8_t payloadType) {
    switch (payloadType) {
        case DAP_PAYLOAD_TYPE_CONFIG_U8:
            return sizeof(DapConfig_t);
        case DAP_PAYLOAD_TYPE_ACTION_U8:
            return sizeof(DapActions_t);
        case DAP_PAYLOAD_TYPE_ACTION_OTA_U8:
            return sizeof(DapActionOta_t);
        case DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8:
            return sizeof(DapBridgeState_t);
        case DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8:
            return sizeof(DAP_servo_config_st_t);
        case DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8:
            return sizeof(DapWifiChannel_t);
        case DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8:
            return sizeof(DapMacAddresses_t);
        // Add other packet types here in the future
        default:
            return 0;
    }
}
void handleWifiScanRequest(bool isHid) {
  ActiveSerial->println("[L]Scanning Wi-Fi channels...");
  int n = WiFi.scanNetworks(false, true);
  int apCount[15] = {0};
  int bestRssi[15];
  for (int i = 0; i < 15; i++) bestRssi[i] = -120;
  int score[15] = {0};

  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);
    if (ch >= 1 && ch <= 13) {
      apCount[ch]++;
      if (rssi > bestRssi[ch]) {
        bestRssi[ch] = rssi;
      }
      int weight = constrain((int)(rssi + 110) / 2, 5, 50);
      score[ch] += weight;
      if (ch > 1) score[ch - 1] += weight / 2;
      if (ch < 13) score[ch + 1] += weight / 2;
      if (ch > 2) score[ch - 2] += weight / 4;
      if (ch < 12) score[ch + 2] += weight / 4;
    }
  }
  WiFi.scanDelete();

  esp_wifi_set_channel(wirelessComm.getChannel(), WIFI_SECOND_CHAN_NONE);

  // Recommend from all 13 channels now, not just the classic non-overlapping
  // 1/6/11 trio - the per-channel scores (with adjacent-channel bleed
  // already folded in above) were always computed for all of them, they
  // just weren't reported before.
  uint8_t recommended = 1;
  int minScore = score[1];
  for (uint8_t ch = 2; ch <= 13; ch++) {
    if (score[ch] < minScore) {
      minScore = score[ch];
      recommended = ch;
    }
  }

  DapWifiChannel_t resp = {};
  resp.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  resp.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  resp.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8;
  resp.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
  resp.payloadHeader_st.pedalTag_u8 = 3;
  resp.payloadWifiChannel_st.command_u8 = WIFI_CH_CMD_SCAN_RES;
  resp.payloadWifiChannel_st.currentChannel_u8 = wirelessComm.getChannel();
  resp.payloadWifiChannel_st.recommendedChannel_u8 = recommended;
  for (uint8_t ch = 1; ch <= 13; ch++) {
    uint8_t idx = ch - 1;
    resp.payloadWifiChannel_st.channelRssi_ai8[idx] = (bestRssi[ch] > -120) ? (int8_t)bestRssi[ch] : (int8_t)0;
    resp.payloadWifiChannel_st.channelApCount_au8[idx] = (uint8_t)constrain(apCount[ch], 0, 255);
    resp.payloadWifiChannel_st.channelApScore_au8[idx] = (uint8_t)constrain(score[ch], 0, 100);
  }
  resp.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  resp.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  resp.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(resp.payloadHeader_st)), sizeof(resp.payloadHeader_st) + sizeof(resp.payloadWifiChannel_st));

#ifdef USB_JOYSTICK
  if (isHid) {
    tinyusbJoystick_.sendData((uint8_t*)&resp, sizeof(DapWifiChannel_t));
  } else {
    ActiveSerial->write((uint8_t*)&resp, sizeof(DapWifiChannel_t));
  }
#else
  ActiveSerial->write((uint8_t*)&resp, sizeof(DapWifiChannel_t));
#endif
  ActiveSerial->printf("[L]Scan done. Best channel: %d (score %d)\n", recommended, minScore);
}

void handleWifiSetChannelRequest(uint8_t newChannel, bool isHid) {
  if (newChannel < 1 || newChannel > 14) return;
  ActiveSerial->printf("[L]Switching Wi-Fi channel from %d to %d...\n", wirelessComm.getChannel(), newChannel);

  DapWifiChannel_t fwd = {};
  fwd.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  fwd.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  fwd.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8;
  fwd.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
  fwd.payloadHeader_st.pedalTag_u8 = 3;
  fwd.payloadWifiChannel_st.command_u8 = WIFI_CH_CMD_SET_REQ;
  fwd.payloadWifiChannel_st.currentChannel_u8 = newChannel;
  fwd.payloadWifiChannel_st.recommendedChannel_u8 = newChannel;
  fwd.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  fwd.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  fwd.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(fwd.payloadHeader_st)), sizeof(fwd.payloadHeader_st) + sizeof(fwd.payloadWifiChannel_st));

  // Every currently-known pedal needs to hear this (they all have to move
  // channel together), so unicast to each one individually rather than a
  // single broadcast - sent BEFORE switching the bridge's own channel below,
  // so pedals still on the old channel actually receive it.
  wirelessComm.sendToAllKnownPedalsRetry((const uint8_t*)&fwd, sizeof(DapWifiChannel_t), 4, 50);

  saveWifiChannelToEeprom(newChannel);
  wirelessComm.setChannel(newChannel);

  delay(20);
  esp_wifi_set_channel(newChannel, WIFI_SECOND_CHAN_NONE);

  fwd.payloadWifiChannel_st.command_u8 = WIFI_CH_CMD_SET_ACK;
  fwd.payloadWifiChannel_st.currentChannel_u8 = newChannel;
  fwd.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(fwd.payloadHeader_st)), sizeof(fwd.payloadHeader_st) + sizeof(fwd.payloadWifiChannel_st));
#ifdef USB_JOYSTICK
  if (isHid) {
    tinyusbJoystick_.sendData((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
  } else {
    ActiveSerial->write((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
  }
#else
  ActiveSerial->write((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
#endif
  ActiveSerial->printf("[L]Wi-Fi channel successfully switched to %d.\n", newChannel);
}

void sendWifiChannelStatus(bool isHid) {
  DapWifiChannel_t fwd = {};
  fwd.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  fwd.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  fwd.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8;
  fwd.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
  fwd.payloadHeader_st.pedalTag_u8 = 3;
  fwd.payloadWifiChannel_st.command_u8 = WIFI_CH_CMD_SET_ACK;
  fwd.payloadWifiChannel_st.currentChannel_u8 = wirelessComm.getChannel();
  fwd.payloadWifiChannel_st.recommendedChannel_u8 = wirelessComm.getChannel();
  fwd.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  fwd.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  fwd.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(fwd.payloadHeader_st)), sizeof(fwd.payloadHeader_st) + sizeof(fwd.payloadWifiChannel_st));
#ifdef USB_JOYSTICK
  if (isHid) {
    tinyusbJoystick_.sendData((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
  } else {
    ActiveSerial->write((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
  }
#else
  ActiveSerial->write((uint8_t*)&fwd, sizeof(DapWifiChannel_t));
#endif
}

void serialCommunicationRxTask( void * pvParameters)
{
  // Buffer to accumulate incoming serial data
  const size_t RX_BUFFER_SIZE = 1028; // Should be at least 2x the largest possible packet
  static uint8_t rx_buffer[RX_BUFFER_SIZE];
  static size_t buffer_len = 0;
  //configDataPackage_t configPackage_st;
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) 
    {
      //ActiveSerial->println("RX kick");
      // --- 1. Read all available data into our buffer ---
      if (ActiveSerial->available())
      {
        // Prevent buffer overflow by only reading what fits
        size_t bytesToRead = min((size_t)ActiveSerial->available(), RX_BUFFER_SIZE - buffer_len);
        if (bytesToRead > 0)
        {
          ActiveSerial->readBytes(&rx_buffer[buffer_len], bytesToRead);
          buffer_len += bytesToRead;
        }

        // ActiveSerial->println("Serial data available");
      }

      // --- 2. Process all complete packets in the buffer ---
      size_t buffer_idx = 0;
      while (buffer_idx < buffer_len)
      {
        // A. Find the next valid Start-of-Frame (SOF)
        if (rx_buffer[buffer_idx] != SOF_BYTE_0_U8 || (buffer_idx + 1 < buffer_len && rx_buffer[buffer_idx + 1] != SOF_BYTE_1_U8))
        {
          buffer_idx++;
          continue; // Keep scanning for a SOF
        }

        // ActiveSerial->println("1st check passed");

        // SOF found at buffer_idx. Check if we have enough data for a header.
        if (buffer_len < buffer_idx + 3)
        {
          // Not enough data for a full header, stop parsing for now
          break;
        }

        // B. Get expected packet size from payload type
        uint8_t payloadType = rx_buffer[buffer_idx + 2];
        size_t expectedSize = getExpectedPacketSize(payloadType);

        if (expectedSize == 0)
        {
          // Unknown payload type, this SOF is corrupt. Skip it and continue scanning.
          buffer_idx++;
          continue;
        }

        // ActiveSerial->println("2nd check passed");

        // C. Check if the full packet has arrived
        if (buffer_len < buffer_idx + expectedSize)
        {
          // Full packet is not yet in the buffer, wait for more data
          break;
        }

        // D. Check for valid End-of-Frame (EOF)
        if (rx_buffer[buffer_idx + expectedSize - 2] != EOF_BYTE_0_U8 || rx_buffer[buffer_idx + expectedSize - 1] != EOF_BYTE_1_U8)
        {
          // EOF is wrong, this packet is corrupt. Skip the SOF and continue scanning.
          buffer_idx++;
          continue;
        }

        // --- We have a candidate packet! Now validate and process it. ---
        uint8_t *packet_start = &rx_buffer[buffer_idx];
        bool structIsValid = true;
        uint16_t received_crc = 0;
        uint16_t calculated_crc = 0;

        switch (payloadType)
        {
          //case config
          case DAP_PAYLOAD_TYPE_CONFIG_U8:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DapConfig_t dap_config_st_local;
              memcpy(&dap_config_st_local, packet_start, sizeof(DapConfig_t));
              //ActiveSerial->readBytes((char *)&dap_config_st_local, sizeof(DapConfig_t));
              if (dap_config_st_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_CONFIG_U8)
              {
                structChecker = false;
                structIsValid=false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG_U8);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_config_st_local.payloadHeader_st.payloadType_u8);
              }
              if (dap_config_st_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG_U8);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_config_st_local.payloadHeader_st.version_u8);
              }
              // checksum validation
              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_config_st_local.payloadHeader_st)), sizeof(dap_config_st_local.payloadHeader_st) + sizeof(dap_config_st_local.payloadPedalConfig_st));
              if (crc != dap_config_st_local.payloadFooter_st.checkSum_u16)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_config_st_local.payloadFooter_st.checkSum_u16);
              }
              // if checks are successfull, overwrite global configuration struct
              if (structChecker == true)
              {
                int pedalIdx = dap_config_st_local.payloadHeader_st.pedalTag_u8;
                // ActiveSerial->println("[L]Updating pedal config");
                memcpy(&dap_config_st[pedalIdx], &dap_config_st_local, sizeof(DapConfig_t));
                configUpdateAvailable[pedalIdx] = true;
              }
            #endif
            break;
          }
          //case action to pedal
          case DAP_PAYLOAD_TYPE_ACTION_U8:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DapActions_t dap_actions_st_local;
              memcpy(&dap_actions_st_local, packet_start, sizeof(DapActions_t));
              //memcpy(&dap_actions_st_local, packet_start, sizeof(DapConfig_t));
              //ActiveSerial->readBytes((char *)&dap_actions_st_local, sizeof(DapActions_t));
              if (dap_actions_st_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ACTION_U8)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_ACTION_U8);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_actions_st_local.payloadHeader_st.payloadType_u8);
              }
              if (dap_actions_st_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG_U8);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_actions_st_local.payloadHeader_st.version_u8);
              }

              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_actions_st_local.payloadHeader_st)), sizeof(dap_actions_st_local.payloadHeader_st) + sizeof(dap_actions_st_local.payloadPedalAction_st));
              if (crc != dap_actions_st_local.payloadFooter_st.checkSum_u16)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_actions_st_local.payloadFooter_st.checkSum_u16);
              }
              if (structChecker == true)
              {
                
                int pedalIdx = dap_actions_st_local.payloadHeader_st.pedalTag_u8;
                uint8_t sysAction = dap_actions_st_local.payloadPedalAction_st.systemAction_u8;
                if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0) {
                  pushPedalAssignmentAction(pedalIdx, 0, dap_actions_st_local);
                } else if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1) {
                  pushPedalAssignmentAction(pedalIdx, 1, dap_actions_st_local);
                } else if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2) {
                  pushPedalAssignmentAction(pedalIdx, 2, dap_actions_st_local);
                } else if (sysAction == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT) {
                  clearPedalAssignmentAction(pedalIdx, dap_actions_st_local);
                } else if (sysAction == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP) {
                  if (pedalIdx >= 0 && pedalIdx < 3 && wirelessComm.getPedalMac(pedalIdx)[0] != 0) {
                    wirelessComm.sendActionToPedal(pedalIdx, dap_actions_st_local);
                  }
                } else if(pedalIdx == PEDAL_ID_CLUTCH || pedalIdx == PEDAL_ID_BRAKE || pedalIdx == PEDAL_ID_THROTTLE)
                {
                  //forward to pedal
                  memcpy(&dap_actions_st[pedalIdx], &dap_actions_st_local, sizeof(DapActions_t));
                  dap_action_update[pedalIdx] = true;
                }
              }
            #endif
            break;
          }

          //case action for ota
          case DAP_PAYLOAD_TYPE_ACTION_OTA_U8:
          {
            #ifndef USB_JOYSTICK
              ActiveSerial->println("[L]get OTA command and its info");
              memcpy(&dap_action_ota_st, packet_start, sizeof(DapActionOta_t));
              //ActiveSerial->readBytes((char *)&dap_action_ota_st, sizeof(DapActionOta_t));
              #ifdef OTA_Update
                bool structChecker_b = true;
                if (dap_action_ota_st.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ACTION_OTA_U8)
                {
                  structChecker_b = false;
                  structIsValid = false;
                }
                if (structChecker_b)
                {
                  SSID = new char[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8 + 1];
                  PASS = new char[dap_action_ota_st.payloadOtaInfo_st.passLength_u8 + 1];
                  memcpy(SSID, dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8, dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8);
                  memcpy(PASS, dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8, dap_action_ota_st.payloadOtaInfo_st.passLength_u8);
                  SSID[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8] = 0;
                  PASS[dap_action_ota_st.payloadOtaInfo_st.passLength_u8] = 0;
                  /*
                  ActiveSerial->printf("[L]Device ID:%d\n",dap_action_ota_st.payloadOtaInfo_st.deviceId_u8);
                  ActiveSerial->print("[L]SSID(uint)=");
                  for(uint i=0; i<dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8;i++)
                  {
                    ActiveSerial->print(dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8[i]);
                    ActiveSerial->print(",");
                  }
                  ActiveSerial->println(" ");
                  ActiveSerial->print("[L]PASS(uint)=");
                  for(uint i=0; i<dap_action_ota_st.payloadOtaInfo_st.passLength_u8;i++)
                  {
                    ActiveSerial->print(dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8[i]);
                    ActiveSerial->print(",");
                  }
                  ActiveSerial->println(" ");

                  ActiveSerial->print("[L]SSID=");
                  ActiveSerial->println(SSID);
                  ActiveSerial->print("[L]PASS=");
                  ActiveSerial->println(PASS);
                  */
                }

                if (dap_action_ota_st.payloadOtaInfo_st.deviceId_u8 == DEVICE_ID && structChecker_b == true)
                {
                  OTA_enable_b = true;
                  ActiveSerial->println("[L] Bridge OTA begin.");
                }
                else if (structChecker_b)
                {
                  g_pedalOtaAction_b = true;
                }
              #endif
            #endif
            break;
          }
          case DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DapBridgeState_t dap_bridge_state_local;
              //dap_bridge_state_local_ptr = &dap_bridge_state_lcl;
              memcpy(&dap_bridge_state_local, packet_start, sizeof(DapBridgeState_t));
              //ActiveSerial->readBytes((char *)dap_bridge_state_local_ptr, sizeof(DapBridgeState_t));
              // check if data is plausible
              if (dap_bridge_state_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_bridge_state_local.payloadHeader_st.payloadType_u8);
              }
              if (dap_bridge_state_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG_U8);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_bridge_state_lcl.payloadHeader_st.version_u8);
              }
              // checksum validation
              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_bridge_state_local.payloadHeader_st)), sizeof(dap_bridge_state_local.payloadHeader_st) + sizeof(dap_bridge_state_local.payloadBridgeState_st));
              if (crc != dap_bridge_state_local.payloadFooter_st.checkSum_u16)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_bridge_state_local.payloadFooter_st.checkSum_u16);
              }
              // if checks are successfull, overwrite global configuration struct
              if (structChecker == true)
              {
                memcpy(&dap_bridge_state_lcl, &dap_bridge_state_local, sizeof(DapBridgeState_t));
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_ENABLE_PAIRING)
                {
                  #ifdef ESPNow_Pairing_function
                    ActiveSerial->println("[L]Bridge Pairing...");
                    g_softwarePairingAction_b = true;
                  #endif
                  #ifndef ESPNow_Pairing_function
                    ActiveSerial->println("[L]Pairing command didn't supported");
                  #endif
                }
                // action=2, restart
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_RESTART)
                {
                  ActiveSerial->println("[L]Bridge Restart");
                  delay(1000);
                  ESP.restart();
                }
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_DOWNLOAD_MODE)
                {
                  // aciton=3 restart into boot mode
                  #ifdef CONFIG_IDF_TARGET_ESP32S3
                    ActiveSerial->println("[L]Bridge Restart into Download mode");
                    delay(1000);
                    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
                    ESP.restart();
                  #else
                    ActiveSerial->println("[L]Command not supported ");
                    delay(1000);
                  #endif
                }
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_DEBUG)
                {
                  if (isBridgeInDebugMode_b)
                  {
                    // aciton=4 print pedal update interval
                    ActiveSerial->println("[L]Bridge debug mode off.");
                    isBridgeInDebugMode_b = false;
                  }
                  else
                  {
                    // aciton=4 print pedal update interval
                    ActiveSerial->println("[L]Bridge debug mode on.");
                    firstDebugMessage_b = true;
                    isBridgeInDebugMode_b = true;
                  }
                }
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_JOYSTICK_FLASHING_MODE)
                {
                  #ifdef External_RP2040
                    ActiveSerial->println("[L]JOYSTICK restart into flashing mode");
                    dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_RESET_INTO_BOOTLOADER;
                  #else
                    ActiveSerial->println("[L]The command is not supported");
                  #endif
                }
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_JOYSTICK_DEBUG)
                {
                  #ifdef External_RP2040
                    ActiveSerial->println("[L]JOYSTICK debug mode on");
                    dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_DEBUG_MODE;
                  #else
                    ActiveSerial->println("[L]The command is not supported");
                  #endif
                }
                if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_SET_PEDAL_WIRELESS_SYNC)
                {
                  wirelessComm.setPedalWirelessSyncEnabled(0, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[0] != 0);
                  wirelessComm.setPedalWirelessSyncEnabled(1, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[1] != 0);
                  wirelessComm.setPedalWirelessSyncEnabled(2, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[2] != 0);
                  for (int pIdx = 0; pIdx < 3; pIdx++)
                  {
                    if (!wirelessComm.isPedalWirelessSyncEnabled(pIdx))
                    {
                      g_joystickValueOriginal_au16[pIdx] = JOYSTICK_MIN_VALUE;
                      g_joystickValue_au16[pIdx] = JOYSTICK_MIN_VALUE;
                      dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pIdx] = 0;
                      dap_bridge_state_st.payloadBridgeState_st.pedalRssiRealtime_ai32[pIdx] = 0;
                      wirelessComm.setRssi(pIdx, 0);
                      if (pIdx == 0) g_pedalClutchValue_u16 = JOYSTICK_MIN_VALUE;
                      if (pIdx == 1) g_pedalBrakeValue_u16 = JOYSTICK_MIN_VALUE;
                      if (pIdx == 2) g_pedalThrottleValue_u16 = JOYSTICK_MIN_VALUE;
                    }
                  }
                }
              }
            #endif
            break;
          }
          case DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8:
          {
            bool structChecker = true;
            DapWifiChannel_t wifiChannel_local;
            memcpy(&wifiChannel_local, packet_start, sizeof(DapWifiChannel_t));
            if (wifiChannel_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8)
            {
              structChecker = false;
              structIsValid = false;
            }
            if (wifiChannel_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
            {
              structChecker = false;
              structIsValid = false;
            }
            uint16_t crc = checksumCalculator((uint8_t *)(&(wifiChannel_local.payloadHeader_st)), sizeof(wifiChannel_local.payloadHeader_st) + sizeof(wifiChannel_local.payloadWifiChannel_st));
            if (crc != wifiChannel_local.payloadFooter_st.checkSum_u16)
            {
              structChecker = false;
              structIsValid = false;
            }
            if (structChecker == true)
            {
              uint8_t cmd = wifiChannel_local.payloadWifiChannel_st.command_u8;
              if (cmd == WIFI_CH_CMD_SCAN_REQ)
              {
                handleWifiScanRequest(false);
              }
              else if (cmd == WIFI_CH_CMD_SET_REQ)
              {
                uint8_t targetCh = wifiChannel_local.payloadWifiChannel_st.currentChannel_u8;
                if (targetCh < 1 || targetCh > 14) {
                  targetCh = wifiChannel_local.payloadWifiChannel_st.recommendedChannel_u8;
                }
                handleWifiSetChannelRequest(targetCh, false);
              }
              else
              {
                sendWifiChannelStatus(false);
              }
            }
            break;
          }
          case DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8:
          {
            bool structChecker = true;
            DapMacAddresses_t macCfg_local;
            memcpy(&macCfg_local, packet_start, sizeof(DapMacAddresses_t));
            if (macCfg_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8)
            {
              structChecker = false;
              structIsValid = false;
            }
            if (macCfg_local.payloadHeader_st.version_u8 != DAP_VERSION_MAC_ADDRESSES_U8)
            {
              structChecker = false;
              structIsValid = false;
            }
            uint16_t crc = checksumCalculator((uint8_t *)(&(macCfg_local.payloadHeader_st)), sizeof(macCfg_local.payloadHeader_st) + sizeof(macCfg_local.payloadMacAddresses_st));
            if (crc != macCfg_local.payloadFooter_st.checkSum_u16)
            {
              structChecker = false;
              structIsValid = false;
            }
            if (structChecker == true)
            {
              if (macCfg_local.payloadHeader_st.storeToEeprom_u8 == 1)
              {
                storeMacAddressesToEeprom(macCfg_local);
                ActiveSerial->println("[L]Stored MAC addresses & channel to EEPROM (Serial)");
                // Only a commit (storeToEeprom=1) carries a real MAC table -
                // a query (storeToEeprom=0, e.g. the plugin's UI auto-detect
                // on tab load) sends an all-zero payload just to ask for the
                // current table back, and must not overwrite the live one.
                wirelessComm.applyMacConfig(macCfg_local);
              }

              DapMacAddresses_t reply = loadMacAddressesFromEeprom();
              reply.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
              reply.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
              reply.payloadHeader_st.storeToEeprom_u8 = 0;
              reply.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)&reply.payloadHeader_st, sizeof(reply.payloadHeader_st) + sizeof(reply.payloadMacAddresses_st));
              ActiveSerial->write((char*)&reply, sizeof(DapMacAddresses_t));
            }
            break;
          }
          //case action for servo config
          case DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DAP_servo_config_st_t dap_servo_config_local;
              memcpy(&dap_servo_config_local, packet_start, sizeof(DAP_servo_config_st_t));
              
              if (dap_servo_config_local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8)
              {
                structChecker = false;
                structIsValid = false;
              }
              if (dap_servo_config_local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
              {
                structChecker = false;
                structIsValid = false;
              }
              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_servo_config_local.payloadHeader_st)), sizeof(dap_servo_config_local.payloadHeader_st) + sizeof(dap_servo_config_local.payloadServoConfig_st));
              if (crc != dap_servo_config_local.payloadFooter_st.checkSum_u16)
              {
                structChecker = false;
                structIsValid = false;
              }
              if (structChecker == true)
              {
                int pedalIdx = dap_servo_config_local.payloadHeader_st.pedalTag_u8;
                if(pedalIdx >= 0 && pedalIdx < 3)
                {
                  memcpy(&dap_servo_config_st[pedalIdx], &dap_servo_config_local, sizeof(DAP_servo_config_st_t));
                  update_servo_config[pedalIdx] = true;
                }
              }
            #endif
            break;
          }
          default:
          {
            ActiveSerial->println("[L]Unknown payload type");
            break;
          }
        }//switch end
        if (!structIsValid)
        {
          ActiveSerial->printf("[L]Invalid packet detected (Type: %d). Skipping SOF.\n", payloadType);
          ActiveSerial->println("");
          buffer_idx++; // Skip the failed SOF and continue scanning
        }
        else
        {
          // Packet was valid and processed, advance index past this packet
          buffer_idx += expectedSize;
        }
      }//while end
      // --- 3. Clean up the buffer ---
      if (buffer_idx > 0)
      {
        size_t remaining_len = buffer_len - buffer_idx;
        if (remaining_len > 0)
        {
          memmove(rx_buffer, &rx_buffer[buffer_idx], remaining_len);
        }
        buffer_len = remaining_len;
      }
    }
  }
}


void serialCommunicationTxTask( void * pvParameters)
{
  for(;;)
  { 
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) 
    {
      //ActiveSerial->println("tx kick");
      uint16_t crc;
      unsigned long current_time=millis();
      #ifndef USB_JOYSTICK
        if(current_time-bridge_state_last_update>200)
        {
          basic_rssi_update=true;
          bridge_state_last_update=millis();
        }
      
        if(current_time-PedalUpdateLast>500)
        {
          PedalUpdateIntervalPrint_b=true;
          PedalUpdateLast=current_time;
        }
      #endif
      if(current_time-UARTJoystickUpdateLast>7)
      {
        UARTJoystickUpdate_b=true;
        UARTJoystickUpdateLast=current_time;
      }
      bool structChecker = true;
      #ifndef USB_JOYSTICK
        for(int i =0; i<3; i++)
        {
          if(g_updateBasicState_ab[i])
          {
            g_updateBasicState_ab[i]=false;
            ActiveSerial->write((char*)&dap_state_basic_st[i], sizeof(DapStateBasic_t));
            ActiveSerial->print("\r\n");
            if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]==0)
            {
              ActiveSerial->print("[L]Found Pedal:");
              ActiveSerial->println(dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
            }
            dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]=1;
            //g_pedalLastUpdate_au32[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]=millis();
            if(g_espNowError_ab[i])
            {
              ActiveSerial->print("[L]Pedal:");
              ActiveSerial->print(dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
              ActiveSerial->print(" E:");
              ActiveSerial->println(dap_state_basic_st[i].payloadPedalStateBasic_st.errorCode_u8);
              g_espNowError_ab[i]=false;    
            }
          }
          if(g_updateExtendState_ab[i])
          {
            g_updateExtendState_ab[i]=false;
            ActiveSerial->write((char*)&dap_state_extended_st[i], sizeof(DapStateExtended_t));
            ActiveSerial->print("\r\n");

          }
          
          // Forward Servo Config Responses to Host ---
          if(send_servo_config_to_host[i])
          {
            send_servo_config_to_host[i] = false;
            ActiveSerial->write((char*)&dap_servo_config_response_st[i], sizeof(DAP_servo_config_st_t));
            ActiveSerial->print("\r\n");
          }

        }
      
        int pedal_config_IDX=0;
        for(pedal_config_IDX=0;pedal_config_IDX<3;pedal_config_IDX++)
        {
          if(g_espNowRequestConfig_ab[pedal_config_IDX])
          {
            DapConfig_t * dap_config_st_local_ptr;
            DapConfig_t dap_config_st_local;
            if(pedal_config_IDX==0)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Clu, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Clu;
            }
            if(pedal_config_IDX==1)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Brk, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Brk;
            }
            if(pedal_config_IDX==2)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Gas, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Gas;
            }
            dap_config_st_local_ptr= &dap_config_st_local;
            
            //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payloadHeader_st)), sizeof(dap_config_st.payloadHeader_st) + sizeof(dap_config_st.payloadPedalConfig_st));

            dap_config_st_local_ptr->payloadHeader_st.pedalTag_u8=dap_config_st_local_ptr->payloadPedalConfig_st.pedalType_u8;
            crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payloadHeader_st)), sizeof(dap_config_st_local.payloadHeader_st) + sizeof(dap_config_st_local.payloadPedalConfig_st));
            dap_config_st_local_ptr->payloadFooter_st.checkSum_u16 = crc;
            ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DapConfig_t));
            ActiveSerial->print("\r\n");
            g_espNowRequestConfig_ab[pedal_config_IDX]=false;
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedal_config_IDX);
            ActiveSerial->println(" config returned");
            delay(3);
          }
        }
      

        if(basic_rssi_update)//Bridge action
        {
          //fill header and footer
          dap_bridge_state_st.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
          dap_bridge_state_st.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
          dap_bridge_state_st.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
          dap_bridge_state_st.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
          int rssi_filter_value=constrain(rssi_filter.process(g_rssiDisplay_i32),-100,0) ;
          dap_bridge_state_st.payloadHeader_st.pedalTag_u8=5; //5 means bridge
          dap_bridge_state_st.payloadHeader_st.payloadType_u8=DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8;
          dap_bridge_state_st.payloadHeader_st.version_u8=DAP_VERSION_CONFIG_U8;
          dap_bridge_state_st.payloadBridgeState_st.bridgeAction_u8=0;
          memcpy(dap_bridge_state_st.payloadBridgeState_st.pedalRssiRealtime_ai32,wirelessComm.getRssiArray(),sizeof(int32_t)*3);
          //parse_version(BRIDGE_FIRMWARE_VERSION,&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[0],&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[1],&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[2]);
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[0]=versionMajor;
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[1]=versionMinor;
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[2]=versionPatch;
          uint8_t unassignedCount = 0;
          memset(dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8, 0, sizeof(dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8));
          for (int p = 0; p < 3; p++) {
            memcpy(&dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8[p * 6], wirelessComm.getPedalMac(p), 6);
          }
          dap_bridge_state_st.payloadBridgeState_st.unassignedPedalCount_u8 = unassignedCount;
          //CRC check should be in the final
          crc = checksumCalculator((uint8_t*)(&(dap_bridge_state_st.payloadHeader_st)), sizeof(dap_bridge_state_st.payloadHeader_st) + sizeof(dap_bridge_state_st.payloadBridgeState_st));
          dap_bridge_state_st.payloadFooter_st.checkSum_u16=crc;
          DapBridgeState_t * dap_bridge_st_local_ptr;
          dap_bridge_st_local_ptr = &dap_bridge_state_st;
          ActiveSerial->write((char*)dap_bridge_st_local_ptr, sizeof(DapBridgeState_t));
          ActiveSerial->print("\r\n");
          basic_rssi_update=false;
          /*
          if(rssi_filter_value<-88)
          {
            ActiveSerial->println("Warning: BAD WIRELESS CONNECTION");
            //ActiveSerial->print("Pedal:");
            //ActiveSerial->print(dap_state_basic_st.payloadHeader_st.pedalTag_u8);
            ActiveSerial->print(" RSSI:");
            ActiveSerial->println(rssi_filter_value);  
          }
          */
          #ifdef ESPNow_debug
              ActiveSerial->print("Pedal:");
              ActiveSerial->print(dap_state_basic_st.payloadHeader_st.pedalTag_u8);
              ActiveSerial->print(" RSSI:");
              ActiveSerial->println(rssi_filter_value);
          #endif
        }
      #endif
        #ifdef External_RP2040
        if(UARTJoystickUpdate_b)
        {
          DAP_JoystickUART_State * dap_joystickUART_state_local_ptr;
          UARTJoystickUpdate_b=false;
          dap_joystickUART_state_lcl._payloadjoystick.payloadtype=(uint8_t)DAP_PAYLOAD_TYPE_JOYSTICKUART;
          dap_joystickUART_state_lcl._payloadjoystick.key = DAP_JOY_KEY;
          dap_joystickUART_state_lcl._payloadjoystick.DAP_JOY_Version = DAP_JOY_VERSION;
          for(int i=0; i<3;i++)
          {
            dap_joystickUART_state_lcl._payloadjoystick.controllerValue_i32[i] = wirelessComm.isPedalWirelessSyncEnabled(i) ? g_joystickValueOriginal_au16[i] : JOYSTICK_MIN_VALUE;
            dap_joystickUART_state_lcl._payloadjoystick.pedalAvailability[i] = wirelessComm.isPedalWirelessSyncEnabled(i) ? dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[i] : 0;
          }
          dap_joystickUART_state_lcl._payloadjoystick.pedal_status=g_pedalStatus_u8;
          dap_joystickUART_state_lcl._payloadfooter.checkSum_u16= checksumCalculator((uint8_t*)(&(dap_joystickUART_state_lcl._payloadjoystick)), sizeof(dap_joystickUART_state_lcl._payloadjoystick));
          _rp2040picoUART->UARTSendPacket((uint8_t*)&dap_joystickUART_state_lcl, sizeof(DAP_JoystickUART_State));
          if(dap_joystickUART_state_lcl._payloadjoystick.JoystickAction!=0)
          {
            dap_joystickUART_state_lcl._payloadjoystick.JoystickAction=0;
          }

        }
        #endif

      uint8_t pedalIDX;
      for(pedalIDX=0;pedalIDX<3;pedalIDX++)
      {
        unsigned long current_time=millis();
        if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pedalIDX]==1)
        {
          if(current_time-g_pedalLastUpdate_au32[pedalIDX]>3000)
          {
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedalIDX);
            ActiveSerial->println(" Disconnected");
            dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pedalIDX]=0;

          }
        }  
      }
      //print log from espnow
      #ifndef USB_JOYSTICK
        PayloadHidMessage_t receivedMsg;
        if (xQueueReceive(g_messageQueueHandle_pv, &receivedMsg, (TickType_t)0) == pdTRUE)
        {
          ActiveSerial->print("[L]");
          ActiveSerial->println(receivedMsg.text_ac);
          ActiveSerial->println("");
        }
      #endif
      /*
      if(getESPNOWLog_b)
      {
        getESPNOWLog_b=false;
        ActiveSerial->print("[L]");
        ActiveSerial->println(espnowLog);
      }
      */

      //debug message print
      #ifndef USB_JOYSTICK
        if(PedalUpdateIntervalPrint_b)
        {
          if(isBridgeInDebugMode_b)
          {
            for(pedalIDX=0;pedalIDX<3;pedalIDX++)
            {
              if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pedalIDX]==1)
              {
                ActiveSerial->print("[L]Pedal ");
                ActiveSerial->print(pedalIDX);
                ActiveSerial->print(" Update interval: ");
                ActiveSerial->print(current_time-g_pedalLastUpdate_au32[pedalIDX]);
                ActiveSerial->print(" RSSI: ");
                ActiveSerial->println(wirelessComm.getRssi(pedalIDX));
              }
              
            }
            ActiveSerial->print("[L]sending:");
            printStructHex(&dap_bridge_state_st);
          }
          PedalUpdateIntervalPrint_b=false;
        }
      #endif
    }
    
    
    //delay(1);
  }
}

void joystickUpdateTask( void * pvParameters )
{
  unsigned long last_serial_joy_out = millis();
  unsigned long now;
  int16_t pedalJoystick_last[3] = {0, 0, 0};
  bool pedalJoystickUpdate_b = false;
  unsigned joystick_test_time = millis();
  bool print_value_b=false;
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {

      for (int i = 0; i < 3; i++)
      {
        if (pedalJoystick_last[i] != g_joystickValueOriginal_au16[i])
        {
          pedalJoystick_last[i] = g_joystickValueOriginal_au16[i];
          pedalJoystickUpdate_b = true;
        }
      }
      #ifdef USB_JOYSTICK
        if (IsControllerReady() /*&& pedalJoystickUpdate_b*/)
        {
          if(isBridgeInDebugMode_b)
          {
            ActiveSerial->print("[L]Throttle value:");
            ActiveSerial->println(g_pedalThrottleValue_u16);
            ActiveSerial->print("[L]Brake value:");
            ActiveSerial->println(g_pedalBrakeValue_u16);
            ActiveSerial->print("[L]Cluth value:");
            ActiveSerial->println(g_pedalClutchValue_u16);
          }
          if (g_pedalStatus_u8 == 0)
          {
            uint16_t clutchVal = wirelessComm.isPedalWirelessSyncEnabled(0) ? g_pedalClutchValue_u16 : JOYSTICK_MIN_VALUE;
            uint16_t brakeVal  = wirelessComm.isPedalWirelessSyncEnabled(1) ? g_pedalBrakeValue_u16  : JOYSTICK_MIN_VALUE;
            uint16_t throttleVal = wirelessComm.isPedalWirelessSyncEnabled(2) ? g_pedalThrottleValue_u16 : JOYSTICK_MIN_VALUE;

            SetControllerOutputValueAccelerator(clutchVal);
            SetControllerOutputValueBrake(brakeVal);
            SetControllerOutputValueThrottle(throttleVal);
            SetControllerOutputValueRudder(JOYSTICK_CENTER);
            SetControllerOutputValueRudder_brake(JOYSTICK_CENTER, JOYSTICK_CENTER);
          }
          if (g_pedalStatus_u8 == 1)
          {
            SetControllerOutputValueAccelerator(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueBrake(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueThrottle(JOYSTICK_MIN_VALUE);
            // No deadzone here by design: rudderValue already comes from the
            // pedal's own joystick-mapping curve (Main.cpp on the pedal,
            // mirrored symmetrically for left/right), which has its own
            // deadzone control - dragging the curve's center control point
            // outward clamps flat near center and ramps from there, with no
            // discontinuity. A second, independent, hardcoded deadzone here
            // was both redundant and (as shipped) buggy - see git history.
            uint16_t rudderValue = wirelessComm.isPedalWirelessSyncEnabled(2) ? g_pedalThrottleValue_u16 : JOYSTICK_CENTER;
            SetControllerOutputValueRudder(rudderValue);
            SetControllerOutputValueRudder_brake(JOYSTICK_MIN_VALUE, JOYSTICK_MIN_VALUE);
          }
          if (g_pedalStatus_u8 == 2)
          {
            SetControllerOutputValueAccelerator(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueBrake(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueThrottle(JOYSTICK_MIN_VALUE);

            uint16_t leftBrakeVal = (dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[0] == 1 && wirelessComm.isPedalWirelessSyncEnabled(0))
                                      ? g_pedalClutchValue_u16
                                      : (wirelessComm.isPedalWirelessSyncEnabled(1) ? g_pedalBrakeValue_u16 : JOYSTICK_MIN_VALUE);
            uint16_t rightBrakeVal = wirelessComm.isPedalWirelessSyncEnabled(2) ? g_pedalThrottleValue_u16 : JOYSTICK_MIN_VALUE;

            // In Toe Brake mode, the pedals act purely as independent wheel brakes:
            // Rudder yaw (X-axis) remains neutral (centered) with zero connection between pedals
            SetControllerOutputValueRudder(JOYSTICK_CENTER);

            // Output independent left and right wheel brakes on Y and Z axes
            SetControllerOutputValueRudder_brake(leftBrakeVal, rightBrakeVal);
          }
          joystickSendState();
          if (pedalJoystickUpdate_b)
          {
            joystickSendState();
            pedalJoystickUpdate_b = false;
          }
          

          // bool joystatus=GetJoystickStatus();
        }
        /*
        if (!GetJoystickStatus())
        {
          RestartJoystick();
          ActiveSerial->println("[L]HID Error, Restart Joystick...");
          // last_serial_joy_out=millis();
        }
          */
      #endif
      // set analog value
      #ifdef Using_analog_output

        uint16_t dacBrakeVal = wirelessComm.isPedalWirelessSyncEnabled(1) ? g_joystickValue_au16[1] : 0;
        uint16_t dacGasVal   = wirelessComm.isPedalWirelessSyncEnabled(2) ? g_joystickValue_au16[2] : 0;
        dacWrite(Analog_brk, (uint16_t)((float)(dacBrakeVal / (float)(JOYSTICK_RANGE)) * 255));
        dacWrite(Analog_gas, (uint16_t)((float)(dacGasVal / (float)(JOYSTICK_RANGE)) * 255));
      #endif
      // set MCP4728 analog value
      #ifdef Using_MCP4728
        // ActiveSerial->print("MCP/");
        now = millis();
        if (MCP_status)
        {
          /*
          if(now-last_serial_joy_out>1000)
          {
            ActiveSerial->print("MCP/");
            ActiveSerial->print(g_joystickValue_au16[0]);
            ActiveSerial->print("/");
            ActiveSerial->print(g_joystickValue_au16[1]);
            ActiveSerial->print("/");
            ActiveSerial->print(g_joystickValue_au16[2]);
          }
          */

          uint16_t mcpClu = wirelessComm.isPedalWirelessSyncEnabled(0) ? g_joystickValue_au16[0] : 0;
          uint16_t mcpBrk = wirelessComm.isPedalWirelessSyncEnabled(1) ? g_joystickValue_au16[1] : 0;
          uint16_t mcpGas = wirelessComm.isPedalWirelessSyncEnabled(2) ? g_joystickValue_au16[2] : 0;
          mcp.setChannelValue(MCP4728_CHANNEL_A, (uint16_t)((float)mcpClu / (float)JOYSTICK_RANGE * 0.8f * 4096));
          mcp.setChannelValue(MCP4728_CHANNEL_B, (uint16_t)((float)mcpBrk / (float)JOYSTICK_RANGE * 0.8f * 4096));
          mcp.setChannelValue(MCP4728_CHANNEL_C, (uint16_t)((float)mcpGas / (float)JOYSTICK_RANGE * 0.8f * 4096));
        }

      #endif
    }
  }
}

//OTA multitask
uint16_t OTA_count=0;
bool message_out_b=false;
bool OTA_enable_start=false;
uint32_t otaTask_stackSizeIdx_u32 = 0;
void otaUpdateTask( void * pvParameters )
{

  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      #ifdef OTA_Update
        if(OTA_count>200)
        {
          message_out_b=true;
          OTA_count=0;
        }
        else
        {
          OTA_count++;
        }

        
        if(OTA_enable_b)
        {
          if(message_out_b)
          {
            message_out_b=false;
            Serial1.println("[L]OTA enable flag on");
          }
          if(OTA_status)
          {
            
            //server.handleClient();
          }
          else
          {
            ActiveSerial->println("[L]de-initialize espnow");
            ActiveSerial->println("[L]wait...");
            esp_err_t result= wirelessComm.deinit();
            delay(200);
            if(result==ESP_OK)
            {
              OTA_status=true;
              delay(1000);
              //ota_wifi_initialize(APhost);
              wifi_initialized(SSID,PASS);
              delay(2000);
              ESP32OTAPull ota;
              char* Version_tag;
              int ret;
              ota.SetCallback(OTAcallback);
              ota.OverrideBoard(BRIDGE_BOARD);
              Version_tag=BRIDGE_FIRMWARE_VERSION;
              if(dap_action_ota_st.payloadOtaInfo_st.otaAction_u8==1)
              {
                Version_tag="0.0.0";
                ActiveSerial->println("Force update");
              }
              switch (dap_action_ota_st.payloadOtaInfo_st.modeSelect_u8)
              {
                case 1:
                  ActiveSerial->printf("[L]Flashing to latest Main, checking %s to see if an update is available...\n", JSON_URL_main);
                  ret = ota.CheckForOTAUpdate(JSON_URL_main, Version_tag);
                  ActiveSerial->printf("[L]CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  break;
                case 2:
                  ActiveSerial->printf("[L]Flashing to latest Dev, checking %s to see if an update is available...\n", JSON_URL_dev);
                  ret = ota.CheckForOTAUpdate(JSON_URL_dev, Version_tag);
                  ActiveSerial->printf("[L]CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  break;
                case 3:
                  ActiveSerial->printf("[L]Flashing to Daily build, checking %s to see if an update is available...\n", JSON_URL_dev);
                  ret = ota.CheckForOTAUpdate(JSON_URL_daily, Version_tag);
                  ActiveSerial->printf("[L]CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  break;
                default:
                break;
              }

              delay(3000);
            }

          }
        }
        
        //delay(2);
      #endif

      #ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
        if( otaTask_stackSizeIdx_u32 == 1000)
        {
          UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
          ActiveSerial->print("StackSize (OTA): ");
          ActiveSerial->println(stackHighWaterMark);
          otaTask_stackSizeIdx_u32 = 0;
        }
        otaTask_stackSizeIdx_u32++;
      #endif
      //delay(2);
    }
  }
}

//LED task

void ledUpdateTask( void * pvParameters)
{
  uint8_t LED_bright_index = 0;
  uint8_t LED_bright_direction = 1;
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      #ifdef LED_ENABLE_WAVESHARE
      //LED status update
        if(LED_Status==0)
        {
          if(LED_bright_index>30)
          {
            LED_bright_direction=-1;
          }
          if(LED_bright_index<2)
          {
            LED_bright_direction=1;
          }
          LED_bright_index=LED_bright_index+LED_bright_direction;
          pixels.setBrightness(LED_bright_index);
          uint8_t led_status=dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[0]+dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[1]*2+dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[2]*4;
          switch (led_status)
          {
            case 0:
              pixels.setPixelColor(0,0xff,0xff,0xff);
              //pixels.setPixelColor(0,0x52,0x00,0xff);//Orange
              pixels.show();
              break;
            case 1:
              pixels.setPixelColor(0,0xff,0x00,0x00);//Red
              pixels.show();
              break;
            case 2:
              pixels.setPixelColor(0,0xff,0x0f,0x00);//Orange
              pixels.show();
              break;
            case 3:
              pixels.setPixelColor(0,0x52,0x00,0xff);//Cyan
              pixels.show();
              break; 
            case 4:
              pixels.setPixelColor(0,0x5f,0x5f,0x00);//Yellow
              pixels.show();
              break;
            case 5:
              pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
              pixels.show();
              break;      
            case 6:
              pixels.setPixelColor(0,0x00,0xff,0x00);//Green
              pixels.show();
              break;  
            case 7:
              pixels.setPixelColor(0, 0x80, 0x00, 0x80);//Purple
              pixels.show();
              break;                                         
            default:
              break;
          }
          delay(150);           
        }
        if(LED_Status==1)//pairing
        {
          
          //delay(1000);
          pixels.setPixelColor(0,0xff,0x00,0x00);//Red       
          pixels.setBrightness(25);
          pixels.show();
          delay(500);
          pixels.setPixelColor(0,0x00,0x00,0x00);//fill no color      
          //pixels.setBrightness(0);
          pixels.show();
          delay(500);
        }

      #endif  
    //delay(10);
    }
  }
}

void ledUpdateDongleTask( void * pvParameters)
{
  uint8_t LED_bright_index = 0;
  uint8_t LED_bright_direction = 1;
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      #ifdef LED_ENABLE_DONGLE
      //LED status update
        if(LED_Status==0)
        {
          if(LED_bright_index>30)
          {
            LED_bright_direction=-1;
          }
          if(LED_bright_index<2)
          {
            LED_bright_direction=1;
          }
          LED_bright_index=LED_bright_index+LED_bright_direction;
          pixels.setBrightness(LED_bright_index);

          for(uint i=0;i<3;i++)
          {
            if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[i]==1)
            {
              pixels.setPixelColor(i,0xff,0x0f,0x00);//Orange
            }
            else
            {
              pixels.setPixelColor(i,0xff,0xff,0xff);//White
            }            
          }
          pixels.show();
          
          delay(150);           
        }
        if(LED_Status==1)//pairing
        {
          
          //delay(1000);
          for(uint i=0;i<3;i++)
          {
            pixels.setPixelColor(i,0xff,0x00,0x00);//Red  
          }
              
          pixels.setBrightness(25);
          pixels.show();
          delay(500);
          for(uint i=0;i<3;i++)
          {
            pixels.setPixelColor(i,0x00,0x00,0x00);//fill no color  
          }
            
          //pixels.setBrightness(0);
          pixels.show();
          delay(500);
        }

      #endif  
      //delay(10);
      }
  }
}


void fanatecUpdateTask(void * pvParameters) 
{
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      #ifdef Fanatec_comunication
        fanatec.communicationUpdate();
        if (fanatec.isPlugged()) {
          uint16_t throttleValue = g_pedalThrottleValue_u16;
          uint16_t brakeValue = g_pedalBrakeValue_u16;
          uint16_t clutchValue = g_pedalClutchValue_u16;
          uint16_t handbrakeValue = 0;             // Set if needed

          // Pedal input values to 0 - 10000
          throttleValue = map(throttleValue, 0, 10000, 0, 65535);
          brakeValue = map(brakeValue, 0, 10000, 0, 22000);
          clutchValue = map(clutchValue, 0, 10000, 0, 65535);

          // Set pedal values in FanatecInterface
          fanatec.setThrottle(throttleValue);
          fanatec.setBrake(brakeValue);
          fanatec.setClutch(clutchValue);
          fanatec.setHandbrake(handbrakeValue);
          
          fanatec.update();
        }
      #endif
    }
    //delay(10);
  }
}

void miscTask(void *pvParameters)
{
  for (;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      // Background misc tasks
    }
  }
}

void hidCommunicaitonRxTask(void *pvParameters)
{
  unsigned long scan_Last=0;
  unsigned long action_Last=0;
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      #ifdef USB_JOYSTICK
        // debug output
        if(tinyusbJoystick_.isInDebug)
        {
          ActiveSerial->print("[L]Report: ");
          for(int i =0; i<tinyusbJoystick_.buffSizeDis; i++)
          {
            ActiveSerial->print("0x");  
            if (tinyusbJoystick_.buffDis[i] < 16) ActiveSerial->print('0');
            ActiveSerial->print(tinyusbJoystick_.buffDis[i], HEX);
            ActiveSerial->print("-");
          }
          ActiveSerial->println("");
        }

        
        if(millis()- scan_Last>1000)
        {
          //tinyusbJoystick_.isGetData = false;
          
          scan_Last = millis();
        }
        for(int i=0; i<3;i++)
        {
          if(tinyusbJoystick_.isConfigGet[i])
          {
            //ActiveSerial->println("");
            //ActiveSerial->printf("[L]Get config for pedal: %d\n", i);
            int pedalIdx = tinyusbJoystick_.tmpConfig[i].payloadHeader_st.pedalTag_u8;
            memcpy(&dap_config_st[pedalIdx], &tinyusbJoystick_.tmpConfig[i], sizeof(DapConfig_t));
            configUpdateAvailable[pedalIdx] = true;
            tinyusbJoystick_.isConfigGet[i]=false;
          }
        }
        for(int i =0; i<8; i++)
        {
          if(tinyusbJoystick_.isActionGet[i])
          {
            //ActiveSerial->println("");
            //ActiveSerial->printf("[L]Get action for pedal: %d time: %lu\n", i, millis()-action_Last);
            //action_Last=millis();
            int pedalIdx = tinyusbJoystick_.tmpAction[i].payloadHeader_st.pedalTag_u8;
            uint8_t sysAction = tinyusbJoystick_.tmpAction[i].payloadPedalAction_st.systemAction_u8;
            if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0) {
              pushPedalAssignmentAction(pedalIdx, 0, tinyusbJoystick_.tmpAction[i]);
            } else if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1) {
              pushPedalAssignmentAction(pedalIdx, 1, tinyusbJoystick_.tmpAction[i]);
            } else if (sysAction == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2) {
              pushPedalAssignmentAction(pedalIdx, 2, tinyusbJoystick_.tmpAction[i]);
            } else if (sysAction == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT) {
              clearPedalAssignmentAction(pedalIdx, tinyusbJoystick_.tmpAction[i]);
            } else if (sysAction == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP) {
              if (pedalIdx >= 0 && pedalIdx < 3 && wirelessComm.getPedalMac(pedalIdx)[0] != 0) {
                wirelessComm.sendActionToPedal(pedalIdx, tinyusbJoystick_.tmpAction[i]);
              }
            } else if(pedalIdx == PEDAL_ID_CLUTCH || pedalIdx == PEDAL_ID_BRAKE || pedalIdx == PEDAL_ID_THROTTLE)
            {
              //forward to pedal
              memcpy(&dap_actions_st[pedalIdx], &tinyusbJoystick_.tmpAction[i], sizeof(DapActions_t));
              dap_action_update[pedalIdx] = true;
            }
            tinyusbJoystick_.isActionGet[i]=false;
          }
        }
                        if(tinyusbJoystick_.isMacAddressesGet)
        {
          DapMacAddresses_t macCfg = tinyusbJoystick_.tmpMacAddresses;
          if (macCfg.payloadHeader_st.storeToEeprom_u8 == 1)
          {
            storeMacAddressesToEeprom(macCfg);
            ActiveSerial->println("[L]Stored MAC addresses & channel to EEPROM");
            // Only a commit (storeToEeprom=1) carries a real MAC table - a
            // query (storeToEeprom=0, e.g. the plugin's UI auto-detect on
            // tab load) sends an all-zero payload just to ask for the
            // current table back, and must not overwrite the live one.
            wirelessComm.applyMacConfig(macCfg);
          }

          DapMacAddresses_t reply = loadMacAddressesFromEeprom();
          reply.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
          reply.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
          reply.payloadHeader_st.storeToEeprom_u8 = 0;
          reply.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)&reply.payloadHeader_st, sizeof(reply.payloadHeader_st) + sizeof(reply.payloadMacAddresses_st));
          tinyusbJoystick_.sendData((uint8_t*)&reply, sizeof(DapMacAddresses_t));

          tinyusbJoystick_.isMacAddressesGet = false;
        }
        if(tinyusbJoystick_.isWifiChannelGet)
        {
          uint8_t cmd = tinyusbJoystick_.tmpWifiChannel.payloadWifiChannel_st.command_u8;
          if (cmd == WIFI_CH_CMD_SCAN_REQ)
          {
            handleWifiScanRequest(true);
          }
          else if (cmd == WIFI_CH_CMD_SET_REQ)
          {
            uint8_t targetCh = tinyusbJoystick_.tmpWifiChannel.payloadWifiChannel_st.currentChannel_u8;
            if (targetCh < 1 || targetCh > 14) {
              targetCh = tinyusbJoystick_.tmpWifiChannel.payloadWifiChannel_st.recommendedChannel_u8;
            }
            handleWifiSetChannelRequest(targetCh, true);
          }
          else
          {
            sendWifiChannelStatus(true);
          }
          tinyusbJoystick_.isWifiChannelGet = false;
        }
        if(tinyusbJoystick_.isBridgeActionGet)
        {
          //ActiveSerial->println("");
          //ActiveSerial->printf("[L]Get Bridge action\n");
          memcpy(&dap_bridge_state_lcl, &tinyusbJoystick_.tmpBridgeAction, sizeof(DapBridgeState_t));
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_ENABLE_PAIRING)
          {
            #ifdef ESPNow_Pairing_function
              tinyusbJoystick_.printf("Bridge Pairing...");
              g_softwarePairingAction_b = true;
            #endif
            #ifndef ESPNow_Pairing_function
              tinyusbJoystick_.printf("Pairing command didn't supported");
            #endif
          }
          // action=2, restart
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_RESTART)
          {
            tinyusbJoystick_.printf("Bridge Restart");
            delay(1000);
            ESP.restart();
          }
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_DOWNLOAD_MODE)
          {
            // aciton=3 restart into boot mode
            #ifdef CONFIG_IDF_TARGET_ESP32S3
              tinyusbJoystick_.printf("Bridge Restart into Download mode");
              delay(1000);
              REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
              ESP.restart();
            #else
              ActiveSerial->println("[L]Command not supported ");
              delay(1000);
            #endif
          }
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_DEBUG)
          {
            if (isBridgeInDebugMode_b)
            {
            // aciton=4 print pedal update interval
              tinyusbJoystick_.printf("Bridge debug mode off.");
              isBridgeInDebugMode_b = false;
            }
            else
            {
              // aciton=4 print pedal update interval
              tinyusbJoystick_.printf("Bridge debug mode on.");
              firstDebugMessage_b = true;
              isBridgeInDebugMode_b = true;
            }
          }
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_JOYSTICK_FLASHING_MODE)
          {
            #ifdef External_RP2040
              ActiveSerial->println("[L]JOYSTICK restart into flashing mode");
              dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_RESET_INTO_BOOTLOADER;
            #else
              tinyusbJoystick_.printf("[L]The command is not supported");
            #endif
          }
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_JOYSTICK_DEBUG)
          {
            #ifdef External_RP2040
              ActiveSerial->println("[L]JOYSTICK debug mode on");
              dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_DEBUG_MODE;
            #else
              tinyusbJoystick_.printf("The command is not supported");
            #endif
          }
          if (dap_bridge_state_lcl.payloadBridgeState_st.bridgeAction_u8 == BRIDGE_ACTION_SET_PEDAL_WIRELESS_SYNC)
          {
            wirelessComm.setPedalWirelessSyncEnabled(0, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[0] != 0);
            wirelessComm.setPedalWirelessSyncEnabled(1, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[1] != 0);
            wirelessComm.setPedalWirelessSyncEnabled(2, dap_bridge_state_lcl.payloadBridgeState_st.pedalAvailability_au8[2] != 0);
            for (int pIdx = 0; pIdx < 3; pIdx++)
            {
              if (!wirelessComm.isPedalWirelessSyncEnabled(pIdx))
              {
                g_joystickValueOriginal_au16[pIdx] = JOYSTICK_MIN_VALUE;
                g_joystickValue_au16[pIdx] = JOYSTICK_MIN_VALUE;
                dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pIdx] = 0;
                dap_bridge_state_st.payloadBridgeState_st.pedalRssiRealtime_ai32[pIdx] = 0;
                wirelessComm.setRssi(pIdx, 0);
                if (pIdx == 0) g_pedalClutchValue_u16 = JOYSTICK_MIN_VALUE;
                if (pIdx == 1) g_pedalBrakeValue_u16 = JOYSTICK_MIN_VALUE;
                if (pIdx == 2) g_pedalThrottleValue_u16 = JOYSTICK_MIN_VALUE;
              }
            }
          }
          tinyusbJoystick_.isBridgeActionGet=false;
        }
        if(tinyusbJoystick_.isOtaActionGet)
        {
          tinyusbJoystick_.printf("Get OTA Packet");
          memcpy(&dap_action_ota_st, &tinyusbJoystick_.tmpOtaAction, sizeof(DapActionOta_t));
          #ifdef OTA_Update
          bool structChecker_b = true;
          if (dap_action_ota_st.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_ACTION_OTA_U8)
          {
            structChecker_b = false;
            //structIsValid = false;
          }
          if (structChecker_b)
          {
            SSID = new char[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8 + 1];
            PASS = new char[dap_action_ota_st.payloadOtaInfo_st.passLength_u8 + 1];
            memcpy(SSID, dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8, dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8);
            memcpy(PASS, dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8, dap_action_ota_st.payloadOtaInfo_st.passLength_u8);
            SSID[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8] = 0;
            PASS[dap_action_ota_st.payloadOtaInfo_st.passLength_u8] = 0;
          }
          if (dap_action_ota_st.payloadOtaInfo_st.deviceId_u8 == DEVICE_ID && structChecker_b == true)
          {
            OTA_enable_b = true;
            tinyusbJoystick_.printf("Bridge OTA begin.");
          }
          else if (structChecker_b)
          {
            g_pedalOtaAction_b = true;
          }
          #endif
          tinyusbJoystick_.isOtaActionGet = false;    
        }
      #endif
    }
  }
}

void hidCommunicaitonTxTask(void *pvParameters)
{
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) 
    {
      #ifdef USB_JOYSTICK
        uint16_t crc;
        unsigned long current_time=millis();
        
        if(current_time-bridge_state_last_update>200)
        {
          basic_rssi_update=true;
          bridge_state_last_update=millis();
        }
        
        if(current_time-PedalUpdateLast>500)
        {
          PedalUpdateIntervalPrint_b=true;
          PedalUpdateLast=current_time;
        }
        /*
        if(current_time-UARTJoystickUpdateLast>7)
        {
          UARTJoystickUpdate_b=true;
          UARTJoystickUpdateLast=current_time;
        }
        */
        bool structChecker = true;
        
        for(int i =0; i<3; i++)
        {
          if(g_updateBasicState_ab[i])
          {
            g_updateBasicState_ab[i]=false;

            if(isBridgeInDebugMode_b) {
              tinyusbJoystick_.printf("BasicState gesendet - Tag: %d", dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
            }
            
            tinyusbJoystick_.sendData((uint8_t*)&dap_state_basic_st[i], sizeof(DapStateBasic_t));
            if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]==0)
            {
              ActiveSerial->print("[L]Found Pedal:");
              ActiveSerial->println(dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
              tinyusbJoystick_.printf("Found Pedal: %d",dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
            }
            dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]=1;
            //g_pedalLastUpdate_au32[dap_state_basic_st[i].payloadHeader_st.pedalTag_u8]=millis();
            if(g_espNowError_ab[i])
            {
              ActiveSerial->print("[L]Pedal:");
              ActiveSerial->print(dap_state_basic_st[i].payloadHeader_st.pedalTag_u8);
              ActiveSerial->print(" E:");
              ActiveSerial->println(dap_state_basic_st[i].payloadPedalStateBasic_st.errorCode_u8);
              tinyusbJoystick_.printf("Pedal: %d E: %d",dap_state_basic_st[i].payloadHeader_st.pedalTag_u8,dap_state_basic_st[i].payloadPedalStateBasic_st.errorCode_u8);
              g_espNowError_ab[i]=false;    
            }
          }
          if(g_updateExtendState_ab[i])
          {
            g_updateExtendState_ab[i]=false;
            tinyusbJoystick_.sendData((uint8_t*)&dap_state_extended_st[i], sizeof(DapStateExtended_t));
            

          }

          // Forward Servo Config Responses to Host via HID ---
          if(send_servo_config_to_host[i])
          {
            send_servo_config_to_host[i] = false;
            tinyusbJoystick_.sendData((uint8_t*)&dap_servo_config_response_st[i], sizeof(DAP_servo_config_st_t));
            // TEMP DIAGNOSTIC (unconditional - rare/user-triggered action).
            // Remove once wireless servo register exchange is confirmed fixed.
            ActiveSerial->printf("[L][DIAG] ServoConfig response forwarded to host for Pedal #%d\n", i);
          }
        }
        

        int pedal_config_IDX=0;
        for(pedal_config_IDX=0;pedal_config_IDX<3;pedal_config_IDX++)
        {
          if(g_espNowRequestConfig_ab[pedal_config_IDX])
          {
            DapConfig_t * dap_config_st_local_ptr;
            DapConfig_t dap_config_st_local;
            if(pedal_config_IDX==0)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Clu, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Clu;
            }
            if(pedal_config_IDX==1)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Brk, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Brk;
            }
            if(pedal_config_IDX==2)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Gas, sizeof(DapConfig_t));
              //dap_config_st_local_ptr = &dap_config_st_Gas;
            }
            dap_config_st_local_ptr= &dap_config_st_local;
            
            //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payloadHeader_st)), sizeof(dap_config_st.payloadHeader_st) + sizeof(dap_config_st.payloadPedalConfig_st));

            dap_config_st_local_ptr->payloadHeader_st.pedalTag_u8=dap_config_st_local_ptr->payloadPedalConfig_st.pedalType_u8;
            crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payloadHeader_st)), sizeof(dap_config_st_local.payloadHeader_st) + sizeof(dap_config_st_local.payloadPedalConfig_st));
            dap_config_st_local_ptr->payloadFooter_st.checkSum_u16 = crc;
            tinyusbJoystick_.sendData((uint8_t*)&dap_config_st_local.payloadHeader_st, sizeof(DapConfig_t));
            g_espNowRequestConfig_ab[pedal_config_IDX]=false;
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedal_config_IDX);
            ActiveSerial->println(" config returned");
            delay(3);
          }
        }

        
        if(basic_rssi_update)//Bridge action
        {
          //fill header and footer
          dap_bridge_state_st.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
          dap_bridge_state_st.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
          dap_bridge_state_st.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
          dap_bridge_state_st.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
          int rssi_filter_value=constrain(rssi_filter.process(g_rssiDisplay_i32),-100,0) ;
          dap_bridge_state_st.payloadHeader_st.pedalTag_u8=5; //5 means bridge
          dap_bridge_state_st.payloadHeader_st.payloadType_u8=DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8;
          dap_bridge_state_st.payloadHeader_st.version_u8=DAP_VERSION_CONFIG_U8;
          dap_bridge_state_st.payloadBridgeState_st.bridgeAction_u8=0;
          memcpy(dap_bridge_state_st.payloadBridgeState_st.pedalRssiRealtime_ai32,wirelessComm.getRssiArray(),sizeof(int32_t)*3);
          //parse_version(BRIDGE_FIRMWARE_VERSION,&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[0],&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[1],&dap_bridge_state_st.payloadBridgeState_st.Bridge_firmware_version_u8[2]);
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[0]=versionMajor;
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[1]=versionMinor;
          dap_bridge_state_st.payloadBridgeState_st.bridgeFirmwareVersion_au8[2]=versionPatch;
          uint8_t unassignedCount = 0;
          memset(dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8, 0, sizeof(dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8));
          for (int p = 0; p < 3; p++) {
            memcpy(&dap_bridge_state_st.payloadBridgeState_st.macAddressDetected_au8[p * 6], wirelessComm.getPedalMac(p), 6);
          }
          dap_bridge_state_st.payloadBridgeState_st.unassignedPedalCount_u8 = unassignedCount;
          //CRC check should be in the final
          crc = checksumCalculator((uint8_t*)(&(dap_bridge_state_st.payloadHeader_st)), sizeof(dap_bridge_state_st.payloadHeader_st) + sizeof(dap_bridge_state_st.payloadBridgeState_st));
          dap_bridge_state_st.payloadFooter_st.checkSum_u16=crc;
          DapBridgeState_t * dap_bridge_st_local_ptr;
          dap_bridge_st_local_ptr = &dap_bridge_state_st;
          tinyusbJoystick_.sendData((uint8_t*)&dap_bridge_state_st, sizeof(DapBridgeState_t));
          basic_rssi_update=false;
        }
        PayloadHidMessage_t receivedMsg;
        if (xQueueReceive(g_messageQueueHandle_pv, &receivedMsg, (TickType_t)0) == pdTRUE)
        {
          tinyusbJoystick_.sendData((uint8_t*)&receivedMsg, sizeof(PayloadHidMessage_t));
          delay(1);
        }
        //
        if(PedalUpdateIntervalPrint_b)
        {
          if(isBridgeInDebugMode_b)
          {
            if(firstDebugMessage_b)
            {
              firstDebugMessage_b = false;
              tinyusbJoystick_.printf("Bridge Board:%s, Version:%s.", BRIDGE_BOARD, BRIDGE_FIRMWARE_VERSION); 
              tinyusbJoystick_.printf("Bridge Original Mac: %02X:%02X:%02X:%02X:%02X:%02X", wirelessComm.getOwnMac()[0], wirelessComm.getOwnMac()[1], wirelessComm.getOwnMac()[2], wirelessComm.getOwnMac()[3], wirelessComm.getOwnMac()[4], wirelessComm.getOwnMac()[5]);
              uint8_t espMac_au8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
              WiFi.macAddress(espMac_au8);  
              tinyusbJoystick_.printf("Bridge Mac OverWritted: %02X:%02X:%02X:%02X:%02X:%02X", espMac_au8[0], espMac_au8[1], espMac_au8[2], espMac_au8[3], espMac_au8[4], espMac_au8[5]);
              
            }
            for(int pedalIDX=0;pedalIDX<3;pedalIDX++)
            {
              if(dap_bridge_state_st.payloadBridgeState_st.pedalAvailability_au8[pedalIDX]==1)
              {
                tinyusbJoystick_.printf("Pedal %d, Update Interval: %d, RSSI: %d", pedalIDX, (int)(millis()-g_pedalLastUpdate_au32[pedalIDX]),wirelessComm.getRssi(pedalIDX));
                //delay(10);
              }
            }
          }
          PedalUpdateIntervalPrint_b=false;
        }

      #endif
    }
  }
}
