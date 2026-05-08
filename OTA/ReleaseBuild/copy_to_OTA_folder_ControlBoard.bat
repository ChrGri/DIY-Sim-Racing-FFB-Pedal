@echo off

set source=..\..\ESP32\.pio\build\ControlBoard_V4\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V4\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V4\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V1X\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V1X\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V1X\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_1_2\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5_without_espnow\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5_without_espnow\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5_without_espnow\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V5\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V5\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X_without_espnow\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X_without_espnow\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_PCBA_V2X_without_espnow\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\Gilphilbert_2_0_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6_without_espnow\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6_without_espnow\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6_without_espnow\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6_without_wireless\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6\firmware.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6\bootloader.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

set source=..\..\ESP32\.pio\build\ControlBoard_V6\partitions.bin
set destination=..\ReleaseBuild\ControlBoard\esp32S3_V6\

echo Copying %source% to %destination%...
xcopy "%source%" "%destination%" /y

echo File copied successfully.
pause
