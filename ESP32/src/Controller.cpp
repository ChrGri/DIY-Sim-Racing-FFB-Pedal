#include "Controller.h"
#include <math.h>

uint16_t IRAM_ATTR_FLAG NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput_u8)
{
  float valRange_fl32 = (maxVal - minVal);
  float deadzoneCorrection_fl32 = 0.005f * valRange_fl32;

  float corrected_min_value_fl32 = minVal + deadzoneCorrection_fl32;
  float corrected_max_value_fl32 = maxVal - deadzoneCorrection_fl32;
  float corrected_valRange_fl32 = (corrected_max_value_fl32 - corrected_min_value_fl32);
  
  if (fabsf(corrected_valRange_fl32) < 0.0000001f)
  {
    return s_JOYSTICK_MIN_VALUE_U16;   // avoid div-by-zero
  }

  float fractional_fl32 = (value - corrected_min_value_fl32) / corrected_valRange_fl32;
  float controller_fl32 = s_JOYSTICK_MIN_VALUE_U16 + (fractional_fl32 * s_JOYSTICK_RANGE_U16);
  uint16_t controller_u16 = constrain(controller_fl32, s_JOYSTICK_MIN_VALUE_U16, (maxGameOutput_u8 * 0.01f) * s_JOYSTICK_MAX_VALUE_U16);
  return controller_u16;
}


#ifdef USB_JOYSTICK

#include <string>
//#include "Adafruit_TinyUSB.h"
#include "Joystick_ESP32S2.h"

#define JOYSTICK_AXIS_MINIMUM_U16 0U
#define JOYSTICK_AXIS_MAXIMUM_U16 65535U

uint16_t pedalType_u16 = 0U; // 0=Clutch, 1=Brake, 2=Throttle

Joystick_ joystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                    0, 0,                 // Button Count, Hat Switch Count
                    false, false, false,  // no X and no Y, no Z Axis
                    true, true, true,  //  Rx, no Ry, no Rz
                    false, false,         // No rudder or throttle
                    false, false, false);  // No accelerator, brake, or steering;

void SetupController_USB(uint8_t pedal_ID)
{
  int pid_i32;
  char *apName_pc;

  pedalType_u16 = pedal_ID;

  switch (pedal_ID)
  {
    case 0:
      pid_i32 = 0x8332;
      apName_pc = "FFB_Pedal_Clutch";
      break;
    case 1:
      pid_i32 = 0x8333;
      apName_pc = "FFB_Pedal_Brake";
      break;
    case 2:
      pid_i32 = 0x8334;
      apName_pc = "FFB_Pedal_Throttle";
      break;
    default:
      pid_i32 = 0x8332;
      apName_pc = "FFB_Pedal_NOASSIGNMENT";
      break;

  }

  int vid_i32 = 0x303A;
  joystick_.setVidPidProductVendorDescriptor(vid_i32, pid_i32, apName_pc, "OpenSource");
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRyAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRzAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.begin();
}

void SetupController()
{
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRyAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRzAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.begin();
}

bool IsControllerReady()
{
  return joystick_.IsReady();
}

void SetControllerOutputValue(uint16_t value)
{
  /*  
  uint8_t highByte = (uint8_t)(value >> 8);
	uint8_t lowByte = (uint8_t)(value & 0x00FF);

  hid_report.brake_lowerByte = lowByte;
  hid_report.brake_higherByte = highByte;

  usb_hid.sendReport(0, &hid_report, sizeof(hid_report));
  */

  switch (pedalType_u16)
  {
    case 0:
      joystick_.setRxAxis(value);
      break;
    case 1:
      joystick_.setRyAxis(value);
      break;
    case 2:
      joystick_.setRzAxis(value);
      break;
    default:
      joystick_.setRxAxis(value);
      break;
  }

  joystick_.sendState();
}



#endif // USB_JOYSTICK


// ============================================================
//  Vendor HID  (UsagePage 0xFF00)  PCB_VERSION == 14
//  INPUT  ReportID=0x02  ESP → PC   63 data bytes per report
//  OUTPUT ReportID=0x03  PC  → ESP  63 data bytes per report
// ============================================================
#ifdef USE_VENDOR_HID

