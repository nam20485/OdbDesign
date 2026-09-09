param(
    # Kubernetes context to use. Omit to use the current context (e.g. k3s "default").
    # For the old k3d cluster pass "k3d-k3dcluster".
    [Parameter(Mandatory=$false)]
    [string]$ClusterName = "",
    # Deployment name
    [Parameter(Mandatory=$false)]
    [string]$DeploymentName = "odbdesign-server-v1",
    # Skip post-deploy gRPC validation
    [switch]$SkipGrpcValidation = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot

# The /usr/local/bin/kubectl symlink is k3s itself; its wrapper forces
# KUBECONFIG=/etc/rancher/k3s/k3s.yaml (root-only, mode 600) unless KUBECONFIG
# is already set. Pin it to the user kubeconfig created by k3s-cluster.ps1 Install.
if ([string]::IsNullOrEmpty($env:KUBECONFIG)) {
    $userKubeConfig = Join-Path $HOME ".kube/config"
    if (Test-Path $userKubeConfig) {
        $env:KUBECONFIG = $userKubeConfig
    }
}

function Invoke-Kubectl {
    param([Parameter(Mandatory=$true)][string[]]$Arguments)

    & kubectl @Arguments
    if ($LASTEXITCODE -ne 0) {
        Write-Host "kubectl $($Arguments -join ' ') failed with exit code $LASTEXITCODE."
        Exit 1
    }
}

# set kubeconfig context (optional; omit to deploy to the current context, e.g. k3s)
if (-not [string]::IsNullOrWhiteSpace($ClusterName)) {
    Invoke-Kubectl @("config", "use-context", $ClusterName)
}

Push-Location $RepoRoot
try {
    #
    # Common (pre)
    #

    # persistent volume
    # spec.hostPath is immutable: on a cluster that already has this PV with the
    # old path, apply fails with "field is immutable" and aborts the whole deploy.
    # Pre-flight with an actionable error instead of the cryptic apply failure.
    $expectedHostPath = '/srv/odbdesign-volume'
    $existingHostPath = $null
    try {
        $existingHostPath = (kubectl get pv k3d-volume -o jsonpath='{.spec.hostPath.path}' 2>$null) -join ''
    }
    catch {
        # PV does not exist yet (fresh cluster) — nothing to pre-flight
    }
    if ($existingHostPath -and $existingHostPath -ne $expectedHostPath) {
        Write-Host "PV 'k3d-volume' exists with hostPath '$existingHostPath', but this manifest declares '$expectedHostPath'."
        Write-Host "spec.hostPath is immutable, so kubectl apply would abort the whole deploy."
        Write-Host "Manual recovery: scale down workloads using PVC k3d-volume-claim, move any data from '$existingHostPath' to '$expectedHostPath', delete the PVC then the PV, and re-run this deploy."
        Exit 1
    }
    Invoke-Kubectl @("apply", "-f", "deploy/kube/k3d-volume-pv.yaml")
    Invoke-Kubectl @("apply", "-f", "deploy/kube/k3d-volume-pvc.yaml")

    #
    # OdbDesignServer
    #

    # secrets
    & (Join-Path $PSScriptRoot "odbdesign-server-request-secret.ps1")
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to create/update the request secret."
        Exit 1
    }

    # apply deployment/service manifests
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/deployment.yaml")
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/service.yaml")
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/service-grpc.yaml")

    # restart deployment
    Invoke-Kubectl @("rollout", "restart", "deployment/$DeploymentName")
    Invoke-Kubectl @("rollout", "status", "deployment/$DeploymentName", "--timeout=300s")


    #
    # Swagger UI
    #

    # (re)generate the swagger spec ConfigMap manifest from the repo swagger
    # file, then apply it; the public swaggerui image bakes an outdated spec,
    # the deployment mounts this ConfigMap over it. Regenerating the committed
    # file here keeps the manifest set self-contained (raw apply / GitOps can
    # resolve the volume) without letting it drift past the last deploy.
    $specPath = "swagger/odbdesign-server-0.9-swagger.yaml"
    $configMapPath = "deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml"
    kubectl create configmap odbdesign-server-swagger-spec `
        --from-file=odbdesign-server-0.9-swagger.yaml=$specPath `
        --dry-run=client -o yaml > $configMapPath
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to generate the swagger spec ConfigMap manifest."
        Exit 1
    }
    Invoke-Kubectl @("apply", "-f", $configMapPath)

    # apply deployment/service manifests
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml")
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer-SwaggerUI/service.yaml")

    # restart deployment
    Invoke-Kubectl @("rollout", "restart", "deployment/odbdesign-server-swaggerui-v1")
    Invoke-Kubectl @("rollout", "status", "deployment/odbdesign-server-swaggerui-v1", "--timeout=300s")

    #
    # Common (post)
    #

    # apply ingress manifest
    Invoke-Kubectl @("apply", "-f", "deploy/kube/local-ingress.yaml")

    if (-not $SkipGrpcValidation) {
        $validateArgs = @{ DeploymentName = $DeploymentName }
        if (-not [string]::IsNullOrWhiteSpace($ClusterName)) {
            $validateArgs.ClusterName = $ClusterName
        }

        & (Join-Path $PSScriptRoot "validate-grpc-exposure.ps1") @validateArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "gRPC exposure validation failed."
            Exit 1
        }
    }
}
finally {
    Pop-Location
}
