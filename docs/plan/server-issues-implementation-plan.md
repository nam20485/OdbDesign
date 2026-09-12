# OdbDesignServer — implementation plan (decisions applied)

| | |
|---|---|
| Status | **ACTIVE — executing.** All decisions recorded (incl. REST TLS cert source = **Option C**, 2026-09-10 follow-up). First tranche on `nam/server-issues-impl`: Phase 0 + M3.1. |
| Source | [server-issues.md](server-issues.md) (options + pros/cons + decisions) — this doc turns those decisions into sequenced, file-level work. |
| Related | [https-tls-options.md](https-tls-options.md) (REST TLS — **DECIDED: Option C**, 2026-09-10) · [opt/phase2-grpc-optimizations.md](opt/phase2-grpc-optimizations.md) (SI19 implementation spec) · [gh-pages-coverage-integration.md](../completed/gh-pages-coverage-integration.md) |
| Method | Every milestone = one `nam/<feature>` branch → PR → `nam20485`, merged with **merge commits only** (AGENTS.md, directive 2026-09-10). C++ milestones ride the multi-platform PR builds; docs/workflow milestones ride the static checks. |

---

## Decisions being implemented

| SI | Decision (from [server-issues.md](server-issues.md) NOTE blocks) | Milestones |
|---|---|---|
| SI1 | Option 1 now — `redoc.html` in the swaggerui sidecar; annotation pass in the single swagger YAML. Spec-provenance question answered in [§SI1-A](#si1-a-spec-provenance-curation--generation) below. | M0.3, M0.4 |
| SI2 | Option B, branch set **`{development, release}`**. | M0.5 |
| SI3 | Keep `main`; no changes. | — (none) |
| SI4 | **Decided 2026-09-10: Option 1** — Traefik `IngressRouteTCP` termination, app plaintext in-cluster. Cert source **decided: Option C** (cert-manager private CA, IP SANs) — M3.1 unblocked. | M3.1 |
| SI5a | Keycloak on k3s + JWT bearers (Crow middleware + gRPC interceptor); stopgap hardening first. | M2.1–M2.3, M2.5 |
| SI5b | Option 1 — ownership sidecar metadata enforced at two choke points; device flow via Keycloak; `sanitizeFilename` fix included. | M2.4 |
| SI6 | Items **1 (serialized-response cache) + 2 (ETag/Cache-Control)**; item 3 via SI7; item 4 opportunistic; 5/6 demand-driven. | M1.3, M1.4, M1.5 |
| SI7 | Option 1 — single-flight background load + `RequestLoadDesign` RPC + `POST /designs/<name>/load` + upload auto-warm; progress deferred to SI8. | M1.1, M1.2 |
| SI8 | Option 1 — WebSockets events channel with polling fallback by construction. | M3.2 |
| SI19 | B+C hybrid — 2.1 + 2.2 now as standalone PRs; 1.3 (arenas) sequenced with SI6.1 under one benchmark. | M0.1, M0.2, M1.4 |

Phase overview (dependencies, not calendar):

```text
Phase 0 (now, all parallel, no interdeps):
  M0.1 proto arenas option      M0.2 compression level     M0.3 /redoc sidecar
  M0.4 spec annotation pass     M0.5 Pages per-branch coverage

Phase 1 (perf track, strictly ordered at the cache boundary):
  M1.1 single-flight + LRU eviction  ──►  M1.2 background load + RequestLoadDesign
  M1.3 ETag/Cache-Control (any time)      M1.4 response cache + arenas (one benchmark) ──► M1.5 watcher (optional)

Phase 2 (identity track, strictly ordered):
  M2.1 auth stopgaps ──► M2.2 Keycloak on k3s ──► M2.3 JWT middleware + interceptor
                       ──► M2.4 ownership + device flow ──► M2.5 Basic removal

Phase 3 (edge & events):
  M3.1 gRPC TLS (Option C — unblocked, first tranche)     M3.2 WS /events (after M1.2)
```

---

## Phase 0 — quick wins (parallel)

### M0.1 — SI19.2.1: enable protobuf arenas in all `.proto` files

- **Branch:** `nam/grpc-arenas-option`
- **Spec:** [phase2-grpc-optimizations.md](opt/phase2-grpc-optimizations.md) Item 2.1 (implementation-grade; follow it verbatim).
- **Files:** all 26 protos under `OdbDesignLib/protoc/` + `OdbDesignServer/protoc/grpc/service.proto` — add `option cc_enable_arenas = true;` after `syntax`, delete the stale `//option optimize_for = CODE_SIZE;` comments.
- **Notes:** wire-compatible, no behavior change. Regeneration wipes the build dir (keep `vcpkg_installed/`) per the protobuf-gencode workspace rule.
- **Accept:** full build (all presets); all tests pass; `grep -L cc_enable_arenas **/*.proto` returns nothing.
- **Effort:** 1–2 h.

### M0.2 — SI19.2.2: compression level config

- **Branch:** `nam/grpc-compression-level`
- **Spec:** phase-2 doc Item 2.2, **plus the recorded delta**: `OdbDesignServer/config.json` now has `compression: {enabled: true, algorithm: "gzip"}` — the rewrite replaces **two** fields with `compression.level` (`"none"|"low"|"medium"|"high"`, unknown → `"high"`), not one.
- **Files:** `OdbDesignServer/Config/GrpcServiceConfig.h/.cpp` (`grpc_compression_level` replaces `bool compression_enabled`; parse helper), `OdbDesignServer/OdbDesignServerApp.cpp` (`SetDefaultCompressionLevel` replaces `SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP)` + startup log), `OdbDesignServer/config.json`, `OdbDesignTests/GrpcServiceConfigTests.cpp` (four level strings + unknown fallback).
- **Accept:** per phase-2 doc acceptance criteria, updated for the `algorithm`-field removal.
- **Effort:** 1–2 h.

### M0.3 — SI1: `/redoc` page in the swaggerui sidecar

- **Branch:** sidecar repo change (image content) + this repo: `nam/redoc-route`
- **This repo:** `deploy/kube/local-ingress.yaml` — add `/redoc` path route → `odbdesign-server-swaggerui` service (mirror the existing `/swagger` block at lines 19–25); `deploy/nginx/nginx.conf` — add a `location /redoc` (or extend the swagger-ui catch-all at 153–165) serving the new page. No `deploy.ps1` change needed — the spec ConfigMap pipeline already refreshes both UIs.
- **Sidecar repo:** add `redoc.html` using a **vendored, pinned** `redoc-standalone` bundle (no CDN dependency — deployments are tailnet/LAN, may have no egress) pointing at the mounted spec path (`/spec/odbdesign-server-0.9-swagger.yaml`), cross-linked from swagger-ui and vice versa.
- **Accept:** `https://<ingress>/redoc` renders definitions from the ConfigMap spec; `/swagger` unchanged; works in both k3s and compose shapes.
- **Effort:** ~0.5 day (the annotation pass below is the long pole, not this).

### M0.4 — SI1: annotation pass on `swagger/odbdesign-server-0.9-swagger.yaml`

- **Branch:** `nam/swagger-annotations`
- **Work** (all in the single YAML — both UIs consume it):
  1. Property-level `description:` on every schema under `components/schemas`, sourced from the comments in `OdbDesignLib/protoc/*.proto`.
  2. Complete error-response sets per operation (`400`/`401`/`403`/`404`/`500`).
  3. `example`s on all large schemas (`StepDirectory`, `LayerDirectory`, `FeaturesFile`, `MatrixFile`, `Design`, `FileArchive`).
  4. Add the SI7/SI8 endpoints to the spec as they land (M1.2 `POST /designs/<name>/load`, M3.2 `/events` note); swap security schemes when M2.3 lands.
- **Validation:** `npx @redocly/cli lint` (or `swagger-cli validate`) in CI as a docs job — catches YAML/schema errors the C++ builds can't see (same rationale as `Analyze (actions)` for workflow edits).
- **Accept:** ReDoc Definitions panel fully described; linter passes with no errors (warnings triaged).
- **Effort:** 1–2 days (mostly transcription from proto comments — agent-shaped work).

#### SI1-A: spec provenance (curation + generation) — answer to the open question

The recorded question: *can the spec be dynamically generated from source or live endpoints instead of hand-curated, and where does Kiota fit?*

1. **From source code: no realistic path in Crow.** Crow registers routes at runtime (`route_dynamic`/`CROW_ROUTE`); it has no annotation/reflection facility for OpenAPI, and no generator exists for it. The C++ frameworks that do generate OpenAPI from code (Drogon ≥1.9 via `drogon_ctl`, oat++ line) require a framework migration — not worth it for this. The proto layer *is* generatable (protoc already emits `*_pb2.py`-style descriptors) — and that's exactly what the current spec's schema blocks already mirror. The hand-curated YAML is the right source of truth for this codebase.
2. **From live endpoints: possible but strictly worse.** Traffic-capture tools (`mitmproxy2swagger`, `har-to-openapi`) infer coarse schemas from observed JSON — no descriptions, wrong-guess types for proto3's omitted-defaults, no enums/constraints. They would *destroy* the current detail level. Useful only as a verification signal, not a generator.
3. **Kiota: wrong direction for this problem.** Kiota is an OpenAPI *consumer* — it generates API clients from a spec, not specs from code. Once the spec is where we want it (M0.4), Kiota is an excellent way to generate typed clients for the server, and worth a look then. It cannot help produce the spec.
4. **The pragmatic best-of-both: drift checking (adopted).** Keep the curated YAML as source of truth, and add a CI check that the *route surface* can't silently drift from it:
   - Add a build-flag-gated `GET /debug/routes` endpoint that dumps Crow's registered routes (Crow exposes the router's rules — enumerate and serialize as JSON), OR a small script parsing `CROW_ROUTE`/`route_dynamic` registrations from `Controllers/*.cpp`.
   - CI job diffs that route list against the spec's `paths:` — new/removed/renamed endpoints fail the check.
   - Schema-level drift (message shape vs spec components) stays manual for now; revisit if it bites.
   This keeps the annotation richness and gets the generator's main benefit (no silent staleness). Fold into M0.4's CI lint job.

