#!/usr/bin/env bash
# Canonical environment setup (Linux)
# See: .github/workflows/copilot-setup-steps.yml

set -euo pipefail

# -----------------------------------------------------------------------------
# Environment variables specify versions to use
# -----------------------------------------------------------------------------
export NODE_VERSION_PIN=22.18.0
export NPM_VERSION_PIN=10.1.0
export PNPM_VERSION_PIN=8.11.0
export YARN_VERSION_PIN=3.6.0
export PLAYWRIGHT_CLI=1.44.1
export PLAYWRIGHT_BROWSERS=chromium,firefox,webkit
export PWSH_VERSION=7.4.6
export GCLOUD_SDK=463.0.0
export GH_CLI=2.37.0
export TERRAFORM=1.6.15
export ANSIBLE=8.9.0
export FIREBASE_TOOLS=11.11.0
export CDKTF=0.16.0
export DOTNET_VERSION_PIN=9.0.102
export DOTNET_CHANNEL=9.0
export DOTNET_QUALITY=GA


echo "=== Starting environment setup (Dockerfile -> shell script) ==="

# -----------------------------------------------------------------------------
# Environment variables equivalent to Dockerfile ENV lines
# -----------------------------------------------------------------------------
export DOTNET_CLI_TELEMETRY_OPTOUT=1
export DOTNET_SKIP_FIRST_TIME_EXPERIENCE=1
export DOTNET_NOLOGO=1
export ASPNETCORE_ENVIRONMENT=Development
export DEBIAN_FRONTEND=noninteractive

# Detect sudo availability (GitHub runners have it by default)
if command -v sudo >/dev/null 2>&1; then
	SUDO="sudo"
else
	SUDO=""
fi

# -----------------------------------------------------------------------------
# Minimal mode control
# - Default: OFF (unset/empty/"0"/"false"/"no"/"n" -> normal/full setup)
# - Enable only when explicitly truthy: 1/true/yes/y (case-insensitive)
# -----------------------------------------------------------------------------
is_true() {
	case "${1:-}" in
		1|true|TRUE|True|yes|YES|y|Y) return 0 ;;
		*) return 1 ;;
	esac
}

# -----------------------------------------------------------------------------
# Node.js: NVM everywhere with exact version pinning (required)
# - Reads exact version from .nvmrc or NODE_VERSION_PIN (e.g., 22.18.0)
# - Exits with error if no exact version provided to ensure determinism
# - npm stays pinned to the version bundled with Node, unless NPM_VERSION_PIN is set
# -----------------------------------------------------------------------------
install_node_via_nvm() {
	# Install NVM if missing (clone repo directly to avoid installer non-zero exits in CI)
	export NVM_DIR="${NVM_DIR:-$HOME/.nvm}"
	# Ensure parent exists, but let git create the final directory to avoid clone errors
	mkdir -p "$(dirname "$NVM_DIR")"
	if [ ! -s "$NVM_DIR/nvm.sh" ]; then
		# If the directory exists but is empty, remove it so git can create it
		if [ -d "$NVM_DIR" ] && [ -z "$(ls -A "$NVM_DIR" 2>/dev/null)" ]; then rmdir "$NVM_DIR" || true; fi
		echo "[nvm] Cloning NVM to $NVM_DIR (v0.39.7)"
		git clone --depth=1 -b v0.39.7 https://github.com/nvm-sh/nvm.git "$NVM_DIR" || true
	fi
	# shellcheck disable=SC1090
	if [ -s "$NVM_DIR/nvm.sh" ]; then
		# Some nvm versions attempt an automatic `nvm use` on sourcing when an .nvmrc is present,
		# which may exit non-zero before the desired version is installed. Temporarily disable -e.
		set +e
		# shellcheck source=/dev/null
		. "$NVM_DIR/nvm.sh"
		source_rc=$?
		set -e
		if [ ${source_rc} -ne 0 ]; then
			echo "[nvm] Sourcing nvm.sh returned non-zero (${source_rc}); continuing with explicit install" >&2
		fi
	else
		echo "ERROR: NVM not found at $NVM_DIR/nvm.sh after clone" >&2
		exit 1
	fi

	# Verify nvm is available after sourcing
	if ! command -v nvm >/dev/null 2>&1; then
		echo "ERROR: 'nvm' command not found after sourcing $NVM_DIR/nvm.sh" >&2
		exit 1
	fi

	# Choose desired version: from NODE_VERSION_PIN, or .nvmrc, else fallback to lts/* (not truly pinned)
	if [ -n "${NODE_VERSION_PIN:-}" ]; then
		DESIRED_NODE_VERSION="$NODE_VERSION_PIN"
	elif [ -f .nvmrc ]; then
		DESIRED_NODE_VERSION="$(cat .nvmrc)"
	else
		echo "ERROR: No exact Node version specified. Set NODE_VERSION_PIN or add .nvmrc. Expected format: e.g., '22.18.0'" >&2
		exit 1
	fi
	echo "[nvm] Ensuring Node $DESIRED_NODE_VERSION"
	nvm install "$DESIRED_NODE_VERSION"
	nvm alias default "$DESIRED_NODE_VERSION"
	nvm use default

	# npm pinning: if NPM_VERSION_PIN is set, install that exact version; otherwise keep the bundled one
	if [ -n "${NPM_VERSION_PIN:-}" ]; then
		npm install -g --no-audit --no-fund "npm@${NPM_VERSION_PIN}" || true
	fi
}

