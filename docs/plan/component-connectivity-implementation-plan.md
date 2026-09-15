# Component connectivity — implementation plan

| | |
|---|---|
| Status | **PLANNED — executing. M0.1 done** (swagger FK wording + deployed copy regenerated). Design and decisions catalog in [component-connectivity.md](component-connectivity.md). D1–D6 open; Phase 2 is gated on **D4** only (the base mismatch that blocked it was resolved 2026-09-15, see §Branch base). Client prep is unblocked now — M0.5. |
| Source | [component-connectivity.md](component-connectivity.md) (verified findings + contract) · [component-id-issue.md](component-id-issue.md) (originating client report) |
| Branch | `nam/component-connectivity` — cut from `nam20485` @ `cd9c0ce`, docs merged to `origin/nam20485` as `ea2c081`, then `development` reconciled back in → tip `d7d1a5b`. **Merge commits only** (AGENTS.md directive 2026-09-10); PR base is `nam20485`. |
| Cross-repo | Phase 3 spans two other repos: `odbdesign-3d-client-prototype` and `Odbdesign-info-client-india79-b`. Their work is specified here but executed as separate PRs in those repos. |
| Build / test | `cmake --preset linux-dynamic-release && cmake --build --preset linux-dynamic-release && ctest --preset linux-dynamic-release`. Single test: `ctest --preset linux-debug -R <TestName>`. `ODB_TEST_DATA_DIR` + `ODB_TEST_ENVIRONMENT_VARIABLE` come from `~/.bashrc`. |

---

## Branch base — resolved 2026-09-15, but record the anomaly

This plan was first cut against `nam20485` @ `cd9c0ce`, at which point `Design::to_protobuf(bool includeNormalizedLists)` and `GetDesignRequest.include_normalized_lists` — the two things M2.1/M2.3 must edit — **did not exist in the base at all**. They had shipped via PR #590 (`274d878`) and PR #591 (`ca9d5c8`), but both were merged from `dev/*` branches straight into **`development`**, skipping `nam20485`. Result: `origin/development` sat 6 commits ahead of `origin/nam20485` with `nam20485` 0 ahead, and because the documented flow (`nam/<feature>` → `nam20485` → `development`) never merges `development` back down, nothing would have carried them into `nam20485` on its own.

Fixed by merging `development` back into `nam20485` (`ea2c081`) and into this branch (`d7d1a5b`). **All citations in both docs are now re-verified against that tip.**

The anomaly is worth keeping in view rather than filing as resolved: `dev/*` → `development` is a shortcut around the integration branch, and it silently makes `nam20485` a stale base for anyone starting work — exactly how the Phase 2 gate in the first revision of this document came out wrong. If that pattern continues, either `nam20485` needs a scheduled sync from `development`, or those PRs should retarget `nam20485` and let the promotion carry them up. **Not this plan's work; flag it if it recurs.**

```text
Phase 0 (now, parallel, no interdeps, no wire risk):
  M0.1 swagger FK wording [DONE]   M0.2 proto sync check     M0.3 connectivity fixture harness
  M0.4 cache flavour inventory ── feeds D4/M2.3              M0.5 client prep (both repos, unblocked)

Phase 1 (server fidelity, independent of Phase 0, gated on D3):
  M1.1 component ;ID= parse ──► M1.3 attributeLookupTable de-dup
  M1.2 feature  ;ID= parse       M1.4 componentRecordsByName populate-or-delete

Phase 2 (gated on D4 only — base unblocked 2026-09-15):
  M2.1 Connectivity proto + derive ──► M2.2 NC/subnet/$NONE$ semantics ──► M2.3 gating + cache ──► M2.4 size bench

Phase 3 (after Phase 2 ships; both clients, same contract):
  M3.1 3D client consumes + deletes joiner      M3.2 info client REST→gRPC + consumes
  M3.3 shared conformance fixture (both)        <── M3.1/M3.2 must not land before this exists

Phase 4 (separate, deliberate):
  M4.1 PinConnection reference-ification ──► revisit include_normalized_lists default
  M4.2 REST surface verdict (D5)
```

---

## Phase 0 — contract hygiene, zero wire risk

### M0.1 Document the foreign keys in swagger — **DONE**

The proximate origin of the defect: swagger never stated what `componentNumber` references. Three fields read only *"Toeprint subnets only."*

