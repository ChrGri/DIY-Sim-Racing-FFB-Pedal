
<p align="center">
  <img src="https://github.com/user-attachments/assets/6828f954-1b4f-4f7d-a388-92cbb616dd6a" alt="DIY Sim Racing FFB Pedal Banner" width="100%">
</p>


[![Arduino Build](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml/badge.svg?branch=main)](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/arduino.yml)
[![Doxygen Action](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml/badge.svg)](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/actions/workflows/main.yml)
[![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]
# License selection
This work is licensed under a [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License][cc-by-nc-sa]
[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

The reason for that license selection is that at some point in time, individuals have done the following without contributing anything to this project.
- Taken the sources and binaries and sold them on the internet
- Started mass production of FFB Pedals from these designs in their living rooms to make money, by taking parts from this project and Simucube's design files
- The DIY FFB Pedal project is not affiliated with the video (https://www.youtube.com/watch?v=9ibAyHcSjO0) or its channel (https://www.youtube.com/@xiaoximu). We do not provide support or after-sales service for the content of this video or the associated website.<br>

All that, without contributing anything to this project. 

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg
# DIY Sim Racing FFB Pedal
If you're used to standard spring or damper-based pedals with rumble motors attached, a force-feedback pedal is the next step. It uses a high power servo attached to a linear rail to control the motion of the pedal. This allows you to change how the pedal feels with a few changes on your PC, whether that's braking pressure, response or travel or the thottle weight or stiffness. You can even use profiles to build different "feels" for different cars, switching profiles between cars to give each vehicle a different driving experience. Additionally, since the pedal movement is controlled, the feedback it can produce is totally different - imaging feeling ABS feedback moving the pedal, the kick of the gear change through the throttle or feedback from road bumps through the pedals. It's an experience like no other! If that sounds like something you want to build for yourself, then read on! This project documents people who want to design and/or build their own force-feedback pedal. <br> <img src="https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design/blob/main/MechanicalDesign_11_20215/Build/Images/DiyPedalAssemblyV3_dilatation_v19.png" width="400"> <br>


> [!TIP]
> **Disclaimer** This repository documents my research progress. I wanted to understand the necessary signal processing and control theory algorithms behind such a device. 

> [!WARNING]
> The FFB pedal is a robot and can be dangerous. Please watch [The Terminator](https://en.wikipedia.org/wiki/The_Terminator) before continuing. If not interacted with care, it may cause harm. I'm not responsible for any harm caused by this design suggestion. Use responsibly and at your own risk.

# Project repositories
This project has been divided into multiple repositories, each with differt purposes. The mechanical design repository provides the information you need to build the mechanics of ChrGri's pedal. It's not the only design, but it's strong and reliable. There are more options on the Wiki, and you can find even more designs on the Discord server. The Software repo (this repo) discusses how to select, order and connect the electronics, flash the firmware and get the pedal up and running. The final repo, contains designs for the recommended circuit boards that control the pedal.
| Description           |  Link |
:------------------------- | :------------------------- |
| ChrGri's mechanical and electrical design | https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal-Mechanical-Design |
| Software (firmware, SimHub plugin, ...) |https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal |
| Control Board and Power Board design | https://github.com/gilphilbert/DIY-Sim-Racing-FFB-Pedal-PCBs |

# Features
- SimHub integration
- Integration into a number of different simulators
- Tune pedal parameters
- Connect over USB or wireless
- Supports Analog output for integration into additional devices
- Multiple feedback types (more details in the [Wiki](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki/Pedal-Effects))
  - ABS (AntiLock Braking System)
  - Traction Control
  - RPM vibration
  - Road rumble
- Powerful servo that can generate torque for heavy braking exceeding 100KGs when paired with a suitable linear rail

# See the pedals in action
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/i2e1ukc1ylA/0.jpg)](https://www.youtube.com/watch?v=i2e1ukc1ylA)

More pedal action examples can be found on the [Wiki](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki/Hardware-designs) and the [Discord server](https://discord.gg/zTfQaxpAUz)

# Contributions
A lot of awesome devs have helped this project grow. Just to name a few:
- [tjfenwick](https://github.com/tjfenwick) started the project with an initial implementation
- [captainchris](https://github.com/ChrGri) continued development. Mostly interested in learning and teaching the aspects of control algorithms.
- [tcfshcrw](https://github.com/tcfshcrw) helped to elevate the SimHub plugin to its current form, added a ton of new pedal effects, provides support on Discord, is a great guy and much more!
- [MichaelJFr](https://github.com/MichaelJFr) helped by refactoring the code at the beginning of this project. Fruitful discussions let to the implementation of the control-loop strategies
- [Ibakha](https://github.com/Ibakha) Our Discord channel CEO
- [gilphilbert](https://github.com/gilphilbert) developed custom PCB assemblies, refactored the Wiki and created the Web Flash tool
- [soyd](https://github.com/al-H1) continued development of custom PCB assemblies

# How to get started
## Read the Wiki
Detailed descriptions of all aspects of the build can be found on the dedicated [Wiki page](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki), which includes instructions on how to select parts, build the pedals and integrate them into your games

## Join the Discord server
Our [Discord server](https://discord.gg/zTfQaxpAUz) has thousands of users just like you and contains additional designs, insight and provides a place to find answers to all of your questions.

# Architecture
A Deepwiki of the sources can be found [here](https://deepwiki.com/ChrGri/DIY-Sim-Racing-FFB-Pedal).

