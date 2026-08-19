# Plan: k3s cluster on Debian 13 VM + management script + admin skill

> Canonical copy of this plan (the `.kilo/plans/` duplicate was removed).
> Review findings + per-item feedback: `docs/plan/k3s-vm-cluster-plan-review.md`.
> This revision incorporates the review verdicts (R1–R18).

## Context

- The Debian 13 VM (IP `192.168.122.200`, Tailscale IP `100.118.225.119`) does not run any Kubernetes workload yet — k3d was never installed or started in this VM (review R1), so there is nothing to decommission. Plain k3s gives a systemd-managed single-node cluster without any Docker/k3d indirection.
- `pwsh` is installed in the VM (`/home/nam20485/.local/bin/pwsh`). No k3s installed yet (`systemctl is-active k3s` -> inactive/not found).
- Existing repo scripts to mirror: `scripts/create-k3d-cluster.ps1`, `scripts/start-k3d-cluster.ps1`.

## Decisions (confirmed with user)

1. **k3s, not k3d** — correct for a dedicated VM.
2. **Management script runs inside the Debian VM** under pwsh (no SSH indirection).
3. **k3s default ports** — built-in Traefik ingress on 80/443 (no HelmChartConfig port remap). Clients use `http://<vm-ip>/`. To make that route, `deploy/kube/local-ingress.yaml` drops its `host: precision5820` rule (review R3) — with the host rule in place, requests to a bare IP get a Traefik 404.
4. **tls-san** — both `192.168.122.200` and `100.118.225.119` so kubeconfig works from the Windows host (libvirt NAT) and anywhere on the tailnet.
5. **gRPC exposure** — no manifest changes needed to `deploy/kube/OdbDesignServer/service-grpc.yaml`: it is `type: LoadBalancer` on 50051; k3s ServiceLB (klipper-lb) binds it on the node directly. (The ingress manifest does change — see decision 3 and task 4.)
6. **Skill location** — client-neutral project-level `.agents/skills/k3s-admin/SKILL.md` (decided in review; note Kilo auto-discovers skills in `.kilo/skills/`, so add a symlink there if Kilo auto-loading is wanted).
7. **De-hardcoding `precision5820[:8081]` is in scope** (review R15) — the cluster now lives on `debian13vm`; every hard-coded `precision5820:8081` URL in manifests, swagger, and docs is updated to the k3s URLs (port 80, no hostname dependency). See task 4.

## Port / access map (after install)

| Service | Address | Notes |
|---|---|---|
| Kubernetes API | `https://192.168.122.200:6443`, `https://100.118.225.119:6443` | tls-san includes both IPs |
| Ingress (Traefik) | `http://192.168.122.200/`, `http://100.118.225.119/`, `https://...:443` | k3s defaults; ingress rule has no host filter |
| gRPC (OdbDesignServer) | `192.168.122.200:50051`, `100.118.225.119:50051` | ServiceLB binds the node port |
| kubeconfig | `/etc/rancher/k3s/k3s.yaml` (root, mode 600), copied to `~/.kube/config` for user | |

## Tasks

### 1. Create `scripts/k3s-cluster.ps1` (single multi-action script)

Runs inside the Debian VM. Guard: throw if `$IsWindows` (repo scripts are shared with Windows host). Use `Set-StrictMode -Version Latest; $ErrorActionPreference = "Stop"` matching repo style. Each action that invokes `sudo` runs `sudo -v` once up front to cache credentials (review R14); note the short sudo timeout when running unattended.

Params:
- `[ValidateSet('Install','Start','Stop','Restart','Status','Uninstall')] [string]$Action = 'Status'`
- `[string[]]$TlsSans = @('192.168.122.200','100.118.225.119')` (Install only)
- `[string]$K3sVersion = ''` — when set, exported as `INSTALL_K3S_VERSION` for a reproducible install; pin the concrete version chosen at install time (review R13). Empty = latest stable.
- `[switch]$Force` — Install: re-run the installer over an existing install; Uninstall: skip the interactive confirmation (mirrors `-ForceDelete` pattern in `create-k3d-cluster.ps1`).
- `[int]$ReadyTimeoutSeconds = 300` — applies to every node-Ready wait: Install and Start (review R6).

