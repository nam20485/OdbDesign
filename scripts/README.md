# OdbDesign Test Automation Scripts

This directory contains scripts for automated testing and development workflows.

## Scripts Overview

### `release_readiness.py`
**Release-readiness checker — progress reporting + autonomous readiness gate**

Evaluates every release-readiness criterion from the validation plan
(`docs/plan/test-coverage-and-evaluation-validation-plan/`) against **real evidence**
and reports completed-vs-remaining, or autonomously gates readiness. Driven by the
criteria registry `release_readiness_criteria.json` (single source of truth).

```bash
# Progress view — always runnable, tolerant (exit 0 even when work remains)
python3 scripts/release_readiness.py report
python3 scripts/release_readiness.py report --json --candidate <sha-or-digest>

# Fast glance without launching live server probes
python3 scripts/release_readiness.py report --no-probes

# Strict autonomous gate — exit 0 only if ALL required criteria pass with
# candidate-matched evidence; exit 1 otherwise (for CI / release sign-off)
python3 scripts/release_readiness.py validate --profile eval
python3 scripts/release_readiness.py validate --profile production
```

- **Statuses:** `pass / fail / pending / blocked / not-applicable / unknown`.
  `validate` passes only on all-`pass` (or pre-approved `--na ID`); a missing,
  skipped, or unevidenced criterion can never produce a false green.
- **No grep-style proof:** criteria pass only on real evidence (coverage tracefile +
  JUnit, process exit codes, socket probes, HTTP/gRPC round trips, checksums,
  structured scenario results). Proto/route drift checks are labelled drift signals.
- **Real probes:** F004 fail-closed auth, server health, commercial license rejection
  and loopback enforcement actually attempt execution. Where the subject is absent
  (e.g. an unbuilt binary or missing shared runtime libs) the probe reports `blocked`
  with the real reason — never a fabricated pass.
- **Candidate binding:** results bind to a commit/digest; `validate` rejects evidence
  from a different candidate. Engine negative controls prove a skip/fail/unknown or
  mixed candidate fails the verdict.
- Acceptance runs emit structured results into `release-readiness/evidence/`
  (gitignored); the checker ingests them. See the companion checker plan for the
  criterion model and how criteria turn green as the plan's milestones land.
- **Release manifest gate (`--manifest`):** pass an `eval-plan.json` release
  manifest (schema in `eval-plan.example.json` +
  `../docs/plan/test-coverage-and-evaluation-validation-plan/eval-plan-manifest-schema.md`)
  and the run additionally aggregates the acceptance evidence against it —
  candidate binding (the manifest candidate must equal the explicit
  `--candidate`, or HEAD when omitted), required-scenario coverage per
  platform (a `pass` never masks a `failed`/`skipped`/`not-run` entry for the
  same scenario — conflicts fail the verdict) — gating `validate` on the
  aggregation verdict and its negative controls (`EVID-MANIFEST`,
  `EVID-NC-MAN-*`):

  ```bash
  python3 scripts/release_readiness.py validate --manifest eval-plan.json --candidate <token>
  python3 scripts/release_readiness.py report --manifest eval-plan.json   # informational
  ```

### `eval_manifest.py`
**eval-plan.json release manifest loader + acceptance-evidence aggregator**

Implements validation-plan §10: schema validation for the immutable release
manifest (candidate identity, proto/OpenAPI hashes, platform matrix, network
boundary, corpus aliases, resource envelope, required scenarios per platform,
public signing-key fingerprints — never secrets), and the aggregator that
requires every required scenario ID to return a structured `pass`; missing,
skipped, failed, not-run, or candidate-mismatched evidence fails the overall
verdict even when later health checks pass.

```bash
python3 scripts/eval_manifest.py check --manifest eval-plan.json     # schema only
python3 scripts/eval_manifest.py aggregate --manifest eval-plan.json \
    --evidence-dir release-readiness/evidence [--candidate <token>] [--json]
python3 scripts/eval_manifest.py self-test                           # negative controls
```

Exit codes: `0` ready / `1` verdict NOT READY (or controls failed) / `2` manifest
missing/invalid.

