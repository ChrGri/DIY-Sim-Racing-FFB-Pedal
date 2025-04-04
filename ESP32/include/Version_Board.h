const char *DAP_FIRMWARE_VERSION = "0.90.01 ";

#if PCB_VERSION==3
	#define CONTROL_BOARD "V3_ESP32"
#endif

#if PCB_VERSION==5
	#define CONTROL_BOARD "Speedcrafter"
#endif

#if PCB_VERSION==6
	#define CONTROL_BOARD "V4_ESP32S3"
#endif

#if PCB_VERSION==7
	#define CONTROL_BOARD "Gilphilbert_PCBAv1"
#endif

#if PCB_VERSION==8
	#define CONTROL_BOARD "Gilphilbert_PCBAv2"
#endif
#if PCB_VERSION==9
	#define CONTROL_BOARD "V3_ESP32"
#endif
#if PCB_VERSION==10
	#define CONTROL_BOARD "V4_ESP32S3"
#endif
#if PCB_VERSION==11
	#define CONTROL_BOARD "Switch-!t_ESP32S3"
#endif
#if PCB_VERSION==12
	#define CONTROL_BOARD "V5_ESP32S3"
#endif

void parse_version(const char *version, uint8_t *major, uint8_t *minor, uint8_t *patch) {
    sscanf(version, "%d.%d.%d", major, minor, patch);
}