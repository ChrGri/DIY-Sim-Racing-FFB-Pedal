// FanatecInterface.h

#ifndef FANATEC_INTERFACE_H
#define FANATEC_INTERFACE_H

#include <Arduino.h>
#include <HardwareSerial.h>

class FanatecInterface {
public:
    // Constructor
    FanatecInterface(int rxPin, int txPin, int plugPin);

    // Initialization function
    void begin();

    // Communication update function (to be called periodically in the loop)
    void update();
    void communicationUpdate();

    // Functions to set pedal values
    void setThrottle(uint16_t value);
    void setBrake(uint16_t value);
    void setClutch(uint16_t value);
    void setHandbrake(uint16_t value);

    // Function to set the connection callback
    void onConnected(void (*callback)(bool));

    // Check if connected to the Fanatec device
    bool isConnected();
    bool isPlugged();
    bool vibrationEnabled() const;
    bool setVibrationEnabled(bool enabled);
    uint8_t throttleVibration() const;
    uint8_t brakeVibration() const;

private:
    // Internal helper functions
    void performCommunicationSteps();
    void resetConnection();
    void changeBaudRate(unsigned long baudrate);
    void makeCRCTable(uint8_t poly);
    uint8_t generateCRC(uint8_t* input, size_t length);
    void createPacket(uint8_t* packet);

    // UART and plug pin settings
    int _rxPin;
    int _txPin;
    int _plugPin;
    unsigned long _lastBaudrate;

    // HardwareSerial object
    HardwareSerial* _serial;

    // CRC table
    uint8_t _crcTable[256];

    // Pedal values
    uint16_t _throttle;
    uint16_t _brake;
    uint16_t _clutch;
    uint16_t _handbrake;

    // Connection status
    bool _connected;
    volatile bool _vibrationEnabled; // setting is shared between tasks
    volatile uint8_t _throttleVibration;
    volatile uint8_t _brakeVibration;
    volatile unsigned long _lastThrottleVibrationAt;
    volatile unsigned long _lastBrakeVibrationAt;
    uint8_t _rxFrame[12];
    size_t _rxFrameIndex;
    unsigned long _lastRxByteAt;

    // Connection callback function pointer
    void (*_connectedCallback)(bool);

    // Hardware initialization and debounced cable state.
    bool _initialized;
    bool _plugState;
    bool _lastRawPlugState;
    unsigned long _plugChangedAt;
    size_t _handshakeStep;
    size_t _handshakeMatched;
    unsigned long _stepStartedAt;
};

#endif // FANATEC_INTERFACE_H
