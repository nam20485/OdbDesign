#!/bin/bash

# OdbDesign Linux Setup Script
# This script sets up a complete Linux development environment for OdbDesign
# Supports Ubuntu/Debian, CentOS/RHEL/Fedora, and Arch Linux

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
VCPKG_DIR="$HOME/vcpkg"
BUILD_TYPE="Release"
BUILD_TESTS=false
INSTALL_DOCKER=true
SKIP_BUILD=false

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage information
show_usage() {
    cat << EOF
OdbDesign Linux Setup Script

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -d, --debug             Build in Debug mode (default: Release)
    -t, --tests             Build and run tests
    --no-docker             Skip Docker installation
    --skip-build            Only install dependencies, skip building
    --vcpkg-dir DIR         Custom vcpkg installation directory (default: ~/vcpkg)

EXAMPLES:
    $0                      # Basic setup and build
    $0 --debug --tests      # Debug build with tests
    $0 --no-docker          # Skip Docker installation
    $0 --skip-build         # Only install dependencies

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -t|--tests)
                BUILD_TESTS=true
                shift
                ;;
            --no-docker)
                INSTALL_DOCKER=false
                shift
                ;;
            --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            --vcpkg-dir)
                VCPKG_DIR="$2"
                shift 2
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Detect Linux distribution
detect_distro() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        DISTRO=$ID
    elif [[ -f /etc/redhat-release ]]; then
        DISTRO="rhel"
    elif [[ -f /etc/debian_version ]]; then
        DISTRO="debian"
    else
        print_error "Unable to detect Linux distribution"
        exit 1
    fi
    
    print_status "Detected distribution: $DISTRO"
}

# Install system dependencies based on distribution
install_system_deps() {
    print_status "Installing system dependencies..."
    
    local base_packages="git cmake ninja-build pkg-config curl zip unzip tar"
    local docker_packages=""
    
    if [[ "$INSTALL_DOCKER" == "true" ]]; then
        docker_packages="docker.io docker-compose-v2"
    fi
    
    case $DISTRO in
        ubuntu|debian|mint)
            sudo apt update
            sudo apt install -y -qq --no-install-recommends \
                build-essential \
                $base_packages \
                $docker_packages
            ;;
        fedora)
            sudo dnf install -y \
                gcc gcc-c++ make \
                $base_packages \
                docker docker-compose
            ;;
        centos|rhel)
            # Enable EPEL for additional packages
            sudo yum install -y epel-release || true
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                cmake3 ninja-build pkg-config curl zip unzip tar \
                git
            # Link cmake3 to cmake if needed
            if ! command -v cmake &> /dev/null && command -v cmake3 &> /dev/null; then
                sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake
            fi
            if [[ "$INSTALL_DOCKER" == "true" ]]; then
                sudo yum install -y docker docker-compose
            fi
            ;;
        arch|manjaro)
            sudo pacman -Syu --noconfirm
            sudo pacman -S --noconfirm \
                base-devel cmake ninja pkg-config curl zip unzip tar git
            if [[ "$INSTALL_DOCKER" == "true" ]]; then
                sudo pacman -S --noconfirm docker docker-compose
            fi
            ;;
        opensuse*|sles)
            sudo zypper refresh
            sudo zypper install -y \
                gcc gcc-c++ make cmake ninja pkg-config curl zip unzip tar git
            if [[ "$INSTALL_DOCKER" == "true" ]]; then
                sudo zypper install -y docker docker-compose
            fi
            ;;
        *)
            print_warning "Unsupported distribution: $DISTRO"
            print_warning "Please install the following packages manually:"
            print_warning "- build tools (gcc, g++, make)"
            print_warning "- cmake, ninja-build, pkg-config"
            print_warning "- git, curl, zip, unzip, tar"
            if [[ "$INSTALL_DOCKER" == "true" ]]; then
                print_warning "- docker, docker-compose"
            fi
            read -p "Continue anyway? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
    
    print_success "System dependencies installed"
}

# Setup and configure Docker (if installed)
setup_docker() {
    if [[ "$INSTALL_DOCKER" == "true" ]] && command -v docker &> /dev/null; then
        print_status "Configuring Docker..."
        
        # Start Docker service
        sudo systemctl enable docker || true
        sudo systemctl start docker || true
        
        # Add user to docker group
        if ! groups $USER | grep -q '\bdocker\b'; then
            sudo usermod -aG docker $USER
            print_warning "Added user to docker group. You may need to log out and back in for changes to take effect."
        fi
        
        print_success "Docker configured"
    fi
}

# Install and setup vcpkg
setup_vcpkg() {
    print_status "Setting up vcpkg at $VCPKG_DIR..."
    
    if [[ -d "$VCPKG_DIR" ]]; then
        print_warning "vcpkg directory already exists at $VCPKG_DIR"
        read -p "Remove and reinstall? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$VCPKG_DIR"
        else
            print_status "Using existing vcpkg installation"
            return
        fi
    fi
    
    # Clone vcpkg
    git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_DIR"
    
    # Bootstrap vcpkg
    "$VCPKG_DIR/bootstrap-vcpkg.sh"
    
    print_success "vcpkg installed and bootstrapped"
}

