@echo off
set source=..\..\ESP32_master\.pio\build\Bridge_Fanatec\firmware.bin
set destination=..\DevBuild\Bridge\Fanatec_Bridge\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_Fanatec\bootloader.bin
set destination=..\DevBuild\Bridge\Fanatec_Bridge\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_Fanatec\partitions.bin
set destination=..\DevBuild\Bridge\Fanatec_Bridge\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_Dongle_V1\firmware.bin
set destination=..\DevBuild\Bridge\Gilphilbert_dongle\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_Dongle_V1\partitions.bin
set destination=..\DevBuild\Bridge\Gilphilbert_dongle\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_Dongle_V1\bootloader.bin
set destination=..\DevBuild\Bridge\Gilphilbert_dongle\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg\firmware.bin
set destination=..\DevBuild\Bridge\dev_kit\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg\bootloader.bin
set destination=..\DevBuild\Bridge\dev_kit\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg\partitions.bin
set destination=..\DevBuild\Bridge\dev_kit\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3_external_joystick\firmware.bin
set destination=..\DevBuild\Bridge\dev_kit_with_external_joystick\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3_external_joystick\bootloader.bin
set destination=..\DevBuild\Bridge\dev_kit_with_external_joystick\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3_external_joystick\partitions.bin
set destination=..\DevBuild\Bridge\dev_kit_with_external_joystick\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg_lowTxPower\firmware.bin
set destination=..\DevBuild\Bridge\dev_kit_with_lower_power\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg_lowTxPower\bootloader.bin
set destination=..\DevBuild\Bridge\dev_kit_with_lower_power\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32_master\.pio\build\Bridge_esp32s3usbotg_lowTxPower\partitions.bin
set destination=..\DevBuild\Bridge\dev_kit_with_lower_power\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

echo File copied successfully.
pause