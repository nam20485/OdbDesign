# When set to true, the cluster will be deleted first
# [Parameter()]
# [Boolean]$DeleteClusterFirst = $false

param(
    [switch]$DeleteClusterFirst = $false
)

$clusterName="k3dcluster"
$hostIp="10.0.0.185"
$numAgents=3
$ingressHostPort=8081

if ($DeleteClusterFirst) {

    $delete = Read-Host "Type cluster name and hit ENTER to delete the cluster first ($clusterName)..."
    if ($delete -ne $clusterName) {
        Write-Host "Cluster name did not match. Exiting..."
        Exit
    }
    else
    {
        k3d cluster delete $clusterName
    }
}

k3d cluster create $clusterName --agents $numAgents --k3s-arg="--tls-san=$hostIp@server:0" -p "${ingressHostPort}:80@loadbalancer"

