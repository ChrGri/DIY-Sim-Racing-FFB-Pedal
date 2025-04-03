#define BRIDGE_FIRMWARE_VERSION "0.90.01"
#if PCB_VERSION==5
	#define BRIDGE_BOARD   "Bridge_FANATEC"
#endif
#if PCB_VERSION==6
	#define BRIDGE_BOARD    "DevKit"
#endif
#if PCB_VERSION==7
	#define BRIDGE_BOARD   "Gilphilbert_Dongle"
#endif

void parse_version(char *version, uint8_t *major, uint8_t *minor, uint8_t *patch) {
    sscanf(version, "%d.%d.%d", major, minor, patch);
}