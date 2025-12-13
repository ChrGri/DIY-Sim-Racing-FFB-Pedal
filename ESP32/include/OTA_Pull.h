#include <Arduino.h>
#include <ESP32OTAPull.h>
#include <esp_wifi.h>
#include <math.h>
#define OTA_JSON_URL_DEV   "https://raw.githubusercontent.com/gilphilbert/pedal-flasher/main/json/dev/Version_ControlBoard.json"
//#define JSON_URL_dev "https://raw.githubusercontent.com/tcfshcrw/playground/main/OTA_test_repo/GH2/Version.json"
#define OTA_JSON_URL_MAIN   "https://raw.githubusercontent.com/gilphilbert/pedal-flasher/main/json/main/Version_ControlBoard.json"
#define OTA_JSON_URL_TEST "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/TestBuild/json/Version_ControlBoard.json"
bool OTA_enable_b =false;
bool OTA_status =false;
bool beepForOtaProgress = false;
bool OTA_enable_start=false;
char* SSID;
char* PASS;

void wifi_initialized(char* Wifi_SSID, char* Wifi_PASS)
{
    ActiveSerial->print("SSID: ");
    ActiveSerial->print(Wifi_SSID);
    ActiveSerial->print(" PASS: ");
    ActiveSerial->println(Wifi_PASS);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    //esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
    WiFi.begin(Wifi_SSID, Wifi_PASS);

    // Display connection progress
    ActiveSerial->print("Connecting to WiFi:");
    ActiveSerial->print(WiFi.SSID());
	ActiveSerial->print(" ");
    // Wait until WiFi is connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ActiveSerial->print(".");
  }
  
  // Print confirmation message when WiFi is connected
  ActiveSerial->println("WiFi connected");
  ActiveSerial->print("WiFi RSSI: ");
  ActiveSerial->println(WiFi.RSSI());

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

	ActiveSerial->printf("Basic-OTA-Example v%s\n", VERSION);
	ActiveSerial->printf("You need to post a JSON (text) file similar to this:\n");
	ActiveSerial->printf("{\n");
	ActiveSerial->printf("  \"Configurations\": [\n");
	ActiveSerial->printf("    {\n");
	ActiveSerial->printf("      \"Board\": \"%s\",\n", ARDUINO_BOARD);
	ActiveSerial->printf("      \"Device\": \"%s\",\n", WiFi.macAddress().c_str());
	ActiveSerial->printf("      \"Version\": \"%s\",\n", VERSION);
	ActiveSerial->printf("      \"URL\": \"%s\"\n", exampleImageURL);
	ActiveSerial->printf("    }\n");
	ActiveSerial->printf("  ]\n");
	ActiveSerial->printf("}\n");
	ActiveSerial->printf("\n");
	ActiveSerial->printf("(Board, Device, Config, and Version are all *optional*.)\n");
	ActiveSerial->printf("\n");
	ActiveSerial->printf("Post the JSON at, e.g., %s\n", JSON_URL_main);
	ActiveSerial->printf("Post the compiled bin at, e.g., %s\n\n", exampleImageURL);
}
*/

void OTAcallback(int offset, int totalLength)
{
	float progressInPrecent = (100.0f * (float)offset / (float)totalLength);
	float progressBeepTolerance= 0.1f;
    ActiveSerial->printf("Updating: %d of %d, (%0.0f %)\n",offset, totalLength, progressInPrecent);
	//call beep callback
	if(fmod(progressInPrecent,10.0f)<=progressBeepTolerance && beepForOtaProgress==false && progressInPrecent>1.0f)
	{
		beepForOtaProgress=true;
	}
}