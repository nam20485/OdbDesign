#!/usr/bin/env pwsh
<#!
Windows environment setup (canonical)
- NVM for Windows everywhere with exact Node pin (from .nvmrc or NODE_VERSION_PIN)
- Optional npm pin via NPM_VERSION_PIN
- Corepack enable (pnpm/yarn)
- Installs: Git, Python3, Google Cloud CLI, GitHub CLI, Terraform, Ansible (pip), global npm CLIs (firebase, angular, CRA, typescript, eslint, prettier, cdktf)
- .NET 9 SDK workloads (wasm-tools, aspire) & Aspire templates (user-local)
Note: Some installers may require admin rights (winget/choco).
!#>

# -----------------------------------------------------------------------------
# Environment variables specify versions to use
# -----------------------------------------------------------------------------
$NODE_VERSION_PIN = '22.18.0'
$NPM_VERSION_PIN = '10.1.0'
$PNPM_VERSION_PIN = '8.11.0'
$YARN_VERSION_PIN = '3.6.0'
$PLAYWRIGHT_CLI = '1.44.1'
$PLAYWRIGHT_BROWSERS = 'chromium, firefox, webkit'
$PWSH_VERSION = '7.4.6'
$GCLOUD_SDK = '463.0.0'
$GH_CLI = '2.37.0'
$TERRAFORM = '1.6.15'
$ANSIBLE = '8.9.0'
$FIREBASE_TOOLS = '11.11.0'
$CDKTF = '0.16.0'
$DOTNET_VERSION_PIN = '9.0.102'
$DOTNET_CHANNEL = '9.0'
$DOTNET_QUALITY = ''

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Write-Host '=== Starting Windows environment setup (Linux parity) ==='

# -----------------------------------------------------------------------------
# Environment variables
# -----------------------------------------------------------------------------
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'
$env:DOTNET_NOLOGO = '1'
$env:ASPNETCORE_ENVIRONMENT = 'Development'



function Test-Command {
    param([Parameter(Mandatory)][string]$Name)
    try { Get-Command $Name -ErrorAction Stop | Out-Null; $true } catch { $false }
}

function Test-IsTrue {
    param([string]$Value)
    if ([string]::IsNullOrWhiteSpace($Value)) { return $false }
    switch -Regex ($Value.ToLowerInvariant()) {
        '^(1|true|yes|y)$' { return $true }
        default { return $false }
    }
}

function Get-PackageManager {
    # Prefer winget on hosted runners; fall back to choco
    if (Test-Command winget) { return 'winget' }
    if (Test-Command choco) { return 'choco' }
    return $null
}

function Install-Git {
    if (Test-Command git) { return }
        $mgr = Get-PackageManager
    if ($mgr -eq 'choco') { choco install git -y --no-progress }
    elseif ($mgr -eq 'winget') { winget install --id Git.Git --silent --accept-source-agreements --accept-package-agreements }
}

# -----------------------------------------------------------------------------
# NVM for Windows + exact Node pin
# -----------------------------------------------------------------------------
function Install-NvmWindows {
    if (Test-Command nvm) { return }
    $wingetAvailable = Test-Command winget
    $chocoAvailable = Test-Command choco

    if ($wingetAvailable) {
        try { winget install --id CoreyButler.NVMforWindows --silent --accept-source-agreements --accept-package-agreements } catch { Write-Warning "winget install of NVM failed: $($_.Exception.Message)" }
        Ensure-NvmInCurrentSession
        if (Test-Command nvm) { return }
    }

    if ($chocoAvailable) {
        try { choco install nvm -y --no-progress } catch { Write-Warning "choco install of nvm failed: $($_.Exception.Message)" }
        # Also ensure the Chocolatey bin folder is on PATH for the current session
        if (($env:ChocolateyInstall) -and (Test-Path $env:ChocolateyInstall)) {
            $chocoBin = Join-Path $env:ChocolateyInstall 'bin'
            if ((Test-Path $chocoBin) -and ($env:PATH -notmatch [regex]::Escape($chocoBin))) { $env:PATH = "$chocoBin;$env:PATH" }
        }
        Ensure-NvmInCurrentSession
        if (Test-Command nvm) { return }
    }

    if (-not ($wingetAvailable -or $chocoAvailable)) {
        Write-Warning 'No package manager found (winget/choco). Download and install NVM for Windows manually from https://github.com/coreybutler/nvm-windows/releases'
    }
}

