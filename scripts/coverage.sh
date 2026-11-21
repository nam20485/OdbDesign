#!/bin/bash

# Canonical M0 coverage pipeline for OdbDesign (truthful measurement).
#
# This script and .github/workflows/code-coverage.yml implement the SAME
# pipeline (see docs/plan/test-coverage-and-evaluation-validation-plan/test-coverage-and-evaluation-validation-plan.md,
# milestone M0). CI builds with the canonical preset and then invokes this
# script with --no-build, so the measured numbers can never drift apart.
#
# Pipeline:
#   1. Configure/build the canonical preset with --coverage flags
#      (default: linux-dynamic-debug — shared protobuf/gRPC runtime).
#   2. Reset counters, then capture initial.info from .gcno BEFORE tests so
#      never-executed production TUs appear with zero-hit records instead of
#      being absent from the denominator.
#   3. Run the real test suite via ctest, writing a JUnit XML result.
#      Test failures/skips FAIL the run (checked by scripts/check_coverage.py).
#   4. Capture runtime.info from .gcda AFTER tests.
#   5. Merge initial + runtime (lcov -a) and produce three DISJOINT scopes:
#        coverage.info               handwritten sources (the gate)
#        coverage-generated.info     *.pb.cc / *.pb.h / *.grpc.pb.cc / *.grpc.pb.h
#        coverage-combined.info      handwritten + generated (audit)
#      Generated code never inflates the handwritten gate; both scopes
#      reconcile to the combined report (enforced by check_coverage.py).
#   6. Branch coverage (--branch-coverage) is enabled on EVERY lcov
#      invocation: capture, merge, remove/extract, summary and genhtml.
#      lcov silently drops BRDA data from outputs when the flag is omitted.
#   7. Record provenance (source SHA, pinned test-data SHA, compiler, lcov
#      version, preset) in provenance.json.
#   8. Optionally (--check) run scripts/check_coverage.py, which parses the
#      RAW LCOV tracefiles + JUnit XML and enforces the release gate:
#      >90% handwritten lines AND functions, >=85% branches (integer math,
#      exactly 90.00% fails), inventory completeness, missing branch data,
#      failed/skipped tests.
#
# Exit codes: 0 success; 1 test/check failure; 2 environment/misconfiguration.
#
# Requires: lcov >= 2.0, genhtml, cmake >= 3.21, ninja, ctest, git, a vcpkg
# toolchain (VCPKG_ROOT) unless --no-build is used.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

DEFAULT_PRESET="linux-dynamic-debug"
PRESET="$DEFAULT_PRESET"
DO_BUILD=true
RUN_CHECK=false
GATE_STRICT=false
OUTPUT_DIR="$PROJECT_ROOT/coverage"
CSS_FILE=""
SNOW_IMAGE=""

COVERAGE_CXX_FLAGS="--coverage -g -O0 -fprofile-update=atomic"
COVERAGE_LINK_FLAGS="--coverage"

# Common lcov switches. --branch-coverage must be present on EVERY lcov
# invocation that reads or writes a tracefile (capture, -a, --remove,
# --extract, --summary); otherwise BRDA/BRF/BRH are silently dropped.
# geninfo_unexecuted_blocks=1 synthesizes zero records for TUs whose .gcda
# is absent at runtime capture (never-executed binaries/TUs).
#
# NOTE on error suppression (M0 exit criterion: no BROAD suppression hiding
# unexplained inconsistencies):
#   * 'mismatch'/'corrupt'/'source' are NEVER suppressed: stale or
#     unreadable gcov data fails the run loudly.
#   * 'unused' (a filter pattern matched nothing) is benign and expected —
#     e.g. on machines where some system prefixes are absent.
#   * 'inconsistent' IS suppressed, but ONLY together with the assertion in
#     run_lcov(): every record lcov auto-repairs must be an EXPLAINED
#     artifact — third-party code (libstdc++/asio headers instantiated into
#     our TUs; e.g. constant-initialized reference returns where gcov
#     reports a covered line inside an uncalled function) or the documented
#     lambda-registration attribution case in product code (see run_lcov).
#     Any other product-path repair FAILS the run.
#     On lcov 2.0 + GCC 13 (CI) no such repairs occur at all.
LCOV_COMMON=(--branch-coverage --rc geninfo_unexecuted_blocks=1 --ignore-errors inconsistent)
LCOV_FILTER_IGNORE=(--ignore-errors unused)  # unused filter patterns are benign

