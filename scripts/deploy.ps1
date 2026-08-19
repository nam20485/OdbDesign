param(
    # Cluster name
    [Parameter(Mandatory=$false)]    
    [string]$ClusterName = "k3d-k3dcluster",
    # Deployment name
    [Parameter(Mandatory=$false)]
    [string]$DeploymentName = "odbdesign-server-v1",
    # Skip post-deploy gRPC validation
    [switch]$SkipGrpcValidation = $false
)

# set kubeconfig
kubectl config use-context $ClusterName
if ($LASTEXITCODE -ne 0) {
    Exit 1    
}

#
# Common (pre)
#

# persistent volume
kubectl apply -f deploy/kube/k3d-volume-pv.yaml
kubectl apply -f deploy/kube/k3d-volume-pvc.yaml

#
# OdbDesignServer
#

# secrets
& (Join-Path $PSScriptRoot 'odbdesign-server-request-secret.ps1')

# apply deployment/service manifests
kubectl apply -f deploy/kube/OdbDesignServer/deployment.yaml
kubectl apply -f deploy/kube/OdbDesignServer/service.yaml
kubectl apply -f deploy/kube/OdbDesignServer/service-grpc.yaml

# restart deployment
kubectl rollout restart deployment/$DeploymentName
kubectl rollout status deployment/$DeploymentName


#
# Swagger UI
#

# apply deployment/service manifests
kubectl apply -f deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml
kubectl apply -f deploy/kube/OdbDesignServer-SwaggerUI/service.yaml

# restart deployment
kubectl rollout restart deployment/odbdesign-server-swaggerui-v1
kubectl rollout status deployment/odbdesign-server-swaggerui-v1

#
# Common (post)
#

# apply ingress manifest
kubectl apply -f deploy/kube/local-ingress.yaml

if (-not $SkipGrpcValidation) {
    & (Join-Path $PSScriptRoot 'validate-grpc-exposure.ps1') `
        -ClusterName $ClusterName `
        -DeploymentName $DeploymentName
}
