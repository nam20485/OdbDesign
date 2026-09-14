#!/usr/bin/env bash
# =============================================================================
# M1.4 benchmark driver: cold vs warm design fetch through the in-process
# gRPC service (GetDesign + GetLayerFeaturesStream) on the largest test design.
#
# Usage (from the repo root, after a linux-debug build):
#   scripts/benchmark-design-fetch.sh [cold_iters] [warm_iters] [design_name]
#
# Environment (inherited if already exported):
#   ODB_TEST_DATA_DIR  - test design directory
#                        (default: /home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA)
#   BENCH_TEST_BIN     - path to the OdbDesignTests binary
#                        (default: <repo>/out/build/linux-debug/OdbDesignTests/OdbDesignTests)
#
# Output: "[BENCH]" lines with min/avg/max ms per scenario; the gtest binary
# must be run against a build that includes OdbDesignTests/DesignFetchBenchmarkTests.cpp.
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_BIN="${BENCH_TEST_BIN:-$REPO_ROOT/out/build/linux-debug/OdbDesignTests/OdbDesignTests}"
COLD="${1:-2}"
WARM="${2:-10}"
DESIGN="${3:-}"

if [[ ! -x "$TEST_BIN" ]]; then
    echo "error: test binary not found or not executable: $TEST_BIN" >&2
    echo "build first:  cmake --build --preset linux-debug" >&2
    exit 1
fi

export ODB_TEST_DATA_DIR="${ODB_TEST_DATA_DIR:-/home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA}"
export ODB_TEST_ENVIRONMENT_VARIABLE="${ODB_TEST_ENVIRONMENT_VARIABLE:-ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS}"
export ODB_DESIGN_FETCH_BENCH=1
export ODB_BENCH_COLD="$COLD"
export ODB_BENCH_WARM="$WARM"
if [[ -n "$DESIGN" ]]; then
    export ODB_BENCH_DESIGN="$DESIGN"
fi

echo "benchmark: binary=$TEST_BIN cold=$COLD warm=$WARM design=${DESIGN:-<largest>} data=$ODB_TEST_DATA_DIR"
"$TEST_BIN" --gtest_filter='DesignFetchBenchmarkTest.*' 2>&1 | tee /tmp/benchmark-design-fetch.log
grep -E '\[BENCH\]' /tmp/benchmark-design-fetch.log