# Required per-file inventory (release_readiness_criteria.json M0-01): the
# nine substantive production TUs previously omitted from the denominator.
REQUIRED_TUS=(
    FileModelController.cpp
    DesignsController.cpp
    FileUploadController.cpp
    HealthCheckController.cpp
    HelloWorldController.cpp
    OdbDesignServerApp.cpp
    OdbServerAppBase.cpp
    main.cpp
    OdbDesignApp.cpp
)

# Release gate thresholds (release_readiness_criteria.json SRC-01).
MIN_LINE=90   # strict >90%: exactly 90.00% FAILS
MIN_FUNC=90   # strict >90%: exactly 90.00% FAILS
MIN_BRANCH=85  # >=85%

print_status() { echo "[INFO] $1"; }
print_success() { echo "[SUCCESS] $1"; }
print_error() { echo "[ERROR] $1" >&2; }

# Run an lcov command with the common repair policy and enforce that every
# auto-repaired 'inconsistent' record is an EXPLAINED artifact:
#   * any path in third-party code (/usr, vcpkg) — known gcov artifacts on
#     constant-initialized reference returns (asio::system_category,
#     std::function, std::tuple, ...);
#   * a product-path repair ONLY when the repaired function is a lambda:
#     gcov attributes the lambda's registration line (executed with the
#     enclosing scope) to the lambda body — the artifact class lcov itself
#     documents in _checkConsistency ("the decl may be executed even if the
#     lambda function itself is not called"), but its first-line-only
#     special case misses later lines.
# Any OTHER product-path repair is an unexplained inconsistency: FAIL.
# Usage: assert_repair_policy <stderr-log-file>
assert_repair_policy() {
    local errlog="$1"
    local line path func demangled
    while IFS= read -r line; do
        path=$(sed -n 's/.*(inconsistent) "\([^"]*\)".*/\1/p' <<< "$line")
        [[ -z "$path" ]] && continue
        case "$path" in
            /usr/*|*/vcpkg/*|*/vcpkg_installed/*)
                print_status "Repaired known gcov artifact in third-party code: $path"
                continue
                ;;
        esac
        # Product path: tolerate ONLY lambda-attribution repairs.
        func=$(sed -n "s/.*(inconsistent) \"[^\"]*\".*function '\([^']*\)'.*/\1/p" <<< "$line")
        if [[ -n "$func" ]]; then
            demangled="$(echo "$func" | c++filt 2>/dev/null || echo "$func")"
            if [[ "$demangled" == *"{lambda"* || "$func" == *"Ul"*"_clE"* ]]; then
                print_status "Repaired lambda-attribution artifact in $path: $demangled"
                continue
            fi
        fi
        print_error "Unexplained coverage inconsistency in product code: $path"
        echo "    $line" >&2
        print_error "This must be investigated, not suppressed. See $errlog"
        exit 1
    done < <(grep -E '\(inconsistent\) "' "$errlog" || true)
}

# Run an lcov command, then enforce the repair policy on its stderr.
# Usage: run_lcov <stderr-log-file> <lcov args...>
run_lcov() {
    local errlog="$1"
    shift
    set +e
    lcov "$@" 2> "$errlog"
    local rc=$?
    set -e
    if [[ $rc -ne 0 ]]; then
        print_error "lcov failed (exit $rc); see $errlog:"
        tail -5 "$errlog" >&2
        return $rc
    fi
    assert_repair_policy "$errlog"
}

