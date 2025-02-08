#define BRIDGE_FIRMWARE_VERSION "0.89.05"
#if PCB_VERSION==5
	#define BRIDGE_BOARD   "Bridge_FANATEC"
#endif
#if PCB_VERSION==6
	#define BRIDGE_BOARD    "DevKit"
#endif
#if PCB_VERSION==7
	#define BRIDGE_BOARD   "Gilphilbert_Dongle"
#endif