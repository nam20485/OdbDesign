# Review findings: `docs/plan/k3s-vm-cluster-plan.md`

Line references to `plan` are to this canonical plan file. Record your verdict and feedback inline on each item (`OPEN` -> `ACCEPTED` / `REJECTED` / `MODIFIED` / `DEFERRED`).

All verdicts recorded (2026-08-19) and applied to `docs/plan/k3s-vm-cluster-plan.md`. Note: R6 had no explicit inline feedback but was applied with the rest of the correctness/robustness section.

| ID | Severity | Finding | Verdict |
|----|----------|---------|---------|
| D1 | — (applied) | `docs/plan/k3s-vm-cluster-plan.md` is the canonical copy; `.kilo/plans/1786784477051-k3s-vm-cluster-plan.md` deleted | DONE |
| D2 | — (applied) | Skill location changed to client-neutral `.agents/skills/k3s-admin/SKILL.md` (plan updated) | DONE (caveat in R20) |
| R1 | Blocker | No task to decommission the existing k3d cluster -> guaranteed port conflicts | REJECTED (SKIP — k3d never installed on this VM) |
| R2 | Blocker | Install step creates wrong PV hostPath dir | REJECTED (SKIP — k3d leftover, untouched) |
| R3 | Blocker | Decision 3 (`http://<vm-ip>/`) contradicts the ingress manifest host rule | ACCEPTED |
| R4 | Blocker | "deploy.ps1 applies fine" is false on Linux pwsh; post-deploy validation is k3d-only | ACCEPTED |
| R5 | Blocker (security) | `--write-kubeconfig-mode 644` makes cluster-admin kubeconfig world-readable | ACCEPTED |
| R6 | Should-fix | `ReadyTimeoutSeconds` not applied to Install | ACCEPTED |
| R7 | Should-fix | Missing Install pre-flight checks (ports, kubeconfig backup, disk, curl/sudo) | ACCEPTED |
| R8 | Should-fix | `-Force` Install semantics undefined; tls-san changes after first start don't rotate cert | ACCEPTED |
| R9 | Should-fix | Status action throws when k3s inactive; brittle `ss | grep` filter | ACCEPTED |
| R10 | Should-fix | Restart (Stop-then-Start) races unit shutdown | ACCEPTED |
| R11 | Should-fix | Uninstall leaves user kubeconfig pointing at dead cluster; no Uninstall validation | ACCEPTED |
| R12 | Minor | kubectl note overstated — installer symlinks `/usr/local/bin/kubectl` by default | ACCEPTED |
| R13 | Minor | No k3s version pin; redundant `INSTALL_K3S_EXEC="server"` | ACCEPTED |
| R14 | Minor | Repeated sudo prompts; add upfront `sudo -v` | ACCEPTED |
| R15 | Should-fix | 8081 -> 80 port move breaks hard-coded URLs outside the plan's scope list | ACCEPTED (fixes moved in scope, plan task 4) |
| R16 | Should-fix | Validation ignores the tailnet IP (half the reason for the tls-san decision) | ACCEPTED |
| R17 | Should-fix | No validation for ingress routing or post-deploy gRPC | ACCEPTED |
| R18 | Minor | No validation for Restart, Uninstall, or kubeconfig-backup behavior | ACCEPTED |
| R19 | — (resolved) | Duplicate plan copies would drift | RESOLVED by D1 |
| R20 | — (resolved) | Kilo skill discovery vs `.agents/skills/` location | RESOLVED by D2, caveat below |
| R21 | Blocker (found during deploy) | Published ghcr image runtime base is Debian 12 (glibc 2.36) but binaries need GLIBC_2.38/GLIBCXX_3.4.32 -> CrashLoopBackOff; pinned digest `b6e2a152...` is bookworm, not trixie as commented | PARTLY FIXED — Dockerfiles repinned to real trixie digest; ghcr image needs CI rebuild; cluster currently runs local trixie image |
| R22 | Should-fix (found during deploy) | Deployment resources/rollout: 2-CPU requests × rolling surge deadlocked 4-core node; SwaggerUI over-requested 2 CPU/2Gi; `master`-label affinity (no-op on k3d) excluded the single control-plane node | ACCEPTED — fixed: Recreate strategy, requests lowered (1 CPU/1Gi server, 100m/128Mi swagger), stale affinity removed |

