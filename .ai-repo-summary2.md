# OdbDesign Development Instructions

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## Working Effectively

OdbDesign is a cross-platform C++ library and REST API server for parsing ODB++ Design archives. It uses CMake with vcpkg for dependency management and is built on Ubuntu, Windows, and macOS.

### Bootstrap, Build, and Test

**CRITICAL TIMING:** Build takes 2-3 minutes for configuration + dependencies, 2-3 minutes for compilation. Tests take under 1 minute. NEVER CANCEL these operations - set timeout to 60+ minutes.

#### Dependencies Setup
```bash
# Install system dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install -y cmake ninja-build build-essential git curl zip unzip pkg-config libarchive-dev

# Set up vcpkg
export VCPKG_ROOT=$HOME/vcpkg
git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT
$VCPKG_ROOT/bootstrap-vcpkg.sh

# Add to shell profile
echo "export VCPKG_ROOT=$HOME/vcpkg" >> ~/.bashrc
source ~/.bashrc
```

#### Build Commands
```bash
# Configure project
export VCPKG_ROOT=$HOME/vcpkg
export PATH=/usr/local/bin:$PATH
export CMAKE_MAKE_PROGRAM=/usr/local/bin/ninja
cmake --preset linux-release

# Build - takes 2-3 minutes. NEVER CANCEL. Set timeout to 60+ minutes.
cmake --build --preset linux-release
```

#### Test Commands
```bash
# Run basic tests (some tests require test data and will fail)
ctest --test-dir ./out/build/linux-release/OdbDesignTests --output-on-failure
```

### Alternative Setup Scripts

#### Automated Linux Setup
```bash
# Run the comprehensive setup script - takes 5-10 minutes. NEVER CANCEL.
./setup-linux.sh --tests --no-docker
```

#### Manual vcpkg Dependency Installation
If vcpkg fails due to network issues (e.g., bzip2 download timeouts):
```bash
# Install libarchive via system package manager as fallback
sudo apt install -y libarchive-dev

# Run vcpkg install with keep-going to skip problematic packages
$VCPKG_ROOT/vcpkg install --keep-going
```

### Running Applications

#### REST API Server
```bash
# Run from build directory
./out/build/linux-release/OdbDesignServer/OdbDesignServer --help
./out/build/linux-release/OdbDesignServer/OdbDesignServer --port 8888
```

**Note:** Server may have runtime issues but builds successfully. Always test with `--help` first.

#### Docker Setup
```bash
# Build and run via Docker
docker compose up
# Server available at http://localhost:8888
# Swagger UI at http://localhost:8080/swagger
```

## Validation

### Build Validation Steps
- ALWAYS run the full build pipeline when making changes.
- NEVER use default timeouts for build commands - use 60+ minutes.
- If vcpkg fails on bzip2/libarchive due to network issues, install libarchive-dev via apt and continue.
- Regenerate protobuf files if compilation fails:
  ```bash
  cd OdbDesignLib/protoc
  $VCPKG_ROOT/../out/build/linux-release/vcpkg_installed/x64-linux/tools/protobuf/protoc --cpp_out=../ProtoBuf *.proto
  ```

### End-to-End Testing
- Build completes successfully with warnings only
- Basic tests pass (2 out of 25 without test data)
- Server executable starts and shows help message
- Docker compose builds and runs

### Manual Validation Requirements
- ALWAYS test that the server executable can be launched
- Validate that the library builds and links correctly
- Check that all major components compile: OdbDesignLib, OdbDesignServer, OdbDesignApp, Utils

## Common Issues and Solutions

### vcpkg Network Failures
If bzip2 or other packages fail to download:
```bash
# Temporarily remove libarchive from vcpkg.json dependencies
# System libarchive-dev package will be used instead
sudo apt install -y libarchive-dev
```

### Protobuf Compilation Errors
If protobuf files cause compilation errors:
```bash
# Regenerate with the vcpkg protoc
cd OdbDesignLib/protoc
$VCPKG_ROOT/../out/build/linux-release/vcpkg_installed/x64-linux/tools/protobuf/protoc --cpp_out=../ProtoBuf *.proto
```

### Precompiled Header Issues
If PCH causes compilation problems:
```bash
# Disable PCH in CMakeLists.txt by changing:
# if (NOT DEFINED ENV{CI})
# to:
# if (FALSE)
```

## Key Projects and Architecture

### Project Structure
- **OdbDesignLib**: Core C++ shared library for ODB++ parsing
- **OdbDesignServer**: REST API server executable using Crow framework
- **OdbDesignApp**: Command-line application
- **Utils**: Utility library with common functionality
- **OdbDesignTests**: GoogleTest-based test suite

### Dependencies
- **vcpkg managed**: protobuf, openssl, zlib, crow, asio
- **System fallback**: libarchive (if vcpkg fails)
- **Build tools**: CMake, Ninja, GCC

### Important Files
- `CMakePresets.json`: Platform-specific build configurations
- `vcpkg.json`: Package dependencies and version overrides
- `setup-linux.sh`: Automated Linux environment setup
- `OdbDesignLib/protoc/*.proto`: Protocol buffer definitions
- `compose.yml`: Docker configuration

## Repository Navigation

### Frequently Modified Areas
- When changing dependencies: Update `vcpkg.json` and relevant CMakeLists.txt
- When adding features: Modify files in `OdbDesignLib/` and add tests in `OdbDesignTests/`
- When fixing server issues: Check `OdbDesignServer/` and `Utils/`
- When updating protocols: Regenerate protobuf files from `OdbDesignLib/protoc/`

### Configuration Files
```
CMakeLists.txt                  # Main build configuration
CMakePresets.json              # Platform build presets
vcpkg.json                     # Dependency specification
.github/workflows/             # CI/CD pipeline
docs/BUILD.md                  # Detailed build instructions
```

### Build Output Locations
```
out/build/linux-release/      # Linux release build
out/build/linux-debug/        # Linux debug build
out/build/x64-release/         # Windows release build
```