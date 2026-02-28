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
        uint32_t start_time = millis();
        uint32_t timeout_ms = 10;
        while (!usb_hid.ready()) 
        {
            if (millis() - start_time > timeout_ms) 
            {
                break;
            }
            delay(1);
        }
        usb_hid.sendReport(HID_PAYLOAD_INPUT, report, PACKET_SIZE); 
        offset += chunkLen;
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
    if(totalLen== sizeof(DapActions_t))
    {
        DapActions_t tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payloadHeader_st.payloadType_u8 !=DAP_PAYLOAD_TYPE_ACTION_U8 ) structChecker = false;
        if(tmp.payloadHeader_st.version_u8 !=DAP_VERSION_CONFIG_U8 ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payloadHeader_st)), sizeof(tmp.payloadHeader_st) + sizeof(tmp.payloadPedalAction_st));
        if(crc != tmp.payloadFooter_st.checkSum_u16) structChecker = false;
        if(structChecker)
        {
            uint8_t pedalTag= tmp.payloadHeader_st.pedalTag_u8;
            memcpy( &tmpAction[pedalTag], &tmp,totalLen);
            isActionGet[pedalTag]= true;
        }

    }
    if(totalLen== sizeof(DapConfig_t))
    {
        DapConfig_t tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payloadHeader_st.payloadType_u8 !=DAP_PAYLOAD_TYPE_CONFIG_U8 ) structChecker = false;
        if(tmp.payloadHeader_st.version_u8 !=DAP_VERSION_CONFIG_U8 ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payloadHeader_st)), sizeof(tmp.payloadHeader_st) + sizeof(tmp.payloadPedalConfig_st));
        if(crc != tmp.payloadFooter_st.checkSum_u16) structChecker = false;
        if(structChecker)
        {
            uint8_t pedalTag= tmp.payloadHeader_st.pedalTag_u8;
            memcpy(&tmpConfig[pedalTag], &tmp,  totalLen);
            isConfigGet[pedalTag]= true;
            isTestConfigGet[pedalTag] = true;
        }

    }
    if(totalLen== sizeof(DapBridgeState_t))
    {
        DapBridgeState_t tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payloadHeader_st.payloadType_u8 !=DAP_PAYLOAD_TYPE_BRIDGE_STATE_U8 ) structChecker = false;
        if(tmp.payloadHeader_st.version_u8 !=DAP_VERSION_CONFIG_U8 ) structChecker = false;
        uint16_t crc = checksumCal((uint8_t*)(&(tmp.payloadHeader_st)), sizeof(tmp.payloadHeader_st) + sizeof(tmp.payloadBridgeState_st));
        if(crc != tmp.payloadFooter_st.checkSum_u16) structChecker = false;
        if(structChecker)
        {
            memcpy(&tmpBridgeAction, &tmp, totalLen);
            isBridgeActionGet= true;
        }
        
    }
    if(totalLen == sizeof(DapActionOta_t))
    {
        DapActionOta_t tmp;
        memcpy(&tmp, rxBuffer, totalLen);
        bool structChecker= true;
        if(tmp.payloadHeader_st.payloadType_u8 !=DAP_PAYLOAD_TYPE_ACTION_OTA_U8 ) structChecker = false;
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

void TinyusbJoystick::printf(const char *log, ...)
{
  char buffer[235];
  va_list args;
  va_start(args, log);
  int logLen = vsnprintf(buffer, sizeof(buffer), log, args);
  va_end(args);
  if (logLen <= 0) return;
  if (logLen >= (int)sizeof(buffer)) {
      logLen = sizeof(buffer) - 1; 
  }
    PayloadHidMessage_t message;
  message.payloadType_u8 = DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
  message.magicKey1_u8 = ESPNOW_LOG_MAGIC_KEY;
  message.magicKey2_u8 = ESPNOW_LOG_MAGIC_KEY_2;
  message.length_u8 = (uint8_t)logLen;
  memcpy(&message.text_ac, buffer, logLen);
    sendData((uint8_t*)&message, sizeof(PayloadHidMessage_t));
  
  //delay(1);
}
