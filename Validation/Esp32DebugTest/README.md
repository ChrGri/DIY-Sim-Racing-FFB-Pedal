# ESP32-S3 PlatformIO Debugging Guide (Manual JTAG Method)

Debugging the ESP32-S3 via its built-in USB-JTAG interface on Windows can be notoriously difficult due to driver conflicts and the hardware Watchdog Timer (WDT) severing the OpenOCD connection. 

This guide outlines a highly reliable, manual debugging workflow. By utilizing both USB ports and manually controlling the boot state via the physical board buttons, we can reliably bypass the WDT and USB controller resets that typically break the debugger.

## 1. Hardware Setup
For this workflow, you must use **two USB cables** connected simultaneously:
1.  **UART Port (e.g., CH343):** Used for flashing the firmware and standard Serial Monitoring.
2.  **Native USB Port:** Used exclusively for the JTAG debugging connection.

## 2. Driver Setup (Zadig)
By default, Windows aggressively claims the ESP32-S3's JTAG interface with a standard Serial driver, blocking OpenOCD (`LIBUSB_ERROR_ACCESS`). You must replace the driver for the JTAG interface with `libusbK`.

1. Plug a USB cable into the **Native USB** port of the ESP32-S3.
2. Download and open [Zadig](https://zadig.akeo.ie/).
3. In the top menu, go to **Options** -> **List All Devices**.
4. In the main dropdown, select **`USB JTAG/serial debug unit (Interface 2)`**.
   > **⚠️ CRITICAL:** Do not select Interface 0. Modifying Interface 0 will break the board's native USB Serial capabilities.
5. On the right side of the green arrow, use the up/down arrows to select **`libusbK`**.
6. Click **Replace Driver** (or Install Driver) and wait for it to finish.
7. Unplug the Native USB cable, wait 3 seconds, and plug it back in.

## 3. PlatformIO Configuration
Ensure your `platformio.ini` is configured to upload via UART but debug via the built-in JTAG. The configuration below also includes necessary compiler flags to generate FreeRTOS symbols for GDB.

```ini
[env:esp32s3_debug]
platform  = [https://github.com/pioarduino/platform-espressif32/releases/download/55.03.30-2/platform-espressif32.zip](https://github.com/pioarduino/platform-espressif32/releases/download/55.03.30-2/platform-espressif32.zip)
framework = arduino
board     = esp32-s3-devkitc-1

; --- Upload & Monitor via UART bridge ---
upload_protocol = esptool
upload_port     = COM12 ; Adjust to your UART COM port
monitor_port    = COM12 ; Adjust to your UART COM port
monitor_speed   = 115200

; --- Debug via Native USB-JTAG ---
debug_tool        = esp-builtin
debug_init_break  = tbreak setup
debug_speed       = 5000
build_type        = debug
debug_load_mode   = manual ; Tells PIO not to flash over JTAG

build_unflags =
    -Os
    -Og
    -DARDUINO_USB_CDC_ON_BOOT=0
    -DARDUINO_USB_MODE=0
    -O3

build_flags =
    -O0
    -g3
    -ggdb3
    -fno-omit-frame-pointer
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    ; FreeRTOS symbol generation for OpenOCD
    -D CONFIG_ESP_CONSOLE_USB_CDC=y
    -D CONFIG_ESP_USB_CDC_ENABLED=y
    -D CONFIG_USB_CDC_TRACE=y
    -D CONFIG_FREERTOS_UNICORE=0
    -D CONFIG_ESP_MAIN_TASK_AFFINITIY=0x1
```

## 4. The Debugging Workflow
Because `debug_load_mode` is set to `manual`, the debugger will attempt to attach to the code already running on the chip. Follow this exact sequence to prevent the Watchdog Timer from crashing the debug session.

### Step 1: Flash the Firmware
Use the standard PlatformIO **Upload** button (the right-pointing arrow in the bottom toolbar) to compile and flash your code over the UART connection.

### Step 2: Enter ROM Bootloader Mode
We must put the chip to sleep in its factory bootloader state. This ensures the Watchdog Timer is completely disabled when the debugger connects.
1. Press and **hold** the physical `BOOT` button on the board.
2. While holding `BOOT`, press and release the `RST` (Reset) button.
3. Release the `BOOT` button.
*The board is now idle, and the WDT is disabled.*

### Step 3: Attach the Debugger
1. Open the **Run and Debug** view in VS Code (`Ctrl+Shift+D`).
2. In the configuration dropdown at the top, select **`PIO Debug (without uploading)`**.
3. Click the green **Play** button to start the debugging session.
4. Wait for the debug console to show that GDB has successfully attached and halted the CPUs.

### Step 4: Boot and Trap
1. Once the debugger is successfully attached and waiting, press the physical **`RST`** button on the board once.
2. The ESP32-S3 will reboot, execute its boot sequence, and the debugger will immediately catch it, halting execution at your `setup()` breakpoint. 

You can now step through your code, inspect variables, and set hardware breakpoints normally.

## 5. Reverting the Driver Change (Uninstalling libusbK)
If you want to use the native USB port for standard serial communication again, or if you want to undo the Zadig configuration, you can easily revert the driver change using Windows Device Manager.

1. Leave the ESP32-S3 plugged in via the **Native USB** port.
2. Open the Windows **Device Manager** (press the Windows key, type "Device Manager", and hit Enter).
3. Look for a category named **libusbK USB Devices** (or potentially **Universal Serial Bus devices**) and expand it.
4. Right-click on **`USB JTAG/serial debug unit (Interface 2)`** and select **Uninstall device**.
5. **⚠️ CRITICAL:** In the confirmation window, check the box that says **"Attempt to remove the driver for this device"**. 
6. Click **Uninstall**.
7. Unplug the Native USB cable from your computer.
8. Wait a few seconds, then plug it back in. Windows will automatically detect the hardware and reinstall its default `usbser` (USB Serial Device) driver.