**Path safety (fail closed):** scenario IDs and platform names must match
`^[A-Za-z0-9][A-Za-z0-9._-]*$` (no `/`, `\`, `..`, whitespace); evidence file
names and evidence scenario IDs pass the same gate; every evidence file must
realpath inside the evidence directory (symlink escapes are quarantined as
binding violations and never read); `--manifest`/`--evidence-dir`/`--output`
must not contain `..` components, and manifests must realpath into the repo,
its sibling-repos parent, or the system temp dir. Traversal attempts are
proven rejected by the `self-test` negative controls.

### `run-tests.sh`
**Comprehensive test runner for local development and CI**

```bash
# Basic usage
./scripts/run-tests.sh

# Clean build with verbose output
./scripts/run-tests.sh --clean --verbose

# Release build with coverage
./scripts/run-tests.sh --release --coverage

# Performance tests only
./scripts/run-tests.sh --performance

# Use specific CMake preset
./scripts/run-tests.sh --preset linux-debug
```

**Features:**
- Automatic prerequisite checking
- Test environment setup
- Flexible build configuration
- Performance and coverage options
- Colored output and progress reporting

### `coverage.sh`
**Canonical M0 truthful-coverage pipeline** (same as code-coverage.yml):
initial zero-hit capture from .gcno, ctest run, runtime capture from .gcda,
merge, disjoint handwritten/generated/combined scopes, branch coverage, and a
provenance record.

```bash
# Full pipeline for the canonical preset (requires ODB_TEST_DATA_DIR)
export ODB_TEST_DATA_DIR=/path/to/OdbDesignTestData/TEST_DATA
./scripts/coverage.sh

# Re-run reports only against an existing coverage build
./scripts/coverage.sh --no-build

# With the M0-01 inventory/reconciliation gate
./scripts/coverage.sh --check

# Release gate (SRC-01: >90% lines AND functions, >=85% branches, no skips)
./scripts/coverage.sh --check --gate-strict

# Custom preset / output / report theming
./scripts/coverage.sh -p linux-dynamic-debug -o ./coverage \
    --css-file scripts/gcov-dark.css --snow-image scripts/snow-dark.png
```

**Environment:**
- `ODB_TEST_DATA_DIR` — REQUIRED (the script hard-fails without it)
- `ODB_TEST_ENVIRONMENT_VARIABLE` — optional; set to a non-empty value to
  enable the cross-platform env tests (set-but-empty disables them)

**Key outputs** (in the output directory): `coverage.info` (handwritten
tracefile — THE GATE), `coverage-generated.info`, `coverage-combined.info`,
`coverage_summary.txt`, `html/`, `provenance.json`.

### `setup-linux.sh`
Automated Linux environment setup. See detailed documentation in [docs/README-SETUP.md](../docs/README-SETUP.md).

```bash
# Run the setup script
./scripts/setup-linux.sh

# For full options, see docs/README-SETUP.md
```

## Prerequisites

### System Requirements
- **CMake 3.21+** - Build system
- **Ninja** - Recommended build tool
- **C++17 compiler** - GCC, Clang, or MSVC
- **vcpkg** - Package manager (optional but recommended)

### For Coverage Analysis
- **gcov** - Basic coverage analysis
- **lcov** - HTML report generation
- **genhtml** - HTML formatting

#### Installing Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake ninja-build gcc g++ gcov lcov
```

**RHEL/CentOS/Fedora:**
```bash
sudo yum install cmake ninja-build gcc gcc-c++ gcov lcov
# or for newer versions:
sudo dnf install cmake ninja-build gcc gcc-c++ gcov lcov
```

**macOS:**
```bash
brew install cmake ninja gcov lcov
```

**Windows:**
- Install Visual Studio with C++ development tools
- Install CMake from cmake.org
- Install vcpkg for dependencies

## Environment Setup

### Environment Variables

| Variable | Purpose | Default |
|----------|---------|---------|
| `VCPKG_ROOT` | vcpkg installation directory | Auto-detected |
| `ODB_TEST_DATA_DIR` | Test data directory | `./test_data` |
| `ODB_TEST_ENVIRONMENT_VARIABLE` | Test environment flag | Set by scripts |

### Test Data Setup

The test suite can operate with or without dedicated test data:

**Option 1: Automatic (Recommended for development)**
```bash
# Scripts will create basic test data automatically
./scripts/run-tests.sh
```

**Option 2: Custom test data directory**
```bash
export ODB_TEST_DATA_DIR=/path/to/your/test/data
./scripts/run-tests.sh
```

**Option 3: Using CI test data**
```bash
# Clone test data repository (if available)
git clone https://github.com/nam20485/OdbDesignTestData.git
export ODB_TEST_DATA_DIR=./OdbDesignTestData/TEST_DATA
./scripts/run-tests.sh
```

## Usage Examples

### Development Workflow

```bash
# Quick test during development
./scripts/run-tests.sh

# Full test suite with coverage
./scripts/run-tests.sh --clean --coverage
./scripts/coverage.sh

# Performance regression testing
./scripts/run-tests.sh --performance --release
```

### CI/CD Integration

```bash
# CI test execution
./scripts/run-tests.sh --preset linux-dynamic-release --verbose

# Coverage for CI (same pipeline as code-coverage.yml)
./scripts/coverage.sh --no-build --check
```

### Debug and Troubleshooting

```bash
# Verbose build and test output
./scripts/run-tests.sh --debug --verbose

# Clean everything and start fresh
./scripts/run-tests.sh --clean
```

## Script Options Reference

### run-tests.sh Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-v, --verbose` | Enable verbose output |
| `-c, --clean` | Clean build directory |
| `-r, --release` | Release build mode |
| `-d, --debug` | Debug build mode (default) |
| `--coverage` | Enable coverage analysis |
| `--performance` | Run performance tests only |
| `--preset NAME` | Use specific CMake preset |
| `--test-data-dir DIR` | Override test data directory |

### coverage.sh Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-p, --preset PRESET` | CMake preset to configure/build (default: linux-dynamic-debug) |
| `--no-build` | Use the existing coverage build; skip configure+build |
| `--check` | Run scripts/check_coverage.py gate (inventory + branch data + reconciliation) |
| `--gate-strict` | With --check: enforce the SRC-01 release gate (>90% lines/functions, >=85% branches) |
| `-o, --output DIR` | Output directory (default: ./coverage) |
| `--css-file PATH` | genhtml CSS for report theming |
| `--snow-image PATH` | Replacement image for genhtml uncovered bar |

## Continuous Integration

These scripts are designed to work in CI environments:

### GitHub Actions Usage
```yaml
- name: Run Tests
  run: ./scripts/run-tests.sh --preset linux-dynamic-release

- name: Generate Coverage
  run: ./scripts/coverage.sh --no-build
  env:
    ODB_TEST_DATA_DIR: ${{ github.workspace }}/OdbDesignTestData/TEST_DATA
  
- name: Upload Coverage
  uses: actions/upload-artifact@v3
  with:
    name: coverage-report
    path: coverage/
```

### Local CI Simulation
```bash
# Simulate CI environment
export CI=true
./scripts/run-tests.sh --preset linux-dynamic-release --verbose
```

## Troubleshooting

### Common Issues

**1. Missing Dependencies**
```bash
# Error: Could not find vcpkg/CMake/etc.
# Solution: Install prerequisites or set environment variables
export VCPKG_ROOT=/path/to/vcpkg
```

**2. Test Data Not Found**
```bash
# Warning: Test data directory not found
# Solution: Set test data directory or let scripts create basic data
export ODB_TEST_DATA_DIR=/path/to/test/data
```

**3. Build Failures**
```bash
# Error: Build failed
# Solution: Clean and try again
./scripts/run-tests.sh --clean --verbose
```

**4. Coverage Tools Missing**
```bash
# Error: gcov/lcov not found
# Solution: Install coverage tools
sudo apt-get install gcov lcov  # Ubuntu/Debian
```

### Performance Issues

**Slow Test Execution:**
- Use `--release` for faster execution
- Use `--performance` to run only performance tests
- Check available system resources

**Large Coverage Reports:**
- Use `--no-build` to regenerate reports from an existing coverage build
- The pipeline splits handwritten vs generated code into disjoint scopes


### Debug Mode

**Enable verbose logging:**
```bash
./scripts/run-tests.sh --verbose
bash -x ./scripts/coverage.sh --no-build
```

**Check script execution:**
```bash
bash -x ./scripts/run-tests.sh
```

## Script Customization

### Extending run-tests.sh

To add new test categories or build configurations:

1. Add new command line options in the argument parsing section
2. Implement the new functionality in the main execution flow
3. Update the usage documentation

### Extending coverage.sh

To add new coverage analysis features:

1. Add new report formats in `generate_coverage_report()`
2. Implement custom filtering in `generate_html_report()`
3. Add new output options

### Creating Custom Scripts

Use existing scripts as templates:

```bash
#!/bin/bash
# Copy from run-tests.sh header
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Add your custom functionality
```

## Best Practices

### Development
- Run tests before committing changes
- Use coverage analysis to ensure good test coverage
- Test on multiple build configurations (Debug/Release)

### CI/CD
- Use presets for consistent builds across environments
- Generate coverage reports for pull requests
- Cache build dependencies when possible

### Maintenance
- Keep scripts updated with project changes
- Test scripts on different platforms
- Document any custom modifications

## Support

For issues with the test automation scripts:

1. Check this documentation
2. Review script output for error messages
3. Try clean builds with `--clean` option
4. Check prerequisites and environment setup
5. Refer to project issues or documentation

## Contributing

When modifying test automation scripts:

1. Test changes on multiple platforms
2. Update documentation
3. Maintain backward compatibility
4. Follow existing code style and patterns
5. Add appropriate error handling