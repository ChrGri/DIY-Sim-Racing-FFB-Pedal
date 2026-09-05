@echo off
setlocal
cd /d "%~dp0"

:: Check for administrative permissions
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] Administrative privileges required to trace the elevated SimHub process.
    echo [INFO] Prompting for UAC elevation...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

set "PLUGIN_DIR=c:\Users\chris\OneDrive\Desktop\GIT\DIY-Sim-Racing-FFB-Pedal_Takeover_From_V7\SimHubPlugin"
set "PERFVIEW=%PLUGIN_DIR%\PerfView.exe"
set "OUTPUT=%PLUGIN_DIR%\SimHubProfile.etl"

echo =======================================================================
echo      SimHub / DiyFfbPedal Hot Path Profiler (Microsoft PerfView)
echo =======================================================================
echo.
echo Target Process: SimHubWPF.exe
echo Target Plugin:  DiyFfbPedal.dll
echo Output File:    %OUTPUT%.zip
echo.
echo [1/2] Starting 20-second CPU sampling and .NET CLR event collection...
echo       (SimHub is running with PID 78376. Please trigger pedal/game events)
echo.

"%PERFVIEW%" /AcceptEULA /NoGui /Zip:true /MaxCollectSec:20 /DataFile:"%OUTPUT%" collect

echo.
echo [2/2] Trace collection complete!
echo Opening PerfView with CPU Stacks...
echo.
echo * TIP: In PerfView's CPU Stacks window:
echo   - Type 'DiyFfbPedal' in the 'IncFilter' box to isolate your plugin's hot paths.
echo   - Double click any method to see caller/callee trees and source line cost.
echo.

start "" "%PERFVIEW%" "%OUTPUT%.zip"

pause
