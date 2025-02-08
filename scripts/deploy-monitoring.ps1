kubectl create namespace monitoring

helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add aqua https://aquasecurity.github.io/helm-charts/
helm repo update

helm upgrade --install prom prometheus-community/kube-prometheus-stack -n monitoring --values ./deploy/helm/values-prom.yaml
kubectl --namespace monitoring get pods -l "release=prom"

#kubectl delete -f https://raw.githubusercontent.com/aquasecurity/trivy-operator/v0.1.5/deploy/static/trivy-operator.yaml
helm install trivy-operator aqua/trivy-operator --namespace trivy-system --create-namespace --version 0.11.0 --values trivy-values.yaml
