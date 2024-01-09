
$clusterName = "k3d-k3dcluster"
$deploymentName = "odbdesign-server-v1"
#$serviceName = "odbdesign-server-service"
#$ingressName = "odbdesign-server-ingress"
#$image = "ghcr.io/nam20485/odbdesign:nam20485-latest"

# set kubeconfig

kubectl config use-context $clusterName
if ($LASTEXITCODE -ne 0) {
    Exit 1    
}

#kubectl get deployment $deploymentName
#if ($LASTEXITCODE -ne 0) {
    kubectl apply -f deploy/kube/OdbDesignServer/deployment.yaml
#}

#kubectl get svc $serviceName
#if ($LASTEXITCODE -ne 0) {
    kubectl apply -f deploy/kube/OdbDesignServer/service.yaml
#}

#kubectl get ingress$ingressName
#if ($LASTEXITCODE -ne 0) {
    kubectl apply -f deploy/kube/OdbDesignServer/ingress.yaml
#}

# kubectl set image deployment/$deploymentName odbdesign-server=$image

# initiate rolling update
kubectl rollout restart deployment/$deploymentName

# wait for kubectl deployment to finish
kubectl rollout status deployment/$deploymentName
