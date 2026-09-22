#pragma once
#ifdef ESPNOW_Enable
#include "DiyActivePedal_types.h"
#include "ESPNowW.h"
#include "StepperMovementStrategy_Rudder.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const bool IS_ESPNOW_ENABLED = true;

#define WIFI_CH_EEPROM_MAGIC 0xA6
#define WIFI_CH_EEPROM_OFFSET 260
struct WifiChannelConfig_t {
  uint8_t magic_u8;
  uint8_t channel_u8;
  uint8_t checksum_u8;
};

// Uncomment to enable verbose wireless transport debug logging (ActiveSerial
// only - the pedal has no tinyusbJoystick_-style USB HID text mirror, that
// pattern is bridge-only). #define WIRELESS_COMM_DEBUG

#define ESPNOW_LOG_MAGIC_KEY_U8 0x99
#define ESPNOW_LOG_MAGIC_KEY_2_U8 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY_U8 0x99

static volatile uint8_t s_localPedalType_u8 = PEDAL_ID_UNKNOWN;

// --- Pedal-behavior state flags -----------------------------------------
// These are business-logic flags that Main.cpp reads/writes regardless of
// the wireless transport internals below; they are not transport state so
// they stay as plain globals rather than class members.
uint16_t g_espNowSend_u16 = 0;
uint16_t g_espNowReceive_u16 = 0;
bool g_espNowRudderUpdate_b = false;
bool g_espNowNoDevice_b = false;
bool g_espNowConfigRequest_b = false;
bool g_espNowRestart_b = false;
bool g_espNowOtaEnable_b = false;
uint8_t g_espNowErrorCode_u8 = 0;
bool g_otaUpdateAction_b = false;
bool g_configUpdate_b = false;
volatile bool g_rudderInitializing_b = false;
volatile bool g_rudderDeinitializing_b = false;
volatile bool g_heliRudderInitializing_b = false;
volatile bool g_heliRudderDeinitializing_b = false;
bool g_espNowBootIntoDownloadMode_b = false;
bool g_getRudderAction_b = false;
bool g_getHeliRudderAction_b = false;
extern bool g_assignmentClear_b;
extern bool g_assignmentUpdate_b;
extern uint8_t g_newAssignedRole_u8;
extern bool buzzerBeepAction_b;
bool g_printPedalInfo_b = false;
bool g_configUpdateBuzzer_b = false;
unsigned long g_rudderInitializedTime_u32 = 0;
volatile uint32_t g_lastPartnerTimestamp_ms = 0;
volatile uint8_t g_currentSyncDelay_ms = 0;
volatile uint32_t g_lastMasterHeartbeat_ms = 0;
volatile bool g_saveWifiChannelDeferred_b = false;

inline uint8_t loadWifiChannelFromEeprom() {
  WifiChannelConfig_t cfg;
  EEPROM.get(WIFI_CH_EEPROM_OFFSET, cfg);
  if (cfg.magic_u8 == WIFI_CH_EEPROM_MAGIC &&
      (uint8_t)(cfg.magic_u8 ^ cfg.channel_u8) == cfg.checksum_u8 &&
      cfg.channel_u8 >= 1 && cfg.channel_u8 <= 14) {
    return cfg.channel_u8;
  }
  return 11;
}

inline void saveWifiChannelToEeprom(uint8_t ch) {
  if (ch < 1 || ch > 14)
    return;
  WifiChannelConfig_t cfg;
  cfg.magic_u8 = WIFI_CH_EEPROM_MAGIC;
  cfg.channel_u8 = ch;
  cfg.checksum_u8 = (uint8_t)(WIFI_CH_EEPROM_MAGIC ^ ch);
  EEPROM.put(WIFI_CH_EEPROM_OFFSET, cfg);
  EEPROM.commit();
}

inline bool macCheck(const uint8_t *Mac_A, const uint8_t *Mac_B) {
  return memcmp(Mac_A, Mac_B, 6) == 0;
}

