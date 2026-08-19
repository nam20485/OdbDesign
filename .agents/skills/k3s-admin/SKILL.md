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
- The k3s installer symlinks `/usr/local/bin/kubectl`, so plain `kubectl` works.

## Port / access map

| Service | Address | Notes |
|---|---|---|
| Kubernetes API | `https://192.168.122.200:6443`, `https://100.118.225.119:6443` | tls-san includes both IPs |
| Ingress (Traefik) | `http://192.168.122.200/`, `http://100.118.225.119/`, `https://...:443` | k3s defaults; `local-ingress.yaml` has no host filter |
| gRPC (OdbDesignServer) | `192.168.122.200:50051`, `100.118.225.119:50051` | ServiceLB binds the node port after deploy |
| kubeconfig | `/etc/rancher/k3s/k3s.yaml` (root), `~/.kube/config` (user) | |

## Deploying OdbDesign

```powershell
pwsh scripts/deploy.ps1 -ClusterName default -SkipGrpcValidation
```

- `-ClusterName default` selects the k3s kubeconfig context (the k3d-specific `k3d-k3dcluster` context does not exist here).
- `-SkipGrpcValidation` is required: `scripts/validate-grpc-exposure.ps1` is k3d/Docker-specific (it inspects the `k3d-*-serverlb` container via `docker port`).
- Validate gRPC manually afterwards: `grpcurl -plaintext 192.168.122.200:50051 list` (and/or the tailnet IP `100.118.225.119`).

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
