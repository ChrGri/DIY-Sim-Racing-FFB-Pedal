#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "Main.h"
#include "DiyActivePedal_types.h"

static char ota_hostname[32] = "pedal_ota";

inline const char* get_ota_hostname_pio()
{
    snprintf(ota_hostname, sizeof(ota_hostname), "pedal_ota");
    return ota_hostname;
}

inline const char* get_ota_hostname(uint8_t pedalRole = PEDAL_ID_UNKNOWN)
{
    switch (pedalRole)
    {
        case PEDAL_ID_CLUTCH:
            snprintf(ota_hostname, sizeof(ota_hostname), "pedal_clutch");
            break;
        case PEDAL_ID_BRAKE:
            snprintf(ota_hostname, sizeof(ota_hostname), "pedal_brake");
            break;
        case PEDAL_ID_THROTTLE:
            snprintf(ota_hostname, sizeof(ota_hostname), "pedal_throttle");
            break;
        default:
            snprintf(ota_hostname, sizeof(ota_hostname), "pedal");
            break;
    }
    return ota_hostname;
}

inline void ota_arduinoota_initialize_with_hostname(const char* hostname)
{
    // Zwingt den WLAN-Chip, dauerhaft wach zu bleiben, um Paketverluste beim OTA zu verhindern!
    WiFi.setSleep(false);
    ArduinoOTA.setHostname(hostname); 
    ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        ActiveSerial->println("Start updating " + type);
        })
        .onEnd([]() {
        ActiveSerial->println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
        ActiveSerial->printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
        ActiveSerial->printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) ActiveSerial->println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) ActiveSerial->println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) ActiveSerial->println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) ActiveSerial->println("Receive Failed");
        else if (error == OTA_END_ERROR) ActiveSerial->println("End Failed");
        });

    ArduinoOTA.begin();

    ActiveSerial->println("OTA Initialized.");
    ActiveSerial->printf("Access via http://%s.local\n", hostname);
    ActiveSerial->print("IP address: ");
    ActiveSerial->println(WiFi.localIP());
}

inline void ota_arduinoota_initialize(uint8_t pedalRole = PEDAL_ID_UNKNOWN)
{
    get_ota_hostname(pedalRole);
    ota_arduinoota_initialize_with_hostname(ota_hostname);
}

inline void ota_arduinoota_initialize_pio()
{
    get_ota_hostname_pio();
    ota_arduinoota_initialize_with_hostname(ota_hostname);
}
