# Grafana, Prometheus, and Trivy (how to open and verify)

This repo deploys **kube-prometheus-stack** (Prometheus + Grafana) into the `monitoring` namespace and the **Trivy Operator** into `trivy-system`. Monitoring is wired to the same Traefik HTTP entrypoint as the app (k3s default ports **80/443** on the Debian 13 VM, host-less ingresses).

## 1. Deploy or refresh the stack

From the repository root:

```powershell
.\scripts\deploy-monitoring.ps1 -Wait
```

- **`-Wait`** waits for Deployments, StatefulSets, and DaemonSets in `monitoring` and `trivy-system` to finish rolling out (default timeout **600** seconds). Omit it if you only want Helm to apply and exit.
- The ingresses are host-less, so any hostname/IP that reaches Traefik works. If you serve under a different external URL, edit `deploy/helm/values-prom.yaml` first: set `grafana.grafana.ini.server.root_url` (and optionally `prometheus.prometheusSpec.externalUrl`) to match, then rerun the script.

Ensure Traefik is running and your ingress exists (for the app you use `deploy/kube/local-ingress.yaml`). Grafana and Prometheus get **their own Ingress objects** in `monitoring` from Helm; they share the same Traefik entrypoint (80/443).

**Prometheus path vs in-cluster clients:** Prometheus must answer the HTTP API at **`http://<prometheus-service>:9090/api/v1/...`** (path `/`, not `/prometheus/...`). Tools such as **Freelens/Lens** use that URL inside the cluster. The deploy script applies **`deploy/kube/traefik-middleware-prometheus-stripprefix.yaml`**, which strips the **`/prometheus`** prefix on Traefik so the browser can still use `http://192.168.122.200/prometheus/` without breaking in-cluster queries. If the Middleware fails to apply (wrong Traefik API version), see the comments in that YAML file.

## 2. URLs (browser)

| What | URL |
|------|-----|
| **Grafana** | `http://192.168.122.200/grafana/` |
| **Prometheus** | `http://192.168.122.200/prometheus/` |
| **OdbDesign** (same Traefik) | `http://192.168.122.200/` |

Use the address that actually resolves from your machine — the VM LAN IP `192.168.122.200` (libvirt NAT) or the Tailscale IP `100.118.225.119` (for example `http://100.118.225.119/grafana/`). Paths are **prefix** routes: Grafana is under `/grafana`, Prometheus under `/prometheus`, so they do not clash with `/` or `/swagger`.

If a page returns 404, check:

1. `kubectl get ingress -n monitoring` — you should see the Grafana and Prometheus ingresses (host-less).
2. `kubectl get pods -n kube-system` (or wherever Traefik runs) — Traefik must be Ready.
3. The k3s Traefik ports **80/443** are reachable from the client you are browsing from (libvirt NAT or tailnet).

## 3. Grafana (web UI)

### 3.1 Log in

1. Open `http://192.168.122.200/grafana/` (or your equivalent).
2. User is **`admin`** unless you changed `grafana.adminUser` in Helm values.
3. Password comes from the Grafana secret (release name `prom` → secret name **`prom-grafana`**):

```powershell
kubectl get secret prom-grafana -n monitoring -o jsonpath='{.data.admin-password}' | ForEach-Object { [System.Text.Encoding]::UTF8.GetString([System.Convert]::FromBase64String($_)) }
```

Copy the printed string and paste it into the Grafana password field.

### 3.2 Confirm Prometheus data source

1. After login, go to **Connections → Data sources** (or **Configuration → Data sources** in older layouts).
2. Open the **Prometheus** data source. URL should point at the in-cluster Prometheus service (the chart provisions this). Click **Save & test** — you want a green success message.

### 3.3 Open a dashboard

1. Go to **Dashboards** and open one of the preloaded Kubernetes / Node dashboards (for example **Kubernetes / Compute Resources / Cluster**).
2. Set time range to **Last 15 minutes** and confirm panels show data. If everything is empty, see §5 (Prometheus targets).

### 3.4 Optional: Explore (PromQL)

