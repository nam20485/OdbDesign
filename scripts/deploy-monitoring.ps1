param(
    [switch]$Wait,
    [int]$WaitTimeoutSeconds = 600
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

function Test-Command {
    param([string]$Name)
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Ensure-Helm {
    if (Test-Command helm) {
        $version = (helm version --short 2>$null)
        Write-Host "helm already installed: $version"
        return
    }

    Write-Host "helm not found on PATH. Attempting install..."

    if (Test-Command winget) {
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
        throw "No supported package manager found (winget / choco / scoop). Install helm manually from https://helm.sh/docs/intro/install/ and rerun."
    }

    # winget/choco add to Machine PATH; refresh current session
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + `
                ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

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
        $names = kubectl get $k -n $Namespace -o name 2>$null
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

helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm repo update

helm upgrade --install prom prometheus-community/kube-prometheus-stack -n monitoring --values ./deploy/helm/values-prom.yaml
kubectl --namespace monitoring get pods -l "release=prom"

if ($Wait) {
    Write-Host "Waiting for monitoring stack rollouts (timeout ${WaitTimeoutSeconds}s)..."
    Wait-RolloutInNamespace -Namespace "monitoring" -TimeoutSeconds $WaitTimeoutSeconds
}

#kubectl delete -f https://raw.githubusercontent.com/aquasecurity/trivy-operator/v0.1.5/deploy/static/trivy-operator.yaml
helm upgrade --install trivy-operator aqua/trivy-operator --namespace trivy-system --create-namespace --version 0.11.0 --values ./deploy/helm/values-trivy.yaml

if ($Wait) {
    Write-Host "Waiting for Trivy Operator rollouts (timeout ${WaitTimeoutSeconds}s)..."
    Wait-RolloutInNamespace -Namespace "trivy-system" -TimeoutSeconds $WaitTimeoutSeconds
}
