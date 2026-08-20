param(
    [switch]$Wait,
    [int]$WaitTimeoutSeconds = 600
)

$ErrorActionPreference = "Stop"
# Fail the script when native commands (kubectl/helm) exit non-zero.
# Without this, $ErrorActionPreference only governs cmdlets — a failed
# `helm upgrade` was silently ignored (observed with the first loki install).
$PSNativeCommandUseErrorActionPreference = $true

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

# Chart version pins (resolved 2026-08-19 via `helm search repo`; update deliberately):
#   kube-prometheus-stack 88.5.0  (app v0.93.1)
#   trivy-operator         0.35.0  (app 0.33.0)
#   loki                   7.3.0   (app 3.6.12, single-binary)
#   promtail               6.17.1  (app 3.5.1)
$KubePrometheusStackChartVersion = "88.5.0"
$TrivyOperatorChartVersion = "0.35.0"
$LokiChartVersion = "7.3.0"
$PromtailChartVersion = "6.17.1"

# The /usr/local/bin/kubectl symlink is k3s itself; its wrapper forces
# KUBECONFIG=/etc/rancher/k3s/k3s.yaml (root-only, mode 600) unless KUBECONFIG
# is already set. Pin it to the user kubeconfig created by k3s-cluster.ps1 Install.
if ([string]::IsNullOrEmpty($env:KUBECONFIG)) {
    $userKubeConfig = Join-Path $HOME ".kube/config"
    if (Test-Path $userKubeConfig) {
        $env:KUBECONFIG = $userKubeConfig
    }
}

