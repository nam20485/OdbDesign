# Argo CD deployment plan — OdbDesign services

> **Goal:** migrate `odbdesign-server` and `odbdesign-server-swaggerui` from
> manual `scripts/deploy.ps1` + mutable `:nam20485-latest` deploys to Argo CD
> GitOps on the debian13vm k3s cluster, and fix the CI trigger/gating debt that
> the migration surfaces (disabled CodeQL, dead required checks, push/PR run
> doubling).
> **Input:** [`argocd-gitops-handoff.md`](argocd-gitops-handoff.md) (platform
> handoff, 2026-09-08). This plan is the OdbDesign-side execution plan built on
> verified repo/cluster/CI facts; where it differs from the handoff, §3 says why.
> **Author:** ZCode agent, from live review of repo + cluster + ghcr + rulesets,
> 2026-09-08. Working branch: `nam/argocd` (contains `nam20485` tip).

---

## 1. TL;DR

- Argo CD v3.5.2 is live and the CLI is already logged in on this machine
  (context `debian13vm.tail11ba79.ts.net/argocd`); zero Applications and zero
  repo connections exist yet — clean slate.
- Deploy branch is **`nam20485`** (what the cluster already runs). Every
  Application pins `targetRevision: nam20485`.
- Deploys become **immutable `<branch>-<run_number>` tags committed to git by
  CI** (new `bump-manifest` job in `docker-publish.yml`). Argo CD auto-syncs
  within ≤3 min. The live images are digest-verified as
  `odbdesign:nam20485-1172` and `odbdesignserver-swaggerui:nam20485-21` —
  those are the initial pins.
- Four Applications: `odbdesign-server`, `odbdesign-swaggerui`,
  `odbdesign-shared` (ingress + PV/PVC), and `odbdesign-root` (app-of-apps so
  the Applications themselves are GitOps-managed).
- One-time repo reshuffle under `deploy/kube/` so each app owns a clean
  directory (handoff §7).
- The **`nam20485` branch ruleset requires signed commits and has no
  pull_request rule** — the bump job must commit through the GitHub Contents
  API (GitHub-signed), not `git push` (§3.3).
- **CI gating is repaired first** (Phase 0): re-enable CodeQL with the
  repo-proven concurrency dedup, add `paths-ignore` guards so deploy commits
  don't rebuild C++, and make the `nam20485` required checks satisfiable again
  so PRs gate pushes (user decision 2026-09-08).
- Two secrets stay out of git and out of Argo CD:
  `odbdesign-server-request-secret` (basic-auth, already live) and a possible
  future `ghcr-pull` (only if packages go private).

## 2. Verified current state (2026-09-08)

### 2.1 Cluster / Argo CD

| Item | Value |
|---|---|
| Node | `debian13vm`, k3s v1.36.3+k3s1, Ready, 4 vCPU / 15 GiB (~59% mem) |
| Argo CD | v3.5.2, ns `argocd`, all deployments ready; **CLI logged in** on this machine |
| Apps / repos in Argo CD | **none / none** (verified via `argocd app list`, `argocd repo list`) |
| Live workloads (`default` ns) | `odbdesign-server-v1` 1/1, `odbdesign-server-swaggerui-v1` 1/1, Recreate, PVC `k3d-volume-claim` → PV `k3d-volume` (hostPath `/srv/odbdesign-volume`, 1Ti, Retain, Bound) |
| Services | http ClusterIP :80 ×2, gRPC LoadBalancer :50051 (answers on every node IP) |
| Ingress | `odbdesign-server-ingress`, traefik, **hostless**, :80 only |
| Secrets | `odbdesign-server-request-secret` present (Opaque, 2 keys) |
| TLS platform | Traefik default `TLSStore` with Let's Encrypt `*.ts.net` cert — HTTPS routes get it automatically, no cert config |

### 2.2 Images (ghcr, digest-verified against what the cluster runs)

| Deployment | Running tag | Digest-equal immutable tag (initial pin) |
|---|---|---|
| `odbdesign-server-v1` | `odbdesign:nam20485-latest` | **`nam20485-1172`** |
| `odbdesign-server-swaggerui-v1` | `odbdesignserver-swaggerui:nam20485-latest` | **`nam20485-21`** |

ghcr tag listing paginates at 1000 — `?n=1000` alone is not the full list
(1426 tags total for `odbdesign`); follow the `Link: rel="next"` header when
auditing tags.

### 2.3 Repo / CI facts that shape the design

1. **`swagger-spec-configmap.yaml` is not stale**: regenerating it from
   `swagger/odbdesign-server-0.9-swagger.yaml`
   (`kubectl create configmap --dry-run=client -o yaml`) is **byte-identical**
   to the committed file (both 85,318 bytes). First Argo CD sync will be a
   near-no-op adoption.
