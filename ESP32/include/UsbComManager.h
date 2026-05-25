#pragma once

#include <Arduino.h>
#include "USB.h"
#include "Controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Main.h"
  // Wenn der Core kein USBCDC bereitstellt, müssen wir es manuell inkludieren und verwalten
#if defined(USE_CDC_INSTEAD_OF_UART)
  #include "USBCDC.h"
#endif

#ifdef USB_JOYSTICK
  #include "Controller.h"
#endif

class UsbComManager : public Stream {
public:
    UsbComManager(uint32_t intervalMs = 5);

    void begin(uint8_t pedalId);

    // Stream Overrides
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    
    int availableForWrite() override;
    int available() override;
    int read() override;
    int peek() override;
    void flush() override;

    void processTxBatch();

#ifdef USB_JOYSTICK
    bool isJoystickReady();
    void sendJoystickValue(uint16_t val);
#endif

private:
    Stream* targetStream;
    #if defined(USE_CDC_INSTEAD_OF_UART)
      USBCDC customUsbSerial;
    #endif
  
};