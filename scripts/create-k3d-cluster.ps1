param(
    # Cluster name
    [Parameter(Mandatory=$true)]
    [string]$ClusterName = "k3dcluster",
    # Number of agents to create
    [Parameter(Mandatory=$true)]
    [int]$NumAgents = 3,
    # Ingress host port
    [Parameter(Mandatory=$true)]
    [int]$IngressHostPort = 8081,
    # When set to true, the cluster will be deleted first
    [switch]$DeleteClusterFirst = $false,
    # When set to true, the cluster will be deleted without asking for confirmation
    [switch]$ForceDelete = $false,
    # Host Volume Path for PersistentVolume
    [Parameter(Mandatory=$true)]
    [string]$HostVolumePath = "D:/k3dvolume",
    # When set to true, register a scheduled task to start the cluster at Windows boot
    [switch]$StartWithWindows = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# $hostIp = "10.0.0.185"
$hostIp = "192.168.1.30"
# $hostIp = "192.168.1.101"
$ingressHostName = "precision5820"
$firewallRuleDisplayName = "k3d $ClusterName ingress $IngressHostPort"

function Ensure-IngressFirewallRule {
    param(
        [Parameter(Mandatory=$true)]
        [string]$DisplayName,
        [Parameter(Mandatory=$true)]
        [int]$Port
    )

    $existingRule = Get-NetFirewallRule -DisplayName $DisplayName -ErrorAction SilentlyContinue
    if ($null -eq $existingRule) {
        Write-Host "Creating firewall rule '$DisplayName' for TCP/$Port..."
        New-NetFirewallRule `
            -DisplayName $DisplayName `
            -Direction Inbound `
            -Action Allow `
            -Protocol TCP `
            -LocalPort $Port `
            -Profile Any | Out-Null
        return
    }

    Write-Host "Firewall rule '$DisplayName' already exists. Ensuring it is enabled..."
    Enable-NetFirewallRule -DisplayName $DisplayName | Out-Null
}

if ($DeleteClusterFirst) {
    if (-not $ForceDelete) {
        $clusterNameInput = Read-Host "Type cluster name ($ClusterName) and hit ENTER to delete the cluster first..."
    }
    else {
        Write-Host "Force delete specified..."
        $clusterNameInput = $ClusterName
    }

    if ($clusterNameInput -ne $ClusterName) {
        Write-Host "Cluster name did not match. Exiting..."
        exit 1
    }

    Write-Host "Deleting cluster '$ClusterName'..."
    k3d cluster delete $ClusterName
    Write-Host "Cluster '$ClusterName' deleted."
}

Write-Host "Creating cluster '$ClusterName'..."

k3d cluster create $ClusterName `
    --agents $NumAgents `
    --k3s-arg="--tls-san=${hostIp}@server:0" `
    --k3s-arg="--tls-san=${ingressHostName}@server:0" `
    --port "${IngressHostPort}:80@loadbalancer" `
    --port "8443:443@loadbalancer" `
    --volume ${HostVolumePath}:/k3dvolume@all
    # --volume ${HostVolumePath}:/tmp/k3dvolume@all

Write-Host "Cluster '$ClusterName' created."

Ensure-IngressFirewallRule -DisplayName $firewallRuleDisplayName -Port $IngressHostPort

if ($StartWithWindows) {
    $registerScriptPath = Join-Path $PSScriptRoot "register-k3d-startup-task.ps1"
    Write-Host "Registering Windows startup task for cluster '$ClusterName'..."
    & $registerScriptPath -ClusterName $ClusterName
}
