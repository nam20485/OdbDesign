<#
.SYNOPSIS
    Sets up vcpkg binary cache with GitHub Packages for local development.

.DESCRIPTION
    This script configures your local machine to use the OdbDesign vcpkg binary cache
    stored in GitHub Packages. This allows you to download pre-built dependencies
    instead of compiling them from source, significantly speeding up builds.

.PARAMETER PAT
    Your GitHub Personal Access Token with read:packages scope.
    If not provided, the script will prompt for it.

.PARAMETER Username
    Your GitHub username. Defaults to 'nam20485'.

.PARAMETER FeedUrl
    The NuGet feed URL. Defaults to the OdbDesign GitHub Packages feed.

.EXAMPLE
    .\setup-vcpkg-cache.ps1 -PAT "ghp_xxxxxxxxxxxx"

.EXAMPLE
    .\setup-vcpkg-cache.ps1
    # Will prompt for PAT interactively

.NOTES
    After running this script, set the environment variable before building:
    $env:VCPKG_BINARY_SOURCES = "clear;nugetconfig,$pwd/nuget.config,read"
    
    Or add it permanently to your PowerShell profile.
#>

param(
    [Parameter(Mandatory = $false)]
    [string]$PAT,
    
    [Parameter(Mandatory = $false)]
    [string]$Username = "nam20485",
    
    [Parameter(Mandatory = $false)]
    [string]$FeedUrl = "https://nuget.pkg.github.com/nam20485/index.json",
    
    [Parameter(Mandatory = $false)]
    [string]$SourceName = "GitHubPackages-OdbDesign"
)

$ErrorActionPreference = "Stop"

Write-Host "=== vcpkg Binary Cache Setup for OdbDesign ===" -ForegroundColor Cyan
Write-Host ""

# Prompt for PAT if not provided
if (-not $PAT)
{
    Write-Host "Enter your GitHub Personal Access Token (needs read:packages scope):" -ForegroundColor Yellow
    $secureToken = Read-Host -AsSecureString
    $PAT = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
        [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureToken)
    )
}

if (-not $PAT)
{
    Write-Error "PAT is required. Create one at https://github.com/settings/tokens with read:packages scope."
    exit 1
}

# Find vcpkg
$vcpkgRoot = $env:VCPKG_ROOT
if (-not $vcpkgRoot)
{
    # Try common locations
    $possiblePaths = @(
        "C:\vcpkg",
        "C:\src\vcpkg",
        "$env:USERPROFILE\vcpkg",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\VC\vcpkg"
    )
    foreach ($path in $possiblePaths)
    {
        $resolved = Resolve-Path $path -ErrorAction SilentlyContinue
        if ($resolved -and (Test-Path "$resolved\vcpkg.exe"))
        {
            $vcpkgRoot = $resolved.Path
            break
        }
    }
}

if (-not $vcpkgRoot -or -not (Test-Path "$vcpkgRoot\vcpkg.exe"))
{
    Write-Error "Could not find vcpkg. Set VCPKG_ROOT environment variable or install vcpkg."
    exit 1
}

Write-Host "Found vcpkg at: $vcpkgRoot" -ForegroundColor Green

# Fetch nuget.exe using vcpkg
Write-Host "Fetching NuGet executable..." -ForegroundColor Cyan
$nugetOutput = & "$vcpkgRoot\vcpkg.exe" fetch nuget 2>&1
$nugetPath = ($nugetOutput | Select-Object -Last 1).Trim()

if (-not (Test-Path $nugetPath))
{
    Write-Error "Failed to fetch nuget.exe. Output: $nugetOutput"
    exit 1
}

Write-Host "Using NuGet at: $nugetPath" -ForegroundColor Green

# Check if source already exists (by name or URL)
$existingSources = & $nugetPath sources list 2>&1

# Remove source by name if it exists
if ($existingSources -match $SourceName)
{
    Write-Host "Removing existing source '$SourceName'..." -ForegroundColor Yellow
    & $nugetPath sources Remove -Name $SourceName 2>&1 | Out-Null
}