### M0.5 — SI2: per-branch coverage on Pages (`{development, release}`)

- **Branch:** `nam/pages-coverage`
- **Constraint (from [gh-pages-coverage-integration.md](../completed/gh-pages-coverage-integration.md)):** Pages = one deployment; **only `jekyll-gh-pages.yml` may call `deploy-pages`** — it stays the sole publisher.
- **Changes, all in `.github/workflows/jekyll-gh-pages.yml`:**
  1. Keep triggers (`push: release`, `workflow_dispatch`); add `schedule: weekly` (staleness refresh).
  2. New step before `upload-pages-artifact`: for each branch in `[development, release]` — find that branch's latest **successful** `code-coverage.yml` run (`gh run list --workflow=code-coverage.yml --branch=<b> --status=success --limit=1 --json databaseId,headSha,createdAt`) and `gh run download <id> -n linux-coverage-report`, extract `html/` → `_site/coverage/<b>/`. (Degrade gracefully: missing/stale artifact → publish a placeholder page saying so, never fail the docs deploy.)
  3. Generate `_site/coverage/index.html` linking each branch report with its run SHA + date.
  4. Permissions: add `actions: read` to the existing `contents: read`, `pages: write`, `id-token: write`.
  5. Un-comment the Doxygen copy step (`cp -r ./OdbDesignLib/doxygen/html ./_site/api`) — `docs/README.md` links there — and fix those README links to the canonical `https://source.odbdesignserver.com`.
  6. Add a "Code coverage" link to the Jekyll index (`docs/index.md`).
