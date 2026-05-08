@echo off
setlocal

set VERSION_FILE=%~dp0verisonDefines.txt

:: Resolve absolute paths
for %%i in ("%~dp0..\SimHubPlugin\VariablesStruct\constants.cs")            do set CONSTANTS_FILE=%%~fi
for %%i in ("%~dp0..\ESP32\include\Version_Board.h")                         do set ESP32_FILE=%%~fi
for %%i in ("%~dp0..\ESP32_master\include\Version_Board.h")                  do set ESP32_MASTER_FILE=%%~fi

:: Read version from file (strips quotes and whitespace)
set /p VERSION=<%VERSION_FILE%
set VERSION=%VERSION:"=%

echo Version read: %VERSION%
echo.

:: 1. Update constants.cs
echo Updating: %CONSTANTS_FILE%
powershell -NoProfile -Command ^
  "(Get-Content '%CONSTANTS_FILE%') -replace 'public const string pluginVersion = \"".*?\"";', 'public const string pluginVersion = \""%VERSION%\"";' | Set-Content '%CONSTANTS_FILE%'"

:: 2. Update ESP32\include\Version_Board.h
echo Updating: %ESP32_FILE%
powershell -NoProfile -Command ^
  "(Get-Content '%ESP32_FILE%') -replace 'const char \*DAP_FIRMWARE_VERSION = \"".*?\"";', 'const char *DAP_FIRMWARE_VERSION = \""%VERSION%\"";' | Set-Content '%ESP32_FILE%'"

:: 3. Update ESP32_master\include\Version_Board.h
echo Updating: %ESP32_MASTER_FILE%
powershell -NoProfile -Command ^
  "(Get-Content '%ESP32_MASTER_FILE%') -replace '#define BRIDGE_FIRMWARE_VERSION \"".*?\""', '#define BRIDGE_FIRMWARE_VERSION \""%VERSION%\""' | Set-Content '%ESP32_MASTER_FILE%'"

echo.
echo Done. All versions set to %VERSION%
endlocal
pause
