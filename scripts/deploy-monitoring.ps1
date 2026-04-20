$ErrorActionPreference = "Stop"

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

Ensure-Helm

kubectl create namespace monitoring --dry-run=client -o yaml | kubectl apply -f -

helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm repo update

helm upgrade --install prom prometheus-community/kube-prometheus-stack -n monitoring --values ./deploy/helm/values-prom.yaml
kubectl --namespace monitoring get pods -l "release=prom"

#kubectl delete -f https://raw.githubusercontent.com/aquasecurity/trivy-operator/v0.1.5/deploy/static/trivy-operator.yaml
helm upgrade --install trivy-operator aqua/trivy-operator --namespace trivy-system --create-namespace --version 0.11.0 --values ./deploy/helm/values-trivy.yaml
