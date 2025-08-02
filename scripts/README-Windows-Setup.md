# Windows Setup Scripts

This directory contains automated setup scripts for the OdbDesign project on Windows.

## Available Scripts

### setup-windows.ps1 (Recommended)
PowerShell script with advanced features and better error handling.

**Usage:**
```powershell
.\setup-windows.ps1 [OPTIONS]
```

**Options:**
- `-VcpkgRoot <path>` - Specify vcpkg installation directory (default: `$env:USERPROFILE\vcpkg`)
- `-SkipVcpkgInstall` - Skip vcpkg installation if already present
- `-Force` - Force reinstall of components
- `-Help` - Show help message

**Examples:**
```powershell
# Basic setup
.\setup-windows.ps1

# Custom vcpkg location
.\setup-windows.ps1 -VcpkgRoot "C:\dev\vcpkg"

# Force reinstall
.\setup-windows.ps1 -Force

# Skip vcpkg if already installed
.\setup-windows.ps1 -SkipVcpkgInstall
```

### setup-windows.bat (Alternative)
Batch script for compatibility with older systems or restricted environments.

**Usage:**
```cmd
setup-windows.bat [OPTIONS]
```

**Options:**
- `--vcpkg-root <path>` - Specify vcpkg installation directory
- `--force` - Force reinstall of components
- `--skip-vcpkg` - Skip vcpkg installation
- `--help` - Show help message

**Examples:**
```cmd
# Basic setup
setup-windows.bat

# Custom vcpkg location
setup-windows.bat --vcpkg-root "C:\dev\vcpkg"

# Force reinstall
setup-windows.bat --force
```

## What These Scripts Do

1. **Check Prerequisites**: Verify that required tools are installed:
   - Git
   - CMake (required)
   - Visual Studio Build Tools (required)
   - Ninja (optional, but recommended)

2. **Install vcpkg**: Download and bootstrap Microsoft's vcpkg package manager if not already present.

3. **Set Environment Variables**: Configure `VCPKG_ROOT` environment variable for the current user.

4. **Apply Patches**: Apply any necessary vcpkg patches using the existing `patch-vcpkg-install.ps1` script.

5. **Provide Build Instructions**: Display commands to build the project after setup completion.

## Prerequisites

Before running these scripts, ensure you have:

- **Git**: Download from [git-scm.com](https://git-scm.com/)
- **Visual Studio 2019/2022** or **Build Tools for Visual Studio**: Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/downloads/)
- **CMake**: Download from [cmake.org](https://cmake.org/download/)
- **Ninja** (optional): Download from [ninja-build.org](https://ninja-build.org/)

## After Setup

Once the setup is complete, you can build the project using:

```cmd
# Configure for Release build
cmake --preset x64-release

# Build the project
cmake --build --preset x64-release

# Run tests
ctest --preset x64-release
```

Build output will be located in `out\build\x64-release\`.

## Troubleshooting

### PowerShell Execution Policy
If you get an execution policy error when running the PowerShell script:

```powershell
# Run once as Administrator
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# Or run the script with bypass
powershell -ExecutionPolicy Bypass -File setup-windows.ps1
```

### Missing Visual Studio Components
If the script reports missing Visual Studio Build Tools, ensure you have installed:
- MSVC v143 - VS 2022 C++ x64/x86 build tools
- Windows 10/11 SDK (latest version)
- CMake tools for Visual Studio (if using Visual Studio IDE)

### vcpkg Issues
If vcpkg installation fails:
1. Check your internet connection
2. Ensure Git is properly installed
3. Try running with `-Force` to reinstall
4. Check that you have write permissions to the installation directory

## Manual Setup
If the automated scripts don't work for your environment, refer to the manual setup instructions in the main README.md file.