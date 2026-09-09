param(
    [string]$ClusterName = "k3dcluster",
    [string]$TaskName,
    [string]$StartupScriptPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($TaskName)) {
    $TaskName = "Start k3d cluster $ClusterName"
}

if ([string]::IsNullOrWhiteSpace($StartupScriptPath)) {
    $StartupScriptPath = Join-Path $PSScriptRoot "start-k3d-cluster.ps1"
}

if (-not (Test-Path -LiteralPath $StartupScriptPath)) {
    throw "Startup script not found: $StartupScriptPath"
}

$pwshCommand = Get-Command "pwsh.exe" -ErrorAction SilentlyContinue
$shellPath = if ($null -ne $pwshCommand) { $pwshCommand.Source } else { "powershell.exe" }
$taskArgs = "-NoProfile -ExecutionPolicy Bypass -File `"$StartupScriptPath`" -ClusterName `"$ClusterName`""

$existingTask = Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
if ($null -ne $existingTask) {
    Write-Host "Scheduled task '$TaskName' already exists. Replacing it..."
    Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
}

$action = New-ScheduledTaskAction -Execute $shellPath -Argument $taskArgs
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet `
    -AllowStartIfOnBatteries `
    -DontStopIfGoingOnBatteries `
    -MultipleInstances IgnoreNew `
    -StartWhenAvailable

Register-ScheduledTask `
    -TaskName $TaskName `
    -Action $action `
    -Trigger $trigger `
    -Principal $principal `
    -Settings $settings `
    -Description "Start the k3d cluster '$ClusterName' during Windows boot." | Out-Null

Write-Host "Scheduled task '$TaskName' registered."
