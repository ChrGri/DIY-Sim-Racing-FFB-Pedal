
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

DAP_config_st dap_config_st[3];
DAP_calculationVariables_st dap_calculationVariables_st;
DAP_state_basic_st dap_state_basic_st[3];
DAP_state_extended_st dap_state_extended_st[3];
DAP_actions_st dap_actions_st[3];
DAP_actions_st dap_actionassignment_st[3];
DAP_bridge_state_st dap_bridge_state_st;
DAP_config_st dap_config_st_Clu;
DAP_config_st dap_config_st_Brk;
DAP_config_st dap_config_st_Gas;
DAP_config_st dap_config_st_Temp;
DAP_ESPPairing_st dap_esppairing_st;//saving
DAP_ESPPairing_st dap_esppairing_lcl;//sending
//DAP_config_st dap_config_st_store[3];
DAP_bridge_state_st dap_bridge_state_lcl;//
DAP_action_ota_st dap_action_ota_st;
#include "ESPNOW_lib.h"


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
  //DAP_config_st dap_config_st_local;
int32_t joystickNormalizedToInt32 = 0;                           
bool resetPedalPosition = false;
bool dap_action_update[3]= {false,false,false};
MovingAverageFilter rssi_filter(30);
int32_t joystickNormalizedToInt32_local = 0;
unsigned long pedal_last_update[3]={1,1,1};
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
    taskScheduler.addScheduledTask(hidCommunicaitonRxTask, "Hid Rx", 2000, 1, 1, 3000);
    taskScheduler.addScheduledTask(hidCommunicaitonTxTask, "Hid Tx", 2000, 1, 1, 3000);
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
  messageQueueHandle = xQueueCreate(10, sizeof(Dap_hidmessage_st));
  if (messageQueueHandle == NULL)
  {
    ActiveSerial->println("[L]Error during xqueue creation.");
    ESP.restart();
  }
  #ifdef ESPNow_Pairing_function
    //button read setup
    pinMode(Pairing_GPIO, INPUT_PULLUP);
    EEPROM.begin(256);
  #endif
  #if !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C6)
    disableCore0WDT();
    disableCore1WDT();
  #endif
  //enable ESP-NOW
  ESPNow_initialize();
  
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
    pedal_last_update[pedalIDX]=millis();
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
    dap_action_ota_st.payloadOtaInfo_.WIFI_PASS[i]=0;
    dap_action_ota_st.payloadOtaInfo_.WIFI_SSID[i]=0;
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
  tinyusbJoystick_.printf("Setup end.");
  
  
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