# Setup environment variables
setup_environment() {
    print_status "Setting up environment variables..."
    
    local vcpkg_root_line="export VCPKG_ROOT=\"$VCPKG_DIR\""
    local shell_profile=""
    
    # Determine shell profile file
    if [[ -n "$ZSH_VERSION" ]]; then
        shell_profile="$HOME/.zshrc"
    elif [[ -n "$BASH_VERSION" ]]; then
        shell_profile="$HOME/.bashrc"
    else
        shell_profile="$HOME/.profile"
    fi
    
    # Add VCPKG_ROOT to shell profile if not already present
    if [[ ! -f "$shell_profile" ]] || ! grep -q "VCPKG_ROOT" "$shell_profile"; then
        echo "" >> "$shell_profile"
        echo "# Added by OdbDesign setup script" >> "$shell_profile"
        echo "$vcpkg_root_line" >> "$shell_profile"
        print_success "Added VCPKG_ROOT to $shell_profile"
    else
        print_status "VCPKG_ROOT already set in $shell_profile"
    fi
    
    # Export for current session
    export VCPKG_ROOT="$VCPKG_DIR"
    
    print_success "Environment configured"
}

# Build the project
build_project() {
    if [[ "$SKIP_BUILD" == "true" ]]; then
        print_status "Skipping build as requested"
        return
    fi
    
    print_status "Building OdbDesign ($BUILD_TYPE)..."
    
    local preset="linux-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    
    # Configure
    print_status "Configuring with preset: $preset"
    cmake --preset "$preset"
    
    # Build
    print_status "Building..."
    cmake --build --preset "$preset"
    
    print_success "Build completed successfully"
    
    # Show build output location
    local build_dir="out/build/$preset"
    print_status "Build output available in: $build_dir"
    
    # List built executables
    if [[ -d "$build_dir" ]]; then
        print_status "Built executables:"
        find "$build_dir" -type f -executable -name "OdbDesign*" | head -10
    fi
}

# Run tests if requested
run_tests() {
    if [[ "$BUILD_TESTS" == "true" ]]; then
        print_status "Running tests..."
        
        local preset="linux-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
        
        if ctest --preset "$preset" --output-on-failure; then
            print_success "All tests passed"
        else
            print_warning "Some tests failed"
        fi
    fi
}

# Verify installation
verify_setup() {
    print_status "Verifying installation..."
    
    local issues=0
    
    # Check required tools
    local tools=("git" "cmake" "ninja" "pkg-config")
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            print_error "$tool is not installed or not in PATH"
            ((issues++))
        fi
    done
    
    # Check vcpkg
    if [[ ! -x "$VCPKG_DIR/vcpkg" ]]; then
        print_error "vcpkg is not properly installed at $VCPKG_DIR"
        ((issues++))
    fi
    
    # Check environment
    if [[ -z "$VCPKG_ROOT" ]]; then
        print_error "VCPKG_ROOT environment variable is not set"
        ((issues++))
    fi
    
    if [[ $issues -eq 0 ]]; then
        print_success "All verification checks passed"
    else
        print_error "Found $issues issues during verification"
        return 1
    fi
}

# Main execution
main() {
    echo "============================================"
    echo "       OdbDesign Linux Setup Script        "
    echo "============================================"
    echo
    
    parse_args "$@"
    
    print_status "Starting setup with configuration:"
    print_status "  Build type: $BUILD_TYPE"
    print_status "  Run tests: $BUILD_TESTS"
    print_status "  Install Docker: $INSTALL_DOCKER"
    print_status "  Skip build: $SKIP_BUILD"
    print_status "  vcpkg directory: $VCPKG_DIR"
    echo
    
    # Check if running as root
    if [[ $EUID -eq 0 ]]; then
        print_error "This script should not be run as root"
        exit 1
    fi
    
    # Main setup steps
    detect_distro
    install_system_deps
    setup_docker
    setup_vcpkg
    setup_environment
    build_project
    run_tests
    verify_setup
    
    echo
    echo "============================================"
    print_success "OdbDesign setup completed successfully!"
    echo "============================================"
    echo
    print_status "Next steps:"
    echo "  1. Restart your shell or run: source ~/.bashrc"
    echo "  2. Navigate to the project directory"
    if [[ "$SKIP_BUILD" == "false" ]]; then
        echo "  3. Run the server: ./out/build/linux-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')/OdbDesignServer"
    else
        echo "  3. Build the project: cmake --preset linux-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]') && cmake --build --preset linux-$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    fi
    echo "  4. Access the API at: http://localhost:8888"
    echo
}

# Run main function with all arguments
main "$@"