2. **`codeql.yml` is disabled** (`.github/workflows/disabled/`) but the
   **`nam20485 branch` ruleset still requires its check contexts**
   (`Analyze (actions)`, `Analyze (c-cpp)`, `Analyze (javascript-typescript)`)
   → PRs into `nam20485` currently cannot satisfy required checks.
   User context: CodeQL was disabled because push + PR lanes doubled the long
   runs; the real fix (already proven in `cmake-multi-platform.yml` and
   `code-coverage.yml`) is a **concurrency group keyed on head repo + branch**
   — CodeQL just never got it.
3. **`nam20485 branch` ruleset** (active): rules = deletion, creation,
   required_signatures, non_fast_forward, required_status_checks (Codacy,
   CodeQL ×3, dependency-review), code_scanning, code_quality,
   copilot_code_review. **No `pull_request` rule → direct pushes are
   allowed**, which is how the bump job can commit, and why the
   `development ↔ nam20485` merges work today. Bypass: RepositoryRole(admin)
   only.
   **`development branch` ruleset** (active): has a `pull_request` rule
   (1 approval, last-push approval, thread resolution) + required checks that
   all actually run (CMake lanes, Codacy, SBOM, dependency-review) — healthy;
   do not use `development` as the GitOps deploy branch (the bump bot can
   never satisfy its review rules).
4. **`required_signatures` applies to bots** (GitHub rules docs: "contributors
   and bots can only push commits that have been signed and verified"; no
   `GITHUB_TOKEN` exemption is documented). Commits created via the GitHub
   **Contents API** are signed by GitHub → verified → pass.
5. **Repo is public** (`visibility: PUBLIC`) → Argo CD can connect
   anonymously; no PAT/deploy key needed while it stays public.
6. **`create-release.yml` is triggered by `trigger_deploy_release_event`** —
   the same repository dispatch that `docker-publish.yml` fires on `release`.
   The handoff's "delete the dispatch" advice would break releases (§3.1).
