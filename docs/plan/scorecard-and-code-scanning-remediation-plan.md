# Scorecard & Code-Scanning Remediation Plan

**Status:** Executed on branch `nam/scorecard-hardening` (PR → `nam20485`).
**Baseline:** OpenSSF Scorecard **6.4/10** (2026-11-21, development@9cf7302); **157 open code-scanning alerts** (2026-11-21; 128 CVE + 15 TokenPermissions + 7 PinnedDependencies + 1 DangerousWorkflow + 6 file-less check alerts = 157).
**Methodology:** fixes ranked by aggregate-score gain ÷ effort, same risk²/effort spirit as
[`docs/plan/comprehensive-security-audit/remediation-plan.md`](../comprehensive-security-audit/remediation-plan.md)
(uncommitted sibling doc). Aggregate math: checks weight by risk (Critical=10, High=7.5,
Medium=5, Low=2.5); current published checks total 105 weight points.

## 1. Alert taxonomy (what the 157 actually are)

| Class | Count | Source |
|---|---:|---|
| `CVE-…` alerts | 128 | Docker Scout SARIF (docker-scout-scan.yml) — OS-package CVEs **inside the published Docker image** (stale `debian:trixie-slim@sha256:d7e12182…` = 2026-08-24 snapshot); locations are container paths like `/usr/share/doc/libssl3t64/copyright` |
| `TokenPermissionsID` | 15 | scorecard-action SARIF — 5 top-level write scopes (instant −10 each), 1 missing top-level block, 9 job-level writes |
| `PinnedDependenciesID` | 7 | scorecard-action SARIF — 5 unpinned `actions/cache/*@v4` refs, 1 unpinned `curl\|bash` (Codacy get.sh), 1 unpinned `pip3 install` (setup-linux.sh) |
| `DangerousWorkflowID` | 1 | scorecard-action SARIF — `docker-publish.yml` checks out `github.event.workflow_run.head_sha` directly (untrusted-code-checkout pattern) |
| file-less check alerts | 6 | Branch-Protection 8, CI-Tests 9, SAST 9, Fuzzing 0, Code-Review 0, CII 2 |