echo "[1/14] Installing base system packages"
$SUDO apt-get update -y
$SUDO apt-get install -y --no-install-recommends \
	build-essential \
	apt-transport-https \
	ca-certificates \
	gnupg \
	curl \
	wget \
	unzip \
	git \
	vim \
	nano \
	jq \
	tree \
	htop \
	python3 \
	python3-pip \
	python3-venv \
	software-properties-common \
	lsb-release

echo "[2/14] Ensuring 'python' points to python3"
if [ ! -e /usr/bin/python ]; then
	$SUDO ln -sf /usr/bin/python3 /usr/bin/python
fi

echo "[3/14] Installing Node.js via NVM (exact pin)"
install_node_via_nvm
echo "[4/14] NVM installed and active"

echo "[5/14] Enabling Corepack (pnpm & yarn) deterministically"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping Corepack activation"
elif command -v corepack >/dev/null 2>&1; then
    corepack enable || true
    # Determine pnpm version: PNPM_VERSION_PIN > package.json packageManager field > package.json pnpm field
    PNPM_VER=""
    if [ -n "${PNPM_VERSION_PIN:-}" ]; then
        PNPM_VER="$PNPM_VERSION_PIN"
    elif [ -f package.json ]; then
        PM_FIELD=$(grep -o '"packageManager"[^\n]*' package.json || true)
        if echo "$PM_FIELD" | grep -q 'pnpm@'; then
            PNPM_VER=$(echo "$PM_FIELD" | sed -E 's/.*"packageManager" *: *"pnpm@([^"]+)".*/\1/')
        else
            PNPM_VER=$(grep -o '"pnpm" *: *"[^"]*"' package.json | sed -E 's/.*"pnpm" *: *"([^"]+)".*/\1/' || true)
        fi
    fi
    if [ -n "$PNPM_VER" ]; then
        corepack prepare "pnpm@$PNPM_VER" --activate || true
    else
        echo "pnpm version not specified; skipping pnpm activation (set PNPM_VERSION_PIN or packageManager)." >&2
    fi

    # Determine yarn version: YARN_VERSION_PIN > package.json packageManager field
    YARN_VER=""
    if [ -n "${YARN_VERSION_PIN:-}" ]; then
        YARN_VER="$YARN_VERSION_PIN"
    elif [ -f package.json ]; then
        PM_FIELD=$(grep -o '"packageManager"[^\n]*' package.json || true)
        if echo "$PM_FIELD" | grep -q 'yarn@'; then
            YARN_VER=$(echo "$PM_FIELD" | sed -E 's/.*"packageManager" *: *"yarn@([^"]+)".*/\1/')
        fi
    fi
    if [ -n "$YARN_VER" ]; then
        corepack prepare "yarn@$YARN_VER" --activate || true
    else
        echo "yarn version not specified; skipping yarn activation (set YARN_VERSION_PIN or packageManager)." >&2
    fi

    echo "Package managers:"
    (npm -v 2>/dev/null || true) | sed 's/^/ - npm /'
    (pnpm -v 2>/dev/null || true) | sed 's/^/ - pnpm /'
    (yarn -v 2>/dev/null || true) | sed 's/^/ - yarn /'