show_usage() {
    cat <<EOF
Usage: $0 [OPTIONS]

Canonical M0 truthful-coverage pipeline (same as code-coverage.yml).

OPTIONS:
    -h, --help              Show this help message
    -p, --preset PRESET     CMake preset to configure/build (default: $DEFAULT_PRESET)
        --no-build          Use the existing coverage build; skip configure+build
        --check             Run scripts/check_coverage.py against the outputs:
                            M0-01 gate (inventory + branch data + reconciliation,
                            no numeric floors). Skipped tests are tolerated.
        --gate-strict       With --check: enforce the SRC-01 release gate
                            (>90% lines AND functions, >=85% branches, no skips).
                            Intended for M6; fails until those floors are met.
    -o, --output DIR        Output directory (default: ./coverage)
        --css-file PATH     genhtml CSS for report theming (optional)
        --snow-image PATH   Replacement image for genhtml uncovered bar (optional)

OUTPUTS (in the output directory):
    coverage.info               handwritten tracefile — THE GATE
    coverage-generated.info     generated protobuf/gRPC tracefile
    coverage-combined.info      handwritten + generated (audit)
    coverage_summary.txt        handwritten line/function/branch summary
    initial.info, runtime.info, merged-all.info   raw pipeline stages
    coverage-testlog.txt        full ctest log
    coverage-testlog.xml        JUnit results (consumed by check_coverage.py)
    html/                       handwritten HTML report
    html-generated/, html-combined/             audit reports
    provenance.json             source SHA, test-data SHA, toolchain versions

ENVIRONMENT:
    ODB_TEST_DATA_DIR       REQUIRED: directory containing test designs
                            (.tgz archives). The run fails (exit 2) if unset
                            or missing — coverage without the required corpus
                            is not measured, it is fabricated.
    ODB_TEST_ENVIRONMENT_VARIABLE   Expected by cross-platform env tests;
                            if unset those tests skip and check_coverage.py
                            fails the run (unexpected skips are findings).
    VCPKG_ROOT              Required for --build (vcpkg toolchain).

EXAMPLES:
    $0                          # full pipeline, no gate check
    $0 --no-build --check       # measure + enforce the gate on existing build
EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -p|--preset|-o|--output|--css-file|--snow-image)
            # Value-taking options must fail with the documented
            # misconfiguration exit (2), not a set -u internals error.
            if [[ $# -lt 2 ]]; then
                print_error "Option $1 requires an argument."
                show_usage
                exit 2
            fi
            case $1 in
                -p|--preset)     PRESET="$2" ;;
                -o|--output)     OUTPUT_DIR="$2" ;;
                --css-file)      CSS_FILE="$2" ;;
                --snow-image)    SNOW_IMAGE="$2" ;;
            esac
            shift 2
            ;;
        --no-build)
            DO_BUILD=false
            shift
            ;;
        --check)
            RUN_CHECK=true
            shift
            ;;
        --gate-strict)
            RUN_CHECK=true
            GATE_STRICT=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 2
            ;;
    esac
done

BUILD_DIR="$PROJECT_ROOT/out/build/$PRESET"
TEST_DIR="$BUILD_DIR/OdbDesignTests"
INITIAL_INFO="$OUTPUT_DIR/initial.info"
RUNTIME_INFO="$OUTPUT_DIR/runtime.info"
MERGED_INFO="$OUTPUT_DIR/merged-all.info"
COMBINED_INFO="$OUTPUT_DIR/coverage-combined.info"
GENERATED_INFO="$OUTPUT_DIR/coverage-generated.info"
HANDWRITTEN_INFO="$OUTPUT_DIR/coverage.info"
TEST_LOG="$OUTPUT_DIR/coverage-testlog.txt"
JUNIT_XML="$OUTPUT_DIR/coverage-testlog.xml"

GENERATED_PATTERNS=('*.pb.cc' '*.pb.h' '*.grpc.pb.cc' '*.grpc.pb.h')
# Production component directories captured for coverage. Capturing ONLY
# these (not the whole build tree) keeps test TU objects out of geninfo
# entirely: lcov >= 2.2 hard-errors on gcov end-line inconsistencies in
# macro-heavy TEST() bodies (e.g. ToolsFileTests.cpp) and excluding tests at
# capture time avoids that without any --ignore-errors suppression, while
# also shrinking capture size. Test code is measured by pass/fail, not by
# covering itself.
COMPONENT_DIRS=(OdbDesignLib OdbDesignServer OdbDesignApp Utils)
# Never part of the production denominator: toolchain/system headers and the
# test sources themselves (test code is measured by passing/failing, not by
# covering itself).
EXCLUDE_PATTERNS=('/usr/*' '*/vcpkg/*' '*/vcpkg_installed/*' '*/_deps/*' '*/OdbDesignTests/*')