function Install-Node-With-Nvm {
    Install-NvmWindows
    Ensure-NvmInCurrentSession
    if (-not (Test-Command nvm)) {
        # Wait briefly for installer/shims to settle on hosted runners
        if (-not (Wait-ForNvm -Seconds 30)) { throw 'nvm (Windows) is not available after install.' }
    }

    # Determine desired Node version
    $nodeVersion = $NODE_VERSION_PIN
    if ([string]::IsNullOrWhiteSpace($nodeVersion)) {
        $nvmrcPath = Join-Path -Path (Get-Location) -ChildPath '.nvmrc'
        if (Test-Path $nvmrcPath) {
            $nodeVersion = (Get-Content $nvmrcPath -Raw).Trim()
        }
    }
    if ([string]::IsNullOrWhiteSpace($nodeVersion)) {
        throw 'No exact Node version specified. Set NODE_VERSION_PIN (e.g., NODE_VERSION_PIN=22.18.0) or add a .nvmrc file with the desired version (e.g., 22.18.0).'
    }

    Write-Host "[nvm] Installing Node $nodeVersion"
    nvm install $nodeVersion | Out-Null
    nvm use $nodeVersion | Out-Null

    # Ensure current session PATH sees node (nvm updates symlink at %ProgramFiles%\nodejs)
    $nodeDir = "$env:ProgramFiles\nodejs"
    if ($env:PATH -notmatch [regex]::Escape($nodeDir)) { $env:PATH = "$nodeDir;$env:PATH" }

    # npm pin (optional). Otherwise keep bundled npm for determinism
    if (-not [string]::IsNullOrWhiteSpace($NPM_VERSION_PIN)) {
        Write-Host "[npm] Pinning npm@$($NPM_VERSION_PIN)"
        npm install -g --no-audit --no-fund "npm@$($NPM_VERSION_PIN)" | Out-Null
    }

    if (Test-IsTrue $env:SETUP_MINIMAL) {
        Write-Host 'SETUP_MINIMAL: Skipping Corepack activation'
    }
    else {
        Write-Host '[corepack] Enabling'
        try { corepack enable | Out-Null } catch {}

        # Resolve pnpm version from PNPM_VERSION_PIN or package.json's packageManager/pnpm fields
        $pnpmVersion = $PNPM_VERSION_PIN
        if ([string]::IsNullOrWhiteSpace($pnpmVersion) -and (Test-Path 'package.json')) {
            try {
                $pkg = Get-Content package.json -Raw | ConvertFrom-Json
                if ($pkg.packageManager -and ($pkg.packageManager -match '^pnpm@(.+)$')) { $pnpmVersion = $Matches[1] }
                if ([string]::IsNullOrWhiteSpace($pnpmVersion) -and $pkg.pnpm) { $pnpmVersion = $pkg.pnpm }
            } catch {}
        }
        if (-not [string]::IsNullOrWhiteSpace($pnpmVersion)) {
            try { corepack prepare "pnpm@$pnpmVersion" --activate | Out-Null } catch {}
        } else {
            Write-Warning 'pnpm version not specified; skipping pnpm activation (set PNPM_VERSION_PIN or packageManager in package.json).'
        }

        # Resolve yarn version from YARN_VERSION_PIN or package.json's packageManager
        $yarnVersion = $YARN_VERSION_PIN
        if ([string]::IsNullOrWhiteSpace($yarnVersion) -and (Test-Path 'package.json')) {
            try {
                $pkg2 = Get-Content package.json -Raw | ConvertFrom-Json
                if ($pkg2.packageManager -and ($pkg2.packageManager -match '^yarn@(.+)$')) { $yarnVersion = $Matches[1] }
            } catch {}
        }
        if (-not [string]::IsNullOrWhiteSpace($yarnVersion)) {
            try { corepack prepare "yarn@$yarnVersion" --activate | Out-Null } catch {}
        } else {
            Write-Warning 'yarn version not specified; skipping yarn activation (set YARN_VERSION_PIN or packageManager in package.json).'
        }
    }
}

