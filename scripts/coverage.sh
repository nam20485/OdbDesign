#!/bin/bash

# Test Coverage Analysis Script for OdbDesign
# Generates coverage reports using gcov and lcov

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
COVERAGE_DIR="$PROJECT_ROOT/coverage"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

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

show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Generate test coverage report for OdbDesign

OPTIONS:
    -h, --help              Show this help message
    -c, --clean             Clean previous coverage data
    -o, --output DIR        Output directory for coverage report (default: ./coverage)
    --html                  Generate HTML coverage report (requires lcov)
    --summary               Show coverage summary only

EXAMPLES:
    $0                      # Generate basic coverage report
    $0 --html               # Generate HTML coverage report
    $0 --clean --html       # Clean and generate HTML report

PREREQUISITES:
    - gcov (for basic coverage)
    - lcov (for HTML reports)
    - genhtml (for HTML generation)

EOF
}

# Parse command line arguments
CLEAN_COVERAGE=false
GENERATE_HTML=false
SUMMARY_ONLY=false
OUTPUT_DIR="$COVERAGE_DIR"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -c|--clean)
            CLEAN_COVERAGE=true
            shift
            ;;
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --html)
            GENERATE_HTML=true
            shift
            ;;
        --summary)
            SUMMARY_ONLY=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

check_prerequisites() {
    print_status "Checking prerequisites..."
    
    local missing_tools=()
    
    command -v gcov >/dev/null 2>&1 || missing_tools+=("gcov")
    
    if [[ "$GENERATE_HTML" == true ]]; then
        command -v lcov >/dev/null 2>&1 || missing_tools+=("lcov")
        command -v genhtml >/dev/null 2>&1 || missing_tools+=("genhtml")
    fi
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_error "Install missing tools:"
        print_error "  Ubuntu/Debian: sudo apt-get install gcov lcov"
        print_error "  RHEL/CentOS: sudo yum install gcc lcov"
        print_error "  macOS: brew install lcov"
        exit 1
    fi
    
    print_success "Prerequisites check passed"
}

clean_coverage_data() {
    if [[ "$CLEAN_COVERAGE" == true ]]; then
        print_status "Cleaning previous coverage data..."
        
        # Remove coverage output directory
        rm -rf "$OUTPUT_DIR"
        
        # Remove gcov files from build directory
        if [[ -d "$BUILD_DIR" ]]; then
            find "$BUILD_DIR" -name "*.gcda" -delete
            find "$BUILD_DIR" -name "*.gcno" -delete
            find "$BUILD_DIR" -name "*.gcov" -delete
        fi
        
        print_success "Coverage data cleaned"
    fi
}

build_with_coverage() {
    print_status "Building project with coverage flags..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with coverage flags
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
        -DCMAKE_EXE_LINKER_FLAGS="--coverage"
    
    # Build the project
    cmake --build .
    
    print_success "Build with coverage complete"
}

run_tests_for_coverage() {
    print_status "Running tests to generate coverage data..."
    
    cd "$BUILD_DIR"
    
    # Set test environment
    export ODB_TEST_ENVIRONMENT_VARIABLE="ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS"
    export ODB_TEST_DATA_DIR="${ODB_TEST_DATA_DIR:-$PROJECT_ROOT/test_data}"
    
    # Create basic test data if directory doesn't exist
    if [[ ! -d "$ODB_TEST_DATA_DIR" ]]; then
        print_warning "Creating basic test data directory: $ODB_TEST_DATA_DIR"
        mkdir -p "$ODB_TEST_DATA_DIR/FILES"
        echo "Test file content" > "$ODB_TEST_DATA_DIR/FILES/test.txt"
    fi
    
    # Run tests
    if ctest --output-on-failure; then
        print_success "Tests completed successfully"
    else
        print_warning "Some tests failed, but coverage data was still generated"
    fi
}

generate_coverage_report() {
    print_status "Generating coverage report..."
    
    mkdir -p "$OUTPUT_DIR"
    cd "$BUILD_DIR"
    
    if [[ "$GENERATE_HTML" == true ]]; then
        generate_html_report
    else
        generate_basic_report
    fi
}

