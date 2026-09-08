# Argo CD GitOps handoff for `nam20485/OdbDesign`

> **Audience:** the OdbDesign project (humans + agents) migrating
> `odbdesign-server` and `odbdesign-server-swaggerui` deployments from manual
> scripts to Argo CD GitOps on the debian13vm k3s cluster.
> **Author:** linux-system-agent (cluster/Argo CD platform owner). **Date:** 2026-09-08.
> **State:** Argo CD v3.5.2 installed, validated, serving trusted HTTPS.
> Platform PR: `nam20485/linux-system-agent#8`. A copy of this doc lives in
> `OdbDesign/docs/plan/argocd-gitops-handoff.md`.

---

## 1. TL;DR

- Argo CD is live at **`https://debian13vm.tail11ba79.ts.net/argocd`** (real
  Let's Encrypt cert; tailnet devices only — **nothing is internet-exposed**,
  a hard requirement).
- You already have everything GitOps needs, except the wiring:
  immutable image tags (`docker-publish.yml` already pushes
  `<branch>-<run_number>`), manifests in `deploy/kube/`, and a cluster running
  those manifests. The migration is: **connect the repo to Argo CD, wrap your
  manifest dirs in `Application` resources, and replace `scripts/deploy.ps1`'s
  `rollout restart` (mutable `:nam20485-latest`) with a CI commit that bumps
  the immutable tag in git.** Argo CD auto-syncs within ~3 minutes of the commit.
- GitHub-hosted runners **cannot reach this cluster** — by design. The
  recommended workflow never talks to the cluster: it only pushes images to
  ghcr.io and commits tag bumps. (Tailnet-joining option documented in §9.3.)
- Two secrets stay **out of git** and out of Argo CD's management:
  `odbdesign-server-request-secret` (basic-auth, already in the cluster) and
  the ghcr pull secret (only if you ever make packages private).

## 2. How you deploy today (as-is)

Verified from the OdbDesign repo, 2026-09-08 (default branch `development`;
de-facto deploy branch `nam20485`):

1. **Build:** `cmake-multi-platform.yml` ("CMake Build Multi-Platform") builds
   on branches `development, staging, main, release, nam20485`.