# -----------------------------------------------------------------------------
# Base tools (Git, Python)
# -----------------------------------------------------------------------------
function Ensure-NvmInCurrentSession {
    # If nvm already resolves, nothing to do
    if (Test-Command nvm) { return }

    # Candidate install locations for nvm.exe (winget/choco defaults)
    $candidates = @()

    # Read machine-scoped environment variables set by installers
    $machineNvmHome = [Environment]::GetEnvironmentVariable('NVM_HOME','Machine')
    $machineNvmSymlink = [Environment]::GetEnvironmentVariable('NVM_SYMLINK','Machine')
    if (-not [string]::IsNullOrWhiteSpace($machineNvmHome)) { $env:NVM_HOME = $env:NVM_HOME ?? $machineNvmHome }
    if (-not [string]::IsNullOrWhiteSpace($machineNvmSymlink)) { $env:NVM_SYMLINK = $env:NVM_SYMLINK ?? $machineNvmSymlink }

    if (-not [string]::IsNullOrWhiteSpace($env:NVM_HOME)) { $candidates += $env:NVM_HOME }
    if (-not [string]::IsNullOrWhiteSpace($env:ChocolateyInstall)) {
        $candidates += @(
            (Join-Path $env:ChocolateyInstall 'bin'),
            (Join-Path $env:ChocolateyInstall 'lib\nvm'),
            (Join-Path $env:ChocolateyInstall 'lib\nvm\tools')
        )
    }
    $candidates += @(
        (Join-Path $env:ProgramFiles 'nvm'),
        (Join-Path ${env:ProgramFiles(x86)} 'nvm'),
        'C:\tools\nvm',
        'C:\nvm',
        'C:\ProgramData\chocolatey\bin',
        'C:\ProgramData\chocolatey\lib\nvm\tools',
        'C:\ProgramData\chocolatey\lib\nvm',
        (Join-Path $env:LOCALAPPDATA 'Programs\nvm'),
        (Join-Path $env:APPDATA 'nvm')
    )

    $found = $false
    foreach ($dir in $candidates | Where-Object { ($_) -and (Test-Path $_) }) {
        $exe = Join-Path $dir 'nvm.exe'
        if (Test-Path $exe) {
            if ($env:PATH -notmatch [regex]::Escape($dir)) { $env:PATH = "$dir;$env:PATH" }
            if ([string]::IsNullOrWhiteSpace($env:NVM_HOME)) { $env:NVM_HOME = $dir }
            if ([string]::IsNullOrWhiteSpace($env:NVM_SYMLINK)) { $env:NVM_SYMLINK = Join-Path $env:ProgramFiles 'nodejs' }
            if (-not (Test-Path $env:NVM_SYMLINK)) { New-Item -ItemType Directory -Path $env:NVM_SYMLINK -Force | Out-Null }
            $found = $true
            break
        }
    }

    if (-not $found) {
        # Fallback: scan common Chocolatey root for nvm.exe as last resort
        $roots = @('C:\ProgramData\chocolatey', $env:ChocolateyInstall) | Where-Object { ($_) -and (Test-Path $_) }
        foreach ($root in $roots) {
            $hit = Get-ChildItem -Path $root -Filter 'nvm.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($hit) {
                $dir = Split-Path -Parent $hit.FullName
                if ($env:PATH -notmatch [regex]::Escape($dir)) { $env:PATH = "$dir;$env:PATH" }
                if ([string]::IsNullOrWhiteSpace($env:NVM_HOME)) { $env:NVM_HOME = $dir }
                if ([string]::IsNullOrWhiteSpace($env:NVM_SYMLINK)) { $env:NVM_SYMLINK = Join-Path $env:ProgramFiles 'nodejs' }
                if (-not (Test-Path $env:NVM_SYMLINK)) { New-Item -ItemType Directory -Path $env:NVM_SYMLINK -Force | Out-Null }
                $found = $true
                break
            }
        }
    }

    # If Chocolatey bin exists, ensure it's present on PATH (shims live here)
    if (($env:ChocolateyInstall) -and (Test-Path $env:ChocolateyInstall)) {
        $chocoBin = Join-Path $env:ChocolateyInstall 'bin'
        if ((Test-Path $chocoBin) -and ($env:PATH -notmatch [regex]::Escape($chocoBin))) { $env:PATH = "$chocoBin;$env:PATH" }
    }
}