**Source of truth:** `swagger/odbdesign-server-0.9-swagger.yaml`, schema **`EdaSubnetRecord`** (`side` / `componentNumber` / `toeprintNumber`) plus `ComponentRecord.id`/`.index`/`.attributeLookupTable`, `ToeprintRecord.netNumber`, and `FeatureRecord.id`.

* `side` → selects which per-side `components` file the numbers index (`comp_+_top` / `comp_+_bot`).
* `componentNumber` → 0-based ordinal of the `CMP` record **within that side's file**; equals the target's `ComponentRecord.index`; explicitly *not* `ComponentRecord.id`; not design-global.
* `toeprintNumber` → positional index into `toeprintRecords`, not a pin number; `pinNumber` is the resolved value and may differ.
* `id` (component + feature) → ODB++ product-model-wide UID from `ID=<id>`: sparse shared number space, permanent, **optional** ("no id present" ≠ `id 0`), and **not** a join key. Both annotated as declared-but-unpopulated, pointing at `attributeLookupTable["ID"]` as today's only carrier.
* `attributeLookupTable` → the literal key `"ID"` is **not** an attribute (no `attributeNames` entry); resolve names only for numeric keys.
* `netNumber` → documents the `-1` / 4294967295 unconnected sentinel and `$NONE$`, so the convention is no longer folklore.

**The deployed copy had drifted 1,063 normalized lines.** `deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml` held 2,471 against the source's 3,534 — missing the entire ETag/`Cache-Control`/304 work (#586), `RequestLoadDesign`, and the earlier annotation pass. The generator **does** exist: `scripts/deploy.ps1:129-131` runs `kubectl create configmap … --dry-run=client -o yaml > $configMapPath`, then applies it, applies the swaggerui deployment/service, and `rollout restart`s the pod (`:135-144`). So the committed manifest is a *deploy-time artifact*, and its staleness means "the spec changed but nothing has been deployed since" — not hand-maintenance. Rather than hand-edit two copies, the configmap was regenerated; running the real generator confirms my output is **byte-identical** (`diff` = 0 lines), so the next deploy will not churn it.

**What actually reaches the cluster, and when.** Merging to `nam20485` publishes the *server image*, which is not where this spec lives — the swaggerui pod mounts it from the `odbdesign-server-swagger-spec` ConfigMap over the image's baked-in (2024-05-08) copy. No CI applies manifests: the only workflows touching `kubectl`/`deploy/kube` are `.github/workflows/disabled/deploy-{eks,local-k8s}.yml`, and `deploy/` contains no Argo CD `Application`. So this commit updates the repo artifact only; **the served spec refreshes when someone runs the `deploy.ps1` Swagger UI block** (regenerate → apply → `rollout restart`). Flagging rather than assuming, since that is a manual step outside this repo's automation.

⚠️ **Gap left open:** the committed manifest can only be as fresh as the last deploy, so reviewers reading `deploy/kube/…` see a stale contract between deploys — exactly the state M0.1 found it in. Two cheap guards: a CI check that the committed file equals the generator's output, or stop committing it and let `deploy.ps1`/GitOps materialize it. A third client (`.NET` info client `Api/Dtos/`) also hand-mirrors schema shapes and is **not** covered by M0.2.

**Verification performed (all executed, not inferred):** both files parse as YAML; all 68 distinct `$ref` targets resolve in the source; the embedded copy is byte-identical to the source; old-vs-new normalized line diff is exactly **4 lines**, all accounted for (3 pre-existing formatting variants that persist in newer form — `- $ref:` allOf wrapping for `Color`/`ToolsFile`, `enum: ["Yes", …]` quoting — plus the 1 line this item rewrote); and `kubectl create configmap --dry-run=client -o yaml` was run against the annotated source, producing output identical to the committed file. Not done: nothing was applied to a cluster and no pod was restarted.

**Exit:** docs/YAML only — no C++ behaviour change, so the multi-platform builds cannot signal a regression here.

### M0.2 Machine-checked proto sync across clients

Both vendored proto trees drift silently today (§4.4 of the design doc) — the info client's copy is missing four service.proto features. "All clients follow the server contract" only holds if they cannot accidentally hold an old one.

Add a CI check that diffs `OdbDesignLib/protoc/*.proto` + `OdbDesignServer/protoc/grpc/service.proto` against:
* `odbdesign-3d-client-prototype/src/OdbDesign3DClient.Core/Protos/`
* `Odbdesign-info-client-india79-b/protoc/`

