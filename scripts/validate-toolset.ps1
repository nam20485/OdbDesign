#requires -Version 7.0
[CmdletBinding()]
param(
    [string]$ToolsetPath = (Join-Path $PSScriptRoot '..\local_ai_instruction_modules\toolset.selected.json')
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ToolsetPath)) {
    throw "Toolset file not found: $ToolsetPath"
}

$json = Get-Content -Raw -LiteralPath $ToolsetPath | ConvertFrom-Json
$enabled = @($json.enabledTools)
$disabled = @($json.disabledTools)

$enabledCount = if ($enabled) { $enabled.Count } else { 0 }
$disabledCount = if ($disabled) { $disabled.Count } else { 0 }

if ($json.enabledCount -ne $enabledCount) {
    throw "enabledCount ($($json.enabledCount)) does not match enabledTools.Count ($enabledCount)"
}
if ($json.disabledCount -ne $disabledCount) {
    throw "disabledCount ($($json.disabledCount)) does not match disabledTools.Count ($disabledCount)"
}

# Ensure uniqueness in enabled
$dupes = $enabled | Group-Object | Where-Object { $_.Count -gt 1 } | Select-Object -ExpandProperty Name
if ($dupes) {
    throw "Duplicate tool entries found in enabledTools: $($dupes -join ', ')"
}

Write-Host "Toolset validation passed: enabled=$enabledCount disabled=$disabledCount" -ForegroundColor Green
