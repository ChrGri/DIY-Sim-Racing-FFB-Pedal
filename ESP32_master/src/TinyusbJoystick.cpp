#include "TinyusbJoystick.h"
#define ESPNOW_LOG_MAGIC_KEY 0x99
#define ESPNOW_LOG_MAGIC_KEY_2 0x97
TinyusbJoystick* TinyusbJoystick::instance = nullptr;
TinyusbJoystick::TinyusbJoystick() 
{    
    isBridgeActionGet=false;
}

bool TinyusbJoystick::IsReady()
{
    bool returnValue_b = true;
    if (!TinyUSBDevice.mounted())
    {
        returnValue_b = false;
    }
    if (!usb_hid.ready())
    {
        returnValue_b = false;
    }

    return returnValue_b;
}

void TinyusbJoystick::begin(int VID, int PID)
{
    instance=this;
    //Serial.println("[L]starting USB joystick");
    TinyUSBDevice.setID(VID, PID);
    TinyUSBDevice.setProductDescriptor("DIY_FFB_PEDAL_JOYSTICK");
    TinyUSBDevice.setManufacturerDescriptor("OPENSOURCE");
    
    //ActiveSerial->
    // Manual begin() is required on core without built-in support e.g. mbed rp2040
    if (!TinyUSBDevice.isInitialized())
    {
        TinyUSBDevice.begin(0);
    }

    // Setup HID
    usb_hid.enableOutEndpoint(true); 
    usb_hid.setPollInterval(1); // time in ms
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.begin();
    usb_hid.setReportCallback(NULL, TinyusbJoystick::context_callback);

    // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
    if (TinyUSBDevice.mounted())
    {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
        
    }
}

void TinyusbJoystick::setRxAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.rx = tmp;
}

void TinyusbJoystick::setRyAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.ry = tmp;
}
void TinyusbJoystick::setRzAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.rz = tmp;
}

void TinyusbJoystick::setXAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.x = tmp;
}
void TinyusbJoystick::setYAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.y = tmp;
}
void TinyusbJoystick::setZAxis(int32_t value)
{
    int32_t tmp = value;
    hid_report.z = tmp;
}

void TinyusbJoystick::sendState()
{
    usb_hid.sendReport(JOYSTICK_STRUCT, &hid_report, sizeof(hid_report));
}

void TinyusbJoystick::sendData(uint8_t* data, size_t totalLen) 
{
    size_t offset = 0;
    
    while (offset < totalLen) 
    {
        uint8_t report[PACKET_SIZE]; 
        memset(report, 0, PACKET_SIZE); 
        size_t chunkLen = totalLen - offset;
        if (chunkLen > PAYLOAD_SIZE) chunkLen = PAYLOAD_SIZE; 

        uint8_t type = (offset == 0) ? PKT_TYPE_START : PKT_TYPE_CONT;
        report[0] = type;
        report[1] = (uint8_t)totalLen; 
        report[2] = (uint8_t)chunkLen; 
        memcpy(&report[3], &data[offset], chunkLen);
        usb_hid.sendReport(HID_PAYLOAD_INPUT, report, PACKET_SIZE); 

        offset += chunkLen;
        delay(2);
    }
}

void TinyusbJoystick::onHIDReceived(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 
{
    //isGetData=true;
    buffSizeDis=bufsize;
    reportID=report_id;
    reportType=report_type;
    memcpy(&buffDis[0], &buffer[0], bufsize);
    if (report_type != HID_REPORT_TYPE_OUTPUT || buffer[0] != HID_PAYLOAD_OUTPUT) return;
    if (bufsize < HEADER_SIZE) return;

    uint8_t type = buffer[1];
    uint8_t totallen = buffer[2];
    uint8_t len = buffer[3];
    if (type == PKT_TYPE_START) {
        rxIndex = 0;
        rawLength = totallen; 
        isReceiving = true;
    }

    if (isReceiving) {
        if (rxIndex + len > sizeof(rxBuffer)) {
            isReceiving = false; 
            return;
        }
        if (len > 0) {
            memcpy(&rxBuffer[rxIndex], &buffer[HEADER_SIZE_GET], len);
            rxIndex += len;
        }
        if (rxIndex >= rawLength) {
            isReceiving = false;
            ProcessFullData(rxBuffer, rawLength);
        }
    }
}
void TinyusbJoystick::context_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) 
{
    if (instance != nullptr) {
        instance->onHIDReceived(report_id, report_type, buffer, bufsize);
    }
}

