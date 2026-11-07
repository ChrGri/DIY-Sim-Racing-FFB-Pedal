#include <Arduino.h>
#include <ESP32OTAPull.h>
#include <esp_wifi.h>
#include "Main.h"

#define JSON_URL_dev   "https://raw.githubusercontent.com/gilphilbert/pedal-flasher/main/json/dev/Version_Bridge.json"
#define JSON_URL_main "https://raw.githubusercontent.com/gilphilbert/pedal-flasher/main/json/main/Version_Bridge.json"
#define JSON_URL_daily "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/TestBuild/json/Version_Bridge.json"


bool OTA_enable_b =false;
bool OTA_status =false;
/*
struct DAP_otaWifiInfo_st
{ 
    uint8_t payloadType;
    uint8_t device_ID;
    uint8_t wifi_action;
    uint8_t mode_select;
    uint8_t SSID_Length;
    uint8_t PASS_Length;
    uint8_t WIFI_SSID[30];
    uint8_t WIFI_PASS[30];
};
*/
//DAP_otaWifiInfo_st _dap_OtaWifiInfo_st;
char* SSID;
char* PASS;

void wifi_initialized(char* Wifi_SSID, char* Wifi_PASS)
{
    Serial.print("[L]SSID: ");
    Serial.print(Wifi_SSID);
    Serial.print(" PASS: ");
    Serial.println(Wifi_PASS);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    //esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    WiFi.begin(Wifi_SSID, Wifi_PASS);

    // Display connection progress
    Serial.print("[L]Connecting to WiFi:");
    Serial.print(WiFi.SSID());
	Serial.print(" ");
    // Wait until WiFi is connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
  }
  
  // Print confirmation message when WiFi is connected
  Serial.println("WiFi connected");
  Serial.print("[L]WiFi RSSI: ");
  Serial.println(WiFi.RSSI());

}
void OTAcallback(int offset, int totallength);

const char *errtext(int code)
{
	switch(code)
	{
		case ESP32OTAPull::UPDATE_AVAILABLE:
			return "An update is available but wasn't installed";
		case ESP32OTAPull::NO_UPDATE_PROFILE_FOUND:
			return "No profile matches";
		case ESP32OTAPull::NO_UPDATE_AVAILABLE:
			return "Profile matched, but update not applicable";
		case ESP32OTAPull::UPDATE_OK:
			return "An update was done, but no reboot";
		case ESP32OTAPull::HTTP_FAILED:
			return "HTTP GET failure";
		case ESP32OTAPull::WRITE_ERROR:
			return "Write error";
		case ESP32OTAPull::JSON_PROBLEM:
			return "Invalid JSON";
		case ESP32OTAPull::OTA_UPDATE_FAIL:
			return "Update fail (no OTA partition?)";
		default:
			if (code > 0)
				return "Unexpected HTTP response code";
			break;
	}
	return "Unknown error";
}
/*
void DisplayInfo()
{
	char exampleImageURL[256];
	snprintf(exampleImageURL, sizeof(exampleImageURL), "https://example.com/Basic-OTA-Example-%s-%s.bin", ARDUINO_BOARD, VERSION);

	Serial.printf("Basic-OTA-Example v%s\n", VERSION);
	Serial.printf("You need to post a JSON (text) file similar to this:\n");
	Serial.printf("{\n");
	Serial.printf("  \"Configurations\": [\n");
	Serial.printf("    {\n");
	Serial.printf("      \"Board\": \"%s\",\n", ARDUINO_BOARD);
	Serial.printf("      \"Device\": \"%s\",\n", WiFi.macAddress().c_str());
	Serial.printf("      \"Version\": \"%s\",\n", VERSION);
	Serial.printf("      \"URL\": \"%s\"\n", exampleImageURL);
	Serial.printf("    }\n");
	Serial.printf("  ]\n");
	Serial.printf("}\n");
	Serial.printf("\n");
	Serial.printf("(Board, Device, Config, and Version are all *optional*.)\n");
	Serial.printf("\n");
	Serial.printf("Post the JSON at, e.g., %s\n", JSON_URL_main);
	Serial.printf("Post the compiled bin at, e.g., %s\n\n", exampleImageURL);
}
*/

void OTAcallback(int offset, int totallength)
{
	Serial.print("[L]Updating: ");
    Serial.print(offset);
    Serial.print(" of ");
    Serial.print(totallength);
    Serial.print("(");
    Serial.print(100 * offset / totallength);
    Serial.println("%)");
    
}