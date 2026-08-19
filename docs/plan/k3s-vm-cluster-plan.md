# Plan: k3s cluster on Debian 13 VM + management script + admin skill

> Canonical copy of this plan (the `.kilo/plans/` duplicate was removed).
> Review findings + per-item feedback: `docs/plan/k3s-vm-cluster-plan-review.md`.

## Context

- The Debian 13 VM (IP `192.168.122.200`, Tailscale IP `100.118.225.119`) currently runs the OdbDesign k8s workload via k3d inside Docker. k3d is unnecessary indirection in a dedicated VM; plain k3s gives a systemd-managed single-node cluster.
- `pwsh` is installed in the VM (`/home/nam20485/.local/bin/pwsh`). No k3s installed yet (`systemctl is-active k3s` -> inactive/not found).
- Existing repo scripts to mirror: `scripts/create-k3d-cluster.ps1`, `scripts/start-k3d-cluster.ps1`.

## Decisions (confirmed with user)

1. **k3s, not k3d** — correct for a dedicated VM.
2. **Management script runs inside the Debian VM** under pwsh (no SSH indirection).
3. **k3s default ports** — built-in Traefik ingress on 80/443 (no HelmChartConfig port remap). Existing clients that assumed host port 8081 must use `http://<vm-ip>/`.
4. **tls-san** — both `192.168.122.200` and `100.118.225.119` so kubeconfig works from the Windows host (libvirt NAT) and anywhere on the tailnet.
5. **gRPC exposure** — no manifest changes needed: `deploy/kube/OdbDesignServer/service-grpc.yaml` is `type: LoadBalancer` on 50051; k3s ServiceLB (klipper-lb) binds it on the node directly.
6. **Skill location** — client-neutral project-level `.agents/skills/k3s-admin/SKILL.md` (decided in review; note Kilo auto-discovers skills in `.kilo/skills/`, so add a symlink there if Kilo auto-loading is wanted).

## Port / access map (after install)

| Service | Address | Notes |
|---|---|---|
| Kubernetes API | `https://192.168.122.200:6443`, `https://100.118.225.119:6443` | tls-san includes both IPs |
| Ingress (Traefik) | `http://192.168.122.200/`, `https://...:443` | k3s defaults |
| gRPC (OdbDesignServer) | `192.168.122.200:50051` | ServiceLB binds node port |
| kubeconfig | `/etc/rancher/k3s/k3s.yaml` (root), copied to `~/.kube/config` for user | |

## Tasks

### 1. Create `scripts/k3s-cluster.ps1` (single multi-action script)

Runs inside the Debian VM. Guard: throw if `$IsWindows` (repo scripts are shared with Windows host). Use `Set-StrictMode -Version Latest; $ErrorActionPreference = "Stop"` matching repo style.

Params:
- `[ValidateSet('Install','Start','Stop','Restart','Status','Uninstall')] [string]$Action = 'Status'`
- `[string[]]$TlsSans = @('192.168.122.200','100.118.225.119')` (Install only)
- `[switch]$Force` (skips the interactive confirmation for Uninstall — mirrors `-ForceDelete` pattern in `create-k3d-cluster.ps1`)
- `[int]$ReadyTimeoutSeconds = 300` — how long to wait for node Ready after start

Actions:
- **Install**:
  - Error out if k3s already installed (`/usr/local/bin/k3s` exists or `systemctl list-unit-files` contains k3s) unless `-Force` (re-run installer is idempotent, but warn).
  - `curl -sfL https://get.k3s.io | INSTALL_K3S_EXEC="server" sh -s - --tls-san <each> --write-kubeconfig-mode 644` (run via `sudo sh -c` so quoting works from pwsh; prefer downloading installer to a temp file then `sudo sh install.sh ...`).
  - `sudo systemctl enable k3s`.
  - Wait until `sudo k3s kubectl get nodes` shows Ready.
  - Copy kubeconfig for the user: `mkdir -p ~/.kube; sudo cp /etc/rancher/k3s/k3s.yaml ~/.kube/config; sudo chown $user ~/.kube/config; chmod 600`. Replace `127.0.0.1` server URL with `https://192.168.122.200:6443`.
  - `sudo mkdir -p /k3dvolume` so `deploy/kube/k3d-volume-pv.yaml` (hostPath) keeps working unchanged.
  - Print summary (port map above + `kubectl get nodes`).
