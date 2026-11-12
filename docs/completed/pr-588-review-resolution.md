# PR #588 Review Comment Resolution Plan

**Status:** COMPLETE — all 14 review issues addressed (2026-09-12). 11 fixed in code (215/215 ctest green, benchmark verified: cold 3.4 s vs warm ~1 µs); W5+S6 fixed after maintainer approval; W4 recorded as decision **D10** in `docs/plan/argocd-deployment-plan.md` (one gRPC Service variant per watched path; TLS variant relocates in the §6.1 reshuffle)
**Date:** 2026-09-12
**Target:** PR #588 (`nam20485` → `development`, the standing integration PR)
**Review source:** kilo-code-bot review of commit `ebd7901` — 14 issues (6 WARNING, 8 SUGGESTION)

## Verification summary

Every claim was verified against the code before planning. Verdicts: **13 confirmed, 1 partial** (S6 — real but only fires in a narrowly-gated branch).

Decisions already made with Nathan:

- **W6 benchmark:** implement the missing cache-level benchmark test (not the full in-process
  gRPC benchmark, not script deletion).
- **Branch:** fix commits are pushed **directly to `nam20485`** (admin bypass of the PR-only
  ruleset; PR #588's development-base checks re-run on the push).
- **No merge of PR #588** as part of this work — comment resolution only.

---

## Review thread inventory (fetched 2026-09-12, post-review re-check)

Re-checked before execution: no new reviews/comments since the kilo review (2026-09-12
01:08–01:09); PR head still `b5b04d4`. There are **13 unresolved inline threads** — the 14th
issue (S8, swagger 304 docs) is **summary-only in the review body** (its target GET blocks are
unchanged lines, so no thread was anchored). S8 is still fixed per §6, but has no thread to
reply/resolve; it is covered by the final PR summary comment.

| Plan ID | Thread ID | Path | Issue (first line) |
|---|---|---|---|
| W1 | `PRRT_kwDOJhkWgs6hrr8d` | `OdbDesignLib/App/DesignCache.cpp` | Eviction leaks the serialized-payload charge from `m_cachedBytes` |
| S1 | `PRRT_kwDOJhkWgs6hrr8j` | `OdbDesignLib/App/DesignCache.cpp` | Byte budget bypassed for `AddFileArchive(save=false)` |
| W2 | `PRRT_kwDOJhkWgs6hrr8k` | `OdbDesignLib/App/RouteController.cpp` | Disk-based ETag returns 304 for in-memory-only changes |
| W3 | `PRRT_kwDOJhkWgs6hrr8l` | `OdbDesignServer/Controllers/FileUploadController.cpp` | Auto-warm silent no-op for already-loaded design; fresh ETag pins stale body |
| S2 | `PRRT_kwDOJhkWgs6hrr8m` | `Utils/ETag.h` | Extension stripping diverges from DesignCache naming; missing `<string_view>` |
| S3 | `PRRT_kwDOJhkWgs6hrr8p` | `OdbDesignServer/Controllers/DesignsController.cpp` | Uncaught exception path in REST load twin |
| S4 | `PRRT_kwDOJhkWgs6hrr8q` | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` | Cached fast-path parse result unchecked |
| S5 | `PRRT_kwDOJhkWgs6hrr8r` | `OdbDesignTests/DesignCacheSingleFlightTests.cpp` | Assertion can never fail |
| W4 | `PRRT_kwDOJhkWgs6hrr8t` | `deploy/kube/OdbDesignServer/service-grpc-loadbalancer.yaml` | Duplicate Service, contradictory `spec.type` |
| W5 | `PRRT_kwDOJhkWgs6hrr8u` | `scripts/deploy.ps1` | TLS-off leaves stale gRPC TLS artifacts breaking LB exposure |
| W6 | `PRRT_kwDOJhkWgs6hrr8v` | `scripts/benchmark-design-fetch.sh` | gtest filter matches no test |
| S6 | `PRRT_kwDOJhkWgs6hrr8w` | `scripts/validate-grpc-exposure.ps1` | localhost TLS check cannot succeed (SANs) |
| S7 | `PRRT_kwDOJhkWgs6hrr8x` | `swagger/odbdesign-server-0.9-swagger.yaml` | `sym_num` vs wire format `symNum` |
| S8 | *(no thread — summary-only)* | `swagger/odbdesign-server-0.9-swagger.yaml` | No 304/If-None-Match/ETag/Cache-Control documented |

---

## 1. DesignCache byte accounting

### W1 — Eviction strands `serializedBytes` in `m_cachedBytes` (WARNING)

**Claim verified.** `EvictOverBudgetLocked` (`OdbDesignLib/App/DesignCache.cpp:696`) subtracts
only `victimIt->second.estimatedBytes`, but a warm design's total contribution to `m_cachedBytes`
is `estimatedBytes + serializedBytes` (charged at `CommitResponsePayloads`, `DesignCache.cpp:1041-1051`).
Every evict-warm-design cycle permanently strands `serializedBytes` worth of phantom bytes;
only `Clear()` resets it. The comment in `EvictCacheEntries` (`DesignCache.cpp:709-712`) asserting
the payload bytes "were already uncharged" is false.

**Fix**

- `EvictOverBudgetLocked`: `m_cachedBytes -= estimatedBytes + serializedBytes` for the victim.
- Correct the stale comment in `EvictCacheEntries`.
- Add a public `cachedBytes()` getter (test seam, styled after the existing
  `cachedResponseBytes()` / `designSerializationCount()`).

**Test** — ResponseCacheTests: warm a design, commit payloads, evict it (small budget + second
design, mirroring `ByteBudget_IncludesSerializedBytes_EvictionDropsPayloads`), assert the budget
drops by the entry's full contribution — no stranded bytes.

### S1 — `save=false` uploads charged only `DESIGN_BYTES_OVERHEAD` = 4096 (SUGGESTION)

**Claim verified — and understated.** `AddFileArchive(..., save=false)` →
`InsertLruAndEvict` → `EstimateDesignBytes` (`DesignCache.cpp:734-737`): no on-disk file
(`FindArchivePath` empty) → charge is `4096 + 0`. Even `save=true` undercharges for fresh
names, because the estimate runs *before* `SaveFileArchive` writes the file and is never
re-synced. Bonus bug found while verifying: `FileModelController.cpp:449` passes raw
`designName`, not `designNameDecoded`, unlike every other handler in the file.

**Fix**

- `AddFileArchive` gains an optional `std::uint64_t injectedBytes = 0` parameter;
  `EstimateDesignBytes`/`InsertLruAndEvict` thread it through so the no-archive fallback
  charges `DESIGN_BYTES_OVERHEAD + StoredBytes + injectedBytes`.
- `FileModelController.cpp:449`: pass `designNameDecoded` (not `designName`) and
  `req.body.size()` as `injectedBytes` — the JSON body is already fully in memory, so its
  size is an honest floor for the parsed archive's footprint.
- For `save=true`: after `SaveFileArchive` returns inside `AddFileArchive`, resync the LRU
  entry via a new private `ResyncLruEstimate(name)` — recompute `EstimateDesignBytes`
  *outside* `m_lruMutex` (same pattern as `InsertLruAndEvict`), then adjust the entry's
  `estimatedBytes` and `m_cachedBytes` by the delta under the lock.

**Test** — `AddFileArchive(name, archive, false, N)` with N large → `cachedBytes()` grows
by more than 4096.

---

## 2. Invalidation + ETag correctness

### W3 — Upload overwrite: auto-warm is a silent no-op; fresh ETag pins stale body (WARNING)

**Claim verified.** `fastmove_file` is `rename()`, which silently replaces an existing archive
(`FileUploadController.cpp:148`). `autoWarmDesign` → `LoadDesignAsync` returns true for a
Loaded design (it's not in any in-flight set), but `BackgroundLoad` → `GetOrLoadSingleFlight`
hits the cache fast path (`DesignCache.h:410-426`) and returns the **stale** object without
ever reading disk. No invalidation trigger exists on this path (`AddFileArchive` is only
called from `POST /filemodels`). Result: fresh on-disk ETag labeling a stale 200 body, then
permanent 304s.

**Fix**

- New public `DesignCache::InvalidateDesign(const std::string& designName)` — the
  invalidation block extracted from `AddFileArchive` (`DesignCache.cpp:75-89`):
  - under `m_cacheMutex` unique lock: erase `m_designsByName[name]`, `m_fileArchivesByName[name]`
  - `InvalidateResponsePayloads(name)` (drops payloads, bumps generation)
  - transition load state to Unloaded with observer notify
  - idempotent for absent names
- `autoWarmDesign` (`FileUploadController.cpp:20-39`): call `InvalidateDesign` before
  `LoadDesignAsync` when the design is already loaded (`GetLoadState != Unloaded`), so the
  background load actually re-parses the replaced archive.

**Test** — DesignCacheSingleFlightTests style: `GetDesign` → p1; `InvalidateDesign`;
`GetDesign` → p2; assert `p2.get() != p1.get()` (fresh parse), state Unloaded after
invalidation, observer saw Loaded→Unloaded, payloads dropped (HasCachedResponse false).

### W2 — `POST /filemodels/<name>` (save=false) → conditional GETs return 304 forever (WARNING)

**Claim verified.** `MakeDesignEtag` (`Utils/ETag.h:132-157`) hashes only
(design name, on-disk mtime, on-disk size, endpoint path). A `save=false` POST replaces the
in-memory cache but never touches the file (`DesignCache.cpp:75-89` invalidates payloads but
not the HTTP validator), so the client's old tag keeps matching → 304 with no fresh body,
indefinitely, for any design that also exists on disk.

**Fix** — mix the per-design response-cache generation into the tag:

- `Utils/ETag.h`: `MakeEtag` and `MakeDesignEtag` gain an optional trailing
  `std::uint64_t generation = 0` folded into the FNV-1a digest. Existing callers and tests
  compile unchanged (default 0).
- New public `DesignCache::GetResponseGeneration(name)` → forwards to
  `ResponseStore::CurrentGeneration(name)` (`DesignCache.h:317`) — per-design, cheap
  (one mutex-guarded map lookup), and **already bumped by every invalidation**
  (`AddFileArchive`, eviction, and the new `InvalidateDesign`).
- `RouteController::checkConditionalGet` (`RouteController.cpp:103`) passes the generation.

After a POST, the generation bump changes the tag → the client's stale tag mismatches →
fresh 200 from the new in-memory archive.

**Tests** — HttpCachingTests: `MakeEtag`/`MakeDesignEtag` generation sensitivity (same inputs,
different generation → different tag; same generation → same tag).

**Note for thread reply:** generation also rotates tags after LRU eviction/invalidation even
when the body is identical — a rare, correctness-neutral extra 200 (cache-efficiency loss
only). Acceptable; will be stated in the reply.

### S2 — ETag archive resolution diverges from DesignCache stem naming; missing include (SUGGESTION)

**Claim verified.** `FindDesignArchiveFile` (`Utils/ETag.h:73-125`) whole-strips multi-part
extensions (`.tar.gz` first), while everything serving-side names designs by
`path::stem()` (last extension only): `DesignCache::FindArchivePath` (`DesignCache.cpp:540`),
the directory scans (`:158/190/227`), `FileUploadController.cpp:23`. Concrete hazards:
`GET /filemodels/flexi` with `flexi.tar.gz` on disk gets an ETag computed for a design the
server would 404; with both `foo.gz` and `foo.tar.gz` present, the validator can attest a
different file than the one served. `HttpCachingTests.cpp:139-148` currently *pins* the
divergent behavior. Also `std::string_view` is used at `ETag.h:82` without
`#include <string_view>` (compiles only via transitive includes today).

**Fix**

- Add `#include <string_view>`.
- Rewrite `FindDesignArchiveFile` to mirror `DesignCache::FindArchivePath` exactly: first
  regular file in the directory whose `path::stem() == designName` (drop the extension
  whitelist loop). Serving and validation then resolve the identical file.
- Update the `HttpCachingTests` pin to stem semantics (`foo.tar.gz` resolves for design
  `foo.tar`, not `foo`; `flexi.tgz` still resolves for `flexi`).

---

## 3. Server handlers

### S3 — REST load handler: `ComputeRequestLoadStatus` can throw, no catch (SUGGESTION)

**Claim verified.** `LoadDesignAsync` rethrows `std::system_error` from `std::thread` spawn
failure (`DesignCache.cpp:429,441`; contract documented at `DesignCache.h:99-101`). The gRPC
twin catches → INTERNAL (`OdbDesignServiceImpl.cpp:661-673`); the REST handler
(`DesignsController.cpp:247-256`) does not, despite its "same as the gRPC twin" comment.
The exception escapes into Crow's worker loop ("Worker Crash" path) — the client gets no
response at all rather than a 500.

**Fix** — mirror `TryGetDesign` in the same file:

```cpp
try { status = ComputeRequestLoadStatus(...); }
catch (const std::exception& e) {
    logexception_msg(e, "failed to kick load for design \"" + designNameDecoded + "\"");
    return crow::response(crow::status::INTERNAL_SERVER_ERROR, ...);
}
```

### S4 — Unchecked `ParseFromString` on the gRPC cached fast path (SUGGESTION)

**Claim verified.** `OdbDesignServiceImpl.cpp:136` discards the return value, then logs
`ByteSizeLong()` and returns `Status::OK` — an empty/partial design served as success.
Project convention checks it (`IProtoBuffable.h:55`). Likelihood is low (bytes serialized by
this same process, atomic store), but the failure mode is silent wrong data.

**Fix** — on parse failure: `logerror`, `response->Clear()`, fall through to the existing
cold path (`:142-156`), which is always safe to run. Poisoned payload stays cached but only
costs a fallback until the next invalidation — correct, minimal.

### S5 — Vacuous `EXPECT_NE(pFirst, nullptr)` (SUGGESTION)

**Claim verified.** `DesignCacheSingleFlightTests.cpp:451` cannot fail (`pFirst` asserted
non-null at :432, never reassigned) and never dereferences, so it detects nothing.

**Fix** — replace with a real liveness dereference:
`EXPECT_FALSE(pFirst->GetName().empty());` (eviction drops the cache's reference; the
caller's object must stay usable).

---

## 4. New benchmark test (per decision: cache-level)

### W6 — `benchmark-design-fetch.sh` filters a test suite that doesn't exist (WARNING)

**Claim verified empirically.** `DesignFetchBenchmarkTest` matches zero tests (verified
against the built binary: "Running 0 tests", exit 0), so line 43's `grep -E '\[BENCH\]'`
exits 1 under `set -e` — the script always fails. No `[BENCH]` producer or `ODB_BENCH_*`
reader exists anywhere; `OdbDesignTests/DesignFetchBenchmarkTests.cpp` was never committed.

**Fix** — add `OdbDesignTests/DesignFetchBenchmarkTests.cpp` (+ CMake source-list entry):

- Suite `DesignFetchBenchmarkTest`, gated on `ODB_DESIGN_FETCH_BENCH=1` (skip otherwise, so
  routine ctest stays green without the env var).
- Cold vs warm `DesignCache::GetDesign` on the largest fixture design found under
  `ODB_TEST_DATA_DIR` (overridable via `ODB_BENCH_DESIGN`).
- Honors `ODB_BENCH_COLD` / `ODB_BENCH_WARM` iteration counts (script defaults 2/10).
- Emits `[BENCH] cold ... min/avg/max ms` / `[BENCH] warm ...` lines the script's grep finds.
- Cold = fresh cache per iteration (new DesignCache / eviction), warm = repeated fetches.

**Verify** — run `scripts/benchmark-design-fetch.sh` end-to-end after the build.

---

## 5. Deploy manifests & scripts

### W4 — Duplicate Service object with contradictory `spec.type` (WARNING)

**Claim verified.** `service-grpc-loadbalancer.yaml:17` (`type: LoadBalancer`) and
`service-grpc.yaml:18` (`type: ClusterIP`) define the **same** Service
(`odbdesign-server-grpc-service`, default namespace) in the **same** directory. Lexical order
means a directory-wide apply silently ends ClusterIP. Only `deploy.ps1` consumes them
selectively today (TLS → ClusterIP, non-TLS → LoadBalancer).

**INVESTIGATION (2026-09-12, live cluster + repo, read-only):**

- **Argo CD has ZERO Applications.** Argo CD itself is installed (`argocd` namespace, 5d22h
  old; the only AppProject is the stock `default`). Nathan confirmed: the GitOps migration is
  an upcoming plan, nothing is watching any repo path. The "GitOps-watched directory" hazard
  in the original plan was **future state** (from `docs/plan/argocd-deployment-plan.md`),
  not current reality.
- **The only deployment mechanism is manual `deploy.ps1`** (CI deploy workflows live in
  `.github/workflows/disabled/`). It applies files individually and **exactly one** gRPC
  Service variant (`deploy.ps1:98-103`). Nobody performs a directory-wide apply.
- **Current mode: non-TLS.** Live: `odbdesign-server-grpc-service` = `LoadBalancer`
  (ServiceLB, node :50051, svclb pod 24d), REST ingress port 80 only, **no cert-manager, no
  HelmChartConfig, no IngressRouteTCP, no Certificates, no TLS secrets**.
- **Image delivery:** Deployment pins the mutable tag
  `ghcr.io/nam20485/odbdesign:nam20485-latest` with `imagePullPolicy: Always`; new images are
  picked up when the deployment restarts (`deploy.ps1` runs `rollout restart` on every run;
  there is no CI restarter).
- The cluster object name (`odbdesign-server-grpc-service`) is what matters; a file
  move/rename in the repo is invisible to the cluster.

**Revised recommendation — RESOLVED (maintainer decision 2026-09-12):** no manifest change
now; the issue moved into the Argo CD deployment plan as decision **D10** (exactly one gRPC
Service variant in the watched path — the LoadBalancer one, matching the live non-TLS cluster
so adoption is a no-op; `service-grpc.yaml` relocates to a `deploy/kube/tls/` dir in the §6.1
reshuffle, never leaving two definitions of the same Service inside an Application path).
Thread replied + resolved with the pointer.

### W5 — Non-TLS deploy leaves Traefik gRPC entrypoint behind (WARNING)

**Claim verified.** TLS path applies `traefik-helmchartconfig-grpc-entrypoint.yaml` (:201)
and `odbdesign-grpc-ingressroute-tcp.yaml` (:220); the non-TLS path (:101-103) applies only
the LB service — `deploy.ps1` contains no `kubectl delete` at all. The leftover entrypoint's
svclb hostPort-binds :50051 on the node, which the LB service's svclb DaemonSet also needs →
conflict → no ingress IP.

**INVESTIGATION (2026-09-12):**

- **Current termination architecture** (design: Option C + M3.1, `deploy.ps1` `-EnableTls`):
  - *non-TLS default (what the cluster runs today):* gRPC = ServiceLB LoadBalancer, plaintext
    on node :50051 directly to the server pod. REST = Traefik :80. No TLS anywhere.
  - *`-EnableTls`:* cert-manager (vendored v1.17.2) + private-CA Certificate
    (`odbdesign.local`, `odbdesign`, `192.168.122.200`, `100.118.225.119`) → secret
    `odbdesign-server-tls`; Traefik **terminates** TLS for REST on :443 (ingress `tls:`
    block) and for gRPC on a new `grpc` entrypoint :50051 (`IngressRouteTCP` → h2c to the
    **ClusterIP** gRPC service). Server itself is always plaintext in-cluster.
- **The conflict scenario is not live:** the cluster has **never had** TLS artifacts (no
  HelmChartConfig / IngressRouteTCP / cert-manager exist). It can only occur after a future
  `-EnableTls` deploy followed by a no-flag redeploy.
- **The proposed fix cannot affect the current deployment:** it only adds
  `--ignore-not-found=true` deletes in the *non-TLS branch* of a manually-run script. On the
  current cluster both deletes are provable no-ops (nothing to delete). It does not touch the
  TLS path, termination, or any manifest. It only changes behavior in exactly the scenario
  that would otherwise break (TLS → non-TLS transition).
- Deleting `helmchartconfig traefik` is precise: no HelmChartConfig exists on this cluster,
  and the only one the repo ever applies is the gRPC-entrypoint one (k3s reconciles the
  packaged Traefik back to defaults when it is removed).

**Revised recommendation — RESOLVED (approved 2026-09-12):** implemented as originally
planned: the non-TLS branch of `deploy.ps1` now deletes the IngressRouteTCP and the Traefik
gRPC-entrypoint HelmChartConfig (`--ignore-not-found=true`) before applying the LB service,
with a comment explaining the :50051 hostPort contention. No-op on the current cluster
(neither artifact exists); only affects a future TLS → non-TLS transition, where it prevents
the breakage instead of causing it.

### S6 — localhost TLS validation fails deterministically (SUGGESTION) — PARTIAL

**Verified partial.** The serving certificate has no localhost/127.0.0.1 SAN
(`certificate-odbs-server.yaml:23-28`), so a TLS grpcurl against `localhost` fails cert
verification. **But** the probe branch (`validate-grpc-exposure.ps1:449-466`) only fires when
`$env:COMPUTERNAME` equals an explicitly-passed `-AdvertisedHost` — skipped in the default
TLS flow (auto-detected host) and impossible on Linux (COMPUTERNAME unset).

**INVESTIGATION (2026-09-12):** doubly hypothetical today — the cluster runs **non-TLS**, so
the `-Tls` validation path isn't even exercised. The fix (skip the localhost probe under
`-Tls`, log the reason) is a **validation-script-only** change: it cannot touch the cluster,
TLS termination, or the deployment. The main grpcurl check (:469) targets the ServiceLB
ingress IP with the CA cert and is unaffected.

**Revised recommendation — RESOLVED (approved 2026-09-12):** implemented: the localhost
probe is skipped when `-Tls`, with a log line stating the reason (serving cert carries no
localhost/127.0.0.1 SAN). Validation-script-only change; the advertised-host probe still
covers TLS mode.


---

## 6. Swagger

### S7 — Examples use `sym_num`; wire format is `symNum` (SUGGESTION)

**Claim verified.** The 5 flagged example lines (`:1566, :1573, :1631, :2554, :2561`) were
added by this PR (commit `7dadf0d`) and use `sym_num`, but REST JSON goes through
`MessageToJsonString` with default options (`IProtoBuffable.h:78-87`) → camelCase `symNum`;
the proto has no `json_name` override.

**Fix** — change the 5 example keys to `symNum`. The same mismatch exists in *pre-existing*
schema properties (`apt_def`, `apt_def_symbol_num`, `width_factor`, `orient_def`,
`vpl_vnd`/`vpl_mpn`, …) — **out of scope here**, will be flagged in the reply as a known
follow-up.

### S8 — No 304 / If-None-Match / ETag / Cache-Control documented anywhere (SUGGESTION)

**Claim verified.** Zero matches for any of those strings in the 3361-line spec, while 19
data GET call sites invoke `checkConditionalGet`. Conditional GET is a headline feature of
this PR (M3.1) — it should be documented.

**Fix**

- Add reusable components:
  - `components/responses/NotModified` — 304, empty body, ETag + Cache-Control headers
  - `components/headers/ETag`, `components/headers/CacheControl`
  - `components/parameters/IfNoneMatch` (in: header)
- For each of the 19 data GET operations (enumerated via `grep checkConditionalGet
  OdbDesignServer/Controllers/*.cpp` at execution): add the If-None-Match parameter,
  ETag/Cache-Control headers on the 200 response, and the 304 response ref.
- Guarded by `python3 scripts/check-openapi-routes.py` (paths-only checker — response/param
  additions cannot break it) plus a YAML-parse sanity check.

---

## Execution sequence (revised after Nathan's feedback)

**Scope:** the 11 approved items only. W4/W5/S6 get investigation-reply comments on their
threads but stay unresolved pending Nathan's decision; no deploy manifests, `deploy.ps1`, or
`validate-grpc-exposure.ps1` changes in this pass. The final PR summary comment is likewise
held until all 14 are resolved.

1. Confirm clean tree on `nam20485`; apply all edits above.
2. Build + test:
   - `cmake --build --preset linux-debug`
   - Targeted: HttpCachingTests, ResponseCacheTests, DesignCacheSingleFlightTests,
     RequestLoadDesignTests, DesignFetchBenchmarkTest
   - Full: `ctest --preset linux-debug -j$(nproc)` with `ODB_TEST_DATA_DIR` +
     `ODB_TEST_ENVIRONMENT_VARIABLE` exported
   - `scripts/benchmark-design-fetch.sh` end-to-end
   - `python3 scripts/check-openapi-routes.py`; `bash -n` on the shell script;
     pwsh syntax-parse of the .ps1 edits if pwsh is available
3. Commit in logical groups (cache accounting/invalidation/ETag · server handlers · test
   fixes · swagger+benchmark), signed per repo config, then **push directly
   to `nam20485`**. Poll PR #588 checks until green (3 CMake legs, SBOM, Codacy,
   dependency-review).