function Wait-ForNvm {
    param([int]$Seconds = 30)
    for ($i = 0; $i -lt $Seconds; $i++) {
        Ensure-NvmInCurrentSession
        if (Test-Command nvm) { return $true }
        Start-Sleep -Seconds 1
    }
    return $false
}
function Install-Python3 {
    if (Test-Command python3) { return }
        $mgr = Get-PackageManager
    if ($mgr -eq 'choco') { choco install python -y --no-progress }
    elseif ($mgr -eq 'winget') { winget install --id Python.Python.3.12 --silent --accept-source-agreements --accept-package-agreements }
}

# -----------------------------------------------------------------------------
# Cloud & DevOps tools: gcloud, gh, terraform, ansible
# -----------------------------------------------------------------------------
function Install-GCloud {
    if (Test-Command gcloud) { return }
        $mgr = Get-PackageManager
    if ($mgr -eq 'choco') { choco install googlecloudsdk -y --no-progress }
    elseif ($mgr -eq 'winget') { winget install --id Google.CloudSDK --silent --accept-source-agreements --accept-package-agreements }
}

function Install-GH {
    if (Test-Command gh) { return }
        $mgr = Get-PackageManager
    if ($mgr -eq 'choco') { choco install gh -y --no-progress }
    elseif ($mgr -eq 'winget') { winget install --id GitHub.cli --silent --accept-source-agreements --accept-package-agreements }
}

function Install-Terraform {
    if (Test-Command terraform) { return }
        $mgr = Get-PackageManager
    if ($mgr -eq 'choco') { choco install terraform -y --no-progress }
    elseif ($mgr -eq 'winget') { winget install --id Hashicorp.Terraform --silent --accept-source-agreements --accept-package-agreements }
}

function Install-Ansible {
    if (Test-Command ansible) { return }
    Write-Warning 'Ansible on Windows is best via WSL. Installing via pip (user scope) as best-effort.'
        Install-Python3
    try { pip3 install --user ansible | Out-Null } catch { Write-Warning "pip3 install ansible failed: $($_.Exception.Message)" }
}

# -----------------------------------------------------------------------------
# .NET 9 SDK (user-local) & workloads
# -----------------------------------------------------------------------------
function Install-DotNet9 {
    $dotnetVersion = $null
    try { $dotnetVersion = (dotnet --version) } catch {}
    if (-not ($dotnetVersion -like '9.*')) {
        Write-Host '[dotnet] Installing .NET 9 SDK (user-local)'
        $installer = Join-Path $env:TEMP 'dotnet-install.ps1'
        Invoke-WebRequest -Uri 'https://dot.net/v1/dotnet-install.ps1' -OutFile $installer
        & powershell -ExecutionPolicy Bypass -File $installer -Channel 9.0 -InstallDir "$env:USERPROFILE\.dotnet" -NoPath | Out-Null
        $env:PATH = "$env:USERPROFILE\.dotnet;$env:PATH"
    }
    if (Test-Command dotnet) {
        try { dotnet workload update | Out-Null } catch {}
        foreach ($wl in 'wasm-tools','aspire') { try { dotnet workload install $wl | Out-Null } catch {} }
        try { dotnet new install Aspire.ProjectTemplates | Out-Null } catch {}
    }
}

