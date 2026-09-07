# ESP-NOW Wi-Fi Channel Scanning & Dynamic Spectrum Selection

## 1. Problem Statement & Background

In DIY Sim Racing FFB Pedal builds operating in **Rudder Mode**, pedal synchronization and state exchange rely on **ESP-NOW** at high update rates (e.g., 200–500 Hz). In the original firmware implementation, the wireless channel was hardcoded to:
```c
#ifndef ESPNOW_WIFI_CHANNEL
  #define ESPNOW_WIFI_CHANNEL 11
#endif
```

### The Root Cause of High / Erratic Latency Estimates
Several users experienced severe delay estimate spikes (e.g. latency jumping from ~2 ms to >20–50 ms), jitter, and packet loss when operating in Rudder mode. 

Because ESP-NOW operates on the standard 2.4 GHz ISM radio band (IEEE 802.11 b/g/n):
1. **Co-Channel Interference**: Channel 11 is one of the three standard non-overlapping 2.4 GHz Wi-Fi channels (1, 6, 11). It is frequently occupied by home Wi-Fi routers, mesh access points, smartphones, and Bluetooth peripherals.
2. **Clear Channel Assessment (CCA) Contention**: Under 802.11 CSMA/CA rules, if another device (such as a 40 MHz wide neighbor Wi-Fi network) transmits on or overlaps with Channel 11, the ESP32 radio hardware must defer transmission. This stalls the ESP-NOW queue, resulting in delayed transmissions, packet timeouts, and jitter in latency estimation.
3. **Asymmetric Signal Strength**: Even if the pedal-to-bridge link has strong RSSI, high ambient RF activity on Channel 11 forces retransmissions and backoffs.

---

## 2. Solution Overview: Dynamic Scan & Set via SimHub (Option 4)

To resolve this without adding dedicated configuration screens or buttons to the pedals, we implemented a full-stack **"Scan & Set via SimHub"** architecture:

```
+-------------------------------------------------------------------------+
|                              SimHub Plugin                              |
|  - "Scan Spectrum" button                                              |
|  - Real-time spectrum cards for Channels 1, 6, 11 (APs, RSSI, Score)    |
|  - Recommended channel recommendation banner                            |
|  - Channel selector dropdown + "Apply" button                           |
+-------------------------------------------------------------------------+
                     |  ^                                 |  ^
        USB HID /    |  | SCAN_RES           USB HID /    |  | SET_ACK
        Serial Port  v  |                    Serial Port  v  |
+-------------------------------------------------------------------------+
|                           ESP32 Master / Bridge                         |
|  - Scans Ch 1, 6, 11 using WiFi.scanNetworks()                         |
|  - Computes congestion score: Score = (AP_Count * 12) + RSSI_Factor     |
|  - Persists channel to EEPROM (Offset 60, Magic 0xA5)                  |
|  - Broadcasts WIFI_CH_CMD_SET_REQ via ESP-NOW                           |
+-------------------------------------------------------------------------+
                                 |
                     ESP-NOW     v  (WIFI_CH_CMD_SET_REQ)
+-------------------------------------------------------------------------+
|                             ESP32 Pedals                                |
|  - Receive channel switch request & update local radio channel          |
|  - Persist channel to EEPROM (Offset 260, Magic 0xA5)                  |
|  - Auto-Hunting Fallback: If disconnected >5s, cycles Ch 1, 6, 11      |
+-------------------------------------------------------------------------+
```

---

## 3. Communication Protocol & Binary Packets

### Payload Identifier & Command Codes
- **Payload Type**: `180` (`DAP_PAYLOAD_TYPE_WIFI_CHANNEL_U8`)
- **Commands**:
  - `1`: `WIFI_CH_CMD_SCAN_REQ` (SimHub -> Master: Request spectrum scan)
  - `2`: `WIFI_CH_CMD_SCAN_RES` (Master -> SimHub: Report scan results)
  - `3`: `WIFI_CH_CMD_SET_REQ` (SimHub -> Master -> Pedals: Set new active channel)
  - `4`: `WIFI_CH_CMD_SET_ACK` (Master -> SimHub: Acknowledge channel switch)

### Data Structure (`PayloadWifiChannel_t`)
Defined in `Common_Libs/DiyActivePedal_types/src/PayloadWifiChannel.h`:

