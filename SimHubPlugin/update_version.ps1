$date = Get-Date
$year = $date.ToString("yy")
# ISO 8601 week number calculation for Windows PowerShell compatibility
$week = [System.Globalization.CultureInfo]::InvariantCulture.Calendar.GetWeekOfYear($date, [System.Globalization.CalendarWeekRule]::FirstFourDayWeek, [System.DayOfWeek]::Monday)
$weekStr = $week.ToString("00")
$day = [int]$date.DayOfWeek
if ($day -eq 0) {
    $day = 7
}
$dayStr = $day.ToString("00")
$version = "${year}.${weekStr}.${dayStr}"

$filepath = "VariablesStruct\constants.cs"
if (Test-Path $filepath) {
    $newVersionStr = "public const string pluginVersion = `"$version`";"
    $maxRetries = 5
    $retryCount = 0
    $success = $false
    while (-not $success -and $retryCount -lt $maxRetries) {
        try {
            $content = [System.IO.File]::ReadAllText((Resolve-Path $filepath).Path)
            if (![string]::IsNullOrWhiteSpace($content)) {
                if (-not $content.Contains($newVersionStr)) {
                    $content = $content -replace 'public const string pluginVersion = ".*?";', $newVersionStr
                    [System.IO.File]::WriteAllText((Resolve-Path $filepath).Path, $content)
                }
            }
            $success = $true
        } catch {
            $retryCount++
            Start-Sleep -Milliseconds 500
        }
    }
    if (-not $success) {
        Write-Warning "Failed to update version in constants.cs after $maxRetries retries."
    }
}
