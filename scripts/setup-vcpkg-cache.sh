#!/usr/bin/env bash
#
# setup-vcpkg-cache.sh
# ---------------------
# Linux/Bash equivalent of scripts/setup-vcpkg-cache.ps1.
#
# Configures the local machine to consume the OdbDesign vcpkg binary cache
# stored in GitHub Packages. After running, set VCPKG_BINARY_SOURCES in your
# shell (or shell rc) to the value printed at the end of this script.
#
# Required env: NUGET_AUTH_TOKEN  (GitHub PAT with read:packages scope)
#
# Optional env:
#   VCPKG_CACHE_USERNAME  (default: nam20485)
#   VCPKG_CACHE_FEED_URL  (default: https://nuget.pkg.github.com/nam20485/index.json)
#
# Usage:
#   NUGET_AUTH_TOKEN=ghp_xxx ./scripts/setup-vcpkg-cache.sh
#   NUGET_AUTH_TOKEN=ghp_xxx ./scripts/setup-vcpkg-cache.sh --add-to-bashrc
#   NUGET_AUTH_TOKEN=ghp_xxx ./scripts/setup-vcpkg-cache.sh --check    # only verify, don't write
#
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
USERNAME="${VCPKG_CACHE_USERNAME:-nam20485}"
FEED_URL="${VCPKG_CACHE_FEED_URL:-https://nuget.pkg.github.com/${USERNAME}/index.json}"
SOURCE_NAME="GitHubPackages-OdbDesign"
LOCAL_CONFIG="${REPO_ROOT}/local.nuget.config"
ADD_TO_BASHRC=0
CHECK_ONLY=0

usage() {
    sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
    exit "${1:-0}"
}

for arg in "$@"; do
    case "$arg" in
        --add-to-bashrc) ADD_TO_BASHRC=1 ;;
        --check)         CHECK_ONLY=1 ;;
        -h|--help)       usage 0 ;;
        *)               echo "Unknown arg: $arg" >&2; usage 1 ;;
    esac
done

c_red()    { printf '\033[31m%s\033[0m\n' "$*"; }
c_green()  { printf '\033[32m%s\033[0m\n' "$*"; }
c_yellow() { printf '\033[33m%s\033[0m\n' "$*"; }
c_cyan()   { printf '\033[36m%s\033[0m\n' "$*"; }

echo
c_cyan "=== vcpkg Binary Cache Setup for OdbDesign (Linux) ==="
echo

# Verify PAT present
if [[ -z "${NUGET_AUTH_TOKEN:-}" ]]; then
    c_red "Error: NUGET_AUTH_TOKEN env var is not set."
    echo "Create a GitHub PAT with read:packages scope and re-run:" >&2
    echo "  https://github.com/settings/tokens/new?scopes=read:packages" >&2
    exit 1
fi

c_green "Using feed: ${FEED_URL}"
c_green "Using username: ${USERNAME}"
echo

# --check: validate the credentials by hitting the feed index, WITHOUT writing
# any files (in particular, not the credential-bearing local.nuget.config).
# Useful as a read-only smoke test before kicking off a long build.
if [[ "${CHECK_ONLY}" -eq 1 ]]; then
    c_cyan "Running --check: probing ${FEED_URL} ..."
    http_code="$(curl -sS -o /dev/null -w '%{http_code}' \
        -u "${USERNAME}:${NUGET_AUTH_TOKEN}" \
        "${FEED_URL}" || echo "000")"
    if [[ "${http_code}" =~ ^2 ]]; then
        c_green "OK (HTTP ${http_code}) — credentials accepted by GitHub Packages."
        exit 0
    elif [[ "${http_code}" == "401" ]]; then
        c_red "FAIL (HTTP 401) — PAT is invalid or lacks read:packages scope."
        exit 1
    else
        c_yellow "WARN (HTTP ${http_code}) — could not confirm; check connectivity."
        exit 2
    fi
fi

# Generate local.nuget.config with embedded credentials.
# Pattern matches scripts/setup-vcpkg-cache.ps1 exactly so behaviour is portable.
cat > "${LOCAL_CONFIG}" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <packageSources>
        <clear />
        <add key="${SOURCE_NAME}" value="${FEED_URL}" />
    </packageSources>
    <packageSourceCredentials>
        <${SOURCE_NAME}>
            <add key="Username" value="${USERNAME}" />
            <add key="ClearTextPassword" value="${NUGET_AUTH_TOKEN}" />
        </${SOURCE_NAME}>
    </packageSourceCredentials>
</configuration>
EOF
chmod 600 "${LOCAL_CONFIG}"

c_green "Wrote ${LOCAL_CONFIG} (mode 600)"

# Compose the VCPKG_BINARY_SOURCES value.
# ,read at the end enforces read-only access — local builds must never push.
ENV_VALUE="clear;nugetconfig,${LOCAL_CONFIG},read"

echo
c_yellow "To use the binary cache, set this env var in your shell:"
echo
echo "    export VCPKG_BINARY_SOURCES=\"${ENV_VALUE}\""
echo

if [[ "${ADD_TO_BASHRC}" -eq 1 ]]; then
    RC="${HOME}/.bashrc"
    [[ -f "${RC}" ]] || touch "${RC}"
    # Idempotent: strip any prior VCPKG_BINARY_SOURCES line first.
    if grep -q '^[[:space:]]*export[[:space:]]\+VCPKG_BINARY_SOURCES=' "${RC}"; then
        sed -i.bak -E '/^[[:space:]]*export[[:space:]]+VCPKG_BINARY_SOURCES=/d' "${RC}"
        rm -f "${RC}.bak"
    fi
    {
        echo
        echo "# vcpkg binary cache (GitHub Packages) — managed by scripts/setup-vcpkg-cache.sh"
        echo "export VCPKG_BINARY_SOURCES=\"${ENV_VALUE}\""
    } >> "${RC}"
    c_green "Added export to ${RC} (existing value overwritten if any)."
fi

echo
c_green "=== Setup complete ==="
echo
c_cyan "Local builds (and docker compose -f compose.local.yml) will now consume"
c_cyan "pre-built packages from GitHub Packages instead of building from source."
echo
