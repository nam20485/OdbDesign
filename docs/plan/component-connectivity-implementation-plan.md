# Component connectivity — implementation plan

| | |
|---|---|
| Status | **PLANNED — not started.** Design and decisions catalog in [component-connectivity.md](component-connectivity.md). D1–D6 open; Phase 2 and 3 are gated on D3/D4. |
| Source | [component-connectivity.md](component-connectivity.md) (verified findings + contract) · [component-id-issue.md](component-id-issue.md) (originating client report) |
| Branch | `nam/component-connectivity` ← `nam20485` @ `cd9c0ce`. **Merge commits only** (AGENTS.md directive 2026-09-10); PR base is `nam20485`. |
| Cross-repo | Phase 3 spans two other repos: `odbdesign-3d-client-prototype` and `Odbdesign-info-client-india79-b`. Their work is specified here but executed as separate PRs in those repos. |
| Build / test | `cmake --preset linux-dynamic-release && cmake --build --preset linux-dynamic-release && ctest --preset linux-dynamic-release`. Single test: `ctest --preset linux-debug -R <TestName>`. `ODB_TEST_DATA_DIR` + `ODB_TEST_ENVIRONMENT_VARIABLE` come from `~/.bashrc`. |

---

## Dependency on unmerged work

`include_normalized_lists` (`f1484a4`) is **not** in `nam20485` — it is only on `dev/getdesign-include-lists-flag`, along with the DesignCache concurrency fixes. That branch rewrites `Design.cpp`/`Design.h` (`to_protobuf(bool)` overload) and `service.proto`, both of which M2.1/M2.3 touch.

**M2 must not start before `dev/getdesign-include-lists-flag` merges to `nam20485`**, or it must be stacked on that branch deliberately. Phases 0 and 1 are independent of it and can proceed now. Line-number citations in the design doc are the `nam20485` base and will shift once that PR lands.

