param(
    [string]$ClusterName = "k3dcluster",
    [int]$DockerReadyTimeoutSeconds = 300,
    [int]$DockerPollIntervalSeconds = 5
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Wait-ForDocker {
    param(
        [Parameter(Mandatory=$true)]
        [int]$TimeoutSeconds,
        [Parameter(Mandatory=$true)]
        [int]$PollIntervalSeconds
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        try {
            docker info *> $null
            if ($LASTEXITCODE -eq 0) {
                return
            }
        }
        catch {
        }

        Start-Sleep -Seconds $PollIntervalSeconds
    }

    throw "Docker did not become ready within $TimeoutSeconds seconds."
}

function Get-ClusterContainerNames {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterName,
        [switch]$RunningOnly
    )

    $dockerArgs = @("ps", "--format", "{{.Names}}")
    if (-not $RunningOnly) {
        $dockerArgs = @("ps", "-a", "--format", "{{.Names}}")
    }

    $names = docker @dockerArgs
    if ($LASTEXITCODE -ne 0 -or $null -eq $names) {
        return @()
    }

    return @($names | Where-Object { $_ -like "k3d-$ClusterName-*" })
}

Write-Host "Waiting for Docker to become ready..."
Wait-ForDocker -TimeoutSeconds $DockerReadyTimeoutSeconds -PollIntervalSeconds $DockerPollIntervalSeconds

$allClusterContainers = Get-ClusterContainerNames -ClusterName $ClusterName
if ($allClusterContainers.Count -eq 0) {
    throw "Could not find any containers for cluster '$ClusterName'."
}

$runningClusterContainers = Get-ClusterContainerNames -ClusterName $ClusterName -RunningOnly
if ($runningClusterContainers.Count -eq $allClusterContainers.Count) {
    Write-Host "Cluster '$ClusterName' is already running."
    exit 0
}

Write-Host "Starting cluster '$ClusterName'..."
k3d cluster start $ClusterName

if ($LASTEXITCODE -ne 0) {
    throw "Failed to start cluster '$ClusterName'."
}

Write-Host "Cluster '$ClusterName' started."