2. **Image:** `docker-publish.yml` (triggered on that workflow's success)
   builds `Dockerfile.prebuilt` (or full `Dockerfile` on manual dispatch) and
   pushes `ghcr.io/nam20485/odbdesign` tagged **`<branch>-<run_number>`** and
   **`<branch>-latest`**, cosign-signs the digest, and — on `release` only —
   fires a `trigger_deploy_release_event` repository dispatch (its consumer
   `deploy-local-k8s.yml` is in `.github/workflows/disabled/`).
3. **SwaggerUI image:** built in the separate **public** repo
   `nam20485/OdbDesignServer-SwaggerUI` → `ghcr.io/nam20485/odbdesignserver-swaggerui`.
4. **Deploy (manual):** `scripts/deploy.ps1` on a cluster-accessible machine —
   PV hostPath pre-flight (`/srv/odbdesign-volume`), applies
   `deploy/kube/k3d-volume-{pv,pvc}.yaml`, creates/refreshes
   `odbdesign-server-request-secret` (via
   `scripts/odbdesign-server-request-secret.ps1`, env-var driven), applies the
   `OdbDesignServer` manifests, **`rollout restart`** to force a re-pull of the
   mutable `:nam20485-latest` tag, regenerates
   `deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml` from
   `swagger/odbdesign-server-0.9-swagger.yaml`, applies the SwaggerUI
   manifests + restart, then validates gRPC exposure
   (`scripts/validate-grpc-exposure.ps1`).

**Why this can't be the GitOps flow:** `rollout restart` + a mutable `latest`
tag means *git never changes* when you deploy — Argo CD would see no diff, and
`selfHeal` would actively fight any manual restart that pulled a newer image
than git records. The fix is not new tooling; your CI already publishes
immutable `<branch>-<run_number>` tags — they just aren't referenced from git.

## 3. Cluster and Argo CD facts (live, verified 2026-09-08)

| Item | Value |
|---|---|
| Cluster | single-node **k3s v1.36.3+k3s1** on `debian13vm` (Debian 13), node Ready |
| Node capacity / load | 4 vCPU, 15 GiB RAM · ~38% CPU, ~59% memory (monitoring + Argo CD + your 2 deployments) |
| Node IPs | Tailscale **`100.118.225.119`** (canonical) · LAN `192.168.122.200` (libvirt-NAT: reachable only from the precision5820 host) |
| Argo CD | **v3.5.2** non-HA, namespace `argocd`, 6 Deployments + 1 StatefulSet all ready |
| Argo CD UI/API | `https://debian13vm.tail11ba79.ts.net/argocd` (rootpath: all UI/API routes under `/argocd/…`; cluster health at `/argocd/api/version` etc.) |
| TLS | Let's Encrypt via `tailscale cert`, auto-renewed (weekly re-import cron deployed); Traefik cluster-default `TLSStore` — your HTTPS routes get the cert automatically |
| Ingress controller | Traefik **3.7.8** (k3s built-in), entrypoints `web` :80 / `websecure` :443; CRDs enabled (`IngressRoute`, `Middleware`, `TLSStore`) |
| Storage | default SC `local-path`; your static PV `k3d-volume` (hostPath `/srv/odbdesign-volume`, 1Ti, `Retain`) — **repo manifests match live objects**, safe to adopt |
| Your live workloads | `default` ns: `odbdesign-server-v1` 1/1 (`ghcr.io/nam20485/odbdesign:nam20485-latest`, Recreate, uid/gid 10001 + fsGroup, PVC `k3d-volume-claim`), `odbdesign-server-swaggerui-v1` 1/1 (`…swaggerui:nam20485-latest`), services :80/:80/:50051-LB, hostless ingress `/` + `/swagger` |
| Image pulls | **no imagePullSecret in cluster** — your ghcr packages are public today |
| Monitoring | Prometheus `https://debian13vm.tail11ba79.ts.net/prometheus` · Grafana `…/grafana` (logs: Grafana → Explore → Loki) |
| Platform tooling | `nam20485/linux-system-agent`: `scripts/argocd.ps1` (lifecycle/validate), `scripts/tailscale-tls.ps1` (cert import), rules `.agents/rules/k3s-cluster.md` |

## 4. Access and credentials

### 4.1 First login (one-time, human on the tailnet)

```bash
kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath='{.data.password}' | base64 -d; echo
```

Log in as `admin` at the UI URL → **rotate the password** (Settings →
Accounts → admin → Update password) → **then** delete the bootstrap secret:
`kubectl -n argocd delete secret argocd-initial-admin-secret` (rotate first —
until rotated, that secret *is* the live password). Platform validation treats
its absence as expected.

### 4.2 Automation account (only if you adopt §9.3)

Ask the platform agent to add a least-privilege `odbdesign` Argo CD account
(`accounts.odbdesign: apiKey,login` in `argocd-cm` + an RBAC role scoped to
your project in `argocd-rbac-cm`) and generate its token. Argo CD's own
RBAC/accounts are platform-owned; you won't need this for the pure-git flow.

### 4.3 Reachability rules

- kubectl / argocd CLI work **only from tailnet devices** (precision5820,
  debian13vm). argocd CLI login:
  `argocd login https://debian13vm.tail11ba79.ts.net/argocd` (binary not
  pre-installed; grab the linux/amd64 release into `~/.local/bin`).
- **GitHub-hosted runners are not on the tailnet** and must not try to reach
  the cluster (§9).
- gRPC endpoint for clients on the tailnet: `100.118.225.119:50051` (your
  LoadBalancer service answers on every node IP; k3s ServiceLB has no per-IP
  binding — verified, no `service-lb-bind-ip` flag exists).

## 5. Ownership boundary

| Concern | Owner |
|---|---|
| Argo CD install/upgrade/health, TLS, Traefik, monitoring, cluster lifecycle | **linux-system-agent** (this repo's rules/skills/scripts) |
| Argo CD accounts/RBAC for OdbDesign automation | linux-system-agent, on request |
| `Application` manifests, k8s manifests, images, workflows, sync behavior of your apps | **OdbDesign** |
| `odbdesign-server-request-secret` content/rotation | **OdbDesign** (out-of-band, §8.3) |

Retire from OdbDesign's `scripts/` once GitOps lands (they predate this
platform and duplicate or bypass it): `deploy.ps1`, `k3s-cluster.ps1`,
`deploy-monitoring.ps1`, `start-k3d-cluster.ps1`, `create-k3d-cluster.ps1`,
`register-k3d-startup-task.ps1`, plus `.github/workflows/disabled/deploy-{eks,local-k8s}.yml`
and `deploy/kube/default-ingress (eks).yaml`, `deploy/kube/issuer.yaml`
(EKS-era cert-manager artifact — LE HTTP-01 is impossible on this private
cluster; TLS is solved by the platform `TLSStore`).

## 6. Decisions to make before onboarding

1. **Deploy branch (`targetRevision`).** Images in the cluster are
   `nam20485-*`, so the de-facto deploy branch is **`nam20485`**; your default
   branch is `development`. Pick the branch whose `deploy/kube/` is the deploy
   truth and pin every Application's `targetRevision` to it (examples below
   use `nam20485`). Whatever you pick, `docker-publish.yml` already builds it.