generate_basic_report() {
    print_status "Generating basic coverage report..."
    
    # Find all .gcno files and generate .gcov files
    local gcov_files=()
    while IFS= read -r -d '' file; do
        gcov_files+=("$file")
    done < <(find . -name "*.gcno" -print0)
    
    if [[ ${#gcov_files[@]} -eq 0 ]]; then
        print_error "No coverage data found. Make sure to build with --coverage flags."
        exit 1
    fi
    
    # Generate gcov files
    for gcno_file in "${gcov_files[@]}"; do
        gcov "$gcno_file" -o "$(dirname "$gcno_file")"
    done
    
    # Create summary report
    local summary_file="$OUTPUT_DIR/coverage_summary.txt"
    {
        echo "OdbDesign Test Coverage Summary"
        echo "Generated on: $(date)"
        echo "==============================="
        echo
        
        # Analyze gcov files
        local total_lines=0
        local covered_lines=0
        
        while IFS= read -r -d '' gcov_file; do
            if [[ -f "$gcov_file" ]]; then
                local file_lines
                local file_covered
                file_lines=$(grep -c ":" "$gcov_file" || true)
                file_covered=$(grep -c "^[ ]*[1-9]" "$gcov_file" || true)
                
                total_lines=$((total_lines + file_lines))
                covered_lines=$((covered_lines + file_covered))
                
                local coverage_percent=0
                if [[ $file_lines -gt 0 ]]; then
                    coverage_percent=$((file_covered * 100 / file_lines))
                fi
                
                echo "$(basename "$gcov_file" .gcov): $file_covered/$file_lines lines (${coverage_percent}%)"
            fi
        done < <(find . -name "*.gcov" -print0)
        
        echo
        local overall_percent=0
        if [[ $total_lines -gt 0 ]]; then
            overall_percent=$((covered_lines * 100 / total_lines))
        fi
        echo "Overall Coverage: $covered_lines/$total_lines lines (${overall_percent}%)"
        
    } > "$summary_file"
    
    # Copy gcov files to output directory
    find . -name "*.gcov" -exec cp {} "$OUTPUT_DIR/" \;
    
    print_success "Basic coverage report generated in: $OUTPUT_DIR"
    
    if [[ "$SUMMARY_ONLY" == true ]]; then
        cat "$summary_file"
    fi
}

generate_html_report() {
    print_status "Generating HTML coverage report..."
    
    local lcov_file="$OUTPUT_DIR/coverage.info"
    local html_dir="$OUTPUT_DIR/html"
    
    # Capture coverage data
    lcov --capture --directory . --output-file "$lcov_file" --quiet
    
    # Filter out system headers and test files
    lcov --remove "$lcov_file" \
        '/usr/*' \
        '*/vcpkg/*' \
        '*/googletest/*' \
        '*/OdbDesignTests/*' \
        --output-file "$lcov_file" --quiet
    
    # Generate HTML report
    genhtml "$lcov_file" --output-directory "$html_dir" --quiet \
        --title "OdbDesign Test Coverage" \
        --show-details --legend
    
    # Generate summary
    lcov --summary "$lcov_file" > "$OUTPUT_DIR/coverage_summary.txt"
    
    print_success "HTML coverage report generated in: $html_dir"
    print_status "Open $html_dir/index.html in your browser to view the report"
    
    if [[ "$SUMMARY_ONLY" == true ]]; then
        lcov --summary "$lcov_file"
    fi
}

main() {
    print_status "Starting coverage analysis for OdbDesign..."
    
    check_prerequisites
    clean_coverage_data
    
    # Check if we need to build
    if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]] || [[ "$CLEAN_COVERAGE" == true ]]; then
        build_with_coverage
    else
        print_status "Using existing build (use --clean to rebuild)"
    fi
    
    run_tests_for_coverage
    generate_coverage_report
    
    print_success "Coverage analysis complete!"
    print_status "Coverage report available in: $OUTPUT_DIR"
}

# Run main function
main "$@"