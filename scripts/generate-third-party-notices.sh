#!/usr/bin/env bash
#
# generate-third-party-notices.sh — assemble THIRD_PARTY_NOTICES.txt from the
# copyright files vcpkg installs for every port in a vcpkg_installed triplet
# directory. Run after a successful configure/build so vcpkg_installed is
# populated, then commit the output (or let the commercial packaging pipeline
# regenerate it per delivery).
#
# Usage:
#   scripts/generate-third-party-notices.sh [<vcpkg-installed-triplet-dir>] [<out-file>]
#
# <vcpkg-installed-triplet-dir> defaults to auto-detection (see below);
# <out-file> defaults to THIRD_PARTY_NOTICES.txt in the repository root.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

TRIPLET_DIR="${1:-}"
if [[ -z "${TRIPLET_DIR}" ]]; then
    # Auto-detect the installed-port set, preferring the dynamic triplet (the
    # shared-runtime release configuration). Two layouts exist:
    #   1. shared VCPKG_INSTALLED_DIR: out/vcpkg_installed/<triplet>/ (current)
    #   2. legacy per-build dirs: out/build/<preset>/vcpkg_installed/<triplet>/
    # Set PREFERRED_BUILD to override the legacy fallback preset.
    prefer_build="${PREFERRED_BUILD:-linux-dynamic-release}"
    for cand in "${REPO_ROOT}"/out/vcpkg_installed/*-dynamic/ \
                "${REPO_ROOT}"/out/vcpkg_installed/*/ \
                "${REPO_ROOT}/out/build/${prefer_build}/vcpkg_installed/"*-dynamic/ \
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
    echo "usage: $0 [<vcpkg-installed-triplet-dir>] [<out-file>]   (e.g. out/build/linux-dynamic-release/vcpkg_installed/x64-linux-dynamic)" >&2
    exit 1
fi
TRIPLET_DIR="${TRIPLET_DIR%/}"

# display the triplet dir repo-relative in the generated file — absolute
# paths would leak machine layout and churn the committed diff
TRIPLET_DIR_DISPLAY="${TRIPLET_DIR#"$REPO_ROOT"/}"

OUT="${2:-${REPO_ROOT}/THIRD_PARTY_NOTICES.txt}"

{
    echo "THIRD-PARTY SOFTWARE NOTICES AND INFORMATION"
    echo
    echo "OdbDesign (https://github.com/nam20485/OdbDesign) is licensed under the"
    echo "GNU Affero General Public License v3.0 (see LICENSE). Binary distributions"
    echo "embed or link the following third-party components. Exact pinned versions"
    echo "are recorded in vcpkg.json (package overrides) and vcpkg-configuration.json"
    echo "(registry baseline), and in the SPDX SBOM generated for each build;"
    echo "per-port license text follows verbatim from the vcpkg 'copyright' file"
    echo "of each installed port."
    echo
    echo "Generated from: ${TRIPLET_DIR_DISPLAY} via scripts/generate-third-party-notices.sh"
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
