param(
    # Kubernetes context to use. Omit to use the current context (e.g. k3s "default").
    [Parameter(Mandatory=$false)]
    [string]$ClusterName = "",
    [Parameter(Mandatory=$false)]
    [string]$DeploymentName = "odbdesign-server-v1",
    [Parameter(Mandatory=$false)]
    [string]$GrpcServiceName = "odbdesign-server-grpc-service",
    [Parameter(Mandatory=$false)]
    [string]$GrpcPortName = "ods-grpc-port",
    [Parameter(Mandatory=$false)]
    [int]$GrpcPort = 50051,
    # Host/IP clients use to reach gRPC. Omit to auto-detect:
    #   k3s: the ServiceLB ingress IP (node IP)
    #   k3d: "precision5820" (legacy default)
    [Parameter(Mandatory=$false)]
    [string]$AdvertisedHost = "",
    # Cluster kind. "Auto" detects k3d from a "k3d-*" context name, otherwise assumes k3s.
    [Parameter(Mandatory=$false)]
    [ValidateSet('Auto','k3d','k3s')]
    [string]$ClusterKind = 'Auto',
    [Parameter(Mandatory=$false)]
    [string]$DesignName,
    [Parameter(Mandatory=$false)]
    [string]$StepName,
    [Parameter(Mandatory=$false)]
    [string]$LayerName,
    [switch]$SkipGrpcUrl = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[validate-grpc] $Message"
}

function Fail {
    param([string]$Message)
    throw "[validate-grpc] $Message"
}

function Require-Command {
    param([string]$Name)

    if ($null -eq (Get-Command $Name -ErrorAction SilentlyContinue)) {
        Fail "Required command '$Name' was not found in PATH."
    }
}

function Invoke-KubectlJson {
    param([string[]]$Arguments)

    $json = & kubectl @Arguments -o json
    if ($LASTEXITCODE -ne 0) {
        Fail "kubectl $($Arguments -join ' ') failed."
    }

    return $json | ConvertFrom-Json
}

function Normalize-ClusterShortName {
    param([string]$Name)

    if (-not [string]::IsNullOrEmpty($Name) -and $Name.StartsWith("k3d-")) {
        return $Name.Substring(4)
    }

    return $Name
}

function Test-GrpcUrlTarget {
    param(
        [string]$Target,
        [string]$GrpcUrlPath,
        [string]$ServiceProtoPath,
        [string]$GrpcProtoPath,
        [string]$ModelProtoPath,
        [string]$DesignName,
        [string]$StepName,
        [string]$LayerName
    )

    $healthArgs = @(
        "-plaintext",
        "-import-path", $GrpcProtoPath,
        "-import-path", $ModelProtoPath,
        "-proto", $ServiceProtoPath,
        "-d", '{"service":"OdbDesignService"}',
        $Target,
        "Odb.Grpc.OdbDesignService/HealthCheck"
    )

    $healthOutput = & $GrpcUrlPath @healthArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        Fail "grpcurl health check failed for $Target.`n$healthOutput"
    }

    Write-Step "grpcurl health check succeeded for $Target."

    if (-not [string]::IsNullOrWhiteSpace($DesignName) -and
        -not [string]::IsNullOrWhiteSpace($StepName) -and
        -not [string]::IsNullOrWhiteSpace($LayerName)) {

        $symbolsArgs = @(
            "-plaintext",
            "-import-path", $GrpcProtoPath,
            "-import-path", $ModelProtoPath,
            "-proto", $ServiceProtoPath,
            "-d", "{`"design_name`":`"$DesignName`",`"step_name`":`"$StepName`",`"layer_name`":`"$LayerName`"}",
            $Target,
            "Odb.Grpc.OdbDesignService/GetLayerSymbols"
        )

        $symbolsOutput = & $GrpcUrlPath @symbolsArgs 2>&1
        if ($LASTEXITCODE -ne 0) {
            Fail "grpcurl GetLayerSymbols failed for $Target.`n$symbolsOutput"
        }

        Write-Step "grpcurl GetLayerSymbols succeeded for $Target."
    }
}

Require-Command "kubectl"

$currentContext = (kubectl config current-context 2>$null)
if ($LASTEXITCODE -ne 0) {
    Fail "Failed to read current kubectl context."
}
$currentContext = "$currentContext".Trim()

