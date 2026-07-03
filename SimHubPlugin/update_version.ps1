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
    try {
        $content = Get-Content $filepath -Raw -ErrorAction Stop
        if (![string]::IsNullOrWhiteSpace($content)) {
            $content = $content -replace 'public const string pluginVersion = ".*?";', "public const string pluginVersion = `"$version`";"
            Set-Content -Path $filepath -Value $content
        }
    } catch {
        Write-Warning "Failed to update version in constants.cs: $_"
    }
}