2. **Tag strategy.** Deploy **`<branch>-<run_number>`** tags (already pushed by
   `docker-publish.yml` — truly immutable) and stop pointing manifests at
   `<branch>-latest`. Keep `imagePullPolicy: Always` or drop it (irrelevant
   with unique tags; harmless to keep).
3. **Application layout** (§8.1): three Applications — `odbdesign-server`,
   `odbdesign-swaggerui`, `odbdesign-shared` (ingress + PV/PVC) — each pointing
   at its own directory. This requires one small repo reshuffle so directories
   don't overlap.
4. **Ingress host rules.** Your live ingress is hostless (matches any host,
   incl. the LAN IP). Recommended: add
   `host: debian13vm.tail11ba79.ts.net` and a `tls: []`-style HTTPS variant so
   `/` and `/swagger` are served on the canonical trusted-HTTPS host. (Both
   plain `Ingress` and Traefik `IngressRoute` work; Traefik's default TLSStore
   supplies the cert on :443 either way — no cert config needed.)
5. **Swagger spec ConfigMap.** Today `deploy.ps1` regenerates
   `swagger-spec-configmap.yaml` at deploy time. Under GitOps the committed
   file *is* the truth — move regeneration into CI (§9.2, step 0): any workflow
   touching `swagger/odbdesign-server-0.9-swagger.yaml` must regenerate and
   commit the ConfigMap in the same run.

## 7. Repo reshuffle (one-time, small)

Directory-based Applications are recursive, so give each one a clean scope:

```text
deploy/kube/
├── OdbDesignServer/            → Application odbdesign-server
├── OdbDesignServer-SwaggerUI/  → Application odbdesign-swaggerui
├── shared/                     → Application odbdesign-shared
│   ├── local-ingress.yaml      (moved; add host rule per §6.4)
│   ├── k3d-volume-pv.yaml      (moved — matches live PV, adoption is a no-op)
│   └── k3d-volume-pvc.yaml     (moved)
├── argocd/                     → Application manifests themselves (§8.2)
│   ├── application-odbdesign-server.yaml
│   ├── application-odbdesign-swaggerui.yaml
│   └── application-odbdesign-shared.yaml
└── legacy-eks/                 → default-ingress (eks).yaml, issuer.yaml (or delete)
```

Update `deploy.ps1`'s paths if you keep it around during transition, or retire
it outright after the first successful auto-sync.

## 8. Onboarding steps

### 8.1 Connect the repo to Argo CD

UI: Settings → Repositories → *Connect repo* →
`https://github.com/nam20485/OdbDesign`, connection **private** with a
fine-grained PAT (**Contents: read-only**, scoped to this repo) or an SSH
deploy key. Prefer a deploy identity over a personal account. The SwaggerUI
repo does **not** need connecting — its manifests live in OdbDesign and its
image is public. (Only OdbDesign manifests are synced; images pull from ghcr
anonymously while packages are public.)

### 8.2 Declare the Applications (in your repo, `deploy/kube/argocd/`)

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: odbdesign-server
  namespace: argocd               # Applications must live in the argocd namespace
spec:
  project: default                # dedicated 'odbdesign' AppProject available on request (§11)
  source:
    repoURL: https://github.com/nam20485/OdbDesign
    targetRevision: nam20485      # §6.1 decision
    path: deploy/kube/OdbDesignServer
  destination:
    server: https://kubernetes.default.svc   # same cluster — no registration needed
    namespace: default
  syncPolicy:
    automated:
      prune: true                 # objects deleted from git get deleted (see §8.3 caveat)
      selfHeal: true              # manual kubectl edits get reverted — deploy.ps1 must go
    syncOptions:
      - CreateNamespace=false     # 'default' exists; never auto-create namespaces
