@echo off
setlocal enabledelayedexpansion

:: Force the Current Working Directory to be the folder where this script lives
cd /d "%~dp0"

echo ===================================================
echo   DIY FFB Pedal - Master Build Script
echo ===================================================
echo.

:: -----------------------------------------------------
:: Locate PlatformIO
:: -----------------------------------------------------
set "PIO_CMD=pio"
where pio >nul 2>nul
if %errorlevel% neq 0 (
    :: If 'pio' isn't in PATH, look for the default VS Code extension installation
    if exist "%USERPROFILE%\.platformio\penv\Scripts\pio.exe" (
        set "PIO_CMD="%USERPROFILE%\.platformio\penv\Scripts\pio.exe""
    ) else (
        echo [ERROR] PlatformIO 'pio' command not found.
        echo Make sure PlatformIO is installed via VS Code.
        if not defined CI pause
        exit /b 1
    )
)

:: -----------------------------------------------------
:: Locate MSBuild (Visual Studio Compiler)
:: -----------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    if not defined CI pause
    exit /b 1
)

:: Ask vswhere to find the path to the latest MSBuild.exe
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set "MSBUILD_PATH=%%i"
)

if not defined MSBUILD_PATH (
    echo [ERROR] MSBuild.exe not found on this system.
    if not defined CI pause
    exit /b 1
)

:: -----------------------------------------------------
:: Step 0: Update Version Strings
:: -----------------------------------------------------
echo === Step 0: Updating Version Strings ===
if exist "CommonResources\UpdateVersionStrings.bat" (
    cd CommonResources
    call UpdateVersionStrings.bat
    cd ..
) else (
    echo [WARNING] CommonResources\UpdateVersionStrings.bat not found. Skipping.
)
echo.

REM pause

:: -----------------------------------------------------
:: Step 1: Build ESP32 Firmwares
:: -----------------------------------------------------
echo === Step 1: Building ESP32 Environments ===
if exist "ESP32\platformio.ini" (
    echo Cleaning ESP32\.pio folder...
    if exist "ESP32\.pio" rmdir /s /q "ESP32\.pio"
    
    cd ESP32
    call !PIO_CMD! run
    cd ..
) else (
    echo [WARNING] ESP32\platformio.ini not found. Skipping.
)
echo.

:: -----------------------------------------------------
:: Step 2: Build ESP32_master Firmwares
:: -----------------------------------------------------
echo === Step 2: Building ESP32_master Environments ===
if exist "ESP32_master\platformio.ini" (
    echo Cleaning ESP32_master\.pio folder...
    if exist "ESP32_master\.pio" rmdir /s /q "ESP32_master\.pio"
    
    cd ESP32_master
    call !PIO_CMD! run
    cd ..
) else (
    echo [WARNING] ESP32_master\platformio.ini not found. Skipping.
)
echo.

:: -----------------------------------------------------
:: Step 3: Build SimHub Plugin DLL
:: -----------------------------------------------------
echo === Step 3: Compiling SimHub Plugin DLL ===

:: Prüfen, ob die Projektdatei existiert, bevor wir MSBuild aufrufen
if exist "SimHubPlugin\DIYFFBPedalUI.csproj" (
    "%MSBUILD_PATH%" "SimHubPlugin\DIYFFBPedalUI.csproj" -p:Configuration=Release -t:Rebuild
) else (
    echo [ERROR] Die Datei "SimHubPlugin\DIYFFBPedalUI.csproj" wurde nicht gefunden.
    echo Bitte den genauen Dateinamen der .csproj Datei im Skript ueberpruefen!
)

echo.
echo ===================================================
echo                 ALL BUILDS COMPLETE!
echo ===================================================
if not defined CI pause