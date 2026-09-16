# Component connectivity — implementation plan

| | |
|---|---|
| Status | **EXECUTING — M0.1, M0.7 done; D3/D4/D5 resolved 2026-09-15** (UID key removed atomically with `id`; `Connectivity` always-on; REST frozen, security-only). Design + decision records in [component-connectivity.md](component-connectivity.md). **Phase 1 and Phase 2 are now unblocked.** Client work: M0.5 prep + [handoff doc](component-connectivity-client-handoff.md). |
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
  M0.6 mirror spec → SwaggerUI image repo (cross-repo; low priority since compose now mounts #1)
  M0.7 spec distribution fixes [DONE]: compose binds #1  +  swagger-spec-configmap-sync.yml

Phase 1 (server fidelity; M1.1 + M1.3 are ATOMIC per D3 — one commit, one release):
  M1.1 component ;ID= parse ─┐
  M1.2 feature  ;ID= parse ──┴──► M1.3 remove attributeLookupTable["ID"] (same commit)
  M1.4 componentRecordsByName: delete the never-populated map field

Phase 2 (ALWAYS-ON per D4 — no flag; rides the cached default flavour):
  M2.1 Connectivity proto + derive ──► M2.2 NC/subnet/$NONE$ semantics ──► M2.4 size bench (merge gate)

Phase 3 (after Phase 2 ships; both clients, same contract):
  M3.1 3D client consumes + deletes joiner      M3.2 info client REST→gRPC + consumes
  M3.3 shared conformance fixture (both)        <── M3.1/M3.2 must not land before this exists
  M3.0 client handoff doc [DONE]                → component-connectivity-client-handoff.md

Phase 4 (separate, deliberate):
  M4.1 PinConnection reference-ification ──► revisit include_normalized_lists default
  M4.2 REST freeze executed as a deletion: remove the two stub handlers (D5)
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

**What actually reaches the cluster, and when.** Merging to `nam20485` publishes the *server image*, which is not where this spec lives — the swaggerui pod mounts the `odbdesign-server-swagger-spec` ConfigMap over the single file (`deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml:36-40`, via `subPath`). No CI applies manifests: the only workflows touching `kubectl`/`deploy/kube` are `.github/workflows/disabled/deploy-{eks,local-k8s}.yml`, and `deploy/` contains no Argo CD `Application`. So this commit updates the repo artifact only; **the served spec refreshes when someone runs the `deploy.ps1` Swagger UI block** (regenerate → apply → `rollout restart`). Flagging rather than assuming, since that is a manual step outside this repo's automation.

**There is a third copy, and it is stale — M0.1 is not complete without it.** The deployed image is `ghcr.io/nam20485/odbdesignserver-swaggerui:nam20485-latest`, built from the **sibling repo** `OdbDesignServer-SwaggerUI`, whose `Dockerfile` does `COPY spec/ /spec` and sets `SWAGGER_JSON=/spec/odbdesign-server-0.9-swagger.yaml`. That baked copy (`../OdbDesignServer-SwaggerUI/spec/odbdesign-server-0.9-swagger.yaml`) is **3,310 lines against this repo's 3,605 — 321 differing lines — and still carries all three bare `"Toeprint subnets only."` descriptions**, i.e. none of this item's annotations. In k3s the ConfigMap mount hides that; in `compose.yml:58-66` there is **no volume mount**, so the baked copy is what a local/compose user actually reads. Tracked as **M0.6**; the repo map and the three-copy rule are now in `AGENTS.md` §"The swagger/OpenAPI spec has THREE copies".

⚠️ **Gap left open:** the committed manifest can only be as fresh as the last deploy, so reviewers reading `deploy/kube/…` see a stale contract between deploys — exactly the state M0.1 found it in. Two cheap guards: a CI check that the committed file equals the generator's output, or stop committing it and let `deploy.ps1`/GitOps materialize it. A third client (`.NET` info client `Api/Dtos/`) also hand-mirrors schema shapes and is **not** covered by M0.2.

**Verification performed (all executed, not inferred):** both files parse as YAML; all 68 distinct `$ref` targets resolve in the source; the embedded copy is byte-identical to the source; old-vs-new normalized line diff is exactly **4 lines**, all accounted for (3 pre-existing formatting variants that persist in newer form — `- $ref:` allOf wrapping for `Color`/`ToolsFile`, `enum: ["Yes", …]` quoting — plus the 1 line this item rewrote); and `kubectl create configmap --dry-run=client -o yaml` was run against the annotated source, producing output identical to the committed file. Not done: nothing was applied to a cluster and no pod was restarted.

**Exit:** docs/YAML only — no C++ behaviour change, so the multi-platform builds cannot signal a regression here.

### M0.2 Machine-checked proto sync across clients

Both vendored proto trees drift silently today (§4.4 of the design doc) — the info client's copy is missing four service.proto features. "All clients follow the server contract" only holds if they cannot accidentally hold an old one.

Add a CI check that diffs `OdbDesignLib/protoc/*.proto` + `OdbDesignServer/protoc/grpc/service.proto` against:
* `odbdesign-3d-client-prototype/src/OdbDesign3DClient.Core/Protos/`
* `Odbdesign-info-client-india79-b/protoc/`

Permitted known differences: `option cc_enable_arenas` (C++ only) and commented `optimize_for` lines. Everything else fails.

**Still open — tracked as D7** in the design doc: pick between (a) a committed hash manifest per client, (b) a `workflow_dispatch` cross-repo diff job, or (c) packaging the protos. Recommendation is (a) for now; (c) is the long-term answer but belongs with the IPC-2581 unified-API decision. Cross-repo checkout makes this awkward in OdbDesign-only CI — cheapest useful version is a workflow_dispatch job with both repos cloned, or a hash manifest of the proto set committed to each client. **Choose one; a check that doesn't run is worse than none.**

### M0.3 Connectivity fixture harness

Before either side changes, capture ground truth so M2.1 and M3.x can be differentially tested.

**File:** new `OdbDesignTests/ConnectivityContractTests.cpp` (name follows the existing `SymbolContractTests.cpp` precedent). Fixtures: `sample_design` (UIDs present, 813 comps / 2,811 connections), `200-40628_Rev1_v7` (UIDs absent, 7,610 comps), `Panel g7162-31800_odb` (the 88-collision case). Golden file per design: `refDes → [(pinNumber, netOrdinal)]`, plus pin counts per net.

**M0.3 delivered — 2026-09-15, on this branch.** `scripts/gen-connectivity-golden.py` + four goldens in `OdbDesignTests/Fixtures/Connectivity/`. Two things the first cut got wrong and both are now fixed and verified:
* Pretty-printed output was **66 MB** (29 MB + 35 MB for the two legacy designs). Now compact JSON, and designs over 10,000 components emit `"fidelity":"sampled"` — aggregates plus a deterministic subset (first/last 50 per side and every ordinal/UID collision) with the net roster capped at 50. Total **1.3 MB**, byte-identical on re-run. `sample_design` and `Panel-g7162` stay `"full"`.
* My own brief handed the implementer **top-only** counts as design totals (7,610 and 11,631). Actual totals: **57,774** (7,610 Top + 50,164 Bottom) and **81,157** (11,631 Top + 69,526 Bottom). The implementer's numbers were right and mine were not — worth stating plainly, because a delegate that "matches the spec" is not automatically correct, and here the spec was the defect.

⚠️ **Fixture selection consequence:** `Panel-g7162` has **zero** connected pins (all 3,078 `TOP` lines are `net_num = -1`) and `200-40628` likewise (all 129,698). So the Panel fixture validates the collision and roster cases **only** — connectivity assertions must run against `sample_design` (2,811/2,811 connected), and `350-41017` (158,742 connected) is the second connected design if a larger one is wanted. Recorded so M3.3 doesn't quietly assert an empty set and call it passing.

Collision figure re-verified independently against the raw source files, not the generator: **88 on Top, 0 on Bottom** — matching design doc §4.3.

### M0.4 Response cache flavour inventory — resolved by D4 (always-on)

`OdbDesignServiceImpl::GetDesign` (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp`) keeps two paths, and M2.3 inherits a constraint it does not yet name:

* **Warm path** returns a pre-serialized `std::string` (`:115-118`, built by the worker at `:200-205`) — **only the default `include_normalized_lists=false` flavour**, per the comment at `:198-199`.
* **Cold path** calls `to_protobuf(includeLists)` (`:220-221`) and serializes per request; the `std::string` return is then copied twice (`:215`, `:230`).

So any request whose flags differ from the default re-serializes the **entire cached `FileArchive`** (`:96-99`) on every call — and that is 537 MB uncompressed for `Turbot` alone. **Decided (D4): connectivity is always-on**, so no flag is added and it rides the existing warm entry. Opt-in would have meant a second per-request full-design serialization on exactly the boards where it hurts most — the opposite of what #590 was for.

### M0.5 Client-side prep — unblocked now, no server dependency

Answering "can the two clients start?": **they cannot finish** — there is nothing to consume until M2.1 emits `Connectivity`, and deleting the joiner before its replacement exists just breaks the inspector. These four items are independent and non-throwaway:

| Item | Repo | Why independent |
|---|---|---|
| **REST → gRPC transport switch** | info client | Prerequisite for ever seeing `Connectivity` (gRPC-only). Largest item, reviewable alone. |
| Refresh vendored `protoc/` and `Api/Dtos/` | info client | Already stale by four service.proto features; needed regardless. |
| refDes **case-sensitivity** fix | both | Spec p.152 makes refDes case-significant; all four name dictionaries use `OrdinalIgnoreCase` (3D `:20`/`:33`/`:183`, info `:147`). Real latent defect, unrelated to the join rewrite. |
| M3.3 conformance harness skeleton | both | Needs only M0.3's golden files, not the contract. |

**Deliberately *not* in this track:** re-keying `netsByComponent` from `(side, Id)` to `refDes` today. It would stop the visible wrongness immediately, but M3.1/M3.2 delete that code path wholesale — so it is throwaway unless the merged-nets list is in front of real users right now. That is a product call, not a technical one.

### M0.6 Mirror the spec into the SwaggerUI image repo — new, cross-repo

**Repo:** `../OdbDesignServer-SwaggerUI`, file `spec/odbdesign-server-0.9-swagger.yaml`. It is baked into the image (`Dockerfile`: `COPY spec/ /spec`), so it is what a **bare `docker run`** of the image serves. compose no longer depends on it (M0.7), which drops this from "the local view is silently stale" to "only affects unmounted image runs" — lower priority than when it was filed.

Copy this repo's annotated `swagger/odbdesign-server-0.9-swagger.yaml` over it, then that repo's `.github/workflows/docker-publish.yml` must run to republish `ghcr.io/nam20485/odbdesignserver-swaggerui:nam20485-latest`; k3s picks it up on the next pull (`imagePullPolicy: Always`) plus a rollout restart.

The 321-line gap is **not** only M0.1's annotations — the image copy also predates #585/#586 and the earlier annotation pass. Ownership is settled: **this repo's `swagger/odbdesign-server-0.9-swagger.yaml` is canonical** and the flow is one-way (#1 → #2 ConfigMap, #1 → #3 image). Verified 2026-09-15 that #3 is strictly behind — it holds no paths #1 lacks (#1 has `/designs/{name}/load` extra) — so a straight overwrite from #1 loses nothing; no three-way reconcile needed.

**Better end state:** a hand-synced copy in a second repo is the defect. Option (a) — always mount — is now done for compose (M0.7); what remains is generating #3 from #1 in the sibling repo's CI, or accepting it as a fallback for unmounted image runs only.

### M0.7 Spec distribution fixes — **DONE**

Two changes so that #1, not a stale projection, is what people actually read:

* **compose binds #1 over the baked copy.** `compose.yml` and `compose.local.yml`, `swagger-ui` service: `./swagger/odbdesign-server-0.9-swagger.yaml:/spec/odbdesign-server-0.9-swagger.yaml:ro` — the same single-file target k3s uses via `subPath`. Local runs are now immune to image staleness. Both files verified to parse and to carry the mount.
* **`.github/workflows/swagger-spec-configmap-sync.yml` lands** (pulled forward standalone from `argocd-deployment-plan.md` §6.5, which had it as GitOps Phase 2). Triggers on `push` to `nam20485` with `paths: ["swagger/**"]` plus `workflow_dispatch`; re-runs `deploy.ps1`'s exact `kubectl create configmap … --dry-run=client -o yaml` command, validates the result (kind, configmap name, embedded spec parses as OpenAPI, ≥20 paths) and **exits before committing** if any check fails, then commits via the Contents API so the commit is GitHub-signed — required because the `nam20485` ruleset requires signed commits and defines no `pull_request` rule (§3.3).

Verification actually executed, not assumed: workflow YAML parses (2 steps, no draft artifacts left behind); the `run` block passes `bash -n`; and the regenerate-and-validate core was run locally — `kubectl` present, output non-empty, **118,020 bytes embedded, 39 paths, 54 schemas**, and byte-identical to the committed manifest. So on merge the workflow takes the "already matches — nothing to do" branch and will **not** emit a bot commit for this PR. Loop-safety also holds independently: the bot commit touches only `deploy/`, outside the `swagger/**` filter, and `GITHUB_TOKEN` pushes do not create runs.

⚠️ Not verified: no `gh api` PUT was exercised (needs a real `nam20485` push), and no compose stack was launched — the mount path is inferred from the working k3s `subPath` mount and the image's `SWAGGER_JSON` env, not from a live request. First `docker compose up` should confirm the served spec shows the new `componentNumber` description.

---

## Phase 1 — server format fidelity

### M1.1 Parse the component UID

**Files:** `OdbDesignLib/FileModel/Design/ComponentsFile.cpp`, `AttributeLookupTable.{h,cpp}`

`ComponentsFile.cpp:489` already hands the whole `;…;ID=…` token to `ParseAttributeLookupTable`. Extend `AttributeLookupTable` with an out-parameter or sibling accessor that returns the parsed ID section separately, then assign `pCurrentComponentRecord->id`. `index` assignment stays untouched at `:478`.

**Wire:** add `set_id(id)` beside `set_index(index)` (`:215`) and `id = message.id()` in `ComponentRecord::from_protobuf`. Both are `optional uint32`, so presence is expressible — **absent must stay absent**; do not write `set_id(0)` for designs lacking `;ID=` (D2).

### M1.2 Parse the feature UID

Same shape, same leak: `FeaturesFile.h:66` declares `id`, `FeaturesFile.cpp:976` serializes it, and six `ParseAttributeLookupTable(attrIdString)` call sites (`:290`, `:367`, `:459`, `:554`, `:577` commented, `:621`) discard it. Wire the accessor through all live call sites, not just one. Feature volumes are far larger (11,868 UIDs in one `sample_design` layer) — measure the payload delta in M2.4, since this one *does* grow the response.

### M1.3 Remove `attributeLookupTable["ID"]` — **DECIDED (D3), and atomic with M1.1**

Drop the key. Dual-write/deprecate was proposed and rejected: the entry conflates an entity identifier with attribute assignments and invites the exact design error behind this defect, and there is no compatibility to preserve — re-verified 2026-09-15, no reader in either client (only a debug log of key *counts* at `OdbDesignGrpcClient.cs:369-377` and a mock writing `["0"]` at `MockOdbDesignClient.cs:183`) and the server only round-trips the map (`ComponentsFile.cpp:229`, `:265`).

**Land it inside the M1.1 commit, not after it.** Populate `id` and delete the key together: split, and there is a window where neither the typed field nor the map carries the UID — strictly worse than today.

**Same commit must fix the contract text.** #594 merged swagger stating the UID arrives "only as the literal key `"ID"`" and marking `id` as not-yet-populated. Both sentences become false the moment this lands, and the spec has a CI job now that will faithfully propagate the lie to the ConfigMap. Rewrite `ComponentRecord.id`, `ComponentRecord.attributeLookupTable`, and `FeatureRecord.id` in the same change.

Implementation note: the key is created by the **shared** `AttributeLookupTable::ParseAttributeLookupTable` third-section branch (`AttributeLookupTable.cpp:48-61`), which has **9 live call sites across four record types** — components (`ComponentsFile.cpp:489`), features (`FeaturesFile.cpp:290`, `:367`, `:459`, `:554`, `:621`; `:577` commented), and eda-data net + package records (`EdaDataFile.cpp:648`, `:906`). Changing only the component path leaves the `"ID"` entry in feature, net and package attribute maps. Fix it in the shared function — return the parsed id to the caller instead of inserting it into the map — and give each record type's `id` field the value, which also means NET and PKG UIDs get typed homes rather than staying smuggled.

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

**D4 decided: `Connectivity` is always-on** — no request flag, no second flavour. See [component-connectivity.md](component-connectivity.md) §D4 for the reasoning (an opt-in flag costs a full `FileArchive` re-serialize per request; `Turbot` is ~537 MB uncompressed).

### M2.1 `Connectivity` message + derive

**Files:** `OdbDesignLib/protoc/connectivity.proto` (new) or extend `design.proto`; `OdbDesignLib/ProductModel/Design.{h,cpp}`.

`design.proto` uses tags 1–11 → `optional Connectivity connectivity = 12;`. Protos are globbed (`OdbDesignLib/CMakeLists.txt:55`, `file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")`) — a new file needs a CMake **reconfigure**, not just a build.

Emit it from **both** flavours, unconditionally: `Design::to_protobuf(bool)` (`Design.cpp:167`) for gRPC and the no-arg `to_protobuf()` (`Design.cpp:159-165`) for REST/`to_pbstring`/round-trip tests. Per D4 it is always-on, so it sits **outside** the `includeNormalizedLists` gate — a client must never have to know to ask for connectivity. The `ProtobufSerializationTests.cpp` round-trip should assert it survives, which only works if the no-arg override populates it.

Derive from `m_nets[].GetPinConnections()`, which already holds resolved `(shared_ptr<Component>, shared_ptr<Pin>)` pairs. **This is a serialization of an existing result — do not write a second join.** Package rosters dedupe by name (71 entries, not 813). Emit `netMembers` from the same pass, sorted by net ordinal then refDes, so output is deterministic and diffable.

### M2.2 Semantics absorbed server-side

`Connection.Kind`: `UNCONNECTED` for netNumber `-1`/`4294967295`; `$NONE$` mapped to `UNKNOWN`/`UNCONNECTED` rather than leaking a magic net name (the server's own `BreakSinglePinNets`/`BuildNoneNet` are disabled at `Design.cpp:138-139`, so NC pins currently ride `$NONE$` — decide whether this contract revives them or just labels them). `ComponentPin.number` resolved from the package pin, so `toeprintNumber` never reaches a client.

Keep the hard failure visible: `Design.cpp:465` `GetPin(pinNumber)` null → whole load fails. Do not degrade that to omission; surface which pin failed.

### M2.3 Cache and payload check (D4 resolved: no flag)

`GetDesignRequest` gains **nothing** - no `include_connectivity`, no second flavour - so `OdbDesignServiceImpl::GetDesign`'s warm path keeps serving the single cached `std::string` and the pre-serialization worker keeps caching exactly what it caches today (see M0.4). Verify rather than assume: assert the warm-path response for an already-warm design contains a non-empty `connectivity` block.

What always-on does change is that every response grows by the connectivity payload, including responses that ignore it. That is the accepted cost of the decision, and M2.4 is what bounds it. If M2.4 measures materially more than the ~tens of KB modelled in section 4.2 for a large board, bring the number back before merging - D4 was made on the model, not on measurement.

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
* **M4.3** **Dropped as a step.** It used to read "remove the dual-write in Phase 4"; D3 rejected dual-writing outright, so the key is deleted atomically with M1.1 and nothing is left for Phase 4. Reopen only if an unenumerated consumer surfaces after M1.1/M1.3 ships — that is the sole risk D3 accepted.
* **M4.4** The dead `BuildPlacementsFromEdaDataFile` (`Design.cpp:521`, zero call sites; declared `Design.h:104`): delete it, or make it the authoritative path deliberately. Leaving an alternative connectivity implementation in the tree is how this divergence stayed invisible.

---

## Risks

| Risk | Mitigation |
|---|---|
| `id` populated before a client stops resolving by `Id` → ~10% silent mis-resolution on `Panel`-shaped designs | Phase 3 deletes that resolution. **This is a deploy constraint, not a merge one** — M1.1 may merge any time, but do not ship a server build with `id` populated into an environment whose clients still resolve `componentNumber` by `Id`. Coordinate rollout per environment and verify the §4.3 collision case in the M3.3 fixtures. |
| `Connectivity` as an opt-in flag re-serializes the whole FileArchive per request | M0.4 — measure before D4; `include_normalized_lists` already costs this, and a second flag doubles it. |
| `nam20485` goes stale under the branch again (`dev/*` → `development` shortcuts) | Sync from `nam20485` before opening each PR; the Phase 2 gate in the first revision of this plan was wrong for exactly this reason (§Branch base). |
| `Connectivity` and `fileModel` disagree for some design | M3.3 differential test is exactly this. A disagreement is a server bug, not a client bug, by construction. |
| Cached bytes omit `connectivity`, or a non-default flavour is served from cache | M2.3 — always-on means connectivity rides the single cached default flavour; assert the warm-path response contains it. |
| Feature UID delta regresses payload | M2.4 measures M1.2 before it merges. |
| Info client transport switch swells this PR's blast radius | Separate PR in a separate repo, reviewable alone (M3.2). |
