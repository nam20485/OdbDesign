param(
    [Parameter(Mandatory=$false)]
    [string]$ClusterName = "k3d-k3dcluster",
    [Parameter(Mandatory=$false)]
    [string]$DeploymentName = "odbdesign-server-v1",
    [Parameter(Mandatory=$false)]
    [string]$GrpcServiceName = "odbdesign-server-grpc-service",
    [Parameter(Mandatory=$false)]
    [string]$GrpcPortName = "ods-grpc-port",
    [Parameter(Mandatory=$false)]
    [int]$GrpcPort = 50051,
    [Parameter(Mandatory=$false)]
    [string]$AdvertisedHost = "precision5820",
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

    if ($Name.StartsWith("k3d-")) {
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
Require-Command "docker"

Write-Step "Switching kubectl context to $ClusterName..."
kubectl config use-context $ClusterName | Out-Null
if ($LASTEXITCODE -ne 0) {
    Fail "Failed to switch kubectl context to $ClusterName."
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

Write-Step "Service '$GrpcServiceName' maps host port $GrpcPort to target '$GrpcPortName'."

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

$clusterShortName = Normalize-ClusterShortName -Name $ClusterName
$loadBalancerContainer = "k3d-$clusterShortName-serverlb"
$publishedPort = & docker port $loadBalancerContainer "$GrpcPort/tcp" 2>$null
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($publishedPort)) {
    Fail "Load balancer container '$loadBalancerContainer' is not publishing TCP/$GrpcPort. If this cluster predates the gRPC exposure change, recreate it with scripts/create-k3d-cluster.ps1 so it includes --port ""$GrpcPort:$GrpcPort@loadbalancer""."
}

if ($publishedPort -notmatch "[:]{1}$GrpcPort\b") {
    Fail "Load balancer '$loadBalancerContainer' is publishing TCP/$GrpcPort as '$publishedPort' instead of host port $GrpcPort. Update the cluster creation port mapping or recreate the cluster."
}

Write-Step "Load balancer '$loadBalancerContainer' publishes TCP/$GrpcPort as: $publishedPort"

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
    Write-Step "Skipping localhost grpcurl validation because this machine is '$computerName', not '$AdvertisedHost'."
}

Test-GrpcUrlTarget `
    -Target "$AdvertisedHost:$GrpcPort" `
    -GrpcUrlPath $grpcurlPath `
    -ServiceProtoPath $serviceProtoPath `
    -GrpcProtoPath $grpcProtoPath `
    -ModelProtoPath $modelProtoPath `
    -DesignName $DesignName `
    -StepName $StepName `
    -LayerName $LayerName

Write-Step "gRPC exposure validation completed successfully."