1. Open **Explore** (compass icon).
2. Choose data source **Prometheus**.
3. Run a simple query, e.g. `up` or `node_cpu_seconds_total`. You should see a graph or table if scraping works.

## 4. Prometheus (web UI)

### 4.1 Graph

1. Open `http://192.168.122.200/prometheus/`.
2. Open the **Graph** tab.
3. Enter `up` and click **Execute**. You should see time series with value `1` for healthy scrape targets.

### 4.2 Targets (most important health check)

1. In Prometheus, open **Status → Targets**.
2. You should see many jobs (e.g. `node-exporter`, `kubelet`, `kubernetes-pods`, `kube-prometheus-stack`). **State** should be **UP** for the ones that apply to your cluster.
3. If the Trivy Operator ServiceMonitor is enabled (`deploy/helm/values-trivy.yaml`), look for a target related to **trivy** (name may vary by version). If it is **DOWN**, check the pod in `trivy-system` and ServiceMonitor labels.

### 4.3 Alerts

Open **Alerts** to see firing / pending Prometheus alerts from the stack.

### 4.4 Fallback: port-forward (no Traefik)

If you need to bypass the ingress:

```powershell
kubectl port-forward -n monitoring svc/prom-kube-prometheus-stack-prometheus 9090:9090
```

Then open `http://127.0.0.1:9090` (or `http://127.0.0.1:9090/prometheus/graph` if your Prometheus instance uses a non-root `routePrefix`). List services with `kubectl get svc -n monitoring` if your release name is not `prom`.

## 5. Trivy (what to open — mostly CRs and metrics)

The **Trivy Operator** does **not** ship a full “scanner web app” like a standalone Trivy UI. You work with **Kubernetes APIs and reports**, and optionally **metrics** in Prometheus / Grafana.

### 5.1 Check the operator is running

```powershell
kubectl get pods -n trivy-system
kubectl get deployment -n trivy-system
```

All pods should be **Running** / **Ready**.

### 5.2 View vulnerability reports (CLI)

After workloads are scanned, reports appear as CRs:

```powershell
kubectl get vulnerabilityreports -A
kubectl get vulnerabilityreports -A -o wide
```

Pick one:

```powershell
kubectl describe vulnerabilityreport <name> -n <namespace>
```

For YAML detail (CVE lists, severity):

```powershell
kubectl get vulnerabilityreport <name> -n <namespace> -o yaml
```

Other report types (depending on operator version and config) may include **ConfigAuditReport**, **ExposedSecretReport**, **RbacAssessmentReport** — use `kubectl api-resources | grep trivy` to see names in your cluster.

### 5.3 See Trivy in Prometheus / Grafana

With `serviceMonitor.enabled: true` in `values-trivy.yaml`, the operator exposes metrics that Prometheus can scrape.

1. In Prometheus (**Status → Targets**), find the scrape job for Trivy.
2. In Grafana **Explore**, try metrics with prefix like `trivy_operator_` (exact names vary by version). Use **Metrics browser** or Prometheus **Graph** with autocomplete.

### 5.4 Optional: metrics port-forward

If you need raw Prometheus metrics from the operator:

```powershell
kubectl get pods -n trivy-system
kubectl port-forward -n trivy-system pod/<trivy-operator-pod-name> 8080:8080
```

Then open `http://127.0.0.1:8080/metrics` in a browser (port may differ; check the pod spec for the metrics port). This is **text**, not a dashboard.

## 6. Quick checklist: “everything works”

- [ ] `http://192.168.122.200/grafana/` loads and login works.
- [ ] Grafana **Prometheus** data source **Save & test** succeeds.
- [ ] Prometheus **Status → Targets** shows most targets **UP**.
- [ ] `kubectl get vulnerabilityreports -A` returns (possibly empty until scans run; operator Ready is what matters first).

If Grafana loads but assets look broken, confirm you are using the **subpath** URL (`/grafana/`) and that `serve_from_sub_path` is set in `deploy/helm/values-prom.yaml` as in this repo.