- **Also:** `.github/workflows/code-coverage.yml` — artifact retention 14 → **90 days** (a quiet branch must stay fetchable between release-window deploys).
- **Never deploy from PRs** (fork or otherwise) — keep deploy steps gated on the existing triggers.
- **Accept:** after a `release` push: `https://source.odbdesignserver.com/coverage/` shows `development/` and `release/` reports with correct SHAs; site otherwise unchanged.
- **Effort:** 1–2 days.

---

## Phase 1 — performance track (touches the DesignCache hot path; ordered)

### M1.1 — SI6.3: single-flight loads + byte-budget LRU eviction in `DesignCache`

- **Branch:** `nam/design-cache-singleflight`
- **Depends:** nothing (start of the perf track).
- **Files:** `OdbDesignLib/App/DesignCache.h/.cpp`; new `OdbDesignLib/App/DesignCacheConfig` (or extend `OdbDesignArgs`): `--cache-max-mb` (default e.g. 4096).
- **Steps:**
  1. In-flight map: `std::unordered_map<std::string, std::shared_future<std::shared_ptr<…>>>` (two: archives + designs) guarded by a dedicated mutex. `GetDesign`/`GetFileArchive` miss → emplace future (parse runs once; concurrent callers `.wait()` on the same future) → on completion, move into the existing maps and erase the in-flight entry. Preserve the current double-checked-locking semantics for readers.
  2. Byte accounting: estimate per-design bytes (archive file size + serialized response sizes once M1.4 exists; file size until then) stored alongside each entry; LRU list on last-served time; evict from the front on insert over budget. Never evict an in-flight or failed entry.
  3. State machine hooks: per-design state (`Unloaded | Loading | Loaded | Failed`) + an observer callback list (load started/finished/failed) — M1.2's RPC, M3.2's events, and M1.5's invalidation all hang off this; design it now so they're additions, not rewrites.
