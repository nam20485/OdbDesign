---
name: k3s-admin
description: Administer the k3s cluster in the Debian 13 VM — install/start/stop/status/uninstall, deploy workloads, kubectl access, ports, logs, troubleshooting.
---

# k3s cluster administration (Debian 13 VM)

## Where the cluster lives

- Single-node **k3s** cluster in the Debian 13 VM (`debian13vm`), managed by the systemd unit `k3s`.
- VM LAN IP: `192.168.122.200` (libvirt NAT). Tailscale IP: `100.118.225.119`.
- No Docker/k3d layer — k3s runs directly on the VM.

## Lifecycle

Run inside the VM from the repo root (`scripts/k3s-cluster.ps1`):

```powershell
pwsh scripts/k3s-cluster.ps1 -Action <Install|Start|Stop|Restart|Status|Uninstall>
```

- `-Action Status` is the default and is safe when k3s is stopped or not installed.
- `-Action Install` options: `-TlsSans` (default `192.168.122.200`,`100.118.225.119`), `-K3sVersion <pin>` (sets `INSTALL_K3S_VERSION`), `-Force` (re-run installer over an existing install), `-ReadyTimeoutSeconds` (default 300; applies to Install/Start/Restart).
- `-Action Uninstall` asks for confirmation (type `uninstall`); `-Force` skips it.
- Mutating actions cache sudo credentials once up front (`sudo -v`); the sudo timeout is short when running unattended.

## kubectl access

- User kubeconfig: `~/.kube/config` (mode 600, created by Install; server URL rewritten to `https://192.168.122.200:6443`).
- Root kubeconfig: `/etc/rancher/k3s/k3s.yaml` (mode 600).
- Kubernetes API reachable at `https://192.168.122.200:6443` and `https://100.118.225.119:6443` (both are tls-san entries).
- **Gotcha:** `/usr/local/bin/kubectl` is a symlink to k3s, and the k3s wrapper forces `KUBECONFIG=/etc/rancher/k3s/k3s.yaml` unless `KUBECONFIG` is already set — as a normal user that file is unreadable (mode 600 root). Use `KUBECONFIG=~/.kube/config kubectl ...` (or export `KUBECONFIG` in your shell profile), or run `sudo k3s kubectl`. `scripts/k3s-cluster.ps1` pins `KUBECONFIG` itself.

## Port / access map

| Service | Address | Notes |
|---|---|---|
| Kubernetes API | `https://192.168.122.200:6443`, `https://100.118.225.119:6443` | tls-san includes both IPs |
| Ingress (Traefik) | `http://192.168.122.200/`, `http://100.118.225.119/`, `https://...:443` | k3s defaults; `local-ingress.yaml` has no host filter |
| gRPC (OdbDesignServer) | `192.168.122.200:50051`, `100.118.225.119:50051` | ServiceLB binds the node port after deploy |
| Swagger UI | `http://192.168.122.200/swagger/` | serves ConfigMap-mounted spec; `tryItOutEnabled` |
| Prometheus | `http://192.168.122.200/prometheus` | StripPrefix middleware → API at `/` |
| Grafana | `http://192.168.122.200/grafana` | admin password from secret (below) |
| kubeconfig | `/etc/rancher/k3s/k3s.yaml` (root), `~/.kube/config` (user) | |

## Deploying OdbDesign

```powershell
pwsh scripts/deploy.ps1 -ClusterName default -SkipGrpcValidation
```

- `-ClusterName default` selects the k3s kubeconfig context (the k3d-specific `k3d-k3dcluster` context does not exist here).
- `-SkipGrpcValidation` is required: `scripts/validate-grpc-exposure.ps1` is k3d/Docker-specific (it inspects the `k3d-*-serverlb` container via `docker port`).
- Validate gRPC manually afterwards: `grpcurl -plaintext 192.168.122.200:50051 list` (and/or the tailnet IP `100.118.225.119`).

## Deploying monitoring

```powershell
pwsh scripts/deploy-monitoring.ps1 -Wait -WaitTimeoutSeconds 900
```

- Deploys helm release `prom` (kube-prometheus-stack) into `monitoring` and `trivy-operator` into `trivy-system`. Chart versions are pinned in the script; update them deliberately via `helm search repo`.
- Prerequisites auto-install: on Linux the script installs helm to `~/.local/bin` (no sudo, tarball from get.helm.sh); on Windows via winget/choco/scoop. It also pins `KUBECONFIG` to `~/.kube/config` (k3s kubectl-symlink gotcha) when unset.
- Prometheus: `http://192.168.122.200/prometheus` (Traefik StripPrefix middleware `monitoring/prometheus-stripprefix` keeps the API at `/` — `traefik.io/v1alpha1`, Traefik 3.x).
- Grafana: `http://192.168.122.200/grafana` — get the admin password with:
  `kubectl get secret prom-grafana -n monitoring -o jsonpath='{.data.admin-password}' | base64 -d`
