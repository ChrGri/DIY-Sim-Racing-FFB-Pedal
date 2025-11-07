@echo off

set source=..\..\ESP32\.pio\build\esp32s3usbotg\firmware.bin
set destination=..\TestBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg\bootloader.bin
set destination=..\TestBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg\partitions.bin
set destination=..\TestBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\firmware.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\bootloader.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert\partitions.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\firmware.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\bootloader.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0\partitions.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5_without_espnow\firmware.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5_without_espnow\bootloader.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5_without_espnow\partitions.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5\firmware.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5\bootloader.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV5\partitions.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0_without_espnow\firmware.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0_without_espnow\bootloader.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg-gilphilbert_2_0_without_espnow\partitions.bin
set destination=..\TestBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6_without_espnow\firmware.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6_without_espnow\bootloader.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6_without_espnow\partitions.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6\firmware.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6\bootloader.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\esp32s3usbotg_pcbV6\partitions.bin
set destination=..\TestBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

echo File copied successfully.
pause