Actions:
- **Install**:
  - Pre-flight (review R7): fail if any of ports 80/443/6443/50051 is already listening (parse `ss -tln` output in PowerShell, not `grep`); back up an existing `~/.kube/config` to `~/.kube/config.backup-<timestamp>`; check free disk space on `/`; verify `curl` and `sudo` exist.
  - Error out if k3s already installed (`/usr/local/bin/k3s` exists or `systemctl list-unit-files` contains k3s) unless `-Force`. `-Force` semantics (review R8): re-run the installer with the given args and warn loudly if the supplied `-TlsSans` differ from the running unit's — cert SANs are baked at first start, and re-running with different `--tls-san` does not rotate the serving cert (requires cert rotation or full reinstall).
  - Download the installer first, then run it (no piping curl into shell): `curl -sfL https://get.k3s.io -o /tmp/k3s-install.sh`, then `sudo [INSTALL_K3S_VERSION=<pin>] sh /tmp/k3s-install.sh --tls-san <each>`. No `INSTALL_K3S_EXEC` (server is the default single-node role) and no `--write-kubeconfig-mode` (default 600 is correct; the user copy below is the documented access path) (reviews R5, R13).
  - `sudo systemctl enable k3s`.
  - Wait until the node reports Ready, bounded by `-ReadyTimeoutSeconds`.
  - Copy kubeconfig for the user: `mkdir -p ~/.kube; sudo cp /etc/rancher/k3s/k3s.yaml ~/.kube/config; sudo chown $user ~/.kube/config; chmod 600`. Replace `127.0.0.1` server URL with `https://192.168.122.200:6443`.
  - Print summary (port map above + `kubectl get nodes`).
- **Start**: `sudo systemctl start k3s`, then poll node Ready (bounded by `-ReadyTimeoutSeconds`). If already active+Ready, print and exit 0 (mirrors `start-k3d-cluster.ps1` behavior).
- **Stop**: `sudo systemctl stop k3s`.
- **Restart**: `sudo systemctl restart k3s`, then poll node Ready — do not hand-roll Stop-then-Start, which races unit shutdown (review R10).
- **Status** (default action; must not throw when k3s is down — review R9): report `systemctl is-active/is-enabled k3s` and listening ports first; run the kubectl sections (`get nodes -o wide`, `get pods -A`) only when the unit is active. Port listing parses `ss -tln` lines in PowerShell for `:6443`, `:50051`, `:80`, `:443` (no brittle `grep` pattern).
- **Uninstall** (review R11): warn that `k3s-uninstall.sh` runs a killall script that can kill processes belonging to other container runtimes; interactive confirmation (type `uninstall` — mirrors k3d cluster-delete confirmation) unless `-Force`; then `sudo /usr/local/bin/k3s-uninstall.sh`; afterwards restore `~/.kube/config` from the Install-time backup when one exists, otherwise delete `~/.kube/config` (it points at a dead cluster); validate: unit inactive, `/usr/local/bin/k3s` gone, ports 6443/80/443/50051 free.

Implementation notes:
- The k3s installer symlinks `/usr/local/bin/kubectl` by default, so plain `kubectl` against `~/.kube/config` works post-install; no fallback chain needed (review R12).
- No firewall rules needed inside the VM (libvirt NAT + tailnet handle reachability); do not port the `Ensure-IngressFirewallRule` function.

### 2. Create `.agents/skills/k3s-admin/SKILL.md`

Frontmatter: `name: k3s-admin`, `description: Administer the k3s cluster in the Debian 13 VM — install/start/stop/status/uninstall, deploy workloads, kubectl access, ports, logs, troubleshooting.`

Content:
- Where the cluster lives (Debian 13 VM, single node, systemd unit `k3s`).
- Lifecycle: `pwsh scripts/k3s-cluster.ps1 -Action <Install|Start|Stop|Restart|Status|Uninstall>` (run inside the VM).
- kubectl access: `~/.kube/config` (user, mode 600) / `/etc/rancher/k3s/k3s.yaml` (root); API endpoints on 6443 via both IPs.
- Deploying OdbDesign (review R4): `pwsh scripts/deploy.ps1 -ClusterName default -SkipGrpcValidation`. `-ClusterName default` selects the k3s kubeconfig context; `-SkipGrpcValidation` is required because `scripts/validate-grpc-exposure.ps1` is k3d/Docker-specific (`docker port` against the `k3d-*-serverlb` container). Validate gRPC manually afterwards: `grpcurl -plaintext <vm-ip>:50051 list`.
- Logs/diagnostics: `sudo journalctl -u k3s -f`, `kubectl logs -n kube-system -l app=traefik`, `kubectl get events -A --sort-by=.lastTimestamp`.
- Port map table (above).
- Common ops: restart deployment, check ServiceLB (`kubectl get svc -A`), node IP discovery.
- Troubleshooting (reviews R8, R11): cert SANs are baked at first start — changing `--tls-san` after install needs cert rotation or reinstall; `k3s-uninstall.sh` killall warning; sudo credential timeout when running unattended.

### 3. Install k3s in the VM (execute)

Run the new script from the repo root in the VM: `pwsh scripts/k3s-cluster.ps1 -Action Install` (requires sudo password interactively; agent should ask user to run or enter password at prompt — the plan agent does not run mutating commands).

