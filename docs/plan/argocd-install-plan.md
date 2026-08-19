# Plan: Install Argo CD on the k3s VM cluster (`scripts/argocd.ps1`)

## Goal

Add a single PowerShell script, `scripts/argocd.ps1`, that installs Argo CD into the single-node k3s cluster in the Debian 13 VM following the official getting-started guide (https://argo-cd.readthedocs.io/en/stable/getting_started/), and supports distinct invocations for install, status, and validation:

```powershell
pwsh scripts/argocd.ps1 -Action Install    # deploy Argo CD (idempotent)
pwsh scripts/argocd.ps1 -Action Status     # read-only overview (default, safe when not installed)
pwsh scripts/argocd.ps1 -Action Validate   # exit-code health check (rollouts + API healthz)
pwsh scripts/argocd.ps1 -Action Uninstall  # remove Argo CD (confirmation required, -Force skips)
```

## Context / environment facts

- Cluster: single-node k3s in the Debian 13 VM (`192.168.122.200`), managed by `scripts/k3s-cluster.ps1`. The new script runs **inside the VM** with pwsh 7.x (same as `k3s-cluster.ps1`).
- kubectl gotcha: `/usr/local/bin/kubectl` is the k3s wrapper and forces `KUBECONFIG=/etc/rancher/k3s/k3s.yaml` (root-only) unless `KUBECONFIG` is preset. The script must pin `$env:KUBECONFIG = ~/.kube/config` (same as `scripts/k3s-cluster.ps1:36`).
- Repo conventions to mimic (`scripts/k3s-cluster.ps1`, `scripts/deploy-monitoring.ps1`): `param()` with `[ValidateSet]` action, `Set-StrictMode -Version Latest`, `$ErrorActionPreference = "Stop"`, throw `$IsWindows` guard, `Test-Command` helper, `Wait-RolloutInNamespace` helper.
- Official install steps (current stable docs):
  ```bash
  kubectl create namespace argocd
  kubectl apply -n argocd --server-side --force-conflicts \
    -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml
  ```
  `--server-side --force-conflicts` is **required** (ApplicationSet CRD exceeds the 262KB client-side apply annotation limit). Docs recommend pinning a version tag for production.

## Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Install method | Raw manifests from `stable` branch (docs' getting-started), overridable via `-Version` param (e.g. `v3.2.0` → `manifests/install.yaml` at that tag) | Matches the linked docs; no Helm dependency. `-Version` enables the docs' pinning recommendation |
| Namespace | `argocd` (default) | Docs default; manifests' ClusterRoleBindings hardcode it |
| Apply flags | `--server-side --force-conflicts` on every apply | Required by CRD size; makes re-runs/upgrades idempotent |
| Exposure | **None by default** — Validate uses `kubectl port-forward svc/argocd-server -n argocd 8080:443` | k3s ServiceLB cannot publish the argocd-server LB on host port 443/80 (Traefik already binds them → LB stuck Pending). Ingress exposure is a follow-up (see Out of scope) |
| Validate scope | Workloads rolled out + API `/healthz` over port-forward + initial-admin secret exists | No `argocd` CLI dependency; fully scriptable exit code |
| Admin password | Not printed by default; `Status` shows whether `argocd-initial-admin-secret` exists and prints the retrieval one-liner | Avoids leaking credentials into logs; docs warn to delete it after first password change |
| Uninstall | Included for symmetry with `k3s-cluster.ps1`; deletes the manifest URL resources + namespace after typed confirmation (`argocd`), `-Force` skips | Cheap to add, completes lifecycle |

## Script specification — `scripts/argocd.ps1`

### Parameters

```powershell
param(
    [ValidateSet('Install','Status','Validate','Uninstall')]
    [string]$Action = 'Status',
    # Install: Argo CD version tag (empty = stable branch manifests)
    [string]$Version = '',
    # Install: wait for all rollouts to complete
    [switch]$Wait = $true,
    [int]$WaitTimeoutSeconds = 600,
    # Validate: local port used for the port-forward health probe
    [int]$HealthCheckPort = 8080,
    # Uninstall: skip the typed confirmation
    [switch]$Force = $false
)
```

### Shared setup (all actions)

1. `Set-StrictMode -Version Latest`; `$ErrorActionPreference = "Stop"`; throw if `$IsWindows` ("must run inside the Debian VM").
2. Pin `$env:KUBECONFIG = (Join-Path $HOME '.kube/config')`.
3. Preflight: `kubectl` on PATH (`Test-Command`); cluster reachable via `kubectl get nodes -o name` — on failure, point the user to `pwsh scripts/k3s-cluster.ps1 -Action Status/Start`.
4. Compute manifest URL: `https://raw.githubusercontent.com/argoproj/argo-cd/{stable|$Version}/manifests/install.yaml`.

### `-Action Install`

1. `kubectl create namespace argocd --dry-run=client -o yaml | kubectl apply -f -` (idempotent; pattern from `deploy-monitoring.ps1:73`).
2. `kubectl apply -n argocd --server-side --force-conflicts -f <manifest URL>`.
3. If `$Wait`: reuse the `Wait-RolloutInNamespace` pattern (deploy/sts/ds in `argocd`, `kubectl rollout status ... --timeout`).
4. Print next steps: port-forward command, initial-password retrieval one-liner, and a note that the initial password secret should be deleted after first login.

### `-Action Status` (default; read-only; must not throw when Argo CD is absent)

- Namespace `argocd` exists? If not, print "not installed" and exit 0.
- `kubectl get pods -n argocd -o wide` (Ready/Status per pod).
- Deployments/statefulsets with ready replicas (`kubectl get deploy,sts -n argocd`).
- `kubectl get svc -n argocd` (argocd-server type/IP).
- Installed version: image tag of `deployment/argocd-server`.
- Presence of `argocd-initial-admin-secret` (+ retrieval one-liner `kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d`), and a reminder to delete it after rotating the password.

### `-Action Validate` (exit 0 = healthy, non-zero otherwise; collects all failures before exiting)

1. Namespace exists and is not terminating.
2. Expected deployments all exist and report `readyReplicas == replicas`: `argocd-server`, `argocd-repo-server`, `argocd-application-controller`, `argocd-applicationset-controller`, `argocd-notifications-controller`; statefulset `argocd-redis` (enumerate via `kubectl get deploy,sts -n argocd` rather than hard-failing on a fixed list, but assert the core ones).
3. No pods in the namespace in `Pending|CrashLoopBackOff|ImagePullBackOff|Error` (report offenders).
4. Argo CD CRDs present (`kubectl get crd applications.argoproj.io applicationsets.argoproj.io appprojects.argoproj.io`).
5. API health probe:
   - Start `kubectl port-forward svc/argocd-server -n argocd ${HealthCheckPort}:443` as a background process (`Start-Process -PassThru`).
   - Poll `Invoke-WebRequest -Uri "https://localhost:${HealthCheckPort}/healthz" -SkipCertificateCheck` (self-signed cert) up to ~30s; success = HTTP 200.
   - Always stop/kill the port-forward process in a `finally` block.
6. `argocd-initial-admin-secret` exists (informational failure if absent — admin login impossible).
7. Print PASS/FAIL summary table; exit 1 if any check failed.

### `-Action Uninstall`

1. Unless `-Force`, require typed confirmation (`argocd`), same UX as `k3s-cluster.ps1` Uninstall.
2. `kubectl delete -n argocd --server-side --force-conflicts -f <manifest URL> --ignore-not-found`.
3. `kubectl delete namespace argocd --ignore-not-found` (warn on stuck finalizers with a hint to inspect `kubectl get ns argocd -o yaml`).

## Implementation steps (ordered)

1. Create `scripts/argocd.ps1` per the spec above, mirroring structure/helpers of `scripts/k3s-cluster.ps1` (strict mode, KUBECONFIG pin, Windows guard, `Test-Command`) and the rollout-wait helper from `scripts/deploy-monitoring.ps1`.
2. Add a short usage section for the script to `scripts/README.md` if that file documents sibling scripts (check first; skip if it doesn't).
3. Update the k3s-admin skill only if the user asks (out of scope by default).

## Validation plan (run inside the VM, from repo root)

```powershell
pwsh scripts/k3s-cluster.ps1 -Action Status          # cluster must be running first
pwsh scripts/argocd.ps1 -Action Status               # "not installed" before install, exit 0
pwsh scripts/argocd.ps1 -Action Install              # applies manifests, waits for rollouts
pwsh scripts/argocd.ps1 -Action Status               # pods ready, version, secret present
pwsh scripts/argocd.ps1 -Action Validate             # exit 0, healthz 200 over port-forward
pwsh scripts/argocd.ps1 -Action Install              # re-run: idempotent, no errors (server-side apply)
pwsh scripts/argocd.ps1 -Action Uninstall            # typed confirmation; namespace removed
pwsh scripts/argocd.ps1 -Action Status               # back to "not installed"
```

Manual smoke test of UI access after Install:
`kubectl port-forward svc/argocd-server -n argocd 8080:443` then browse `https://localhost:8080` (accept self-signed cert), login `admin` + secret password.

## Risks / failure modes

- **Port-forward probe flakiness**: `kubectl port-forward` can race the API; mitigate with retry/poll loop (30s) and guaranteed process cleanup in `finally`.
- **`stable` branch drift**: unpinned manifests can change under us; `-Version` pin is available and Install prints the resolved manifest URL.
- **Image pulls**: first install pulls ~7 images; on slow links the default 600s wait may need `-WaitTimeoutSeconds` bump.
- **Port 8080 busy** during Validate: pre-check the port (`ss -tln`) and fail fast with a clear message, or accept `-HealthCheckPort` override.
- **sudo/k3s not involved**: script needs no sudo; only user kubeconfig access.

## Out of scope (follow-ups)

- Traefik Ingress route for the Argo CD UI (needs `ssl-passthrough` annotation or cert replacement per the Argo CD ingress docs) — ServiceLB on 443/80 conflicts with Traefik, so no LoadBalancer patch by default.
- Installing the `argocd` CLI binary.
- Creating/syncing Argo CD Applications (guestbook example from the docs).
- HA manifests variant and external cluster registration.