Permitted known differences: `option cc_enable_arenas` (C++ only) and commented `optimize_for` lines. Everything else fails. Cross-repo checkout makes this awkward in OdbDesign-only CI — cheapest useful version is a workflow_dispatch job with both repos cloned, or a hash manifest of the proto set committed to each client. **Choose one; a check that doesn't run is worse than none.**

### M0.3 Connectivity fixture harness

Before either side changes, capture ground truth so M2.1 and M3.x can be differentially tested.

**File:** new `OdbDesignTests/ConnectivityContractTests.cpp` (name follows the existing `SymbolContractTests.cpp` precedent). Fixtures: `sample_design` (UIDs present, 813 comps / 2,811 connections), `200-40628_Rev1_v7` (UIDs absent, 7,610 comps), `Panel g7162-31800_odb` (the 88-collision case). Golden file per design: `refDes → [(pinNumber, netOrdinal)]`, plus pin counts per net.

### M0.4 Response cache flavour inventory — new, settle before M2.3

`OdbDesignServiceImpl::GetDesign` (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp`) keeps two paths, and M2.3 inherits a constraint it does not yet name:

* **Warm path** returns a pre-serialized `std::string` (`:115-118`, built by the worker at `:200-205`) — **only the default `include_normalized_lists=false` flavour**, per the comment at `:198-199`.
* **Cold path** calls `to_protobuf(includeLists)` (`:220-221`) and serializes per request; the `std::string` return is then copied twice (`:215`, `:230`).

So any request whose flags differ from the default re-serializes the **entire cached `FileArchive`** (`:96-99`) on every call — and that is 537 MB uncompressed for `Turbot` alone. Adding `include_connectivity` as a flag would create a second per-request full-design serialization, which is the opposite of what #590 was for. Options: cache serialized bytes per flavour, or make connectivity always-on so it rides the existing warm entry. Decide this in D4, with the numbers from M2.4 — not ad hoc in code.

### M0.5 Client-side prep — unblocked now, no server dependency

Answering "can the two clients start?": **they cannot finish** — there is nothing to consume until M2.1 emits `Connectivity`, and deleting the joiner before its replacement exists just breaks the inspector. These four items are independent and non-throwaway:

| Item | Repo | Why independent |
|---|---|---|
| **REST → gRPC transport switch** | info client | Prerequisite for ever seeing `Connectivity` (gRPC-only). Largest item, reviewable alone. |
| Refresh vendored `protoc/` and `Api/Dtos/` | info client | Already stale by four service.proto features; needed regardless. |
| refDes **case-sensitivity** fix | both | Spec p.152 makes refDes case-significant; all four name dictionaries use `OrdinalIgnoreCase` (3D `:20`/`:33`/`:183`, info `:147`). Real latent defect, unrelated to the join rewrite. |
| M3.3 conformance harness skeleton | both | Needs only M0.3's golden files, not the contract. |

**Deliberately *not* in this track:** re-keying `netsByComponent` from `(side, Id)` to `refDes` today. It would stop the visible wrongness immediately, but M3.1/M3.2 delete that code path wholesale — so it is throwaway unless the merged-nets list is in front of real users right now. That is a product call, not a technical one.

---

## Phase 1 — server format fidelity

### M1.1 Parse the component UID

**Files:** `OdbDesignLib/FileModel/Design/ComponentsFile.cpp`, `AttributeLookupTable.{h,cpp}`

`ComponentsFile.cpp:489` already hands the whole `;…;ID=…` token to `ParseAttributeLookupTable`. Extend `AttributeLookupTable` with an out-parameter or sibling accessor that returns the parsed ID section separately, then assign `pCurrentComponentRecord->id`. `index` assignment stays untouched at `:478`.

**Wire:** add `set_id(id)` beside `set_index(index)` (`:215`) and `id = message.id()` in `ComponentRecord::from_protobuf`. Both are `optional uint32`, so presence is expressible — **absent must stay absent**; do not write `set_id(0)` for designs lacking `;ID=` (D2).

### M1.2 Parse the feature UID

Same shape, same leak: `FeaturesFile.h:66` declares `id`, `FeaturesFile.cpp:976` serializes it, and six `ParseAttributeLookupTable(attrIdString)` call sites (`:290`, `:367`, `:459`, `:554`, `:577` commented, `:621`) discard it. Wire the accessor through all live call sites, not just one. Feature volumes are far larger (11,868 UIDs in one `sample_design` layer) — measure the payload delta in M2.4, since this one *does* grow the response.

### M1.3 `attributeLookupTable["ID"]` (D3)

Decide: drop the `"ID"` key now that `id` carries it, or dual-write + deprecate. Nothing in either client reads `"ID"` today (verified by grep across both repos), so dropping is available — but it is a behaviour change for unenumerated consumers. **Recommend dual-write + deprecate, remove in Phase 4.**

### M1.4 `componentRecordsByName`

Either populate during parse (`ComponentsFile.cpp` has a standing `// TODO: add records to maps by name while adding to their vectors` above the member) and serialize it in `to_protobuf`, or delete the field from `componentsfile.proto:72` and swagger. Note the key is `compName`, which is case-sensitive (spec p.152) while `std::map<std::string,…>` orders byte-wise — fine, but state it.

**Populating costs payload**: it duplicates 813 full records per side. Given §4.2, **recommend delete** and let clients index `refDes → record` locally (allowed by conformance rule 1).

### Phase 1 tests

* `ComponentsFileParseTests`: `;ID=` present → `id` set + `has_id()`; absent → unset (`200-40628` fixture); attribute-only record (`;0,2=0,3=1`, spec p.150 example) → parses without error and leaves id unset.
* **`index` invariance**: assert per-side ordinals still restart at 0 for each side. This is the load-bearing property for `componentNumber`; a regression here is silent and breaks the netlist join.
* Round-trip in `ProtobufSerializationTests.cpp`: id presence survives `to_protobuf`→`from_protobuf`.
* Re-run `ComponentHeightPipelineTests` and `FileArchiveLoadTests` — the attribute-table change sits under the height resolution path.

---

## Phase 2 — server-owned connectivity

**Gated on D4 only** (base unblocked — see §Branch base).

### M2.1 `Connectivity` message + derive

**Files:** `OdbDesignLib/protoc/connectivity.proto` (new) or extend `design.proto`; `OdbDesignLib/ProductModel/Design.{h,cpp}`.

`design.proto` uses tags 1–11 → `optional Connectivity connectivity = 12;`. Protos are globbed (`OdbDesignLib/CMakeLists.txt:55`, `file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")`) — a new file needs a CMake **reconfigure**, not just a build.

Emit it from `Design::to_protobuf(bool includeNormalizedLists)` (`Design.cpp:167`) — the gRPC flavour. Decide in D4 whether `Connectivity` joins the normalized-lists gate or gets its own; note the no-arg `to_protobuf()` (`Design.cpp:159-165`) deliberately still emits the full flavour for REST/`to_pbstring`/round-trip tests, so a new field added only to the parameterized path will **not** appear in those tests unless wired separately.

Derive from `m_nets[].GetPinConnections()`, which already holds resolved `(shared_ptr<Component>, shared_ptr<Pin>)` pairs. **This is a serialization of an existing result — do not write a second join.** Package rosters dedupe by name (71 entries, not 813). Emit `netMembers` from the same pass, sorted by net ordinal then refDes, so output is deterministic and diffable.

### M2.2 Semantics absorbed server-side

`Connection.Kind`: `UNCONNECTED` for netNumber `-1`/`4294967295`; `$NONE$` mapped to `UNKNOWN`/`UNCONNECTED` rather than leaking a magic net name (the server's own `BreakSinglePinNets`/`BuildNoneNet` are disabled at `Design.cpp:138-139`, so NC pins currently ride `$NONE$` — decide whether this contract revives them or just labels them). `ComponentPin.number` resolved from the package pin, so `toeprintNumber` never reaches a client.

Keep the hard failure visible: `Design.cpp:465` `GetPin(pinNumber)` null → whole load fails. Do not degrade that to omission; surface which pin failed.

### M2.3 Gating + cache interaction (D4)

If opt-in, add `GetDesignRequest.include_connectivity` beside `include_normalized_lists` (`service.proto:51`), and respect the fast-path rule PR #590 established in `OdbDesignServiceImpl::GetDesign`: the pre-serialization worker caches **only** the default flavour, so any non-default response serializes cold. If always-on, it becomes part of the cached default — better for clients, and the size data from M2.4 is what justifies it. Either way, check the interaction with the serialized-response cache (SI6/M1.4) so the cached bytes and the flag actually agree.

### M2.4 Size benchmark

Extend `DesignFetchBenchmarkTests.cpp` across the three fixture designs × {pruned, +connectivity, +normalized lists}. Acceptance: `connectivity` ≤ 150 KB on `sample_design` and ≤ 10% of the fileModel payload on the 11,631-component legacy design. Also record the M1.2 feature-UID delta.

---

## Phase 3 — clients follow the contract

Neither client may keep a private join after this phase. Both currently do.

### M3.1 3D prototype

**File:** `src/OdbDesign3DClient.Core/Services/Implementations/ComponentDetailBuilder.cs` (562 lines).

Delete: `byId`/`byIndex`/`byIdAnySide`/`byIndexAnySide` construction (`:98-103`), `ResolveComponent` (`:440-460`), the netlist inversion loop building `netsByComponent` (`:110-135`, `:172-178`), `ResolvePinNumber` (`:479-481`). Rebuild `ComponentDetailIndex` as a lookup over `Connectivity.components`, keyed `refDes` — which also removes the `(BoardSide, uint Id)` key at `:14`/`:44`/`:238` that caused the collapse. Keep `NetDetail` for the net tab, sourced from `netMembers`.

### M3.2 info client

**Files:** `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` (531 lines — same deletions at `:74`, `:96-108`, `:415-432`, `:448-453`), `src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs`, `DesignService.cs`.

Bigger change: this client reads design data over **REST** (`Api/Dtos/`) while already carrying gRPC codegen (`OdbDesign.ProductModel.csproj:32-37`). Switching its design read to gRPC is prerequisite to it seeing `Connectivity` at all — `Connectivity` is deliberately not built for REST (D5). Sequence: transport switch first (its own PR, reviewable alone), then consume `Connectivity`. Refresh `protoc/` as part of it — the copy is stale by four features.

Also fix `refDes` case handling here and in M3.1 (§8 hazard 3).

### M3.3 Shared conformance fixture

Publish the M0.3 golden files to both clients as test data. Each asserts its `refDes → (pin, net)` view equals the server's. This is what makes rule 2 in §7 enforceable — a client that re-introduces a join fails a test rather than shipping.

---

## Phase 4 — debt, sequenced deliberately

* **M4.1** `PinConnection` reference-ification (`Component.cpp:75-76` embeds full `Package`+`Part`). Breaking wire change to the normalized lists. Lands only if it makes `include_normalized_lists` unnecessary — that's its justification.
* **M4.2** D5 verdict on the REST surface: maintain / freeze / deprecate. Blocks on nothing; do it before someone implements `designs_component_route_handler` (`DesignsController.cpp:311`) for a client that is leaving REST.
* **M4.3** Remove the `attributeLookupTable["ID"]` dual-write from M1.3.
* **M4.4** The dead `BuildPlacementsFromEdaDataFile` (`Design.cpp:521`, zero call sites; declared `Design.h:104`): delete it, or make it the authoritative path deliberately. Leaving an alternative connectivity implementation in the tree is how this divergence stayed invisible.

---

## Risks

| Risk | Mitigation |
|---|---|
| `id` populated before a client stops resolving by `Id` → ~10% silent mis-resolution on `Panel`-shaped designs | Phase 3 deletes that resolution. **This is a deploy constraint, not a merge one** — M1.1 may merge any time, but do not ship a server build with `id` populated into an environment whose clients still resolve `componentNumber` by `Id`. Coordinate rollout per environment and verify the §4.3 collision case in the M3.3 fixtures. |
| `Connectivity` as an opt-in flag re-serializes the whole FileArchive per request | M0.4 — measure before D4; `include_normalized_lists` already costs this, and a second flag doubles it. |
| `nam20485` goes stale under the branch again (`dev/*` → `development` shortcuts) | Sync from `nam20485` before opening each PR; the Phase 2 gate in the first revision of this plan was wrong for exactly this reason (§Branch base). |
| `Connectivity` and `fileModel` disagree for some design | M3.3 differential test is exactly this. A disagreement is a server bug, not a client bug, by construction. |
| Response cache serves the wrong flavour | M2.3 — reuse PR #590's fast-path rule (cache holds the default flavour only). |
| Feature UID delta regresses payload | M2.4 measures M1.2 before it merges. |
| Info client transport switch swells this PR's blast radius | Separate PR in a separate repo, reviewable alone (M3.2). |