void espNowCommunicationTxTask( void * pvParameters )
{
  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) 
    {
      #ifdef ESPNow_Pairing_function
        if(digitalRead(Pairing_GPIO)==LOW||software_pairing_action_b)
        {
          ActiveSerial->println("[L]Bridge Pairing.....");
          delay(1000);
          Pairing_state_start=millis();
          Pairing_state_last_sending=millis();
          ESPNow_pairing_action_b=true;
          building_dap_esppairing_lcl=true;
          LED_Status=1;
          software_pairing_action_b=false;
        }
        if(ESPNow_pairing_action_b)
        {
          unsigned long now=millis();
          //sending package
          if(building_dap_esppairing_lcl)
          {
            uint16_t crc=0;          
            building_dap_esppairing_lcl=false;
            dap_esppairing_lcl.payloadESPNowInfo_._deviceID=deviceID;
            dap_esppairing_lcl.payLoadHeader_.payloadType=DAP_PAYLOAD_TYPE_ESPNOW_PAIRING;
            dap_esppairing_lcl.payLoadHeader_.PedalTag=deviceID;
            dap_esppairing_lcl.payLoadHeader_.version=DAP_VERSION_CONFIG;
            crc = checksumCalculator((uint8_t*)(&(dap_esppairing_lcl.payLoadHeader_)), sizeof(dap_esppairing_lcl.payLoadHeader_) + sizeof(dap_esppairing_lcl.payloadESPNowInfo_));
            dap_esppairing_lcl.payloadFooter_.checkSum=crc;
          }
          if(now-Pairing_state_last_sending>400)
          {
            Pairing_state_last_sending=now;
            ESPNow.send_message(broadcast_mac,(uint8_t *) &dap_esppairing_lcl, sizeof(dap_esppairing_lcl));
          }
          

          //timeout check
          if(now-Pairing_state_start>Pairing_timeout)
          {
            ActiveSerial->println("[L]Bridge Pairing timeout!");
            ESPNow_pairing_action_b=false;
            LED_Status=0;
            if(UpdatePairingToEeprom)
            {
              EEPROM.put(EEPROM_offset,_ESP_pairing_reg);
              EEPROM.commit();
              UpdatePairingToEeprom=false;
              
              //list eeprom
              ESP_pairing_reg ESP_pairing_reg_local;
              EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
              for(int i=0;i<4;i++)
              {
                if(ESP_pairing_reg_local.Pair_status[i]==1)
                {                
                  ActiveSerial->print("[L]#");
                  ActiveSerial->print(i);
                  ActiveSerial->print("Pair: ");
                  ActiveSerial->print(ESP_pairing_reg_local.Pair_status[i]);
                  ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X", ESP_pairing_reg_local.Pair_mac[i][0], ESP_pairing_reg_local.Pair_mac[i][1], ESP_pairing_reg_local.Pair_mac[i][2], ESP_pairing_reg_local.Pair_mac[i][3], ESP_pairing_reg_local.Pair_mac[i][4], ESP_pairing_reg_local.Pair_mac[i][5]);
                  ActiveSerial->println("");
                }
                ActiveSerial->println("");
              }
              ActiveSerial->println("");
              //adding peer
              /*
              for(int i=0; i<4;i++)
              {
                if(_ESP_pairing_reg.Pair_status[i]==1)
                {
                  if(i==0)
                  {
                    ESPNow.remove_peer(Clu_mac);
                    memcpy(&Clu_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(300);
                    ESPNow.add_peer(Clu_mac);
                    
                  }
                  if(i==1)
                  {
                    ESPNow.remove_peer(Brk_mac);
                    memcpy(&Brk_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(300);
                    ESPNow.add_peer(Brk_mac);
                    //ActiveSerial->printf("[L]Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", Brk_mac[0], Brk_mac[1], Brk_mac[2], Brk_mac[3], Brk_mac[4], Brk_mac[5]);

                  }
                  if(i==2)
                  {
                    ESPNow.remove_peer(Gas_mac);
                    memcpy(&Gas_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(300);
                    ESPNow.add_peer(Gas_mac);
                  }        
                  if(i==3)
                  {
                    ESPNow.remove_peer(esp_Host);
                    memcpy(&esp_Host,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(300);
                    ESPNow.add_peer(esp_Host);                
                  }        
                }
              }
              */
            }
            
          }
        }
      #endif
      for(int i=0;i<3;i++)
      {
        if(configUpdateAvailable[i])
        {
          configUpdateAvailable[i] = false;
          if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[i]==1)
          {
            esp_err_t err;
            switch (i)
            {
              case PEDAL_ID_CLUTCH:
                ActiveSerial->print("[L]Sending Clutch config,Result:");
                err = ESPNow.send_message(Clu_mac, (uint8_t *)&dap_config_st[i], sizeof(DAP_config_st));
                break;
              case PEDAL_ID_BRAKE:
                ActiveSerial->print("[L]Sending Brake config,Result:");
                err = ESPNow.send_message(Brk_mac, (uint8_t *)&dap_config_st[i], sizeof(DAP_config_st));
                break;
              case PEDAL_ID_THROTTLE:
                ActiveSerial->print("[L]Sending Throttle config,Result:");
                err = ESPNow.send_message(Gas_mac, (uint8_t *)&dap_config_st[i], sizeof(DAP_config_st));
                break;
              default:
                break;
            }
            ActiveSerial->println(esp_err_to_name(err));
          }
        }
      }
      

      for(int i=0; i<3; i++)
      {
        if(dap_action_update[i] )
        {
          dap_action_update[i]=false;
          if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[i]==1)
          {
            switch (i)
            {
              case PEDAL_ID_CLUTCH:
                ESPNow.send_message(Clu_mac,(uint8_t *) &dap_actions_st[i],sizeof(DAP_actions_st));
                break;
              case PEDAL_ID_BRAKE:
                ESPNow.send_message(Brk_mac,(uint8_t *) &dap_actions_st[i],sizeof(DAP_actions_st));
                break;
              case PEDAL_ID_THROTTLE:
                ESPNow.send_message(Gas_mac,(uint8_t *) &dap_actions_st[i],sizeof(DAP_actions_st));
                break;
              default:
                break;
            }
          }
        }

        if(sendAssignment_b[i])
        {
          sendAssignment_b[i]=false;
          esp_err_t err;
          auto it = unassignedPeersList.begin();
          std::advance(it, i);
          err = ESPNow.send_message(it->mac, (uint8_t *)&dap_actionassignment_st[i], sizeof(DAP_actions_st));
          ActiveSerial->printf("[L]Send assignment to pedal: %0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X, result: ", it->mac[0], it->mac[1], it->mac[2], it->mac[3], it->mac[4], it->mac[5]);
          ActiveSerial->println(esp_err_to_name(err));
        }
      }
    
      //forward the basic wifi info for pedals
      if(pedal_OTA_action_b)
      {
        switch(dap_action_ota_st.payloadOtaInfo_.device_ID)
        {
          case 0:
            ESPNow.send_message(Clu_mac,(uint8_t *) &dap_action_ota_st,sizeof(DAP_action_ota_st));
            ActiveSerial->println("[L]Forward OTA command to Clutch");
          break;
          case 1:
            ESPNow.send_message(Brk_mac,(uint8_t *) &dap_action_ota_st,sizeof(DAP_action_ota_st));
            ActiveSerial->println("[L]Forward OTA command to Brake");
          break;
          case 2:
            ESPNow.send_message(Gas_mac,(uint8_t *) &dap_action_ota_st,sizeof(DAP_action_ota_st));
            ActiveSerial->println("[L]Forward OTA command to Throttle");
          break;
        }
        pedal_OTA_action_b=false;
      }
    }    
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
        case DAP_PAYLOAD_TYPE_CONFIG:
            return sizeof(DAP_config_st);
        case DAP_PAYLOAD_TYPE_ACTION:
            return sizeof(DAP_actions_st);
        case DAP_PAYLOAD_TYPE_ACTION_OTA:
            return sizeof(DAP_action_ota_st);
        case DAP_PAYLOAD_TYPE_BRIDGE_STATE:
            return sizeof(DAP_bridge_state_st);
        // Add other packet types here in the future
        default:
            return 0;
    }
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
        if (rx_buffer[buffer_idx] != SOF_BYTE_0 || (buffer_idx + 1 < buffer_len && rx_buffer[buffer_idx + 1] != SOF_BYTE_1))
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
        if (rx_buffer[buffer_idx + expectedSize - 2] != EOF_BYTE_0 || rx_buffer[buffer_idx + expectedSize - 1] != EOF_BYTE_1)
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
          case DAP_PAYLOAD_TYPE_CONFIG:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DAP_config_st dap_config_st_local;
              memcpy(&dap_config_st_local, packet_start, sizeof(DAP_config_st));
              //ActiveSerial->readBytes((char *)&dap_config_st_local, sizeof(DAP_config_st));
              if (dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG)
              {
                structChecker = false;
                structIsValid=false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_config_st_local.payLoadHeader_.payloadType);
              }
              if (dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_config_st_local.payLoadHeader_.version);
              }
              // checksum validation
              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
              if (crc != dap_config_st_local.payloadFooter_.checkSum)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_config_st_local.payloadFooter_.checkSum);
              }
              // if checks are successfull, overwrite global configuration struct
              if (structChecker == true)
              {
                int pedalIdx = dap_config_st_local.payLoadHeader_.PedalTag;
                // ActiveSerial->println("[L]Updating pedal config");
                memcpy(&dap_config_st[pedalIdx], &dap_config_st_local, sizeof(DAP_config_st));
                configUpdateAvailable[pedalIdx] = true;
              }
            #endif
            break;
          }
          //case action to pedal
          case DAP_PAYLOAD_TYPE_ACTION:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DAP_actions_st dap_actions_st_local;
              memcpy(&dap_actions_st_local, packet_start, sizeof(DAP_actions_st));
              //memcpy(&dap_actions_st_local, packet_start, sizeof(DAP_config_st));
              //ActiveSerial->readBytes((char *)&dap_actions_st_local, sizeof(DAP_actions_st));
              if (dap_actions_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_ACTION);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_actions_st_local.payLoadHeader_.payloadType);
              }
              if (dap_actions_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_actions_st_local.payLoadHeader_.version);
              }

              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_actions_st_local.payLoadHeader_)), sizeof(dap_actions_st_local.payLoadHeader_) + sizeof(dap_actions_st_local.payloadPedalAction_));
              if (crc != dap_actions_st_local.payloadFooter_.checkSum)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_actions_st_local.payloadFooter_.checkSum);
              }
              if (structChecker == true)
              {
                
                int pedalIdx = dap_actions_st_local.payLoadHeader_.PedalTag;
                if(pedalIdx == PEDAL_ID_CLUTCH || pedalIdx == PEDAL_ID_BRAKE || pedalIdx == PEDAL_ID_THROTTLE)
                {
                  //forward to pedal
                  memcpy(&dap_actions_st[pedalIdx], &dap_actions_st_local, sizeof(DAP_actions_st));
                  dap_action_update[pedalIdx] = true;
                }
                if (pedalIdx == PEDAL_ID_TEMP_1 || pedalIdx == PEDAL_ID_TEMP_2 || pedalIdx == PEDAL_ID_TEMP_3)
                {
                  //make those assignement action to pedal with specific mac address
                  int tempIdx = pedalIdx - PEDAL_ID_TEMP_1;
                  memcpy(&dap_actionassignment_st[tempIdx], &dap_actions_st_local, sizeof(DAP_actions_st));
                  sendAssignment_b[tempIdx] = true;
                  //dap_action_update[pedalIdx] = true;
                }
              }
            #endif
            break;
          }

          //case action for ota
          case DAP_PAYLOAD_TYPE_ACTION_OTA:
          {
            #ifndef USB_JOYSTICK
              ActiveSerial->println("[L]get OTA command and its info");
              memcpy(&dap_action_ota_st, packet_start, sizeof(DAP_action_ota_st));
              //ActiveSerial->readBytes((char *)&dap_action_ota_st, sizeof(DAP_action_ota_st));
              #ifdef OTA_Update
                bool structChecker_b = true;
                if (dap_action_ota_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION_OTA)
                {
                  structChecker_b = false;
                  structIsValid = false;
                }
                if (structChecker_b)
                {
                  SSID = new char[dap_action_ota_st.payloadOtaInfo_.SSID_Length + 1];
                  PASS = new char[dap_action_ota_st.payloadOtaInfo_.PASS_Length + 1];
                  memcpy(SSID, dap_action_ota_st.payloadOtaInfo_.WIFI_SSID, dap_action_ota_st.payloadOtaInfo_.SSID_Length);
                  memcpy(PASS, dap_action_ota_st.payloadOtaInfo_.WIFI_PASS, dap_action_ota_st.payloadOtaInfo_.PASS_Length);
                  SSID[dap_action_ota_st.payloadOtaInfo_.SSID_Length] = 0;
                  PASS[dap_action_ota_st.payloadOtaInfo_.PASS_Length] = 0;
                  /*
                  ActiveSerial->printf("[L]Device ID:%d\n",dap_action_ota_st.payloadOtaInfo_.device_ID);
                  ActiveSerial->print("[L]SSID(uint)=");
                  for(uint i=0; i<dap_action_ota_st.payloadOtaInfo_.SSID_Length;i++)
                  {
                    ActiveSerial->print(dap_action_ota_st.payloadOtaInfo_.WIFI_SSID[i]);
                    ActiveSerial->print(",");
                  }
                  ActiveSerial->println(" ");
                  ActiveSerial->print("[L]PASS(uint)=");
                  for(uint i=0; i<dap_action_ota_st.payloadOtaInfo_.PASS_Length;i++)
                  {
                    ActiveSerial->print(dap_action_ota_st.payloadOtaInfo_.WIFI_PASS[i]);
                    ActiveSerial->print(",");
                  }
                  ActiveSerial->println(" ");

                  ActiveSerial->print("[L]SSID=");
                  ActiveSerial->println(SSID);
                  ActiveSerial->print("[L]PASS=");
                  ActiveSerial->println(PASS);
                  */
                }

                if (dap_action_ota_st.payloadOtaInfo_.device_ID == DEVICE_ID && structChecker_b == true)
                {
                  OTA_enable_b = true;
                  ActiveSerial->println("[L] Bridge OTA begin.");
                }
                else if (structChecker_b)
                {
                  pedal_OTA_action_b = true;
                }
              #endif
            #endif
            break;
          }
          case DAP_PAYLOAD_TYPE_BRIDGE_STATE:
          {
            #ifndef USB_JOYSTICK
              bool structChecker = true;
              DAP_bridge_state_st dap_bridge_state_local;
              //dap_bridge_state_local_ptr = &dap_bridge_state_lcl;
              memcpy(&dap_bridge_state_local, packet_start, sizeof(DAP_bridge_state_st));
              //ActiveSerial->readBytes((char *)dap_bridge_state_local_ptr, sizeof(DAP_bridge_state_st));
              // check if data is plausible
              if (dap_bridge_state_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_BRIDGE_STATE)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Payload type expected: ");
                ActiveSerial->print(DAP_PAYLOAD_TYPE_BRIDGE_STATE);
                ActiveSerial->print(",   Payload type received: ");
                ActiveSerial->println(dap_bridge_state_local.payLoadHeader_.payloadType);
              }
              if (dap_bridge_state_local.payLoadHeader_.version != DAP_VERSION_CONFIG)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]Config version expected: ");
                ActiveSerial->print(DAP_VERSION_CONFIG);
                ActiveSerial->print(",   Config version received: ");
                ActiveSerial->println(dap_bridge_state_lcl.payLoadHeader_.version);
              }
              // checksum validation
              uint16_t crc = checksumCalculator((uint8_t *)(&(dap_bridge_state_local.payLoadHeader_)), sizeof(dap_bridge_state_local.payLoadHeader_) + sizeof(dap_bridge_state_local.payloadBridgeState_));
              if (crc != dap_bridge_state_local.payloadFooter_.checkSum)
              {
                structChecker = false;
                structIsValid = false;
                ActiveSerial->print("[L]CRC expected: ");
                ActiveSerial->print(crc);
                ActiveSerial->print(",   CRC received: ");
                ActiveSerial->println(dap_bridge_state_local.payloadFooter_.checkSum);
              }
              // if checks are successfull, overwrite global configuration struct
              if (structChecker == true)
              {
                memcpy(&dap_bridge_state_lcl, &dap_bridge_state_local, sizeof(DAP_bridge_state_st));
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_ENABLE_PAIRING)
                {
                  #ifdef ESPNow_Pairing_function
                    ActiveSerial->println("[L]Bridge Pairing...");
                    software_pairing_action_b = true;
                  #endif
                  #ifndef ESPNow_Pairing_function
                    ActiveSerial->println("[L]Pairing command didn't supported");
                  #endif
                }
                // action=2, restart
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_RESTART)
                {
                  ActiveSerial->println("[L]Bridge Restart");
                  delay(1000);
                  ESP.restart();
                }
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_DOWNLOAD_MODE)
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
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_DEBUG)
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
                    isBridgeInDebugMode_b = true;
                  }
                }
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_JOYSTICK_FLASHING_MODE)
                {
                  #ifdef External_RP2040
                    ActiveSerial->println("[L]JOYSTICK restart into flashing mode");
                    dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_RESET_INTO_BOOTLOADER;
                  #else
                    ActiveSerial->println("[L]The command is not supported");
                  #endif
                }
                if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_JOYSTICK_DEBUG)
                {
                  #ifdef External_RP2040
                    ActiveSerial->println("[L]JOYSTICK debug mode on");
                    dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_DEBUG_MODE;
                  #else
                    ActiveSerial->println("[L]The command is not supported");
                  #endif
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
          if(update_basic_state[i])
          {
            update_basic_state[i]=false;
            ActiveSerial->write((char*)&dap_state_basic_st[i], sizeof(DAP_state_basic_st));
            ActiveSerial->print("\r\n");
            if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[dap_state_basic_st[i].payLoadHeader_.PedalTag]==0)
            {
              ActiveSerial->print("[L]Found Pedal:");
              ActiveSerial->println(dap_state_basic_st[i].payLoadHeader_.PedalTag);
            }
            dap_bridge_state_st.payloadBridgeState_.Pedal_availability[dap_state_basic_st[i].payLoadHeader_.PedalTag]=1;
            pedal_last_update[dap_state_basic_st[i].payLoadHeader_.PedalTag]=millis();
            if(ESPNow_error_b[i])
            {
              ActiveSerial->print("[L]Pedal:");
              ActiveSerial->print(dap_state_basic_st[i].payLoadHeader_.PedalTag);
              ActiveSerial->print(" E:");
              ActiveSerial->println(dap_state_basic_st[i].payloadPedalState_Basic_.error_code_u8);
              ESPNow_error_b[i]=false;    
            }
          }
          if(update_extend_state[i])
          {
            update_extend_state[i]=false;
            ActiveSerial->write((char*)&dap_state_extended_st[i], sizeof(DAP_state_extended_st));
            ActiveSerial->print("\r\n");

          }
        }
      
        int pedal_config_IDX=0;
        for(pedal_config_IDX=0;pedal_config_IDX<3;pedal_config_IDX++)
        {
          if(ESPNow_request_config_b[pedal_config_IDX])
          {
            DAP_config_st * dap_config_st_local_ptr;
            DAP_config_st dap_config_st_local;
            if(pedal_config_IDX==0)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Clu, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Clu;
            }
            if(pedal_config_IDX==1)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Brk, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Brk;
            }
            if(pedal_config_IDX==2)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Gas, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Gas;
            }
            dap_config_st_local_ptr= &dap_config_st_local;
            
            //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));

            dap_config_st_local_ptr->payLoadHeader_.PedalTag=dap_config_st_local_ptr->payLoadPedalConfig_.pedal_type;
            crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
            dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
            ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
            ActiveSerial->print("\r\n");
            ESPNow_request_config_b[pedal_config_IDX]=false;
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedal_config_IDX);
            ActiveSerial->println(" config returned");
            delay(3);
          }
        }
      

        if(basic_rssi_update)//Bridge action
        {
          //fill header and footer
          dap_bridge_state_st.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
          dap_bridge_state_st.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
          dap_bridge_state_st.payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
          dap_bridge_state_st.payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
          int rssi_filter_value=constrain(rssi_filter.process(rssi_display),-100,0) ;
          dap_bridge_state_st.payloadBridgeState_.unassignedPedalCount=(byte)unassignedPeersList.size();
          dap_bridge_state_st.payLoadHeader_.PedalTag=5; //5 means bridge
          dap_bridge_state_st.payLoadHeader_.payloadType=DAP_PAYLOAD_TYPE_BRIDGE_STATE;
          dap_bridge_state_st.payLoadHeader_.version=DAP_VERSION_CONFIG;
          dap_bridge_state_st.payloadBridgeState_.Bridge_action=0;
          memcpy(dap_bridge_state_st.payloadBridgeState_.Pedal_RSSI_Realtime,rssi,sizeof(int32_t)*3);
          //parse_version(BRIDGE_FIRMWARE_VERSION,&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0],&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1],&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2]);
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0]=versionMajor;
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1]=versionMinor;
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2]=versionPatch;
          int indexMac = 0;
          for (UnassignedPeer &item : unassignedPeersList) 
          {
            memcpy(&dap_bridge_state_st.payloadBridgeState_.macAddressDetected[indexMac], item.mac,6);
            indexMac=indexMac+6;
          }
          //CRC check should be in the final
          crc = checksumCalculator((uint8_t*)(&(dap_bridge_state_st.payLoadHeader_)), sizeof(dap_bridge_state_st.payLoadHeader_) + sizeof(dap_bridge_state_st.payloadBridgeState_));
          dap_bridge_state_st.payloadFooter_.checkSum=crc;
          DAP_bridge_state_st * dap_bridge_st_local_ptr;
          dap_bridge_st_local_ptr = &dap_bridge_state_st;
          ActiveSerial->write((char*)dap_bridge_st_local_ptr, sizeof(DAP_bridge_state_st));
          ActiveSerial->print("\r\n");
          basic_rssi_update=false;
          /*
          if(rssi_filter_value<-88)
          {
            ActiveSerial->println("Warning: BAD WIRELESS CONNECTION");
            //ActiveSerial->print("Pedal:");
            //ActiveSerial->print(dap_state_basic_st.payLoadHeader_.PedalTag);
            ActiveSerial->print(" RSSI:");
            ActiveSerial->println(rssi_filter_value);  
          }
          */
          #ifdef ESPNow_debug
              ActiveSerial->print("Pedal:");
              ActiveSerial->print(dap_state_basic_st.payLoadHeader_.PedalTag);
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
            dap_joystickUART_state_lcl._payloadjoystick.controllerValue_i32[i]=Joystick_value_original[i];
            dap_joystickUART_state_lcl._payloadjoystick.pedalAvailability[i] = dap_bridge_state_st.payloadBridgeState_.Pedal_availability[i];
          }
          dap_joystickUART_state_lcl._payloadjoystick.pedal_status=pedal_status;
          dap_joystickUART_state_lcl._payloadfooter.checkSum= checksumCalculator((uint8_t*)(&(dap_joystickUART_state_lcl._payloadjoystick)), sizeof(dap_joystickUART_state_lcl._payloadjoystick));
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
        if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[pedalIDX]==1)
        {
          if(current_time-pedal_last_update[pedalIDX]>3000)
          {
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedalIDX);
            ActiveSerial->println(" Disconnected");
            dap_bridge_state_st.payloadBridgeState_.Pedal_availability[pedalIDX]=0;

          }
        }  
      }
      //print log from espnow
      #ifndef USB_JOYSTICK
        Dap_hidmessage_st receivedMsg;
        if (xQueueReceive(messageQueueHandle, &receivedMsg, (TickType_t)0) == pdTRUE)
        {
          ActiveSerial->print("[L]");
          ActiveSerial->println(receivedMsg.text);
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
              if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[pedalIDX]==1)
              {
                ActiveSerial->print("[L]Pedal ");
                ActiveSerial->print(pedalIDX);
                ActiveSerial->print(" Update interval: ");
                ActiveSerial->print(current_time-pedal_last_update[pedalIDX]);
                ActiveSerial->print(" RSSI: ");
                ActiveSerial->println(rssi[pedalIDX]);
              }
              
            }
            ActiveSerial->print("[L]sending:");
            print_struct_hex(&dap_bridge_state_st);
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
        if (pedalJoystick_last[i] != Joystick_value_original[i])
        {
          pedalJoystick_last[i] = Joystick_value_original[i];
          pedalJoystickUpdate_b = true;
        }
      }
      #ifdef USB_JOYSTICK
        if (IsControllerReady() /*&& pedalJoystickUpdate_b*/)
        {
          if(isBridgeInDebugMode_b)
          {
            ActiveSerial->print("[L]Throttle value:");
            ActiveSerial->println(pedal_throttle_value);
            ActiveSerial->print("[L]Brake value:");
            ActiveSerial->println(pedal_brake_value);
            ActiveSerial->print("[L]Cluth value:");
            ActiveSerial->println(pedal_cluth_value);
          }
          if (pedal_status == 0)
          {
            SetControllerOutputValueAccelerator(pedal_cluth_value);
            SetControllerOutputValueBrake(pedal_brake_value);
            SetControllerOutputValueThrottle(pedal_throttle_value);
            SetControllerOutputValueRudder(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueRudder_brake(JOYSTICK_MIN_VALUE, JOYSTICK_MIN_VALUE);
          }
          if (pedal_status == 1)
          {
            SetControllerOutputValueAccelerator(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueBrake(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueThrottle(JOYSTICK_MIN_VALUE);
            // 3% deadzone
            if (pedal_throttle_value < ((int16_t)(0.47f * JOYSTICK_RANGE + JOYSTICK_MIN_VALUE)) || pedal_throttle_value > ((int16_t)(0.53f * JOYSTICK_RANGE + JOYSTICK_MIN_VALUE)))
            {
              uint16_t rudderValue = pedal_throttle_value;
              SetControllerOutputValueRudder(rudderValue);
            }
            else
            {
              SetControllerOutputValueRudder((int16_t)(0.5f * JOYSTICK_RANGE+JOYSTICK_MIN_VALUE));
            }
            SetControllerOutputValueRudder_brake(JOYSTICK_MIN_VALUE, JOYSTICK_MIN_VALUE);
          }
          if (pedal_status == 2)
          {
            SetControllerOutputValueAccelerator(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueBrake(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueThrottle(JOYSTICK_MIN_VALUE);
            SetControllerOutputValueRudder((int16_t)(0.5f * JOYSTICK_RANGE+JOYSTICK_MIN_VALUE));
            // int16_t filter_brake=0;
            // int16_t filter_throttle=0;
            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability[0] == 1)
            {
              SetControllerOutputValueRudder_brake(pedal_cluth_value, pedal_throttle_value);
            }
            else
            {
              SetControllerOutputValueRudder_brake(pedal_brake_value, pedal_throttle_value);
            }
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

        dacWrite(Analog_brk, (uint16_t)((float)((Joystick_value[1]) / (float)(JOYSTICK_RANGE)) * 255));
        dacWrite(Analog_gas, (uint16_t)((float)((Joystick_value[2]) / (float)(JOYSTICK_RANGE)) * 255));
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
            ActiveSerial->print(Joystick_value[0]);
            ActiveSerial->print("/");
            ActiveSerial->print(Joystick_value[1]);
            ActiveSerial->print("/");
            ActiveSerial->print(Joystick_value[2]);
          }
          */

          mcp.setChannelValue(MCP4728_CHANNEL_A, (uint16_t)((float)Joystick_value[0] / (float)JOYSTICK_RANGE * 0.8f * 4096));
          mcp.setChannelValue(MCP4728_CHANNEL_B, (uint16_t)((float)Joystick_value[1] / (float)JOYSTICK_RANGE * 0.8f * 4096));
          mcp.setChannelValue(MCP4728_CHANNEL_C, (uint16_t)((float)Joystick_value[2] / (float)JOYSTICK_RANGE * 0.8f * 4096));
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
            esp_err_t result= esp_now_deinit();
            ESPNow_initial_status=false;
            ESPNOW_status=false;
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
              if(dap_action_ota_st.payloadOtaInfo_.ota_action==1)
              {
                Version_tag="0.0.0";
                ActiveSerial->println("Force update");
              }
              switch (dap_action_ota_st.payloadOtaInfo_.mode_select)
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
          uint8_t led_status=dap_bridge_state_st.payloadBridgeState_.Pedal_availability[0]+dap_bridge_state_st.payloadBridgeState_.Pedal_availability[1]*2+dap_bridge_state_st.payloadBridgeState_.Pedal_availability[2]*4;
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
            if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[i]==1)
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
          uint16_t throttleValue = pedal_throttle_value;
          uint16_t brakeValue = pedal_brake_value;
          uint16_t clutchValue = pedal_cluth_value;
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
  unsigned long unassignedPedalScan_Last=0;
  bool unassignedPedalScan_b= false;
  int unassignedScanInterval=100;
  int unassignedPedalCount_Last=0;
  for (;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
      if (millis() - unassignedPedalScan_Last > unassignedScanInterval)
      {
        unassignedPedalScan_b=true;
        unassignedPedalScan_Last=millis();
      }

      if(unassignedPedalScan_b && unassignedPeersList.size()>0)
      {
        checkAndRemoveTimeoutUnssignedPedal();
        unassignedPedalScan_b=false;
      }

      if (unassignedPeersList.size() != unassignedPedalCount_Last)
      {
        unassignedPedalCount_Last = unassignedPeersList.size();
        if(unassignedPeersList.size()>0)
        {
          ActiveSerial->printf("[L]Found %d Unconfigured Pedals", unassignedPedalCount_Last);
          ActiveSerial->println("");
          for (UnassignedPeer &item : unassignedPeersList) 
          {
            if(!item.peerAdded)
            {
              esp_now_peer_info_t peerInfo = {};
              memcpy(peerInfo.peer_addr, item.mac, 6);
              peerInfo.channel = 0; 
              peerInfo.ifidx = WIFI_IF_STA; 
              peerInfo.encrypt = false; 
              esp_err_t result = esp_now_add_peer(&peerInfo);

              if (result == ESP_OK) 
              {
                ActiveSerial->println("[L]SUCCESS: ESPNow peer added.");
              } 
              else if (result == ESP_ERR_ESPNOW_EXIST) 
              {
                ActiveSerial->println("[L]Peer already exists. No action taken.");
              }
              else 
              {
                ActiveSerial->print("[L]FAIL: esp_now_add_peer failed! Error Code: ");
                ActiveSerial->println(result);
              }
              item.peerAdded=true;
            }
          }

        }



      }
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
        if(tinyusbJoystick_.isGetData)
        {
          tinyusbJoystick_.isGetData= false;
          ActiveSerial->println("");
          ActiveSerial->print("[L]Raw Length:");
          ActiveSerial->println(tinyusbJoystick_.rawLength);
          ActiveSerial->print("[L]isGetData:");
          ActiveSerial->print(tinyusbJoystick_.isGetData);
          ActiveSerial->print(", buff size:");
          ActiveSerial->print(tinyusbJoystick_.buffSizeDis);
          ActiveSerial->print(", report type:");
          ActiveSerial->print(tinyusbJoystick_.reportType);
          ActiveSerial->print(", report ID:");
          ActiveSerial->println(tinyusbJoystick_.reportID);
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
            int pedalIdx = tinyusbJoystick_.tmpConfig[i].payLoadHeader_.PedalTag;
            memcpy(&dap_config_st[pedalIdx], &tinyusbJoystick_.tmpConfig[i], sizeof(DAP_config_st));
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
            int pedalIdx = tinyusbJoystick_.tmpAction[i].payLoadHeader_.PedalTag;
            if(pedalIdx == PEDAL_ID_CLUTCH || pedalIdx == PEDAL_ID_BRAKE || pedalIdx == PEDAL_ID_THROTTLE)
            {
            //forward to pedal
              memcpy(&dap_actions_st[pedalIdx], &tinyusbJoystick_.tmpAction[i], sizeof(DAP_actions_st));
              dap_action_update[pedalIdx] = true;
            }
            if (pedalIdx == PEDAL_ID_TEMP_1 || pedalIdx == PEDAL_ID_TEMP_2 || pedalIdx == PEDAL_ID_TEMP_3)
            {
              //make those assignement action to pedal with specific mac address
              int tempIdx = pedalIdx - PEDAL_ID_TEMP_1;
              memcpy(&dap_actionassignment_st[tempIdx], &tinyusbJoystick_.tmpAction[i], sizeof(DAP_actions_st));
              sendAssignment_b[tempIdx] = true;
              //dap_action_update[pedalIdx] = true;
            }
            tinyusbJoystick_.isActionGet[i]=false;
          }
        }
        if(tinyusbJoystick_.isBridgeActionGet)
        {
          //ActiveSerial->println("");
          //ActiveSerial->printf("[L]Get Bridge action\n");
          memcpy(&dap_bridge_state_lcl, &tinyusbJoystick_.tmpBridgeAction, sizeof(DAP_bridge_state_st));
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_ENABLE_PAIRING)
          {
            #ifdef ESPNow_Pairing_function
              ActiveSerial->println("[L]Bridge Pairing...");
              software_pairing_action_b = true;
            #endif
            #ifndef ESPNow_Pairing_function
              ActiveSerial->println("[L]Pairing command didn't supported");
            #endif
          }
          // action=2, restart
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_RESTART)
          {
            ActiveSerial->println("[L]Bridge Restart");
            delay(1000);
            ESP.restart();
          }
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_DOWNLOAD_MODE)
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
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_DEBUG)
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
              isBridgeInDebugMode_b = true;
            }
          }
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_JOYSTICK_FLASHING_MODE)
          {
            #ifdef External_RP2040
              ActiveSerial->println("[L]JOYSTICK restart into flashing mode");
              dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_RESET_INTO_BOOTLOADER;
            #else
              ActiveSerial->println("[L]The command is not supported");
            #endif
          }
          if (dap_bridge_state_lcl.payloadBridgeState_.Bridge_action == BRIDGE_ACTION_JOYSTICK_DEBUG)
          {
            #ifdef External_RP2040
              ActiveSerial->println("[L]JOYSTICK debug mode on");
              dap_joystickUART_state_lcl._payloadjoystick.JoystickAction = JOYSTICKACTION_DEBUG_MODE;
            #else
              ActiveSerial->println("[L]The command is not supported");
            #endif
          }
          tinyusbJoystick_.isBridgeActionGet=false;
        }
        if(tinyusbJoystick_.isOtaActionGet)
        {
          ActiveSerial->println("[L]get OTA command and its info");
          memcpy(&dap_action_ota_st, &tinyusbJoystick_.tmpOtaAction, sizeof(DAP_action_ota_st));
          #ifdef OTA_Update
          bool structChecker_b = true;
          if (dap_action_ota_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION_OTA)
          {
            structChecker_b = false;
            //structIsValid = false;
          }
          if (structChecker_b)
          {
            SSID = new char[dap_action_ota_st.payloadOtaInfo_.SSID_Length + 1];
            PASS = new char[dap_action_ota_st.payloadOtaInfo_.PASS_Length + 1];
            memcpy(SSID, dap_action_ota_st.payloadOtaInfo_.WIFI_SSID, dap_action_ota_st.payloadOtaInfo_.SSID_Length);
            memcpy(PASS, dap_action_ota_st.payloadOtaInfo_.WIFI_PASS, dap_action_ota_st.payloadOtaInfo_.PASS_Length);
            SSID[dap_action_ota_st.payloadOtaInfo_.SSID_Length] = 0;
            PASS[dap_action_ota_st.payloadOtaInfo_.PASS_Length] = 0;
          }
          if (dap_action_ota_st.payloadOtaInfo_.device_ID == DEVICE_ID && structChecker_b == true)
          {
            OTA_enable_b = true;
            ActiveSerial->println("[L] Bridge OTA begin.");
          }
          else if (structChecker_b)
          {
            pedal_OTA_action_b = true;
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
          if(update_basic_state[i])
          {
            update_basic_state[i]=false;
            tinyusbJoystick_.sendData((uint8_t*)&dap_state_basic_st[i], sizeof(DAP_state_basic_st));
            if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[dap_state_basic_st[i].payLoadHeader_.PedalTag]==0)
            {
              ActiveSerial->print("[L]Found Pedal:");
              ActiveSerial->println(dap_state_basic_st[i].payLoadHeader_.PedalTag);
            }
            dap_bridge_state_st.payloadBridgeState_.Pedal_availability[dap_state_basic_st[i].payLoadHeader_.PedalTag]=1;
            pedal_last_update[dap_state_basic_st[i].payLoadHeader_.PedalTag]=millis();
            if(ESPNow_error_b[i])
            {
              ActiveSerial->print("[L]Pedal:");
              ActiveSerial->print(dap_state_basic_st[i].payLoadHeader_.PedalTag);
              ActiveSerial->print(" E:");
              ActiveSerial->println(dap_state_basic_st[i].payloadPedalState_Basic_.error_code_u8);
              ESPNow_error_b[i]=false;    
            }
          }
          if(update_extend_state[i])
          {
            update_extend_state[i]=false;
            tinyusbJoystick_.sendData((uint8_t*)&dap_state_extended_st[i], sizeof(DAP_state_extended_st));
            

          }
        }
        

        int pedal_config_IDX=0;
        for(pedal_config_IDX=0;pedal_config_IDX<3;pedal_config_IDX++)
        {
          if(ESPNow_request_config_b[pedal_config_IDX])
          {
            DAP_config_st * dap_config_st_local_ptr;
            DAP_config_st dap_config_st_local;
            if(pedal_config_IDX==0)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Clu, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Clu;
            }
            if(pedal_config_IDX==1)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Brk, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Brk;
            }
            if(pedal_config_IDX==2)
            {
              memcpy(&dap_config_st_local, &dap_config_st_Gas, sizeof(DAP_config_st));
              //dap_config_st_local_ptr = &dap_config_st_Gas;
            }
            dap_config_st_local_ptr= &dap_config_st_local;
            
            //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));

            dap_config_st_local_ptr->payLoadHeader_.PedalTag=dap_config_st_local_ptr->payLoadPedalConfig_.pedal_type;
            crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
            dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
            tinyusbJoystick_.sendData((uint8_t*)&dap_config_st_local.payLoadHeader_, sizeof(DAP_config_st));
            ESPNow_request_config_b[pedal_config_IDX]=false;
            ActiveSerial->print("[L]Pedal:");
            ActiveSerial->print(pedal_config_IDX);
            ActiveSerial->println(" config returned");
            delay(3);
          }
        }

        
        if(basic_rssi_update)//Bridge action
        {
          //fill header and footer
          dap_bridge_state_st.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
          dap_bridge_state_st.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
          dap_bridge_state_st.payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
          dap_bridge_state_st.payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
          int rssi_filter_value=constrain(rssi_filter.process(rssi_display),-100,0) ;
          dap_bridge_state_st.payloadBridgeState_.unassignedPedalCount=(byte)unassignedPeersList.size();
          dap_bridge_state_st.payLoadHeader_.PedalTag=5; //5 means bridge
          dap_bridge_state_st.payLoadHeader_.payloadType=DAP_PAYLOAD_TYPE_BRIDGE_STATE;
          dap_bridge_state_st.payLoadHeader_.version=DAP_VERSION_CONFIG;
          dap_bridge_state_st.payloadBridgeState_.Bridge_action=0;
          memcpy(dap_bridge_state_st.payloadBridgeState_.Pedal_RSSI_Realtime,rssi,sizeof(int32_t)*3);
          //parse_version(BRIDGE_FIRMWARE_VERSION,&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0],&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1],&dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2]);
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0]=versionMajor;
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1]=versionMinor;
          dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2]=versionPatch;
          int indexMac = 0;
          for (UnassignedPeer &item : unassignedPeersList) 
          {
            memcpy(&dap_bridge_state_st.payloadBridgeState_.macAddressDetected[indexMac], item.mac,6);
            indexMac=indexMac+6;
          }
          //CRC check should be in the final
          crc = checksumCalculator((uint8_t*)(&(dap_bridge_state_st.payLoadHeader_)), sizeof(dap_bridge_state_st.payLoadHeader_) + sizeof(dap_bridge_state_st.payloadBridgeState_));
          dap_bridge_state_st.payloadFooter_.checkSum=crc;
          DAP_bridge_state_st * dap_bridge_st_local_ptr;
          dap_bridge_st_local_ptr = &dap_bridge_state_st;
          tinyusbJoystick_.sendData((uint8_t*)&dap_bridge_state_st, sizeof(DAP_bridge_state_st));
          basic_rssi_update=false;
        }
        Dap_hidmessage_st receivedMsg;
        if (xQueueReceive(messageQueueHandle, &receivedMsg, (TickType_t)0) == pdTRUE)
        {
          tinyusbJoystick_.sendData((uint8_t*)&receivedMsg, sizeof(Dap_hidmessage_st));
          delay(1);
        }
        //
        if(PedalUpdateIntervalPrint_b)
        {
          if(isBridgeInDebugMode_b)
          {
            for(int pedalIDX=0;pedalIDX<3;pedalIDX++)
            {
              if(dap_bridge_state_st.payloadBridgeState_.Pedal_availability[pedalIDX]==1)
              {
                tinyusbJoystick_.printf("Pedal %d, Update Interval: %d, RSSI: %d", pedalIDX, (int)(millis()-pedal_last_update[pedalIDX]),rssi[pedalIDX]);
                delay(10);
              }
            }
          }
          PedalUpdateIntervalPrint_b=false;
        }

      #endif
    }
  }
}