#include "Adafruit_TinyUSB.h"
#include "freertos/queue.h"
#include "DapAttributeProtocol.h"

// Minimal HID report descriptor for a vendor page device
static const uint8_t s_vendor_hid_desc[] = {
  0x06, 0x00, 0xFF,              // Usage Page (Vendor 0xFF00)
  0x09, 0x01,                    // Usage 0x01
  0xA1, 0x01,                    // Collection (Application)
    0x85, 0x02,                  //   Report ID 2 – INPUT  (ESP→PC)
    0x09, 0x02, 0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08, 0x95, 0x3F,      //   8-bit x 63
    0x81, 0x02,                  //   Input (Data,Var,Abs)
    0x85, 0x03,                  //   Report ID 3 – OUTPUT (PC→ESP)
    0x09, 0x03, 0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08, 0x95, 0x3F,
    0x91, 0x02,                  //   Output (Data,Var,Abs)
  0xC0                           // End Collection
};

static Adafruit_USBD_HID s_vendor_hid(
  s_vendor_hid_desc, sizeof(s_vendor_hid_desc),
  HID_ITF_PROTOCOL_NONE, 1 /*ms poll*/, false);

#define VHID_REPORT_BYTES  63
#define VHID_HEADER_BYTES   3    // [pkt_type][totalLen_lo][chunkLen]
#define VHID_CHUNK_DATA    (VHID_REPORT_BYTES - VHID_HEADER_BYTES)  // 60
#define VHID_PKT_START     0x01
#define VHID_PKT_CONT      0x02
#define VHID_PACKET_MAX    512
#define VHID_QUEUE_DEPTH     4

// Attribute packet queue (depth 32, one entry per inbound attribute write/read)
#define VHID_ATTR_QUEUE_DEPTH 32

typedef struct { uint8_t data[VHID_PACKET_MAX]; uint16_t len; } VhidPacket_t;

static QueueHandle_t s_vhid_pkt_queue = NULL;

// Attribute packet queue – populated from s_vendor_set_report() when
// buf[0] == DAP_ATTR_HID_MARKER; drained by serialCommunicationTaskRx.
static QueueHandle_t s_vhid_attr_queue = NULL;

// Reassembly state – written only from USB OUTPUT callback (ISR-like context)
static uint8_t  s_vhid_rx_buf[VHID_PACKET_MAX];
static uint16_t s_vhid_rx_len      = 0;
static uint16_t s_vhid_rx_expected = 0;
static bool     s_vhid_rx_active   = false;

static void s_vendor_set_report(uint8_t report_id, hid_report_type_t rtype,
                                uint8_t const* buf, uint16_t bufsize)
{
  if (report_id != 0x03 || bufsize < VHID_HEADER_BYTES) return;

  // --- Attribute packet fast path ---
  // If the first byte is the attribute marker (0xA1) the payload is a
  // DapAttrPacket_t (12 bytes) starting at buf[1].  Queue it directly and
  // skip the legacy reassembly state machine entirely.
  if (buf[0] == DAP_ATTR_HID_MARKER) {
    if (bufsize >= 1u + DAP_ATTR_PACKET_BYTES && s_vhid_attr_queue) {
      DapAttrPacket_t pkt;
      memcpy(&pkt, buf + 1, DAP_ATTR_PACKET_BYTES);
      BaseType_t woken = pdFALSE;
      xQueueSendFromISR(s_vhid_attr_queue, &pkt, &woken);
      if (woken) portYIELD_FROM_ISR();
    }
    return;
  }

  // --- Legacy chunked-struct reassembly ---
  uint8_t  pkt_type  = buf[0];
  uint16_t total_len = buf[1];
  uint8_t  chunk_len = buf[2];
  if (chunk_len > VHID_CHUNK_DATA) chunk_len = VHID_CHUNK_DATA;

  if (pkt_type == VHID_PKT_START) {
    s_vhid_rx_len = 0; s_vhid_rx_expected = total_len; s_vhid_rx_active = true;
    if (total_len > VHID_PACKET_MAX) { s_vhid_rx_active = false; return; }
  }
  if (s_vhid_rx_active && chunk_len > 0 &&
      s_vhid_rx_len + chunk_len <= VHID_PACKET_MAX) {
    memcpy(s_vhid_rx_buf + s_vhid_rx_len, buf + VHID_HEADER_BYTES, chunk_len);
    s_vhid_rx_len += chunk_len;
  }
  if (s_vhid_rx_active && s_vhid_rx_len >= s_vhid_rx_expected) {
    s_vhid_rx_active = false;
    if (s_vhid_pkt_queue) {
      VhidPacket_t pkt; pkt.len = s_vhid_rx_expected;
      memcpy(pkt.data, s_vhid_rx_buf, pkt.len);
      BaseType_t woken = pdFALSE;
      xQueueSendFromISR(s_vhid_pkt_queue, &pkt, &woken);
      if (woken) portYIELD_FROM_ISR();
    }
  }
}

