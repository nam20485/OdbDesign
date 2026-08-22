# Review findings: `docs/plan/argocd-install-plan.md`

Line references to `plan` are to this canonical plan file. Record your verdict and feedback inline on each item (`OPEN` -> `APPLIED` / `ACCEPTED` / `REJECTED` / `MODIFIED` / `DEFERRED`).

Review performed 2026-08-19 against: current `stable` manifest (`argo-cd/stable/manifests/install.yaml`, resolves to **v3.5.1**, 80 objects), the official getting-started docs, and the sibling scripts `scripts/k3s-cluster.ps1` / `scripts/deploy-monitoring.ps1`.

| ID | Severity | Finding | Verdict |
|----|----------|---------|---------|
| R1 | Blocker | Uninstall command uses apply-only flags (`--server-side --force-conflicts`) on `kubectl delete` -> fails with "unknown flag" | APPLIED |
| R2 | Should-fix (factual) | Validate expects `argocd-redis` as a StatefulSet; in current stable it is a Deployment (only `argocd-application-controller` is a StatefulSet) | APPLIED |
| R3 | Should-fix | `[switch]$Wait = $true` default-on switch is awkward and deviates from the repo's opt-in `-Wait` convention | APPLIED |
| R4 | Should-fix | `-Version` without leading `v` yields a raw.githubusercontent 404 mid-install; no format validation/normalization | APPLIED |
| R5 | Should-fix | Validate behavior when Argo CD is not installed is unspecified (StrictMode + EAP=Stop would stack-trace) | APPLIED |
| R6 | Should-fix | Health-check port pre-check left as either/or in Risks; should be a hard preflight reusing existing helpers | APPLIED |
| R7 | Should-fix | `argocd-initial-admin-secret` check is "informational failure" — ambiguous vs. "exit 1 if any check failed" | APPLIED |
| R8 | Minor | Implementation step 2 (scripts/README.md): plan's own condition resolves to *skip* — README documents only test-automation shell scripts | APPLIED |
| R9 | Minor | Validate probe implementation details unspecified (output redirection, per-attempt timeout, exception-as-retry) | APPLIED |
| R10 | Minor | No memory-headroom risk entry (7 pods, ~1–1.5 GiB RSS next to monitoring stack + OdbDesign server on a single-node VM) | APPLIED |
| R11 | Minor | `base64 -d` retrieval one-liner is bash-flavored; script is pwsh | APPLIED |
| R12 | Should-fix | Validation plan gaps: no `-Version` pin/upgrade path, no `-Force`/wrong-confirmation uninstall, no Validate-when-absent or port-busy cases | APPLIED |

---

## Blocking issues

### R1. Uninstall uses apply-only flags on `kubectl delete` (plan line 98)
- **Where:** plan line 98 — `kubectl delete -n argocd --server-side --force-conflicts -f <manifest URL> --ignore-not-found`.
- **Finding:** `--server-side` and `--force-conflicts` are `kubectl apply` flags only; `kubectl delete` rejects them (`unknown flag: --server-side`). The Uninstall action as specified dies on its first command. Plain `delete -f` is sufficient anyway: deletion enumerates object references from the manifest itself, so it works regardless of whether the resources were applied server-side (no `last-applied-configuration` annotation involved).
- **Proposed fix:** `kubectl delete -n argocd -f <manifest URL> --ignore-not-found`, then delete the namespace as already planned. Additionally, the manifest's CRDs (`applications/applicationsets/appprojects.argoproj.io`) are cluster-scoped and get deleted via the manifest — if any Applications/ApplicationSets exist, CRD deletion can hang on finalizers; extend the existing stuck-finalizer hint (plan line 99) to cover CRDs, or note that on this single-purpose cluster this is acceptable.
- Verdict: APPLIED — Feedback: _

---

## Correctness / robustness

### R2. `argocd-redis` is a Deployment, not a StatefulSet (plan line 85)
- **Where:** plan line 85 — "… statefulset `argocd-redis`".
- **Finding:** Verified against the current `stable` install.yaml (v3.5.1): the non-HA manifest contains exactly one StatefulSet (`argocd-application-controller`) and six Deployments — `argocd-server`, `argocd-repo-server`, `argocd-redis`, `argocd-dex-server`, `argocd-notifications-controller`, `argocd-applicationset-controller`. A validator asserting `sts/argocd-redis` false-fails on a healthy install. Also, the plan's core-assert list omits `argocd-dex-server`, which is part of the default manifest.
- **Proposed fix:** Expected workloads: Deployments {argocd-server, argocd-repo-server, argocd-redis, argocd-dex-server, argocd-notifications-controller, argocd-applicationset-controller} + StatefulSet {argocd-application-controller}; keep the `kubectl get deploy,sts -n argocd` enumeration as the source of truth so a manifest change (e.g. a future redis kind flip) degrades to a warning rather than a hard mismatch.
- Verdict: APPLIED — Feedback: _

