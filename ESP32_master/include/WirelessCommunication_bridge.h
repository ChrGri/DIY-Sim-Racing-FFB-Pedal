#pragma once
#include <WiFi.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "esp_now.h"
#include "ESPNowW.h"
#include "Main.h"

#define ESPNOW_LOG_MAGIC_KEY_U8 0x99
#define ESPNOW_LOG_MAGIC_KEY_2_U8 0x97
#define ESPNOW_ASSIGNMENT_MAGIC_KEY_U8 0x99

// Uncomment to enable verbose wireless transport debug logging, mirrored to
// both ActiveSerial and (when built with USB_JOYSTICK) the USB HID text log.
// #define WIRELESS_COMM_DEBUG

#define WIFI_CH_EEPROM_MAGIC 0xA6
#ifndef EEPROM_offset
#define EEPROM_offset 15
#endif
#define WIFI_CH_EEPROM_OFFSET 60
struct WifiChannelConfig_t {
  uint8_t magic_u8;
  uint8_t channel_u8;
  uint8_t checksum_u8;
};

int g_rssiDisplay_i32;
bool g_espNowNoDevice_b = false;
bool g_updateBasicState_ab[3] = {false, false, false};
bool g_updateExtendState_ab[3] = {false, false, false};
bool g_pedalOtaAction_b = false;
uint16_t g_joystickValue_au16[] = {0, 0, 0};
uint16_t g_joystickThrottleValueFromPedal_u16 = 0;
uint16_t g_joystickValueOriginal_au16[] = {0, 0, 0};
unsigned long g_pedalLastUpdate_au32[3] = {1, 1, 1};
bool g_espNowRequestConfig_ab[3] = {false, false, false};
bool g_espNowError_ab[3] = {false, false, false};
uint16_t g_pedalThrottleValue_u16 = 0;
uint16_t g_pedalBrakeValue_u16 = 0;
uint16_t g_pedalClutchValue_u16 = 0;
uint8_t g_pedalStatus_u8 = 0;
QueueHandle_t g_messageQueueHandle_pv;

extern DAP_servo_config_st_t dap_servo_config_response_st[3];
extern bool send_servo_config_to_host[3];

// This is the live pedal-assignment/pairing persistence registry (EEPROM
// offset EEPROM_offset) used by clearPedalAssignmentAction/
// pushPedalAssignmentAction/handleWifiSetChannelRequest - not part of the
// dead ESPNow_Pairing_function button-pairing flow, so it is kept as-is.
typedef struct EspPairingReg_t {
  uint8_t pairStatus_au8[4];
  uint8_t pairMac_aau8[4][6];
} EspPairingReg_t;
EspPairingReg_t g_espPairingReg_st;

inline bool macCheck(const uint8_t *Mac_A, const uint8_t *Mac_B) {
  return memcmp(Mac_A, Mac_B, 6) == 0;
}

inline bool isAllZeroMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0) return false;
  }
  return true;
}

