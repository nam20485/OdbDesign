@echo off
::
:: setup-windows.bat - Windows setup script for OdbDesign (Batch version)
::
:: This script provides a simplified setup process for users who prefer batch files
:: or don't have PowerShell execution policy configured.
::
:: SECURITY WARNING: Running scripts from the internet can execute arbitrary code.
:: Please review this script's contents before running it on your system.
::

setlocal enabledelayedexpansion

:: Default settings
set "VCPKG_ROOT_DEFAULT=%USERPROFILE%\vcpkg"
set "VCPKG_ROOT=%VCPKG_ROOT_DEFAULT%"
set "FORCE_INSTALL=0"
set "SKIP_VCPKG=0"

:: Parse command line arguments
:parse_args
if "%~1"=="" goto :main
if /i "%~1"=="--vcpkg-root" (
    set "VCPKG_ROOT=%~2"
    shift
    shift
    goto :parse_args
)
if /i "%~1"=="--force" (
    set "FORCE_INSTALL=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--skip-vcpkg" (
    set "SKIP_VCPKG=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" goto :show_help
if /i "%~1"=="/?" goto :show_help
shift
goto :parse_args

:show_help
echo OdbDesign Windows Setup Script (Batch version)
echo.
echo Usage: setup-windows.bat [OPTIONS]
echo.
echo Options:
echo   --vcpkg-root ^<path^>   Specify vcpkg installation directory
echo                        (default: %VCPKG_ROOT_DEFAULT%)
echo   --force              Force reinstall of components
echo   --skip-vcpkg         Skip vcpkg installation
echo   --help               Show this help message
echo.
echo This script will:
echo   1. Check for required prerequisites
echo   2. Install or update vcpkg
echo   3. Set up environment variables
echo   4. Provide build instructions
echo.
echo For advanced options, use the PowerShell version: setup-windows.ps1
goto :eof

:log_message
echo [%~1] %~2
goto :eof

:log_success
call :log_message "SUCCESS" "%~1"
goto :eof

:log_info
call :log_message "INFO" "%~1"
goto :eof

:log_warning
call :log_message "WARNING" "%~1"
goto :eof

:log_error
call :log_message "ERROR" "%~1"
goto :eof

:check_command
where "%~1" >nul 2>&1
goto :eof

:check_prerequisites
call :log_info "Checking prerequisites..."
set "PREREQ_OK=1"

:: Check Git
call :check_command git
if errorlevel 1 (
    call :log_error "Git: Not found - Please install Git from https://git-scm.com/"
    set "PREREQ_OK=0"
) else (
    call :log_success "Git: Found"
)

:: Check CMake
call :check_command cmake
if errorlevel 1 (
    call :log_error "CMake: Not found - Please install CMake from https://cmake.org/"
    set "PREREQ_OK=0"
) else (
    for /f "tokens=*" %%i in ('cmake --version 2^>nul ^| findstr /r "cmake version"') do (
        call :log_success "CMake: Found (%%i)"
    )
)

:: Check Ninja (optional)
call :check_command ninja
if errorlevel 1 (
    call :log_warning "Ninja: Not found - Please install Ninja or use Visual Studio generator"
) else (
    call :log_success "Ninja: Found"
)

:: Check for Visual Studio (simplified check)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    call :log_success "Visual Studio Build Tools: Found"
) else (
    if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" (
        call :log_success "Visual Studio Build Tools: Found"
    ) else (
        call :log_error "Visual Studio Build Tools: Not found"
        call :log_info "Download from: https://visualstudio.microsoft.com/downloads/"
        set "PREREQ_OK=0"
    )
)

goto :eof

:install_vcpkg
call :log_info "Installing vcpkg to %VCPKG_ROOT%..."

if exist "%VCPKG_ROOT%" (
    if "%FORCE_INSTALL%"=="1" (
        call :log_warning "Removing existing vcpkg installation..."
        rmdir /s /q "%VCPKG_ROOT%"
    ) else (
        call :log_warning "vcpkg already exists at %VCPKG_ROOT%. Use --force to reinstall."
        goto :eof
    )
)

:: Clone vcpkg
call :log_info "Cloning vcpkg repository..."
git clone https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
if errorlevel 1 (
    call :log_error "Failed to clone vcpkg repository"
    exit /b 1
)

:: Bootstrap vcpkg
call :log_info "Bootstrapping vcpkg..."
call "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
if errorlevel 1 (
    call :log_error "Failed to bootstrap vcpkg"
    exit /b 1
)

call :log_success "vcpkg installed successfully!"
goto :eof

:set_environment
call :log_info "Setting environment variables..."

:: Set VCPKG_ROOT environment variable for current user
setx VCPKG_ROOT "%VCPKG_ROOT%" >nul
set "VCPKG_ROOT=%VCPKG_ROOT%"

call :log_success "VCPKG_ROOT set to: %VCPKG_ROOT%"
goto :eof

:apply_patches
call :log_info "Checking for vcpkg patches..."

if exist "%~dp0patch-vcpkg-install.ps1" (
    call :log_info "Applying vcpkg patches..."
    powershell -ExecutionPolicy Bypass -File "%~dp0patch-vcpkg-install.ps1"
    if errorlevel 1 (
        call :log_warning "vcpkg patches script completed with warnings"
    ) else (
        call :log_success "vcpkg patches applied successfully!"
    )
) else (
    call :log_info "No vcpkg patches found to apply"
)
goto :eof

:show_build_instructions
echo.
echo =================================
echo Setup Complete!
echo =================================
echo.
echo To build OdbDesign, run the following commands:
echo.
echo For Release build:
echo   cmake --preset x64-release
echo   cmake --build --preset x64-release
echo.
echo For Debug build:
echo   cmake --preset x64-debug
echo   cmake --build --preset x64-debug
echo.
echo To run tests:
echo   ctest --preset x64-release
echo.
echo Build output will be in:
echo   out\build\x64-release\
echo.
echo Note: You may need to restart your terminal or IDE to pick up
echo the new environment variables.
echo.
goto :eof

:main
echo OdbDesign Windows Setup Script (Batch version)
echo ===============================================
echo.

:: Check prerequisites
call :check_prerequisites
if "%PREREQ_OK%"=="0" (
    call :log_error "Prerequisites check failed. Please install missing components and try again."
    exit /b 1
)

:: Install or update vcpkg
if "%SKIP_VCPKG%"=="0" (
    if not exist "%VCPKG_ROOT%" (
        call :install_vcpkg
        if errorlevel 1 (
            call :log_error "Failed to install vcpkg. Exiting."
            exit /b 1
        )
    ) else (
        if "%FORCE_INSTALL%"=="1" (
            call :install_vcpkg
            if errorlevel 1 (
                call :log_error "Failed to install vcpkg. Exiting."
                exit /b 1
            )
        ) else (
            call :log_info "vcpkg already exists at %VCPKG_ROOT%. Use --force to reinstall or --skip-vcpkg to skip."
        )
    )
) else (
    call :log_info "Skipping vcpkg installation as requested."
)

:: Set environment variables
call :set_environment

:: Apply patches
call :apply_patches

:: Show build instructions
call :show_build_instructions

call :log_success "Windows setup completed successfully!"

:eof