### R3. Default-on `[switch]$Wait = $true` (plan lines 50–51)
- **Where:** plan param block.
- **Finding:** With a defaulted-on switch, `-Wait` on the command line is a no-op and opting out requires the non-obvious `-Wait:$false`. `deploy-monitoring.ps1:2` uses opt-in `[switch]$Wait`.
- **Proposed fix:** Either match the repo convention (opt-in `-Wait`) or invert the name (`-NoWait`) so the default-on behavior has a sane off-switch. Document whichever is chosen in the usage comment.
- Verdict: APPLIED — Feedback: _

### R4. `-Version` format not validated/normalized (plan lines 48, 64)
- **Where:** manifest URL construction — `https://raw.githubusercontent.com/argoproj/argo-cd/{stable|$Version}/manifests/install.yaml`.
- **Finding:** GitHub tags need the leading `v` (`v3.2.0`). A user passing `-Version 3.2.0` gets an HTTP 404 from raw.githubusercontent and a kubectl error that doesn't point at the cause. (Reference point: `stable` currently resolves to v3.5.1, so the plan's `v3.2.0` example is an older-but-valid tag.)
- **Proposed fix:** Normalize (`if ($Version -and $Version -notmatch '^v') { $Version = "v$Version" }`) plus a `ValidatePattern('v?\d+\.\d+\.\d+')`, and keep printing the resolved URL (already planned — good).
- Verdict: APPLIED — Feedback: _

### R5. Validate behavior when Argo CD absent is unspecified (plan lines 82–93)
- **Where:** `-Action Validate` spec.
- **Finding:** `Status` explicitly defines the not-installed path (plan lines 73–74), Validate doesn't. Under `Set-StrictMode -Version Latest; $ErrorActionPreference = "Stop"`, a naive implementation stack-traces on the first kubectl call.
- **Proposed fix:** First Validate step: if namespace missing, print "Argo CD is not installed (run: pwsh scripts/argocd.ps1 -Action Install)" and `exit 1` (Validate is a health check; absence is failure, but it should be a clean single-line failure).
- Verdict: APPLIED — Feedback: _

### R6. Make the health-check port pre-check a hard preflight (plan line 128)
- **Where:** Risks — "pre-check the port (`ss -tln`) and fail fast …, or accept `-HealthCheckPort` override".
- **Finding:** The either/or leaves the behavior to the implementer; without the pre-check, a busy local port surfaces as a confusing 30 s probe-timeout failure.
- **Proposed fix:** Make it a required preflight in Validate: reuse the `Get-ListeningPorts` / `Test-TcpPortReachable` helpers from `k3s-cluster.ps1:38-74` and fail fast with a message pointing at `-HealthCheckPort`. Keep `-HealthCheckPort` as the escape hatch.
- Verdict: APPLIED — Feedback: _

### R7. Soft vs. hard failure for the admin-secret check (plan lines 92–93)
- **Where:** Validate step 6 ("informational failure if absent") vs. step 7 ("exit 1 if any check failed").
- **Finding:** Contradiction: is a missing `argocd-initial-admin-secret` a PASS-with-warning (workloads healthy, only admin login blocked) or a FAIL?
- **Proposed fix:** Recommend warning-only (exit 0) — the docs note the secret is legitimately deleted after the first password change (plan itself reminds the user to delete it, plan line 80), so treating its absence as failure would break Validate on a correctly-hardened install. State the choice explicitly in the spec.
- Verdict: APPLIED — Feedback: _

---

## Minor / notes

### R8. Implementation step 2 resolves to "skip" (plan line 104)
- **Finding:** `scripts/README.md` documents only the test-automation shell scripts (`run-tests.sh`, `coverage.sh`, `setup-linux.sh`); neither `k3s-cluster.ps1` nor `deploy-monitoring.ps1` is documented there. Per the plan's own condition ("if that file documents sibling scripts"), the answer is no — adding argocd.ps1 there would be the odd one out.
- **Proposed fix:** Resolve the conditional explicitly in the plan: skip `scripts/README.md`; the cluster-script documentation home is the k3s-admin skill (out of scope per plan line 105). Update the skill later if desired.
- Verdict: APPLIED — Feedback: _

