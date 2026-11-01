#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "Main.h"
const char* ota_hostname="pedal_ota";
void ota_arduinoota_initialize()
{
    // Zwingt den WLAN-Chip, dauerhaft wach zu bleiben, um Paketverluste beim OTA zu verhindern!
    WiFi.setSleep(false);
    ArduinoOTA.setHostname(ota_hostname); 
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
    ActiveSerial->printf("Access via http://%s.local\n", ota_hostname);
    ActiveSerial->print("IP address: ");
    ActiveSerial->println(WiFi.localIP());
}