- **Start**: `sudo systemctl start k3s`, then poll `kubectl get nodes` until Ready (timeout param). If already active+Ready, print and exit 0 (mirrors `start-k3d-cluster.ps1` behavior).
- **Stop**: `sudo systemctl stop k3s`.
- **Restart**: Stop then Start.
- **Status**: `systemctl is-active/is-enabled k3s`, `kubectl get nodes -o wide`, `kubectl get pods -A`, listening ports (`sudo ss -tlnp | grep -E '6443|50051|:80 |:443 '`).
- **Uninstall**: interactive confirmation (type `uninstall` — mirrors k3d cluster-delete confirmation), then `sudo /usr/local/bin/k3s-uninstall.sh`.

Implementation notes:
- `kubectl` in the VM = `k3s kubectl`; if `/usr/local/bin/kubectl` symlink isn't on PATH, use `sudo k3s kubectl` or the user kubeconfig with `kubectl`. Prefer plain `kubectl` against `~/.kube/config`.
- No firewall rules needed inside the VM (libvirt NAT + tailnet handle reachability); do not port the `Ensure-IngressFirewallRule` function.

### 2. Create `.agents/skills/k3s-admin/SKILL.md`

Frontmatter: `name: k3s-admin`, `description: Administer the k3s cluster in the Debian 13 VM — install/start/stop/status/uninstall, deploy workloads, kubectl access, ports, logs, troubleshooting.`

Content:
- Where the cluster lives (Debian 13 VM, single node, systemd unit `k3s`).
- Lifecycle: `pwsh scripts/k3s-cluster.ps1 -Action <Install|Start|Stop|Restart|Status|Uninstall>` (run inside the VM).
- kubectl access: `~/.kube/config` (user) / `/etc/rancher/k3s/k3s.yaml` (root); API endpoints on 6443 via both IPs.
- Deploying OdbDesign: `scripts/deploy.ps1` applies fine but step 1 (`kubectl config use-context k3d-k3dcluster`) is k3d-specific — with k3s the kubeconfig context is `default`; note to skip that line or pass context accordingly (out of scope to change deploy.ps1 unless asked).
- Logs/diagnostics: `sudo journalctl -u k3s -f`, `kubectl logs -n kube-system -l app=traefik`, `kubectl get events -A --sort-by=.lastTimestamp`.
- Port map table (above).
- Common ops: restart deployment, check ServiceLB (`kubectl get svc -A`), node IP discovery.

### 3. Install k3s in the VM (execute)

Run the new script from the repo root in the VM: `pwsh scripts/k3s-cluster.ps1 -Action Install` (requires sudo password interactively; agent should ask user to run or enter password at prompt — the plan agent does not run mutating commands).

## Out of scope

- Modifying `deploy.ps1` / manifests for a k3s context (deploy.ps1's `kubectl config use-context k3d-k3dcluster` line is k3d-specific; document in the skill instead).
- Migrating/copying the Docker-registry secret or any persisted data from the old k3d cluster — fresh cluster assumes `deploy.ps1` will be re-run to apply manifests.
- Any Windows-host-side scripts or scheduled tasks (`StartWithWindows` equivalent is just VM boot; k3s auto-starts via systemd enable).

## Validation

1. `pwsh scripts/k3s-cluster.ps1 -Action Status` shows k3s active, node Ready, pods Running (traefik, coredns, metrics-server).
2. From VM: `kubectl get nodes -o wide` using `~/.kube/config`.
3. From Windows host: `curl -k https://192.168.122.200:6443/livez` returns ok; `kubectl --server=https://192.168.122.200:6443 ...` (after copying kubeconfig) works without TLS errors.
4. `sudo ss -tlnp` shows 6443 and 80/443 (traefik); after running `deploy.ps1`, 50051 bound by ServiceLB.
5. `pwsh scripts/k3s-cluster.ps1 -Action Stop` -> Status shows inactive -> Start returns node to Ready.

## Risks

- get.k3s.io network fetch may be blocked/slow in the VM — installer download could fail; retry or fetch installer via alternative mirror.
- Port 80/443 collision with anything in the VM (Docker bridges present; check `ss -tlnp` before install) — Traefik binds host ports 80/443 via ServiceLB; a conflicting container would break ingress startup.
- `~/.kube/config` may already exist in the VM — back it up before overwrite.