# -----------------------------------------------------------------------------
# Global npm CLIs (user scope under NVM-managed Node)
# -----------------------------------------------------------------------------
function Install-GlobalNpmCLIs {
    $pkgs = @('firebase-tools','@angular/cli','create-react-app','typescript','eslint','prettier','cdktf-cli')
    foreach ($p in $pkgs) {
    try { npm install -g --no-audit --no-fund $p | Out-Null } catch { Write-Warning "Failed to install ${p}: $($_.Exception.Message)" }
    }
}

# -----------------------------------------------------------------------------
# Execution
# -----------------------------------------------------------------------------
try {
        Install-Git
        Install-Python3
    Install-Node-With-Nvm
        if (Test-IsTrue $env:SETUP_MINIMAL) { Write-Host 'SETUP_MINIMAL: Skipping Google Cloud CLI install' } else { Install-GCloud }
        if (Test-IsTrue $env:SETUP_MINIMAL) { Write-Host 'SETUP_MINIMAL: Skipping GitHub CLI install' } else { Install-GH }
        if (Test-IsTrue $env:SETUP_MINIMAL) { Write-Host 'SETUP_MINIMAL: Skipping Terraform install' } else { Install-Terraform }
        if (Test-IsTrue $env:SETUP_MINIMAL) { Write-Host 'SETUP_MINIMAL: Skipping Ansible install' } else { Install-Ansible }
    if (Test-IsTrue $env:SETUP_MINIMAL) { Write-Host 'SETUP_MINIMAL: Skipping global npm CLI tool installs' } else { Install-GlobalNpmCLIs }
        Install-DotNet9

    Write-Host '\nEnvironment summary:' -ForegroundColor Cyan
    Write-Host ('- .NET SDK: ' + ($(Get-Command dotnet -ErrorAction SilentlyContinue) ? (dotnet --version) : 'Not Installed'))
    Write-Host ('- Node.js: ' + ($(Get-Command node -ErrorAction SilentlyContinue) ? (node -v) : 'Not Installed'))
    Write-Host ('- npm: ' + ($(Get-Command npm -ErrorAction SilentlyContinue) ? (npm -v) : 'Not Installed'))
    Write-Host ('- Python: ' + ($(Get-Command python -ErrorAction SilentlyContinue) ? (python --version) : ($(Get-Command python3 -ErrorAction SilentlyContinue) ? (python3 --version) : 'Not Installed')))
    Write-Host ('- PowerShell: ' + $PSVersionTable.PSVersion.ToString())
    Write-Host ''
    Write-Host ('- Google Cloud CLI: ' + ($(Get-Command gcloud -ErrorAction SilentlyContinue) ? ((gcloud version 2>$null | Select-String "Google Cloud SDK" ) -replace 'Google Cloud SDK ', '') : 'Not Installed'))
    Write-Host ('- Firebase CLI: ' + ($(Get-Command firebase -ErrorAction SilentlyContinue) ? (firebase --version) : 'Not Installed'))
    Write-Host ('- GitHub CLI: ' + ($(Get-Command gh -ErrorAction SilentlyContinue) ? ((gh --version | Select-Object -First 1).Trim()) : 'Not Installed'))
    Write-Host ('- Terraform: ' + ($(Get-Command terraform -ErrorAction SilentlyContinue) ? ((terraform --version | Select-Object -First 1).Trim()) : 'Not Installed'))
    Write-Host ('- Ansible: ' + ($(Get-Command ansible -ErrorAction SilentlyContinue) ? ((ansible --version | Select-Object -First 1).Trim()) : 'Not Installed'))
    Write-Host ('- CDKTF: ' + ($(Get-Command cdktf -ErrorAction SilentlyContinue) ? ((cdktf --version).Trim()) : 'Not Installed'))

    Write-Host '\n=== Windows environment setup complete ===' -ForegroundColor Green
}
catch {
    Write-Error "Setup failed: $($_.Exception.Message)"
    throw
}