- **Tests:** concurrent `GetDesign` on a cold design parses exactly once (instrument or count via test seam); eviction respects budget; failed parse doesn't poison the cache (retry works).
- **Thread-safety review:** this is the boundary change everything else depends on — review `FileArchive`/`Design` construction for cross-thread safety (phase-1 gRPC work established the pattern).
- **Effort:** 2–3 days.

### M1.2 — SI7: background load + `RequestLoadDesign` + REST twin + upload auto-warm

- **Branch:** `nam/request-load-design`
- **Depends:** M1.1 (in-flight map + state machine).
- **Files:**
  - `OdbDesignServer/protoc/grpc/service.proto`: `RequestLoadDesign(RequestLoadDesignRequest{design_name}) returns (RequestLoadDesignResponse{status, design_name})`; `enum LoadStatus { ACCEPTED = 0; ALREADY_LOADING = 1; ALREADY_LOADED = 2; NOT_FOUND = 3; }`.
  - `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`: `RequestLoadDesign` = state-machine lookup → if `Unloaded`, kick `DesignCache::LoadDesignAsync` (async, does not wait) → return status. `GetDesign` semantics unchanged (warm = data; cold = blocks — now no longer the only path).
  - `OdbDesignServer/Controllers/DesignsController.cpp`: `CROW_ROUTE … /designs/<string>/load` (POST) → `202 Accepted` + same status JSON.
  - `OdbDesignServer/Controllers/FileUploadController.cpp`: after `fastmove_file`, fire-and-forget `LoadDesignAsync` (log failures; never fail the upload response on warm failure).
  - Config: `--max-background-loads` semaphore (default 2) to prevent upload-batch parse storms.
  - `swagger/odbdesign-server-0.9-swagger.yaml`: document the POST route.
- **Tests:** in-process gRPC test (pattern of existing `OdbDesignTests/Grpc*Tests.cpp`); REST route test; NOT_FOUND for unknown name; ALREADY_* transitions.
- **Accept:** `RequestLoadDesign` returns in <100 ms for a large design; a subsequent `GetDesign` after load completion returns data without re-parsing.
- **Effort:** 2–3 days.

### M1.3 — SI6.2: ETag + Cache-Control on data endpoints

