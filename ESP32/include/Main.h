#pragma once
#include <stdint.h>
#include <arduino.h>
// #include <task.h>

#define PI_FL32         3.1415926535897932384626433832795f
#define DEG_TO_RAD_FL32 0.017453292519943295769236907684886f
#define RAD_TO_DEG_FL32 57.295779513082320876798154814105f

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
#define DAP_MICROSECONDS_PER_SECOND_U32 1000000U

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
#define ADC_SAMPLE_RATE_U32 ADS1256_DRATE_2000SPS
#define PUT_TARGET_CYCLE_TIME_IN_US_U32 (DAP_MICROSECONDS_PER_SECOND_U32 / 2000U)

// #define PUT_TARGET_CYCLE_TIME_IN_US 300

// 1.0kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_1000SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 1000


#define SERVO_MAX_VOLTAGE_IN_V_36V 38.0f
#define SERVO_MAX_VOLTAGE_IN_V_48V 50.0f

/********************************************************************/
/*                      Loadcell defines                            */
/********************************************************************/
#define LOADCELL_WEIGHT_RATING_KG 300.0f
#define LOADCELL_EXCITATION_V 5.0f
#define LOADCELL_SENSITIVITY_MV_V 2.0f


/********************************************************************/
/*                      Motor defines                            */
/********************************************************************/
//#define MOTOR_INVERT_MOTOR_DIR false
static const uint32_t s_maximumStepperRpm_u32 = 4000;
static const uint32_t s_secondsPerMinute_u32 = 60;
#define MAXIMUM_STEPPER_SPEED_U32 (uint32_t)MAX_SPEED_IN_HZ//100000//  max steps per second, see https://github.com/gin66/FastAccelStepper


/********************************************************************/
/*                      PIN defines                                 */
/********************************************************************/
// initial version of dev PCB for regular ESP32
#if PCB_VERSION == 1
  // ADC defines
  #define PIN_DRDY_U8 17// 17 --> DRDY
  #define PIN_RST_U8  16 // X --> X
  #define PIN_SCK_U8 18 // 18 -->SCLK
  #define PIN_MISO_U8 19 // 19 --> DOUT
  #define PIN_MOSI_U8 23 // 23 --> DIN
  #define PIN_CS_U8 5 // 5 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    0
  #define STEP_PIN_STEPPER_U8   4


  // level shifter not present on this PCB design
  #define SENSORLESS_HOMING false

  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
  #define RUN_IN_CACHE
#endif


// initial version of dev PCB for ESP32 S2 mini
#if PCB_VERSION == 2
  // ADC defines
  #define PIN_DRDY_U8 37// 37 --> DRDY
  #define PIN_RST_U8  16 // X --> X
  #define PIN_SCK_U8 18 // 18 -->SCLK
  #define PIN_MISO_U8 35 // 35 --> DOUT
  #define PIN_MOSI_U8 33 // 33 --> DIN
  #define PIN_CS_U8 39 // 39 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    8
  #define STEP_PIN_STEPPER_U8   9


  // level shifter not present on this PCB design
  #define SENSORLESS_HOMING false

  #define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
  #define RUN_IN_CACHE
#endif



// V3 version of dev PCB for regular ESP32
#if PCB_VERSION == 3
  // ADC defines
  #define PIN_DRDY_U8 19// 19 --> DRDY
  #define PIN_RST_U8  15 // X --> X
  #define PIN_SCK_U8 16 // 16 -->SCLK
  #define PIN_MISO_U8 18 // 18 --> DOUT
  #define PIN_MOSI_U8 17 // 17 --> DIN
  #define PIN_CS_U8 21 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    22
  #define STEP_PIN_STEPPER_U8   23
  //analog output pin
  #define DAC_OUTPUT_PIN_U8 25 
  //I2Cpins
  #define I2C_SDA 32
  #define I2C_SCL 33

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 27 //17
  #define ISV57_RXPIN 26 // 16
  //pedal assignment
  // #define PEDAL_ASSIGNMENT
  // #define CFG1 15
  // #define CFG2 12
  //#define Using_analog_output
  //#define Using_I2C_Sync
  #define ESPNOW_Enable
  #define ESPNow_ESP32
  //#define I2C_slave_address 0x15
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
  //#define ESPNow_Pairing_function
  //#define Pairing_GPIO 13
  //#define ESPNow_debug_rudder
  #define OTA_update
  //#define BRAKE_RESISTOR_PIN 13
  //#define OTA_update_ESP32
  #define RUN_IN_CACHE
#endif