# cluster kind is resolved below, AFTER any -ClusterName context switch:
# detecting it here would use the pre-switch context and run the wrong
# validation path against the switched cluster.

if ([string]::IsNullOrWhiteSpace($ClusterName)) {
    $ClusterName = $currentContext
}
else {
    Write-Step "Switching kubectl context to $ClusterName..."
    kubectl config use-context $ClusterName | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Fail "Failed to switch kubectl context to $ClusterName."
    }
}

# resolve cluster kind from the effective context (post-switch)
$kind = $ClusterKind
if ($kind -eq 'Auto') {
    if ($ClusterName.StartsWith("k3d-")) { $kind = 'k3d' } else { $kind = 'k3s' }
}
Write-Step "Cluster kind: $kind (context: $ClusterName)"

if ($kind -eq 'k3s' -and [string]::IsNullOrWhiteSpace($AdvertisedHost)) {
    # resolve after service inspection below
}
elseif ([string]::IsNullOrWhiteSpace($AdvertisedHost)) {
    $AdvertisedHost = "precision5820"
}

$deployment = Invoke-KubectlJson -Arguments @("get", "deployment", $DeploymentName)
$deploymentPorts = @(
    $deployment.spec.template.spec.containers |
    ForEach-Object { @($_.ports) } |
    Where-Object { $_.name -eq $GrpcPortName -and [int]$_.containerPort -eq $GrpcPort }
)

if ($deploymentPorts.Count -eq 0) {
    Fail "Deployment '$DeploymentName' is not exposing named port '$GrpcPortName' on container port $GrpcPort."
}

Write-Step "Deployment '$DeploymentName' exposes '$GrpcPortName' on port $GrpcPort."

$service = Invoke-KubectlJson -Arguments @("get", "service", $GrpcServiceName)
if ($service.spec.type -ne "LoadBalancer") {
    Fail "Service '$GrpcServiceName' must be type LoadBalancer but was '$($service.spec.type)'."
}

$servicePorts = @(
    @($service.spec.ports) |
    Where-Object {
        $_.name -eq $GrpcPortName -and
        [int]$_.port -eq $GrpcPort -and
        ($_.targetPort -eq $GrpcPortName -or [int]$_.targetPort -eq $GrpcPort)
    }
)

if ($servicePorts.Count -eq 0) {
    Fail "Service '$GrpcServiceName' is not mapping port $GrpcPort to targetPort '$GrpcPortName'."
}

Write-Step "Service '$GrpcServiceName' maps port $GrpcPort to target '$GrpcPortName'."