inline DapMacAddresses_t loadMacAddressesFromEeprom() {
  DapMacAddresses_t macCfg;
  EEPROM.get(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  if (macCfg.payloadHeader_st.payloadType_u8 == DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8 &&
      macCfg.payloadHeader_st.version_u8 == DAP_VERSION_MAC_ADDRESSES_U8) {
    uint16_t crc = checksumCalculator((uint8_t*)(&(macCfg.payloadHeader_st)),
                                      sizeof(macCfg.payloadHeader_st) + sizeof(macCfg.payloadMacAddresses_st));
    if (crc == macCfg.payloadFooter_st.checkSum_u16) {
      WiFi.macAddress(macCfg.payloadMacAddresses_st.ownMacAddress_au8);
      macCfg.payloadMacAddresses_st.ownNodeType_u8 = 3; // Bridge
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
  macCfg.payloadMacAddresses_st.ownNodeType_u8 = 3; // Bridge
  return macCfg;
}

inline void storeMacAddressesToEeprom(DapMacAddresses_t &macCfg) {
  macCfg.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  macCfg.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  macCfg.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_MAC_ADDRESSES_U8;
  macCfg.payloadHeader_st.version_u8 = DAP_VERSION_MAC_ADDRESSES_U8;
  macCfg.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  macCfg.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
  macCfg.payloadFooter_st.checkSum_u16 = checksumCalculator((uint8_t*)(&(macCfg.payloadHeader_st)),
                                                            sizeof(macCfg.payloadHeader_st) + sizeof(macCfg.payloadMacAddresses_st));
  EEPROM.put(DAP_MAC_ADDRESSES_EEPROM_OFFSET_U32, macCfg);
  EEPROM.commit();
}

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
  if (ch < 1 || ch > 14) return;
  WifiChannelConfig_t cfg;
  cfg.magic_u8 = WIFI_CH_EEPROM_MAGIC;
  cfg.channel_u8 = ch;
  cfg.checksum_u8 = (uint8_t)(WIFI_CH_EEPROM_MAGIC ^ ch);
  EEPROM.put(WIFI_CH_EEPROM_OFFSET, cfg);
  EEPROM.commit();
}

// Forward declarations - ESP-NOW's C callback API needs plain function
// pointers, not member function pointers, so these small trampolines just
// forward into the wirelessComm singleton (defined below).
void sWirelessCommRecvTrampoline(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void sWirelessCommSentTrampoline(const esp_now_send_info_t *info, esp_now_send_status_t status);

// =========================================================================
// WirelessCommunicationBridge
//
// Unicast: every send targets a specific pedal's registered MAC peer via
// sendTo(), which is the sole esp_now_send() call site and implements the
// flow-control this repo already learned it needs the hard way - see the
// history note on sendTo() below. The old broadcast peer/sendBroadcast()
// path is kept only as a fallback for anything not yet converted; nothing
// in this file's converted send methods uses it. Sender slot for an
// incoming telemetry/config/servo-config packet is derived strictly from
// which configured pedal MAC sent it (never trusted from the packet's own
// self-reported tag), and unrecognized senders are dropped (no
// auto-discovery: a pedal's MAC must already be provisioned via
// DapMacAddresses_t before its traffic is accepted).
// =========================================================================
class WirelessCommunicationBridge {
public:
  void begin(const DapMacAddresses_t &macCfg) {
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(false, false);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    ActiveSerial->println("[L]Initializing Wifi.");
    delay(1000);
    WiFi.macAddress(_ownMac);
    ActiveSerial->printf("[L]Bridge Factory Hardware Mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
                         _ownMac[0], _ownMac[1], _ownMac[2], _ownMac[3], _ownMac[4], _ownMac[5]);

    ActiveSerial->println("[L]Initializing ESP-NOW");
    ESPNow.init();
    delay(1000);

    #ifdef Using_Board_ESP32
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
    #endif
    #ifdef Using_Board_ESP32S3
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_11M_L);
      #ifdef LOW_TX_POWER
      esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
      ActiveSerial->println("[L]Setting Wifi strength to 8.5dbm ");
      #endif
    #endif

    applyMacConfig(macCfg);
    for (int i = 0; i < 3; i++) {
      ActiveSerial->printf("[L]Configured Pedal #%d Mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
                           i, _pedalMac[i][0], _pedalMac[i][1], _pedalMac[i][2],
                           _pedalMac[i][3], _pedalMac[i][4], _pedalMac[i][5]);
#ifdef USB_JOYSTICK
      tinyusbJoystick_.printf("[L]Configured Pedal #%d Mac: %02X:%02X:%02X:%02X:%02X:%02X",
                           i, _pedalMac[i][0], _pedalMac[i][1], _pedalMac[i][2],
                           _pedalMac[i][3], _pedalMac[i][4], _pedalMac[i][5]);
#endif
    }

    bool peerOk = true;
    if (!esp_now_is_peer_exist(_broadcastMac)) {
      esp_now_peer_info_t broadcastPeer = {};
      memcpy(broadcastPeer.peer_addr, _broadcastMac, 6);
      broadcastPeer.channel = 0;
      broadcastPeer.ifidx = WIFI_IF_STA;
      broadcastPeer.encrypt = false;
      if (esp_now_add_peer(&broadcastPeer) != ESP_OK) {
        peerOk = false;
      }
    }
    if (peerOk) {
      ActiveSerial->println("[L]Peers added successfully.");
    #ifdef USB_JOYSTICK
      tinyusbJoystick_.printf("[L]Peers added successfully.\n");
    #endif
    }

    uint8_t hwChan = 0;
    wifi_second_chan_t secChan;
    esp_wifi_get_channel(&hwChan, &secChan);
#ifdef USB_JOYSTICK
    tinyusbJoystick_.printf("[L]Bridge HW MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                            _ownMac[0], _ownMac[1], _ownMac[2], _ownMac[3], _ownMac[4], _ownMac[5]);
    tinyusbJoystick_.printf("[L]Bridge Radio HW-Channel: %d (cfg: %d)\n", hwChan, _currentChannel);
#endif

    ESPNow.reg_recv_cb(sWirelessCommRecvTrampoline);
    ESPNow.reg_send_cb(sWirelessCommSentTrampoline);
    ActiveSerial->printf("[L]ESPNow Channel: %d\n", _currentChannel);
#ifdef USB_JOYSTICK
    tinyusbJoystick_.printf("[L]ESPNow Channel: %d\n", _currentChannel);
#endif
    _started = true;
    ActiveSerial->println("[L]ESPNow Initialized");
  }

  // Updates channel + trusted pedal MAC table only (no WiFi/ESP-NOW re-init)
  // - used both by begin() and whenever the host pushes an updated MAC
  // table at runtime over serial/HID.
  void applyMacConfig(const DapMacAddresses_t &macCfg) {
    uint8_t ch = macCfg.payloadMacAddresses_st.wifiChannel_u8;
    if (ch >= 1 && ch <= 13) {
      _currentChannel = ch;
    }
    esp_wifi_set_channel(_currentChannel, WIFI_SECOND_CHAN_NONE);
    for (int i = 0; i < 3; i++) {
      memcpy(_pedalMac[i], macCfg.payloadMacAddresses_st.macAddress_aau8[i], 6);
    }
    syncPeerTable();
  }

  esp_err_t deinit() {
    esp_err_t result = esp_now_deinit();
    _started = false;
    return result;
  }

  bool isStarted() const { return _started; }
  const uint8_t *getOwnMac() const { return _ownMac; }
  const uint8_t *getPedalMac(uint8_t idx) const { return _pedalMac[idx < 3 ? idx : 0]; }
  void setPedalMac(uint8_t idx, const uint8_t *mac) {
    if (idx < 3) memcpy(_pedalMac[idx], mac, 6);
  }
  uint8_t getChannel() const { return _currentChannel; }
  void setChannel(uint8_t ch) { _currentChannel = ch; }
  bool isPedalWirelessSyncEnabled(uint8_t idx) const { return idx < 3 ? _pedalWirelessSyncEnabled[idx] : false; }
  void setPedalWirelessSyncEnabled(uint8_t idx, bool enabled) {
    if (idx < 3) _pedalWirelessSyncEnabled[idx] = enabled;
  }
  const int32_t *getRssiArray() const { return _rssi; }
  int32_t getRssi(uint8_t idx) const { return idx < 3 ? _rssi[idx] : 0; }
  void setRssi(uint8_t idx, int32_t value) { if (idx < 3) _rssi[idx] = value; }
  uint32_t getTxSuccessCount() const { return _txSuccessCount; }
  uint32_t getTxFailCount() const { return _txFailCount; }
  uint32_t getTxErrCount() const { return _txErrCount; }
  uint32_t getTxNoMemCount() const { return _txNoMemCount_u32; }
  uint32_t getTxBusySkipCount() const { return _txBusySkipCount_u32; }
  bool isTxBusy() const { return _txInFlight_b || millis() < _noMemBackoffUntil_ms; }

  esp_err_t sendBroadcast(const uint8_t *data, size_t len) {
    return sendTo(_broadcastMac, data, len);
  }

  // Fire-and-forget reliability helper for one-shot commands targeting a
  // SPECIFIC already-known peer (assignment change, etc.): unicast frames
  // get hardware ACK/retry, but a single attempt can still be dropped if
  // the peer briefly missed it, so send it a few times with a short delay
  // instead of waiting on any application-level acknowledgement.
  void sendUnicastRetry(const uint8_t *targetMac, const uint8_t *data, size_t len, int times, uint32_t delayMs) {
    if (isAllZeroMac(targetMac)) return;
    for (int i = 0; i < times; i++) {
      sendTo(targetMac, data, len);
      delay(delayMs);
    }
  }

  // Same as sendUnicastRetry, but for the rare case a packet genuinely must
  // reach every currently-known pedal (e.g. a wifi channel change - every
  // pedal has to move channel together, and you don't know in advance
  // which ones are actually powered on and listening). One round = one
  // unicast attempt to each known pedal, then a single delay before the
  // next round (not per-pedal), so total time stays bounded.
  void sendToAllKnownPedalsRetry(const uint8_t *data, size_t len, int times, uint32_t delayMs) {
    for (int i = 0; i < times; i++) {
      for (int p = 0; p < 3; p++) {
        if (!isAllZeroMac(_pedalMac[p])) {
          sendTo(_pedalMac[p], data, len);
        }
      }
      delay(delayMs);
    }
  }

  esp_err_t sendConfigToPedal(uint8_t pedalIdx, const DapConfig_t &pkt) {
    if (pedalIdx >= 3 || isAllZeroMac(_pedalMac[pedalIdx])) return ESP_ERR_INVALID_ARG;
    logDebug("TX Config to pedal #%u", pedalIdx);
    return sendTo(_pedalMac[pedalIdx], (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendActionToPedal(uint8_t pedalIdx, const DapActions_t &pkt) {
    if (pedalIdx >= 3 || isAllZeroMac(_pedalMac[pedalIdx])) return ESP_ERR_INVALID_ARG;
    logDebug("TX Action to pedal #%u", pedalIdx);
    return sendTo(_pedalMac[pedalIdx], (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendServoConfigToPedal(uint8_t pedalIdx, const DAP_servo_config_st_t &pkt) {
    if (pedalIdx >= 3 || isAllZeroMac(_pedalMac[pedalIdx])) return ESP_ERR_INVALID_ARG;
    logDebug("TX ServoConfig to pedal #%u", pedalIdx);
    return sendTo(_pedalMac[pedalIdx], (const uint8_t *)&pkt, sizeof(pkt));
  }

  esp_err_t sendOtaToPedal(uint8_t pedalIdx, const DapActionOta_t &pkt) {
    if (pedalIdx >= 3 || isAllZeroMac(_pedalMac[pedalIdx])) return ESP_ERR_INVALID_ARG;
    logDebug("TX Ota to pedal #%u", pedalIdx);
    return sendTo(_pedalMac[pedalIdx], (const uint8_t *)&pkt, sizeof(pkt));
  }

  // pkt.deviceId_u8 says which pedal this particular assignment-sync packet
  // is "about" - callers (see syncPairingTableToPedals() in Main.cpp) already
  // loop over all pedals and set deviceId_u8 per call before invoking this,
  // so this must send to exactly that one pedal, not fan out itself (that
  // would both triple-send and attach the wrong deviceId_u8 to the wrong
  // pedal).
  esp_err_t sendAssignmentSync(const DapAssignmentReg_t &pkt) {
    if (pkt.deviceId_u8 >= 3 || isAllZeroMac(_pedalMac[pkt.deviceId_u8])) return ESP_ERR_INVALID_ARG;
    logDebug("TX AssignmentSync to pedal #%u", pkt.deviceId_u8);
    return sendTo(_pedalMac[pkt.deviceId_u8], (const uint8_t *)&pkt, sizeof(pkt));
  }

  void handleRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    static uint32_t s_rxCount = 0;
    static uint32_t lastRxDiagTime = 0;
    if (info == NULL || info->src_addr == NULL || data == NULL || len <= 0) {
      return;
    }
    s_rxCount++;
    if (millis() - lastRxDiagTime > 3000) {
      lastRxDiagTime = millis();
//       ActiveSerial->printf("[ESPNOW RX] Packet len=%d from %02X:%02X:%02X:%02X:%02X:%02X (ch=%d)\n",
//                            len,
//                            info->src_addr[0], info->src_addr[1], info->src_addr[2],
//                            info->src_addr[3], info->src_addr[4], info->src_addr[5],
//                            _currentChannel);
// #ifdef USB_JOYSTICK
//       tinyusbJoystick_.printf("[ESPNOW RX] Count=%u, len=%d from %02X:%02X:%02X:%02X:%02X:%02X (ch=%d)\n",
//                               s_rxCount, len,
//                               info->src_addr[0], info->src_addr[1], info->src_addr[2],
//                               info->src_addr[3], info->src_addr[4], info->src_addr[5],
//                               _currentChannel);
// #endif
    }

    // Log packets are self-identified by magic bytes and accepted from any
    // configured pedal MAC (no per-type struct validation, matches today).
    bool isLogPacket = (data[0] == DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8 &&
                        data[1] == ESPNOW_LOG_MAGIC_KEY_U8 &&
                        data[2] == ESPNOW_LOG_MAGIC_KEY_2_U8);

    // MAC allow-list first, unconditionally: the matched index IS the pedal
    // slot - never trust a self-reported tag from the payload for routing.
    int matchedSlot = -1;
    for (int p = 0; p < 3; p++) {
      if (macCheck(info->src_addr, _pedalMac[p])) {
        matchedSlot = p;
        if (info->rx_ctrl != NULL) {
          _rssi[p] = info->rx_ctrl->rssi;
          g_rssiDisplay_i32 = _rssi[p];
        }
        break;
      }
    }

    if (matchedSlot < 0 && !isLogPacket) {
      logDebug("RX dropped: sender not a provisioned pedal MAC");
      return;
    }

    if (isLogPacket) {
      handleLogPacket(data, len);
      return;
    }

    if (len == sizeof(DapStateBasic_t)) {
      handleStateBasicPacket(data, (uint8_t)matchedSlot);
      return;
    }
    if (len == sizeof(DapStateExtended_t)) {
      handleStateExtendedPacket(data, (uint8_t)matchedSlot);
      return;
    }
    if (len == sizeof(DapConfig_t)) {
      handleConfigEchoPacket(data, (uint8_t)matchedSlot);
      return;
    }
    if (len == sizeof(DAP_servo_config_st_t)) {
      handleServoConfigPacket(data, (uint8_t)matchedSlot);
      return;
    }
  }

  void handleSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
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
  uint8_t _pedalMac[3][6] = {{0}};
  uint8_t _registeredPedalMac[3][6] = {{0}};
  uint8_t _currentChannel = 11;
  bool _started = false;
  bool _pedalWirelessSyncEnabled[3] = {true, true, true};
  int32_t _rssi[3] = {0, 0, 0};
  uint32_t _txSuccessCount = 0;
  uint32_t _txFailCount = 0;
  uint32_t _txErrCount = 0;

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
  uint32_t _lastSendTime_u32 = 0;
  uint32_t _noMemBackoffUntil_ms = 0;
  uint32_t _txNoMemCount_u32 = 0;
  uint32_t _txBusySkipCount_u32 = 0;

  esp_err_t sendTo(const uint8_t *mac, const uint8_t *data, size_t len) {
    uint32_t now = millis();
    if (now < _noMemBackoffUntil_ms) {
      _txBusySkipCount_u32++;
      return ESP_ERR_ESPNOW_NO_MEM;
    }
    if (_txInFlight_b) {
      if (now - _lastSendTime_u32 > 25) {
        // Safety-timeout recovery: the send callback never fired (dropped
        // or stalled) - don't let one lost callback wedge TX forever.
        _txInFlight_b = false;
      } else {
        _txBusySkipCount_u32++;
        return ESP_ERR_ESPNOW_INTERNAL;
      }
    }
    _txInFlight_b = true;
    _lastSendTime_u32 = now;
    esp_err_t res = esp_now_send(mac, data, len);
    if (res != ESP_OK) {
      _txInFlight_b = false;
      _txErrCount++;
      if (res == ESP_ERR_ESPNOW_NO_MEM) {
        _noMemBackoffUntil_ms = now + 15;
        _txNoMemCount_u32++;
      }
      logDebug("TX failed: %s", esp_err_to_name(res));
    }
    return res;
  }

  // --- Unicast peer management ------------------------------------------
  bool ensurePeer(const uint8_t *mac) {
    if (isAllZeroMac(mac)) return false;
    if (esp_now_is_peer_exist(mac)) return true;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
  }

  // Reconciles the registered ESP-NOW peer table against the desired
  // _pedalMac[] table - called from applyMacConfig() so this stays correct
  // both at boot and whenever the host pushes an updated MAC table at
  // runtime. Idempotent: a slot whose MAC hasn't changed is left alone.
  void syncPeerTable() {
    for (int i = 0; i < 3; i++) {
      bool changed = memcmp(_pedalMac[i], _registeredPedalMac[i], 6) != 0;
      if (!changed) continue;
      if (!isAllZeroMac(_registeredPedalMac[i])) {
        esp_now_del_peer(_registeredPedalMac[i]);
        memset(_registeredPedalMac[i], 0, 6);
      }
      if (!isAllZeroMac(_pedalMac[i]) && ensurePeer(_pedalMac[i])) {
        memcpy(_registeredPedalMac[i], _pedalMac[i], 6);
      }
    }
  }

  void handleLogPacket(const uint8_t *data, int len) {
    PayloadHidMessage_t receivedMsg;
    int copyLen = data[3];
    if (copyLen >= (int)sizeof(receivedMsg.text_ac)) copyLen = sizeof(receivedMsg.text_ac) - 1;
    if (copyLen <= 0 || 4 + copyLen > len) return;
    memset(receivedMsg.text_ac, 0, sizeof(receivedMsg.text_ac));
    receivedMsg.payloadType_u8 = DAP_PAYLOAD_TYPE_ESPNOW_LOG_U8;
    receivedMsg.magicKey1_u8 = ESPNOW_LOG_MAGIC_KEY_U8;
    receivedMsg.magicKey2_u8 = ESPNOW_LOG_MAGIC_KEY_2_U8;
    receivedMsg.length_u8 = copyLen;
    memcpy(receivedMsg.text_ac, &data[4], copyLen);
    receivedMsg.text_ac[copyLen] = '\0';
    xQueueSend(g_messageQueueHandle_pv, &receivedMsg, 0);
  }

  void handleStateBasicPacket(const uint8_t *data, uint8_t pedalTag) {
    DapStateBasic_t local;
    memcpy(&local, data, sizeof(DapStateBasic_t));
    if (local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8 ||
        local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_STATE_BASIC_U8) {
      logDebug("RX StateBasic dropped: bad type/version");
      return;
    }
    uint16_t crc = checksumCalculator((uint8_t*)(&(local.payloadHeader_st)),
        sizeof(local.payloadHeader_st) + sizeof(local.payloadPedalStateBasic_st));
    if (crc != local.payloadFooter_st.checkSum_u16) {
      logDebug("RX StateBasic dropped: bad CRC");
      return;
    }
    if (!_pedalWirelessSyncEnabled[pedalTag]) {
      return;
    }
    memcpy(&dap_state_basic_st[pedalTag], data, sizeof(DapStateBasic_t));
    // Force the tag to the MAC-verified slot, not whatever the sender
    // embedded in the payload - a pedal freshly reassigned to a new role
    // (e.g. Throttle -> Clutch) keeps broadcasting its old self-reported
    // pedalTag_u8 for a while, and downstream code (pedalAvailability_au8,
    // "Found Pedal" logging, and the packet forwarded to the PC) all key off
    // this field. handleStateExtendedPacket() already does this; this
    // sibling function was missing it.
    dap_state_basic_st[pedalTag].payloadHeader_st.pedalTag_u8 = pedalTag;
    g_updateBasicState_ab[pedalTag] = true;
    g_pedalLastUpdate_au32[pedalTag] = millis();
    if (local.payloadPedalStateBasic_st.errorCode_u8 != 0) g_espNowError_ab[pedalTag] = true;
    float joystickData_u32 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16 / 32767.0f * 10000.0f;
    uint16_t joystickNormalizedToInt16 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
    switch (pedalTag) {
      case PEDAL_ID_CLUTCH:
        g_pedalClutchValue_u16 = joystickNormalizedToInt16;
        g_joystickValue_au16[0] = joystickData_u32;
        g_joystickValueOriginal_au16[0] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
        break;
      case PEDAL_ID_BRAKE:
        g_pedalBrakeValue_u16 = joystickNormalizedToInt16;
        g_joystickValue_au16[1] = joystickData_u32;
        g_joystickValueOriginal_au16[1] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
        break;
      case PEDAL_ID_THROTTLE:
        g_pedalThrottleValue_u16 = joystickNormalizedToInt16;
        g_joystickValue_au16[2] = joystickData_u32;
        g_joystickValueOriginal_au16[2] = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
        g_pedalStatus_u8 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.pedalStatus_u8;
        g_joystickThrottleValueFromPedal_u16 = dap_state_basic_st[pedalTag].payloadPedalStateBasic_st.joystickOutput_u16;
        break;
      default:
        break;
    }
    logDebug("RX StateBasic accepted, pedal=%u", pedalTag);
  }

  void handleStateExtendedPacket(const uint8_t *data, uint8_t pedalTag) {
    DapStateExtended_t local;
    memcpy(&local, data, sizeof(DapStateExtended_t));
    if (local.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8 ||
        local.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_STATE_EXTENDED_U8) {
      logDebug("RX StateExtended dropped: bad type/version");
      return;
    }
    uint16_t crc = checksumCalculator((uint8_t*)(&(local.payloadHeader_st)),
        sizeof(local.payloadHeader_st) + sizeof(local.payloadPedalStateExtended_st));
    if (crc != local.payloadFooter_st.checkSum_u16) {
      logDebug("RX StateExtended dropped: bad CRC");
      return;
    }
    if (!_pedalWirelessSyncEnabled[pedalTag]) {
      return;
    }
    memcpy(&dap_state_extended_st[pedalTag], data, sizeof(DapStateExtended_t));
    dap_state_extended_st[pedalTag].payloadHeader_st.pedalTag_u8 = pedalTag;
    g_updateExtendState_ab[pedalTag] = true;
    logDebug("RX StateExtended accepted, pedal=%u", pedalTag);
  }

  void handleConfigEchoPacket(const uint8_t *data, uint8_t pedalTag) {
    // TEMP DIAGNOSTIC (unconditional, low-volume - config echoes are rare).
    // Remove once the root cause is confirmed.
    ActiveSerial->printf("[L][DIAG] ConfigEcho RX from pedal=%u, syncEnabled=%u\n",
                         pedalTag, (unsigned)_pedalWirelessSyncEnabled[pedalTag]);
    #ifdef USB_JOYSTICK
    tinyusbJoystick_.printf("[DIAG] ConfigEcho RX from pedal=%u, syncEnabled=%u",
                            pedalTag, (unsigned)_pedalWirelessSyncEnabled[pedalTag]);
    #endif
    if (!_pedalWirelessSyncEnabled[pedalTag]) {
      return;
    }
    memcpy(&dap_config_st_Temp, data, sizeof(DapConfig_t));
    dap_config_st_Temp.payloadHeader_st.pedalTag_u8 = pedalTag;
    dap_config_st_Temp.payloadPedalConfig_st.pedalType_u8 = pedalTag;
    g_espNowRequestConfig_ab[pedalTag] = true;
    if (pedalTag == 0) {
      memcpy(&dap_config_st_Clu, &dap_config_st_Temp, sizeof(DapConfig_t));
      memcpy(&dap_config_st[0], &dap_config_st_Temp, sizeof(DapConfig_t));
    } else if (pedalTag == 1) {
      memcpy(&dap_config_st_Brk, &dap_config_st_Temp, sizeof(DapConfig_t));
      memcpy(&dap_config_st[1], &dap_config_st_Temp, sizeof(DapConfig_t));
    } else if (pedalTag == 2) {
      memcpy(&dap_config_st_Gas, &dap_config_st_Temp, sizeof(DapConfig_t));
      memcpy(&dap_config_st[2], &dap_config_st_Temp, sizeof(DapConfig_t));
    }
    logDebug("RX ConfigEcho accepted, pedal=%u", pedalTag);
  }

  void handleServoConfigPacket(const uint8_t *data, uint8_t pedalTag) {
    DAP_servo_config_st_t received_servo_config;
    memcpy(&received_servo_config, data, sizeof(DAP_servo_config_st_t));
    // TEMP DIAGNOSTIC (unconditional - "Load From Servo" is rare/user-
    // triggered, not continuous). Tracking down why servo register
    // exchange doesn't complete over wireless. Remove once confirmed fixed.
    ActiveSerial->printf("[L][DIAG] ServoConfig response RX from pedal=%u, syncEnabled=%u\n",
                         pedalTag, (unsigned)_pedalWirelessSyncEnabled[pedalTag]);
    #ifdef USB_JOYSTICK
    tinyusbJoystick_.printf("[DIAG] ServoConfig response RX from pedal=%u, syncEnabled=%u",
                            pedalTag, (unsigned)_pedalWirelessSyncEnabled[pedalTag]);
    #endif
    if (received_servo_config.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8 ||
        received_servo_config.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8) {
      logDebug("RX ServoConfig dropped: bad type/version");
      return;
    }
    uint16_t crc = checksumCalculator((uint8_t*)(&(received_servo_config.payloadHeader_st)),
        sizeof(received_servo_config.payloadHeader_st) + sizeof(received_servo_config.payloadServoConfig_st));
    if (crc != received_servo_config.payloadFooter_st.checkSum_u16) {
      logDebug("RX ServoConfig dropped: bad CRC");
      return;
    }
    if (!_pedalWirelessSyncEnabled[pedalTag]) {
      return;
    }
    received_servo_config.payloadHeader_st.pedalTag_u8 = pedalTag;
    memcpy(&dap_servo_config_response_st[pedalTag], &received_servo_config, sizeof(DAP_servo_config_st_t));
    send_servo_config_to_host[pedalTag] = true;
    logDebug("RX ServoConfig accepted, pedal=%u", pedalTag);
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
#ifdef USB_JOYSTICK
    tinyusbJoystick_.printf("[WirelessComm] %s\n", buf);
#endif
  }
#else
  inline void logDebug(const char *fmt, ...) { (void)fmt; }
#endif
};

extern WirelessCommunicationBridge wirelessComm;
WirelessCommunicationBridge wirelessComm;

inline void sWirelessCommRecvTrampoline(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  wirelessComm.handleRecv(info, data, len);
}

inline void sWirelessCommSentTrampoline(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  wirelessComm.handleSent(info, status);
}

// Debug helper (unrelated to the wireless transport, kept here since it
// lived in the file this header replaces).
inline void printStructHex(DapBridgeState_t* s) {
  const uint8_t* p = (const uint8_t*)s;
  for (size_t i = 0; i < sizeof(DapBridgeState_t); i++) {
    ActiveSerial->print("0x");
    if (p[i] < 16) ActiveSerial->print('0');
    ActiveSerial->print(p[i], HEX);
    ActiveSerial->print("-");
  }
  ActiveSerial->println("");
}