// speedcrafter PCB V1.3
#if PCB_VERSION == 4
  // ADC defines
  #define PIN_DRDY_U8 27// 19 --> DRDY
  #define PIN_RST_U8  5 // X --> X
  #define PIN_SCK_U8 14 // 16 -->SCLK
  #define PIN_MISO_U8 12 // 18 --> DOUT
  #define PIN_MOSI_U8 13 // 17 --> DIN
  #define PIN_CS_U8 15 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    32
  #define STEP_PIN_STEPPER_U8   33

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 27 //17
  #define ISV57_RXPIN 26 // 16

  //#define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 3
  //#define Pairing_GPIO 0
  //#define ESPNow_Pairing_function
  #define ESPNow_ESP32
  #define RUN_IN_CACHE
#endif




// Speedcrafters PCB V1.4
#if PCB_VERSION == 5
  // ADC defines
  #define PIN_DRDY_U8 19// 19 --> DRDY
  #define PIN_RST_U8  34 // X --> X
  #define PIN_SCK_U8 15 // 16 -->SCLK
  #define PIN_MISO_U8 18 // 18 --> DOUT
  #define PIN_MOSI_U8 13 // 17 --> DIN
  #define PIN_CS_U8 21 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    22
  #define STEP_PIN_STEPPER_U8   23

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 26 //17
  #define ISV57_RXPIN 27 // 16
  #define ESPNOW_Enable
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 3
  #define PAIRING_GPIO_U8 0
  //#define ESPNow_Pairing_function
  #define ESPNow_ESP32
  #define OTA_update
  #define RUN_IN_CACHE
#endif




// V4 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
// 2. USB CDC On Boot Enabled
#if PCB_VERSION == 6
  // ADC defines
  #define PIN_DRDY_U8 15//19// 19 --> DRDY
  #define PIN_RST_U8  6 // X --> X
  #define PIN_SCK_U8 16//16 // 16 -->SCLK
  #define PIN_MISO_U8 18 // 18 --> DOUT
  #define PIN_MOSI_U8 17 // 17 --> DIN
  #define PIN_CS_U8 7//21 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    37//22
  #define STEP_PIN_STEPPER_U8   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA_U8 48
  #define MCP_SCL_U8 47

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  //#define Using_analog_output_ESP32_S3

  #define USB_JOYSTICK
  #define ESPNOW_Enable
  #define ESPNow_S3
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 0
  //#define ESPNow_Pairing_function
  //#define Pairing_GPIO 0
  #define OTA_update
  #define CONTROLLER_SPECIFIC_VIDPID
  #define BAUDRATE3M
  #define PEDAL_SOFTWARE_ASSIGNMENT
#endif

// For Gilphilbert PCBA design
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. USB CDC On Boot Enabled
#if PCB_VERSION == 7
  // ADC defines
  #define PIN_DRDY_U8 15//19// 19 --> DRDY
  #define PIN_RST_U8  6 // X --> X
  #define PIN_SCK_U8 16//16 // 16 -->SCLK
  #define PIN_MISO_U8 18 // 18 --> DOUT
  #define PIN_MOSI_U8 17 // 17 --> DIN
  #define PIN_CS_U8 7//21 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    37//22
  #define STEP_PIN_STEPPER_U8   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA_U8 5
  #define MCP_SCL_U8 4

  // Pedal assignment pin
  //#define PEDAL_HARDWARE_ASSIGNMENT
  #define PEDAL_SOFTWARE_ASSIGNMENT
  #define CFG1_U8 1
  #define CFG2_U8 2

  #define EMERGENCY_BUTTON
  #define BUZZER_PIN_U8 6
  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  #define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  //#define Hardware_Pairing_button
  #define PAIRING_GPIO_U8 0
  //#define ESPNow_debug_rudder
  #define USING_LED
  #define LED_GPIO_U8 12
  #define OTA_update
  #define USING_BUZZER
  #define USE_CDC_INSTEAD_OF_UART
#endif

