# Linux Setup Script

The `setup-linux.sh` script provides an automated way to set up a complete development environment for OdbDesign on Linux systems.

## Features

- **Multi-Distribution Support**: Automatically detects and supports Ubuntu/Debian, CentOS/RHEL/Fedora, Arch Linux, and openSUSE
- **Dependency Management**: Installs all required system dependencies (cmake, ninja, build tools, etc.)
- **vcpkg Integration**: Automatically installs and configures vcpkg package manager
- **Environment Setup**: Configures shell environment variables automatically
- **Flexible Building**: Supports Debug/Release builds with optional testing
- **Docker Support**: Optional Docker installation and configuration

## Quick Start

```bash
# Clone the repository
git clone https://github.com/nam20485/OdbDesign.git
cd OdbDesign

# Run the setup script
./setup-linux.sh
```

That's it! The script will handle everything automatically.

## Usage Options

```bash
./setup-linux.sh [OPTIONS]

OPTIONS:
    -h, --help              Show help message
    -d, --debug             Build in Debug mode (default: Release)
    -t, --tests             Build and run tests
    --no-docker             Skip Docker installation
    --skip-build            Only install dependencies, skip building
    --vcpkg-dir DIR         Custom vcpkg installation directory (default: ~/vcpkg)
```

## Examples

```bash
# Basic setup and build
./setup-linux.sh

# Debug build with tests
./setup-linux.sh --debug --tests

# Skip Docker installation  
./setup-linux.sh --no-docker

# Only install dependencies, don't build
./setup-linux.sh --skip-build

# Use custom vcpkg directory
./setup-linux.sh --vcpkg-dir /opt/vcpkg
```

## What the Script Does

1. **Detects your Linux distribution** and selects appropriate package manager
2. **Installs system dependencies** including:
   - Build tools (gcc, g++, make)
   - CMake and Ninja build system
   - Git, curl, zip utilities
   - pkg-config
   - Docker (optional)
3. **Sets up vcpkg** by cloning and bootstrapping the Microsoft vcpkg package manager
4. **Configures environment** by adding VCPKG_ROOT to your shell profile
5. **Builds the project** using the appropriate CMake preset
6. **Runs tests** (if requested)
7. **Verifies installation** to ensure everything is working correctly

## Supported Distributions

- **Ubuntu/Debian/Mint**: Uses `apt` package manager
- **Fedora**: Uses `dnf` package manager  
- **CentOS/RHEL**: Uses `yum` package manager
- **Arch Linux/Manjaro**: Uses `pacman` package manager
- **openSUSE/SLES**: Uses `zypper` package manager

## Requirements

- Linux system with sudo access
- Internet connection for downloading dependencies
- Bash shell

## Troubleshooting

If the script fails:

1. **Check the error message** - the script provides detailed error information
2. **Verify internet connectivity** - vcpkg needs to download packages
3. **Ensure you have sudo access** - system packages require root privileges
4. **Check disk space** - the build process requires several GB of space

## Manual Alternative

If you prefer manual setup, see the [BUILD.md](docs/BUILD.md) documentation for step-by-step instructions.