check_prerequisites() {
    local missing=()
    # python3/nproc/c++ are unconditional pipeline requirements too
    # (provenance + gate checker, parallel ctest, provenance compiler line).
    for tool in lcov genhtml cmake ninja ctest git python3 nproc c++; do
        command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing[*]}"
        exit 2
    fi

    # Missing test data fails closed (M0): never measure a reduced corpus
    # and present it as project coverage.
    if [[ -z "${ODB_TEST_DATA_DIR:-}" ]]; then
        print_error "ODB_TEST_DATA_DIR is not set; refusing to measure without the required test corpus."
        exit 2
    fi
    if [[ ! -d "$ODB_TEST_DATA_DIR" ]]; then
        print_error "ODB_TEST_DATA_DIR='$ODB_TEST_DATA_DIR' does not exist."
        exit 2
    fi

    if [[ "$DO_BUILD" == true && -z "${VCPKG_ROOT:-}" ]]; then
        print_error "VCPKG_ROOT is not set; required to build. (Or pass --no-build with an existing coverage build.)"
        exit 2
    fi
    if [[ "$DO_BUILD" == false ]]; then
        if [[ ! -d "$BUILD_DIR" ]]; then
            print_error "Build dir '$BUILD_DIR' does not exist; configure first (drop --no-build)."
            exit 2
        fi
        # The existing build must actually be instrumented; an uninstrumented
        # build produces an empty denominator that would silently pass every
        # gate. (-print -quit avoids a SIGPIPE'd pipeline that set -o
        # pipefail would misreport as "no files found".) In full-pipeline
        # mode this check is skipped so a fresh checkout can configure+build
        # here; an uninstrumented build still fails loudly at initial
        # capture (lcov errors out without .gcno).
        if [[ -z "$(find "$BUILD_DIR" -name '*.gcno' -print -quit)" ]]; then
            print_error "No .gcno files under $BUILD_DIR; the build was not compiled with --coverage."
            exit 2
        fi
    fi
}

configure_and_build() {
    print_status "Configuring preset '$PRESET' with coverage flags..."
    cmake --preset "$PRESET" \
        -DCMAKE_CXX_FLAGS="$COVERAGE_CXX_FLAGS" \
        -DCMAKE_EXE_LINKER_FLAGS="$COVERAGE_LINK_FLAGS" \
        -DCMAKE_SHARED_LINKER_FLAGS="$COVERAGE_LINK_FLAGS" \
        -DCMAKE_MODULE_LINKER_FLAGS="$COVERAGE_LINK_FLAGS"

    print_status "Building preset '$PRESET'..."
    cmake --build --preset "$PRESET"
}

reset_counters() {
    print_status "Resetting coverage counters..."
    lcov --zerocounters --directory "$BUILD_DIR" --quiet || true
    find "$BUILD_DIR" -name '*.gcda' -delete
}

set_capture_dir_args() {
    CAPTURE_DIR_ARGS=()
    local comp
    for comp in "${COMPONENT_DIRS[@]}"; do
        CAPTURE_DIR_ARGS+=(--directory "$BUILD_DIR/$comp")
    done
}