// PCBA V2
#if PCB_VERSION == 9
  // ADC defines
  #define PIN_DRDY_U8 15//--> DRDY
  #define PIN_RST_U8  6 //--> X
  #define PIN_SCK_U8 16//-->SCLK
  #define PIN_MISO_U8 18 //--> DOUT
  #define PIN_MOSI_U8 17 //--> DIN
  #define PIN_CS_U8 7//--> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    37//22
  #define STEP_PIN_STEPPER_U8   38//23

  #define MCP_SDA_U8 5
  #define MCP_SCL_U8 4

  // Pedal assignment pin
  //#define PEDAL_HARDWARE_ASSIGNMENT
  #define PEDAL_SOFTWARE_ASSIGNMENT
  #define CFG1_U8 1
  #define CFG2_U8 2

  // #define EMERGENCY_BUTTON
  // #define ShutdownPin 6
  #define BUZZER_PIN_U8 21
  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10 // 27 //17
  #define ISV57_RXPIN 9  // 26 // 16

  #define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  // #define ESPNow_Pairing_function
  // #define Hardware_Pairing_button
  #define PAIRING_GPIO_U8 33
  // #define ESPNow_debug_rudder
  #define CONTROLLER_SPECIFIC_VIDPID
  #define USING_LED
  #define LED_GPIO_U8 12
  #define OTA_update
  #define USING_BUZZER
  #define BRAKE_RESISTOR_PIN_U8 4
  #define SERVO_POWER_PIN 3
  #define EMERGENCY_PIN_U8 6
  #define BAUDRATE3M
#endif
// Switch-!t PCB for Waveshare ESP32-S3-DEV-KIT-N8R8
// More information at https://github.com/gaggi/ActivePedalPCB
#if PCB_VERSION == 11
  // ADC defines
  #define PIN_DRDY_U8 15//19// 19 --> DRDY
  #define PIN_RST_U8  6 // X --> X
  #define PIN_SCK_U8 16//16 // 16 -->SCLK
  #define PIN_MISO_U8 18 // 18 --> DOUT
  #define PIN_MOSI_U8 17 // 17 --> DIN
  #define PIN_CS_U8 7//21 // 21 --> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    37//22
  #define STEP_PIN_STEPPER_U8   36//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA_U8 5
  #define MCP_SCL_U8 4

  // endstop pins
  #define MIN_PIN_U8 12
  #define MAX_PIN_U8 13
  // Pedal assignment pin
  #define PEDAL_HARDWARE_ASSIGNMENT
  #define CFG1_U8 2
  #define CFG2_U8 1

  #define EMERGENCY_BUTTON
  #define BUZZER_PIN_U8 5
  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  //#define Using_analog_output_ESP32_S3
  #define ESPNOW_Enable
  #define ESPNow_S3
  #define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  //#define ESPNow_Pairing_function
  #define Hardware_Pairing_button
  #define PAIRING_GPIO_U8 33
  //#define ESPNow_debug_rudder
  #define CONTROLLER_SPECIFIC_VIDPID
  #define USING_LED
  #define LED_GPIO_U8 38
  #define LED_ENABLE_RGB
  #define OTA_update
  #define USING_BUZZER
  #define PEDAL_SOFTWARE_ASSIGNMENT
#endif




// V5 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
// 2. USB CDC On Boot Enabled
#if PCB_VERSION == 12
  // ADC defines
  #define PIN_DRDY_U8 16 //--> DRDY
  #define PIN_RST_U8  18 // X --> X
  #define PIN_SCK_U8 6 //-->SCLK
  #define PIN_MISO_U8 15 //--> DOUT
  #define PIN_MOSI_U8 7 //--> DIN
  #define PIN_CS_U8 17 //--> CS

  // stepper pins
  #define DIR_PIN_STEPPER_U8    36
  #define STEP_PIN_STEPPER_U8   37

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 2
  #define ISV57_RXPIN 1

  #define BRAKE_RESISTOR_PIN_U8 35

  #define USB_JOYSTICK
  //#define ESPNOW_Enable
  //#define ESPNow_S3
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 0
  //#define ESPNow_Pairing_function
  #define PAIRING_GPIO_U8 0
  #define OTA_update
  #define CONTROLLER_SPECIFIC_VIDPID
  #define BAUDRATE3M
  #define PEDAL_SOFTWARE_ASSIGNMENT
  // #define ANGLE_SENSOR_GPIO 11 // disabled by default, since to much runtime impact of ADC
#endif



// V6 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
#if PCB_VERSION == 13
  // ADC defines
  #define USES_ADS1220
  #define FFB_ADS1220_SCLK    6
  #define FFB_ADS1220_DIN     7     // MOSI
  #define FFB_ADS1220_DOUT    15    // MISO
  #define FFB_ADS1220_DRDY    16
  #define FFB_ADS1220_CS      17

  // stepper pins
  #define DIR_PIN_STEPPER_U8    36
  #define STEP_PIN_STEPPER_U8   37

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true

  #define ISV57_TXPIN 2
  #define ISV57_RXPIN 1

  #define BRAKE_RESISTOR_PIN_U8 35
  
  #ifndef DEBUG_KEEP_USB_SERIAL_JTAG
    #define USB_JOYSTICK
  #endif
  //#define ESPNOW_Enable
  //#define ESPNow_S3
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 0
  //#define ESPNow_Pairing_function
  #define PAIRING_GPIO_U8 0
  #define OTA_update
  #ifndef DEBUG_KEEP_USB_SERIAL_JTAG
    #define CONTROLLER_SPECIFIC_VIDPID
  #endif
  #define BAUDRATE3M
  #define PEDAL_SOFTWARE_ASSIGNMENT
  // #define ANGLE_SENSOR_GPIO 11 // disabled by default, since to much runtime impact of ADC
