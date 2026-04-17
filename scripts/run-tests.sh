#!/bin/bash

# OdbDesign Test Runner Script
# Provides easy automation for running tests locally and in CI

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DATA_DIR="${ODB_TEST_DATA_DIR:-$PROJECT_ROOT/test_data}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration options
BUILD_TYPE="Debug"
VERBOSE=false
CLEAN_BUILD=false
RUN_COVERAGE=false
RUN_PERFORMANCE=false
PRESET=""

# Function to print colored output
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

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    -c, --clean             Clean build directory before building
    -r, --release           Build in Release mode (default: Debug)
    -d, --debug             Build in Debug mode (explicit)
    --coverage              Run with coverage analysis
    --performance           Run performance/benchmark tests
    --preset NAME           Use specific CMake preset
    --test-data-dir DIR     Override test data directory

EXAMPLES:
    $0                      # Basic test run
    $0 --clean --verbose    # Clean build with verbose output  
    $0 --release --coverage # Release build with coverage
    $0 --preset linux-debug # Use specific preset

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --coverage)
            RUN_COVERAGE=true
            shift
            ;;
        --performance)
            RUN_PERFORMANCE=true
            shift
            ;;
        --preset)
            PRESET="$2"
            shift 2
            ;;
        --test-data-dir)
            TEST_DATA_DIR="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check for required tools
    local missing_tools=()
    
    command -v cmake >/dev/null 2>&1 || missing_tools+=("cmake")
    command -v ninja >/dev/null 2>&1 || missing_tools+=("ninja")
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_error "Please install the missing tools and try again"
        exit 1
    fi
    
    # Check for vcpkg
    if [[ -z "$VCPKG_ROOT" ]]; then
        print_warning "VCPKG_ROOT not set. Dependencies may not be available."
    fi
    
    print_success "Prerequisites check passed"
}

# Function to setup test environment
setup_test_environment() {
    print_status "Setting up test environment..."
    
    # Set test environment variables
    export ODB_TEST_ENVIRONMENT_VARIABLE="ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS"
    export ODB_TEST_DATA_DIR="$TEST_DATA_DIR"
    
    # Create test data directory if it doesn't exist
    if [[ ! -d "$TEST_DATA_DIR" ]]; then
        print_warning "Test data directory not found: $TEST_DATA_DIR"
        print_status "Creating basic test data directory structure..."
        mkdir -p "$TEST_DATA_DIR/FILES"
        echo "Test file content" > "$TEST_DATA_DIR/FILES/test.txt"
    fi
    
    print_success "Test environment setup complete"
}

# Function to clean build directory
clean_build() {
    if [[ "$CLEAN_BUILD" == true ]]; then
        print_status "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

# Function to configure CMake
configure_cmake() {
    print_status "Configuring CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    local cmake_args=()
    
    if [[ -n "$PRESET" ]]; then
        cmake_args+=("--preset" "$PRESET")
    else
        cmake_args+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
        
        # Add vcpkg toolchain if available
        if [[ -n "$VCPKG_ROOT" ]]; then
            cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")
        fi
        
        # Add coverage flags if requested
        if [[ "$RUN_COVERAGE" == true ]]; then
            cmake_args+=("-DCMAKE_CXX_FLAGS=--coverage")
            cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS=--coverage")
        fi
    fi
    
    if [[ "$VERBOSE" == true ]]; then
        echo "CMake command: cmake ${cmake_args[*]} .."
    fi
    
    cmake "${cmake_args[@]}" ..
    
    print_success "CMake configuration complete"
}

# Function to build the project
build_project() {
    print_status "Building project..."
    
    cd "$BUILD_DIR"
    
    local build_args=("--build" ".")
    
    if [[ "$VERBOSE" == true ]]; then
        build_args+=("--verbose")
    fi
    
    cmake "${build_args[@]}"
    
    print_success "Build complete"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$BUILD_DIR"
    
    local ctest_args=()
    
    if [[ "$VERBOSE" == true ]]; then
        ctest_args+=("--verbose")
    else
        ctest_args+=("--output-on-failure")
    fi
    
    # Add specific test filters
    if [[ "$RUN_PERFORMANCE" == true ]]; then
        print_status "Running performance tests..."
        ctest_args+=("-R" "Performance")
    fi
    
    # Run the tests
    if ctest "${ctest_args[@]}"; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed!"
        return 1
    fi
}

# Function to generate coverage report
generate_coverage() {
    if [[ "$RUN_COVERAGE" == true ]]; then
        print_status "Generating coverage report..."
        
        if command -v gcov >/dev/null 2>&1; then
            cd "$BUILD_DIR"
            gcov -r *.gcno
            print_success "Coverage report generated"
        else
            print_warning "gcov not available, skipping coverage report"
        fi
    fi
}

# Main execution
main() {
    print_status "Starting OdbDesign test automation..."
    print_status "Build type: $BUILD_TYPE"
    print_status "Project root: $PROJECT_ROOT"
    print_status "Build directory: $BUILD_DIR"
    print_status "Test data directory: $TEST_DATA_DIR"
    
    check_prerequisites
    setup_test_environment
    clean_build
    
    # Only try to build if we can configure
    if configure_cmake; then
        if build_project; then
            if run_tests; then
                generate_coverage
                print_success "Test automation completed successfully!"
            else
                print_error "Tests failed!"
                exit 1
            fi
        else
            print_error "Build failed!"
            exit 1
        fi
    else
        print_error "CMake configuration failed!"
        print_warning "This might be due to missing dependencies."
        print_warning "Try installing dependencies with: vcpkg install"
        exit 1
    fi
}

# Run main function
main "$@"