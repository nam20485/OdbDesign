# Ensures NVM for Windows is available in the current session, then runs the upstream setup script.
# All installation logic happens inside this script to comply with the self-contained requirement.

param(
  [string]$UpstreamScriptPath = (Join-Path (Get-Location) 'scripts\setup-environment.ps1')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-Command { param([Parameter(Mandatory)][string]$Name) try { Get-Command $Name -ErrorAction Stop | Out-Null; $true } catch { $false } }

function Set-NvmOnPath {
  # Read machine-scoped env that installers commonly set
  $machineNvmHome = [Environment]::GetEnvironmentVariable('NVM_HOME','Machine')
  $machineNvmSymlink = [Environment]::GetEnvironmentVariable('NVM_SYMLINK','Machine')
  if ($machineNvmHome -and -not $env:NVM_HOME) { $env:NVM_HOME = $machineNvmHome }
  if ($machineNvmSymlink -and -not $env:NVM_SYMLINK) { $env:NVM_SYMLINK = $machineNvmSymlink }

  $candidates = @()
  if ($env:NVM_HOME) { $candidates += $env:NVM_HOME }
  if ($env:ChocolateyInstall) {
    $candidates += @(
      (Join-Path $env:ChocolateyInstall 'bin'),
      (Join-Path $env:ChocolateyInstall 'lib\nvm'),
      (Join-Path $env:ChocolateyInstall 'lib\nvm\tools'),
      (Join-Path $env:ChocolateyInstall 'lib\nvm.install\tools')
    )
  }
  $candidates += @(
    (Join-Path $env:ProgramFiles 'nvm'),
    (Join-Path ${env:ProgramFiles(x86)} 'nvm'),
    'C:\\tools\\nvm',
    'C:\\nvm',
    'C:\\ProgramData\\chocolatey\\bin',
    'C:\\ProgramData\\chocolatey\\lib\\nvm\\tools',
    'C:\\ProgramData\\chocolatey\\lib\\nvm',
    'C:\\ProgramData\\chocolatey\\lib\\nvm.install\\tools',
    (Join-Path $env:LOCALAPPDATA 'Programs\nvm'),
    (Join-Path $env:APPDATA 'nvm'),
    (Join-Path $HOME '.nvm')
  )

  foreach ($dir in ($candidates | Where-Object { $_ -and (Test-Path $_) })) {
    $exe = Join-Path $dir 'nvm.exe'
    if (Test-Path $exe) {
      if ($env:Path -notmatch [regex]::Escape($dir)) { $env:Path = "$dir;" + $env:Path }
      if (-not $env:NVM_HOME) { $env:NVM_HOME = $dir }
      if (-not $env:NVM_SYMLINK) { $env:NVM_SYMLINK = (Join-Path $env:ProgramFiles 'nodejs') }
      if (-not (Test-Path $env:NVM_SYMLINK)) { New-Item -ItemType Directory -Path $env:NVM_SYMLINK -Force | Out-Null }
      return $true
    }
  }
  # Ensure Chocolatey bin is on PATH for shims
  if ($env:ChocolateyInstall) {
    $chocoBin = Join-Path $env:ChocolateyInstall 'bin'
    if ((Test-Path $chocoBin) -and ($env:PATH -notmatch [regex]::Escape($chocoBin))) { $env:PATH = "$chocoBin;" + $env:PATH }
  }
  return (Test-Command nvm)
}

function Install-Nvm-PortableFallback {
  $version = '1.2.2' # Align with Chocolatey package
  $zipUrl = "https://github.com/coreybutler/nvm-windows/releases/download/$version/nvm-noinstall.zip"
  $tempZip = Join-Path $env:TEMP "nvm-$version.zip"
  $target = Join-Path $HOME '.nvm'
  if (-not (Test-Path $target)) { New-Item -ItemType Directory -Path $target -Force | Out-Null }
  Invoke-WebRequest -Uri $zipUrl -OutFile $tempZip -UseBasicParsing
  Expand-Archive -Path $tempZip -DestinationPath $target -Force
  Remove-Item $tempZip -ErrorAction SilentlyContinue
  $env:NVM_HOME = $target
  if ($env:Path -notmatch [regex]::Escape($target)) { $env:Path = "$target;" + $env:Path }
  if (-not $env:NVM_SYMLINK) { $env:NVM_SYMLINK = (Join-Path $env:ProgramFiles 'nodejs') }
  if (-not (Test-Path $env:NVM_SYMLINK)) { New-Item -ItemType Directory -Path $env:NVM_SYMLINK -Force | Out-Null }
}

Write-Host '==[bootstrap]== Ensuring NVM for Windows is available'

if (-not (Set-NvmOnPath)) {
  $wingetOk = Test-Command winget
  if ($wingetOk) {
    try { winget install --id CoreyButler.NVMforWindows --silent --accept-source-agreements --accept-package-agreements | Out-Null } catch { Write-Warning "winget NVM install failed: $($_.Exception.Message)" }
  }
}

if (-not (Set-NvmOnPath)) {
  $chocoOk = Test-Command choco
  if ($chocoOk) {
    try { choco install nvm -y --no-progress | Out-Null } catch { Write-Warning "choco nvm install failed: $($_.Exception.Message)" }
  }
}

if (-not (Set-NvmOnPath)) {
  # Some choco packages place nvm-setup.exe as .ignore; attempt to run it silently
  $cands = @(
    'C:\\ProgramData\\chocolatey\\lib\\nvm.install\\tools\\nvm-setup.exe',
    'C:\\ProgramData\\chocolatey\\lib\\nvm.install\\tools\\nvm-setup.exe.ignore'
  )
  foreach ($p in $cands) {
    if (Test-Path $p) {
      $exe = $p
      if ($exe.EndsWith('.ignore')) {
        $real = $exe -replace '\\.ignore$',''
        try { Copy-Item -Path $exe -Destination $real -Force } catch {}
        if (Test-Path $real) { $exe = $real }
      }
      try { Start-Process -FilePath $exe -ArgumentList '/SILENT' -Wait -NoNewWindow } catch { Write-Warning "Running nvm-setup.exe failed: $($_.Exception.Message)" }
      break
    }
  }
}

if (-not (Ensure-NvmOnPath)) {
  Write-Host '==[bootstrap]== Falling back to portable nvm-noinstall.zip'
  Install-Nvm-PortableFallback
  Set-NvmOnPath | Out-Null
}

if (-not (Test-Command nvm)) { throw 'nvm is not available after bootstrap.' }

Write-Host '==[bootstrap]== nvm available:' (nvm version)

# Now run upstream setup script (it will detect nvm and proceed)
& $UpstreamScriptPath
$code = $LASTEXITCODE
if ($code -ne 0) { exit $code }