else
    echo "corepack not found; skipping pnpm/yarn activation" >&2
fi

echo "[6/14] Installing uv (Python package/dependency manager)"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping uv install"
elif ! command -v uv >/dev/null 2>&1; then
	curl -LsSf https://astral.sh/uv/install.sh -o /tmp/uv-install.sh
	sh /tmp/uv-install.sh >/dev/null 2>&1 || sh /tmp/uv-install.sh
	export PATH="$HOME/.local/bin:$PATH"
fi

echo "[7/14] Installing Playwright CLI and browsers (chromium, firefox, webkit)"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping Playwright install"
else
	# Install Playwright CLI and browsers. --with-deps installs required system libraries on Ubuntu runners.
	npx -y playwright@latest install --with-deps chromium firefox webkit || \
	npx -y playwright@latest install chromium firefox webkit || true
	npx -y playwright@latest --version || true
fi

echo "[8/14] Installing PowerShell"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping PowerShell install"
elif ! command -v pwsh >/dev/null 2>&1; then
	wget -q "https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb" -O packages-microsoft-prod.deb
	$SUDO dpkg -i packages-microsoft-prod.deb
	rm -f packages-microsoft-prod.deb
	$SUDO apt-get update -y
	$SUDO apt-get install -y --no-install-recommends powershell
fi

echo "[9/14] Installing Google Cloud CLI"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping Google Cloud CLI install"
elif ! command -v gcloud >/dev/null 2>&1; then
	echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | $SUDO tee /etc/apt/sources.list.d/google-cloud-sdk.list >/dev/null
	curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | $SUDO gpg --dearmor -o /usr/share/keyrings/cloud.google.gpg
	$SUDO apt-get update -y
	$SUDO apt-get install -y --no-install-recommends google-cloud-cli
fi

echo "[10/14] Installing GitHub CLI (gh)"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping GitHub CLI install"
elif ! command -v gh >/dev/null 2>&1; then
	curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | $SUDO dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
	$SUDO chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg
	echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | $SUDO tee /etc/apt/sources.list.d/github-cli.list >/dev/null
	$SUDO apt-get update -y
	$SUDO apt-get install -y --no-install-recommends gh
fi

echo "[11/14] Installing Terraform"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping Terraform install"
elif ! command -v terraform >/dev/null 2>&1; then
	curl -fsSL https://apt.releases.hashicorp.com/gpg | $SUDO gpg --dearmor -o /usr/share/keyrings/hashicorp-archive-keyring.gpg
	echo "deb [signed-by=/usr/share/keyrings/hashicorp-archive-keyring.gpg] https://apt.releases.hashicorp.com $(lsb_release -cs) main" | $SUDO tee /etc/apt/sources.list.d/hashicorp.list
	$SUDO apt-get update -y
	$SUDO apt-get install -y --no-install-recommends terraform
fi

echo "[12/14] Installing Ansible"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping Ansible install"
elif ! command -v ansible >/dev/null 2>&1; then
	$SUDO apt-get update -y
	$SUDO add-apt-repository --yes --update ppa:ansible/ansible
	$SUDO apt-get install -y --no-install-recommends ansible
fi

echo "[13/14] Installing global npm CLI tools (firebase-tools, angular, CRA, typescript, eslint, prettier, cdktf)"
if is_true "${SETUP_MINIMAL:-}"; then
	echo "SETUP_MINIMAL: Skipping global npm CLI tool installs"
else
	# With NVM-managed Node, install globals without sudo to the user scope
	npm install -g --no-audit --no-fund firebase-tools @angular/cli create-react-app typescript eslint prettier cdktf-cli
fi

