#include "TinyusbJoystick.h"

TinyusbJoystick::TinyusbJoystick() 
{     
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

void TinyusbJoystick::begin()
{
    // Set VID and PID
    int PID = 0x8213;
    int VID = 0x3035;
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
    usb_hid.setPollInterval(8); // time in ms
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.begin();

    // If already enumerated, additional class driverr begin() e.g msc, hid, midi won't take effect until re-enumeration
    if (TinyUSBDevice.mounted())
    {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }
}

void TinyusbJoystick::setRxAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.rx = tmp;
}

void TinyusbJoystick::setRyAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.ry = tmp;
}
void TinyusbJoystick::setRzAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.rz = tmp;
}

void TinyusbJoystick::setXAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.x = tmp;
}
void TinyusbJoystick::setYAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.y = tmp;
}
void TinyusbJoystick::setZAxis(int16_t value)
{
    int16_t tmp = value;
    hid_report.z = tmp;
}

void TinyusbJoystick::sendState()
{
    usb_hid.sendReport(0, &hid_report, sizeof(hid_report));
}