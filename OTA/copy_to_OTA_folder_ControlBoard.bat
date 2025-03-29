@echo off
set source=..\ESP32\.pio\build\esp32\firmware.bin
set destination=..\OTA\ControlBoard\esp32\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32\bootloader.bin
set destination=..\OTA\ControlBoard\esp32\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32\partitions.bin
set destination=..\OTA\ControlBoard\esp32\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg\firmware.bin
set destination=..\OTA\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg\bootloader.bin
set destination=..\OTA\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg\partitions.bin
set destination=..\OTA\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\firmware.bin
set destination=..\OTA\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\bootloader.bin
set destination=..\OTA\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\partitions.bin
set destination=..\OTA\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\firmware.bin
set destination=..\OTA\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\bootloader.bin
set destination=..\OTA\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\partitions.bin
set destination=..\OTA\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32-speedcrafter\firmware.bin
set destination=..\OTA\ControlBoard\Speedcrafter\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32-speedcrafter\bootloader.bin
set destination=..\OTA\ControlBoard\Speedcrafter\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32-speedcrafter\partitions.bin
set destination=..\OTA\ControlBoard\Speedcrafter\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg_pcbV5\firmware.bin
set destination=..\OTA\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg_pcbV5\bootloader.bin
set destination=..\OTA\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\ESP32\.pio\build\esp32s3usbotg_pcbV5\partitions.bin
set destination=..\OTA\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

echo File copied successfully.
pause
