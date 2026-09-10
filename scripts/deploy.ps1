param(
    # Kubernetes context to use. Omit to use the current context (e.g. k3s "default").
    # For the old k3d cluster pass "k3d-k3dcluster".
    [Parameter(Mandatory=$false)]
    [string]$ClusterName = "",
    # Deployment name
    [Parameter(Mandatory=$false)]
    [string]$DeploymentName = "odbdesign-server-v1",
    # Skip post-deploy gRPC validation
    [switch]$SkipGrpcValidation = $false,
    # TLS (https-tls-options.md Option C + M3.1 gRPC TLS): installs vendored
    # cert-manager, applies the CA Issuer + Certificate (waits for READY),
    # adds the Traefik `grpc` entrypoint + IngressRouteTCP (TLS-terminated
    # gRPC on node :50051) and migrates the gRPC service to ClusterIP.
    # Default (no flag) = plain-HTTP deployment, unchanged.
    [switch]$EnableTls = $false
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

    # apply deployment/service manifests; the gRPC service variant depends on
    # the TLS mode: -EnableTls routes gRPC through Traefik (ClusterIP backend
    # of the IngressRouteTCP), otherwise ServiceLB publishes node :50051
    # directly (LoadBalancer variant — the pre-TLS behavior).
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/deployment.yaml")
    Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/service.yaml")
    if ($EnableTls) {
        Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/service-grpc.yaml")
    }
    else {
        Invoke-Kubectl @("apply", "-f", "deploy/kube/OdbDesignServer/service-grpc-loadbalancer.yaml")
    }

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

    # TLS (optional; -EnableTls): cert-manager + private-CA Certificate, then
    # the Traefik gRPC TLS path. Applied before the ingress so the secret the
    # ingress tls block references already exists when Traefik picks the
    # ingress up. See deploy/kube/cert-manager/README.md.
    if ($EnableTls) {
        # cert-manager (vendored, version-pinned v1.17.2)
        Invoke-Kubectl @("apply", "-f", "deploy/kube/cert-manager/cert-manager.yaml")
        foreach ($certManagerDeployment in @("cert-manager", "cert-manager-cainjector", "cert-manager-webhook")) {
            Invoke-Kubectl @("-n", "cert-manager", "rollout", "status", "deployment/$certManagerDeployment", "--timeout=300s")
        }

        # CA Issuer + serving Certificate. Requires the odbdesign-root-ca
        # secret to exist first (scripts/make-tls-ca.sh, one-time).
        # try/catch + 2>$null mirrors the PV pre-flight above: under
        # ErrorActionPreference=Stop a missing secret can surface as a
        # thrown record, not just a non-zero exit code.
        $rootCaSecret = $null
        try {
            $rootCaSecret = (kubectl get secret odbdesign-root-ca -n default --no-headers 2>$null) -join ''
        }
        catch {
            # secret does not exist yet — actionable error below
        }
        if ([string]::IsNullOrEmpty("$rootCaSecret")) {
            Write-Host "Secret 'odbdesign-root-ca' not found in namespace 'default'."
            Write-Host "Generate and import the root CA first (one-time):"
            Write-Host "  ./scripts/make-tls-ca.sh <output-dir>"
            Write-Host "  kubectl create secret tls odbdesign-root-ca --cert=<output-dir>/ca.crt --key=<output-dir>/ca.key -n default"
            Exit 1
        }
        Invoke-Kubectl @("apply", "-f", "deploy/kube/issuer-ca.yaml")
        Invoke-Kubectl @("apply", "-f", "deploy/kube/certificate-odbs-server.yaml")

        # Wait for the Certificate to go READY with a clear, actionable error.
        Write-Host "Waiting for certificate/odbdesign-server-tls to become READY..."
        kubectl wait --for=condition=Ready certificate/odbdesign-server-tls --timeout=300s
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Certificate 'odbdesign-server-tls' did not become READY within 300s."
            Write-Host "Diagnose with: kubectl describe certificate odbdesign-server-tls"
            Write-Host "  kubectl get issuer odbdesign-ca -o yaml   (and 'kubectl describe secret odbdesign-root-ca')"
            Write-Host "  kubectl -n cert-manager logs deploy/cert-manager -f"
            Exit 1
        }

        # gRPC over TLS: Traefik `grpc` entrypoint (:50051) + TCP route
        # terminating TLS to the ClusterIP gRPC service (h2c in-cluster).
        # HelmChartConfig is named after the packaged chart; if the cluster
        # already has its own traefik HelmChartConfig with other values, merge
        # the ports block manually instead (see manifest comments).
        Invoke-Kubectl @("apply", "-f", "deploy/kube/traefik-helmchartconfig-grpc-entrypoint.yaml")
        Invoke-Kubectl @("-n", "kube-system", "rollout", "status", "deployment/traefik", "--timeout=300s")
        Invoke-Kubectl @("apply", "-f", "deploy/kube/odbdesign-grpc-ingressroute-tcp.yaml")
    }

    # apply ingress manifest
    Invoke-Kubectl @("apply", "-f", "deploy/kube/local-ingress.yaml")

    if (-not $SkipGrpcValidation) {
        $validateArgs = @{ DeploymentName = $DeploymentName }
        if (-not [string]::IsNullOrWhiteSpace($ClusterName)) {
            $validateArgs.ClusterName = $ClusterName
        }
        if ($EnableTls) {
            $validateArgs.Tls = $true
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