```text
Phase 0 (now, parallel, no interdeps, no wire risk):
  M0.1 swagger FK wording      M0.2 proto sync check       M0.3 connectivity fixture harness

Phase 1 (server fidelity, independent of Phase 0, gated on D3):
  M1.1 component ;ID= parse ──► M1.3 attributeLookupTable de-dup
  M1.2 feature  ;ID= parse       M1.4 componentRecordsByName populate-or-delete

Phase 2 (BLOCKED on f1484a4 merge + D4):
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

### M0.1 Document the foreign keys in swagger

The proximate origin of the defect: swagger never states what `componentNumber` references. Three fields currently read only *"Toeprint subnets only."*

**File:** `swagger/odbdesign-server-0.9-swagger.yaml` (`:2429`, `:2433`, `:2437`).

* `side` → which per-side components file the numbers index.
* `componentNumber` → **the ordinal position of the CMP record within that side's `components` file**. Not `ComponentRecord.id`. Not design-global.
* `toeprintNumber` → ordinal into that record's `toeprintRecords`; `pinNumber` is the resolved value, provided.
* `ComponentRecord.id` → ODB++ product-model-wide unique ID, **optional**, permanent, and **not** a join key. `index` → per-side ordinal, positional, **not** a stable key.
* `ComponentRecord.attributeLookupTable` → note that key `"ID"` is the UID, not an attribute index (pending D3).

Also mirror into `deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml` (the deployed copy) or the served spec goes stale.

**Exit:** docs-only; `Analyze (actions)` + Codacy green. No C++ build signal expected (AGENTS.md: builds cannot detect a docs regression).

### M0.2 Machine-checked proto sync across clients

Both vendored proto trees drift silently today (§4.4 of the design doc) — the info client's copy is missing four service.proto features. "All clients follow the server contract" only holds if they cannot accidentally hold an old one.

Add a CI check that diffs `OdbDesignLib/protoc/*.proto` + `OdbDesignServer/protoc/grpc/service.proto` against:
* `odbdesign-3d-client-prototype/src/OdbDesign3DClient.Core/Protos/`
* `Odbdesign-info-client-india79-b/protoc/`

Permitted known differences: `option cc_enable_arenas` (C++ only) and commented `optimize_for` lines. Everything else fails. Cross-repo checkout makes this awkward in OdbDesign-only CI — cheapest useful version is a workflow_dispatch job with both repos cloned, or a hash manifest of the proto set committed to each client. **Choose one; a check that doesn't run is worse than none.**

### M0.3 Connectivity fixture harness

Before either side changes, capture ground truth so M2.1 and M3.x can be differentially tested.

**File:** new `OdbDesignTests/ConnectivityContractTests.cpp` (name follows the existing `SymbolContractTests.cpp` precedent). Fixtures: `sample_design` (UIDs present, 813 comps / 2,811 connections), `200-40628_Rev1_v7` (UIDs absent, 7,610 comps), `Panel g7162-31800_odb` (the 88-collision case). Golden file per design: `refDes → [(pinNumber, netOrdinal)]`, plus pin counts per net.

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

**Gated on the §Dependency note and D4.**

### M2.1 `Connectivity` message + derive

**Files:** `OdbDesignLib/protoc/connectivity.proto` (new) or extend `design.proto`; `OdbDesignLib/ProductModel/Design.{h,cpp}`.

`design.proto` uses tags 1–11 → `optional Connectivity connectivity = 12;`. Protos are globbed (`OdbDesignLib/CMakeLists.txt:55`, `file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")`) — a new file needs a CMake **reconfigure**, not just a build.

Derive from `m_nets[].GetPinConnections()`, which already holds resolved `(shared_ptr<Component>, shared_ptr<Pin>)` pairs. **This is a serialization of an existing result — do not write a second join.** Package rosters dedupe by name (71 entries, not 813). Emit `netMembers` from the same pass, sorted by net ordinal then refDes, so output is deterministic and diffable.

### M2.2 Semantics absorbed server-side

`Connection.Kind`: `UNCONNECTED` for netNumber `-1`/`4294967295`; `$NONE$` mapped to `UNKNOWN`/`UNCONNECTED` rather than leaking a magic net name (the server's own `BreakSinglePinNets`/`BuildNoneNet` are disabled at `Design.cpp:138-139`, so NC pins currently ride `$NONE$` — decide whether this contract revives them or just labels them). `ComponentPin.number` resolved from the package pin, so `toeprintNumber` never reaches a client.

Keep the hard failure visible: `Design.cpp:454` `GetPin(pinNumber)` null → whole load fails. Do not degrade that to omission; surface which pin failed.

### M2.3 Gating + cache interaction (D4)

If opt-in, add `GetDesignRequest.include_connectivity` beside the existing `include_normalized_lists`, and respect the fast-path rule from that commit: the cache pre-serializes only the default flavour, so a connectivity-requesting response serializes cold. If always-on, it becomes part of the cached default — better for clients, and the size data from M2.4 is what justifies it.

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
* **M4.4** The dead `BuildPlacementsFromEdaDataFile` (`Design.cpp:510`, zero call sites): delete it, or make it the authoritative path deliberately. Leaving an alternative connectivity implementation in the tree is how this divergence stayed invisible.

---

## Risks

| Risk | Mitigation |
|---|---|
| `id` populated before a client stops resolving by `Id` → ~10% silent mis-resolution on `Panel`-shaped designs | Phase 3 deletes that resolution; M1.1 and M3.x must not both ship to the same deployed client. Verify §4.3 collision case in M3.3 fixtures. |
| M2 built on the pre-`f1484a4` tree | §Dependency gate; re-cut from `nam20485` after that PR merges. |
| `Connectivity` and `fileModel` disagree for some design | M3.3 differential test is exactly this. A disagreement is a server bug, not a client bug, by construction. |
| Response cache serves the wrong flavour | M2.3 — reuse the `f1484a4` fast-path rule (cache holds default flavour only). |
| Feature UID delta regresses payload | M2.4 measures M1.2 before it merges. |
| Info client transport switch swells this PR's blast radius | Separate PR in a separate repo, reviewable alone (M3.2). |