```

Duplicate for `odbdesign-swaggerui` (path `deploy/kube/OdbDesignServer-SwaggerUI`)
and `odbdesign-shared` (path `deploy/kube/shared`).

**Bootstrap (first time only):** the Applications can't be applied by Argo CD
before Argo CD knows them. From a tailnet machine:
`kubectl apply -f deploy/kube/argocd/` (or create them once via the UI).
Afterwards they're self-managed — optionally add a fourth app-of-apps
Application pointing at `deploy/kube/argocd/` to GitOps the Apps themselves.

### 8.3 Out-of-band resources (stay out of git)

- **`odbdesign-server-request-secret`** (default ns, basic-auth via
  `secretKeyRef`): already exists in the cluster. Argo CD only prunes resources
  *it tracks from git* — untracked secrets are untouched by prune/selfHeal.
  Keep rotating it with `scripts/odbdesign-server-request-secret.ps1` from a
  tailnet machine (or fold into your secret manager later). Verify after the
  first sync: `kubectl -n default get secret odbdesign-server-request-secret`.
- **PV data safety:** the shared Application manages `k3d-volume-pv` (reclaim
  `Retain`). If you ever delete that Application, remove its
  `resources-finalizer` cascade first or accept that the PV object is deleted —
  with `Retain` the hostPath data at `/srv/odbdesign-volume` survives, but
  re-binding needs care. Simplest: never prune the shared app casually.
- **ghcr pull secret** — only if packages go private (§9.4).

### 8.4 First sync

1. Apps appear in the UI after the initial repo crawl (≤ ~3 min) as
   **OutOfSync** — they'll want to adopt the live objects.
2. Review the diff **in the UI before syncing**: your committed manifests match
   the live state (verified 2026-09-08, incl. PV/PVC), so this should be a
   near-no-op adoption. The expected diffs: image tag if you bump to an
   immutable tag first (do the bump *after* adoption to keep the first sync
   boring), and any ingress host-rule change from §6.4.
3. Sync. Note `strategy: Recreate` + single replica = brief downtime when a
   real rollout happens (your deliberate choice for node capacity — fine).
4. Verify: UI apps **Healthy/Synced**; `https://debian13vm.tail11ba79.ts.net/swagger/`
   loads; gRPC `100.118.225.119:50051` answers (`scripts/validate-grpc-exposure.ps1`
   still works from a tailnet machine); platform gate
   `./scripts/argocd.ps1 -Action Validate` in the linux-system-agent repo.

## 9. GitHub Actions: the deployment workflow

### 9.1 Pattern: pure git (recommended — runner never touches the cluster)

Git is the only interface. CI: build → push immutable tag → **bump that tag in
the manifest → commit**. Argo CD auto-syncs ≤3 min later. Concrete addition to
`docker-publish.yml` (it already has `contents: write` and the tag scheme):

```yaml
  bump-manifest:
    name: Bump image tag in deploy manifests (GitOps trigger)
    needs: build
    if: github.event_name == 'workflow_run' && github.event.workflow_run.head_branch == 'nam20485'
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4
        with:
          ref: ${{ github.event.workflow_run.head_branch }}
      - name: Bump odbdesign image tag
        run: |
          TAG="${BRANCH_NAME}-${GITHUB_RUN_NUMBER}"   # same value docker-publish pushed
          sed -i "s|image: ghcr.io/nam20485/odbdesign:.*|image: ghcr.io/nam20485/odbdesign:${TAG}|" \
            deploy/kube/OdbDesignServer/deployment.yaml
          git config user.name  "github-actions[bot]"
          git config user.email "github-actions[bot]@users.noreply.github.com"
          git add deploy/kube/OdbDesignServer/deployment.yaml
          git diff --cached --quiet || git commit -m "deploy: odbdesign-server ${TAG}"
          git push
```

Notes:

- Set `BRANCH_NAME` the same way the existing job derives it (it's already an
  env in your workflow); the pushed tag is exactly the one the build job
  published, so the manifest always references an image that exists.
- The bump commit re-enters the workflow's branch filter — guard the CMake →
  docker-publish chain with `paths-ignore: ["deploy/**", "docs/**"]` (or accept
  a no-op rebuild cycle). A `[skip ci]` commit-message convention also works if
  your triggers honor it.
- Same pattern in `nam20485/OdbDesignServer-SwaggerUI`'s publish workflow for
  its image — it needs a PAT (repo secret) with contents:write **on OdbDesign**
  to commit the bump into `deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml`
  (cross-repo commit), or bump that tag manually/in a scheduled job.
- **Step 0 (from §6.5):** add a small job (here or in the workflow that owns
  `swagger/`) that regenerates `swagger-spec-configmap.yaml`
  (`kubectl create configmap … --dry-run=client -o yaml` equivalent, or a
  scripted YAML transform) and commits it whenever the spec file changes —
  otherwise git carries a stale spec.

### 9.2 What each old step maps to

