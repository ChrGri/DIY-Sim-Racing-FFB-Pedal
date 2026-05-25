#include "UsbComManager.h"

#define BAUD_3M_U32 3000000U

#if defined(USE_CDC_INSTEAD_OF_UART)
    #if __has_include("esp32-hal-tinyusb.h")
        #include "esp32-hal-tinyusb.h"
    #endif
    #if __has_include("soc/rtc_cntl_reg.h")
        #include "soc/rtc_cntl_reg.h"
    #endif
    
    extern USBCDC customUsbSerial;
#endif

UsbComManager::UsbComManager(uint32_t intervalMs) {
    #if defined(USE_CDC_INSTEAD_OF_UART)
        targetStream = &customUsbSerial;
    #else
        targetStream = &Serial;
    #endif
}

void UsbComManager::begin(uint8_t pedalId) {
    #if defined(USE_CDC_INSTEAD_OF_UART)
        customUsbSerial.begin(115200);
        // WICHTIG: Großzügiger Timeout. Erlaubt dem Tx-Task zu blockieren, 
        // ohne Structs zu zerschneiden, falls der 16KB Puffer wirklich mal voll ist.
        customUsbSerial.setTxTimeoutMs(50); 
        customUsbSerial.enableReboot(false);
    #else
        Serial.begin(BAUD_3M_U32);
    #endif
    
    #ifdef USB_JOYSTICK
      SetupController_USB(pedalId); 
    #endif
}

size_t IRAM_ATTR_FLAG UsbComManager::write(uint8_t c) {
    return write(&c, 1);
}

size_t IRAM_ATTR_FLAG UsbComManager::write(const uint8_t *buffer, size_t size) {
    if (size == 0) return 0;

    #if defined(USE_CDC_INSTEAD_OF_UART)
        if (!customUsbSerial) return size;
        // Check entfernt! Wir vertrauen auf den 16KB Puffer und den Timeout.
        // Wenn voll, wartet der Task. Wenn Timeout abläuft, bricht er ab.
        return customUsbSerial.write(buffer, size);
    #else
        if (!Serial) return size;
        return Serial.write(buffer, size);
    #endif
}

int IRAM_ATTR_FLAG UsbComManager::availableForWrite() { 
    return targetStream->availableForWrite();
}

int IRAM_ATTR_FLAG UsbComManager::available() { return targetStream->available(); }
int IRAM_ATTR_FLAG UsbComManager::read() { return targetStream->read(); }
int IRAM_ATTR_FLAG UsbComManager::peek() { return targetStream->peek(); }

void IRAM_ATTR_FLAG UsbComManager::flush() { 
    targetStream->flush(); 
}

void IRAM_ATTR_FLAG UsbComManager::processTxBatch() {
    #if defined(USE_CDC_INSTEAD_OF_UART)
    if (!customUsbSerial) return;

    static bool rebootEnabled = false;
    if (!rebootEnabled && millis() > 5000) {
        rebootEnabled = true;
    }
    
    if (rebootEnabled && customUsbSerial.baudRate() == 1200) {
        #if __has_include("esp32-hal-tinyusb.h")
            usb_persist_restart(RESTART_BOOTLOADER);
        #else
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
            ESP.restart();
        #endif
    }
    #endif
}

#ifdef USB_JOYSTICK
bool IRAM_ATTR_FLAG UsbComManager::isJoystickReady() { return IsControllerReady(); }
void IRAM_ATTR_FLAG UsbComManager::sendJoystickValue(uint16_t val) { SetControllerOutputValue(val); }
#endif