@echo off
setlocal enabledelayedexpansion

:: Set destination paths
set "DEST_DIR=SimHubPlugin\Resources\Firmware"
set "MANIFEST_FILE=%DEST_DIR%\manifest.txt"

:: Define all source directories to scan (separated by spaces)
set "SOURCE_DIRS=ESP32\.pio\build ESP32_master\.pio\build"

echo Cleaning old firmware resources...
if exist "%DEST_DIR%" rmdir /s /q "%DEST_DIR%"
mkdir "%DEST_DIR%"

:: Create an empty manifest file
type nul > "%MANIFEST_FILE%"

echo Scanning for PlatformIO builds...

:: Loop through each defined source directory
for %%S in (%SOURCE_DIRS%) do (
    echo.
    echo --- Checking directory: %%S ---
    
    if exist "%%S\*" (
        for /d %%D in ("%%S\*") do (
            if exist "%%D\firmware.bin" (
                set "BOARD_NAME=%%~nxD"
                echo Found firmware for: !BOARD_NAME!
                
                :: Create target directory for this specific board
                mkdir "%DEST_DIR%\!BOARD_NAME!"
                
                :: Copy the 4 required binaries
                copy /y "%%D\bootloader.bin" "%DEST_DIR%\!BOARD_NAME!\" >nul
                copy /y "%%D\partitions.bin" "%DEST_DIR%\!BOARD_NAME!\" >nul
                copy /y "%%D\boot_app0.bin" "%DEST_DIR%\!BOARD_NAME!\" >nul
                copy /y "%%D\firmware.bin" "%DEST_DIR%\!BOARD_NAME!\" >nul
                
                :: Append the folder name to the manifest
                echo !BOARD_NAME!>>"%MANIFEST_FILE%"
            )
        )
    ) else (
        echo Directory not found or empty: %%S
    )
)

echo.
echo Firmware extraction complete! Manifest updated.
pause