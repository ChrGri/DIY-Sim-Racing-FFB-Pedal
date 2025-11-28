#pragma once
//#include <ADS1256.h>

/********************************************************************/
/*                      Select the PCB      */
/********************************************************************/
#ifndef PCB_VERSION
  //#define PCB_VERSION 1 // V1 for regular ESP32
  //#define PCB_VERSION 2 // V1 for ESP32 S2 mini
  #define PCB_VERSION 3 // V3 for regular ESP32
  //#define PCB_VERSION 4 // speedcrafter PCB V1.3
  //#define PCB_VERSION 5 // speedcrafter PCB V1.4
  //#define PCB_VERSION 6 // V1 for ESP32 S3
#endif



/********************************************************************/
/*                      Other defines       */
/********************************************************************/

// target cycle time for pedal update task, to get constant cycle times, required for FIR filtering
#define DAP_MICROSECONDS_PER_SECOND 1000000

// 15kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_15000SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 15000

// 7.5kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_7500SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 7500

// 3.75kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_3750SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 3750

// 2.0kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_2000SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 2000

// 1.0kHz
#define ADC_SAMPLE_RATE ADS1256_DRATE_1000SPS
#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 1000



/********************************************************************/
/*                      Loadcell defines                            */
/********************************************************************/
#define LOADCELL_WEIGHT_RATING_KG 300.0
#define LOADCELL_EXCITATION_V 5.0
#define LOADCELL_SENSITIVITY_MV_V 2.0


/********************************************************************/
/*                      Motor defines                            */
/********************************************************************/
//#define MOTOR_INVERT_MOTOR_DIR false




/********************************************************************/
/*                      PIN defines                                 */
/********************************************************************/

// V3 version of dev PCB for regular ESP32
#if PCB_VERSION == 3
  // ADC defines
  #define PIN_DRDY 19// 19 --> DRDY
  #define PIN_RST  15 // X --> X
  #define PIN_SCK 16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    22
  #define stepPinStepper   23
  //analog output pin
  #define Analog_brk 25 
  #define Analog_gas 26
  //I2Cpins
  #define I2C_SDA 32
  #define I2C_SCL 33
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  //I2C pins
  #define MCP_SDA 21
  #define MCP_SCL 22

  #define Using_analog_output
  //#define Using_I2C_Sync
  #define ESPNOW_Enable
  #define Using_MCP4728
  #define Using_Board_ESP32
  //#define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 98
  #define Pairing_GPIO 0
#endif

// V3 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
// 2. USB CDC On Boot Enabled
#if PCB_VERSION == 5
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 4
  #define MCP_SCL 5
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16
  #define ESPNOW_Enable
  #define Using_Board_ESP32S3
  //#define Using_MCP4728
  //#define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define USING_LED
  #define LED_ENABLE_WAVESHARE
  #define LED_GPIO 38
  #define Pairing_GPIO 13
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 99
  #define Fanatec_comunication
  #define Fanatec_serial_RX 18
  #define Fanatec_serial_TX 17
  #define Fanatec_plug 16
  #define OTA_Update
#endif



// V3 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
// 2. USB CDC On Boot Enabled
#if PCB_VERSION == 6
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 4
  #define MCP_SCL 5
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16
  #define ESPNOW_Enable
  #define Using_Board_ESP32S3
  //#define Using_MCP4728
  //#define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK
  #define USING_LED
  #define LED_ENABLE_WAVESHARE
  #define LED_GPIO 38
  #define Pairing_GPIO 12
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 99
  #define OTA_Update
#endif

#if PCB_VERSION == 7
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 4
  #define MCP_SCL 5
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16
  #define ESPNOW_Enable
  #define Using_Board_ESP32S3
  //#define Using_MCP4728
  //#define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK
  #define USING_LED
  #define LED_ENABLE_DONGLE
  #define LED_GPIO 39
  #define Pairing_GPIO 0
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 99
  #define OTA_Update
#endif

#if PCB_VERSION == 8
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 4
  #define MCP_SCL 5
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16
  #define ESPNOW_Enable
  #define Using_Board_ESP32S3
  //#define Using_MCP4728
  //#define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define USING_LED
  #define LED_ENABLE_WAVESHARE
  #define LED_GPIO 38
  #define Pairing_GPIO 12
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 99
  #define OTA_Update
  #define External_RP2040
#endif

#if PCB_VERSION == 9

  #define ESPNOW_Enable
  #define Using_Board_ESP32S3
  //#define Using_MCP4728
  //#define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define USING_LED
  #define LED_ENABLE_WAVESHARE
  #define LED_GPIO 38
  #define Pairing_GPIO 12
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define DEVICE_ID 99
  #define OTA_Update
#endif

//task defination
#define CORE_ID_SERIAL_TX_TASK (uint8_t)1
#define CORE_ID_SERIAL_RX_TASK (uint8_t)1
#define CORE_ID_ESPNOW_TX_TASK (uint8_t)0
#define CORE_ID_JOYSTICK_UPDATE_TASK (uint8_t)0
#define CORE_ID_OTA_UPDATE_TASK (uint8_t)1
#define CORE_ID_FANATEC_UPDATE_TASK (uint8_t)1
#define CORE_ID_LED_UPDATE_TASK (uint8_t)1
#define CORE_ID_MISC_TASK (uint8_t)1

#define REPETITION_INTERVAL_SERIAL_TX_TASK (int64_t)2000
#define REPETITION_INTERVAL_SERIAL_RX_TASK (int64_t)2000
#define REPETITION_INTERVAL_ESPNOW_TX_TASK (int64_t)2000
#define REPETITION_INTERVAL_JOYSTICK_UPDATE_TASK (int64_t)8000
#define REPETITION_INTERVAL_OTA_UPDATE_TASK (int64_t)50000
#define REPETITION_INTERVAL_FANATEC_UPDATE_TASK (int64_t)10000
#define REPETITION_INTERVAL_LED_UPDATE_TASK (int64_t)10000
#define REPETITION_INTERVAL_MISC_TASK (int64_t)50000

#define TASK_PRIORITY_SERIAL_TX_TASK (UBaseType_t) 1
#define TASK_PRIORITY_SERIAL_RX_TASK (UBaseType_t) 1
#define TASK_PRIORITY_ESPNOW_TX_TASK (UBaseType_t) 1
#define TASK_PRIORITY_JOYSTICK_UPDATE_TASK (UBaseType_t) 1
#define TASK_PRIORITY_OTA_UPDATE_TASK (UBaseType_t) 1
#define TASK_PRIORITY_FANATEC_UPDATE_TASK (UBaseType_t) 1
#define TASK_PRIORITY_LED_UPDATE_TASK (UBaseType_t) 1
#define TASK_PRIORITY_MISC_TASK (UBaseType_t)1

#define STACK_SIZE_SERIAL_TX_TASK (int64_t) 5000
#define STACK_SIZE_SERIAL_RX_TASK (int64_t) 5000  
#define STACK_SIZE_ESPNOW_TX_TASK (int64_t) 10000
#define STACK_SIZE_JOYSTICK_UPDATE_TASK (int64_t) 10000
#define STACK_SIZE_OTA_UPDATE_TASK (int64_t) 16000
#define STACK_SIZE_FANATEC_UPDATE_TASK (int64_t) 3000
#define STACK_SIZE_LED_UPDATE_TASK (int64_t) 3000
#define STACK_SIZE_MISC_TASK (int64_t)5000


extern Stream *ActiveSerial;