### R9. Validate probe implementation details (plan lines 88–91)
- **Finding:** Three unspecified details that commonly bite: (1) `Start-Process -PassThru kubectl port-forward` without output redirection can interleave/block — redirect stdout/stderr to temp files; (2) each `Invoke-WebRequest` attempt needs `-TimeoutSec` (default can hang far beyond the 30 s budget); (3) non-2xx throws by default — catch and retry, or use `-SkipHttpErrorCheck` (PS 7+) and test the status code.
- **Proposed fix:** Spell these out in the spec so the implementer doesn't rediscover them.
- Verdict: APPLIED — Feedback: _

### R10. No memory-headroom risk entry (plan line 127)
- **Finding:** Risks cover image-pull time but not footprint: the non-HA install adds 7 pods (~1–1.5 GiB RSS total) alongside kube-prometheus-stack, trivy-operator, and the OdbDesign server on a single-node VM whose sizing isn't documented anywhere in the repo.
- **Proposed fix:** Add a risk line (and optionally a `kubectl top node` sanity check after rollout wait); sufficient for a dev cluster, but worth recording.
- Verdict: APPLIED — Feedback: _

### R11. `base64 -d` one-liner is bash-flavored (plan lines 80, 71)
- **Finding:** `kubectl … -o jsonpath="{.data.password}" | base64 -d` works in pwsh on Linux only via native-command piping; since the script itself is pwsh, the pwsh-native form is more robust when printed as a hint:
  `[Text.Encoding]::UTF8.GetString([Convert]::FromBase64String((kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath='{.data.password}')))`.
- **Proposed fix:** Either form is acceptable for a printed hint; prefer the pwsh-native one for consistency with the script's shell.
- Verdict: APPLIED — Feedback: _

---

## Validation gaps (plan lines 107–118)

### R12. Untested paths
- The happy path (Install → Status → Validate → idempotent re-Install → Uninstall) is covered, plus Status-when-absent. Not covered:
  - **Pinned install / upgrade:** `-Version v3.5.1` install, Status shows the pinned tag, then re-Install with a different `-Version` (this is the docs' upgrade mechanism — exactly what re-runs are for).
  - **Uninstall variants:** `-Force` skip-confirmation and a mistyped confirmation (must exit non-zero without deleting).
  - **Validate failure paths:** Validate when not installed (per R5), Validate with the health-check port busy (per R6), and at least one induced unhealthy state (e.g. scaled-down deployment) to prove the exit code is actually 1.
  - **`-Wait` opt-out** (per R3) returns promptly.
- **Proposed fix:** Add these as explicit lines in the validation plan; most are cheap one-liners.
- Verdict: APPLIED — Feedback: _

---

## Verified accurate (no change needed)

Recorded so the implementer doesn't re-litigate:

- **Install command matches the official getting-started guide exactly**, including `--server-side --force-conflicts` and the docs' own rationale (ApplicationSet CRD exceeds the 262 KB client-side-apply annotation limit) and the pinned-version recommendation (plan lines 19–25, 31).
- **Cross-references are accurate:** `scripts/k3s-cluster.ps1:36` is the `KUBECONFIG` pin; `scripts/deploy-monitoring.ps1:73` is the idempotent namespace-create pipe; `Wait-RolloutInNamespace` is at `deploy-monitoring.ps1:54`; `Test-Command` at `deploy-monitoring.ps1:11`.
- **Exposure decision is sound:** the manifest ships `argocd-server` as ClusterIP; port-forward `8080:443` is the docs' listed access method, and the LoadBalancer-patch avoidance rationale (Traefik holds 80/443 hostPorts on k3s; ServiceLB would conflict) is correct.
- **Admin-secret semantics match the docs:** auto-generated, stored in `argocd-initial-admin-secret`, should be deleted after the first password change, recreated on demand (which also informs R7).
- **`-SkipCertificateCheck` requires pwsh 7** — consistent with the stated VM environment (pwsh 7.x).
- Manifest scale sanity check: 80 objects, ~34 k lines — the server-side-apply requirement is current, and the idempotent re-run claim holds for SSA.