$endpoints = Invoke-KubectlJson -Arguments @("get", "endpoints", $GrpcServiceName)
$endpointAddresses = @(
    foreach ($subset in @($endpoints.subsets)) {
        foreach ($address in @($subset.addresses)) {
            $address.ip
        }
    }
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

if ($endpointAddresses.Count -eq 0) {
    Fail "Service '$GrpcServiceName' has no endpoints. The pod may not be ready or the selector may not match."
}

Write-Step "Service '$GrpcServiceName' has endpoints: $($endpointAddresses -join ', ')"

#
# LoadBalancer exposure check (cluster-kind specific)
#

$grpcTarget = $AdvertisedHost

if ($kind -eq 'k3d') {
    Require-Command "docker"

    $clusterShortName = Normalize-ClusterShortName -Name $ClusterName
    $loadBalancerContainer = "k3d-$clusterShortName-serverlb"
    $publishedPort = & docker port $loadBalancerContainer "$GrpcPort/tcp" 2>$null
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($publishedPort)) {
        Fail "Load balancer container '$loadBalancerContainer' is not publishing TCP/$GrpcPort. If this cluster predates the gRPC exposure change, recreate it with scripts/create-k3d-cluster.ps1 so it includes --port ""${GrpcPort}:${GrpcPort}@loadbalancer""."
    }

    if ($publishedPort -notmatch "[:]{1}$GrpcPort\b") {
        Fail "Load balancer '$loadBalancerContainer' is publishing TCP/$GrpcPort as '$publishedPort' instead of host port $GrpcPort. Update the cluster creation port mapping or recreate the cluster."
    }

    Write-Step "Load balancer '$loadBalancerContainer' publishes TCP/$GrpcPort as: $publishedPort"
}
else {
    # k3s: ServiceLB (klipper-lb) publishes the LoadBalancer service on the node.
    # Re-read the service to pick up its assigned ingress IP.
    $service = Invoke-KubectlJson -Arguments @("get", "service", $GrpcServiceName)

    # StrictMode-safe lookup: kubectl's JSON omits the empty "ingress" key
    # while the LoadBalancer IP is pending (status.loadBalancer: {}), which
    # would otherwise throw PropertyNotFoundException and hide the guard below.
    $ingressIps = @()
    $statusProp = $service.PSObject.Properties['status']
    if ($statusProp) {
        $lbProp = $statusProp.Value.PSObject.Properties['loadBalancer']
        if ($lbProp) {
            $ingressProp = $lbProp.Value.PSObject.Properties['ingress']
            if ($ingressProp) {
                $ingressIps = @(
                    foreach ($entry in @($ingressProp.Value)) {
                        # entries may omit "ip" (hostname-only) — StrictMode-safe lookup
                        $ipProp = $entry.PSObject.Properties['ip']
                        $hostProp = $entry.PSObject.Properties['hostname']
                        if ($ipProp -and -not [string]::IsNullOrWhiteSpace("$($ipProp.Value)")) { "$($ipProp.Value)" }
                        elseif ($hostProp -and -not [string]::IsNullOrWhiteSpace("$($hostProp.Value)")) { "$($hostProp.Value)" }
                    }
                ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            }
        }
    }

    if ($ingressIps.Count -eq 0) {
        Fail "ServiceLB has not assigned an ingress IP to service '$GrpcServiceName' yet. Check the svclb pod: kubectl -n kube-system get pods -l svccontroller.k3s.cattle.io/svcname=$GrpcServiceName"
    }

    Write-Step "ServiceLB publishes service '$GrpcServiceName' on: $($ingressIps -join ', ')"

    if ([string]::IsNullOrWhiteSpace($AdvertisedHost)) {
        $grpcTarget = $ingressIps[0]
    }
}

if ($SkipGrpcUrl) {
    Write-Step "Skipping grpcurl checks."
    exit 0
}

$grpcurlCommand = Get-Command "grpcurl" -ErrorAction SilentlyContinue
if ($null -eq $grpcurlCommand) {
    $grpcurlCommand = Get-Command "grpcurl.exe" -ErrorAction SilentlyContinue
}

if ($null -eq $grpcurlCommand) {
    Write-Warning "[validate-grpc] grpcurl was not found in PATH. Skipping RPC validation."
    exit 0
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$grpcProtoPath = Join-Path $repoRoot "OdbDesignServer/protoc/grpc"
$modelProtoPath = Join-Path $repoRoot "OdbDesignLib/protoc"
$serviceProtoPath = Join-Path $grpcProtoPath "service.proto"
$grpcurlPath = $grpcurlCommand.Source

$computerName = $env:COMPUTERNAME
if (-not [string]::IsNullOrWhiteSpace($computerName) -and
    -not [string]::IsNullOrWhiteSpace($AdvertisedHost) -and
    $computerName.Equals($AdvertisedHost, [System.StringComparison]::OrdinalIgnoreCase)) {
    Test-GrpcUrlTarget `
        -Target "localhost:$GrpcPort" `
        -GrpcUrlPath $grpcurlPath `
        -ServiceProtoPath $serviceProtoPath `
        -GrpcProtoPath $grpcProtoPath `
        -ModelProtoPath $modelProtoPath `
        -DesignName $DesignName `
        -StepName $StepName `
        -LayerName $LayerName
}
else {
    Write-Step "Skipping localhost grpcurl validation (machine is '$computerName', advertised host is '$AdvertisedHost')."
}

Test-GrpcUrlTarget `
    -Target "${grpcTarget}:$GrpcPort" `
    -GrpcUrlPath $grpcurlPath `
    -ServiceProtoPath $serviceProtoPath `
    -GrpcProtoPath $grpcProtoPath `
    -ModelProtoPath $modelProtoPath `
    -DesignName $DesignName `
    -StepName $StepName `
    -LayerName $LayerName

Write-Step "gRPC exposure validation completed successfully."
