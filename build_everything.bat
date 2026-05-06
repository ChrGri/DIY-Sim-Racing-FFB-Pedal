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
        pause
        exit /b 1
    )
)

:: -----------------------------------------------------
:: Locate MSBuild (Visual Studio Compiler)
:: -----------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    pause
    exit /b 1
)

:: Ask vswhere to find the path to the latest MSBuild.exe
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set "MSBUILD_PATH=%%i"
)

if not defined MSBUILD_PATH (
    echo [ERROR] MSBuild.exe not found on this system.
    pause
    exit /b 1
)

:: -----------------------------------------------------
:: Step 1: Build ESP32 Firmwares
:: -----------------------------------------------------
echo === Step 1: Building ESP32 Environments ===
if exist "ESP32\platformio.ini" (
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
:: Note: This will automatically trigger copy_firmwares.bat because of our MSBuild Target!
if exist "SimHubPlugin" (
    "!MSBUILD_PATH!" "SimHubPlugin" -p:Configuration=Release -t:Rebuild
) else (
    echo [ERROR] SimHubPlugin folder not found.
)

echo.
echo ===================================================
echo                 ALL BUILDS COMPLETE!
echo ===================================================
pause