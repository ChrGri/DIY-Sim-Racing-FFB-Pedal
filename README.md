<p align="center">
  <img src="https://github.com/user-attachments/assets/6828f954-1b4f-4f7d-a388-92cbb616dd6a" alt="DIY Sim Racing FFB Pedal Banner" width="100%">
</p>

# DIY Sim Racing Force Feedback (FFB) Pedal

<p align="center">
  <a href="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml"><img src="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml/badge.svg?branch=main" alt="Arduino Build"></a>
  <a href="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml"><img src="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml/badge.svg" alt="Doxygen Action"></a>
  <a href="https://discord.gg/zTfQaxpAUz"><img src="https://img.shields.io/badge/Discord-3.4k%2B%20Members-5865F2?logo=discord&logoColor=white" alt="Discord Community"></a>
  <a href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img src="https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg" alt="CC BY-NC-SA 4.0"></a>
  <a href="#-support-the-team"><img src="https://img.shields.io/badge/Support-The%20Team-ff5e5b.svg?logo=kofi&logoColor=white" alt="Support the Team"></a>
</p>

---

## 📜 License

This project is licensed under the **[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)][cc-by-nc-sa]**.

<p align="left">
  <a href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img src="https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png" alt="CC BY-NC-SA 4.0"></a>
</p>

