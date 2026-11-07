#!/usr/bin/env bash
#
# generate-third-party-notices.sh — assemble THIRD_PARTY_NOTICES.txt from the
# copyright files vcpkg installs for every port in a vcpkg_installed triplet
# directory. Run after a successful configure/build so vcpkg_installed is
# populated, then commit the output (or let the commercial packaging pipeline
# regenerate it per delivery).
#
# Usage:
#   scripts/generate-third-party-notices.sh [<vcpkg-installed-triplet-dir>]
#
# The default triplet dir is auto-detected from the newest
# out/build/*/vcpkg_installed/* directory that contains share/*/copyright files.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

TRIPLET_DIR="${1:-}"
if [[ -z "${TRIPLET_DIR}" ]]; then
    # Default to the main release build's target triplet: prefer the dynamic
    # triplet (the shared-runtime release configuration) inside the
    # linux-dynamic-release build; fall back to any triplet with copyright
    # files there, then any other build dir. Set PREFERRED_BUILD to override.
    prefer_build="${PREFERRED_BUILD:-linux-dynamic-release}"
    for cand in "${REPO_ROOT}/out/build/${prefer_build}/vcpkg_installed/"*-dynamic/ \
                "${REPO_ROOT}/out/build/${prefer_build}/vcpkg_installed/"*/ \
                "${REPO_ROOT}"/out/build/*/vcpkg_installed/*/; do
        [[ -d "${cand}" ]] || continue
        if ls "${cand}"/share/*/copyright >/dev/null 2>&1; then
            TRIPLET_DIR="${cand%/}"
            break
        fi
    done
fi

if [[ -z "${TRIPLET_DIR}" ]] || ! ls "${TRIPLET_DIR}"/share/*/copyright >/dev/null 2>&1; then
    echo "error: no vcpkg_installed triplet dir with share/*/copyright files found." >&2
    echo "usage: $0 [<vcpkg-installed-triplet-dir>]   (e.g. out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic)" >&2
    exit 1
fi
TRIPLET_DIR="${TRIPLET_DIR%/}"

OUT="${2:-${REPO_ROOT}/THIRD_PARTY_NOTICES.txt}"

{
    echo "THIRD-PARTY SOFTWARE NOTICES AND INFORMATION"
    echo
    echo "OdbDesign (https://github.com/nam20485/OdbDesign) is licensed under the"
    echo "GNU Affero General Public License v3.0 (see LICENSE). Binary distributions"
    echo "embed or link the following third-party components. Exact pinned versions"
    echo "are recorded in vcpkg.json (baseline + overrides) and in the SPDX SBOM"
    echo "generated for each build; per-port license text follows verbatim from the"
    echo "vcpkg 'copyright' file of each installed port."
    echo
    echo "Generated from: ${TRIPLET_DIR}"
    echo "Generated date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo

    for copyright_file in "${TRIPLET_DIR}"/share/*/copyright; do
        port_dir="$(dirname "${copyright_file}")"
        port="$(basename "${port_dir}")"
        echo "================================================================================"
        echo "### ${port}"
        echo "================================================================================"
        cat "${copyright_file}"
        echo
    done
} > "${OUT}"

echo "wrote ${OUT}"
