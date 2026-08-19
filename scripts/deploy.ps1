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
