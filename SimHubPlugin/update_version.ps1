$date = Get-Date
$year = $date.ToString("yy")
$week = (Get-Date -UFormat "%V")
$day = [int]$date.DayOfWeek
if ($day -eq 0) {
    $day = 7
}
$dayStr = $day.ToString("00")
$version = "${year}.${week}.${dayStr}"

$filepath = "VariablesStruct\constants.cs"
if (Test-Path $filepath) {
    $content = Get-Content $filepath -Raw
    $content = $content -replace 'public const string pluginVersion = ".*?";', "public const string pluginVersion = `"$version`";"
    Set-Content -Path $filepath -Value $content
}