- Verify: `curl http://192.168.122.200/prometheus/-/healthy` and `curl http://192.168.122.200/grafana/api/health`; pods via `kubectl get pods -n monitoring` and `kubectl get pods -n trivy-system`.
- Ordering matters: trivy's `serviceMonitor.enabled=true` needs the prometheus-operator CRDs, so prom is installed first (script already does this).
- Node headroom: the stack adds ~1.5–2.5 GiB RSS and trivy scans spike CPU; check with `kubectl top node`.

## Generating the OpenAPI spec (swaggerui)

Regenerate `swagger/odbdesign-server-0.9-swagger.yaml` when the REST API changes. The filename is FIXED — the swaggerui image's initializer loads `./odbdesign-server-0.9-swagger.yaml`, mounted from the `odbdesign-server-swagger-spec` ConfigMap.

Workflow:

1. **Enumerate routes**: `grep -n "CROW_ROUTE" OdbDesignServer/Controllers/*.cpp` — skip commented-out registrations. Each route's lambda shows the auth wrapper (`AuthenticateRequest` → requires BasicAuth → 401) and the handler called.
2. **Handler semantics**: read the `*_route_handler` implementations in the same files for status codes (400 empty name, 404 not-found with plain-text body, 500 file errors) and response shapes. List endpoints return `{"filearchives": [...]}` / `{"steps": [...]}` etc. (see `OdbDesignLib/App/RouteController.cpp` `makeLoadedFileModelsResponse`: 200, 201 after upload, 204 when empty). Health/hello routes are unauthenticated (`security: []`).
3. **Schemas**: map returned objects to protobuf messages in `OdbDesignLib/protoc/*.proto`. JSON uses default proto3 JSON options (`OdbDesignLib/IProtoBuffable.h`): lowerCamelCase keys, enums as names, unset/default fields omitted, Timestamps as RFC 3339. Handlers that build JSON directly (e.g. `diagnostics/symbol_units`) use snake_case keys — derive from the handler code, not the protos.
4. **Write the spec**: OpenAPI 3.0.3, keep `info.version` in sync with the server version, reuse `components/parameters` + `components/responses` ($refs), describe every operation (operationId/summary/description), enum every proto enum, and add response examples. Query params: e.g. `include_filearchive` on `GET /designs/{name}`.
5. **Validate**: `python3 -c "import yaml; yaml.safe_load(open('swagger/odbdesign-server-0.9-swagger.yaml'))"` plus a `$ref` resolution check (there is a check script pattern in the repo history; a quick re-parse + spot-check suffices).
6. **Ship to the cluster** (subPath mounts don't hot-reload):

```bash
kubectl create configmap odbdesign-server-swagger-spec \
  --from-file=odbdesign-server-0.9-swagger.yaml=swagger/odbdesign-server-0.9-swagger.yaml \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl rollout restart deployment/odbdesign-server-swaggerui-v1
kubectl rollout status deployment/odbdesign-server-swaggerui-v1
```

7. **Verify**: `curl -s http://192.168.122.200/swagger/odbdesign-server-0.9-swagger.yaml | head` shows the new content; UI at `http://192.168.122.200/swagger/` renders it (Try-it-out needs the server's BasicAuth credentials from the `odbdesign-server-request-secret` secret).

## Logs and diagnostics

```bash
sudo journalctl -u k3s -f
kubectl logs -n kube-system -l app=traefik
kubectl get events -A --sort-by=.lastTimestamp
```

## Common operations

- Restart the server deployment: `kubectl rollout restart deployment/odbdesign-server-v1 && kubectl rollout status deployment/odbdesign-server-v1`
- Check ServiceLB / gRPC exposure: `kubectl get svc -A` (look for `klipper-lb` and the LoadBalancer on 50051).
- Node IPs: `kubectl get nodes -o wide`.
- Ingress routing check: `curl -s -o /dev/null -w '%{http_code}' http://192.168.122.200/` should return 200.

## Troubleshooting

- **Cert SANs are baked at first start.** Changing `--tls-san` (e.g. re-running Install with `-Force` and different `-TlsSans`) rewrites the unit file but does NOT rotate the serving cert. Rotate certs or fully uninstall/reinstall for new SANs to take effect.
- **Uninstall kill-all warning:** `k3s-uninstall.sh` runs a kill-all script that can kill processes belonging to other container runtimes.
- **sudo timeout:** mutating actions call `sudo -v` once up front; on long unattended runs the cached credentials may expire mid-action.
- **Install pre-flight failures:** ports 80/443/6443/50051 must be free, `/` needs >= 5 GB free, and `curl`/`sudo` must exist. Inspect conflicts with `ss -tlnp`.
- **kubeconfig lifecycle:** Install backs up an existing `~/.kube/config` to `~/.kube/config.backup-<timestamp>`; Uninstall restores the newest backup or removes the file.