void TinyusbJoystick::ProcessFullData(uint8_t *rxBuffer, uint8_t totalLen)
{
    if(totalLen== sizeof(DAP_actions_st))
    {
        DAP_actions_st tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payLoadHeader_.payloadType !=DAP_PAYLOAD_TYPE_ACTION ) structChecker = false;
        if(tmp.payLoadHeader_.version !=DAP_VERSION_CONFIG ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payLoadHeader_)), sizeof(tmp.payLoadHeader_) + sizeof(tmp.payloadPedalAction_));
        if(crc != tmp.payloadFooter_.checkSum) structChecker = false;
        if(structChecker)
        {
            uint8_t pedalTag= tmp.payLoadHeader_.PedalTag;
            memcpy( &tmpAction[pedalTag], &tmp,totalLen);
            isActionGet[pedalTag]= true;
        }

    }
    if(totalLen== sizeof(DAP_config_st))
    {
        DAP_config_st tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payLoadHeader_.payloadType !=DAP_PAYLOAD_TYPE_CONFIG ) structChecker = false;
        if(tmp.payLoadHeader_.version !=DAP_VERSION_CONFIG ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payLoadHeader_)), sizeof(tmp.payLoadHeader_) + sizeof(tmp.payLoadPedalConfig_));
        if(crc != tmp.payloadFooter_.checkSum) structChecker = false;
        if(structChecker)
        {
            uint8_t pedalTag= tmp.payLoadHeader_.PedalTag;
            memcpy(&tmpConfig[pedalTag], &tmp,  totalLen);
            isConfigGet[pedalTag]= true;
            isTestConfigGet[pedalTag] = true;
        }

    }
    if(totalLen== sizeof(DAP_bridge_state_st))
    {
        DAP_bridge_state_st tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payLoadHeader_.payloadType !=DAP_PAYLOAD_TYPE_BRIDGE_STATE ) structChecker = false;
        if(tmp.payLoadHeader_.version !=DAP_VERSION_CONFIG ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payLoadHeader_)), sizeof(tmp.payLoadHeader_) + sizeof(tmp.payloadBridgeState_));
        if(crc != tmp.payloadFooter_.checkSum) structChecker = false;
        if(structChecker)
        {
            memcpy(&tmpBridgeAction, &tmp, totalLen);
            isBridgeActionGet= true;
        }
        
    }
    if(totalLen == sizeof(DAP_action_ota_st))
    {
        DAP_action_ota_st tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payLoadHeader_.payloadType !=DAP_PAYLOAD_TYPE_ACTION_OTA ) structChecker = false;
        if(structChecker)
        {
            memcpy(&tmpOtaAction, &tmp, totalLen);
            isOtaActionGet = true;
        }
    }
}

uint16_t TinyusbJoystick::checksumCal(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}


void TinyusbJoystick::printf(const char *log,...)
{
  uint8_t buffer[235];
  uint8_t payloadType = DAP_PAYLOAD_TYPE_ESPNOW_LOG;
  Dap_hidmessage_st message;
  //uint8_t logLen = strlen(log); 
  va_list args;
  char* result = NULL;
  int needed_size;
  va_start(args, log); // initialized va_list
  needed_size = vsnprintf(NULL, 0, log, args);
  va_end(args); 
  if (needed_size < 0) return;
  result = (char*)malloc(needed_size + 1);
  // malloc error
  if (result == NULL) return;
  va_start(args, log); 
  vsnprintf(result, needed_size + 1, log, args);
  va_end(args); 
  int logLen=strlen(result);
  if (logLen > 235) logLen = 235;
  message.payloadType = payloadType;
  message.magicKey1 = ESPNOW_LOG_MAGIC_KEY;
  message.magicKey2 = ESPNOW_LOG_MAGIC_KEY_2;
  message.length = logLen;
  memcpy(&message.text, result, logLen);
  sendData((uint8_t*)&message, sizeof(Dap_hidmessage_st));
  delay(10);
  //ESPNow.send_message(broadcast_mac, (uint8_t *)buffer, 4 + logLen);
  free(result);
}
