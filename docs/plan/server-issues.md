# OdbDesignServer — open issues: options & recommendations (SI1–SI8, SI19)

| | |
|---|---|
| Status | **RECOMMENDATIONS DRAFT** — two decisions still open (see [Open decisions](#open-decisions)): SI2 branch set, SI5 IdP choice. |
| Date | 2026-09-09 |
| Scope | OdbDesignServer (Crow REST + gRPC), CI / GitHub Pages publishing, branch topology |
| Related | [https-tls-options.md](https-tls-options.md) (REST TLS decision, §8 gRPC TLS) · [gh-pages-coverage-integration.md](../gh-pages-coverage-integration.md) · [html report coverage pages.md](../html%20report%20coverage%20pages.md) · [opt/phase2-grpc-optimizations.md](opt/phase2-grpc-optimizations.md) |

Each issue: **Current state** (facts, with code references) → **Options** (pros/cons) → **Recommendation + why** → **Dependencies / effort**.

---

## Original task list (verbatim)

> Create a doc with options + pros/cons and recommendations + why for all of these issues.
>
> - **SI1** add redoc page — maximally annotated to maximize detailed type and api behavior in redoc page's definitions — `/redoc`
> - **SI2** add coverage report HTML to the pages site when deploying pages — do we generate results and coverage reports per environment branch (i.e. development, staging, main, release)? — post per-branch for development, staging, main, release (not main)
> - **SI3** do we need the main branch? it seems like development, staging, main, release are only ones we need. Actually, ideally we would be using production instead of main. Dont change unless risk is totally nontrivial.
> - **SI4** grpc tls
> - **SI5a** Need unified overall authentication mechanism that is more secure than Basic we have for REST and nothing we have for grpc
> - **SI5b** Need Authorization scheme based on logged-in user and resource-by-resource (grpc and REST). Both communication channel type AND specific data resources (e.g. only designs you have uploaded or are shared to all users). Need regular interactive client scheme AND device flow in addition to for machine → server auth
> - **SI6** Server-side optimizations — any optimizations options? (client design fetching, loading, display, and visibility toggle has been heavily optimized, >10x faster fetch→load→display, almost instantaneous component and layer visibility toggling)
> - **SI7** RequestLoadDesign(Async): start a FileArchive/Design load (parse) and return immediately. Next GetDesignAsync request for that same design can skip loading and return response data immediately
> - **SI8** server communication channel — non-language-specific: SSE? websockets? optional connection and use — app still functions, falls back even if connection is not attempted or attempted but can't complete successfully
> - **SI19** remaining grpc optimization items from `docs/plan/opt/phase2-grpc-optimizations.md` — implement remaining issues from the previous grpc optimization round

---

## Server today (shared context)

Facts referenced by several sections below; verified 2026-09-09.

| Area | Current state |
|---|---|
| REST framework | Crow 1.3.3 (`vcpkg.json` override), `crow::Crow<crow::CORSHandler>` (`Utils/crow_win.h`). GZIP response compression on (`OdbServerAppBase.cpp`, `use_compression(GZIP)`); CORS wide open (`origin("*")`, `headers("*")`, `OdbDesignServerApp.cpp`). No TLS in-app (`OdbDesignArgs::useHttps()` is a dead declaration — declared, never implemented, never called). |
| REST auth | HTTP **Basic**, enforced by an explicit `AuthenticateRequest(req)` call inside every protected route lambda (e.g. `DesignsController.cpp`, `FileModelController.cpp`). Credentials from `ODBDESIGN_SERVER_REQUEST_USERNAME/PASSWORD` env vars with **hardcoded fallback `odb`/`plusplus`** when unset (`OdbDesignLib/App/BasicRequestAuthentication.cpp`). Bypass in `_DEBUG` + `Environment=Local`. Unauthenticated: `/healthz*`, `/ready`, `/helloworld*`. |
| gRPC | `Odb.Grpc.OdbDesignService` (`OdbDesignServer/protoc/grpc/service.proto`): `GetDesign`, `GetLayerFeaturesStream`, `GetLayerFeaturesBatchStream` (feature-flagged), `GetLayerSymbols`, `GetStandardFonts`, `HealthCheck`; reflection + standard health service enabled. Bound with `InsecureServerCredentials()` (`OdbDesignServerApp.cpp`) — **no auth at all**, no interceptors anywhere. gzip default compression; 250 MB max message (`OdbDesignServer/config.json`). |
| Design loading | One shared `DesignCache` (`OdbDesignLib/App/DesignCache.cpp`) for REST + gRPC: shared-lock read, on miss **synchronous parse in the requesting call** (minute-scale for large designs), double-checked insert. **No eviction, no in-flight dedup, no serialized-response cache** — every REST response re-runs protobuf `to_json()`. List endpoints rescan the designs directory per call. Uploads (`FileUploadController`) write the archive but never parse it. `--load-all` startup preload exists but no deployment uses it. |
| API docs | Hand-maintained OpenAPI 3.0.3 spec: `swagger/odbdesign-server-0.9-swagger.yaml` (37 paths, already tagged/example-annotated). Served by a **sidecar container** (`ghcr.io/nam20485/odbdesignserver-swaggerui`, separate repo) whose spec is refreshed via ConfigMap (`scripts/deploy.ps1` → `deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml`); Traefik routes `/swagger` to it (`deploy/kube/local-ingress.yaml`). No ReDoc anywhere. |
| Pages / CI | Pages deploys the Jekyll `docs/` site on **push to `release` only** (`.github/workflows/jekyll-gh-pages.yml`; custom domain `source.odbdesignserver.com`; Doxygen runs but the copy-to-site step is commented out). Coverage workflow (`.github/workflows/code-coverage.yml`, lcov + `genhtml` → HTML, artifact `linux-coverage-report`, 14-day retention, Codacy upload) runs on push/PR for `development`, `staging`, `main`, `release`, `nam20485`. |
| Branch topology | `nam/<feature> → nam20485 → development → staging → main → release`; `production` is defunct legacy (see `AGENTS.md`). Ruleset 169360 covers main/release/staging/production and requires the three CMake legs, SBOM, Codacy, dependency-review **and** `Docker-Build-and-Publish`. |

---

## SI1 — add ReDoc page at `/redoc`, maximally annotated

### Current state

The OpenAPI spec (`swagger/odbdesign-server-0.9-swagger.yaml`) is the single source of truth and is already well-annotated (info block, per-tag descriptions, response examples). Swagger UI is a sidecar container; ReDoc is not deployed anywhere. The spec is hand-maintained — there is no generator in-repo (the "GENERATED from route registrations" header is aspirational).

### Options

**Option 1 — `redoc.html` in the existing Swagger UI sidecar.**
Add a static `redoc.html` (redoc-standalone: one HTML file + CDN script tag pointing at the existing spec URL) next to swagger-ui in the sidecar image / nginx content; route `/redoc` to the sidecar exactly as `/swagger` is routed today (Traefik ingress + nginx compose catch-all already exist).

- ✅ Zero C++ changes, zero new containers; reuses the ConfigMap spec-refresh pipeline (`deploy.ps1` regenerates it) so both UIs always show the same spec.
- ✅ ReDoc is the better *reference* UI: three-pane layout, searchable schema definitions, expandable models — exactly the "maximize detailed type and API behavior in definitions" ask.
- ❌ Keeps docs in a separate repo/image (spec ships from this repo via ConfigMap, UI ships elsewhere).
- ❌ No "try it out" (that's what the existing `/swagger` page is for — the two complement each other).

**Option 2 — serve `/redoc` + `/openapi.json` from the Crow binary.**
Embed the spec (CMake define or a resource compiler) and serve it plus a bundled `redoc.html` from the server itself.

- ✅ Docs always match the *running* server version; works locally with zero deployment deps; eventually lets the sidecar be retired.
- ✅ One place to keep spec version == server version (`0.9` today).
- ❌ C++ build/packaging changes; two copies of the spec unless the sidecar is migrated at the same time.
- ❌ `/redoc`, `/openapi.json` must be added to the unauthenticated-route list (or docs sit behind Basic auth, which breaks ReDoc CDN-less rendering in some setups).

**Option 3 — publish a static ReDoc build to the GitHub Pages site.**
Build `redoc-cli bundle` (or a `redoc.html` + spec copy) into the Jekyll site, e.g. at `/api/redoc` — the same mechanism SI2 will use for coverage, and the slot `docs/README.md` already (stale-)links to (`…github.io/OdbDesign/api`).

- ✅ Public, always-reachable docs with no cluster dependency; fits the FOSS project story.
- ✅ Redeploys are cheap (spec file + one HTML).
- ❌ Public docs can lag deployed servers unless published per-branch (same open question as SI2).
- ❌ `Try it out` against a private cluster doesn't work from the public page anyway.

**Annotation work (orthogonal to hosting — it's all in the YAML).**
Whatever hosts the page, "maximally annotated" means investing in `swagger/odbdesign-server-0.9-swagger.yaml`:

1. Property-level `description:` on every schema (source material already exists as comments in `OdbDesignLib/protoc/*.proto`).
2. Complete error response sets per operation (`400`/`401`/`403`/`404`/`500` — several operations currently document only success + 401/404).
3. `example`/`examples` on all large schemas (`StepDirectory`, `LayerDirectory`, `FeaturesFile`, `MatrixFile`, `Design`).
4. `commonParameters`/header docs, and `deprecated` markers on the commented-out legacy routes if they return.
5. When SI5 lands: replace the global `BasicAuth` security scheme with the new scheme(s), per-route.

### Recommendation

**Option 1 now, Option 3 as the public home; keep Option 2 in the back pocket.**
Option 1 is pure deployment config on infrastructure that already routes `/swagger` — the lowest-risk way to get `/redoc` tomorrow, and the annotation investment lands in the same YAML either way. Option 3 pairs naturally with the SI2 Pages work (one publisher workflow assembling Jekyll + coverage + API docs). Option 2's "spec matches the running binary" property becomes valuable once auth (SI5) and new endpoints (SI7) start changing the API faster; do it as part of SI5 rather than standalone.

**Dependencies/effort:** Option 1 ≈ a day (sidecar repo + ingress route + spec polish is the long pole — the annotation pass is the bulk of the work). No blockers.

---

## SI2 — publish HTML coverage report(s) on the Pages site

### Current state

- Pages: single deployment, one publisher — `jekyll-gh-pages.yml` builds `docs/` → `_site` and deploys on **push to `release`** only. Every deploy *replaces* the whole site.
- Coverage: `code-coverage.yml` produces `genhtml` HTML (`coverage/html/`) and uploads artifact `linux-coverage-report` (**14-day retention**) plus Codacy, on push/PR to all five branches.
- Prior analysis exists: [gh-pages-coverage-integration.md](../gh-pages-coverage-integration.md) (Solutions A/B/C, single-branch `/coverage`) and [html report coverage pages.md](../html%20report%20coverage%20pages.md) (the "upload artifact every run, Pages grabs latest when it deploys" idea).

### Options

**Option A — latest-only `/coverage` (prior Solution A/B).**
One publisher workflow assembles Jekyll site + the single latest coverage artifact at `_site/coverage/`.

- ✅ Simplest; coverage and docs deploy together from one commit.
- ❌ Only reflects whichever branch ran coverage last — usually `development`, so the published site (deployed from `release`) shows mismatched coverage.
- ❌ Doesn't answer the per-branch question at all.

**Option B — per-branch `/coverage/<branch>/` (recommended default: `development` + `release`).**
Keep `code-coverage.yml` as the *producer* (already uploads the artifact every run). Extend the Pages workflow (staying the **only** `deploy-pages` caller) to, before uploading: for each branch in the chosen set, download the latest `linux-coverage-report` artifact from that branch's most recent successful coverage run (`dawidd6/action-download-artifact` with `workflow: code-coverage.yml, branch: <b>` — or `gh api` for exact control) into `_site/coverage/<b>/`, plus a small generated `_site/coverage/index.html` linking each branch report with its run date/SHA.

- ✅ Each environment's report is a few clicks away; `development` = freshest, `release` = shipped state; staging/main are transitory promotion points whose coverage equals their source at promotion time (low marginal value — which is also the honest reading of the task's contradictory "(not main)").
- ✅ Producer/consumer split survives branch-topology changes (SI3) — the branch set is one array in one workflow.
- ❌ Branch reports go stale between deploys (mitigate: also trigger the Pages workflow on `schedule` weekly, and/or on coverage workflow completion).
- ❌ Artifact retention must be raised (14 days → 90) or a quiet branch's report becomes unfetchable.
- ❌ Adds ~N×10–30 MB of genhtml trees per deploy; Pages soft limit is 1 GB — fine for years, but worth noting.

**Option C — all four environment branches (`development`, `staging`, `main`, `release`).**
Same mechanism as B, bigger set.

- ✅ Complete per-environment picture; directly matches the task text's first clause.
- ❌ 2× the artifact traffic and staleness surface for branches that are (by design) near-copies of their source branch; the "(not main)" clause suggests even the author doubts the value.

### Recommendation

**Option B with `{development, release}`** as the default set — freshest state and shipped state are the two reports anyone actually consults; adding branches later is a one-line change. While implementing, fix two adjacent bugs in the same workflow: the Doxygen copy step (`# cp -r ./OdbDesignLib/doxygen/html ./_site/api`) is commented out while `docs/README.md` still links to `/api`, and those README links point at `nam20485.github.io/OdbDesign` although the site's canonical URL is `source.odbdesignserver.com` (`docs/_config.yml`). Either restore the Doxygen copy or fix the links — folding it in here is cheaper than a separate CI change.

**Open decision (flagged):** the branch set — `dev+release` (recommended) vs all four vs the literal "not main" reading (`dev, staging, release`). Mechanism is identical; only the array changes.

**Dependencies/effort:** 1–2 days of workflow work; no C++ changes. Requires `actions: read` (artifact download) alongside existing Pages permissions, and the retention bump.

---

## SI3 — do we need the `main` branch?

### Current state

Flow per `AGENTS.md`: `nam20485 → development → staging → main → release`; `production` is a **defunct legacy branch** (absent from CI triggers, not part of the flow). `main` and `release` were re-baselined onto `development` on 2026-09-08, so today `main ≈ development` tip — the cheapest moment to remove a branch that will ever come. `main` is wired into: ruleset 169360 (required checks incl. `Docker-Build-and-Publish`, code-owner + last-push approvals), the trigger lists of `cmake-multi-platform.yml`, `code-coverage.yml`, `docker-publish.yml`, `sbom-generate-submit.yml`, `dependency-review.yml`, and `docker-scout-scan.yml`'s `compare-to-main` baseline.

### Options

**Option 1 — keep `main` as-is.**
- ✅ Zero risk, zero work; the name is purely cosmetic — it does a real job (the "known-good, release-candidate" checkpoint between `staging` and `release`).
- ❌ One more promotion hop and one more ruleset-covered branch to maintain for no functional difference vs. dropping it.

**Option 2 — drop `main` (flow: `development → staging → release`).**
- ✅ Removes one promotion step; one fewer branch in every trigger list and ruleset; matches "development, staging, release are the only ones we need".
- ❌ Touches: ruleset 169360 branch targets, 5+ workflow trigger lists, `docker-scout` `compare-to-main` baseline, `AGENTS.md`/docs, promotion muscle memory, and any external references to `main`. Each is small; the sum is a coordinated change with real (if bounded) misfire risk — exactly the "nontrivial risk / trivial gain" quadrant the task says to avoid.
- ❌ Loses the pre-release CI signal (a `main` leg of the matrix build ran on the exact bits about to ship).

**Option 3 — rename `main` → `production`.**
- ❌ `production` already exists as a defunct branch, so the rename collides; deleting `production` first adds risk for a second time. Buys nothing over Option 1 or 2. **Rejected.**

### Recommendation

**Option 1 — keep `main`, change nothing.**
The task's own constraint ("don't change unless risk is totally nontrivial") plus the analysis above: the rename is impossible-without-cleanup, the drop is a coordinated multi-system change whose only payoff is one fewer `gh pr merge`. If branch count ever actually hurts, Option 2 is the correct variant *now* (post-re-baseline equivalence makes the diff trivially verifiable) — file it as a deliberate future cleanup, not an urgent one. `production` stays defunct and ignored, per `AGENTS.md`.

**Dependencies/effort:** none (decision only).

---

## SI4 — gRPC TLS

### Current state

gRPC binds plaintext (`InsecureServerCredentials()`, `OdbDesignServerApp.cpp`) and is exposed as a plaintext TCP LoadBalancer Service on :50051 (`deploy/kube/OdbDesignServer/service-grpc.yaml`) — it does **not** pass through the HTTP ingress, so REST ingress TLS changes nothing for it. In the compose deployment, the nginx sidecar already proxies gRPC over TLS :443 (`deploy/nginx/nginx.conf`, h2 `grpc_pass` incl. reflection routes). The REST TLS decision is **OPEN** with a full option matrix in [https-tls-options.md](https-tls-options.md); its §8 already sketches gRPC options and the principle *"decision for gRPC should follow the REST decision (same cert source)"*.

### Options (from https-tls-options.md §8, evaluated)

**Option 1 — Traefik TCP termination (`IngressRouteTCP` :50051 with `tls:`).**
Replace the LoadBalancer Service with a Traefik TCP route presenting the same cert Secret the REST ingress uses (whatever wins the A–E REST decision feeds it).

- ✅ One cert source for everything; no C++ changes; cert renewal stays at the ingress layer (cert-manager/BYO).
- ✅ Clients switch from `-plaintext` to implicit TLS only — no code changes beyond the channel credentials.
- ❌ Traefik-specific (EKS/ALB deployments need their own TCP/NLB story); one more CRD to maintain; h2c backend config needed.

**Option 2 — TLS passthrough (app serves the cert).**
Traefik routes raw TCP; the gRPC server loads the cert/key from mounted secrets.

- ✅ End-to-end encryption inside the cluster (mTLS-ready for SI5).
- ❌ C++ changes (`grpc::SslServerCredentials` + config plumbing), cert distribution/reload in-app; most effort for little gain until mTLS is actually wanted.

**Option 3 — app-native TLS only (`SslServerCredentials`, plaintext never exposed).**
- ✅ Most portable (compose/nginx, k3s, bare metal all identical).
- ❌ Same costs as Option 2 plus deployment config on every target; TLS termination at the edge is the established pattern in both existing deployments (nginx sidecar already does exactly this for compose).

**Option 4 — status quo: plaintext on the trusted network, terminate at an edge for public exposure.**
- ✅ Zero work; matches today's reality (tailnet/LAN-only exposure).
- ❌ Leaves an easy hardening step undone; any accidental direct exposure of :50051 is unauthenticated *and* unencrypted (SI5 makes the unauthenticated part worse, not better).

### Recommendation

**Option 1 — Traefik `IngressRouteTCP` termination sharing the REST cert Secret**, decided and implemented *together with* the REST TLS decision (don't pick a gRPC cert source now that might diverge from it). Keep the app plaintext-in-cluster; revisit passthrough/app-native TLS only if/when SI5b wants mTLS for machine clients. Note for implementation: `grpcurl` then drops `-plaintext`; reflection already routes through the proxy in the compose nginx config, which is the reference for the Traefik router shape.

**Dependencies/effort:** blocked on the REST TLS decision ([https-tls-options.md](https-tls-options.md) open questions §10); ~1 day once decided.

---

## SI5a — unified authentication (more secure than Basic; currently nothing on gRPC)

### Current state

Basic auth per-route on REST with **hardcoded fallback credentials** (`odb`/`plusplus`) when env vars are unset (`BasicRequestAuthentication.cpp`); auth is an explicit call inside every handler lambda (easy to miss on a new route); gRPC has none — any network-reachable client can read every design. CORS is `*` in all environments. These are the concrete defects any option must fix: single shared password, plaintext-equivalent basic credentials on every request, no identity (no way to say *who*), no coverage of gRPC, no revocation.

### Options

**Option 1 — self-hosted OIDC provider (Keycloak) on the k3s cluster; JWT bearer tokens everywhere.**
Keycloak realm holds users/clients; the server validates access tokens: a Crow auth middleware (`crow::Crow<CORSHandler, AuthMiddleware>` — replaces the per-handler `AuthenticateRequest` calls) checking `Authorization: Bearer` against Keycloak's JWKS (e.g. `jwt-cpp`, available as a vcpkg port), and a gRPC sync interceptor doing the same for the `authorization` metadata key. `/healthz*` stays anonymous. `--disable-authentication` stays as the local/dev escape hatch.

- ✅ Real identity model (users, groups, roles) — the prerequisite for SI5b's per-resource authorization.
- ✅ Device flow, token refresh, revocation, and machine clients come **built-in** (Keycloak implements the OAuth 2.0 Device Authorization Grant) — SI5b's "machine → server" requirement is configuration, not code.
- ✅ Fits the deployment direction: Keycloak on k3s managed via the Argo CD GitOps repo like every other workload.
- ✅ Standard, auditable protocol; every language has an OIDC client library ("non-language-specific", same requirement as SI8).
- ❌ One more stateful service to run (Postgres + Keycloak, ~1 GB RAM) — the real cost.
- ❌ First-time setup (realm, clients, flows) is a day of yak-shaving; local dev needs a fallback path (`--disable-authentication` already exists).

**Option 2 — server-issued tokens, no new infrastructure.**
The C++ server issues and validates its own signed JWTs (or long-lived API keys); an identity/ownership store (see SI5b) maps token → user.

- ✅ Zero new services; simplest deployment story for FOSS consumers of the repo.
- ❌ You own the crypto and auth-code correctness forever (issuance, rotation, expiry, alg confusion, key storage) — a standing security-maintenance liability for a solo-maintained project, and the user's security posture explicitly favors not hand-rolling this.
- ❌ Device flow must be implemented from scratch (verification URIs, polling, rate-limited codes) — weeks of careful code that Keycloak already has.

**Option 3 — managed external IdP (Auth0/Logto/…).**
- ✅ Least operational work; polished UX.
- ❌ External dependency + cost for an AGPL FOSS project; design data metadata (usernames, tokens) transits a third party; free tiers put ceilings exactly where a growing deployment hits them.

**Option 4 — keep Basic, harden it (per-user credentials, constant-time compare, HTTPS-only).**
- ✅ Smallest diff; kills the hardcoded fallback immediately.
- ❌ Still one factor shared per user, still no token semantics, still nothing for gRPC unless duplicated; doesn't satisfy SI5b at all. Worth doing *regardless* as a stopgap (below), not as the answer.

### Recommendation

**Option 1 — Keycloak on k3s, JWT bearer validated in a Crow middleware + a gRPC interceptor.**
It is the only option that satisfies SI5a and SI5b (identity, per-resource decisions, device flow, machine clients) without hand-rolled security code, and it rides the existing GitOps deployment pattern. Sequence: (1) stopgap hardening now — remove the `odb`/`plusplus` fallback, fail closed when env creds are missing, move auth from per-handler calls to one middleware, tighten CORS to configured origins; (2) introduce Keycloak + bearer validation with a transition window where Basic still works; (3) remove Basic. Option 4's stopgaps happen on day one regardless of the final choice.

**Dependencies/effort:** stopgaps ~1 day; Keycloak deployment + realm ~2–3 days; middleware/interceptor ~2–4 days. Interacts with SI4 (tokens over TLS) and SI1 (spec security schemes).

---

## SI5b — per-user, per-resource authorization + device flow (gRPC and REST)

### Current state

No identity exists (see SI5a), hence no authorization: every authenticated caller can read every design, upload anywhere, and both channels are equivalent-or-nothing ("both communication channel type AND specific data resources" from the task is currently unsatisfiable). The resource set is small: designs (archives on disk / in `DesignCache`), uploads, and channel-level actions.

### Options

**Option 1 — ownership metadata + centralized enforcement (recommended).**
Each design gets an ownership record — start as a JSON sidecar next to the archive (`<design>.tgz.owners.json`) or a small index file; graduate to SQLite only if concurrent-write pain appears. Record: `owner`, `shared: all | none`, optional explicit user list. Enforcement lives in **one** place per channel: a check in `RouteController`/auth middleware for REST and a helper in `OdbDesignServiceImpl` for gRPC, keyed off the identity established by SI5a (`sub` claim). Roles from the IdP: `admin` (everything) vs `user` (own + shared). Uploads stamp the authenticated user as owner; a migration rule marks pre-existing designs `owner=admin, shared=all` so nothing breaks on rollout.

- ✅ Directly models the task's rule ("only designs you have uploaded or are shared to all users"); works identically over REST and gRPC because it sits behind identity, not the channel.
- ✅ Trivially inspectable/debuggable sidecar files; no schema migration ceremony.
- ❌ Enforcement is only as good as its placement — every new route/RPC must go through the two choke points (mitigate: make the check part of the SI5a middleware/interceptor, not a convention).
- ❌ Sidecar files need atomic writes and to travel with the archive (backup/copy story).

**Option 2 — per-user directory namespaces (`designs/<user>/…`).**
- ✅ Authorization becomes path structure; almost no new code.
- ❌ Breaks "shared to all" (needs a union listing anyway), changes every existing path/URL/RPC name assumption, and leaks usernames into the API surface. Rejected as primary, useful later as an optional organizer *on top of* Option 1's metadata.

**Option 3 — externalized authorization (OPA sidecar / SpiceDB).**
- ✅ Policy-as-code, powerful for complex sharing graphs.
- ❌ Two more services and a policy language to operate, for a resource model with two rules. Overkill now; revisit if sharing grows into real ACLs/groups.

**Device flow (machine → server).** With Keycloak (SI5a Option 1) this is configuration: enable the Device Authorization Grant on the realm, register a public client for CLIs/agents, and machines do the standard `device_authorization_endpoint` → user approves in browser → token poll dance. REST/gRPC clients just send the resulting bearer token like interactive clients. Without an IdP this is a from-scratch protocol implementation — the strongest single argument for Keycloak.

### Recommendation

**Option 1 + Keycloak device flow, sequenced strictly after SI5a.**
The ownership store is intentionally boring (JSON sidecar first), the enforcement points piggyback on SI5a's middleware/interceptor so there is exactly one place per channel to get it right, and the migration rule (`shared=all`) keeps today's behavior intact at rollout. REST and gRPC get identical decisions from the same claim set, satisfying the "both channel type and data resources" requirement by construction.

**Dependencies/effort:** ~3–5 days after SI5a lands (store + choke points + migration + tests). The upload path also gains its missing `sanitizeFilename` implementation while it's being touched (`FileUploadController.cpp` TODO) — a pre-existing security gap in the same code.

---

## SI6 — server-side optimization options

### Current state

Client-side fetch→load→display is already >10× faster; the remaining costs are server-side per request: (a) every REST response re-serializes protobuf → JSON (`to_json()`) even for unchanged designs — the dominant CPU cost on large designs; (b) first-touch parse blocks the calling request for minutes; (c) list endpoints rescan the designs directory per call; (d) `DesignCache` never evicts — memory grows monotonically; (e) gRPC matrix/bulk surface is thinner than REST's. Gzip is already on for both channels; gRPC batch streaming already exists (feature-flagged).

### Options (ranked)

**1. Serialized-response cache.**
At design-load time (and on invalidation), pre-serialize the hot payloads — protobuf bytes for gRPC, JSON strings per endpoint for REST (`/filemodels/<d>`, `/designs/<d>`, `…/matrix/matrix`, `…/layers/<l>/features`, …) — and serve bytes from cache.

- ✅ Removes the repeated `to_json()` cost entirely for warm designs (the #1 server CPU cost); combines naturally with SI7 (parse once → serialize once → serve N times).
- ❌ Memory cost (JSON strings are big; needs SI6.3's eviction to stay bounded); cache invalidation on re-upload/reload must be airtight.

**2. HTTP conditional requests: ETag + `Cache-Control`.**
ETag = cheap hash of (design file mtime, size, endpoint); honor `If-None-Match` → `304`. Send `Cache-Control: private, max-age=…` for immutable design payloads.

- ✅ Tiny change, big win for repeat clients (visibility toggles refetching the same features payloads); works through proxies; no memory cost.
- ❌ Only helps clients that send conditional requests (the optimized client already avoids refetching — measure first).

**3. Cache eviction (byte-budget LRU) + single-flight in-flight dedup.**
Cap `DesignCache` by estimated bytes; evict least-recently-served. Deduplicate concurrent misses on the same design (one parse, others wait) via a `shared_future` map — required once SI7 makes background loads possible.

- ✅ Bounds memory (today it's unbounded); kills thundering-herd double-parses; prerequisite for SI7's async loads.
- ❌ Eviction policy tuning (a design evicted mid-session re-parses on next touch — minute-scale).

**4. Filesystem watcher (inotify) for the designs directory.**
Replace per-call directory rescans with a watch-driven cached listing; optionally detect replaced archives → invalidate caches proactively.

- ✅ Cheap; removes scans from every list call; makes the swagger-documented "watches a designs directory" claim true.
- ❌ Platform code (inotify/`std::filesystem` polling fallback for network mounts/PVCs — k8s PVCs may not deliver inotify events reliably; a slow-poll fallback is mandatory).

**5. Pagination / field projection on heavy endpoints.**
`?offset=&limit=` and/or sparse fieldsets on the large JSON endpoints.

- ✅ Helps memory-constrained clients; standard API hygiene.
- ❌ The optimized client already streams layers incrementally; server-side pagination adds complexity for a client that may not need it. Demand-driven only.

**6. gRPC surface parity (matrix, bulk fetch).**
Add e.g. `GetMatrix`, and batch variants of the heavy unary RPCs so gRPC clients get the same shapes REST clients have.

- ✅ gRPC clients (binary, compressed, streaming-friendly) then avoid the JSON path entirely — often the biggest end-to-end win; proto messages already exist (`matrixfile.proto` et al.).
- ❌ New RPCs to spec/maintain (SI1 docs), duplicate surface area.

### Recommendation

**Do 1 + 2 first (measure before/after with a large design), fold 3 into SI7 (it's a prerequisite there anyway), do 4 opportunistically, keep 5 and 6 demand-driven.**
The ordering follows cost/benefit: response caching + ETags attack the only costs that recur on *every* request; everything else is a first-touch or listing cost. When SI19.1.3 (arena allocation) and option 1 land together, the whole serialize-once story covers both channels.

**Dependencies/effort:** (1) ~3–5 days incl. invalidation tests; (2) ~1 day; (3) counted in SI7; (4) ~1–2 days with the PVC fallback; (5)/(6) sized on demand.

---

## SI7 — `RequestLoadDesign(Async)`: kick a load, return immediately

### Current state

`DesignCache::GetDesign/GetFileArchive` parse **synchronously in the requesting call** — a client's first `GetDesign` for a big design blocks for the parse duration (minute-scale; the k8s probe comments budget for it). There is no `RequestLoadDesign`/`GetDesignAsync` RPC or REST equivalent today. The task: start the parse, return immediately; subsequent `GetDesign` for that design hits the warm cache and returns data immediately.

### Options

**Option 1 — background single-flight load + `RequestLoadDesign` RPC (+ REST twin + auto-warm on upload).**
Add to `DesignCache` an in-flight map (`design name → std::shared_future`) so exactly one parse runs per design regardless of how many callers request it. Expose:

- gRPC: unary `RequestLoadDesign(design_name) returns RequestLoadDesignResponse { status: Accepted | AlreadyLoading | AlreadyLoaded | NotFound, design_name }` — kicks the background load, returns instantly.
- REST twin: `POST /designs/<name>/load` → `202 Accepted` with the same status body (keeps the channels feature-equivalent — SI5b's requirement, and cheap once the cache layer exists).
- Auto-warm: `FileUploadController` kicks the same background load after `fastmove_file` — uploads pay the parse cost at upload time instead of first-read time.

`GetDesign` semantics unchanged: warm → immediate data; cold → still blocks (correct, just no longer the only path) or — later, if clients want it — returns a distinct "loading" error with retry guidance.

- ✅ Directly implements the task; single-flight prevents duplicate minute-scale parses under concurrent first-touch; no changes to existing callers.
- ✅ Pairs with SI6.1: after parse completes, pre-serialize the hot payloads in the same background task — first `GetDesign` is *also* serialization-free.
- ❌ Background threads touch `FileArchive`/`Design` construction — thread-safety review needed at the cache boundary (Phase-1 gRPC work already established the pattern).
- ❌ Memory pressure from eager loads → requires SI6.3 eviction in the same change, or unbounded growth gets worse (now designs get loaded that nobody requested).

**Option 2 — progress streaming too (`LoadDesignProgress` server-streaming RPC).**
Stream parse progress events to interested clients while the background load runs.
- ✅ Great UX for minute-scale loads; natural fit for the SI8 events channel (one mechanism, two uses).
- ❌ Parse code must emit progress (it currently doesn't); more surface than the task asks for. **Defer** — design the background-load state machine so progress *can* be added without rework.

**Option 3 — startup `--load-all` for known sets.**
Already exists (`OdbAppBase`), used by no deployment: unbounded memory, no selectivity. Not the mechanism, though worth documenting as the "small fixed set of designs" answer.

### Recommendation

**Option 1, with SI6.3 (eviction + single-flight) built as part of it and Option 2 deferred-but-not-precluded.**
This is the item where SI6 and SI7 genuinely merge: single-flight is the concurrency backbone for both, and pre-serialization after background load makes "request → instant data" true end-to-end. Ship `RequestLoadDesign` + `POST /designs/<name>/load` + upload auto-warm together; add progress via SI8's channel when it exists.

**Dependencies/effort:** ~4–6 days including tests and the eviction coupling. Touches `DesignCache`, `service.proto` (+ codegen), `OdbDesignServiceImpl`, `FileUploadController`, and the swagger spec (SI1).

---

## SI8 — optional push channel (SSE? WebSockets?) with mandatory fallback

### Current state

No SSE or WebSocket usage anywhere (verified: zero hits for websocket/event-stream in server sources). The only server-initiated streaming is gRPC server-streaming (`GetLayerFeaturesStream`/`BatchStream`) — proven in production. **Framework fact that gates the options:** the pinned Crow 1.3.3 has **native WebSocket support** but **no chunked/SSE response API** (verified in the vendored headers; upstream CrowCpp/Crow#1013 tracks chunked/SSE). The task's requirement is that the channel be *optional*: the app must function unchanged if the connection is never attempted or fails mid-flight.

### Options

**Option 1 — WebSockets (Crow-native) as an events channel.**
A `/events` WS route (Crow `websocket_handler`) pushing small typed JSON events: `design-load-started/finished/failed`, `design-list-changed` (upload), optionally `server-shutting-down`. Clients feature-detect: try WS, and if it fails or drops, keep polling REST exactly as today — the fallback is the status quo, so "app still functions without it" holds by construction.

- ✅ Supported by the framework *today* (no Crow upgrade); bidirectional (leaves room for client→server subscriptions, e.g. "watch design X"); works from browsers natively.
- ✅ Small, discrete events are a different traffic shape than bulk data — doesn't compete with the gRPC streams.
- ❌ WS needs proxy awareness: Traefik over the HTTP ingress handles upgrades, but timeouts must be reviewed, and the compose nginx needs an explicit WS location block (`Upgrade`/`Connection` headers, raised `proxy_read_timeout`) — the current `nginx.conf` doesn't have one.
- ❌ Hand-rolled event schema (no auto-reconnect/event-id semantics like SSE gives for free) — client must implement reconnect + resume.

**Option 2 — SSE.**
- ✅ Simplest client story (EventSource, auto-reconnect, `Last-Event-ID`), unidirectional is exactly right for notifications, works over plain HTTP proxies.
- ❌ **Blocked on the pinned Crow 1.3.3** — no chunked/streaming response API (CrowCpp/Crow#1013 still open). Would require upgrading/patching Crow (a vcpkg baseline bump with everything that entails) or a sidecar translating gRPC streams into SSE. Revisit only when a Crow upgrade happens anyway.

**Option 3 — gRPC server-streaming as the push channel.**
Extend the existing streaming machinery with e.g. a `SubscribeDesignEvents` stream; non-browser clients get events with zero new infrastructure.
- ✅ Already-proven path in this codebase; strong typing via proto; compression/keepalive already configured.
- ❌ Browsers need grpc-web + a translating proxy (Envoy) — new moving parts at the edge; heavier than a WS for browser clients. Right answer for *native* clients, not the universal one.

### Recommendation

**Option 1 (WebSockets) for the browser-facing events channel + Option 3 already covers native/bulk clients; SSE is deferred until a Crow upgrade lands for other reasons.**
The fallback requirement decides the architecture more than the transport choice: whatever is built must be a pure optimization layer over the existing polling REST API, never a dependency — that invariant should be written into the client contract (and the SI1 spec) explicitly. SI7's load events are the first payload this channel should carry, which sequences SI8 after (or with) SI7.

**Dependencies/effort:** ~3–5 days (route + event schema + reconnect semantics + proxy config for both deployment shapes).

---

## SI19 — remaining phase-2 gRPC optimization items

### Current state (verified 2026-09-09)

[phase2-grpc-optimizations.md](opt/phase2-grpc-optimizations.md) specced three items behind PR #498 (`feature/grpc`, merged 2026-02-14). **None of the three landed** — PR #498 evidently carried only the phase-1 work:

| Item | Spec | Status in code |
|---|---|---|
| 2.1 — `cc_enable_arenas = true` in all `.proto` files | 27 files | **Not done** — 0/27 protos carry the option; stale `//option optimize_for = CODE_SIZE;` comments remain |
| 1.3 — arena allocation in streaming RPCs (`to_protobuf(TPbMessage*)` overload, arena in `GetLayerFeaturesStream`/`BatchStream`) | hot-path rework | **Not done** — no arena usage or pointer-population overload in `IProtoBuffable.h` / `OdbDesignServiceImpl` |
| 2.2 — compression *level* config replacing the boolean | config surface | **Not done** — `GrpcServiceConfig.h` still `bool compression_enabled`; `config.json` still `compression: {enabled: true, algorithm: "gzip"}` |

The phase-2 doc is implementation-grade (per-file change lists, acceptance criteria, verified-against-headers API notes) — this section decides *whether/when*, not *how*.

### Options

**Option A — execute all three as specced (2.1 → 2.2 → 1.3).**
- ✅ Completes the specced round; each item is independently testable; order respects the 2.1→1.3 dependency.
- ❌ 1.3 is the large/riskiest item (virtual overload on `IProtoBuffable`, hot-path lifetime rules) with no post-SI6 measurement justifying it *yet*.

**Option B — cheap pair now (2.1 + 2.2), defer 1.3 until measured.**
- ✅ 2.1 and 2.2 are small, low-risk, wire-compatible (full rebuild only); 2.2 replaces the boolean with level-based negotiation, which the phase-2 doc argues well and which future-proofs the config for the LAN-vs-WAN split.
- ❌ Leaves the streaming alloc optimization (the actual perf item) unimplemented.

**Option C — fold 1.3 into the SI6 optimization round.**
1.3 and SI6.1 (serialized-response cache) are complementary, not overlapping: arenas cut **per-message allocation** in streaming RPCs; the response cache cuts **re-serialization** of unary/poll responses. Doing them together means one measurement campaign, one before/after benchmark on a large design, and one review of the serialization hot path.
- ✅ Avoids optimizing blind; the two changes cover both channels' dominant costs in one pass.
- ❌ Couples an already-specced item to a not-yet-specced one (SI6.1 has no implementation doc yet).

### Recommendation

**B + C hybrid: land 2.1 and 2.2 now as standalone PRs (they're finished specs sitting idle); sequence 1.3 as part of the SI6.1 work with a shared benchmark.**
One delta the phase-2 spec must absorb when 2.2 is implemented: `config.json` has since grown an `algorithm: "gzip"` field alongside `enabled` — the rewrite replaces *two* fields with `level`, and the migration note in the spec should say so.

**Dependencies/effort:** 2.1 ≈ 1–2 h; 2.2 ≈ 1–2 h (+ spec delta); 1.3 ≈ 4–8 h, scheduled with SI6.1. All are C++/proto changes → feature branch → `nam20485` per the merge flow, protobuf regeneration wipes build dir (see workspace notes on major-bump gencode).

---

## Cross-cutting sequencing

```text
now, independent:   SI1 (redoc + annotations)     SI2 (pages coverage)     SI19.2.1/2.2 (cheap pair)
decision-gated:     SI4 (gRPC TLS)  ──after──►  REST TLS decision (https-tls-options.md, still OPEN)
identity track:     SI5a (Keycloak + middleware/interceptor) ──► SI5b (ownership + device flow)
perf track:         SI6.3+SI7 (single-flight, background load, eviction, auto-warm)
                       └─ with ─► SI6.1 (serialized-response cache) + SI19.1.3 (arenas) — one benchmark
events track:       SI8 (WS events channel) ──after/with──► SI7 (its first events are load lifecycle)
```

Two coupling rules worth stating explicitly:

1. **SI6.1, SI7, and SI19.1.3 all touch the same serialize/parse hot path.** Plan them as one measurement campaign even if they ship as separate PRs; otherwise each change invalidates the others' benchmarks.
2. **SI5a must precede SI5b and should precede any public exposure** enabled by SI4 — TLS without authentication still leaves every design readable by anyone who can reach the port.

## Open decisions

| # | Decision | Status |
|---|---|---|
| 1 | SI2 branch set for published coverage: `development+release` (recommended) vs all four vs "not main" literal three | **Open** — task text self-contradictory ("…main, release (not main)") |
| 2 | SI5 IdP: Keycloak-on-k3s (recommended) vs server-issued tokens vs managed IdP | **Open** — recommendation contingent on accepting one new stateful service |
| 3 | REST TLS option A–E (prerequisite for SI4's shared-cert plan) | **Open** — tracked in [https-tls-options.md](https-tls-options.md) §10 |
| 4 | SI3 branch topology: keep `main` (recommended) | Effectively decided unless branch-count pain materializes |