### 4. De-hardcode `precision5820[:8081]` (reviews R3, R15)

The cluster moves from the `precision5820` k3d box to `debian13vm` with k3s default ports; every hard-coded reference is updated:
- `deploy/kube/local-ingress.yaml` — remove `host: precision5820` from the rule (and the commented TLS hosts) so requests to any host, including both VM IPs, route instead of 404 (review R3).
- `deploy/helm/values-prom.yaml` — `grafana.ingress.hosts`, `grafana.grafana.ini.server.root_url`, `prometheus.ingress.hosts` (plus the comments referencing `precision5820:8081` / k3d): host-agnostic ingress and `http://192.168.122.200/grafana|/prometheus` URLs on port 80.
- `swagger/odbdesign-server-0.9-swagger.yaml:17` — server URL `http://precision5820:8081` -> `http://192.168.122.200` (tailnet IP as a secondary entry).
- `docs/research/API.md:117` — Local Network URL -> `http://192.168.122.200` (plus the tailnet URL).
- `docs/monitoring-grafana-prometheus-trivy.md` — rewrite the access model (intro, access table, firewall step, checklists): k3s Traefik on 80/443, no `precision5820` host requirement, no k3d 8081 port mapping.

### 5. Make `scripts/deploy.ps1` path-portable (review R4)

Replace the backslash joins at `scripts/deploy.ps1:31` and `:63` (`"$PSScriptRoot\..."`) with `Join-Path $PSScriptRoot '...'` so the script runs under Linux pwsh in the VM. `validate-grpc-exposure.ps1` itself stays k3d-only (it is skipped via `-SkipGrpcValidation`).

## Out of scope

- `deploy/kube/k3d-volume-pv.yaml` hostPath (`/mnt/d/k3dvolume`) — review R2 was rejected ("why are we fixing the k3d script?"); the PV/PVC manifests are k3d leftovers and are not touched here. Known consequence: workloads mounting that PV will not run correctly in the VM until the PV is reworked separately.
- Migrating/copying the Docker-registry secret or any persisted data — fresh cluster assumes `deploy.ps1` will be re-run to apply manifests. (No old k3d cluster exists to migrate from — review R1.)
- Any Windows-host-side scripts or scheduled tasks (`StartWithWindows` equivalent is just VM boot; k3s auto-starts via systemd enable).

## Validation

1. `pwsh scripts/k3s-cluster.ps1 -Action Status` shows k3s active, node Ready, pods Running (traefik, coredns, metrics-server).
2. From VM: `kubectl get nodes -o wide` using `~/.kube/config`.
3. From Windows host: `curl -k https://192.168.122.200:6443/livez` returns ok; `kubectl --server=https://192.168.122.200:6443 ...` (after copying kubeconfig) works without TLS errors.
4. Tailnet IP (review R16): `curl -k https://100.118.225.119:6443/livez` returns ok — the tailnet SAN is half the justification for decision 4.
5. `sudo ss -tlnp` shows 6443 and 80/443 (traefik); after running `deploy.ps1`, 50051 bound by ServiceLB.
6. Ingress routing (review R17): `curl -s -o /dev/null -w '%{http_code}' http://192.168.122.200/` and the same against `http://100.118.225.119/` both return 200 (not a Traefik 404) — would have caught review R3.
7. Post-deploy gRPC (reviews R16, R17): `grpcurl -plaintext 192.168.122.200:50051 list` and `grpcurl -plaintext 100.118.225.119:50051 list` both succeed (deploy.ps1 runs with `-SkipGrpcValidation`, so this is the gRPC gate).
8. `pwsh scripts/k3s-cluster.ps1 -Action Stop` -> Status shows inactive and does not throw (review R9) -> Start returns the node to Ready.
9. Restart (review R18): `-Action Restart` returns the node to Ready without manual Stop/Start.
10. Uninstall + kubeconfig lifecycle (review R18, in a throwaway install): Install with a pre-existing `~/.kube/config` creates the timestamped backup; Uninstall restores (or removes) it, and afterwards `systemctl is-active k3s` is inactive, `/usr/local/bin/k3s` is gone, and the ports are free; `-Force` skips the confirmation.

## Risks

- get.k3s.io network fetch may be blocked/slow in the VM — installer download could fail; retry or fetch installer via alternative mirror.
- Port 80/443/6443/50051 collision with anything in the VM — mitigated by the Install pre-flight port check (task 1, review R7).
- `~/.kube/config` may already exist in the VM — mitigated by the timestamped backup (task 1, review R7).
- Cert SANs are baked at first start; a later `-TlsSans` change under `-Force` does not rotate the serving cert (review R8) — documented in the skill's troubleshooting section.
