@echo off
setlocal

:: Lege den Pfad zur Textdatei fest (im selben Ordner wie dieses Skript)
set "VERSION_FILE=%~dp0verisonDefines.txt"

:: PowerShell aufrufen, um das Datum als YY.MM.DD zu berechnen
for /f "usebackq tokens=*" %%V in (`powershell -NoProfile -Command "Get-Date -Format 'yy.MM.dd'"`) do (
    set "NEW_VERSION=%%V"
)

echo Generierte Versionsnummer: %NEW_VERSION%

:: Schreibe die neue Version in die Datei (inklusive Anfuehrungszeichen, da UpdateVersionStrings das so erwartet)
> "%VERSION_FILE%" echo "%NEW_VERSION%"

echo Die Datei verisonDefines.txt wurde erfolgreich aktualisiert!
echo.

:: CI-sicherer Pause-Befehl
::if not defined CI pause