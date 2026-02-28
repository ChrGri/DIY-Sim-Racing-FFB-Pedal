#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#define DAP_PAYLOAD_TYPE_JOYSTICKUART 215
#define DAP_JOY_VERSION 0x01
#define DAP_JOY_KEY 0x97
const int RP2040baudrate=921600;
const int handshakeGPIO=17;
const int RP2040txPin=15;
const int RP2040rxPin=16;
struct __attribute__((packed)) payloadjoystick
{
    uint8_t payloadtype;
    uint8_t key;
    uint8_t DAP_JOY_Version;
    int16_t controllerValue_i32[3];
    int8_t pedal_status;
    uint8_t pedalAvailability[3];
    uint8_t JoystickAction;
};
struct __attribute__((packed)) JoystickPayloadFooter
{
    // To check if structure is valid
    uint16_t checkSum_u16;
};
struct __attribute__((packed)) DAP_JoystickUART_State
{
    payloadjoystick _payloadjoystick;
    JoystickPayloadFooter _payloadfooter;
    
};

enum JoystickAction
{
    NONE,
    JOYSTICKACTION_DEBUG_MODE,
    JOYSTICKACTION_RESET_INTO_BOOTLOADER,
    JOYSTICK_RESTART
};

class RP2040PicoUART
{
    public:
        RP2040PicoUART(int rxPin, int txPin, int handshakePin, int baud);
        void UARTrestart();
        void UARTSendPacket(uint8_t* data, size_t len);
        HardwareSerial* SerialExt;
        int _txPin;
        int _rxPin;
        int _handshakePin;
        int _baud;


};