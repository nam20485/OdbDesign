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
| Validate scope | Workloads rolled out + API `/healthz` over port-forward + initial-admin secret exists (warning-only if absent) | No `argocd` CLI dependency; fully scriptable exit code |
| Admin password | Not printed by default; `Status` shows whether `argocd-initial-admin-secret` exists and prints the retrieval one-liner | Avoids leaking credentials into logs; docs warn to delete it after first password change |
| Uninstall | Included for symmetry with `k3s-cluster.ps1`; deletes the manifest URL resources + namespace after typed confirmation (`argocd`), `-Force` skips | Cheap to add, completes lifecycle |

## Script specification — `scripts/argocd.ps1`

### Parameters

```powershell
param(
    [ValidateSet('Install','Status','Validate','Uninstall')]
    [string]$Action = 'Status',
    # Install: Argo CD version tag (empty = stable branch manifests); leading 'v' optional ('3.5.1' -> 'v3.5.1')
    [ValidatePattern('^v?\d+\.\d+\.\d+$')]
    [string]$Version = '',
    # Install: opt-in wait for all rollouts to complete (repo convention, cf. deploy-monitoring.ps1)
    [switch]$Wait,
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
4. Normalize `-Version` (`if ($Version -and $Version -notmatch '^v') { $Version = "v$Version" }`) — GitHub tags require the leading `v`; without it raw.githubusercontent returns a bare 404 mid-install. Then compute the manifest URL: `https://raw.githubusercontent.com/argoproj/argo-cd/{stable|$Version}/manifests/install.yaml`, and print the resolved URL before applying.

### `-Action Install`

1. `kubectl create namespace argocd --dry-run=client -o yaml | kubectl apply -f -` (idempotent; pattern from `deploy-monitoring.ps1:73`).
2. `kubectl apply -n argocd --server-side --force-conflicts -f <manifest URL>`.
3. If `$Wait` (opt-in): reuse the `Wait-RolloutInNamespace` pattern (deploy/sts/ds in `argocd`, `kubectl rollout status ... --timeout`). After the wait, print a `kubectl top node` snapshot as a memory-headroom sanity check (see Risks).
4. Print next steps: port-forward command, initial-password retrieval one-liner (pwsh-native form, see Status), and a note that the initial password secret should be deleted after first login.

### `-Action Status` (default; read-only; must not throw when Argo CD is absent)

- Namespace `argocd` exists? If not, print "not installed" and exit 0.
- `kubectl get pods -n argocd -o wide` (Ready/Status per pod).
- Deployments/statefulsets with ready replicas (`kubectl get deploy,sts -n argocd`).
- `kubectl get svc -n argocd` (argocd-server type/IP).
- Installed version: image tag of `deployment/argocd-server`.
- Presence of `argocd-initial-admin-secret` (+ retrieval one-liner; prefer the pwsh-native form since the script itself is pwsh: `[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String((kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath='{.data.password}')))`), and a reminder to delete it after rotating the password.

### `-Action Validate` (exit 0 = healthy, non-zero otherwise; collects all failures before exiting)

1. Namespace exists and is not terminating. If the namespace is missing, print the single line `Argo CD is not installed (run: pwsh scripts/argocd.ps1 -Action Install)` and `exit 1` — absence is failure for a health check, but it must be a clean failure, not a StrictMode/`$ErrorActionPreference = "Stop"` stack trace from the first kubectl call.
2. Expected workloads exist and report `readyReplicas == replicas` — Deployments: `argocd-server`, `argocd-repo-server`, `argocd-redis`, `argocd-dex-server`, `argocd-notifications-controller`, `argocd-applicationset-controller`; StatefulSet: `argocd-application-controller`. This is the split in the current stable manifest (v3.5.1): `argocd-redis` is a **Deployment** and the only StatefulSet is `argocd-application-controller`. Enumerate `kubectl get deploy,sts -n argocd` as the source of truth so a future manifest change (e.g. a redis kind flip) degrades to a warning rather than a hard mismatch, but assert the core set above.
3. No pods in the namespace in `Pending|CrashLoopBackOff|ImagePullBackOff|Error` (report offenders).
4. Argo CD CRDs present (`kubectl get crd applications.argoproj.io applicationsets.argoproj.io appprojects.argoproj.io`).
5. Health-check port preflight (hard): verify `-HealthCheckPort` is not already listening, reusing the `Get-ListeningPorts` / `Test-TcpPortReachable` helpers from `k3s-cluster.ps1:38-74`; if busy, fail fast with a message pointing at the `-HealthCheckPort` override (without this, a busy port surfaces as a confusing 30s probe-timeout failure).
6. API health probe:
   - Start `kubectl port-forward svc/argocd-server -n argocd ${HealthCheckPort}:443` as a background process (`Start-Process -PassThru`), redirecting stdout/stderr to temp files so unredirected native output cannot interleave/block the probe.
   - Poll `Invoke-WebRequest -Uri "https://localhost:${HealthCheckPort}/healthz" -SkipCertificateCheck -TimeoutSec 5` (self-signed cert) up to ~30s; success = HTTP 200. Every attempt needs `-TimeoutSec` (the default can hang far beyond the 30s budget). Treat non-2xx as retryable, not fatal: either wrap in try/catch and retry, or use `-SkipHttpErrorCheck` (pwsh 7+) and test `.StatusCode -eq 200`.
   - Always stop/kill the port-forward process in a `finally` block.
7. `argocd-initial-admin-secret` exists — **warning-only** if absent (does not affect the exit code): the docs say to delete this secret after the first password change (and Install reminds the user to do so), so treating absence as failure would break Validate on a correctly-hardened install.
8. Print PASS/FAIL summary table; exit 1 if any hard check (steps 1–6) failed.

