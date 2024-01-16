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
    [switch]$ForceDelete = $false
)

# $clusterName="k3dcluster"
$hostIp = "10.0.0.185"
# $numAgents=3
# $ingressHostPort=8081

if ($DeleteClusterFirst) {

    if (! $ForceDelete) {`
        $clusterNameInput = Read-Host "Type cluster name ($ClusterName) and hit ENTER to delete the cluster first..."
    }
    else {
        Write-Host "Force delete specified..."
        $clusterNameInput = $ClusterName
    }
    
    if ($clusterNameInput -ne $ClusterName) {
        Write-Host "Cluster name did not match. Exiting..."
        Exit 1
    }
    else {
        Write-Host "Deleting cluster '$ClusterName'..."
        k3d cluster delete $ClusterName
        Write-Host "Cluster '$ClusterName' deleted."
    }
}

Write-Host "Creating cluster '$ClusterName'..."

k3d cluster create $ClusterName `
    --agents $NumAgents `
    --k3s-arg="--tls-san=${hostIp}@server:0" `
    --k3s-arg="--tls-san=precision5820@server:0" `
    -p "${IngressHostPort}:80@loadbalancer"

Write-Host "Cluster '$ClusterName' created."
