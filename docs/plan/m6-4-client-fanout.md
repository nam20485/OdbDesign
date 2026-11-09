# M6.4 — GetDesignStream client fan-out (3D + info clients)

| | |
|---|---|
| Status | **3D client IMPLEMENTED + REVIEW-FIXED 2026-11-01 — PR [intel-agency/odbdesign-3d-client-prototype#67](https://github.com/intel-agency/odbdesign-3d-client-prototype/pull/67)** (branch `feat/getdesign-stream`, `2299e6b` + review-fix `6dd8592`; suite **1248/1248** post-fixes, no new warnings — ~35 pre-existing nullable warnings live in untouched files; live stream≡monolithic equivalence gate passed against the merged-tip server AND independently reproduced by the review, which also re-ran the suite). Three-lens review of `2299e6b`: 13 reported → 9 unique, **all fixed in `6dd8592`** — `connectivity.proto` byte-sync (the 28th proto, stale since the #62-era sync), empty-stream header-seen guard (surfaced as **DataLoss**, not Internal — Internal is in the transient-retry set), OCE-shape conversion for mid-stream generation cancel, bounded **stream-only** transient retry (R-4 intact), null-Step present-but-empty parity, `StreamTimeoutSeconds` docs, `DESIGN_STREAM_SMOKE_DESIGN` smoke parametrization (Panel-g7162 is not in local TEST_DATA — zero-connectivity live coverage deferred; unit tests cover empty-batch semantics). Awaiting CI/review on `6dd8592`. Server #609 **merged** (`99f03f3`), **deployed + live-accepted 2026-11-02** (`nam20485-1287`, digest `29a4501d`). Info client **DONE 2026-11-02** — #27 merged (`8679ac8`), promoted `nam20485`→`development`→`main` (#28/#29, `c98a179`); proto-sync only. **A stream fallback remains owed to the info client — the §4 deferral premise was wrong and is corrected below (§2 recon note + §4).** |
| Parent | [getdesign-streaming-oom-plan.md](getdesign-streaming-oom-plan.md) (M6.4 definition, §M6.5 gates, chunk contract) · [component-connectivity-client-followups.md](../completed/component-connectivity/component-connectivity-client-followups.md) (F1–F7 client backlog; F6 = proto upkeep) |
| Split | **3D client → the OdbDesign session that authored #609** (this doc §3). **Info client → separate session** (this doc §4 + handoff prompt). FixtureCapture `--stream` stays in this repo and rides the M6.6 PR. |

## 1. Driver

The #609 dispatch guard rejects over-budget monolithic `GetDesign` with `RESOURCE_EXHAUSTED`
("use the GetDesignStream RPC" — `OdbDesignServiceImpl.cpp:272-278`). The 3D client renders from the
full `Design.fileModel` fetched via monolithic `GetDesign`, so without adopting the stream it cannot
load the two big designs (200-40628, 350-41017) from any post-#609 server. No regression — those
designs OOM today's server — the update converts the server fix into a usable client feature.

## 2. Recon (verified 2026-11-01)

### 3D client — `intel-agency/odbdesign-3d-client-prototype`
- Clean tree on `development`; PR #65 merged; no open PRs *(at recon time — #67 opened later the same day; the local checkout now sits on `feat/getdesign-stream`)*. Branch style `feat/*`; remote is the **intel-agency org** (CI there randomly cancels runs — rerun before re-diagnosing).
- `GetDesignAsync` at `src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs:122`: client-side design cache (hit / in-flight join / generation-canceled eviction). **`RpcException … ResourceExhausted` handler at `:224` currently logs + rethrows — that is the fallback insertion seam.**
- Streaming precedent exists (`GetLayerFeaturesStream`, `StreamTimeoutSeconds` config); real-server smoke pattern exists (`tests/…/Rendering/SolidBoardRealServerSmokeTests.cs`).
- Proto drift: all 27 message protos differ **cosmetically only** (`option cc_enable_arenas`, comments) except two semantic gaps shared with the info client (below). **No C# code references `ComponentRecordsByName`** (grep-clean) → wholesale `protoc/` sync is safe, no code fallout beyond regenerate.
- `service.proto` missing: `GetDesignStream` + `DesignChunk` contract, `RequestLoadDesign` (+ messages).

### Info client — `nam20485/Odbdesign-info-client-india79-b`
*(Corrected 2026-11-01, post-execution. The original first bullet said "REST-first; gRPC used only for `Connectivity`" — **wrong even at recon time**: design reads went gRPC with #23 on 2026-10-15. That error is what produced the bad §4 deferral.)*
- **Design reads are gRPC** (#23, 2026-10-15): `DesignService.GetDesignArchiveAsync` → `IOdbDesignGrpcClient.GetDesignAsync` (`src/OdbDesignInfoClient.Services/DesignService.cs:413`) — the method's own doc-comment calls the gRPC `GetDesign` response "the single content source". REST covers the design list (`GetDesignNamesAsync`, `:114`) and auth. Codegen wired in `OdbDesign.ProductModel.csproj`. Default branch `main`.
- **Branch flow resolved** (was "needs untangling"): #25 (F6) and #26 (F4/F5) both merged to `nam20485`; #27 cut from `8aa2b95`; promoted `nam20485`→`development` (#28, `72f700d`) → `main` (#29, `c98a179`, 2026-11-02T00:42Z). `main` is current; residue is stale local `feature/*` branches only.
- **`ComponentRecordsByName` verified** (was "unverified"): zero hand-written C# consumers — generated `obj/` code only — so #27's proto sync was regenerate-only, no code fallout.

### Shared semantic proto gaps (vs server `OdbDesignLib/protoc/`)
1. `componentsfile.proto` — client still declares `map<string, ComponentRecord> componentRecordsByName = 10;`; server **deleted + reserved** it (never populated by the parser; conformance rule 1: clients index refDes → record locally).
2. `edadatafile.proto` — server added `optional uint32 id = 7` (NET `;ID=`) and `id = 13` (PKG) from the connectivity epic; clients lack them.

## 3. 3D client scope (ONE PR: `feat/getdesign-stream` from `development`) — EXECUTED as #67

1. **Proto sync (wholesale):** copy `OdbDesignLib/protoc/*` + `OdbDesignServer/protoc/grpc/service.proto` from the post-merge `nam20485` tip (pre-merge: the `nam/getdesign-streaming` tip — proto content is frozen, PR approved 272/272). Kills the cosmetic noise so future diffs are meaningful; brings the two semantic fixes + `GetDesignStream`.
2. **Fallback in `GetDesignAsync`:** on **any** `ResourceExhausted` from the monolithic call, fall back once to `GetDesignStream` and reassemble, feeding the existing cache path. Implemented as any-RE rather than matching the guard's detail only: both RE flavors (dispatch-guard refusal and "larger than max") are size failures, and bounded chunks remedy both. Pre-streaming servers (`Unimplemented` on the stream) surface the original refusal. Server decides monolithic-vs-stream; client never guesses size. (Two existing contract tests that pinned the pre-fallback fail-fast behavior were updated: still exactly one monolithic attempt — R-4 — plus one chunk-sized fallback attempt.)
3. **Reassembler** per the contract comments in `service.proto:146+` (authority):
   - `DesignHeader` scalars once; `StepSkeleton` insert into `fileModel.stepsByName[step_name]`; `FeatureBatch` append into `…layersByName[layer_name].featureFile.featureRecords`; Design-level repeated batches append in order; `ConnectivityBatch` `MergeFrom` into `Design.connectivity`; `NormalizedMapBatch` maps merge.
   - **Resolve map entries by iteration, never `find()`/`at()`** — the #599 address-layout-dependent protobuf-Map bug; the proto comments call this out for client implementers.
   - Sanitization-colliding step/layer names arrive with the monolith's last-wins semantics (server guarantee) — reassembler appends/overwrites, never dedupes.
4. **Tests:** unit reassembly from crafted chunk sequences (incl. empty/cancelled mid-stream); equivalence — stream-reassembled Design deep-equals monolithic `GetDesign` for the same flavour on `sample_design` + a zero-connectivity design (`Panel-g7162`); fallback unit test with a mocked channel throwing `ResourceExhausted`; real-server smoke following `SolidBoardRealServerSmokeTests`.
5. **Local server for tests:** build merged-tip OdbDesign (`linux-dynamic-release` preset, `VCPKG_ROOT` exported), run with `--designs-dir ../OdbDesignTestData/TEST_DATA` (relative path — `CommandLineArgs` treats leading `/` as a flag).

Gates: full sln build + suite green (1154+ baseline) + new tests green.

## 4. Info client scope (separate session, ONE PR) — EXECUTED 2026-11-02 as #27

**Shipped:** proto sync from server `99f03f3` (message protos + `service.proto`) + regenerate. Original item 3 collapsed to nothing — the `ComponentRecordsByName` grep came back clean (§2), so no code fix or targeted test was needed. Branch cut from `8aa2b95` (both #25/#26 already merged — the original "cut from the tip carrying `0e46581`" instruction described the pre-#26 picture). Suite 119/119, all CI legs green, merged `8679ac8`, promoted to `main` via #28/#29 (`c98a179`).

**Deferral rationale CORRECTED 2026-11-01.** The original "Out of scope (deliberate)" paragraph deferred a `GetDesignStream` reader on two premises, both stale:
1. *"the info client has no gRPC design-read consumer"* — false since #23 (2026-10-15): `DesignService.GetDesignArchiveAsync` → `IOdbDesignGrpcClient.GetDesignAsync` (`DesignService.cs:413`) **is** the design-read path.
2. *"reassembly lands with F3 (REST→gRPC transport switch)"* — F3 had already shipped (#23); #27 was its proto refresh, and no reassembly landed with it.

The deferral's *outcome* stands as shipped, but it is **remaining work, not dead-code avoidance**: on any post-#609 server, `GetDesignAsync` on the two big designs (200-40628, 350-41017) fails with a clean `RESOURCE_EXHAUSTED`, and the info client currently swallows it — `FetchDesignArchiveAsync`'s catch-all logs a warning and returns `null` (`DesignService.cs:429-434`), so the design renders empty with no user-visible error and no retry. When big-design support is wanted, mirror 3D #67: any-`ResourceExhausted` → single `GetDesignStream` fallback → reassembler per the contract comments in `service.proto:146+` (iterate maps, never `find()`/`at()` — the #599 landmine). While in there, stop nulling genuine transport failures — surface them.

## 5. Sequencing — updated 2026-11-01

- ~~Proto content is frozen → client branches can be cut immediately~~ **done**: info #27 merged + promoted; 3D #67 open (green, MERGEABLE).
- ~~#609 merge → docker publish → rollout → live acceptance~~ **DONE 2026-11-02**: merged `99f03f3` → published `nam20485-1287` → rolled out (pod on digest `29a4501d…`, 00:21Z) → live acceptance passed (big-design stream OK at 3.31 GiB HWM; over-budget monolithic = clean `RESOURCE_EXHAUSTED`).
- **Remaining:** review + merge 3D #67 → M6.6 (`nam/m66-residency-budget`, cut from `99f03f3`) with FixtureCapture `--stream` riding it → info-client stream fallback (§4) whenever big-design support is wanted there.
