
#define BRIDGE_FIRMWARE_VERSION "0.90.26"
#if PCB_VERSION==5
	#define BRIDGE_BOARD "Bridge_FANATEC"
#endif
#if PCB_VERSION==6
    #ifdef LOW_TX_POWER
	    #define BRIDGE_BOARD "DevKit_with_low_TX_power"
    #else
        #define BRIDGE_BOARD "DevKit"
    #endif
#endif
#if PCB_VERSION==7
	#define BRIDGE_BOARD "Gilphilbert_Dongle"
#endif
#if PCB_VERSION==8
	#define BRIDGE_BOARD "Bridge_with_external_Joystick"
#endif
#if PCB_VERSION==9
	#define BRIDGE_BOARD "ESP32-C6 devkit"
#endif
void parse_version(char *version, uint8_t *major, uint8_t *minor, uint8_t *patch) {
    int imajor, iminor, ipatch;
    sscanf(version, "%d.%d.%d", &imajor, &iminor, &ipatch);
    *major = (uint8_t)imajor;
    *minor = (uint8_t)iminor;
    *patch = (uint8_t)ipatch;
}
uint8_t versionMajor;
uint8_t versionMinor;
uint8_t versionPatch;