function Test-Command {
    param([string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-HelmLinux {
    # No-sudo install: download the release tarball from get.helm.sh and extract
    # the binary to ~/.local/bin (sudo on the VM requires a password, so a
    # system-wide install is not reliable non-interactively).
    $installDir = Join-Path $HOME ".local/bin"
    $helmPath = Join-Path $installDir "helm"

    if (Test-Path $helmPath) {
        Write-Host "helm already installed: $(& $helmPath version --short)"
        if ($env:Path -notlike "*$installDir*") { $env:Path = "${installDir}:$env:Path" }
        return
    }

    Write-Host "helm not found. Installing latest helm for linux-amd64 to $installDir ..."
    $version = (Invoke-RestMethod -Uri "https://get.helm.sh/helm-latest-version" -TimeoutSec 30).Trim()
    $tarball = "helm-$version-linux-amd64.tar.gz"
    $tmpDir = New-Item -ItemType Directory -Path (Join-Path ([System.IO.Path]::GetTempPath()) "helm-install-$([guid]::NewGuid().ToString('N').Substring(0,8))") -Force

    try {
        Invoke-WebRequest -Uri "https://get.helm.sh/$tarball" -OutFile (Join-Path $tmpDir $tarball) -TimeoutSec 300
        tar -xzf (Join-Path $tmpDir $tarball) -C $tmpDir
        New-Item -ItemType Directory -Path $installDir -Force | Out-Null
        Move-Item -Path (Join-Path $tmpDir "linux-amd64/helm") -Destination $helmPath -Force
        chmod +x $helmPath
    }
    finally {
        Remove-Item -Path $tmpDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    if ($env:Path -notlike "*$installDir*") { $env:Path = "${installDir}:$env:Path" }

    if (-not (Test-Command helm)) {
        throw "helm install appeared to succeed but 'helm' is still not on PATH. Open a new shell and rerun."
    }

    Write-Host "helm installed: $(helm version --short)"
}

function Ensure-Helm {
    if (Test-Command helm) {
        $version = (helm version --short 2>$null)
        Write-Host "helm already installed: $version"
        return
    }

    Write-Host "helm not found on PATH. Attempting install..."

    if ($IsLinux) {
        Ensure-HelmLinux
    }
    elseif (Test-Command winget) {
        Write-Host "Installing Helm.Helm via winget..."
        winget install --id Helm.Helm --exact --silent `
            --accept-package-agreements --accept-source-agreements
    }
    elseif (Test-Command choco) {
        Write-Host "Installing kubernetes-helm via Chocolatey..."
        choco install kubernetes-helm -y
    }
    elseif (Test-Command scoop) {
        Write-Host "Installing helm via Scoop..."
        scoop install helm
    }
    else {
        throw "No supported package manager found (apt/Linux handled automatically; winget / choco / scoop on Windows). Install helm manually from https://helm.sh/docs/intro/install/ and rerun."
    }

    if (-not $IsLinux) {
        # winget/choco add to Machine PATH; refresh current session
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + `
                    ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    }

    if (-not (Test-Command helm)) {
        throw "helm install appeared to succeed but 'helm' is still not on PATH. Open a new shell and rerun."
    }

    $version = (helm version --short 2>$null)
    Write-Host "helm installed: $version"
}

function Wait-RolloutInNamespace {
    param(
        [string]$Namespace,
        [int]$TimeoutSeconds
    )
    $kinds = @("deploy", "sts", "ds")
    foreach ($k in $kinds) {
        # `kubectl get <kind>` exits non-zero when the namespace has no
        # objects of that kind — expected, not a failure (PSNativeCommand... throws otherwise)
        try { $names = kubectl get $k -n $Namespace -o name 2>$null }
        catch { continue }
        if (-not $names) { continue }
        foreach ($line in $names) {
            if ([string]::IsNullOrWhiteSpace($line)) { continue }
            Write-Host "  rollout status $line (namespace $Namespace)..."
            kubectl rollout status $line -n $Namespace --timeout="${TimeoutSeconds}s"
        }
    }
}

Ensure-Helm

kubectl create namespace monitoring --dry-run=client -o yaml | kubectl apply -f -

# Traefik strips /prometheus on ingress so Prometheus can keep API at / (Freelens, Grafana).
# NOTE: traefik.io/v1alpha1 — the k3s v1.36 cluster runs Traefik 3.x (2.x used traefik.containo.us).
kubectl apply -f ./deploy/kube/traefik-middleware-prometheus-stripprefix.yaml

helm repo add prometheus-community https://prometheus-community.github.io/helm-charts --force-update
helm repo add aqua https://aquasecurity.github.io/helm-charts/ --force-update
helm repo add grafana https://grafana.github.io/helm-charts --force-update
helm repo update

helm upgrade --install prom prometheus-community/kube-prometheus-stack `
    -n monitoring --version $KubePrometheusStackChartVersion --values ./deploy/helm/values-prom.yaml
kubectl --namespace monitoring get pods -l "release=prom"

if ($Wait) {
    Write-Host "Waiting for monitoring stack rollouts (timeout ${WaitTimeoutSeconds}s)..."
    Wait-RolloutInNamespace -Namespace "monitoring" -TimeoutSeconds $WaitTimeoutSeconds
}

# Loki (log store) + Promtail (DaemonSet log collector) — logs are viewable in Grafana
# via the provisioned "Loki" datasource (values-prom.yaml grafana.additionalDataSources).
# Single-binary Loki for this single-node VM; see deploy/helm/values-loki.yaml.
helm upgrade --install loki grafana/loki `
    -n monitoring --version $LokiChartVersion --values ./deploy/helm/values-loki.yaml

helm upgrade --install promtail grafana/promtail `
    -n monitoring --version $PromtailChartVersion --values ./deploy/helm/values-promtail.yaml

if ($Wait) {
    Write-Host "Waiting for Loki rollouts (timeout ${WaitTimeoutSeconds}s)..."
    Wait-RolloutInNamespace -Namespace "monitoring" -TimeoutSeconds $WaitTimeoutSeconds
}

# trivy serviceMonitor needs the prometheus-operator CRDs, so install after kube-prometheus-stack.
helm upgrade --install trivy-operator aqua/trivy-operator `
    --namespace trivy-system --create-namespace --version $TrivyOperatorChartVersion --values ./deploy/helm/values-trivy.yaml

if ($Wait) {
    Write-Host "Waiting for Trivy Operator rollouts (timeout ${WaitTimeoutSeconds}s)..."
    Wait-RolloutInNamespace -Namespace "trivy-system" -TimeoutSeconds $WaitTimeoutSeconds
}