# Also check if the URL is registered under a different name and remove it
$sourceLines = $existingSources -split "`n"
for ($i = 0; $i -lt $sourceLines.Count; $i++)
{
    if ($sourceLines[$i] -match $FeedUrl)
    {
        # URL found, get the source name from the previous line
        if ($i -gt 0 -and $sourceLines[$i - 1] -match '^\s*\d+\.\s+(\S+)')
        {
            $existingName = $Matches[1]
            if ($existingName -ne $SourceName)
            {
                Write-Host "Removing existing source '$existingName' (same URL)..." -ForegroundColor Yellow
                & $nugetPath sources Remove -Name $existingName 2>&1 | Out-Null
            }
        }
    }
}

# Add the NuGet source with credentials
Write-Host "Adding NuGet source '$SourceName'..." -ForegroundColor Cyan
& $nugetPath sources Add `
    -Name $SourceName `
    -Source $FeedUrl `
    -UserName $Username `
    -Password $PAT `
    -StorePasswordInClearText

if ($LASTEXITCODE -ne 0)
{
    Write-Error "Failed to add NuGet source."
    exit 1
}

Write-Host ""
Write-Host "=== NuGet Source Added! ===" -ForegroundColor Green
Write-Host ""

# Get the repo root (where nuget.config is)
$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not (Test-Path "$repoRoot\nuget.config"))
{
    $repoRoot = (Get-Location).Path
}
$nugetConfigPath = Join-Path $repoRoot "nuget.config"
$envVarValue = "clear;nugetconfig,$nugetConfigPath,read"

Write-Host "To use the binary cache, you need to set VCPKG_BINARY_SOURCES environment variable." -ForegroundColor Yellow
Write-Host ""
Write-Host "Value: " -NoNewline -ForegroundColor Cyan
Write-Host $envVarValue -ForegroundColor White
Write-Host ""

# Ask if user wants to add to profile
$addToProfile = Read-Host "Would you like to add this to your PowerShell profile? (Y/n)"

if ($addToProfile -eq '' -or $addToProfile -match '^[Yy]')
{
    # Ensure profile directory exists
    $profileDir = Split-Path -Parent $PROFILE
    if (-not (Test-Path $profileDir))
    {
        New-Item -ItemType Directory -Path $profileDir -Force | Out-Null
    }
    
    # Check if already in profile
    $profileContent = ""
    if (Test-Path $PROFILE)
    {
        $profileContent = Get-Content $PROFILE -Raw -ErrorAction SilentlyContinue
    }
    
    $lineToAdd = "`$env:VCPKG_BINARY_SOURCES = `"$envVarValue`""
    
    if ($profileContent -match "VCPKG_BINARY_SOURCES")
    {
        Write-Host "VCPKG_BINARY_SOURCES already exists in your profile." -ForegroundColor Yellow
        $overwrite = Read-Host "Do you want to update it? (Y/n)"
        if ($overwrite -eq '' -or $overwrite -match '^[Yy]')
        {
            # Remove existing line and add new one
            $newContent = ($profileContent -split "`n" | Where-Object { $_ -notmatch "VCPKG_BINARY_SOURCES" }) -join "`n"
            $newContent = $newContent.TrimEnd() + "`n`n# vcpkg binary cache (GitHub Packages)`n$lineToAdd`n"
            Set-Content -Path $PROFILE -Value $newContent
            Write-Host "Updated VCPKG_BINARY_SOURCES in profile: $PROFILE" -ForegroundColor Green
        }
    }
    else
    {
        # Add to profile
        Add-Content -Path $PROFILE -Value "`n# vcpkg binary cache (GitHub Packages)`n$lineToAdd"
        Write-Host "Added VCPKG_BINARY_SOURCES to profile: $PROFILE" -ForegroundColor Green
    }
    
    # Also set it for current session
    $env:VCPKG_BINARY_SOURCES = $envVarValue
    Write-Host "Also set for current session." -ForegroundColor Green
}
else
{
    Write-Host ""
    Write-Host "To set manually, run:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  `$env:VCPKG_BINARY_SOURCES = `"$envVarValue`"" -ForegroundColor White
    Write-Host ""
    Write-Host "Or add to your profile:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Add-Content `$PROFILE '`$env:VCPKG_BINARY_SOURCES = `"$envVarValue`"'" -ForegroundColor White
}

Write-Host ""
Write-Host "=== Setup Complete! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Your local builds will now download cached packages from GitHub Packages" -ForegroundColor Cyan
Write-Host "instead of compiling from source (after CI populates the cache)." -ForegroundColor Cyan