The 5 `cpp/path-injection` alerts (#4239–4243, `scripts/eval_manifest.py`) were already fixed
by PR #648 (merged 3d6ac90) and are closed. Secret-scanning: 0 open alerts. Dependabot:
0 open alerts (#607 vcpkg action bump is a PR, not an alert).

## 2. Fixes applied (ranked by gain ÷ effort)

| # | Fix | Check (Δ aggregate) | Files |
|---|---|---|---|
| 1 | Stop interpolating `github.event.client_payload.*` / untrusted refs into `run:`; env-var indirection everywhere | Dangerous-Workflow 0→10 (**+0.95**, Critical) | `docker-publish.yml` (validated env-mediated ref resolution, fail-closed same-repo push/dispatch guard), `create-release.yml` (dispatch gate, release version, github-script args) |
| 2 | Top-level `permissions: read-all` (or `contents: read`) on every workflow; write scopes moved to the jobs that need them; vestigial scopes dropped (`code-coverage` packages, `dependency-review` security-events/id-token, `sbom` id-token); the cmake build job's `contents: write` is deliberately **retained** — vcpkg `dependencygraph` submission requires the Dependency Submission API (job-scoped alongside `packages: write` and `checks: write`); `prebuild.yml` **deleted** (its `if: github.repository == 'nam20485/agent-instructions'` guard makes it a permanent no-op in this repo) | Token-Permissions 0→10 (**+0.71**, High) | cmake-multi-platform, code-coverage, vcpkg-cache-warm, swagger-spec-configmap-sync, dependency-review, sbom-generate-submit, jekyll-gh-pages, docker-publish, create-release |
| 3 | Pin `actions/cache/restore|save` ×5 to v5.0.5 SHA (the version the warm lane already runs, shares keys); Codacy `get.sh` via raw.githubusercontent URL pinned to full commit SHA (the only URL form Scorecard accepts for download-then-run); replace EOL unpinned `pip3 install docker-compose` with Docker's GPG-checked Fedora repo (`docker-compose-plugin`) | Pinned-Dependencies 9→10 (**+0.05**, Medium) | cmake-multi-platform.yml, code-coverage.yml, scripts/setup-linux.sh |
| 4 | `actions/attest-build-provenance` (v3.2.0) on the three release zips; bundle uploaded as `OdbDesign-binaries.intoto.jsonl` release asset | Signed-Releases 8→10 (**+0.14** at next release, High) | create-release.yml, scripts/create-release.js |
| 5 | All 128 CVE alerts: base-image digest bump `d7e12182…`→`a72b4433…` (2026-11-04 snapshot) ×3 FROMs + `apt-get -y upgrade` in all 3 apt chains; Docker Scout SARIF upload now `only-fixed: true` (unfixable no-DSA CVEs are non-actionable noise; still visible in Scout dashboard/compare) | all 128 CVE alerts close after next image publish + scan | Dockerfile, Dockerfile.prebuilt, docker-scout-scan.yml |

**Projected aggregate: 6.4 → ~8.2 now, ~8.35 at next signed release.**

## 3. Deliberately not pursued (documented trade-offs)

| Item | Reason |
|---|---|
| CII Best-Practices "passing" | User decision 2026-11-21: first tier alone is weeks of work; not this round (score +0.19 at Low weight anyway) |
| Fuzzing (OSS-Fuzz/ClusterFuzz) | Scorecard only credits OSS-Fuzz integration — 1–2+ weeks; aligns with audit F027; deferred as its own initiative |
| Code-Review 0→ | Requires non-author approvals; solo-maintainer structural limit (1/11 approved changesets) |
| Branch-Protection 8→10 "maximal" | Maximal requirements (dismiss-stale-reviews, last-push-approval) were deliberately removed 2026-11-01 as unsatisfiable for a solo maintainer — do not reintroduce |
| SAST 9→10 | Self-healing: default-setup CodeQL analyzes default-branch pushes + PRs (27/30 covered and climbing); no workflow change needed |
| CI-Tests 9→10 | Self-healing as CI-covered PRs merge (10/11) |
| Contributors 6 | Structural (distinct contributing orgs) |

## 4. Alert lifecycle after merge

- **Auto-close** once scorecard-action/scout re-run on `development`: all top-level TP findings,
  the deleted prebuild.yml's 2, all 7 PinnedDependencies findings, the DangerousWorkflow finding,
  and every `only-fixed`-filterable / digest-bump-fixable CVE (of the 128).
- **Dismissed with justification** (sticky; re-dismissal not needed): the ~10 job-level write
  findings that are *inherent* to what each job does (feed pushes, image publish + cosign,
  release creation, dependency submission, test-reporter check runs) — least-privilege scopes
  on declared-top-level-read workflows are the recommended pattern; and the 4 structural
  check alerts (Code-Review, Branch-Protection, CII, Fuzzing).
- **Left open to self-heal**: SASTID, CITestsID.

## 5. Verification (real checks, no grep-style)

1. Every edited workflow YAML-parses with expected shape; `bash -n setup-linux.sh`; `node --check create-release.js`; pinned Codacy URL fetched and serves the real uploader.
2. Local Scorecard run (docker, against the pushed branch head) asserting Dangerous-Workflow=10, Token-Permissions=10, Pinned-Dependencies=10 before merge.
3. CI green on the PR (workflows parse at the GitHub level; the changed release pipeline is exercised at the next release; Docker build at next publish).
4. Post-merge on `development`: published Scorecard score rises; open-alert count drops to the dismissal list + 2 self-healing; CVE alerts drop after next Docker publish + Sunday scan.