inline DapMacAddresses_t loadMacAddressesFromEeprom() {
  DapMacAddresses_t macCfg;
  EEPROM.get(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  if (macCfg.payloadHeader_st.payloadType_u8 ==
          DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8 &&
      macCfg.payloadHeader_st.version_u8 == DAP_VERSION_MAC_ADDRESSES_U8) {
    uint16_t crc =
        checksumCalculator_u16((uint8_t *)(&(macCfg.payloadHeader_st)),
                               sizeof(macCfg.payloadHeader_st) +
                                   sizeof(macCfg.payloadMacAddresses_st));
    if (crc == macCfg.payloadFooter_st.checkSum_u16) {
      WiFi.macAddress(macCfg.payloadMacAddresses_st.ownMacAddress_au8);
      macCfg.payloadMacAddresses_st.ownNodeType_u8 = s_localPedalType_u8;
      return macCfg;
    }
  }
  memset(&macCfg, 0, sizeof(macCfg));
  macCfg.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  macCfg.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  macCfg.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
  macCfg.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
  macCfg.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  macCfg.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  macCfg.payloadMacAddresses_st.wifiChannel_u8 = 11;
  WiFi.macAddress(macCfg.payloadMacAddresses_st.ownMacAddress_au8);
  macCfg.payloadMacAddresses_st.ownNodeType_u8 = s_localPedalType_u8;
  return macCfg;
}

inline void storeMacAddressesToEeprom(DapMacAddresses_t &macCfg) {
  macCfg.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  macCfg.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  macCfg.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
  macCfg.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
  macCfg.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  macCfg.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  macCfg.payloadFooter_st.checkSum_u16 = checksumCalculator_u16(
      (uint8_t *)(&(macCfg.payloadHeader_st)),
      sizeof(macCfg.payloadHeader_st) + sizeof(macCfg.payloadMacAddresses_st));
  EEPROM.put(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  EEPROM.commit();
}

// Forward declarations - ESP-NOW's C callback API needs plain function
// pointers, not member function pointers, so these small trampolines just
// forward into the wirelessComm singleton (defined below).
void sWirelessCommRecvTrampoline(const esp_now_recv_info_t *info,
                                 const uint8_t *data, int len);
void sWirelessCommSentTrampoline(const esp_now_send_info_t *info,
                                 esp_now_send_status_t status);

// =========================================================================
// WirelessCommunicationPedal
//
// Unicast: bridge-bound traffic targets _hostMac, rudder sync targets
// whichever sibling pedal is the currently-resolved rudder partner - all
// through sendTo(), the sole esp_now_send() call site, which implements
// the flow-control this repo already learned it needs the hard way (see
// the history note on sendTo() below). The old broadcast peer/
// sendBroadcast() path is kept only as a fallback for anything not yet
// converted. Receivers filter by source MAC against the addresses loaded
// from DapMacAddresses_t (no auto-discovery: a MAC must already be
// provisioned, e.g. via the USB config tool, before its traffic is
// accepted).
// =========================================================================
class WirelessCommunicationPedal {
public:
  void begin(const DapMacAddresses_t &macCfg) {
    DapConfig_t dap_config_espnow_init_st;
    global_dap_config_class.getConfig(&dap_config_espnow_init_st, 500);
    s_localPedalType_u8 =
        dap_config_espnow_init_st.payloadPedalConfig_st.pedalType_u8;

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(false, false);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    delay(500);
    ActiveSerial->println("Initializing Wifi, please wait");
    WiFi.macAddress(_ownMac);
    ActiveSerial->printf("Device Mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
                         _ownMac[0], _ownMac[1], _ownMac[2], _ownMac[3],
                         _ownMac[4], _ownMac[5]);

    ActiveSerial->println("Initializing ESP-NOW");
    esp_err_t initRes = ESPNow.init();
    if (initRes != ESP_OK) {
      ActiveSerial->printf("[ESPNOW ERROR] ESPNow.init() failed: %s\n",
                           esp_err_to_name(initRes));
    }

    applyMacConfig(macCfg);

#ifdef ESPNow_S3
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
#ifdef LOWER_WIFI_TRANSMISSION_POWER
    esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
#endif
#endif
#ifdef ESPNow_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
#endif

    ActiveSerial->printf(
        "[MAC] Configured Bridge MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        _hostMac[0], _hostMac[1], _hostMac[2], _hostMac[3], _hostMac[4],
        _hostMac[5]);

    uint8_t hwChan = 0;
    wifi_second_chan_t secChan;
    esp_wifi_get_channel(&hwChan, &secChan);
    ActiveSerial->printf(
        "ESP-NOW Radio Channel active: %d (Hardware Wi-Fi Channel: %d)\n",
        _currentChannel, hwChan);

    if (!esp_now_is_peer_exist(_broadcastMac)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, _broadcastMac, 6);
      peerInfo.channel = 0;
      peerInfo.ifidx = WIFI_IF_STA;
      peerInfo.encrypt = false;
      esp_now_add_peer(&peerInfo);
    }
    ActiveSerial->println("Sucess to add peers");

    ESPNow.reg_recv_cb(sWirelessCommRecvTrampoline);
    ESPNow.reg_send_cb(sWirelessCommSentTrampoline);

    _initialStatus = true;
    _started = true;
    ActiveSerial->println("ESPNow Initialized");
  }

  // Updates channel + trusted MAC table only (no WiFi/ESP-NOW re-init) -
  // used both by begin() and whenever the host pushes an updated MAC table
  // at runtime over serial.
  void applyMacConfig(const DapMacAddresses_t &macCfg) {
    uint8_t ch = macCfg.payloadMacAddresses_st.wifiChannel_u8;
    if (ch >= 1 && ch <= 13) {
      _currentChannel = ch;
    }
    esp_wifi_set_channel(_currentChannel, WIFI_SECOND_CHAN_NONE);

    // Bridge is node 3
    memcpy(_hostMac, macCfg.payloadMacAddresses_st.macAddress_aau8[3], 6);
    // Pedals are nodes 0..2
    for (int i = 0; i < 3; i++) {
      memcpy(_pedalMac[i], macCfg.payloadMacAddresses_st.macAddress_aau8[i], 6);
    }
    syncPeerTable();
  }

  esp_err_t deinit() {
    esp_err_t result = esp_now_deinit();
    _initialStatus = false;
    _started = false;
    return result;
  }

  bool isStarted() const { return _started; }
  bool isInitialStatusDone() const { return _initialStatus; }
  const uint8_t *getOwnMac() const { return _ownMac; }
  const uint8_t *getHostMac() const { return _hostMac; }
  const uint8_t *getPedalMac(uint8_t idx) const {
    return _pedalMac[idx < 3 ? idx : 0];
  }
  uint8_t getChannel() const { return _currentChannel; }
  void setChannel(uint8_t ch) { _currentChannel = ch; }
  int32_t *getRssiArray() { return _rssi; }
  const DapRudder_t &getRudderRx() const { return _rudderRx; }
  uint32_t getTxSuccessCount() const { return _txSuccessCount; }
  uint32_t getTxFailCount() const { return _txFailCount; }
  uint32_t getTxErrCount() const { return _txErrCount; }
  uint32_t getTxNoMemCount() const { return _txNoMemCount_u32; }
  uint32_t getTxBusySkipCount() const { return _txBusySkipCount_u32; }
  bool isTxBusy() const { return _txInFlight_b || millis() < _noMemBackoffUntil_ms; }
  uint32_t getLastTxTime() const { return _lastTxTime; }
  uint32_t getLastRxTime() const { return _lastRxTime; }

  // TEMP DIAGNOSTICS for the unicast rudder-sync path - rudder is the exact
  // packet type/cadence that historically caused the TX FIFO stall this
  // repo already hit once under unicast (see the history note on sendTo()),
  // and it's also the newest piece of unicast-specific logic (partner
  // resolution). These make it possible to tell apart "partner never
  // resolved" vs "stuck busy every attempt" vs "sends fine but receiver
  // rejects it" from the field without guessing. Remove once rudder sync
  // is confirmed working again.
  uint8_t getRudderPartnerId() const { return _rudderPartnerId_u8; }
  uint32_t getRudderTxOkCount() const { return _rudderTxOk_u32; }
  uint32_t getRudderTxSkipNoPartnerCount() const { return _rudderTxSkipNoPartner_u32; }
  uint32_t getRudderTxSkipBusyCount() const { return _rudderTxSkipBusy_u32; }
  uint32_t getRudderRxAcceptedCount() const { return _rudderRxAccepted_u32; }
  uint32_t getRudderRxRejectedCount() const { return _rudderRxRejected_u32; }
  // Call from the rudder-sync task-loop call site instead of isTxBusy()
  // directly, so a busy-skip there is counted here too.
  bool shouldSkipRudderSyncForBusy() {
    if (isTxBusy()) {
      _rudderTxSkipBusy_u32++;
      return true;
    }
    return false;
  }

  esp_err_t sendBasicStateToBridge(const DapStateBasic_t &pkt) {
    if (isAllZero(_hostMac)) return ESP_ERR_INVALID_ARG;
    logDebug("TX BasicState len=%u", (unsigned)sizeof(pkt));
    return sendTo(_hostMac, (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendExtendedStateToBridge(const DapStateExtended_t &pkt) {
    if (isAllZero(_hostMac)) return ESP_ERR_INVALID_ARG;
    logDebug("TX ExtendedState len=%u", (unsigned)sizeof(pkt));
    return sendTo(_hostMac, (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendConfigEchoToBridge(const DapConfig_t &pkt) {
    if (isAllZero(_hostMac)) return ESP_ERR_INVALID_ARG;
    logDebug("TX ConfigEcho len=%u", (unsigned)sizeof(pkt));
    return sendTo(_hostMac, (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendServoConfigResponseToBridge(const DAP_servo_config_st_t &pkt) {
    if (isAllZero(_hostMac)) return ESP_ERR_INVALID_ARG;
    logDebug("TX ServoConfigResponse len=%u", (unsigned)sizeof(pkt));
    return sendTo(_hostMac, (const uint8_t *)&pkt, sizeof(pkt));
  }

  // Targets whichever sibling pedal handleActionsPacket's rudder mode-select
  // most recently resolved as our partner (see _rudderPartnerId_u8) - safe
  // no-op if no partner is currently resolved, rather than ever sending to
  // a garbage/zero MAC.
  esp_err_t sendRudderSync(const DapRudder_t &pkt) {
    if (_rudderPartnerId_u8 >= 3 || isAllZero(_pedalMac[_rudderPartnerId_u8])) {
      _rudderTxSkipNoPartner_u32++;
      return ESP_ERR_INVALID_ARG;
    }
    _rudderTx = pkt;
    logDebug("TX RudderSync len=%u", (unsigned)sizeof(pkt));
    esp_err_t res = sendTo(_pedalMac[_rudderPartnerId_u8], (const uint8_t *)&pkt, sizeof(pkt));
    if (res == ESP_OK) {
      _rudderTxOk_u32++;
    }
    return res;
  }

  void sendLogToBridge(const char *fmt, ...) {
    if (isAllZero(_hostMac)) return;
    uint8_t buffer[250];
    char textBuf[240];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(textBuf, sizeof(textBuf), fmt, args);
    va_end(args);
    if (len <= 0)
      return;
    if (len > 235)
      len = 235;
    buffer[0] = DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
    buffer[1] = ESPNOW_LOG_MAGIC_KEY_U8;
    buffer[2] = ESPNOW_LOG_MAGIC_KEY_2_U8;
    buffer[3] = (uint8_t)len;
    memcpy(&buffer[4], textBuf, len);
    logDebug("TX Log: %s", textBuf);
    sendTo(_hostMac, buffer, 4 + len);
  }

  void handleRecv(const esp_now_recv_info_t *info, const uint8_t *data,
                  int len) {
    if (info == NULL || info->src_addr == NULL || data == NULL || len <= 0) {
      return;
    }
    const uint8_t *src = info->src_addr;

    // Ignore self loopback
    if (macCheck(_ownMac, src)) {
      return;
    }

    if (!_started) {
      return;
    }

    bool hasHost = !isAllZero(_hostMac);
    bool isHostSender = hasHost && macCheck(_hostMac, src);

    if (info->rx_ctrl != NULL) {
      for (int i = 0; i < 3; i++) {
        if (macCheck(src, _pedalMac[i])) {
          _rssi[i] = info->rx_ctrl->rssi;
          break;
        }
      }
      if (isHostSender) {
        _rssi[3] = info->rx_ctrl->rssi;
      }
    }

    _lastRxTime = millis();

    // --- Rudder relay: accepted from any of the two other configured pedals
    // ---
    bool isKnownPedalSender = macCheck(src, _pedalMac[0]) ||
                              macCheck(src, _pedalMac[1]) ||
                              macCheck(src, _pedalMac[2]);
    if (isKnownPedalSender && len == sizeof(DapRudder_t)) {
      DapRudder_t rudderLocal;
      memcpy(&rudderLocal, data, sizeof(DapRudder_t));
      bool ok =
          rudderLocal.payloadHeader_st.payloadType_u8 ==
              DAP_PAYLOAD_TYPE_ESPNOW_RUDDER_U8 &&
          rudderLocal.payloadHeader_st.version_u8 == DAP_VERSION_CONFIG_U8;
      if (ok) {
        uint16_t crc = checksumCalculator_u16(
            (uint8_t *)(&(rudderLocal.payloadHeader_st)),
            sizeof(rudderLocal.payloadHeader_st) +
                sizeof(rudderLocal.payloadRudderState_st));
        ok = (crc == rudderLocal.payloadFooter_st.checkSum_u16);
      }
      if (ok) {
        _rudderRx = rudderLocal;
        g_espNowRudderUpdate_b = true;

        dap_calculationVariables_st.syncPedalPosition_u32 =
            rudderLocal.payloadRudderState_st.pedalPosition_u16;
        dap_calculationVariables_st.syncPedalPositionRatio_fl32 =
            rudderLocal.payloadRudderState_st.pedalPositionRatio_fl32;
        dap_calculationVariables_st.syncPedalForce_N_fl32 =
            rudderLocal.payloadRudderState_st.pedalForce_N_fl32;

        uint32_t incomingSendTime =
            rudderLocal.payloadRudderState_st.sendTimestamp_ms;
        uint32_t incomingEchoTime =
            rudderLocal.payloadRudderState_st.echoTimestamp_ms;
        g_lastPartnerTimestamp_ms = incomingSendTime;
        if (incomingEchoTime > 0) {
          uint32_t now_ms = millis();
          if (now_ms >= incomingEchoTime) {
            uint32_t rtt = now_ms - incomingEchoTime;
            if (rtt < 255) {
              g_currentSyncDelay_ms = (uint8_t)(rtt / 2);
            }
          }
        }
        _rudderRxAccepted_u32++;
        logDebug("RX RudderSync accepted");
      } else {
        _rudderRxRejected_u32++;
        logDebug("RX RudderSync dropped: bad version/CRC");
      }
      return;
    }

    // Everything else must come from the provisioned bridge MAC.
    if (!isHostSender) {
      logDebug("RX dropped: sender not the provisioned bridge MAC");
      return;
    }

    DapConfig_t dap_config_espnow_recv_st;
    if (!global_dap_config_class.getConfig(&dap_config_espnow_recv_st, 0)) {
      dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 =
          s_localPedalType_u8;
    }

    if (len == sizeof(DapConfig_t)) {
      handleConfigPacket(data, dap_config_espnow_recv_st);
      return;
    }

    if (len == sizeof(DapActions_t)) {
      handleActionsPacket(data, dap_config_espnow_recv_st);
      return;
    }

    if (len == sizeof(DapActionOta_t)) {
      handleOtaPacket(data, len);
      return;
    }

    // DapWifiChannel_t and DAP_servo_config_st_t are both exactly 52 bytes
    // (6-byte header + 42-byte payload + 4-byte footer, coincidentally
    // identical for both) - length alone can't tell them apart, so whichever
    // check ran first here always won, silently swallowing every servo
    // register read/write over wireless (DapWifiChannel_t was checked
    // first, so it always "won", and handleWifiChannelPacket()'s own
    // payload-type check then rejected the payload) while the
    // WIFI_CH_CMD_SET_REQ path was never actually broken. Both structs
    // share the same PayloadHeader_t layout, so byte 2 (payloadType_u8) is
    // always safe to read here to disambiguate.
    if (len == sizeof(DapWifiChannel_t)) {
      if (data[2] == DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8) {
        handleServoConfigPacket(data);
      } else {
        handleWifiChannelPacket(data);
      }
      return;
    }

    if (len == sizeof(DapAssignmentReg_t)) {
      handleAssignmentPacket(data);
      return;
    }
  }

  void handleSent(const esp_now_send_info_t *info,
                  esp_now_send_status_t status) {
    (void)info;
    // Flow control (see sendTo() below): a send is only "in flight" until
    // this callback fires, one way or the other.
    _txInFlight_b = false;
    if (status == ESP_NOW_SEND_SUCCESS) {
      _txSuccessCount++;
    } else {
      _txFailCount++;
    }
  }

private:
  uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t _ownMac[6] = {0};
  uint8_t _hostMac[6] = {0};
  uint8_t _pedalMac[3][6] = {{0}};
  uint8_t _registeredHostMac[6] = {0};
  uint8_t _registeredPedalMac[3][6] = {{0}};
  uint8_t _currentChannel = 11;
  bool _started = false;
  bool _initialStatus = false;
  int32_t _rssi[4] = {0, 0, 0, 0}; // clutch, brake, throttle, bridge
  uint32_t _txSuccessCount = 0;
  uint32_t _txFailCount = 0;
  uint32_t _txErrCount = 0;
  uint32_t _lastTxTime = 0;
  uint32_t _lastRxTime = 0;
  bool _lastSendOk = true;
  DapRudder_t _rudderRx = {};
  DapRudder_t _rudderTx = {};
  uint8_t _rudderPartnerId_u8 = PEDAL_ID_UNKNOWN;
  uint32_t _rudderTxOk_u32 = 0;
  uint32_t _rudderTxSkipNoPartner_u32 = 0;
  uint32_t _rudderTxSkipBusy_u32 = 0;
  uint32_t _rudderRxAccepted_u32 = 0;
  uint32_t _rudderRxRejected_u32 = 0;

  // --- Unicast flow control -------------------------------------------
  // History: this repo already tried unicast once (see commits fafe7737..
  // 4bd7749d) and hit a real hardware lockup - rudder-sync packets at a
  // 2ms interval exhausted the 32 ESP-IDF TX descriptors
  // (ESP_ERR_ESPNOW_NO_MEM) because nothing paced app-level esp_now_send()
  // calls against what the WiFi driver could actually retire (unicast
  // frames cost more per-send than broadcast due to hardware ACK+retry).
  // The fix that worked, ported here: track whether a send is still
  // in-flight (cleared by handleSent(), with a 25ms safety-timeout
  // auto-recovery in case the callback is ever dropped), and back off for
  // 15ms after any ESP_ERR_ESPNOW_NO_MEM before attempting another send.
  bool _txInFlight_b = false;
  uint32_t _noMemBackoffUntil_ms = 0;
  uint32_t _txNoMemCount_u32 = 0;
  uint32_t _txBusySkipCount_u32 = 0;

  static bool isAllZero(const uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
      if (mac[i] != 0)
        return false;
    }
    return true;
  }

  esp_err_t sendTo(const uint8_t *mac, const uint8_t *data, size_t len) {
    uint32_t now = millis();
    if (now < _noMemBackoffUntil_ms) {
      _txBusySkipCount_u32++;
      return ESP_ERR_ESPNOW_NO_MEM;
    }
    if (_txInFlight_b) {
      if (now - _lastTxTime > 25) {
        // Safety-timeout recovery: the send callback never fired (dropped
        // or stalled) - don't let one lost callback wedge TX forever.
        _txInFlight_b = false;
      } else {
        _txBusySkipCount_u32++;
        return ESP_ERR_ESPNOW_INTERNAL;
      }
    }
    _txInFlight_b = true;
    _lastTxTime = now;
    esp_err_t res = esp_now_send(mac, data, len);
    if (res != ESP_OK) {
      _txInFlight_b = false;
      _txErrCount++;
      if (res == ESP_ERR_ESPNOW_NO_MEM) {
        _noMemBackoffUntil_ms = now + 15;
        _txNoMemCount_u32++;
      }
      logDebug("TX failed: %s", esp_err_to_name(res));

      // ActiveSerial->printf("TX failed: %s", esp_err_to_name(res));

      // One-shot diagnostic dump the moment sending transitions from
      // healthy to failing, to tell a heap leak (FreeHeap/MinHeap dropping)
      // apart from heap fragmentation (FreeHeap fine, LargestFreeBlock
      // small) - both present as ESP_ERR_ESPNOW_NO_MEM from esp_now_send().
      if (_lastSendOk) {
        uint32_t freeHeap = esp_get_free_heap_size();
        uint32_t minHeap = esp_get_minimum_free_heap_size();
        uint32_t largestBlock =
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
        // High-water mark of *unused* stack for the calling task (this is
        // called from espNowCommunicationTaskTx, whose stack is currently
        // sized generously at 10000 bytes) - lets us right-size it with
        // data instead of guessing.
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        ActiveSerial->printf(
            "[WirelessComm] First send failure (%s) after %u successes! "
            "FreeHeap=%u MinHeap=%u LargestFreeBlock=%u "
            "CallingTaskStackFreeBytes=%u\n",
            esp_err_to_name(res), _txSuccessCount, freeHeap, minHeap,
            largestBlock, (unsigned)stackHighWaterMark);
      }
      _lastSendOk = false;
    } else {
      _lastSendOk = true;
    }
    return res;
  }

  esp_err_t sendBroadcast(const uint8_t *data, size_t len) {
    return sendTo(_broadcastMac, data, len);
  }

  // Resolves who our rudder-sync partner pedal is, given our own role and
  // the currently-active rudder action, so sendRudderSync() knows a
  // concrete unicast target instead of relying on broadcast to reach
  // whichever sibling happens to be relevant. Returns PEDAL_ID_UNKNOWN if
  // localPedalId isn't part of the pair this rudderAct describes.
  static uint8_t resolveRudderPartnerId(uint8_t localPedalId, uint8_t rudderAct) {
    uint8_t a = PEDAL_ID_UNKNOWN, b = PEDAL_ID_UNKNOWN;
    if (rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_BRAKE ||
        rudderAct == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_BRAKE) {
      a = PEDAL_ID_THROTTLE;
      b = PEDAL_ID_BRAKE;
    } else if (rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH ||
               rudderAct == (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH) {
      a = PEDAL_ID_THROTTLE;
      b = PEDAL_ID_CLUTCH;
    } else {
      return PEDAL_ID_UNKNOWN;
    }
    if (localPedalId == a) return b;
    if (localPedalId == b) return a;
    return PEDAL_ID_UNKNOWN;
  }

  // --- Unicast peer management ------------------------------------------
  bool ensurePeer(const uint8_t *mac) {
    if (isAllZero(mac)) return false;
    if (esp_now_is_peer_exist(mac)) return true;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
  }

  // Reconciles the registered ESP-NOW peer table (host + siblings) against
  // the desired _hostMac/_pedalMac[] - called from applyMacConfig() so this
  // stays correct both at boot and whenever the host pushes an updated MAC
  // table at runtime. Idempotent: a slot whose MAC hasn't changed is left
  // alone. Self (matched via _ownMac) is skipped - a pedal never needs a
  // peer entry for its own MAC.
  void syncPeerTable() {
    if (memcmp(_hostMac, _registeredHostMac, 6) != 0) {
      if (!isAllZero(_registeredHostMac)) {
        esp_now_del_peer(_registeredHostMac);
        memset(_registeredHostMac, 0, 6);
      }
      if (!isAllZero(_hostMac) && ensurePeer(_hostMac)) {
        memcpy(_registeredHostMac, _hostMac, 6);
      }
    }
    for (int i = 0; i < 3; i++) {
      if (!isAllZero(_ownMac) && macCheck(_ownMac, _pedalMac[i])) continue;
      bool changed = memcmp(_pedalMac[i], _registeredPedalMac[i], 6) != 0;
      if (!changed) continue;
      if (!isAllZero(_registeredPedalMac[i])) {
        esp_now_del_peer(_registeredPedalMac[i]);
        memset(_registeredPedalMac[i], 0, 6);
      }
      if (!isAllZero(_pedalMac[i]) && ensurePeer(_pedalMac[i])) {
        memcpy(_registeredPedalMac[i], _pedalMac[i], 6);
      }
    }
  }

  void handleConfigPacket(const uint8_t *data,
                          DapConfig_t &dap_config_espnow_recv_st) {
    memcpy(&dap_config_espnow_recv_st, data, sizeof(DapConfig_t));

    bool structChecker = true;
    if (dap_config_espnow_recv_st.payloadHeader_st.payloadType_u8 !=
        DAP_PAYLOAD_TYPE_CONFIG_U8) {
      structChecker = false;
      g_espNowErrorCode_u8 = 101;
    }
    if (dap_config_espnow_recv_st.payloadHeader_st.version_u8 !=
        DAP_VERSION_CONFIG_U8) {
      structChecker = false;
      if (g_espNowErrorCode_u8 == 0)
        g_espNowErrorCode_u8 = 102;
    }
    uint16_t crc = checksumCalculator_u16(
        (uint8_t *)(&(dap_config_espnow_recv_st.payloadHeader_st)),
        sizeof(dap_config_espnow_recv_st.payloadHeader_st) +
            sizeof(dap_config_espnow_recv_st.payloadPedalConfig_st));
    if (crc != dap_config_espnow_recv_st.payloadFooter_st.checkSum_u16) {
      structChecker = false;
      if (g_espNowErrorCode_u8 == 0)
        g_espNowErrorCode_u8 = 103;
    }
    if (structChecker && !isPedalConfigPlausible(dap_config_espnow_recv_st)) {
      structChecker = false;
      if (g_espNowErrorCode_u8 == 0)
        g_espNowErrorCode_u8 = 104;
    }

    // Target Role Protection: once assigned, an incoming (non-EEPROM-store)
    // config must match our own role.
    if (structChecker && s_localPedalType_u8 < 3 &&
        dap_config_espnow_recv_st.payloadHeader_st.storeToEeprom_u8 == 0) {
      if (dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8 !=
          s_localPedalType_u8) {
        structChecker = false;
      }
    }

    if (!structChecker) {
      logDebug("RX Config dropped: validation failed (err=%u)",
               g_espNowErrorCode_u8);
      return;
    }

    g_lastMasterHeartbeat_ms = millis();
    configDataPackage_t configPackage_st;
    configPackage_st.config_st = dap_config_espnow_recv_st;
    if (dap_config_espnow_recv_st.payloadHeader_st.storeToEeprom_u8 == 1 ||
        s_localPedalType_u8 == PEDAL_ID_UNKNOWN) {
      s_localPedalType_u8 =
          dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8;
    }
    xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, 0);
    if (dap_config_espnow_recv_st.payloadHeader_st.storeToEeprom_u8 == 1) {
      g_configUpdateBuzzer_b = true;
    }
    logDebug("RX Config accepted");
  }

  void handleActionsPacket(const uint8_t *data,
                           const DapConfig_t &dap_config_espnow_recv_st) {
    DapActions_t dap_actions_st;
    memcpy(&dap_actions_st, data, sizeof(DapActions_t));

    uint8_t incomingTag = dap_actions_st.payloadHeader_st.pedalTag_u8;
    uint8_t myTag =
        dap_config_espnow_recv_st.payloadPedalConfig_st.pedalType_u8;
    uint8_t sysAct = dap_actions_st.payloadPedalAction_st.systemAction_u8;

    bool isAssignmentAction =
        (sysAct == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT ||
         sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0 ||
         sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1 ||
         sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2 ||
         sysAct == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP);

    // TEMP DIAGNOSTIC (unconditional, not gated behind WIRELESS_COMM_DEBUG -
    // relayed to the PC's Serial Logs via sendLogToBridge) - tracking down
    // why the config-echo request never completes over wireless (isConfigRequest)
    // and why Brake never resolves a rudder partner while Throttle does
    // (isRudderAction). Only fires for these two rare action subtypes, so it
    // stays low-volume even though this handler otherwise runs for every
    // high-frequency FFB action packet. Remove once both are confirmed fixed.
    bool isConfigRequest =
        dap_actions_st.payloadPedalAction_st.returnPedalConfig_u8 != 0;
    bool isRudderAction =
        dap_actions_st.payloadPedalAction_st.rudderAction_u8 != 0;
    if (isConfigRequest) {
      sendLogToBridge(
          "[DIAG] ConfigReq RX: incomingTag=%u myTag=%u localTag=%u typeOk=%u",
          incomingTag, myTag, s_localPedalType_u8,
          (unsigned)(dap_actions_st.payloadHeader_st.payloadType_u8 ==
                     DAP_PAYLOAD_TYPE_ACTION_U8));
    }
    if (isRudderAction) {
      sendLogToBridge(
          "[DIAG] RudderAct RX: incomingTag=%u myTag=%u localTag=%u "
          "rudderAct=%u typeOk=%u",
          incomingTag, myTag, s_localPedalType_u8,
          dap_actions_st.payloadPedalAction_st.rudderAction_u8,
          (unsigned)(dap_actions_st.payloadHeader_st.payloadType_u8 ==
                     DAP_PAYLOAD_TYPE_ACTION_U8));
    }

    if (dap_actions_st.payloadHeader_st.payloadType_u8 !=
            DAP_PAYLOAD_TYPE_ACTION_U8 ||
        !(isAssignmentAction || incomingTag == myTag ||
          incomingTag == s_localPedalType_u8)) {
      if (isConfigRequest) {
        sendLogToBridge("[DIAG] ConfigReq DROPPED at type/tag gate");
      }
      if (isRudderAction) {
        sendLogToBridge("[DIAG] RudderAct DROPPED at type/tag gate");
      }
      return;
    }

    if (dap_actions_st.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8) {
      if (g_espNowErrorCode_u8 == 0)
        g_espNowErrorCode_u8 = 112;
      if (isConfigRequest) {
        sendLogToBridge("[DIAG] ConfigReq DROPPED: bad version");
      }
      if (isRudderAction) {
        sendLogToBridge("[DIAG] RudderAct DROPPED: bad version");
      }
      logDebug("RX Actions dropped: bad version");
      return;
    }
    uint16_t crc = checksumCalculator_u16(
        (uint8_t *)(&(dap_actions_st.payloadHeader_st)),
        sizeof(dap_actions_st.payloadHeader_st) +
            sizeof(dap_actions_st.payloadPedalAction_st));
    if (crc != dap_actions_st.payloadFooter_st.checkSum_u16) {
      if (g_espNowErrorCode_u8 == 0)
        g_espNowErrorCode_u8 = 113;
      if (isConfigRequest) {
        sendLogToBridge("[DIAG] ConfigReq DROPPED: bad CRC");
      }
      if (isRudderAction) {
        sendLogToBridge("[DIAG] RudderAct DROPPED: bad CRC");
      }
      logDebug("RX Actions dropped: bad CRC");
      return;
    }

    g_lastMasterHeartbeat_ms = millis();

    if (sysAct == (uint8_t)PedalSystemAction::CLEAR_ASSIGNMENT) {
      g_assignmentClear_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_0) {
      g_newAssignedRole_u8 = 0;
      g_assignmentUpdate_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_1) {
      g_newAssignedRole_u8 = 1;
      g_assignmentUpdate_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::SET_ASSIGNMENT_2) {
      g_newAssignedRole_u8 = 2;
      g_assignmentUpdate_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::ASSIGNMENT_CHECK_BEEP) {
      buzzerBeepAction_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::PEDAL_RESTART) {
      g_espNowRestart_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::ENABLE_OTA) {
      g_espNowOtaEnable_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE) {
      g_espNowBootIntoDownloadMode_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO) {
      g_printPedalInfo_b = true;
    }
    if (sysAct == (uint8_t)PedalSystemAction::WAKEUP_PEDAL) {
      if (g_pedalOperationalState_u8 ==
          (uint8_t)PEDAL_STATE_STANDBY_WAITING_FOR_WAKEUP_E) {
        g_pedalOperationalState_u8 = (uint8_t)PEDAL_STATE_HOMING_E;
      }
    }

    if (dap_actions_st.payloadPedalAction_st.triggerAbs_u8 > 0) {
      absOscillation.trigger();
      if (dap_actions_st.payloadPedalAction_st.triggerAbs_u8 > 1) {
        dap_calculationVariables_st.trackCondition_u8 =
            dap_actions_st.payloadPedalAction_st.triggerAbs_u8 - 1;
      } else {
        dap_calculationVariables_st.trackCondition_u8 =
            dap_actions_st.payloadPedalAction_st.triggerAbs_u8 = 0;
      }
    }
    g_rpmOscillation_st.rpmValue_fl32 =
        dap_actions_st.payloadPedalAction_st.rpm_u8;
    g_gForceEffect_st.gValue_fl32 =
        dap_actions_st.payloadPedalAction_st.gValue_u8 - 128;
    if (dap_actions_st.payloadPedalAction_st.wheelSlip_u8) {
      g_wsOscillation_st.trigger();
    }
    if (dap_calculationVariables_st.rudderStatus_b == false) {
      g_roadImpactEffect_st.roadImpactValue_u8 =
          dap_actions_st.payloadPedalAction_st.impactValue_u8;
    } else {
      g_rudderGForce_st.gValue_u8 =
          dap_actions_st.payloadPedalAction_st.impactValue_u8;
    }
    if (dap_actions_st.payloadPedalAction_st.triggerCv1_u8)
      g_customVibration1_st.trigger();
    if (dap_actions_st.payloadPedalAction_st.triggerCv2_u8)
      g_customVibration2_st.trigger();
    if (dap_actions_st.payloadPedalAction_st.triggerCv3_u8)
      g_customVibration3_st.trigger();
    if (dap_actions_st.payloadPedalAction_st.triggerCv4_u8)
      g_customVibration4_st.trigger();
    if (dap_actions_st.payloadPedalAction_st.returnPedalConfig_u8) {
      g_espNowConfigRequest_b = true;
      if (isConfigRequest) {
        sendLogToBridge("[DIAG] ConfigReq ACCEPTED, g_espNowConfigRequest_b set");
      }
    }

    // Rudder mode select - also resolves who sendRudderSync() should unicast
    // to (see resolveRudderPartnerId()), since unicast needs a concrete
    // target MAC instead of relying on broadcast to reach whichever sibling
    // happens to be relevant.
    uint8_t rudderAct = dap_actions_st.payloadPedalAction_st.rudderAction_u8;
    // TEMP DIAGNOSTIC: edge-triggered (only logs when the resolved partner
    // actually changes), so this stays low-volume despite handleActionsPacket
    // running for every high-frequency FFB action packet. Confirms whether
    // the rudder-enable action even reaches this pedal and what partner it
    // resolves to. Remove once rudder sync is confirmed working again.
    uint8_t prevRudderPartner = _rudderPartnerId_u8;
    if (rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_BRAKE ||
        rudderAct == (uint8_t)RudderAction::RUDDER_THROTTLE_AND_CLUTCH) {
      g_getRudderAction_b = true;
      dap_calculationVariables_st.rudderStatus_b = true;
      dap_calculationVariables_st.helicopterRudderStatus_b = false;
      _rudderPartnerId_u8 = resolveRudderPartnerId(s_localPedalType_u8, rudderAct);
    } else if (rudderAct ==
                   (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_BRAKE ||
               rudderAct ==
                   (uint8_t)RudderAction::HELIRUDDER_THROTTLE_AND_CLUTCH) {
      g_getHeliRudderAction_b = true;
      dap_calculationVariables_st.helicopterRudderStatus_b = true;
      dap_calculationVariables_st.rudderStatus_b = false;
      _rudderPartnerId_u8 = resolveRudderPartnerId(s_localPedalType_u8, rudderAct);
    } else if (rudderAct == (uint8_t)RudderAction::RUDDER_CLEAR_RUDDER_STATUS) {
      dap_calculationVariables_st.rudderStatus_b = false;
      dap_calculationVariables_st.helicopterRudderStatus_b = false;
      dap_calculationVariables_st.rudderBrakeStatus_b = false;
      moveSlowlyToPosition_b = true;
      ResetRudderStrategyState();
      _rudderPartnerId_u8 = PEDAL_ID_UNKNOWN;
    }
    if (_rudderPartnerId_u8 != prevRudderPartner) {
      sendLogToBridge(
          "[DIAG] Rudder partner changed: %u -> %u (myRole=%u, rudderAct=%u)",
          prevRudderPartner, _rudderPartnerId_u8, s_localPedalType_u8, rudderAct);
    }

    uint8_t brakeAct =
        dap_actions_st.payloadPedalAction_st.rudderBrakeAction_u8;
    if (brakeAct == 1) {
      g_getRudderAction_b = true;
      if (dap_calculationVariables_st.rudderBrakeStatus_b == false &&
          (dap_calculationVariables_st.rudderStatus_b == true ||
           dap_calculationVariables_st.helicopterRudderStatus_b == true)) {
        dap_calculationVariables_st.rudderBrakeStatus_b = true;
      } else {
        dap_calculationVariables_st.rudderBrakeStatus_b = false;
      }
    } else if (brakeAct == 2) {
      g_getRudderAction_b = true;
      if (dap_calculationVariables_st.rudderStatus_b == true ||
          dap_calculationVariables_st.helicopterRudderStatus_b == true) {
        dap_calculationVariables_st.rudderBrakeStatus_b = true;
      }
    } else if (brakeAct == 3) {
      g_getRudderAction_b = true;
      dap_calculationVariables_st.rudderBrakeStatus_b = false;
    }
    logDebug("RX Actions accepted: sysAct=%u", sysAct);
  }

  void handleOtaPacket(const uint8_t *data, int len) {
    (void)len;
    DapActionOta_t otaLocal;
    memcpy(&otaLocal, data, sizeof(DapActionOta_t));
    if (otaLocal.payloadHeader_st.payloadType_u8 !=
            DAP_PAYLOAD_TYPE_ACTION_OTA_U8 ||
        otaLocal.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8) {
      logDebug("RX Ota dropped: bad type/version");
      return;
    }
    uint16_t crc = checksumCalculator_u16(
        (uint8_t *)(&(otaLocal.payloadHeader_st)),
        sizeof(otaLocal.payloadHeader_st) + sizeof(otaLocal.payloadOtaInfo_st));
    if (crc != otaLocal.payloadFooter_st.checkSum_u16) {
      logDebug("RX Ota dropped: bad CRC");
      return;
    }
    dap_action_ota_st = otaLocal;
    g_otaUpdateAction_b = true;
    logDebug("RX Ota accepted");
  }

  void handleWifiChannelPacket(const uint8_t *data) {
    DapWifiChannel_t wifiChPacket;
    memcpy(&wifiChPacket, data, sizeof(DapWifiChannel_t));
    if (wifiChPacket.payloadHeader_st.payloadType_u8 !=
            DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8 ||
        wifiChPacket.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8) {
      return;
    }
    uint16_t crc =
        checksumCalculator_u16((uint8_t *)(&(wifiChPacket.payloadHeader_st)),
                               sizeof(wifiChPacket.payloadHeader_st) +
                                   sizeof(wifiChPacket.payloadWifiChannel_st));
    if (crc != wifiChPacket.payloadFooter_st.checkSum_u16) {
      return;
    }
    if (wifiChPacket.payloadWifiChannel_st.command_u8 != WIFI_CH_CMD_SET_REQ) {
      return;
    }
    g_lastMasterHeartbeat_ms = millis();
    uint8_t newCh = wifiChPacket.payloadWifiChannel_st.currentChannel_u8;
    if (newCh < 1 || newCh > 14) {
      newCh = wifiChPacket.payloadWifiChannel_st.recommendedChannel_u8;
    }
    if (newCh >= 1 && newCh <= 14) {
      _currentChannel = newCh;
      esp_wifi_set_channel(newCh, WIFI_SECOND_CHAN_NONE);
      g_saveWifiChannelDeferred_b = true;
      ActiveSerial->printf("Switched Wi-Fi channel to %d via ESP-NOW SET_REQ\n",
                           newCh);
    }
    logDebug("RX WifiChannel SET_REQ -> %u", newCh);
  }

  void handleAssignmentPacket(const uint8_t *data) {
    DapAssignmentReg_t incomingReg;
    memcpy(&incomingReg, data, sizeof(DapAssignmentReg_t));
    if (incomingReg.payloadType_u8 != DAP_PAYLOAD_TYPE_ASSIGNMENT_U8 ||
        incomingReg.magicKey_u8 != ESPNOW_ASSIGNMENT_MAGIC_KEY_U8) {
      return;
    }
    uint16_t crc =
        checksumCalculator_u16((uint8_t *)(&incomingReg),
                               sizeof(DapAssignmentReg_t) - sizeof(uint16_t));
    if (crc != incomingReg.crc_u16) {
      logDebug("RX Assignment dropped: bad CRC");
      return;
    }
    g_lastMasterHeartbeat_ms = millis();
    for (int p = 0; p < 3; p++) {
      if (incomingReg.pairStatus_au8[p] == 1) {
        memcpy(_pedalMac[p], incomingReg.pairedMac_aau8[p], 6);
      }
    }
    if (incomingReg.pairStatus_au8[3] == 1) {
      memcpy(_hostMac, incomingReg.pairedMac_aau8[3], 6);
    }

    if (incomingReg.deviceId_u8 < 3 &&
        macCheck(_ownMac,
                 incomingReg.pairedMac_aau8[incomingReg.deviceId_u8])) {
      if (s_localPedalType_u8 != incomingReg.deviceId_u8) {
        ActiveSerial->printf("[ESP-NOW] Pairing table sync: Bridge assigned "
                             "this pedal to role %u!\n",
                             incomingReg.deviceId_u8);
        g_newAssignedRole_u8 = incomingReg.deviceId_u8;
        g_assignmentUpdate_b = true;
      }
    }
    logDebug("RX Assignment accepted");
  }

  void handleServoConfigPacket(const uint8_t *data) {
    DAP_servo_config_st received_servo_config;
    memcpy(&received_servo_config, data, sizeof(DAP_servo_config_st));

    // TEMP DIAGNOSTIC (unconditional - "Load From Servo" is a rare,
    // user-triggered action, never continuous, so no volume concern here).
    // Tracking down why servo register exchange doesn't complete over
    // wireless despite working over direct USB. Remove once confirmed fixed.
    sendLogToBridge(
        "[DIAG] ServoConfig RX: tag=%u localTag=%u typeOk=%u versionOk=%u",
        received_servo_config.payloadHeader_st.pedalTag_u8, s_localPedalType_u8,
        (unsigned)(received_servo_config.payloadHeader_st.payloadType_u8 ==
                   DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8),
        (unsigned)(received_servo_config.payloadHeader_st.version_u8 ==
                   DAP_VERSION_CONFIG_U8));

    if (received_servo_config.payloadHeader_st.payloadType_u8 !=
            DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8 ||
        received_servo_config.payloadHeader_st.version_u8 !=
            DAP_VERSION_CONFIG_U8) {
      return;
    }
    uint16_t crc = checksumCalculator_u16(
        (uint8_t *)(&(received_servo_config.payloadHeader_st)),
        sizeof(received_servo_config.payloadHeader_st) +
            sizeof(received_servo_config.payloadServoConfig_st));
    if (crc != received_servo_config.payloadFooter_st.checkSum_u16) {
      sendLogToBridge("[DIAG] ServoConfig DROPPED: bad CRC");
      logDebug("RX ServoConfig dropped: bad CRC");
      return;
    }
    if (s_localPedalType_u8 < 3 &&
        received_servo_config.payloadHeader_st.pedalTag_u8 !=
            s_localPedalType_u8) {
      sendLogToBridge("[DIAG] ServoConfig DROPPED: role tag mismatch");
      logDebug("RX ServoConfig dropped: role tag mismatch");
      return;
    }
    if (s_servoConfigRxQueue != NULL) {
      xQueueSend(s_servoConfigRxQueue, &received_servo_config, (TickType_t)0);
    }
    sendLogToBridge("[DIAG] ServoConfig ACCEPTED, queued for Modbus task");
    logDebug("RX ServoConfig accepted");
  }

#ifdef WIRELESS_COMM_DEBUG
  void logDebug(const char *fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ActiveSerial->print("[WirelessComm] ");
    ActiveSerial->println(buf);
  }
#else
  inline void logDebug(const char *fmt, ...) { (void)fmt; }
#endif
};

extern WirelessCommunicationPedal wirelessComm;
WirelessCommunicationPedal wirelessComm;

inline void sWirelessCommRecvTrampoline(const esp_now_recv_info_t *info,
                                        const uint8_t *data, int len) {
  wirelessComm.handleRecv(info, data, len);
}

inline void sWirelessCommSentTrampoline(const esp_now_send_info_t *info,
                                        esp_now_send_status_t status) {
  wirelessComm.handleSent(info, status);
}

#else
static const bool IS_ESPNOW_ENABLED = false;
static uint8_t g_espNowErrorCode_u8 = 0;
static bool g_espNowRudderUpdate_b = false;
static bool g_espNowRestart_b = false;
static bool g_espNowOtaEnable_b = false;
static bool g_printPedalInfo_b = false;
static bool g_configUpdateBuzzer_b = false;
static unsigned long g_rudderInitializedTime_u32 = 0;
static uint32_t g_lastPartnerTimestamp_ms = 0;
static uint8_t g_currentSyncDelay_ms = 0;
static volatile bool g_rudderInitializing_b = false;
static volatile bool g_rudderDeinitializing_b = false;
static volatile bool g_heliRudderInitializing_b = false;
static volatile bool g_heliRudderDeinitializing_b = false;
static bool g_espNowBootIntoDownloadMode_b = false;
static bool g_getRudderAction_b = false;
static bool g_getHeliRudderAction_b = false;
static bool g_otaUpdateAction_b = false;

class WirelessCommunicationPedal {
public:
  void begin(const DapMacAddresses_t &) {}
  void applyMacConfig(const DapMacAddresses_t &) {}
  int deinit() { return 0; }
  bool isStarted() const { return false; }
  bool isInitialStatusDone() const { return false; }
  uint8_t getChannel() const { return 11; }
  uint32_t getTxSuccessCount() const { return 0; }
  uint32_t getTxFailCount() const { return 0; }
  uint32_t getTxErrCount() const { return 0; }
  void sendLogToBridge(const char *, ...) {}
};
static WirelessCommunicationPedal wirelessComm;
#endif