#endif


#ifdef ENABLE_ESP_NOW
  #define ESPNOW_Enable
  #define ESPNow_S3
#endif

//IRAM switch flag
#ifdef RUN_IN_CACHE
  #define IRAM_ATTR_FLAG
#else
  #define IRAM_ATTR_FLAG IRAM_ATTR
#endif





/********************************************************************/
/*                      Task defines                                */
/********************************************************************/
#define CORE_ID_PEDAL_UPDATE_TASK_U8 (uint8_t)1
#define CORE_ID_SERIAL_COMMUNICATION_TASK_U8 (uint8_t)0
#define CORE_ID_JOYSTICK_TASK_U8 (uint8_t)1
#define CORE_ID_MISC_TASK_U8 (uint8_t)0
#define CORE_ID_OTA_TASK_U8 (uint8_t)0
#define CORE_ID_SERVO_COMMUNICATION_TASK_U8 (uint8_t)0
#define CORE_ID_ESPNOW_TASK_U8 (uint8_t)0
#define CORE_ID_STEPPER_TASK_U8 (uint8_t)1
#define CORE_ID_LOADCELLREADING_TASK_U8 (uint8_t)1
#define CORE_ID_PROFILER_TASK_U8 (uint8_t)0
#define CORE_ID_CONFIG_HANDLING_TASK_U8 (uint8_t)0

// #define CORE_ID_PEDAL_UPDATE_TASK ( 0x7FFFFFFF )
// #define CORE_ID_SERIAL_COMMUNICATION_TASK ( 0x7FFFFFFF )
// #define CORE_ID_JOYSTICK_TASK ( 0x7FFFFFFF )
// #define CORE_ID_MISC_TASK ( 0x7FFFFFFF )
// #define CORE_ID_OTA_TASK ( 0x7FFFFFFF )
// #define CORE_ID_SERVO_COMMUNICATION_TASK ( 0x7FFFFFFF )
// #define CORE_ID_ESPNOW_TASK ( 0x7FFFFFFF )
// #define CORE_ID_STEPPER_TASK ( 0x7FFFFFFF )
// #define CORE_ID_LOADCELLREADING_TASK ( 0x7FFFFFFF )

#ifdef RUN_IN_CACHE
  #define REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 (int64_t)600
#else
  #define REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 (int64_t)300
#endif

#define REPETITION_INTERVAL_JOYSTICKOUTPUT_TASK_IN_US_I64 (int64_t)10000
#define REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US_I64 (int64_t)10000
#define REPETITION_INTERVAL_ESPNOW_TASK_IN_US_I64 (int64_t)3000
#define REPETITION_INTERVAL_OTA_TASK_IN_US_I64 (int64_t)10000
#define REPETITION_INTERVAL_SERVO_COMMUNICATION_TASK_IN_US_I64 (int64_t)10000


#ifdef RUN_IN_CACHE
  // equal task priority --> round robin 
  #define TASK_PRIORITY_PEDAL_UPDATE_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_JOYSTICKOUTPUT_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_LOADCELL_READING_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_SERIALCOMMUNICATION_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_SERIALCOMMUNICATION_TX_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_ESPNOW_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_OTA_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_SERVO_COMMUNICATION_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_CONFIG_HANDLING_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_PROFILER_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_MISC_TASK_UBASETYPE (UBaseType_t)1
#else
  #define TASK_PRIORITY_PEDAL_UPDATE_TASK_UBASETYPE (UBaseType_t)3
  #define TASK_PRIORITY_JOYSTICKOUTPUT_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_LOADCELL_READING_TASK_UBASETYPE (UBaseType_t)2
  #define TASK_PRIORITY_SERIALCOMMUNICATION_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_SERIALCOMMUNICATION_TX_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_ESPNOW_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_OTA_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_SERVO_COMMUNICATION_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_CONFIG_HANDLING_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_PROFILER_TASK_UBASETYPE (UBaseType_t)1
  #define TASK_PRIORITY_MISC_TASK_UBASETYPE (UBaseType_t)1
#endif


// alias to serial stream, thus it can dynamically switch depending on board
extern Stream *ActiveSerial;

#define BAUDRATE_U32 3000000U