4. Resolve threads via GraphQL (memory-documented gotcha: reply mutation takes
   `pullRequestReviewThreadId`, resolve takes `threadId`):
   - fetch unresolved `reviewThreads` on PR #588
   - per thread: `addPullRequestReviewThreadReply` with the explanation (change + commit
     SHA, or why no change), then `resolveReviewThread`
   - W4/W5/S6: reply with the §5 investigation findings, **leave unresolved**
5. After Nathan decides W4/W5/S6: apply (or permanently defer) those, resolve their threads,
   leave the final PR summary comment, and re-fetch to confirm 0 unresolved remain.

## Risks / notes

- ETag generation input rotates tags on eviction/invalidation even when the body is
  identical → rare extra 200s, correctness-neutral (stated in the W2 reply).
- `Clear()` resets response-store generations (absent = 0), so tags can revert to their
  generation-0 values after a cache clear — harmless (tags are only ever compared to
  themselves server-side).
- The stem-semantics change (S2) is a small observable behavior change in `FindDesignArchiveFile`
  (`.tar.gz` archives now validate under their stem name, matching how they are served and
  uploaded); the pinned test is updated accordingly.
- Direct pushes to `nam20485` bypass the PR-only ruleset with an admin notice — accepted
  practice for review fixes on the integration PR.
