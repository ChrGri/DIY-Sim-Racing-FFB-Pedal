# ESP-NOW Wireless Placement Guide: PCBs & Bridge Module

To ensure ultra-low latency (~3 ms) and jitter-free communication at ~280 Hz over ESP-NOW, the physical placement and orientation of your pedal controller PCBs and the USB Bridge module are critical.

Target Signal Strength: **RSSI > -66 dBm** (e.g., between -33 dBm and -60 dBm).

---

## 1. Physical Placement & Polarization Alignment

Mount the central Bridge receiver centrally between your pedals so all antennas maintain an unobstructed, direct line-of-sight and share matching antenna polarization.

### Comparison: Cross-Polarized vs. Co-Polarized Bridge Mounting

| Configuration | Setup & Signal Impact | Recommendation |
| :--- | :--- | :--- |
| **Cross-Polarized**<br>*(Bridge Horizontal, Pedal PCBs Vertical)* | ![Cross-Polarized Bridge Placement](media/images/BoardAndBridgePlacement_0.JPG)<br>The bridge module is lying flat horizontally while the pedal PCBs are mounted vertically on the uprights (90° polarization mismatch). This causes **15 to 25 dB polarization attenuation**, lowering RSSI to ~-60 to -75 dBm and increasing susceptibility to multipath nulls. | ⚠️ **Suboptimal**: Prone to RF jitter and packet retries in high-interference environments. |
| **Co-Polarized**<br>*(Bridge Vertical, Pedal PCBs Vertical)* | ![Co-Polarized Bridge Placement](media/images/BoardAndBridgePlacement_1.JPG)<br>The bridge module is mounted upright vertically, matching the exact plane of the pedal PCBs. Both broadside radiation lobes align, maximizing effective gain and yielding excellent signal strength (**~-33 to -45 dBm**). | ✅ **Recommended**: Delivers rock-solid wireless stability, lowest latency, and maximum SNR. |

---

### Key Mounting Guidelines:
- **Co-Polarized Alignment (Vertical Orientation)**: Always align the bridge module vertically when the pedal boards are mounted vertically on the uprights. Co-polarization avoids the 90° cross-polarization penalty.
- **Central Bridge Position**: Mount the Bridge module centered on the transverse profile between the pedals.
- **Inward-Facing PCBs**: Mount the pedal controller boards on the inner vertical sides of each pedal upright. This ensures the onboard antennas point directly toward the central bridge rather than through pedal mechanics.
- **Avoid Aluminum Profile Shielding**: Aluminum 8020/4040 extrusions block 2.4 GHz RF signals completely. Keep antennas positioned slightly clear of thick aluminum beams; never enclose the antenna inside an extrusion channel.
- **Keep Cable Bundles Clear of Antennas**: Keep heavy motor and power wires bundled behind the board so they do not shadow the RF path.

---

## 2. Antenna Alignment & Orientation Principles

ESP32-S3 boards (both PCB trace antennas on DevKit modules and ceramic chip antennas on Zero modules) have directional radiation lobes.

![Optimal ESP-NOW Antenna Alignment Guide](media/images/OptimumAntennaPlacementForEspNowCommunication.jpg)

### Recommended Configurations:
1. **Parallel / Broadside Alignment (Best for Pedal Rigs)**:
   - Align the broadside faces of the antennas parallel to each other.
   - The broadside radiation lobe delivers maximum gain.
2. **Vertical Stacking (Out-Of-Plane)**:
   - When boards are stacked above or below one another, keep their faces aligned in the same orientation.

### Configurations to Avoid:
- ❌ **Null-to-Null Alignment**: Never point the very tip (top edge) of one ESP32 antenna directly at the tip of another board. The antenna tips are radiation null zones with significantly reduced gain.
- ❌ **Edge-on Null Alignment**: Avoid aligning boards perpendicular such that the edge points into the dead zone of the receiver.

---

## 3. Real-Time Verification in SimHub

Once installed, verify wireless connectivity directly in the SimHub plugin under the rudder tab and **ESP-NOW Wireless Telemetry & Sync** monitor.

![ESP-NOW Realtime RSSI Monitor](media/images/EspNowRealtimeRssi_0.png)

### What to Look For:
| Metric | Healthy Target | Action Required if Degraded |
| :--- | :--- | :--- |
| **Pedal RSSI** | **> -66 dBm** (e.g., -33 to -60 dBm) | If below -66 dBm (e.g., -70 to -85 dBm), rotate or reposition the bridge module. |
| **Delay / Latency** | **~2 to 4 ms** | Check line-of-sight if latency spikes above 10 ms. |
| **Telemetry Rate** | **~280 Hz** | Sustained rate confirms zero dropped frames. |
| **Jitter** | **< ±0.5 ms** | Indicates clean RF environment without retransmission delays. |