void SetupVendorHID() {
  s_vhid_pkt_queue  = xQueueCreate(VHID_QUEUE_DEPTH,      sizeof(VhidPacket_t));
  s_vhid_attr_queue = xQueueCreate(VHID_ATTR_QUEUE_DEPTH, sizeof(DapAttrPacket_t));
  s_vendor_hid.setReportCallback(NULL, s_vendor_set_report);
  s_vendor_hid.begin();
}

// Send arbitrary byte array to PC, chunked into 63-byte INPUT reports.
// Waits for USB readiness before each chunk using vTaskDelay(1) so the
// TinyUSB task can run. Total blocking time ≈ num_chunks × 1ms poll interval.
void VendorHID_SendData(const uint8_t* data, size_t len) {
  size_t offset = 0;
  while (offset < len) {
    // Wait for the host to consume the previous INPUT report (at most 20ms).
    uint32_t t0 = millis();
    while (!s_vendor_hid.ready()) {
      if ((millis() - t0) > 20) return;  // host stopped polling – abort
      vTaskDelay(1);
    }
    uint8_t report[VHID_REPORT_BYTES] = {0};
    size_t chunk = len - offset;
    if (chunk > VHID_CHUNK_DATA) chunk = VHID_CHUNK_DATA;
    report[0] = (offset == 0) ? VHID_PKT_START : VHID_PKT_CONT;
    report[1] = (uint8_t)len;       // total length (max 255 bytes; all our structs fit)
    report[2] = (uint8_t)chunk;
    memcpy(report + VHID_HEADER_BYTES, data + offset, chunk);
    if (s_vendor_hid.sendReport(0x02, report, VHID_REPORT_BYTES)) {
      offset += chunk;  // advance only on successful queue
    }
  }
}

// Dequeue one reassembled packet; returns true if data available
bool VendorHID_ReadPacket(uint8_t* buf, size_t* outLen) {
  if (!s_vhid_pkt_queue) return false;
  VhidPacket_t pkt;
  if (xQueueReceive(s_vhid_pkt_queue, &pkt, 0) == pdPASS) {
    size_t copy = (pkt.len < *outLen) ? pkt.len : *outLen;
    memcpy(buf, pkt.data, copy);
    *outLen = copy;
    return true;
  }
  return false;
}

// Dequeue one attribute packet; returns true if data available.
// Called from serialCommunicationTaskRx to drain s_vhid_attr_queue.
bool VendorHID_ReadAttrPacket(DapAttrPacket_t* out) {
  if (!s_vhid_attr_queue) return false;
  return xQueueReceive(s_vhid_attr_queue, out, 0) == pdPASS;
}

// Send a 12-byte attribute reply to the PC as a single HID INPUT report.
// buf[0] = DAP_ATTR_HID_MARKER, buf[1..12] = *pkt, remainder zero.
void VendorHID_SendAttrReply(const DapAttrPacket_t* pkt) {
  if (!pkt) return;
  uint8_t report[VHID_REPORT_BYTES] = {0};
  report[0] = DAP_ATTR_HID_MARKER;
  memcpy(report + 1, pkt, DAP_ATTR_PACKET_BYTES);
  // Wait up to 20ms for the host to be ready, then send.
  uint32_t t0 = millis();
  while (!s_vendor_hid.ready()) {
    if ((millis() - t0) > 20) return;
    vTaskDelay(1);
  }
  s_vendor_hid.sendReport(0x02, report, VHID_REPORT_BYTES);
}

#endif // USE_VENDOR_HID