---

## Decisions already applied

### D1. Canonical plan location
- `docs/plan/k3s-vm-cluster-plan.md` is the single canonical copy.
- `.kilo/plans/1786784477051-k3s-vm-cluster-plan.md` has been deleted.

### D2. Skill location -> `.agents/skills/`
- Plan decision 6 and Task 2 updated to `.agents/skills/k3s-admin/SKILL.md`.
- **Caveat (R20):** Kilo auto-discovers skills in `.kilo/{skill,skills}/<name>/SKILL.md`, not `.agents/skills/`. Client-neutral is fine, but if you want Kilo to auto-load it, add a symlink `.kilo/skills/k3s-admin -> ../../.agents/skills/k3s-admin` (or reference it in config). Otherwise the skill is just a doc other agents/humans read manually.
- Verdict: DONE — Feedback: _

---

## Blocking issues

### R1. Missing task: decommission the existing k3d cluster
- **Where:** plan `Tasks` section (nothing between Install and the current state); only a passive risk note at plan line 92.
- **Finding:** The VM currently runs k3d, which publishes `0.0.0.0:6443` (API), `50051` (gRPC), and `8081` (ingress) on the VM host (`scripts/create-k3d-cluster.ps1:89-92`). k3s binds 6443 and ServiceLB binds 50051 -> hard conflicts. k3s will fail to start, or klipper-lb/traefik pods will crash-loop. k3d containers use Docker restart policies, so the conflict **recurs on every VM reboot** even if stopped once.
- **Proposed fix:** Add a pre-install task (script action or documented step): `k3d cluster delete <name>` (or `docker rm -f` of `k3d-*` containers + volumes), then verify `ss -tlnp` shows 6443/50051/8081 free. Optionally `sudo systemctl disable --now docker` since nothing else needs it. Also clean stale `k3d-*` contexts from kubeconfigs (`kubectl config delete-context`), which ties into R11.
- Verdict: REJECTED — Feedback: SKIP- k3d is not installed on this VM- no k3d cluster has ever been created or started. (Plan context updated accordingly.)
### R2. Wrong PV hostPath in Install step
- **Where:** plan line 46 (`sudo mkdir -p /k3dvolume ... keeps working unchanged`).
- **Finding:** The manifest's hostPath is `/mnt/d/k3dvolume` (`deploy/kube/k3d-volume-pv.yaml:15`), not `/k3dvolume`. The Install step as written does nothing useful, and pods mounting the PV will fail with `CreateContainerConfigError`/runnable-on-node issues.
- **Proposed fix:** Either (a) `sudo mkdir -p /mnt/d/k3dvolume`, or (b) change the PV manifest path to something sane like `/srv/k3dvolume` (preferred — `/mnt/d` is a leftover Windows-drive path). Option (b) means the "no manifest changes" claim in decision 5 (plan line 15, about service-grpc.yaml — that part is correct) should not be conflated with the PV manifest.
- Verdict: REJECTED — Feedback: SKIP- why are we fixing the k3d script? (PV manifests left untouched; the mkdir step was dropped from Install and the k3d-leftover PV is listed as out of scope with the known consequence noted.)
### R3. Decision 3 ingress claim contradicts `local-ingress.yaml`
- **Where:** plan line 13 ("clients must use `http://<vm-ip>/`"), port map line 23, validation step 1.
- **Finding:** `deploy/kube/local-ingress.yaml:10` has a single rule with `host: precision5820` and no default backend. A request to `http://192.168.122.200/` (or the tailnet IP) without `Host: precision5820` gets a Traefik 404. `precision5820` also won't resolve from the tailnet unless a hosts entry / MagicDNS record exists.
- **Proposed fix (pick one):** (a) add the VM IPs (and/or a tailnet hostname) as additional rules / remove the host filter in `local-ingress.yaml`; (b) keep the host rule and document that clients must send `Host: precision5820` / use a hosts entry; (c) serve `precision5820` via `/etc/hosts` on clients. This is a functional regression if unhandled — decide explicitly.
- Verdict: ACCEPTED — Feedback: Apply (fix (a): plan task 4 removes the `host: precision5820` rule from `local-ingress.yaml`).