7. Bump-commit loop exposure of active workflows (commit touches only
   `deploy/kube/…`):

   | Workflow | Trigger | Effect of a deploy commit | Action |
   |---|---|---|---|
   | `cmake-multi-platform.yml` | push + PR, no paths filter | **full CMake rebuild → docker-publish → new image → one redundant auto-deploy** (the bot's own bump commit cannot re-trigger: GITHUB_TOKEN pushes create no workflow runs — this is build waste, not a loop) | add `paths-ignore: [deploy/**, docs/**]` to push |
   | `code-coverage.yml` | push + PR, no paths filter | full coverage build per deploy | same `paths-ignore` |
   | `sbom-generate-submit.yml` | push incl. `nam20485` | cheap Syft run | same `paths-ignore` (optional) |
   | `docker-publish.yml` | workflow_run(CMake) | none (CMake skipped) | — |
   | `docker-scout-scan.yml` | workflow_run(Docker Publish), branches **exclude `nam20485`** | none — **deployed images are never Scout-scanned** | add `nam20485` to its branch filter |
   | `dependency-review.yml` | PR only | none | — |
   | `prebuild.yml` | push main/development | none | — |

## 3. Corrections to the handoff (with reasons)

1. **Keep the `Trigger Deploy and Release Workflows` step in
   `docker-publish.yml`.** `create-release.yml` consumes
   `trigger_deploy_release_event`; only the disabled
   `workflows/disabled/deploy-local-k8s.yml` / `deploy-eks.yml` consumers are
   dead. Delete those two files; keep the dispatch step.
2. **No repo credentials for Argo CD.** The repo is public; connect
   anonymously (`argocd repo add https://github.com/nam20485/OdbDesign`).
   If the repo ever goes private, add a fine-grained PAT (Contents: read) or
   deploy key then.
3. **Bump job commits via the GitHub Contents API, not `git push`.** The
   `nam20485` ruleset's `required_signatures` rule rejects unsigned bot
   commits pushed by git CLI; Contents-API commits are GitHub-signed and
   verified (§2.3.4). The handoff's `sed + git push` snippet would fail.
4. **`k3s-cluster.ps1` stays.** The handoff's retire list includes it, but the
   repo-local `.agents/skills/k3s-admin/SKILL.md` (cluster lifecycle runbook)
   depends on it. Retire list trimmed accordingly (§6.9).
5. **CI gating repair is part of this plan** (Phase 0) — the handoff assumes
   PRs into the deploy branch work; today they can't satisfy required checks
   (§2.3.2). Also matches the user's direction: PRs gate pushes via workflow
   run checks, with the push/PR doubling fixed by concurrency, not by
   disabling workflows.
6. **App-of-apps layout uses a subdir** (`deploy/kube/argocd/apps/`) so the
   root Application doesn't sit inside its own source path.

## 4. Decisions

| # | Decision | Rationale |
|---|---|---|
| D1 | Deploy branch / `targetRevision` = **`nam20485`** | Images in cluster are `nam20485-*`; no `pull_request` rule there (bump bot can push); `development`'s review rules would block automation. `nam20485 ↔ development` sync flow unchanged. |
| D2 | Immutable `<branch>-<run_number>` tags, CI commits the bump; stop deploying `-latest` | Handoff §6.2; `-latest` + `rollout restart` is invisible to git and fights `selfHeal`. |
| D3 | Four Applications (server, swaggerui, shared, root app-of-apps) | Handoff §6.3/§8.2 + self-managed apps. |
| D4 | Ingress: keep hostless HTTP rule, add `debian13vm.tail11ba79.ts.net` host rule + TLS | Handoff §6.4. Keeping the hostless rule preserves direct LAN-IP access on :80 (used today); the host rule gets the platform TLSStore cert on :443. |
| D5 | Committed `swagger-spec-configmap.yaml` is truth; new CI workflow regenerates + commits it when `swagger/**` changes | Handoff §6.5; verified currently byte-identical (§2.3.1). |
| D6 | SwaggerUI image bumped **manually** in git; no cross-repo PAT | That image changes ~never (spec updates flow via the ConfigMap from this repo). A PAT with contents:write on OdbDesign stored in the SwaggerUI repo is a new secret for no present value (user prefers minimal secrets). Revisit if SwaggerUI releases become frequent. |
| D7 | Sync policy: `automated {prune, selfHeal}`, `ServerSideApply=true`, `CreateNamespace=false`, **no `resources-finalizer` on any Application** | Prune/selfHeal per handoff; SSA avoids last-applied bloat on the 85 KB ConfigMap; no finalizers means deleting an Application orphans live objects instead of cascading — deliberate PV-safety choice (handoff §8.3). |
| D8 | CI gating restored in Phase 0 (CodeQL re-enabled with concurrency dedup; ruleset checks made satisfiable) | §2.3.2–3, user direction 2026-09-08. |
| D9 | Deploy commits carry `[skip ci]` **and** workflows get `paths-ignore` | Bot commits made with `GITHUB_TOKEN` never trigger workflow runs anyway; `[skip ci]` is defense-in-depth for human-pushed equivalents, `paths-ignore` guards human deploy/docs-only commits. |

## 5. Target flow

```text
push/PR merge → nam20485
  ├─ PR lane (gating): CMake + CodeQL + Codacy + dependency-review
  └─ push lane: CMake build → docker-publish.yml
        ├─ push ghcr.io/nam20485/odbdesign:nam20485-<run_number> (+ cosign)
        └─ bump-manifest job: Contents-API commit
           "deploy: odbdesign-server nam20485-<run_number> [skip ci]"
                ↓ (≤3 min poll; no webhook — API is tailnet-only)
           Argo CD odbdesign-server sync → Recreate rollout (brief downtime,
           single replica by design)
```

Swagger spec change → `swagger-spec-configmap-sync.yml` regenerates +
commits ConfigMap → Argo CD syncs `odbdesign-swaggerui` (no image change).
SwaggerUI image change (rare) → manual manifest edit + PR.
Everything else (secret rotation, Argo CD/TLS/monitoring) stays out of band
or with the platform owner.

## 6. File changes

### 6.1 Repo reshuffle (one-time, `git mv`)

```text
deploy/kube/
├── OdbDesignServer/                      # unchanged contents
├── OdbDesignServer-SwaggerUI/            # unchanged contents
├── shared/                               # NEW dir
│   ├── local-ingress.yaml                # moved from deploy/kube/
│   ├── k3d-volume-pv.yaml                # moved (matches live PV — adoption no-op)
│   └── k3d-volume-pvc.yaml               # moved
├── argocd/                               # NEW dir
│   ├── odbdesign-root.yaml               # root Application (§6.2)
│   └── apps/
│       ├── odbdesign-server.yaml
│       ├── odbdesign-swaggerui.yaml
│       └── odbdesign-shared.yaml
├── legacy-eks/                           # NEW dir (or delete outright)
│   ├── default-ingress (eks).yaml        # moved
│   └── issuer.yaml                       # moved
└── monitoring/
    └── traefik-middleware-prometheus-stripprefix.yaml   # moved; NOT Argo-managed
                                                          # (monitoring ns = platform scope)
```

`deploy/helm/*` (monitoring values) stays where it is — unmanaged by these
Applications.

### 6.2 Application manifests (`deploy/kube/argocd/`)

`apps/odbdesign-server.yaml` (swaggerui/shared differ only in name/path;
shared has the PV/PVC note):

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: odbdesign-server
  namespace: argocd        # Applications live in the argocd namespace
  # deliberately NO resources-finalizer: deleting this Application orphans
  # the live objects instead of cascading (PV safety, handoff §8.3)
spec:
  project: default         # dedicated 'odbdesign' AppProject available from
                           # the platform on request (handoff §11)
  source:
    repoURL: https://github.com/nam20485/OdbDesign
    targetRevision: nam20485
    path: deploy/kube/OdbDesignServer
  destination:
    server: https://kubernetes.default.svc
    namespace: default
  syncPolicy:
    automated:
      prune: true          # git deletions delete objects
      selfHeal: true       # manual kubectl edits get reverted — deploy.ps1 must go
    syncOptions:
      - CreateNamespace=false
      - ServerSideApply=true   # 85KB swagger ConfigMap; avoids last-applied bloat
    retry:
      limit: 5
      backoff:
        duration: 15s
        factor: 2
        maxDuration: 5m
```

`apps/odbdesign-shared.yaml`: same shape, `path: deploy/kube/shared`.
The shared app owns the ingress + PV + PVC; the PV manifest already matches
the live object (hostPath `/srv/odbdesign-volume`, `manual` storageClass,
Retain) — verified 2026-09-08.

`odbdesign-root.yaml` (app-of-apps):

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: odbdesign-root
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/nam20485/OdbDesign
    targetRevision: nam20485
    path: deploy/kube/argocd/apps
  destination:
    server: https://kubernetes.default.svc
    namespace: argocd
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=false
      - ServerSideApply=true
```

Bootstrap (one-time, from a tailnet machine):

```bash
argocd repo add https://github.com/nam20485/OdbDesign   # anonymous: repo is public
kubectl apply -f deploy/kube/argocd/odbdesign-root.yaml # root creates the 3 apps
```

### 6.3 Ingress (`deploy/kube/shared/local-ingress.yaml`, Phase 3)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: odbdesign-server-ingress
spec:
  ingressClassName: traefik
  tls:
    - hosts:
        - debian13vm.tail11ba79.ts.net   # no secretName: platform default TLSStore
  rules:
    # canonical HTTPS host on :443 with the Let's Encrypt ts.net cert
    - host: debian13vm.tail11ba79.ts.net
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: odbdesign-server-service
                port:
                  name: ods-svc-port
          - path: /swagger
            pathType: Prefix
            backend:
              service:
                name: odbdesign-server-swaggerui-service
                port:
                  name: oss-svc-port
    # hostless catch-all keeps direct LAN/tailnet-IP access on :80 working
    - http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: odbdesign-server-service
                port:
                  name: ods-svc-port
          - path: /swagger
            pathType: Prefix
            backend:
              service:
                name: odbdesign-server-swaggerui-service
                port:
                  name: oss-svc-port
```

Traefik's longer-path platform routes (`/argocd`, `/prometheus`, `/grafana`)
already outrank the `/` Prefix rule today; adding the host rule doesn't
change that.

### 6.4 `docker-publish.yml` — bump job (Phase 2)

New job after `build` (reuses the build's pinned checkout action SHA):

```yaml
  bump-manifest:
    name: Bump image tag in git (GitOps deploy trigger)
    needs: build
    if: >
      github.event_name == 'workflow_run' &&
      github.event.workflow_run.head_branch == 'nam20485'
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - name: Checkout deploy branch tip
        uses: actions/checkout@08c6903cd8c0fde910a37f88322edcfb5dd907a8 # v5.0.0
        with:
          # branch tip, NOT the build's head_sha: never clobber commits that
          # landed while the image was building (a mid-air collision fails the
          # contents PUT with 409 instead — visible, self-heals next build)
          ref: nam20485

      - name: Bump image tag and commit via GitHub API
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          TAG="nam20485-${{ github.run_number }}"   # exactly what this run pushed
          FILE="deploy/kube/OdbDesignServer/deployment.yaml"
          # Blob SHA of the COMMITTED file — capture it before sed rewrites
          # the file: the Contents API PUT replaces the blob this sha
          # identifies, and hashing the post-sed content would 409 every run.
          SHA="$(git rev-parse "HEAD:$FILE")"
          sed -i "s|image: ghcr.io/nam20485/odbdesign:.*|image: ghcr.io/nam20485/odbdesign:${TAG}|" "$FILE"
          # Commit through the Contents API, not git push: the nam20485
          # ruleset's required_signatures rule rejects unsigned bot commits,
          # and API-created commits are signed+verified by GitHub.
          gh api --method PUT "repos/${{ github.repository }}/contents/${FILE}" \
            -f message="deploy: odbdesign-server ${TAG} [skip ci]" \
            -f content="$(base64 -w0 < "$FILE")" \
            -f sha="$SHA" \
            -f branch=nam20485
```

Same PR also pins the current images in the manifests (digest-verified,
§2.2):

- `deploy/kube/OdbDesignServer/deployment.yaml`:
  `ghcr.io/nam20485/odbdesign:nam20485-1172`
- `deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml`:
  `ghcr.io/nam20485/odbdesignserver-swaggerui:nam20485-21` (manual-bump
  policy, D6)

Note: manual `workflow_dispatch` full builds do **not** deploy (the `if`
excludes them) — dispatch builds are for image experiments only.

### 6.5 New workflow `.github/workflows/swagger-spec-configmap-sync.yml` (Phase 2)

```yaml
name: Swagger spec ConfigMap sync
on:
  push:
    branches: [nam20485]
    paths: ["swagger/**"]
  workflow_dispatch:
permissions:
  contents: write
jobs:
  regen:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@08c6903cd8c0fde910a37f88322edcfb5dd907a8 # v5.0.0
        with:
          ref: nam20485
      - name: Commit if changed
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          FILE="deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml"
          # Blob SHA of the COMMITTED file — capture it before regenerating
          # rewrites the file (same 409 trap as §6.4 otherwise)
          SHA="$(git rev-parse "HEAD:$FILE")"
          kubectl create configmap odbdesign-server-swagger-spec \
            --from-file=odbdesign-server-0.9-swagger.yaml=swagger/odbdesign-server-0.9-swagger.yaml \
            --dry-run=client -o yaml > "$FILE"
          # diff regenerated vs committed: a fresh checkout is always clean,
          # so this check is only meaningful after the regeneration above
          git diff --quiet -- "$FILE" \
            && { echo "ConfigMap up to date"; exit 0; }
          gh api --method PUT "repos/${{ github.repository }}/contents/${FILE}" \
            -f message="chore(swagger): regenerate spec ConfigMap [skip ci]" \
            -f content="$(base64 -w0 < "$FILE")" \
            -f sha="$SHA" \
            -f branch=nam20485
```

Loop-safe twice over: the bot's commit is made with `GITHUB_TOKEN`, and
GitHub does not create workflow runs for pushes made with `GITHUB_TOKEN`;
the commit also touches only `deploy/`, outside this workflow's
`paths: ["swagger/**"]` filter. CMake's `paths-ignore` (§6.6) additionally
saves wasted builds on human deploy/docs-only commits.

### 6.6 Trigger hygiene (Phase 0)

- `cmake-multi-platform.yml`, `code-coverage.yml` (and optionally
  `sbom-generate-submit.yml`): add to the **push** trigger
    ```yaml
    paths-ignore: ["deploy/**", "docs/**"]
    ```
    PR-lane path-skips still satisfy required checks (GitHub marks the
    contexts skipped, which counts as passing for required checks).
- `docker-scout-scan.yml`: add `nam20485` to the `workflow_run.branches`
  filter so the images we actually deploy get scanned.

### 6.7 Re-enable CodeQL (Phase 0)

`git mv .github/workflows/disabled/codeql.yml .github/workflows/codeql.yml`
— the disabled file needs three fixes as part of the move:

1. **Structural:** its `steps:` block is mis-indented inside
   `strategy.matrix:` (a sibling of `include:`), leaving the `analyze` job
   with no job-level `steps` — GitHub rejects such a workflow ("Every job
   must define a `steps` or a `uses` key"), so the moved file would fail
   validation as-is. Re-indent `steps:` to job level.
2. **Check contexts:** the job is named
   `CodeQL-Analysis-${{ matrix.language }}` with a c-cpp-only matrix, so it
   can only ever report `CodeQL-Analysis-c-cpp` — none of the three contexts
   the `nam20485` ruleset requires. Rename the job to `Analyze` and extend
   the matrix to `[actions, c-cpp, javascript-typescript]` so it reports the
   exact required contexts `Analyze (actions)`, `Analyze (c-cpp)`,
   `Analyze (javascript-typescript)` — or trim the ruleset's required list
   instead (§6.8); as-is the file can satisfy neither.
3. **Concurrency dedup** — add the pattern already proven in
   `cmake-multi-platform.yml` / `code-coverage.yml`:

```yaml
concurrency:
  # same-repo PRs fire both the push lane (refs/heads/<branch>) and the PR
  # lane (refs/pull/<n>/merge) for one commit; keying on head repo + branch
  # puts both in one group so the duplicate is cancelled
  group: codeql-${{ github.event_name == 'pull_request' && format('{0}/{1}', github.event.pull_request.head.repo.full_name, github.head_ref) || format('{0}/{1}', github.repository, github.ref_name) }}
  cancel-in-progress: true
```

Its existing `paths:` filters already exclude `deploy/**` (bump commits never
trigger it).

### 6.8 Ruleset repair (Phase 0, admin one-liners or UI)

After CodeQL is re-enabled the `nam20485 branch` ruleset's existing contexts
become live again. Verify — and fix if contexts drifted:

```bash
gh api repos/nam20485/OdbDesign/rulesets --jq '.[] | select(.name=="nam20485 branch")'
# required_status_checks contexts must reference checks that actually run on
# PRs into nam20485: Analyze (actions|c-cpp|javascript-typescript), Codacy,
# dependency-review
```

Keep the `pull_request` rule **absent** from `nam20485` (the bump bot needs
direct push; gating happens on PRs, which the required checks now gate
properly). Do not add `development`-style review rules to `nam20485` without
redesigning the bump bot (PR + auto-merge cannot satisfy
`require_last_push_approval`).

### 6.9 Retirement & docs (Phase 4)

**Delete:** `scripts/deploy.ps1` (fights `selfHeal`), `scripts/deploy-monitoring.ps1`
(coordinate with platform first — monitoring is platform-owned per handoff §5),
`scripts/start-k3d-cluster.ps1`, `scripts/create-k3d-cluster.ps1`,
`scripts/register-k3d-startup-task.ps1` (k3d-era), and
`.github/workflows/disabled/deploy-eks.yml`,
`.github/workflows/disabled/deploy-local-k8s.yml`.

**Keep:** `scripts/odbdesign-server-request-secret.ps1` (out-of-band secret
rotation), `scripts/validate-grpc-exposure.ps1` (post-deploy check from a
tailnet machine), `scripts/k3s-cluster.ps1` (k3s-admin skill depends on it),
and the `Trigger Deploy and Release Workflows` dispatch step in
`docker-publish.yml` (`create-release.yml` consumes it — §3.1).

**Doc updates:** `.agents/skills/k3s-admin/SKILL.md` (deploy section → Argo
CD flow, port map gains the HTTPS host), `docs/monitoring-grafana-prometheus-trivy.md`
(references to `deploy-monitoring.ps1`), `AGENTS.md` deployment-tooling
section (deploy = git commit to `nam20485`; `deploy.ps1` gone; CLI/MCP rules
per §12).

## 7. Phased execution

Each phase lands as a PR into `nam20485` (`gh pr create --base nam20485 …`)
and is gated by the checks repaired in Phase 0. Verification commands use the
argocd CLI (already logged in on this machine); §12 documents it and the
argocd MCP server, including the read-vs-mutate rules for agents.

### Phase 0 — CI gating + trigger hygiene (prerequisite)

1. PR `nam/ci-gating` → `nam20485`: §6.6 paths-ignore changes, §6.7 CodeQL
   re-enable, docker-scout branch filter.
2. Verify/adjust the `nam20485` ruleset contexts (§6.8).
   Chicken-and-egg: this first PR's CodeQL checks can't have run before the
   workflow exists on the branch — merge it with the admin bypass (Repository
   admins bypass this ruleset), everything after is properly gated.
3. **Verify:** draft PR into `nam20485` from a throwaway branch shows all
   required contexts reporting (not "expected" forever); a commit touching
   only `deploy/**` on `nam20485` triggers **no** CMake/coverage run; a
   commit touching `OdbDesignLib/**` triggers exactly one CMake run per lane
   (push+PR collapse or supersede via concurrency).

### Phase 1 — Argo CD onboarding (adoption, zero behavior change)

1. PR `nam/argocd` → `nam20485`: §6.1 reshuffle + §6.2 Application manifests.
   Manifests still say `:nam20485-latest` here (identical to live → first
   sync is a no-op adoption).
2. From a tailnet machine: `argocd repo add …`, `kubectl apply -f
   deploy/kube/argocd/odbdesign-root.yaml`.
3. **Verify:**
   - `argocd app list` → 4 apps, all `Healthy/Synced`;
   - `argocd app diff odbdesign-server` (and each of the other two apps)
     → empty (adoption was a no-op);
   - `kubectl -n default get secret odbdesign-server-request-secret` intact;
   - `http://192.168.122.200/swagger/` loads; gRPC at
     `100.118.225.119:50051` answers (`scripts/validate-grpc-exposure.ps1`);
   - platform gate: `./scripts/argocd.ps1 -Action Validate`
     (linux-system-agent repo).

### Phase 2 — immutable tags + CI bump (the GitOps switch)

1. PR → `nam20485`: §6.4 bump job + image pins (`nam20485-1172`,
   `nam20485-21`) + §6.5 swagger-sync workflow.
2. The PR touches `.github/workflows/**` → CMake builds post-merge →
   docker-publish pushes a **new** tag → bump job commits it → first real
   auto-deploy (Recreate: brief downtime).
   **Caveat:** `workflow_run`-triggered workflows always execute the copy of
   the workflow file on the **default branch** (`development`), regardless of
   which branch was pushed — a `bump-manifest` job merged only into
   `nam20485` never runs, so images get pushed but no bump commit lands.
   Activation lands when the workflow change flows `nam20485 → development`
   (normal merge flow), or cherry-pick it to `development` to activate
   early. The bump commit itself still targets `nam20485`: the job checks
   out `ref: nam20485` and PUTs to that branch.
3. **Verify:** bump commit visible on `nam20485`; `argocd app get
   odbdesign-server` → Synced/Healthy; live image equals the git tag
   (`kubectl -n default get deploy odbdesign-server-v1 -o
   jsonpath='{.spec.template.spec.containers[0].image}'`); endpoints answer;
   a second push to `OdbDesignLib/**` repeats the cycle end-to-end.

### Phase 3 — ingress host + HTTPS

1. PR → `nam20485`: §6.3 ingress.
2. **Verify:** `https://debian13vm.tail11ba79.ts.net/swagger/` and `/` serve
   with a valid cert; `http://192.168.122.200/` still works;
   `https://…/argocd` unaffected.

### Phase 4 — retirement + docs

§6.9 deletions and doc updates; remove any lingering local habits
(`deploy.ps1` muscle memory). Final check: selfHeal demonstration —
`kubectl -n default scale deploy odbdesign-server-swaggerui-v1 --replicas=2`
reverts within one sync cycle (≤3 min), proving drift protection.

## 8. Out-of-band (never in git, never Argo-managed)

- `odbdesign-server-request-secret` — rotate with
  `scripts/odbdesign-server-request-secret.ps1` from a tailnet machine;
  Argo CD prune/selfHeal never touches untracked secrets.
- `ghcr-pull` docker-registry secret — create only if packages go private
  (handoff §9.4), plus `imagePullSecrets` in both deployments.
- Argo CD accounts/RBAC/AppProject, TLS, Traefik, monitoring → platform
  (linux-system-agent), on request/escalation only.

## 9. Rollback

| Situation | Action |
|---|---|
| Bad deploy (bad image) | `git revert <bump commit>` on `nam20485` → Argo CD re-syncs the previous immutable tag |
| Suspect sync loop / Argo incident | `argocd app set odbdesign-server --sync-policy none` (repeat per app) — automated sync off, live objects untouched |
| Full GitOps exit | `argocd app delete odbdesign-root` — with no finalizers, the Applications disappear and **all live objects are orphaned in place** (service keeps running); resume script-based deploys from a tag |
| PV caution | Never casually prune/delete `odbdesign-shared`: reclaim is `Retain` so `/srv/odbdesign-volume` data survives, but re-binding needs care (handoff §8.3) |

## 10. Risks

- **required_signatures vs bot push** — designed around via Contents-API
  commits; if the PUT is still rejected in Phase 2, fall back to a
  repo-local signing key or add GitHub Actions as a ruleset bypass
  Integration (platform/admin change). Watch the first bump job run.
- **409 races** on the bump commit (branch moved mid-build) — job fails
  loudly; next build self-heals. Rare on a single-maintainer branch.
- **Deploy latency** — Argo CD polls ≤3 min (tailnet-only API, no GitHub
  webhook). Acceptable; §9.3 of the handoff (tailscale/github-action) exists
  if instant sync is ever needed — deliberately not in scope.
- **Recreate downtime** per rollout — deliberate (single node, capacity);
  unchanged from today.
- **Node capacity** (~59% mem) — flag to platform before adding anything
  heavy; this plan adds only Argo CD's own controller load (already running).
- **Mutable-tag window** between Phase 1 and Phase 2 adoption — git says
  `-latest`, matching live; don't linger between phases.
- **`development` rulesets untouched** — this plan changes nothing about the
  `development` gating (its checks are healthy); only `nam20485` ruleset is
  verified/adjusted.

## 11. End-state acceptance

- [ ] 4 Argo CD Applications `Healthy/Synced`; `argocd app diff` clean
- [ ] A push to `nam20485` touching source auto-deploys: image tag in git ==
      tag running in the cluster within ~5 min, no human action
- [ ] A commit touching only `deploy/**` or `docs/**` triggers no builds
- [ ] PRs into `nam20485` are gated by CMake + CodeQL + Codacy +
      dependency-review (no dead "expected" contexts)
- [ ] Deployed `nam20485` images are cosign-signed **and** Scout-scanned
- [ ] `https://debian13vm.tail11ba79.ts.net/swagger/` trusted-HTTPS;
      LAN-IP HTTP still works; gRPC :50051 answers
- [ ] `odbdesign-server-request-secret` present and untouched by syncs
- [ ] `deploy.ps1` and k3d-era scripts gone; handoff §5 retire list honored
      (minus `k3s-cluster.ps1`, §3.4)
- [ ] Manual `kubectl edit` on a managed resource is reverted by selfHeal
      within one sync cycle

## 12. Operating Argo CD from this repo: CLI + MCP server

Both tools drive the same tailnet-only Argo CD at
`https://debian13vm.tail11ba79.ts.net/argocd`. Rule of thumb for agents:
**read freely, mutate through git.** The only ad-hoc mutation that fits the
GitOps model is triggering a sync early — via the **CLI** (the MCP account
is read-only by platform design, §12.2); every real change (image tag,
manifest, Application set) is a commit on `nam20485`.

### 12.1 argocd CLI (installed and logged in)

State on this machine (verified 2026-09-08): v3.5.2 at `~/.local/bin/argocd`
(pinned to match the server), context `debian13vm.tail11ba79.ts.net/argocd`
is CURRENT (`~/.config/argocd/config`). Works only from tailnet devices.
Re-auth when the session expires: `argocd relogin`, or full login — note
**both** flags, required by the Traefik rootpath:
`argocd login debian13vm.tail11ba79.ts.net --username admin --grpc-web --grpc-web-root-path /argocd`.

| Task | Command |
|---|---|
| App status (§7 verifications) | `argocd app list` · `argocd app get odbdesign-server` |
| Diff live vs git (adoption check) | `argocd app diff odbdesign-server` |
| Skip the ≤3 min poll wait | `argocd app get odbdesign-server --refresh` |
| Trigger the git-recorded state early | `argocd app sync odbdesign-server` |
| Pause automated sync (rollback §9) | `argocd app set odbdesign-server --sync-policy none` |
| Resume automated sync | `argocd app set odbdesign-server --sync-policy automated` |
| Connect repo (Phase 1 bootstrap) | `argocd repo add https://github.com/nam20485/OdbDesign` |
| Rotate the `mcp` account token (§12.2) | `argocd account generate-token --account mcp --expires-in 8760h` → update `ARGOCD_API_TOKEN` in `~/.api-keys-export.sh` → restart ZCode |

Never `kubectl apply` or `argocd app sync --local` against app-managed
resources (handoff §9.5) — selfHeal reverts it within one cycle.

### 12.2 argocd MCP server (`.zcode/config.json`)

A project-scoped ZCode MCP server is already configured (official
`argoproj-labs/mcp-for-argocd`, `argocd-mcp@0.9.0`, stdio; Node ≥18 — v20
present) — identical to the platform repo's config:

```json
"argocd-mcp": {
  "type": "stdio",
  "command": "npx",
  "args": ["-y", "argocd-mcp@0.9.0", "stdio"],
  "env": { "ARGOCD_BASE_URL": "https://debian13vm.tail11ba79.ts.net/argocd" }
}
```

**Auth: the token is deliberately NOT in the config — it arrives via
environment inheritance.** The stdio server process inherits
`ARGOCD_API_TOKEN` from the ZCode process environment; there is nothing to
inline or interpolate in the file. The value lives in
`~/.api-keys-export.sh` (single-source key architecture: shells + Plasma via
`~/.config/environment.d/50-api-keys.conf`) and belongs to the platform's
dedicated **`mcp`** Argo CD account (`apiKey` capability, no UI login) —
verified live 2026-09-08: `session/userinfo → {"loggedIn":true,"username":"mcp"}`.

Practical consequences:

- **Activation**: ZCode must be *launched* from an environment that exports
  the var, and restarted afterwards — MCP servers load at startup only. On
  this machine: interactive shells get the var via `.profile` (line 46) and
  `.bashrc` (line 232, interactive-only — tool shells in agent sessions skip
  it). For **Plasma-menu** launches, `~/.config/environment.d/50-api-keys.conf`
  carries the var (kept fresh by the `api-keys-sync.path` unit), but
  environment.d is only read at user-manager startup — after any token
  rotation, log out/in (or run
  `systemctl --user import-environment ARGOCD_API_TOKEN` from a sourced
  shell) before launching ZCode from the menu; a terminal-launched ZCode
  inherits it without either.
- **Rotation (1-year expiry policy — never unexpiring)**: as admin,
  `argocd account generate-token --account mcp --expires-in 8760h`, update
  `ARGOCD_API_TOKEN` in `~/.api-keys-export.sh`, restart ZCode.
- **RBAC: the `mcp` account is read-only** (`role:readonly` fallback —
  `argocd-rbac-cm` sets no default policy). `sync_application`,
  `create_application`, `update_application`, `delete_application`, and
  `run_resource_action` are **denied by design**; extending RBAC is a
  deliberate platform change, not something this repo works around.
- **Upstream rootpath bug**: `argocd-mcp@0.9.0` drops the base-URL path
  (absolute `/api/v1` literals; upstream issues #81/#99, PR #116 pending).
  The platform routes around it with an `argocd-api` IngressRoute +
  `argocd-addprefix` middleware (`…/api/v1/…` → `/argocd/api/v1/…`). Keep
  the `/argocd` base in our config — it stays correct with or without the
  workaround; retire the workaround only when upstream ships the fix.
- TLS: real Let's Encrypt cert — do **not** set
  `NODE_TLS_REJECT_UNAUTHORIZED=0` (README suggests it only for private CAs).
- `.zcode/` stays gitignored — the config is credential-free by design, and
  the ignore entry keeps any future edit that way.

Tools (v0.9.0) mapped to this plan — read tools work under the `mcp`
account's RBAC:

| Tool | Use here |
|---|---|
| `list_applications`, `get_application` | §7 verifications (Healthy/Synced), watching a rollout land |
| `get_application_managed_resources`, `get_application_resource_tree` | adoption diffs, what each app owns |
| `get_application_workload_logs`, `get_resource_events` | crash-loop triage — replaces the first `kubectl logs`/`describe` |
| `get_appproject`, `list_clusters`, `get_resources`, `get_application_events` | platform-scoped reads |
| `sync_application` | **denied by RBAC** — trigger an early sync with the CLI instead |
| `create_application`, `update_application`, `delete_application`, `run_resource_action` | **denied by RBAC — and wrong anyway**: Applications are git-managed in `deploy/kube/argocd/apps/`; out-of-band edits are drift the root app's selfHeal/prune will fight |

Agent guardrail: the MCP server is a **read-only view** of Argo CD. Early
sync-triggering uses the CLI; every real change is a commit on `nam20485`.
Platform docs are the source of truth for the server, account, and RBAC:
`nam20485/linux-system-agent` → `.agents/rules/tools.md` (§ Argo CD (MCP))
and `.agents/rules/k3s-cluster.md` (account gotchas, CLI login flags,
rootpath workaround).
