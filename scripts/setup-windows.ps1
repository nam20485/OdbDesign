#!/usr/bin/env powershell

#
# setup-windows.ps1 - Windows setup script for OdbDesign
#
# This script sets up the development environment for building OdbDesign on Windows.
# It installs the required dependencies and configures the build environment.
#

param(
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg",
    [switch]$SkipVcpkgInstall,
    [switch]$Force,
    [switch]$Help
)

function Show-Help {
    Write-Host "OdbDesign Windows Setup Script" -ForegroundColor Green
    Write-Host ""
    Write-Host "Usage: .\setup-windows.ps1 [OPTIONS]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -VcpkgRoot <path>     Specify vcpkg installation directory (default: $env:USERPROFILE\vcpkg)"
    Write-Host "  -SkipVcpkgInstall     Skip vcpkg installation if already present"
    Write-Host "  -Force               Force reinstall of components"
    Write-Host "  -Help                Show this help message"
    Write-Host ""
    Write-Host "This script will:"
    Write-Host "  1. Check for required prerequisites"
    Write-Host "  2. Install or update vcpkg"
    Write-Host "  3. Set up environment variables"
    Write-Host "  4. Apply necessary patches"
    Write-Host ""
}

function Test-Command {
    param([string]$Command)
    try {
        if (Get-Command $Command -ErrorAction SilentlyContinue) {
            return $true
        }
    }
    catch {
        return $false
    }
    return $false
}

function Test-VisualStudioBuildTools {
    # Check for Visual Studio or Build Tools
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $installations = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
        return $installations.Count -gt 0
    }
    return $false
}

function Write-StatusMessage {
    param([string]$Message, [string]$Status = "INFO")
    $color = switch ($Status) {
        "SUCCESS" { "Green" }
        "WARNING" { "Yellow" }
        "ERROR" { "Red" }
        default { "White" }
    }
    Write-Host "[$Status] $Message" -ForegroundColor $color
}

function Install-Vcpkg {
    param([string]$InstallPath)
    
    Write-StatusMessage "Installing vcpkg to $InstallPath..." "INFO"
    
    if (Test-Path $InstallPath) {
        if ($Force) {
            Write-StatusMessage "Removing existing vcpkg installation..." "WARNING"
            Remove-Item -Path $InstallPath -Recurse -Force
        } else {
            Write-StatusMessage "vcpkg already exists at $InstallPath. Use -Force to reinstall." "WARNING"
            return $false
        }
    }
    
    try {
        # Clone vcpkg
        Write-StatusMessage "Cloning vcpkg repository..." "INFO"
        git clone https://github.com/microsoft/vcpkg.git $InstallPath
        
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to clone vcpkg repository"
        }
        
        # Bootstrap vcpkg
        Write-StatusMessage "Bootstrapping vcpkg..." "INFO"
        $bootstrapScript = Join-Path $InstallPath "bootstrap-vcpkg.bat"
        & $bootstrapScript
        
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to bootstrap vcpkg"
        }
        
        Write-StatusMessage "vcpkg installed successfully!" "SUCCESS"
        return $true
    }
    catch {
        Write-StatusMessage "Failed to install vcpkg: $_" "ERROR"
        return $false
    }
}

function Set-EnvironmentVariables {
    param([string]$VcpkgPath)
    
    Write-StatusMessage "Setting environment variables..." "INFO"
    
    # Set VCPKG_ROOT environment variable
    [Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgPath, [EnvironmentVariableTarget]::User)
    $env:VCPKG_ROOT = $VcpkgPath
    
    Write-StatusMessage "VCPKG_ROOT set to: $VcpkgPath" "SUCCESS"
}

function Apply-VcpkgPatches {
    Write-StatusMessage "Checking for vcpkg patches..." "INFO"
    
    $patchScript = Join-Path $PSScriptRoot "patch-vcpkg-install.ps1"
    if (Test-Path $patchScript) {
        Write-StatusMessage "Applying vcpkg patches..." "INFO"
        try {
            & $patchScript
            if ($LASTEXITCODE -eq 0) {
                Write-StatusMessage "vcpkg patches applied successfully!" "SUCCESS"
            } else {
                Write-StatusMessage "vcpkg patches script completed with warnings" "WARNING"
            }
        }
        catch {
            Write-StatusMessage "Failed to apply vcpkg patches: $_" "WARNING"
        }
    } else {
        Write-StatusMessage "No vcpkg patches found to apply" "INFO"
    }
}