### Non-Commercial Policy
This license was specifically chosen because commercial entities and individuals have previously:
- Taken open-source binaries and sources to resell them for commercial gain without attribution or contribution.
- Mass-produced pedals based on these designs and proprietary files without contributing back to the community.
- Produced unofficial video guides or storefronts with which we have no affiliation (e.g., the DIY FFB Pedal project is not affiliated with [this video](https://www.youtube.com/watch?v=9ibAyHcSjO0) or [channel](https://www.youtube.com/@xiaoximu), and we cannot provide support for third-party commercial kits).

Please respect the open-source spirit: **Build it, learn from it, improve it, and share your modifications with the community under the same open-source terms.**

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/

---

<p align="center">
  <a href="#-overview"><b>Overview</b></a> •
  <a href="#-hardware-at-a-glance"><b>Hardware at a Glance</b></a> •
  <a href="#-key-features"><b>Features</b></a> •
  <a href="#-see-the-pedals-in-action"><b>Video Demo</b></a> •
  <a href="#-project-ecosystem--repositories"><b>Ecosystem</b></a> •
  <a href="#-how-to-get-started"><b>Getting Started</b></a> •
  <a href="#-support-the-team"><b>Support the Team</b></a> •
  <a href="#️-disclaimer--safety"><b>Disclaimer & Safety</b></a>
</p>

---

## 📖 Overview

If you are used to standard spring- or elastomer-based pedals with small rumble motors, a **force feedback (FFB) pedal** is a game changer. 

Instead of static mechanical resistance, an FFB pedal uses an industrial-grade high-power servo motor connected to a precision linear rail/ball screw mechanism. This empowers you to define and alter how the pedal behaves purely through software:
- **Full software adjustability:** Modify braking force, travel distance, preload, and force curves in seconds without swapping rubbers, springs, or elastomers.
- **Per-car profiles:** Switch between a modern GT3 with heavy, short-travel hydraulic brake feel, a classic formula car, or a road car clutch with a realistic physical bite point.
- **Dynamic telemetry effects:** Feel true telemetry-driven sensations through your feet—**ABS pulses**, **traction control kick**, **engine RPM rumble**, **wheel slip**, and **road surface bumps**.

<p align="center">
  <img src="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design/blob/main/MechanicalDesign_11_20215/Build/Images/DiyPedalAssemblyV3_dilatation_v19.png" alt="DIY Pedal Assembly" width="550">
</p>

> [!TIP]
> **Research & Community Project:** This repository documents ongoing research into control theory, motor control algorithms, and signal processing for DIY sim racing hardware.

### 🛠️ Hardware at a Glance

| Component | Recommendation | Purpose / Notes |
| :--- | :--- | :--- |
| **Microcontroller (MCU)** | ESP32-S3 (V7) | Runs 4 kHz control loop, telemetry decoding, and servo communication. |
| **Integrated Servo Motor** | Stepperonline iSV57-130 | High-torque closed-loop servo delivering dynamic pedal resistance & telemetry pulses. |
| **Linear Rail & Ball Screw** | JKK60 / KK60 rail (100–150 mm travel, 5–10 mm pitch) | Converts rotary torque into smooth, heavy-duty linear pedal travel. |
| **Load Cell & ADC** | DYLY-107 (100–200 kg) + ADS1220 24-bit ADC | Precision force measurement for realistic hydraulic brake pressure curves. |
| **Controller & Power PCBs** | ControlBoard V6 / V7 + Power Distribution Board | Clean wiring with RC filtering, diode protection, and RS232 telemetry transceiver. |
| **Power Supply (PSU)** | 36V (e.g., MeanWell LRS / RSP series) | High-current DC power for single or multi-pedal configurations. |
| **Estimated Build Cost** | ~€250 – €400 per pedal | High-end commercial FFB performance at a fraction of the cost. |

---

## ⚡ Key Features

- **True Force Feedback:** Servo-driven linear motion delivering realistic pedal feel with customizable resistance curves.
- **High Force Capacity:** Capable of exceeding 100 kg of braking pressure when paired with recommended servo motors (e.g., Stepperonline iSV57) and ballscrew linear rails.
- **Rich Telemetry Effects:**
  - ABS (Anti-lock Braking System) vibration and pulses
  - Traction Control (TC) kick & wheel slip effects
  - Engine RPM and rev-limiter rumble
  - Road surface texture and curbs
  - Simulated clutch bite point with tactile snap-through
- **Flight Sim & Rudder Mode (✈️ / 🚁):** Two pedals can be linked wirelessly via ESP-NOW to act as synchronized flight rudder pedals with authentic dynamics for airplanes ✈️ (self-centering, speed-dependent aerodynamic stiffness) and helicopters 🚁 (force trim release, friction hold). Read more in the **[Rudder Modes & Flight Dynamics Guide](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/main/docs/rudder_modes_flight_dynamics.md)**.
- **SimHub Integration:** Deep integration with SimHub for telemetry effect configuration, live telemetry tuning, and profile switching.
- **Versatile Connectivity:** Supports USB serial communication, wireless ESP-NOW pedal-to-pedal linking, and analog output for external compatibility.
- **Cross-Simulator Support:** Works seamlessly across major racing simulators (iRacing, Assetto Corsa, ACC, rFactor 2, Automobilista 2, BeamNG, and more) as well as flight simulators (MSFS 2020/2024, X-Plane, DCS World).

---

## 🎬 See the Pedals in Action

[![DIY FFB Pedal in Action](https://img.youtube.com/vi/i2e1ukc1ylA/0.jpg)](https://www.youtube.com/watch?v=i2e1ukc1ylA)

> 📺 **Watch on YouTube:** [DIY FFB Sim Racing Pedal in Action](https://www.youtube.com/watch?v=i2e1ukc1ylA)

---

## 🌐 Project Ecosystem & Repositories

The DIY FFB ecosystem consists of modular repositories covering mechanical design, electronics, software, and companion hardware:

| Repository | Description | Link |
| :--- | :--- | :--- |
| **Software & Firmware** (This Repo) | Microcontroller firmware (ESP32), SimHub integration, control loop code, and effect algorithms. | [DIY-Sim-Racing-FFB-Pedal](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal) |
| **Mechanical Design** | 3D models, CAD files, laser-cut templates, and assembly instructions for the pedal mechanics. | [DIY-Sim-Racing-FFB-Pedal-Mechanical-Design](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design) |
| **Control & Power PCBs** | Custom PCB designs (ControlBoard V6 / V7 and Power Distribution) for clean, reliable wiring. | [DIY-Sim-Racing-FFB-Pedal-PCBs](https://github.com/gilphilbert/DIY-Sim-Racing-FFB-Pedal-PCBs) |
| **FFB Belt Tensioner** | Force feedback seatbelt tensioner firmware & guide — reuses the exact same PCB and servo motor ecosystem! | [DIY-Sim-Racing-FFB-BeltTensioner](https://github.com/ChrGri/DIY-Sim-Racing-FFB-BeltTensioner) |

---

## 🚀 How to Get Started

1. **Read the Wiki:**  
   Head over to the **[DIY FFB Pedal Wiki](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki)**. It contains step-by-step guides on component selection (servos, rails, load cells, power supplies), mechanical assembly, wiring diagrams, and software flashing.
2. **Review the Source Architecture:**  
   Explore the deep architecture documentation on **[Deepwiki](https://deepwiki.com/ChrGri/DIY-Sim-Racing-FFB-Pedal)**.
3. **Join the Discord Community:**  
   Join our **[Discord Server](https://discord.gg/zTfQaxpAUz)** with thousands of builders. You'll find community designs, troubleshooting help, 3D printing mods, and shared pedal profiles.

---

## ☕ Support the Team

We ❤️ doing research. New hardware (e.g. oscilloscopes, logic analyzers, servos, PCBs, load cells) is very expensive. Feel free to support us and thus accelerate our research activity:

| Dev | captainchris | tcfshcrw | gilphilbert |
| :--- | :---: | :---: | :---: |
| **Buy me a coffee** | <a href="https://www.buymeacoffee.com/Captainchris"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" height="24px" alt="Buy Me A Coffee"></a> | <a href="https://www.buymeacoffee.com/tcfshcrw"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" height="24px" alt="Buy Me A Coffee"></a> | <a href="https://www.buymeacoffee.com/gilphilbert"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" height="24px" alt="Buy Me A Coffee"></a> |
| **Ko-fi** | [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/captainchris88) | [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/tcfshcrw) | [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/gilphilbert) |

Your support directly helps fund prototype components, PCBs, test equipment, and future companion sim racing hardware!

---

## 🤝 Contributions & Credits

This project thrives thanks to an incredible open-source community:

- **[tjfenwick](https://github.com/tjfenwick)** – Started the project with the initial proof of concept and implementation.
- **[captainchris](https://github.com/ChrGri)** – Maintained and advanced the project, focusing on control algorithms, signal processing, and system architecture.
- **[tcfshcrw](https://github.com/tcfshcrw)** – Elevated the SimHub plugin to its modern form, engineered pedal effects, and provides dedicated Discord support.
- **[MichaelJFr](https://github.com/MichaelJFr)** – Refactored the core codebase in the early stages and co-developed control-loop strategies.
- **[Ibakha](https://github.com/Ibakha)** – Discord community lead and community coordination.
- **[gilphilbert](https://github.com/gilphilbert)** – Designed custom PCB assemblies, overhauled the Wiki, and developed the Web Flasher tool.
- **[soyd](https://github.com/al-H1)** – Continued development and evolution of custom PCB hardware.

---

## 📈 Star History

<p align="center">
  <a href="https://star-history.com/#ChrGri/DIY-Sim-Racing-FFB-Pedal&ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design&ChrGri/DIY-Sim-Racing-FFB-BeltTensioner&Date">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=ChrGri/DIY-Sim-Racing-FFB-Pedal,ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design,ChrGri/DIY-Sim-Racing-FFB-BeltTensioner&type=Date&theme=dark" />
      <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=ChrGri/DIY-Sim-Racing-FFB-Pedal,ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design,ChrGri/DIY-Sim-Racing-FFB-BeltTensioner&type=Date" />
      <img src="https://api.star-history.com/svg?repos=ChrGri/DIY-Sim-Racing-FFB-Pedal,ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design,ChrGri/DIY-Sim-Racing-FFB-BeltTensioner&type=Date" alt="Star History Chart" width="700">
    </picture>
  </a>
</p>

---

## ⚠️ Disclaimer & Safety

> [!WARNING]
> **The FFB pedal is a robot and can be dangerous. Please watch [The Terminator](https://en.wikipedia.org/wiki/The_Terminator) before continuing!**
> 
> Force feedback pedals and servo actuators produce high mechanical forces and rapid motions. If not interacted with care, it may cause harm:
> - **Mechanical pinch points:** Keep hands, loose clothing, and cables clear of linear rails, pedal arms, and moving parts during operation.
> - **Mounting & rigidity:** Ensure your rig structure and pedal base are rigid and securely mounted to safely absorb high braking loads (100+ kg).
> - **Emergency shutoff:** Always include a readily accessible emergency power cut-off / kill switch for high-voltage power supplies.
> - **Liability:** I'm not responsible for any harm or damage caused by this design suggestion. Use responsibly and entirely at your own risk.
