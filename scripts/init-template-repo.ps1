<#
.SYNOPSIS
    Initialize a repository cloned from a template by renaming workspace/devcontainer artifacts.

.DESCRIPTION
    Performs the template-specific renames required by our workflows:
      - Rename the *.code-workspace file to <RepoName>.code-workspace
      - Update .devcontainer/devcontainer.json "name" property to "<RepoName>-devcontainer"

.PARAMETER RepoPath
    Absolute or relative path to the local repository root.

.PARAMETER RepoName
    Name of the repository (used for naming the workspace file and devcontainer name).

.EXAMPLE
    ./scripts/init-template-repo.ps1 -RepoPath ../dynamic_workflows/myrepo -RepoName myrepo

.NOTES
    Safe to run multiple times (idempotent). Will skip steps already satisfied.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$RepoPath,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$RepoName
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Info($msg) { Write-Host $msg -ForegroundColor Cyan }
function Write-Ok($msg) { Write-Host $msg -ForegroundColor Green }
function Write-Warn($msg) { Write-Host $msg -ForegroundColor Yellow }

# Resolve path
$root = Resolve-Path -LiteralPath $RepoPath | Select-Object -ExpandProperty Path
if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw "RepoPath does not exist or is not a directory: $RepoPath"
}

Write-Info "Initializing template artifacts in: $root"

# 1) Workspace file rename
$desiredWorkspace = Join-Path $root ("$RepoName.code-workspace")
$existingWorkspace = Get-ChildItem -LiteralPath $root -Filter '*.code-workspace' -File -ErrorAction SilentlyContinue | Select-Object -First 1

if ($existingWorkspace) {
    if ($existingWorkspace.FullName -ieq $desiredWorkspace) {
        Write-Ok "Workspace file already named correctly: $($existingWorkspace.Name)"
    } else {
        Write-Info "Renaming workspace file '$($existingWorkspace.Name)' -> '$(Split-Path -Leaf $desiredWorkspace)'"
        Rename-Item -LiteralPath $existingWorkspace.FullName -NewName (Split-Path -Leaf $desiredWorkspace)
        Write-Ok "Workspace file renamed."
    }
} else {
    # If none exists, create a minimal one to satisfy structure
    Write-Warn "No .code-workspace file found. Creating minimal workspace file: $(Split-Path -Leaf $desiredWorkspace)"
    $ws = @{ folders = @(@{ path = "." }); settings = @{} } | ConvertTo-Json -Depth 4
    Set-Content -LiteralPath $desiredWorkspace -Value $ws -Encoding UTF8
}

# 2) Devcontainer name update
$devcontainerPath = Join-Path $root '.devcontainer/devcontainer.json'
if (Test-Path -LiteralPath $devcontainerPath -PathType Leaf) {
    Write-Info "Updating devcontainer name in: $devcontainerPath"
    try {
        $json = Get-Content -LiteralPath $devcontainerPath -Raw | ConvertFrom-Json
    } catch {
        throw "Failed to parse JSON from ${devcontainerPath}: $($_.Exception.Message)"
    }
    $desiredName = "$RepoName-devcontainer"
    if ($json.name -ne $desiredName) {
        $json | Add-Member -NotePropertyName name -NotePropertyValue $desiredName -Force
        ($json | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $devcontainerPath -Encoding UTF8
        Write-Ok "Devcontainer name set to: $desiredName"
    } else {
        Write-Ok "Devcontainer name already correct: $desiredName"
    }
} else {
    Write-Warn "Devcontainer file not found. Creating minimal devcontainer at: $devcontainerPath"
    $devDir = Split-Path -Parent $devcontainerPath
    if (-not (Test-Path -LiteralPath $devDir)) { New-Item -ItemType Directory -Path $devDir | Out-Null }
    $minimal = @{ name = "$RepoName-devcontainer"; image = "mcr.microsoft.com/devcontainers/dotnet:9.0"; features = @{} } | ConvertTo-Json -Depth 10
    Set-Content -LiteralPath $devcontainerPath -Value $minimal -Encoding UTF8
}

Write-Ok "Template initialization complete."
