<#
.SYNOPSIS
  Logs the joystick axes exactly as Windows (winmm / joy.cpl) sees them, for
  every attached DIY FFB pedal / bridge gamepad (Espressif VID 0x303A).

.DESCRIPTION
  Polls joyGetPosEx at ~1 kHz and writes one CSV row whenever any axis of a
  device changes. Each row carries hostTimeUnixMs, the same clock the SimHub
  plugin writes into the pedal state log (column hostTimeUnixMs), so both logs
  can be lined up sample by sample.

  Axis naming follows winmm: X, Y, Z, R, U, V. The bridge maps
  clutch=Ry, brake=Rx, throttle=Rz; a standalone pedal reports on its own
  single device. If unsure which column is which, press each pedal once at the
  start of the capture.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File Log-Joystick.ps1 -Seconds 60
#>
param(
    [int]$Seconds = 60,
    [string]$OutFile = ("JoystickLog_{0}.csv" -f (Get-Date -Format "yyyyMMdd_HHmmss")),
    [int]$VendorId = 0x303A
)

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class WinMmJoy
{
    [StructLayout(LayoutKind.Sequential)]
    public struct JOYINFOEX
    {
        public int dwSize, dwFlags, dwXpos, dwYpos, dwZpos, dwRpos, dwUpos, dwVpos,
                   dwButtons, dwButtonNumber, dwPOV, dwReserved1, dwReserved2;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct JOYCAPSW
    {
        public ushort wMid, wPid;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string szPname;
        public int wXmin, wXmax, wYmin, wYmax, wZmin, wZmax, wNumButtons, wPeriodMin, wPeriodMax,
                   wRmin, wRmax, wUmin, wUmax, wVmin, wVmax, wCaps, wMaxAxes, wNumAxes, wMaxButtons;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string szRegKey;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)] public string szOEMVxD;
    }

    public const int JOY_RETURNALL = 0xFF;

    [DllImport("winmm.dll")] public static extern int joyGetNumDevs();
    [DllImport("winmm.dll", CharSet = CharSet.Unicode)]
    public static extern int joyGetDevCapsW(IntPtr uJoyID, ref JOYCAPSW caps, int cbjc);
    [DllImport("winmm.dll")] public static extern int joyGetPosEx(int uJoyID, ref JOYINFOEX pji);
    [DllImport("winmm.dll")] public static extern int timeBeginPeriod(int uPeriod);
    [DllImport("winmm.dll")] public static extern int timeEndPeriod(int uPeriod);

    public static JOYINFOEX Read(int id, out int result)
    {
        JOYINFOEX info = new JOYINFOEX();
        info.dwSize = Marshal.SizeOf(typeof(JOYINFOEX));
        info.dwFlags = JOY_RETURNALL;
        result = joyGetPosEx(id, ref info);
        return info;
    }
}
"@

# find all matching devices
$devices = @()
$numDevs = [WinMmJoy]::joyGetNumDevs()
for ($id = 0; $id -lt $numDevs; $id++) {
    $caps = New-Object WinMmJoy+JOYCAPSW
    if ([WinMmJoy]::joyGetDevCapsW([IntPtr]$id, [ref]$caps, [System.Runtime.InteropServices.Marshal]::SizeOf($caps)) -ne 0) { continue }
    $res = 0
    [void][WinMmJoy]::Read($id, [ref]$res)
    if ($res -ne 0) { continue }   # id not connected
    if ($caps.wMid -ne $VendorId) { continue }
    $devices += [pscustomobject]@{ Id = $id; Pid = $caps.wPid; Name = $caps.szPname }
}

if ($devices.Count -eq 0) {
    Write-Error ("No connected joystick with vendor id 0x{0:X4} found." -f $VendorId)
    exit 1
}
foreach ($d in $devices) {
    Write-Host ("Logging joyId {0}: PID 0x{1:X4} '{2}'" -f $d.Id, $d.Pid, $d.Name)
}

$writer = New-Object System.IO.StreamWriter($OutFile, $false)
$writer.WriteLine("hostTimeUnixMs,joyId,pid,X,Y,Z,R,U,V")
$inv = [System.Globalization.CultureInfo]::InvariantCulture
$epoch = New-Object DateTime(1970, 1, 1, 0, 0, 0, [DateTimeKind]::Utc)
$last = @{}
$rows = 0

[void][WinMmJoy]::timeBeginPeriod(1)
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($Seconds)
    Write-Host ("Capturing for {0} s to {1} ..." -f $Seconds, $OutFile)
    while ([DateTime]::UtcNow -lt $deadline) {
        foreach ($d in $devices) {
            $res = 0
            $j = [WinMmJoy]::Read($d.Id, [ref]$res)
            if ($res -ne 0) { continue }
            $key = "{0},{1},{2},{3},{4},{5}" -f $j.dwXpos, $j.dwYpos, $j.dwZpos, $j.dwRpos, $j.dwUpos, $j.dwVpos
            if ($last[$d.Id] -eq $key) { continue }
            $last[$d.Id] = $key
            $t = ([DateTime]::UtcNow - $epoch).TotalMilliseconds.ToString("F3", $inv)
            $writer.WriteLine(("{0},{1},0x{2:X4},{3}" -f $t, $d.Id, $d.Pid, $key))
            $rows++
        }
        [System.Threading.Thread]::Sleep(1)
    }
}
finally {
    [void][WinMmJoy]::timeEndPeriod(1)
    $writer.Close()
    Write-Host ("Done, {0} rows written to {1}" -f $rows, (Resolve-Path $OutFile))
}