echo "[14/14] Ensuring .NET 10 Preview SDK and workloads (wasm-tools)"
# If dotnet missing or major version < 10, install via dotnet-install script
if command -v dotnet >/dev/null 2>&1; then
	DOTNET_VER=$(dotnet --version || echo "0")
else
	DOTNET_VER="0"
fi
if ! echo "$DOTNET_VER" | grep -q '^10\.'; then
	# Allow override via env:
	#   DOTNET_VERSION_PIN: exact SDK version (e.g., 10.0.100-preview.7.XXXXX)
	#   DOTNET_CHANNEL: channel family (default 10.0)
	#   DOTNET_QUALITY: desired quality (default preview)
	DOTNET_CHANNEL_VAL="${DOTNET_CHANNEL:-10.0}"
	DOTNET_QUALITY_VAL="${DOTNET_QUALITY:-preview}"
	echo "Installing .NET ${DOTNET_CHANNEL_VAL} (${DOTNET_QUALITY_VAL}) SDK locally (dotnet-install script)"
	curl -sSL https://dot.net/v1/dotnet-install.sh -o /tmp/dotnet-install.sh
	if [ -n "${DOTNET_VERSION_PIN:-}" ]; then
		bash /tmp/dotnet-install.sh --version "$DOTNET_VERSION_PIN" --install-dir "$HOME/.dotnet" --no-path
	else
		bash /tmp/dotnet-install.sh --channel "$DOTNET_CHANNEL_VAL" --quality "$DOTNET_QUALITY_VAL" --install-dir "$HOME/.dotnet" --no-path
	fi
	export PATH="$HOME/.dotnet:$PATH"
fi
if command -v dotnet >/dev/null 2>&1; then
	# Run dotnet commands outside the repo to avoid global.json pinning to an older SDK
	set +e
	(
		cd /tmp || exit 0
		dotnet workload update || true
		dotnet workload install wasm-tools || true
		dotnet new install Aspire.ProjectTemplates || true
	)
	set -e
else
	echo "dotnet not found on PATH; skipping workload installation (consider using actions/setup-dotnet)." >&2
fi

echo "Creating environment summary script (/tmp/show-env.sh)"
cat <<'EOF' >/tmp/show-env.sh
#!/usr/bin/env bash
echo "=== AI Agent Instructions Development Environment (Script Install) ==="
echo "Core Development Stack:"
echo "- .NET SDK: $(command -v dotnet >/dev/null 2>&1 && (cd /tmp && dotnet --version) || echo 'Not Installed')"
echo "- Node.js: $(node --version 2>/dev/null || echo 'Not Installed')"
echo "- npm: $(npm --version 2>/dev/null || echo 'Not Installed')"
echo "- Python: $(python --version 2>/dev/null || echo 'Not Installed')"
echo "- PowerShell: $(pwsh --version 2>/dev/null || echo 'Not Installed')"
echo "- uv: $(uv --version 2>/dev/null || echo 'Not Installed')"
echo

echo "Cloud & DevOps Tools:"
if command -v gcloud >/dev/null 2>&1; then
	GCLOUD_VER=$(gcloud version 2>/dev/null | sed -n "s/^Google Cloud SDK \(.*\)$/\1/p" | head -1)
	echo "- Google Cloud CLI: ${GCLOUD_VER:-Installed}"
else
	echo "- Google Cloud CLI: Not Installed"
fi
echo "- Firebase CLI: $(firebase --version 2>/dev/null || echo 'Not Installed')"
echo "- GitHub CLI: $(gh --version 2>/dev/null | head -1 || echo 'Not Installed')"
echo "- Terraform: $(terraform --version 2>/dev/null | head -1 || echo 'Not Installed')"
echo "- Ansible: $(ansible --version 2>/dev/null | head -1 || echo 'Not Installed')"
echo "- CDKTF: $(cdktf --version 2>/dev/null || echo 'Not Installed')"
echo "- Playwright: $(npx -y playwright@latest --version 2>/dev/null || echo 'Not Installed')"
echo

echo "Ready for ASP.NET Core + Blazor + AI + Google Cloud development!"
EOF
chmod +x /tmp/show-env.sh

echo "Environment setup complete. Summary:"
/tmp/show-env.sh

echo "=== Finished environment setup ==="