- **Branch:** `nam/http-caching`
- **Depends:** nothing inside Phase 1 (can land before/parallel to M1.1/M1.2).
- **Files:** `OdbDesignLib/App/RouteController.cpp` (or a small `Utils/ETag.h` used by it): ETag = hash(design name, archive mtime, size, endpoint path); set `ETag` + `Cache-Control: private, max-age=3600` on data GETs; honor `If-None-Match` → `304` with empty body (before doing any serialization work — that's the whole point).
- **Notes:** applies to the per-design data endpoints, not the directory-list endpoints (those are cheap and change often). Gzip interplay: ETag must be the *strong* variant of the representation — compute before compression; vary handling via Crow's existing compression middleware.
- **Tests:** 200 + ETag on first fetch; conditional refetch → 304; changed mtime → new ETag.
- **Effort:** ~1 day.

### M1.4 — SI6.1 + SI19.1.3: serialized-response cache + arena allocation (one benchmark)

- **Branch:** `nam/serialized-response-cache` (arenas may be a stacked branch `nam/grpc-arena-alloc` off it, or one PR — one benchmark either way).
- **Depends:** M1.2 (populate after background load completes).
- **Files:**
  - New `OdbDesignLib/App/ResponseCache` (or extend `DesignCache`): keyed (design, variant) → `std::string` JSON per endpoint family + serialized protobuf bytes for `GetDesign`. Populated by the M1.2 background-load completion callback; served byte-identical from cache. Byte accounting feeds M1.1's budget.
  - Invalidation: archive mtime/size change (M1.5 watcher or reload check), re-upload overwrite (`FileUploadController`), `POST /filemodels/<string>` (`DesignCache::AddFileArchive` path). Invalidation drops the entries; next request re-populates. Airtight invalidation is the acceptance bar.
  - SI19.1.3 per the phase-2 spec: `IProtoBuffable::to_protobuf(TPbMessage*)` default (swap-based) + `FeatureRecord` in-place override + arenas in `GetLayerFeaturesStream`/`GetLayerFeaturesBatchStream`.
  - Benchmark: a script (checked into `scripts/`) timing N cold + N warm fetches of the largest test design, both channels, recorded in the PR before/after. One campaign covers M1.1–M1.4 + arenas.
- **Tests:** warm response byte-identical to cold; invalidation on all three triggers; arena messages serialize identically (golden-message tests per phase-2 acceptance).
- **Effort:** 3–5 days.

### M1.5 — SI6.4 (optional, opportunistic): designs-dir watcher

- **Branch:** `nam/designs-dir-watcher`
- **Files:** `DesignCache`: background thread, `std::filesystem` **polling** at a slow interval (e.g. 5 s) — inotify is unreliable over PVC/network mounts, so polling-with-mtime-compare is the portable design; refresh cached listing (feeds list endpoints) + detect replaced archives → invalidate M1.4 entries via the M1.1 state machine.
- **Accept:** list endpoints no longer scan per call; replacing an archive invalidates caches without server restart.
- **Effort:** 1–2 days.

---

## Phase 2 — identity track (strictly ordered)

### M2.1 — SI5a stopgaps (day one, regardless of Keycloak timing)

- **Branch:** `nam/auth-stopgaps`
- **Files:**
  - `OdbDesignLib/App/BasicRequestAuthentication.cpp`: **delete the `odb`/`plusplus` hardcoded fallback** (lines 48–65); env creds missing + auth enabled → fail closed (503 with loud log on protected routes; startup warning). Update the spec's auth description (it documents the fallback behavior).
  - `Utils/crow_win.h` + new `OdbDesignLib/App/AuthMiddleware` (Crow middleware in the `crow::Crow<CORSHandler, AuthMiddleware>` stack) performing the check once per request **before** handlers; delete the per-handler `AuthenticateRequest(req)` calls from all controllers. Skip-list: `/healthz*`, `/ready`, `/helloworld*`.
  - `OdbDesignServer/OdbDesignServerApp.cpp` CORS: origins from `ODBDESIGN_SERVER_CORS_ORIGINS` env (comma list); `*` only when `--disable-authentication`.
- **Tests:** request without creds → 401 (not 403-with-fallback); missing env + enabled → fail closed; every existing route still enforced (middleware-level, so provable by construction); CORS headers match configured origins.
- **Effort:** ~1 day.

### M2.2 — SI5a: Keycloak on k3s (Argo CD GitOps)

- **Where:** platform/deploy repo (GitOps branch `nam20485`), not this repo — same place as the rest of the workload manifests (per the Argo CD handoff plan). No C++ changes.
- **Steps:** pinned Keycloak chart (keycloakx) + Postgres w/ PVC; IngressRoute for `auth.<domain>` (tailnet-only until M3.1's TLS decision lands — do not expose plaintext beyond the tailnet); realm export JSON committed as config: realm `odbdesign`, roles `admin`/`user`, public clients `odbdesign-web` (PKCE) and `odbdesign-cli` (**device flow enabled**); initial admin via one-time bootstrap secret, then disabled.
- **Accept:** OIDC discovery (`/.well-known/openid-configuration`) reachable in-cluster and from tailnet; device flow end-to-end with a throwaway user; Argo CD shows synced/healthy.
- **Effort:** 2–3 days (mostly realm config; the yak-shaving line item).

### M2.3 — SI5a: JWT validation in Crow middleware + gRPC interceptor

- **Branch:** `nam/jwt-auth`
- **Depends:** M2.1 (middleware shape), M2.2 (issuer exists).
- **Files:**
  - `vcpkg.json`: add `jwt-cpp` (pinned version, per the override rules in AGENTS.md).
  - `OdbDesignLib/App/TokenAuthentication` (new, next to `BasicRequestAuthentication`): parse `Authorization: Bearer`, verify against Keycloak JWKS (fetched at startup, refresh on unknown `kid`), check `iss`/`aud`/`exp`, extract `sub` + roles into a request-context struct. Shared by both channels.
  - Crow: `AuthMiddleware` gains mode `basic | bearer | both` (env/arg; default `both` during transition) — the transition window.
  - gRPC: `OdbDesignServer/Interceptors/AuthInterceptor` (first interceptor in the codebase — `ServerBuilder::InterceptorFactory` returning a sync interceptor reading the `authorization` metadata key, same `TokenAuthentication` core; health/reflection services stay anonymous, everything else enforced).
  - `swagger/…swagger.yaml`: add `bearerAuth` security scheme alongside `BasicAuth`.
- **Tests:** valid/missing/expired/wrong-audience tokens on both channels; `both` mode accepts either; `--disable-authentication` still bypasses (local dev).
- **Accept:** a Keycloak-issued token fetches a design over REST and gRPC; Basic still works in `both` mode.
- **Effort:** 2–4 days.

### M2.4 — SI5b: ownership store + enforcement + device flow + `sanitizeFilename`

- **Branch:** `nam/design-ownership`
- **Depends:** M2.3 (identity in request context).
- **Files:**
  - Ownership store in `DesignCache`: sidecar `<design>.meta.json` (`{ "owner": "<sub>", "shared": "all" | "none" }`), atomic write (temp + rename), loaded with the design, listed in directory scans; missing meta → synthesize `owner=admin, shared=all` (legacy designs keep working; write-back optional).
  - Enforcement choke points (exactly two): REST — `RouteController`/`AuthMiddleware` path check resolving design name from the route; gRPC — shared preamble helper in `OdbDesignServiceImpl`. Rule: `admin` role passes; `owner == sub` passes; `shared == all` passes; else 403 / `PERMISSION_DENIED`. Applies to reads, uploads (stamp `owner = sub`), deletes when they exist, and `RequestLoadDesign`.
  - `FileUploadController::sanitizeFilename` — implement it (strip path separators, control chars, reserved names, length cap); it's a live traversal-shape gap in the same code being touched.
  - Device flow: documented client recipe (`docs/`): `odbdesign-cli` public client, device-authorization endpoint → approve in browser → poll → bearer token → same `Authorization` header. No server code beyond M2.3.
  - Swagger: annotate per-route auth notes; document the ownership model.
- **Tests:** owner/admin/stranger × shared/private matrix over both channels; upload stamps owner; legacy design without meta readable by all; filename sanitization cases.
- **Effort:** 3–5 days.

### M2.5 — SI5a completion: remove Basic

- **Branch:** `nam/remove-basic-auth`
- **Depends:** M2.4 steady + clients migrated (the optimized client uses tokens).
- **Files:** delete `BasicRequestAuthentication` + `BasicAuth` scheme from the spec; default auth mode → `bearer`; `--disable-authentication` stays for local dev.
- **Effort:** ~0.5 day.

---

## Phase 3 — edge & events

### M3.1 — SI4: gRPC TLS via Traefik `IngressRouteTCP` + Option C cert chain

- **Decided:** Option 1 mechanism (2026-09-10) + **Option C cert source** (cert-manager + self-managed CA, IP SANs — 2026-09-10 follow-up). Unblocked.
- **Files (this repo):**
  - `scripts/make-tls-ca.sh` (new): one-time CA generation (`openssl req -x509`, 10-yr EC root), usage documented.
  - `deploy/kube/cert-manager/` (new): vendored, version-pinned cert-manager static manifest + README (CA distribution/import instructions per platform).
  - `deploy/kube/issuer-ca.yaml` (new; retires the stale staging `issuer.yaml`): `Issuer` kind `CA` → secret `odbdesign-root-ca`.
  - `deploy/kube/certificate-odbs-server.yaml` (new): `Certificate` with `ipAddresses: [192.168.122.200, 100.118.225.119]` + `dnsNames: [odbdesign.local, debian13vm…]` → secret `odbdesign-server-tls`.
  - `deploy/kube/local-ingress.yaml`: add `tls:` referencing the secret (host-less IP routing preserved — clients verify against SANs, not SNI).
  - `deploy/kube/OdbDesignServer/service-grpc.yaml`: `type: LoadBalancer` → `ClusterIP`.
  - `deploy/kube/odbdesign-grpc-ingressroute-tcp.yaml` (new): `IngressRouteTCP` on :50051 with `tls:` → same secret, h2c backend.
  - `scripts/deploy.ps1`: optional `-EnableTls` flag applying the TLS pieces (default remains plaintext — `https-tls-options.md` §3.6); `scripts/validate-grpc-exposure.ps1` updated for TLS mode (grpcurl drops `-plaintext`).
- **Files (this repo):** `deploy/kube/OdbDesignServer/service-grpc.yaml` → convert from `type: LoadBalancer` to ClusterIP backend of a new `IngressRouteTCP` on :50051 with `tls:` (h2c to the backend); `scripts/deploy.ps1` applies it; `scripts/validate-grpc-exposure.ps1` updated (grpcurl drops `-plaintext`).
- **Notes:** app stays plaintext in-cluster; revisit passthrough/app-native TLS only if mTLS for machine clients is ever wanted (SI5b follow-up, not now).
- **Effort:** ~1 day once unblocked.

### M3.2 — SI8: WebSocket events channel (`/events`) with polling fallback

- **Branch:** `nam/ws-events`
- **Depends:** M1.2 (its events are the first payload).
- **Files:**
  - `OdbDesignServer` new `EventsController`: `CROW_WEBSOCKET_ROUTE` at `/events`; connection registry + broadcast helper; typed JSON events `{type: "design.load.started|finished|failed" | "design.list.changed", design, ts, runId}` sourced from the M1.1 observer callbacks. Auth: requires the M2.3 token (query-param or first-message token; WS headers are awkward from browsers).
  - Proxy configs: Traefik ingress handles WS upgrades — review idle timeouts; `deploy/nginx/nginx.conf` needs an explicit `location /events` with `Upgrade`/`Connection` headers + raised `proxy_read_timeout`.
  - Swagger + client contract note: the channel is **optional by construction** — clients feature-detect and fall back to REST polling (the status quo), never a dependency. State this invariant in the spec description and client docs; OpenAPI doesn't model WS, so document as a note.
  - Client reconnect/resume semantics documented (no `Last-Event-ID` equivalent in WS — simple reconnect + refetch state).
- **Tests:** event broadcast on load lifecycle; connection drop → clients keep functioning via polling (fallback smoke test).
- **Effort:** 3–5 days.

---

## Risk register

| Risk | Where | Mitigation |
|---|---|---|
| `DesignCache` concurrency regressions | M1.1–M1.4 | The state machine + single-flight land first and alone (M1.1); everything after consumes it; concurrent-parse test is the acceptance bar. |
| Cache invalidation misses (stale data served) | M1.4 | Three explicit invalidation triggers + tests for each; when in doubt, invalidate. |
| Auth cutover breaks clients | M2.3/M2.5 | `both` transition mode; Basic removal is its own late milestone after the optimized client is on tokens. |
| Ownership rollout locks out users | M2.4 | Legacy-synthesis rule (`shared=all`) keeps every pre-existing design readable; matrix tests. |
| Keycloak becomes a single point of failure | M2.2 | Server caches JWKS with refresh-on-unknown-kid; token validation keeps working through short IdP outages (already-issued tokens verify locally). |
| Memory growth from eager loads | M1.2 | M1.1's budget/eviction is a hard prerequisite; background-load semaphore caps parse storms. |
| WS through proxies silently broken | M3.2 | Proxy configs are per-deployment acceptance criteria (k3s AND compose), not afterthoughts. |

## Definition of done (per milestone)

1. Branch `nam/<feature>` from `nam20485`; PR to `nam20485`; **merge commit only** (never squash/rebase).
2. Multi-platform builds + tests green (C++ milestones); lint/static checks green (docs/workflow milestones).
3. Acceptance criteria above demonstrated in the PR description (benchmark numbers for M1.4).
4. Swagger spec + docs updated in the same PR when the API surface changes.
5. k3s + compose deployment shapes both updated when deploy files change.