# Runtime capture must only visit directories that actually produced .gcda:
# never-executed executables (OdbDesignServer, OdbDesignApp) legitimately
# have none — their zero-hit records come from the initial .gcno capture.
# Scoping here (instead of lcov --ignore-errors empty) keeps the pipeline
# free of any error suppression, and a build where NO directory produced
# .gcda is a hard failure (tests cannot have run).
set_runtime_dir_args() {
    RUNTIME_DIR_ARGS=()
    local comp
    for comp in "${COMPONENT_DIRS[@]}"; do
        if [[ -n "$(find "$BUILD_DIR/$comp" -name '*.gcda' -print -quit)" ]]; then
            RUNTIME_DIR_ARGS+=(--directory "$BUILD_DIR/$comp")
        fi
    done
    if [[ ${#RUNTIME_DIR_ARGS[@]} -eq 0 ]]; then
        print_error "No .gcda files found in any component directory; tests did not run under instrumentation."
        exit 1
    fi
}

capture_initial() {
    print_status "Capturing INITIAL (zero-hit) coverage from .gcno — before tests..."
    run_lcov "$OUTPUT_DIR/initial-capture-stderr.log" \
        --capture --initial "${CAPTURE_DIR_ARGS[@]}" \
        "${LCOV_COMMON[@]}" \
        --output-file "$INITIAL_INFO" --quiet
    local records
    records=$(grep -c '^SF:' "$INITIAL_INFO" || true)
    print_status "Initial capture: $records source records (includes never-executed TUs)."
}

run_tests() {
    print_status "Running test suite (ctest)..."
    # Parallel: suite is race-free as of 313918e. Failed tests fail coverage
    # (M0), unlike the old script that generated reports past red suites.
    # ctest's exit status is the COUNT of failed tests (not a boolean), so it
    # is captured and mapped explicitly: any nonzero is a test failure
    # (exit 1), never the misconfiguration exit (2).
    # NOTE: ODB_TEST_ENVIRONMENT_VARIABLE is passed to the child only when
    # set to a non-empty value — a set-but-EMPTY variable (what the workflow
    # renders for an unset repo `vars.` entry) would flip the cross-platform
    # env tests from skip to hard failure, so it is explicitly unset for the
    # child; an unset var (skip) is what check_coverage.py polices.
    # env requires options before assignments, so -u must come first.
    local -a env_args
    if [[ -n "${ODB_TEST_ENVIRONMENT_VARIABLE:-}" ]]; then
        env_args=(ODB_TEST_ENVIRONMENT_VARIABLE="$ODB_TEST_ENVIRONMENT_VARIABLE"
                  ODB_TEST_DATA_DIR="$ODB_TEST_DATA_DIR")
    else
        env_args=(-u ODB_TEST_ENVIRONMENT_VARIABLE
                  ODB_TEST_DATA_DIR="$ODB_TEST_DATA_DIR")
    fi
    local ctest_rc=0
    env "${env_args[@]}" \
        ctest --test-dir "$TEST_DIR" \
        -j "$(nproc)" \
        --output-log "$TEST_LOG" \
        --output-junit "$JUNIT_XML" \
        --output-on-failure || ctest_rc=$?
    if [[ $ctest_rc -ne 0 ]]; then
        # ctest reports the failed-test COUNT in its exit status.
        print_error "Test suite FAILED ($ctest_rc failing test(s) reported by ctest)."
        exit 1
    fi
    print_success "Test suite passed."
}

capture_runtime() {
    set_runtime_dir_args
    print_status "Capturing RUNTIME coverage from .gcda — after tests..."
    run_lcov "$OUTPUT_DIR/runtime-capture-stderr.log" \
        --capture "${RUNTIME_DIR_ARGS[@]}" \
        "${LCOV_COMMON[@]}" \
        --output-file "$RUNTIME_INFO" --quiet
}

merge_and_scope() {
    print_status "Merging initial + runtime into the combined denominator..."
    run_lcov "$OUTPUT_DIR/merge-stderr.log" \
        -a "$INITIAL_INFO" -a "$RUNTIME_INFO" \
        "${LCOV_COMMON[@]}" \
        --output-file "$MERGED_INFO" --quiet

    print_status "Filtering system/toolchain/test records -> combined scope..."
    run_lcov "$OUTPUT_DIR/filter-combined-stderr.log" \
        --remove "$MERGED_INFO" "${EXCLUDE_PATTERNS[@]}" \
        "${LCOV_COMMON[@]}" "${LCOV_FILTER_IGNORE[@]}" \
        --output-file "$COMBINED_INFO" --quiet

    print_status "Splitting generated (*.pb.cc/*.pb.h/*.grpc.pb.*) scope..."
    run_lcov "$OUTPUT_DIR/filter-generated-stderr.log" \
        --extract "$COMBINED_INFO" "${GENERATED_PATTERNS[@]}" \
        "${LCOV_COMMON[@]}" "${LCOV_FILTER_IGNORE[@]}" \
        --output-file "$GENERATED_INFO" --quiet

    run_lcov "$OUTPUT_DIR/filter-handwritten-stderr.log" \
        --remove "$COMBINED_INFO" "${GENERATED_PATTERNS[@]}" \
        "${LCOV_COMMON[@]}" "${LCOV_FILTER_IGNORE[@]}" \
        --output-file "$HANDWRITTEN_INFO" --quiet
}

write_summaries_and_html() {
    print_status "Writing summaries and HTML reports..."
    lcov --summary "$HANDWRITTEN_INFO" "${LCOV_COMMON[@]}" \
        > "$OUTPUT_DIR/coverage_summary.txt" 2>&1
    lcov --summary "$GENERATED_INFO" "${LCOV_COMMON[@]}" \
        > "$OUTPUT_DIR/coverage_generated_summary.txt" 2>&1
    lcov --summary "$COMBINED_INFO" "${LCOV_COMMON[@]}" \
        > "$OUTPUT_DIR/coverage_combined_summary.txt" 2>&1
    # genhtml renders only; no data rewriting, so no repair checks needed.

    local -a html_flags=(--branch-coverage --show-details --legend)
    if [[ -n "$CSS_FILE" ]]; then
        html_flags+=(--css-file "$CSS_FILE")
    fi

    genhtml "$HANDWRITTEN_INFO" "${html_flags[@]}" \
        --output-directory "$OUTPUT_DIR/html" --quiet \
        --title "OdbDesign Handwritten Coverage"
    genhtml "$GENERATED_INFO" "${html_flags[@]}" \
        --output-directory "$OUTPUT_DIR/html-generated" --quiet \
        --title "OdbDesign Generated Coverage"
    genhtml "$COMBINED_INFO" "${html_flags[@]}" \
        --output-directory "$OUTPUT_DIR/html-combined" --quiet \
        --title "OdbDesign Combined Coverage"
    if [[ -n "$SNOW_IMAGE" && -f "$OUTPUT_DIR/html/snow.png" ]]; then
        cp "$SNOW_IMAGE" "$OUTPUT_DIR/html/snow.png"
        cp "$SNOW_IMAGE" "$OUTPUT_DIR/html-generated/snow.png" 2>/dev/null || true
        cp "$SNOW_IMAGE" "$OUTPUT_DIR/html-combined/snow.png" 2>/dev/null || true
    fi
}

write_provenance() {
    print_status "Recording provenance..."
    local source_sha source_dirty testdata_sha compiler_ver lcov_ver
    source_sha="$(git -C "$PROJECT_ROOT" rev-parse HEAD)"
    source_dirty="$(git -C "$PROJECT_ROOT" status --porcelain | head -1 || true)"
    testdata_sha="$(git -C "$ODB_TEST_DATA_DIR" rev-parse HEAD 2>/dev/null || echo "unknown")"
    compiler_ver="$(c++ --version | head -1)"
    lcov_ver="$(lcov --version 2>&1 | head -1)"

    local dirty_flag
    dirty_flag="$([[ -n "$source_dirty" ]] && echo true || echo false)"
    ODB_PROVENANCE="$OUTPUT_DIR/provenance.json" \
    ODB_SOURCE_SHA="$source_sha" \
    ODB_SOURCE_DIRTY="$dirty_flag" \
    ODB_PRESET="$PRESET" \
    ODB_TESTDATA_SHA="$testdata_sha" \
    ODB_TESTDATA_PATH="$ODB_TEST_DATA_DIR" \
    ODB_COMPILER_VER="$compiler_ver" \
    ODB_LCOV_VER="$lcov_ver" \
    python3 -I - "${GENERATED_PATTERNS[@]}" -- "${REQUIRED_TUS[@]}" <<'PYEOF'
import datetime, json, os, sys

patterns, tus = [], []
bucket = patterns
for arg in sys.argv[1:]:
    if arg == "--":
        bucket = tus
        continue
    bucket.append(arg)

provenance = {
    "source_commit": os.environ["ODB_SOURCE_SHA"],
    "source_dirty": os.environ["ODB_SOURCE_DIRTY"] == "true",
    "preset": os.environ["ODB_PRESET"],
    "test_data": {
        "repository": "nam20485/OdbDesignTestData",
        "commit": os.environ["ODB_TESTDATA_SHA"],
        "path": os.environ["ODB_TESTDATA_PATH"],
    },
    "compiler": os.environ["ODB_COMPILER_VER"],
    "lcov": os.environ["ODB_LCOV_VER"],
    "generated_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    "branch_coverage": True,
    "initial_capture": True,
    "generated_patterns": patterns,
    "required_inventory": tus,
}
with open(os.environ["ODB_PROVENANCE"], "w") as f:
    json.dump(provenance, f, indent=2)
    f.write("\n")
PYEOF
    cat "$OUTPUT_DIR/provenance.json"
}

run_gate_check() {
    local -a gate_args=()
    local tu
    for tu in "${REQUIRED_TUS[@]}"; do
        gate_args+=(--require-inventory "$tu")
    done
    if [[ "$GATE_STRICT" == true ]]; then
        # SRC-01 (M6 release gate): strict >90% lines AND functions,
        # >=85% branches, no skipped tests allowed.
        print_status "Running STRICT release gate (SRC-01: >90/>90/>=85)..."
        gate_args+=(--min-line "$MIN_LINE" --min-func "$MIN_FUNC" --min-branch "$MIN_BRANCH")
    else
        # M0-01 gate: truthful measurement only (min_*: 0 disables numeric
        # floors) — inventory completeness, branch data presence,
        # reconciliation, failed tests, and unexpected skips are findings.
        # Skips are allowed until M1-03 makes them a hard criterion.
        print_status "Running M0 gate (inventory + branch data + reconciliation)..."
        gate_args+=(--min-line 0 --min-func 0 --min-branch 0 --allow-skips)
    fi
    python3 -I "$SCRIPT_DIR/check_coverage.py" \
        --handwritten "$HANDWRITTEN_INFO" \
        --generated "$GENERATED_INFO" \
        --combined "$COMBINED_INFO" \
        --junit "$JUNIT_XML" \
        "${gate_args[@]}" \
        || {
            print_error "Coverage gate FAILED (see findings above)."
            exit 1
        }
    print_success "Coverage gate passed."
}

# Remove THIS pipeline's known stage outputs so an aborted rerun can never
# mix artifacts from two generations (the old script's --clean behavior,
# scoped to files this script owns rather than the whole output dir).
clear_stage_outputs() {
    print_status "Clearing previous stage outputs from $OUTPUT_DIR..."
    rm -f "$OUTPUT_DIR"/initial.info "$OUTPUT_DIR"/runtime.info \
        "$OUTPUT_DIR"/merged-all.info \
        "$OUTPUT_DIR"/coverage.info \
        "$OUTPUT_DIR"/coverage-generated.info \
        "$OUTPUT_DIR"/coverage-combined.info \
        "$OUTPUT_DIR"/coverage_summary.txt \
        "$OUTPUT_DIR"/coverage_generated_summary.txt \
        "$OUTPUT_DIR"/coverage_combined_summary.txt \
        "$OUTPUT_DIR"/coverage-testlog.txt \
        "$OUTPUT_DIR"/coverage-testlog.xml \
        "$OUTPUT_DIR"/provenance.json \
        "$OUTPUT_DIR"/*-stderr.log
    rm -rf "$OUTPUT_DIR"/html "$OUTPUT_DIR"/html-generated "$OUTPUT_DIR"/html-combined
}

print_status "Starting canonical M0 coverage pipeline (preset: $PRESET)..."
check_prerequisites
set_capture_dir_args
mkdir -p "$OUTPUT_DIR"
clear_stage_outputs

if [[ "$DO_BUILD" == true ]]; then
    configure_and_build
fi

reset_counters
capture_initial
run_tests
capture_runtime
merge_and_scope
write_summaries_and_html
write_provenance

print_success "Coverage analysis complete. Outputs in: $OUTPUT_DIR"
echo "--- handwritten (THE GATE) ---"
cat "$OUTPUT_DIR/coverage_summary.txt"

if [[ "$RUN_CHECK" == true ]]; then
    run_gate_check
fi