### R4. "deploy.ps1 applies fine" (plan line 66) is false — twice
- **Where:** plan line 66 (skill content) and plan line 77 (out-of-scope framing).
- **Finding:**
  1. `scripts/deploy.ps1:31,63` build paths with `"$PSScriptRoot\odbdesign-server-request-secret.ps1"` and `"$PSScriptRoot\validate-grpc-exposure.ps1"` — backslash joins are broken on Linux pwsh. deploy.ps1 fails in the VM before applying anything.
  2. `scripts/validate-grpc-exposure.ps1:118` requires `docker`, and lines 174-183 inspect the `k3d-<name>-serverlb` container via `docker port` — meaningless on k3s; the post-deploy validation always fails.
- **Proposed fix:** Skill must instruct: `pwsh scripts/deploy.ps1 -ClusterName default -SkipGrpcValidation` (the `-ClusterName` param already handles the context — no need to "skip a line"), then manual `grpcurl -plaintext <vm-ip>:50051 list` as the gRPC check. Optionally fix the backslash joins with `Join-Path` so deploy.ps1 is path-portable (small, in-scope-adjacent fix).
- Verdict: ACCEPTED — Feedback: Apply (skill deploy instructions updated; `Join-Path` fix added as plan task 5).

### R5. Security: `--write-kubeconfig-mode 644` (plan line 42)
- **Finding:** Makes the cluster-admin kubeconfig world-readable at `/etc/rancher/k3s/k3s.yaml`. Unnecessary — Install already sudo-copies it to `~/.kube/config` with `chmod 600` (plan line 45).
- **Proposed fix:** Drop the flag (default is 600). If the user copy is the documented access path, there is no reason to loosen the root copy.
- Verdict: ACCEPTED — Feedback: Apply (flag dropped; default 600 kept).

---

## Correctness / robustness

### R6. `ReadyTimeoutSeconds` scope (plan line 37)
- Declared as "after start" but Install also waits for Ready (plan line 44). Apply the same timeout to both waits.

### R7. Install pre-flight checks missing
- Risks at plan lines 92-93 are noted but not handled as steps. Add to Install: port-conflict check (80/443/6443/50051 via `ss`), backup existing `~/.kube/config` (timestamped copy), disk space check, `curl`/`sudo` presence, and R1's k3d decommission.

- Apply
### R8. `-Force` semantics for Install undefined (plan line 41)
- "re-run installer is idempotent" only holds with identical args. If `TlsSans` differ, the unit file is rewritten but **cert SANs are baked at first start** — changing tls-san later requires cert rotation or reinstall. Define: `-Force` re-runs installer with same-or-warned-different args; document the cert-SAN caveat in the skill's troubleshooting section.APPLY
### R9. Status action brittleness (plan line 51)
- With `Set-StrictMode -Version Latest; $ErrorActionPreference = "Stop"` (plan line 31), `kubectl get nodes` throws when k3s is inactive, so `-Action Status` (the default!) crashes exactly when you need it most. Degrade gracefully: check `systemctl is-active` first and skip kubectl sections when inactive. Also replace the trailing-space `grep -E '6443|50051|:80 |:443 '` filter (brittle against `ss` output) with a PowerShell filter on the `ss` output lines.
 APPLY
