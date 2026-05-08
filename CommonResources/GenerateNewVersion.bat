@echo off
setlocal

:: Lege den Pfad zur Textdatei fest (im selben Ordner wie dieses Skript)
set "VERSION_FILE=%~dp0verisonDefines.txt"

:: PowerShell aufrufen, um das Datum als YY.WW.DD zu berechnen
:: WW = Kalenderwoche nach ISO 8601, DD = Wochentag (01=Mo ... 07=So)
for /f "usebackq tokens=*" %%V in (`powershell -NoProfile -Command "$d=Get-Date; $y=$d.ToString('yy'); $ci=[cultureinfo]::InvariantCulture; $w=$ci.Calendar.GetWeekOfYear($d,[System.Globalization.CalendarWeekRule]::FirstFourDayWeek,[DayOfWeek]::Monday).ToString('D2'); $dow=if($d.DayOfWeek -eq 'Sunday'){7}else{[int]$d.DayOfWeek}; $dowStr=$dow.ToString('D2'); Write-Output ""$y.$w.$dowStr"""`) do (
    set "NEW_VERSION=%%V"
)

echo Generierte Versionsnummer: %NEW_VERSION%

:: Schreibe die neue Version in die Datei (inklusive Anfuehrungszeichen, da UpdateVersionStrings das so erwartet)
> "%VERSION_FILE%" echo "%NEW_VERSION%"

echo Die Datei verisonDefines.txt wurde erfolgreich aktualisiert!
echo.

:: CI-sicherer Pause-Befehl
::if not defined CI pause