```cpp
typedef struct __attribute__((packed)) PayloadWifiChannel
{
  uint8_t command_u8;            // 1=ScanReq, 2=ScanRes, 3=SetReq, 4=SetAck
  uint8_t currentChannel_u8;     // Active Wi-Fi channel (1-13)
  uint8_t recommendedChannel_u8; // Recommended clean channel (1, 6, 11)
  int8_t  channel1Rssi_i8;       // Strongest AP RSSI on Ch 1
  int8_t  channel6Rssi_i8;       // Strongest AP RSSI on Ch 6
  int8_t  channel11Rssi_i8;      // Strongest AP RSSI on Ch 11
  uint8_t channel1ApCount_u8;    // Detected AP count on Ch 1
  uint8_t channel6ApCount_u8;    // Detected AP count on Ch 6
  uint8_t channel11ApCount_u8;   // Detected AP count on Ch 11
  uint8_t channel1ApScore_u8;    // Congestion score (0-100) on Ch 1
  uint8_t channel6ApScore_u8;    // Congestion score (0-100) on Ch 6
  uint8_t channel11ApScore_u8;   // Congestion score (0-100) on Ch 11
} PayloadWifiChannel_t;

typedef struct __attribute__((packed)) DapWifiChannel
{
  PayloadHeader_t payloadHeader_st;
  PayloadWifiChannel_t payloadWifiChannel_st;
  PayloadFooter_t payloadFooter_st;
} DapWifiChannel_t;
```

---

## 4. Implementation Details

### 4.1. ESP32 Master Firmware (`ESP32_master`)
- **Scanning Logic**: Triggered upon receipt of `WIFI_CH_CMD_SCAN_REQ`. The Master calls `WiFi.scanNetworks(false, true)` to perform a passive/active scan. It iterates through detected networks, categorizes them into Channels 1, 6, and 11, records peak RSSI, and computes congestion:
  $$	ext{Score} = \min(100, (	ext{AP Count} 	imes 12) + 	ext{RSSI Penalty})$$
  The channel with the minimum congestion score is marked as `recommendedChannel_u8`.
- **Channel Switching**: Upon receiving `WIFI_CH_CMD_SET_REQ`:
  1. Broadcasts the packet over ESP-NOW 3 times with brief 10 ms delays to guarantee pedal reception.
  2. Saves the new channel to EEPROM at **Offset 60** with magic byte `0xA5`.
  3. Reconfigures local radio: `esp_wifi_set_channel(newCh, WIFI_SECOND_CHAN_NONE)`.
  4. Returns `WIFI_CH_CMD_SET_ACK` to SimHub.
- **Dual Transport**: Supports both CDC Virtual COM Port (Serial) and TinyUSB HID paths.

### 4.2. ESP32 Pedal Firmware (`ESP32`)
- **EEPROM Persistence**: Active channel stored at **Offset 260** with magic byte `0xA5`.
- **OTA Reception**: In `ESPNOW_lib.h`, `onRecv()` intercepts `DapWifiChannel_t` packets with `WIFI_CH_CMD_SET_REQ`, updates `g_currentWifiChannel_u8`, saves to EEPROM, and switches the radio channel.
- **Auto-Hunting Fallback**:
  - Implemented via `checkWifiChannelHunting()`, called on every iteration of `espNowCommunicationTaskTx`.
  - If no ESP-NOW packet is received for $> 5000	ext{ ms}$, the pedal cycles through channels 1, 6, and 11 every 1500 ms.
  - As soon as valid communication with the Master is re-established on any channel, `onRecv()` saves the newly discovered channel to EEPROM.

### 4.3. SimHub Plugin (`SimHubPlugin`)
- **Rudder Tab UI**:
  - Located directly beneath the **Wireless Sync Latency & Stability Monitor** graph.
  - Features real-time cards for Channels 1, 6, and 11 showing:
    - Detected AP Count
    - Peak RSSI in dBm
    - Color-coded Congestion badge (Green $\le 25\%$, Orange $\le 60\%$, Red $> 60\%$)
  - ComboBox pre-populated with the recommended cleanest channel.
  - Status banner providing clear diagnostic feedback.

---

## 5. User Guide: How to Optimize Wi-Fi Channel in SimHub

1. Open **SimHub** and navigate to the **DIY FFB Pedal** plugin.
2. Select the **RUDDER** tab.
3. In the **WI-FI SPECTRUM & CHANNEL OPTIMIZER** card:
   - Click **"🔍 Scan Spectrum"**.
   - Wait ~1 second while the ESP32 Master analyzes 2.4 GHz RF traffic.
   - Review the AP count and congestion percentage across Channels 1, 6, and 11.
4. The system will automatically select the cleanest channel in the dropdown.
5. Click **"Apply"**:
   - The Master broadcasts the new channel to all connected pedals.
   - The Master and Pedals switch radio frequencies simultaneously.
   - The settings are saved to EEPROM on all devices and will persist across power cycles.

---

## 6. Verification & Build Artifacts

All firmware and software components have been validated with zero build errors:
- **`ESP32_master`**: `pio run -e Bridge_esp32s3usbotg` -> `SUCCESS`
- **`ESP32` (Pedals)**:
  - `pio run -e ControlBoard_V7` -> `SUCCESS`
  - `pio run -e ControlBoard_PCBA_V2X` -> `SUCCESS`
  - `pio run -e ControlBoard_V7_without_espnow` -> `SUCCESS`
- **`SimHubPlugin`**: `MSBuild.exe DIYFFBPedalUI.csproj -t:Build -p:Configuration=Release` -> `SUCCESS` (0 warnings, 0 errors, deployed to SimHub).