function Test-Prerequisites {
    Write-StatusMessage "Checking prerequisites..." "INFO"
    
    $allGood = $true
    
    # Check Git
    if (Test-Command "git") {
        Write-StatusMessage "Git: Found" "SUCCESS"
    } else {
        Write-StatusMessage "Git: Not found - Please install Git from https://git-scm.com/" "ERROR"
        $allGood = $false
    }
    
    # Check CMake
    if (Test-Command "cmake") {
        $cmakeVersion = cmake --version | Select-Object -First 1
        Write-StatusMessage "CMake: Found ($cmakeVersion)" "SUCCESS"
    } else {
        Write-StatusMessage "CMake: Not found - Please install CMake from https://cmake.org/" "ERROR"
        $allGood = $false
    }
    
    # Check Ninja
    if (Test-Command "ninja") {
        Write-StatusMessage "Ninja: Found" "SUCCESS"
    } else {
        Write-StatusMessage "Ninja: Not found - Please install Ninja or use Visual Studio generator" "WARNING"
    }
    
    # Check Visual Studio Build Tools
    if (Test-VisualStudioBuildTools) {
        Write-StatusMessage "Visual Studio Build Tools: Found" "SUCCESS"
    } else {
        Write-StatusMessage "Visual Studio Build Tools: Not found - Please install Visual Studio or Build Tools" "ERROR"
        Write-StatusMessage "Download from: https://visualstudio.microsoft.com/downloads/" "INFO"
        $allGood = $false
    }
    
    return $allGood
}

function Show-BuildInstructions {
    Write-Host ""
    Write-Host "=================================" -ForegroundColor Green
    Write-Host "Setup Complete!" -ForegroundColor Green
    Write-Host "=================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "To build OdbDesign, run the following commands:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "For Release build:" -ForegroundColor White
    Write-Host "  cmake --preset x64-release" -ForegroundColor Cyan
    Write-Host "  cmake --build --preset x64-release" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "For Debug build:" -ForegroundColor White
    Write-Host "  cmake --preset x64-debug" -ForegroundColor Cyan
    Write-Host "  cmake --build --preset x64-debug" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To run tests:" -ForegroundColor White
    Write-Host "  ctest --preset x64-release" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Build output will be in:" -ForegroundColor White
    Write-Host "  out\build\x64-release\" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Note: You may need to restart your terminal or IDE to pick up" -ForegroundColor Yellow
    Write-Host "the new environment variables." -ForegroundColor Yellow
    Write-Host ""
}

# Main script execution
if ($Help) {
    Show-Help
    exit 0
}

Write-Host "OdbDesign Windows Setup Script" -ForegroundColor Green
Write-Host "===============================" -ForegroundColor Green
Write-Host ""

# Check prerequisites
if (-not (Test-Prerequisites)) {
    Write-StatusMessage "Prerequisites check failed. Please install missing components and try again." "ERROR"
    exit 1
}

# Install or update vcpkg
if (-not $SkipVcpkgInstall) {
    if (-not (Test-Path $VcpkgRoot) -or $Force) {
        if (-not (Install-Vcpkg -InstallPath $VcpkgRoot)) {
            Write-StatusMessage "Failed to install vcpkg. Exiting." "ERROR"
            exit 1
        }
    } else {
        Write-StatusMessage "vcpkg already exists at $VcpkgRoot. Use -Force to reinstall or -SkipVcpkgInstall to skip." "INFO"
    }
} else {
    Write-StatusMessage "Skipping vcpkg installation as requested." "INFO"
}

# Set environment variables
Set-EnvironmentVariables -VcpkgPath $VcpkgRoot

# Apply patches
Apply-VcpkgPatches

# Show build instructions
Show-BuildInstructions

Write-StatusMessage "Windows setup completed successfully!" "SUCCESS"