
$clusterName = "k3d-k3dcluster"
$deploymentName = "odbdesign-server"
$image = "ghcr.io/nam20485/odbdesign:nam20485-latest"

# set kubeconfig

kubectl config use-context $clusterName

# kubectl apply -f deploy/kube/OdbDesignServer/deployment.yaml
# kubectl apply -f deploy/kube/OdbDesignServer/service.yaml
# kubectl apply -f deploy/kube/OdbDesignServer/ingress.yaml

kubectl set image deployment/$deploymentName odbdesign-server=$image

# wait for kubectl deployment to finish
kubectl rollout status deployment/$deploymentName