| Old (manual) | New (GitOps) |
|---|---|
| `deploy.ps1` applies manifests | Argo CD auto-sync from git |
| `rollout restart` to pull `:nam20485-latest` | CI commits `<branch>-<run_number>` tag bump → sync rolls the deployment |
| swagger ConfigMap regeneration at deploy time | CI regeneration + commit (§9.1 step 0) |
| PV hostPath pre-flight | one-time; PV already matches live (§7) |
| `validate-grpc-exposure.ps1` post-deploy | keep — run from a tailnet machine post-sync, or inside a §9.3 job |
| `trigger_deploy_release_event` dispatch → disabled `deploy-local-k8s.yml` | delete both; the bump-commit *is* the deploy trigger |

### 9.3 If a runner ever needs cluster access (instant sync, smoke tests)

Official **`tailscale/github-action`** joins the runner to the tailnet for the
job: needs a Tailscale OAuth client (`TS_OAUTH_CLIENT_ID`/`TS_OAUTH_SECRET`
repo secrets, tag e.g. `tag:ci` — ask the platform/user to add the ACL) and,
for API calls, the `odbdesign` account token (§4.2). Then
`https://debian13vm.tail11ba79.ts.net/argocd/...` and
`argocd app sync odbdesign-server` work from the job. Keep such jobs to
sync-triggering and read-only verification; deployment truth stays in git.
(A self-hosted runner on a tailnet machine is the heavier alternative.)

### 9.4 If ghcr packages go private

```bash
kubectl -n default create secret docker-registry ghcr-pull \
  --docker-server=ghcr.io --docker-username=<gh-user> --docker-password=<PAT read:packages>
```

Add `imagePullSecrets: [{name: ghcr-pull}]` to both deployments (the secret
itself stays out of git, like §8.3).

### 9.5 Hard don'ts

- ❌ `kubectl apply` from CI with a kubeconfig secret — bypasses git;
  `selfHeal` reverts it within minutes.
- ❌ Exposing the Argo CD API/ingress publicly to simplify CI (§4.3).
- ❌ Mutable-tag deploys (`:nam20485-latest` + restart) once Applications exist —
  this is the exact drift `selfHeal` will fight.

## 10. Operations and troubleshooting

| Symptom | First look |
|---|---|
| App stuck `OutOfSync` after the bump commit | UI → app → *Refresh*; poll interval is ≤3 min; verify `targetRevision`/path; GitHub-cloud webhooks can't reach the API (tailnet-only), so there is no instant trigger without §9.3 |
| `ImagePullBackOff` after a bump | tag exists on ghcr? package visibility vs `ghcr-pull` (§9.4)? |
| Pod crash-loops after sync | Grafana → Explore → Loki `{namespace="default"}`; `kubectl -n default describe pod …` |
| `/swagger` 404 or stale spec | ConfigMap regeneration step (§9.1 step 0); ingress paths (§6.4) |
| gRPC unreachable | `100.118.225.119:50051` from a tailnet device; ServiceLB answers on all node IPs (no per-IP bind exists in k3s) |
| `selfHeal` keeps reverting something | that something is manual drift — fix it in git instead |
| Argo CD itself unhealthy | platform: `./scripts/argocd.ps1 -Action Validate` (linux-system-agent repo); escalate to the platform agent |
| TLS errors on the ts.net URL | platform: `/var/log/tailscale-tls-sync.log` on the VM, `kubectl -n kube-system get tlsstore default`; escalate |

Capacity note: node is at ~59% memory with everything running; your Recreate
strategy already accounts for the single-node overlap problem. Flag capacity
concerns to the platform agent before adding heavy workloads (e.g. CI runners
on the VM).

## 11. Optional hardening

Dedicated `odbdesign` **AppProject** (restrict `sourceRepos` to
`github.com/nam20485/OdbDesign`, destinations to the `default` namespace) —
platform applies it on request; then set `spec.project: odbdesign` on your
Applications.

## 12. Platform references (repo `nam20485/linux-system-agent`)

- `scripts/argocd.ps1` — Argo CD lifecycle (Install/Status/Validate/Uninstall)
- `scripts/tailscale-tls.ps1`, `deploy/kube/tailscale-tlsstore.yaml` — cluster TLS
- `deploy/kube/argocd-ingress.yaml` — working path-prefix IngressRoute example
- `.agents/rules/k3s-cluster.md` — cluster facts & conventions (source of truth)
- `.agents/skills/k3s-admin/SKILL.md` — day-2 procedures
- `docs/plans/.complete/argocd-install-plan.md` — install plan + outcome

**Escalation:** cluster-level anything (Argo CD upgrades, accounts/RBAC,
tailnet ACLs, certs, capacity, Traefik) → linux-system-agent. App-level
(manifests, images, syncs of your Applications) → OdbDesign repo.