### R10. Restart races shutdown (plan line 50)
- Stop-then-Start should wait for the unit to reach inactive before starting, or simply use `sudo systemctl restart k3s` + Ready poll.
APPLY
### R11. Uninstall cleanup incomplete (plan line 52)
- `k3s-uninstall.sh` runs the killall script (kills k3d/other container runtimes' processes if present — worth a warning). Afterwards `~/.kube/config` points at a dead cluster: restore from the R7 backup or remove it. No validation covers Uninstall or `-Force` (see R18).
 APPLY
### R12. kubectl availability note overstated (plan line 55)
- The k3s installer creates `/usr/local/bin/kubectl` symlink by default. Simplify: plain `kubectl` (with `~/.kube/config`) will exist post-install; the fallback chain is unnecessary complexity.
 APPLY
- Post-install note (2026-08-19): the symlink exists, but it is k3s itself and its wrapper forces `KUBECONFIG=/etc/rancher/k3s/k3s.yaml` (root-only, mode 600) unless `KUBECONFIG` is set. `k3s-cluster.ps1` pins `KUBECONFIG=~/.kube/config`; the skill documents the gotcha.
### R13. Version pinning / redundant flag (plan line 42)
- Add `INSTALL_K3S_VERSION=<pin>` (or documented channel) for reproducibility. `INSTALL_K3S_EXEC="server"` is redundant — server is the default single-node role; drop it. APPLY

### R14. Sudo UX
- Many separate `sudo` invocations across actions will each re-prompt. Cache credentials once with `sudo -v` at action start (and note the short sudo timeout when running unattended). APPLY
---

## Collateral of the 8081 -> 80 port move

### R15. Hard-coded `precision5820:8081` URLs outside the plan's scope list
- The plan's out-of-scope section (plan lines 75-79) doesn't cover these, and they break with decision 3:
  - `deploy/helm/values-prom.yaml` — Grafana `root_url`, Prometheus `externalUrl`/ingress hosts
  - `swagger/odbdesign-server-0.9-swagger.yaml:17` — server URL
  - `docs/research/API.md:117` — Local Network URL
  - `docs/monitoring-grafana-prometheus-trivy.md` — throughout (access table, firewall step, checklists)
- Proposed fix: at minimum, add an inventory/follow-up list to the plan or skill so these aren't discovered piecemeal later. Note the monitoring docs' whole access model (Traefik on 8081 for host `precision5820`) needs a rethink under k3s defaults.
WHY DONT WE ADD FIXING THESE TO THE PLAN? Why are they out of scope? Harcodingprecision5820 is patently incorrect- we arecreating the cluster on `debian13vm` now.
---

## Validation gaps (plan `Validation` section)

APPLY ALL

### R16. Tailnet IP untested
- Validation only exercises `192.168.122.200`. Add `curl -k https://100.118.225.119:6443/livez` and a grpcurl check against `100.118.225.119:50051` — the tailnet IP is half the justification for the tls-san decision (plan line 14).

### R17. No ingress routing or post-deploy gRPC validation
- Nothing verifies `http://192.168.122.200/` actually routes (would have caught R3) or that gRPC works end-to-end after deploy (relevant given R4 forces `-SkipGrpcValidation`). Add: `curl -s -o /dev/null -w '%{http_code}' http://192.168.122.200/` (expect 200, not 404) and a grpcurl health check.

### R18. No validation for Restart, Uninstall, `-Force`, or kubeconfig-backup behavior
- Validation covers Status/Stop/Start only. Add at least: Uninstall confirmation flow (in a throwaway install if needed) and kubeconfig backup/restore around Install/Uninstall.

---

## Process / resolved

### R19. Duplicate plan copies — RESOLVED by D1
- The plan existed in both `docs/plan/` and `.kilo/plans/1786784477051-...`; they would drift. `.kilo/plans` copy deleted; `docs/plan/` is canonical and now links to this review doc.

### R20. Kilo skill discovery vs `.agents/skills/` — RESOLVED by D2, with caveat
- Plan originally said `.kilo/skills/` per Kilo discovery. User chose client-neutral `.agents/skills/k3s-admin/SKILL.md`; plan updated. Caveat stands (see D2): Kilo will not auto-discover it there; symlink `.kilo/skills/k3s-admin` if auto-loading is wanted.
- Verdict: DONE — Feedback: _