### `-Action Uninstall`

1. Unless `-Force`, require typed confirmation (`argocd`), same UX as `k3s-cluster.ps1` Uninstall.
2. `kubectl delete -n argocd -f <manifest URL> --ignore-not-found` — plain `delete -f`, no `--server-side --force-conflicts`: those are `kubectl apply`-only flags and `kubectl delete` rejects them (`unknown flag`). Deletion enumerates object references from the manifest itself, so it works regardless of whether the resources were applied server-side.
3. `kubectl delete namespace argocd --ignore-not-found`. Warn on stuck finalizers with a hint to inspect `kubectl get ns argocd -o yaml`, and extend the hint to the cluster-scoped CRDs deleted in step 2 (`applications/applicationsets/appprojects.argoproj.io`): if any Applications/ApplicationSets still exist, CRD deletion can hang on finalizers too (`kubectl get crd <name> -o yaml`). Acceptable on this single-purpose cluster.

## Implementation steps (ordered)

1. Create `scripts/argocd.ps1` per the spec above, mirroring structure/helpers of `scripts/k3s-cluster.ps1` (strict mode, KUBECONFIG pin, Windows guard, `Test-Command`) and the rollout-wait helper from `scripts/deploy-monitoring.ps1`.
2. `scripts/README.md`: **skip** (condition resolved) — that file documents only the test-automation shell scripts (`run-tests.sh`, `coverage.sh`, `setup-linux.sh`); neither `k3s-cluster.ps1` nor `deploy-monitoring.ps1` is documented there, so adding `argocd.ps1` would be the odd one out. The cluster-script documentation home is the k3s-admin skill (step 3).
3. Update the k3s-admin skill only if the user asks (out of scope by default).

## Validation plan (run inside the VM, from repo root)

```powershell
pwsh scripts/k3s-cluster.ps1 -Action Status          # cluster must be running first
pwsh scripts/argocd.ps1 -Action Status               # "not installed" before install, exit 0
pwsh scripts/argocd.ps1 -Action Install -Wait        # applies manifests, waits for rollouts
pwsh scripts/argocd.ps1 -Action Status               # pods ready, version, secret present
pwsh scripts/argocd.ps1 -Action Validate             # exit 0, healthz 200 over port-forward
pwsh scripts/argocd.ps1 -Action Install -Wait        # re-run: idempotent, no errors (server-side apply)
pwsh scripts/argocd.ps1 -Action Uninstall            # typed confirmation; namespace removed
pwsh scripts/argocd.ps1 -Action Status               # back to "not installed"
```

Additional coverage:

```powershell
# Wait opt-out: Install without -Wait returns promptly after apply (no rollout wait)
pwsh scripts/argocd.ps1 -Action Install

# Pinned install / upgrade (the docs' upgrade mechanism = re-install with a different version)
pwsh scripts/argocd.ps1 -Action Install -Version v3.5.1 -Wait   # pinned tag; Status shows v3.5.1 image tag
pwsh scripts/argocd.ps1 -Action Install -Version 3.5.1 -Wait    # missing leading 'v' is normalized, same manifest
pwsh scripts/argocd.ps1 -Action Install -Wait                   # back to stable branch manifests

# Uninstall variants
pwsh scripts/argocd.ps1 -Action Uninstall             # mistyped confirmation -> non-zero exit, nothing deleted
pwsh scripts/argocd.ps1 -Action Uninstall -Force      # skips confirmation

# Validate failure paths
pwsh scripts/argocd.ps1 -Action Validate               # not installed -> clean one-line failure, exit 1
pwsh scripts/argocd.ps1 -Action Validate -HealthCheckPort <busy-port>   # port preflight fails fast, exit 1
kubectl -n argocd scale deploy/argocd-server --replicas=0; pwsh scripts/argocd.ps1 -Action Validate   # induced unhealthy -> exit 1
kubectl -n argocd scale deploy/argocd-server --replicas=1                                             # restore
```

Manual smoke test of UI access after Install:
`kubectl port-forward svc/argocd-server -n argocd 8080:443` then browse `https://localhost:8080` (accept self-signed cert), login `admin` + secret password.

## Risks / failure modes

- **Port-forward probe flakiness**: `kubectl port-forward` can race the API; mitigate with retry/poll loop (30s) and guaranteed process cleanup in `finally`.
- **`stable` branch drift**: unpinned manifests can change under us; `-Version` pin is available and Install prints the resolved manifest URL.
- **Image pulls**: first install pulls ~7 images; on slow links the default 600s wait may need `-WaitTimeoutSeconds` bump.
- **Port 8080 busy** during Validate: hard preflight (Validate step 5) checks the port via the `k3s-cluster.ps1` helpers and fails fast pointing at `-HealthCheckPort`, instead of letting the probe time out after 30s.
- **Memory footprint**: the non-HA install adds 7 pods (~1–1.5 GiB RSS total) next to kube-prometheus-stack, trivy-operator, and the OdbDesign server on the single-node VM; Install prints a `kubectl top node` snapshot after the rollout wait as a sanity check.
- **sudo/k3s not involved**: script needs no sudo; only user kubeconfig access.

## Out of scope (follow-ups)

- Traefik Ingress route for the Argo CD UI (needs `ssl-passthrough` annotation or cert replacement per the Argo CD ingress docs) — ServiceLB on 443/80 conflicts with Traefik, so no LoadBalancer patch by default.
- Installing the `argocd` CLI binary.
- Creating/syncing Argo CD Applications (guestbook example from the docs).
- HA manifests variant and external cluster registration.
