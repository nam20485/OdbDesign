# Plan: Install Argo CD on the k3s VM cluster (`scripts/argocd.sh`)

> **Re-targeted 2026-08-31** for the daemon-tier environment (agent user `linux-admin-agent`
> running on debian13vm itself, Hermes profile `linux-admin-agent-vm`). The original revision
> specified a PowerShell script (`scripts/argocd.ps1`) for a VM with pwsh 7.x; **pwsh is not
> installed on this VM**, so the deliverable is now a bash script following the repo's
> bash-twin convention (`setup-vcpkg-cache.sh`, `compress-artifacts.sh`). All review findings
> R1–R12 from `docs/plan/argocd-install-plan-review.md` remain incorporated, translated to bash.
>
> **Branch targeting (per AGENTS.md "Branching & Merge Flow"):** all Argo CD work — this plan,
> `scripts/argocd.sh`, and its follow-ups — lives on `nam/argocd` (cut from `nam20485`) and is
> delivered via PR into `nam20485`.

## Goal

Add a single bash script, `scripts/argocd.sh`, that installs Argo CD into the single-node k3s
cluster on the Debian 13 VM following the official getting-started guide
(https://argo-cd.readthedocs.io/en/stable/getting_started/), and supports distinct invocations
for install, status, and validation:

```bash
bash scripts/argocd.sh install             # deploy Argo CD (idempotent)
bash scripts/argocd.sh status              # read-only overview (default action, safe when not installed)
bash scripts/argocd.sh validate            # exit-code health check (rollouts + API healthz)
bash scripts/argocd.sh uninstall           # remove Argo CD (typed confirmation required, --force skips)
```

## Context / environment facts (verified on the VM, 2026-08-31)

- **Execution tier:** daemon tier — the script runs as `linux-admin-agent` **on debian13vm
  itself** (headless, no GUI). The cluster is documented by the **`k3s-cluster-admin` skill**
  (profile `linux-admin-agent-vm`): single-node k3s **v1.36.3+k3s1**, context `k3s-cluster`,
  API `https://100.118.225.119:6443` (Tailscale), node internal IP `192.168.122.200`.
  The skill's `scripts/health.sh` is the live source of truth for cluster state;
  `scripts/k3s-cluster.ps1` from the old plan is **not** part of this environment's workflow.
- **No pwsh on the VM** — bash 5.2.37 only. Repo precedent for bash twins of pwsh scripts:
  `scripts/setup-vcpkg-cache.sh`, `scripts/compress-artifacts.sh`. Conventions to mimic
  (`scripts/run-tests.sh`, `scripts/setup-vcpkg-cache.sh`): `#!/usr/bin/env bash` header,
  `set -euo pipefail`, `SCRIPT_DIR` resolution, colored `print_status`-style helpers,
  `usage()` + flag parsing, `command -v` existence checks.
- kubectl gotcha (confirmed live): `/usr/local/bin/kubectl` is a symlink to the `k3s` wrapper
  and defaults to root-only `/etc/rancher/k3s/k3s.yaml` → *permission denied* as this user
  unless `KUBECONFIG` is set. The agent's login shell exports it, but the script must not rely
  on that: `export KUBECONFIG="${KUBECONFIG:-$HOME/.kube/config}"`. All kubectl operations work
  without sudo (client-certificate auth from `~/.kube/config`).
- Available tools: `curl`, `openssl`, `ss`, `base64`. **No `jq`** — use `kubectl -o jsonpath`
  / go-template instead; the script must not grow a jq dependency.
- **Port 8080 is already busy on this VM** (busy listeners verified: 631, 2222, 4170, 6333,
  6334, 6443–6444, **8080**, 8082, 9100, 9119, 9222, 10010, 10248–10259, …; the
  k3s-cluster-admin skill also lists 8080/8082/6333/6334/9119 as claimed by non-k8s services).
  The old plan's default health-check port 8080 would fail its own preflight → new default is
  **8443** (verified free).
- Memory headroom: node capacity 16 GiB, current usage ≈6.6 GiB (41 %) — Argo CD's ~1–1.5 GiB
  fits comfortably; Install still prints a `kubectl top node` sanity check.
- Argo CD is **not yet installed** (no `argocd` namespace). Latest upstream release at probe
  time: **v3.5.2** (the review verified the workload split against v3.5.1; unchanged).
- Official install steps (current stable docs):
  ```bash
  kubectl create namespace argocd
  kubectl apply -n argocd --server-side --force-conflicts \
    -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml
  ```
  `--server-side --force-conflicts` is **required** (ApplicationSet CRD exceeds the 262KB
  client-side apply annotation limit). Docs recommend pinning a version tag for production.
- **Daemon-tier operating rules** (apply whenever the agent executes this script, not to the
  script itself): destructive operations — `uninstall` — follow the 6-step cycle
  (Detect → Confirm → Backup → Execute → Verify → Report) and are logged in
  `system-changes/CHANGELOG.md` of the journal repo
  (`~/src/github/nam20485/linux-admin-agent-vm`), `Target: debian13vm (daemon tier)`.
  After any install/uninstall, update the k3s-cluster-admin skill's
  `references/cluster-inventory.md` (skill mandate: "Update it after any change").

## Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Script language | **bash** (`scripts/argocd.sh`), not the old plan's pwsh (`argocd.ps1`) | pwsh not installed on the daemon-tier VM; repo has bash-twin precedent (`setup-vcpkg-cache.sh`, `compress-artifacts.sh`); headless tier is bash-native. Installing pwsh just for this script was considered and rejected as unnecessary weight |
| Install method | Raw manifests from `stable` branch (docs' getting-started), overridable via `--version` (e.g. `v3.5.2` → `manifests/install.yaml` at that tag) | Matches the linked docs; no Helm dependency. `--version` enables the docs' pinning recommendation |
| Namespace | `argocd` (default) | Docs default; manifests' ClusterRoleBindings hardcode it |
| Apply flags | `--server-side --force-conflicts` on every apply | Required by CRD size; makes re-runs/upgrades idempotent |
| Exposure | **None by default** — Validate uses `kubectl port-forward svc/argocd-server -n argocd 8443:443` | k3s ServiceLB cannot publish the argocd-server LB on host port 443/80 (Traefik already binds them → LB stuck Pending). Default local port **8443** because **8080 is busy on this VM**. Ingress exposure is a follow-up (see Out of scope) |
| Validate scope | Workloads rolled out + API `/healthz` over port-forward (`curl -sk`) + initial-admin secret exists (warning-only if absent) | No `argocd` CLI dependency; fully scriptable exit code |
| Admin password | Not printed by default; `status` shows whether `argocd-initial-admin-secret` exists and prints the bash retrieval one-liner (`base64 -d`) | Avoids leaking credentials into logs; docs warn to delete it after first password change |
| Uninstall | Deletes the manifest URL resources + namespace after typed confirmation (`argocd`), `--force` skips | Completes lifecycle; when the *agent* runs it, the journal's destructive-op cycle + CHANGELOG entry additionally apply (profile hard rule) |

## Script specification — `scripts/argocd.sh`

### Interface

```text
Usage: argocd.sh [action] [options]

Actions (default: status):
  install      Deploy Argo CD (idempotent; server-side apply)
  status       Read-only overview; safe when not installed (exit 0)
  validate     Health check; exit 0 = healthy
  uninstall    Remove Argo CD (typed confirmation; --force skips)

Options:
  --version <tag>         install: Argo CD version tag (empty = stable branch manifests);
                          leading 'v' optional ('3.5.2' -> 'v3.5.2')
  --wait                  install: wait for all rollouts to complete (opt-in, repo convention)
  --wait-timeout <sec>    install: rollout wait timeout (default 600)
  --health-check-port <p> validate: local port for the healthz probe (default 8443)
  --force                 uninstall: skip the typed confirmation
  -h, --help
```

### Shared setup (all actions)

1. `set -euo pipefail`; colored status helpers; `usage()` for `-h/--help` and unknown args.
2. `export KUBECONFIG="${KUBECONFIG:-$HOME/.kube/config}"` (see kubectl gotcha above).
3. Preflight: `command -v kubectl` and `command -v curl`; cluster reachable via
   `kubectl get nodes -o name` — on failure, point the user at the k3s-cluster-admin skill
   (`sudo systemctl status k3s`, skill `scripts/health.sh`).
4. Normalize `--version` (`[[ $VERSION =~ ^[0-9] ]] && VERSION="v$VERSION"`; validate
   `^v?[0-9]+\.[0-9]+\.[0-9]+$`) — GitHub tags require the leading `v`; without it
   raw.githubusercontent returns a bare 404 mid-install. Then compute the manifest URL:
   `https://raw.githubusercontent.com/argoproj/argo-cd/{stable|$VERSION}/manifests/install.yaml`,
   and print the resolved URL before applying.

### `install`

1. `kubectl create namespace argocd --dry-run=client -o yaml | kubectl apply -f -` (idempotent).
2. `kubectl apply -n argocd --server-side --force-conflicts -f "$MANIFEST_URL"`.
3. If `--wait` (opt-in): for every deployment/statefulset/daemonset in `argocd`,
   `kubectl rollout status <kind>/<name> -n argocd --timeout="${WAIT_TIMEOUT}s"`. After the
   wait, print a `kubectl top node` snapshot as a memory-headroom sanity check (see Risks).
4. Print next steps: port-forward command (8443), initial-password retrieval one-liner
   (bash form, see `status`), and a note that the initial password secret should be deleted
   after first login.

### `status` (default; read-only; must not fail when Argo CD is absent)

- Namespace `argocd` exists? If not, print "not installed" and exit 0. (Guard the check so
  `set -e` doesn't turn the expected NotFound into a stack trace:
  `kubectl get namespace argocd >/dev/null 2>&1 || { echo "not installed"; exit 0; }`.)
- `kubectl get pods -n argocd -o wide` (Ready/Status per pod).
- Deployments/statefulsets with ready replicas (`kubectl get deploy,sts -n argocd`).
- `kubectl get svc -n argocd` (argocd-server type/IP).
- Installed version: image tag of `deployment/argocd-server`
  (`-o jsonpath='{.spec.template.spec.containers[0].image}'`).
- Presence of `argocd-initial-admin-secret` (+ retrieval one-liner, bash-native here since the
  script is bash):
  ```bash
  kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath='{.data.password}' | base64 -d; echo
  ```
  and a reminder to delete it after rotating the password.

### `validate` (exit 0 = healthy, non-zero otherwise; collects all failures before exiting)

1. Namespace exists and is not terminating. If missing, print the single line
   `Argo CD is not installed (run: bash scripts/argocd.sh install)` and `exit 1` — absence is
   failure for a health check, but it must be a clean failure, not a `set -e` stack trace from
   the first kubectl call.
2. Expected workloads exist and report `readyReplicas == replicas` — Deployments:
   `argocd-server`, `argocd-repo-server`, `argocd-redis`, `argocd-dex-server`,
   `argocd-notifications-controller`, `argocd-applicationset-controller`; StatefulSet:
   `argocd-application-controller`. This is the split in the current stable manifest
   (v3.5.1/v3.5.2): `argocd-redis` is a **Deployment** and the only StatefulSet is
   `argocd-application-controller` (review finding R2). Enumerate
   `kubectl get deploy,sts -n argocd` as the source of truth so a future manifest change
   (e.g. a redis kind flip) degrades to a warning rather than a hard mismatch, but assert the
   core set above.
3. No pods in the namespace in `Pending|CrashLoopBackOff|ImagePullBackOff|Error` (report
   offenders).
4. Argo CD CRDs present
   (`kubectl get crd applications.argoproj.io applicationsets.argoproj.io appprojects.argoproj.io`).
5. Health-check port preflight (hard): verify `--health-check-port` is not already listening
   via `ss -tln` (e.g. `ss -tln | grep -E "[:.]${PORT}\b"`); if busy, fail fast with a message
   pointing at the `--health-check-port` override (without this, a busy port surfaces as a
   confusing 30s probe-timeout failure). Note: 8080/8082/6333/6334/9119 are known-busy on this
   VM — hence the 8443 default.
6. API health probe:
   - Start `kubectl port-forward svc/argocd-server -n argocd ${PORT}:443` as a background
     process (`&`, capture `$!`), redirecting stdout/stderr to temp files so background output
     cannot interleave with probe output; register cleanup in a `trap ... EXIT` (bash's
     "finally") that kills the port-forward PID on every exit path.
   - Poll `curl -sk --max-time 5 -o /dev/null -w '%{http_code}' "https://127.0.0.1:${PORT}/healthz"`
     up to ~30s; success = HTTP 200. Every attempt needs `--max-time` (curl's default can hang
     far beyond the 30s budget). Treat non-2xx as retryable, not fatal.
7. `argocd-initial-admin-secret` exists — **warning-only** if absent (does not affect the exit
   code): the docs say to delete this secret after the first password change (and `install`
   reminds the user to do so), so treating absence as failure would break `validate` on a
   correctly-hardened install (review finding R7).
8. Print PASS/FAIL summary; exit 1 if any hard check (steps 1–6) failed.

### `uninstall`

1. Unless `--force`, require typed confirmation (`argocd`); a mistyped confirmation exits
   non-zero without deleting anything.
2. `kubectl delete -n argocd -f "$MANIFEST_URL" --ignore-not-found` — plain `delete -f`, no
   `--server-side --force-conflicts`: those are `kubectl apply`-only flags and `kubectl delete`
   rejects them (`unknown flag`) (review finding R1). Deletion enumerates object references from
   the manifest itself, so it works regardless of whether the resources were applied server-side.
3. `kubectl delete namespace argocd --ignore-not-found`. Warn on stuck finalizers with a hint to
   inspect `kubectl get ns argocd -o yaml`, and extend the hint to the cluster-scoped CRDs
   deleted in step 2 (`applications/applicationsets/appprojects.argoproj.io`): if any
   Applications/ApplicationSets still exist, CRD deletion can hang on finalizers too
   (`kubectl get crd <name> -o yaml`). Acceptable on this single-purpose cluster.
4. *(Agent-only, not script logic:)* record the removal in the journal CHANGELOG and update the
   k3s-cluster-admin skill inventory — see Context / environment facts.

## Implementation steps (ordered)

1. Create `scripts/argocd.sh` per the spec above, mirroring structure/helpers of
   `scripts/run-tests.sh` and `scripts/setup-vcpkg-cache.sh` (bash header, `set -euo pipefail`,
   colored helpers, flag parsing, `command -v` checks); `chmod +x`.
2. `scripts/README.md`: **skip** (condition resolved, review finding R8) — that file documents
   only the test-automation shell scripts (`run-tests.sh`, `coverage.sh`, `setup-linux.sh`);
   the cluster/infra scripts are not documented there, so adding `argocd.sh` would be the odd
   one out. The cluster documentation home is the k3s-cluster-admin skill.
3. After a successful install, update the k3s-cluster-admin skill's
   `references/cluster-inventory.md` (daemon-tier chore, not script logic).

## Validation plan (run on the VM as `linux-admin-agent`, from repo root)

```bash
# Cluster preflight (k3s-cluster-admin skill health script; or plain `kubectl get nodes`)
bash ~/.hermes/profiles/linux-admin-agent-vm/skills/devops/k3s-cluster-admin/scripts/health.sh

bash scripts/argocd.sh status                # "not installed" before install, exit 0
bash scripts/argocd.sh validate              # not installed -> clean one-line failure, exit 1
bash scripts/argocd.sh install --wait        # applies manifests, waits for rollouts, top-node snapshot
bash scripts/argocd.sh status                # pods ready, version, secret present
bash scripts/argocd.sh validate              # exit 0, healthz 200 over port-forward
bash scripts/argocd.sh install --wait        # re-run: idempotent, no errors (server-side apply)
bash scripts/argocd.sh uninstall             # typed confirmation; namespace removed
bash scripts/argocd.sh status                # back to "not installed"
```

Additional coverage:

```bash
# Wait opt-out: install without --wait returns promptly after apply (no rollout wait)
bash scripts/argocd.sh install

# Pinned install / upgrade (the docs' upgrade mechanism = re-install with a different version)
bash scripts/argocd.sh install --version v3.5.2 --wait   # pinned tag; status shows v3.5.2 image tag
bash scripts/argocd.sh install --version 3.5.2 --wait    # missing leading 'v' is normalized, same manifest
bash scripts/argocd.sh install --wait                    # back to stable branch manifests

# Uninstall variants
bash scripts/argocd.sh uninstall             # mistyped confirmation -> non-zero exit, nothing deleted
bash scripts/argocd.sh uninstall --force     # skips confirmation

# Validate failure paths
bash scripts/argocd.sh validate --health-check-port 8080  # 8080 is busy on this VM -> preflight fails fast, exit 1
kubectl -n argocd scale deploy/argocd-server --replicas=0; bash scripts/argocd.sh validate  # induced unhealthy -> exit 1
kubectl -n argocd scale deploy/argocd-server --replicas=1                                   # restore
```

Manual smoke test of UI access after install:
inside the VM `kubectl port-forward svc/argocd-server -n argocd 8443:443`, then from the
hypervisor host tunnel through SSH (`ssh -N -L 8443:127.0.0.1:8443 debian13vm`) and browse
`https://localhost:8443` (accept self-signed cert), login `admin` + secret password.

## Risks / failure modes

- **Port-forward probe flakiness**: `kubectl port-forward` can race the API; mitigate with
  retry/poll loop (30s) and guaranteed process cleanup via `trap ... EXIT`.
- **`stable` branch drift**: unpinned manifests can change under us; `--version` pin is
  available and `install` prints the resolved manifest URL.
- **Image pulls**: first install pulls ~7 images; on slow links the default 600s wait may need
  a bigger `--wait-timeout`.
- **Local port contention** during validate: hard preflight (validate step 5) checks the port
  via `ss -tln` and fails fast pointing at `--health-check-port`. Default moved from 8080 to
  **8443** because 8080 is busy on this VM (along with 8082, 6333, 6334, 9119, 9222, …).
- **Memory footprint**: the non-HA install adds 7 pods (~1–1.5 GiB RSS total) next to
  kube-prometheus-stack, trivy-operator, and the OdbDesign server on the single-node VM;
  verified headroom 2026-08-31: 16 GiB capacity, 41 % used. Install prints a `kubectl top node`
  snapshot after the rollout wait as a sanity check.
- **No jq on the VM**: all JSON extraction uses `kubectl -o jsonpath`; do not introduce a jq
  dependency.
- **Workload-set drift vs. assertions**: expected-workload list was verified at v3.5.1/v3.5.2;
  the enumeration fallback (validate step 2) degrades kind/name drift to warnings.
- **sudo/k3s not involved**: script needs no sudo; only user kubeconfig access.

## Out of scope (follow-ups)

- Traefik Ingress route for the Argo CD UI (needs `ssl-passthrough` annotation or cert
  replacement per the Argo CD ingress docs) — ServiceLB on 443/80 conflicts with Traefik, so no
  LoadBalancer patch by default.
- Installing the `argocd` CLI binary.
- Installing pwsh on the VM to keep the old `.ps1` deliverable (bash covers the need).
- Creating/syncing Argo CD Applications (guestbook example from the docs).
- HA manifests variant and external cluster registration.
