# Component connectivity — Phase 2 execution plan

| | |
|---|---|
| Purpose | File-level execution plan for the next steps of the connectivity epic: server Phase 2 (M2.1–M2.4), the Phase 2 deploy, and the client fan-out it unblocks (F6 → F4/F5). Derived from [program-status-2026-10-21.md](program-status-2026-10-21.md); milestones inherit their definitions from [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) and the contract/decisions in [component-connectivity.md](component-connectivity.md). |
| Status | **MERGED — PR #606 (`5a074c9`, 2026-10-22) closes server Phase 2. Differential tests pass against all four goldens; M2.4 gates passed (sample 119.5 KB ≤ 150 KB · 350-41017 0.59% ≤ 10%). Image `nam20485-1279`/`nam20485-latest` published; k3s rollout pending node access (see checklist step 10). Remaining: client fan-out (F6 → F4/F5, both client repos).** |
| Ground truth | Verified 2026-10-21 against `nam20485` @ `785ae26` (local FF'd after arriving 5 behind). **#603 merged 2026-10-22 — Phase 1 is CLOSED**; this supersedes the program status doc, which was written while #603 was still open. Every code claim below was re-grepped on that tip, not inherited. |
| Branch plan | One branch `nam/connectivity-phase2` cut from `nam20485`, **one PR** carrying M2.1→M2.4 as ordered commits; M2.4's size numbers in the PR description are the merge gate. PR base `nam20485`, merge commits only (`gh pr merge --merge`). |
| Build / test | `cmake --preset linux-dynamic-release && cmake --build --preset linux-dynamic-release && ctest --preset linux-dynamic-release`. **A CMake reconfigure is required** — protos are globbed (`OdbDesignLib/CMakeLists.txt:55`), so the new `connectivity.proto` is invisible to an existing build dir. |

---

## 0. What changed since the program status doc (2026-10-21 → today)

- **#603 (M1.4) is MERGED** (`785ae26`). Phase 1 is complete: M1.1+M1.2+M1.3 (#601) + M1.4 (#603). The plan-doc Status row still says "M1.4 next" — this PR updates it, same as #601 did.
- The epic's remaining server work is exactly Phase 2. Nothing else gates the clients.

## 1. Verified ground truth the plan rests on (all re-checked on `785ae26`)

Facts that shape the design, each verified this session:

1. **`design.proto` tags 1–11 are used; field 12 is free** for `optional Connectivity connectivity = 12;` exactly as the contract assumes.
2. **Both `to_protobuf` flavours route through one implementation**: the no-arg override (`Design.cpp:159`) delegates to `to_protobuf(/*includeNormalizedLists=*/true)` (`:167`). Normalized collections (`m_nets`, `m_components`, `m_packages`, …) are emitted only under that gate. `Connectivity` will be emitted **outside** the gate (D4 always-on), derived from the in-memory members.
3. **`Net::GetPinConnections()` has no const overload** (`Net.h:24` returns `PinConnection::Vector&`), but `to_protobuf() const` is const. A const overload is a required enabler edit.
4. **The ProductModel `Component` has no UID member** (`Component.h`: refDes, partName, package, index, side, part only). The #601 UID lives in the *FileModel* `ComponentRecord.id` (`std::optional<unsigned int>`). The contract's `PlacedComponent.id` cannot be emitted without threading it through → **DP1**.
5. **`subnetNumber` is already in hand at the build call site but discarded** — `Design.cpp:443` has `//auto subnetNumber = pToeprintRecord->subnetNumber;` commented out directly above the `CreatePinConnection` call. `PinConnection` has no subnet member. The contract's `Connection.subnets` needs it threaded → **DP2**.
6. **Unconnected toeprints never reach the product model**: `BuildPlacementsFromComponentsFile` skips `netNumber == -1` records entirely (`Design.cpp:447-450`) — they create no `PinConnection`. `Kind.UNCONNECTED` pins must come from the package-roster set-difference (connected pins of a component, subtracted from its package's pin roster) → **DP3** covers the ordering consequence.
7. **The goldens are richer than the contract sketch**: `OdbDesignTests/Fixtures/Connectivity/*.golden.json` record per pin `{ordinal (toeprint), pinNumber, name, netOrdinal, subnetOrdinal}` and per component `{refDes, uid, side, index}`; `netOrdinal` is `null` for the unconnected sentinels. The differential test can therefore assert UID, subnet and pin-name fidelity, not just `(pinNumber, netOrdinal)`.
8. **`$NONE$` is NET ordinal 0 in every design** (all four goldens) and its members are counted as *connected* by the goldens (e.g. `sample_design` MTG1/MTG2 mounting holes ride ordinal 0). `BuildNoneNet`/`BreakSinglePinNets` stay disabled (`Design.cpp:138-139`) — Phase 2 labels the convention, it does not revive them → **DP4**.
9. **Connectivity coverage per fixture** (from the goldens): `sample_design` 2,811/2,811 connected, 0 unconnected; `Panel-g7162` 0/3,078 connected (all UNCONNECTED); `200-40628` 0/129,698 (all UNCONNECTED, `fidelity: sampled`); `350-41017` 158,742/158,742 connected (`fidelity: sampled`). No single fixture exercises both kinds — the differential test must run at least `sample_design` + `Panel-g7162`.
10. **`DesignFetchBenchmarkTests.cpp` exists** (opt-in via `ODB_DESIGN_FETCH_BENCH=1`, cold/warm timings, `scripts/benchmark-design-fetch.sh` driver) — M2.4 extends it with payload-size scenarios rather than creating a new vehicle. `ResponseCacheTests.cpp` exists for the M2.3 warm-path assertion.
11. **Warm path**: `OdbDesignServiceImpl::GetDesign` serves cached bytes only for the default flavour (`OdbDesignServiceImpl.cpp:169-181`, `!includeNormalizedLists && TryGetDesignBytes`). `GetDesignRequest` gains **nothing** in Phase 2 (D4).
12. **`GetPin` hard-failure stays**: `CreatePinConnection` returns false on unresolvable pin → whole design load fails (`Design.cpp:465-477` region). Phase 2 must not soften this (design doc §8 hazard 1).

## 2. Decisions DP1–DP4 — adopted 2026-10-21

The living docs left four things genuinely open. All four are **decided** — the plan's recommendations were adopted on review. Rejected alternatives are recorded design-doc style so nobody re-litigates them without new evidence.

| # | Question | **Decision (adopted)** | Rejected alternative |
|---|---|---|---|
| **DP1** | How does `PlacedComponent.id` (the ODB++ UID) reach the derive, given the ProductModel `Component` lacks it? | **Thread it through the ProductModel**: `BuildPlacementsFromComponentsFile` passes `pComponentRecord->id` into the `Component` (new `std::optional<unsigned int> m_id` + ctor param), plus `component.proto` `optional uint32 id = 7` + `to_protobuf`/`from_protobuf` wiring. Additive wire change; the D7 push-PR carries it to clients. Survives round-trips and works after `from_protobuf`. | Read the UID from `m_pFileModel` at derive time — less code, but breaks after a protobuf round-trip (fileModel is not restored) and couples the derive to the FileArchive. |
| **DP2** | Does `Connection.subnets` get populated (contract specifies it; `PinConnection` lacks the member)? | **Thread it**: uncomment `Design.cpp:443`, add `unsigned int subnetNumber` to `PinConnection` (ctor arg; in-memory only — **not** serialized into the normalized `pinconnection.proto`, so no wire change there). The goldens already record `subnetOrdinal`, so the differential test asserts it. | Emit `subnets` empty and document as reserved — cheaper, but leaves the golden's subnet column untested server-side, and a later thread re-touches the same code. |
| **DP3** | `PlacedComponent.pins` order — the contract comment says "in toeprint order", but toeprint order is not reconstructible from `m_nets` (per-net vectors lose the global build sequence). | **Emit in package-roster order (pin number ascending)** and update the contract comment. Deterministic, diffable, derive stays FileModel-free. The differential comparator sorts both sides by `pinNumber`, so the golden's toeprint `ordinal` remains an assertion target, just not the emission order. | Thread a toeprint ordinal through `CreatePinConnection` (same shape as DP2) to preserve exact toeprint order — one more ctor arg, worth it only if a client is known to need toeprint sequence; none is. |
| **DP4** | `$NONE$` semantics (M2.2's open "label vs revive" question). Sharpened by fact 8: `$NONE$` is always ordinal 0 and carries real members (e.g. `sample_design`'s MTG mounting holes). | **Label it**: pins whose resolved net name is `$NONE$` emit `Kind.UNCONNECTED` with `net` left unset **and are excluded from `netMembers`**; the `nets` roster stays **dense** (every eda-data NET record in sequence, `$NONE$` name kept verbatim) so `net` ordinals keep their spec identity (p.153: ordinal = NET record sequence) and the golden cross-check holds. Clients branch on `kind`, never on the name (conformance rule 2). `BuildNoneNet`/`BreakSinglePinNets` stay disabled. | `Kind.UNKNOWN` instead — would distinguish "grouped-but-no-signal" from `net_num = -1`; nothing needs that today. Reviving `BuildNoneNet`/`BreakSinglePinNets` — changes net construction, far beyond "absorb semantics". |

## 3. Milestones

### M2.1 — `connectivity.proto` + always-on derive

**Branch:** `nam/connectivity-phase2` from `nam20485` @ `785ae26`.

**New file `OdbDesignLib/protoc/connectivity.proto`** — the §6 contract verbatim (nested `NetEntry`, `PackagePin(s)`, `Connection{Kind}`, `ComponentPin`, `PlacedComponent`, `Member`, `NetMembers`; top-level fields `nets=1, packages=2, components=3, netMembers=4`), package `odbdesign.productmodel` matching the sibling protos, plus a header comment stating the identity rule (refDes / package name / net ordinal; message-local ordinals never persisted).

**`design.proto`:** add `optional Connectivity connectivity = 12;` after `partsByName = 11`.

**Enabler edits (small, each justified by a verified gap):**
- `Net.h`: `const PinConnection::Vector& GetPinConnections() const;` overload (const-correctness for the const derive).
- UID threading (DP1): `Component` optional `std::optional<unsigned int> m_id` + ctor param + `component.proto id = 7` + both serializations.
- Subnet threading (DP2): `PinConnection` subnet member + ctor arg (default 0), `Design.cpp:443` uncommented and passed.

**Derive** — new private helper `Design::BuildConnectivity()` (or free function in `Design.cpp`) called from `to_protobuf(bool)` **after** the normalized-lists block, outside the gate, populating `pDesignMsg->mutable_connectivity()`:

1. `nets`: iterate `m_nets` in order → `{ordinal = net->GetIndex(), name}`. Dense, includes `$NONE$` (DP4).
2. `packages`: iterate `m_packages` → `{package = name, pins = [{number = pin->GetIndex(), name}]}` in roster order. Deduped by construction (71 entries for `sample_design`, not 813).
3. `components`: for each `m_component` → `{refDes, id (if set), side, package = package name, pins}`. Pin list built by unioning: (a) every edge in `m_nets[].GetPinConnections()` whose component matches → `{number = pin->GetIndex(), name = pin->GetName(), connection = {kind, net, subnets = [subnet]}}`, where kind/net are `CONNECTED` + the net ordinal — **except edges onto the `$NONE$` net, which emit `UNCONNECTED` with `net` unset and join no `netMembers` entry (DP4)**; (b) remaining package-roster pins not hit by any edge → `Kind.UNCONNECTED`, `net` unset. Sorted by pin number (DP3). `netMembers` from the same single pass (net ordinal → `{refDes, pins[]}`), sorted by net ordinal then refDes for determinism.
4. This is a **serialization of an existing result** — the edges come from `m_nets`, the rosters from `m_packages`. No new join is written; the only FileModel touch is the DP1/DP2 threading at *build* time, where the values are already in hand.

**Tests (same PR):**
- New `OdbDesignTests/ConnectivityContractTests.cpp` — differential vs the M0.3 goldens: load fixture → `FileArchive` → `Design::Build` → `to_protobuf(false)` → walk `Connectivity` → compare against `*.golden.json` (components: refDes/uid/side; pins sorted by pinNumber: pinNumber, name, netOrdinal, subnetOrdinal; pin counts per net). **One DP4 comparator rule:** a golden `netOrdinal` equal to the `$NONE$` ordinal asserts as `UNCONNECTED` with `net` unset — and such pins must be absent from `netMembers`. Full fidelity on `sample_design` + `Panel-g7162`; the two `sampled` designs assert their recorded subset + aggregates (bounded CI cost). `Panel-g7162` and `200-40628` are the UNCONNECTED-kind coverage; `sample_design` the CONNECTED + `$NONE$` coverage (fact 9).
- `ProtobufSerializationTests.cpp` — round-trip: `to_protobuf()` (full) → `from_protobuf` → `to_protobuf()` → connectivity survives (this only works because the full flavour restores `m_nets`/`m_components` and the derive re-runs; assert UID survival too, which is DP1's payoff).
- Existing suites green: `FileArchiveLoadTests`, `ComponentHeightPipelineTests`, `AttributeLookupTableTests`.

**Mechanics to not trip on:** wipe nothing, but **reconfigure** (fact: globbed protos). Never commit generated `.pb.*` (repo convention; zero tracked today). Do not gate emission on `includeNormalizedLists` (D4). Do not add any `GetDesignRequest` field (M2.3).

**Docs in the same PR:** update the living plan doc Status row (Phase 1 closed, Phase 2 executing), and the contract §6 comment per DP3/DP4 outcomes.

### M2.2 — semantics absorbed server-side

Realized *inside* the M2.1 derive (same commit or the next on the branch — not a separate PR):

| Source convention | Emitted as |
|---|---|
| `TOP netNumber == -1` (wire `4294967295`) | Pin absent from `m_nets` → `Kind.UNCONNECTED`, `net` unset (roster difference) |
| Net name `$NONE$` (ordinal 0, members present) | `Kind.UNCONNECTED`, `net` unset; roster entry stays dense (ordinal identity preserved), members excluded from `netMembers` (DP4) |
| `toeprintNumber → pinNumber` | Resolved at build time already; clients get `ComponentPin.number` and never see a toeprint ordinal as a pin number |
| `CreatePinConnection` pin-resolution failure | **Unchanged hard failure** of the whole load, existing log names the design/pin — explicitly *not* degraded to omission (§8 hazard 1) |

Contract tests above assert each row (`Panel`/`200-40628` for row 1, `sample_design` MTG pins for row 2).

### M2.3 — warm-path cache assertion

`GetDesignRequest` unchanged. Extend `ResponseCacheTests.cpp` (the existing cached-bytes test vehicle): warm a design through the same pre-serialization path the service uses, fetch via the cached-bytes branch (default flavour), parse, assert `has_connectivity() && connectivity().components_size() > 0`. This is D4's proof that connectivity rides the single cached default flavour — verify, not assume (per the living plan's own warning).

### M2.4 — size benchmark (merge gate)

Extend `DesignFetchBenchmarkTests.cpp` (opt-in, same env-knob pattern) with serialized-size scenarios on `sample_design`, `350-41017`, `Panel-g7162`:

- report total response bytes, `connectivity` submessage bytes, `fileModel` submessage bytes;
- **acceptance: `connectivity` ≤ 150 KB on `sample_design`, and ≤ 10% of the `fileModel` payload on `350-41017`** (81,157 components, 100% connected — the stress case);
- discharge the outstanding **M1.2 feature-UID delta** measurement: serialize the fileModel message, then a copy with all feature `id` fields cleared, report the difference (deterministic, no need to check out pre-#601);
- note the constraint: `GetDesign` on `350-41017` already OOMs the 10 Gi pod (~12.6 GiB est. peak) — the 10% bound is what keeps Phase 2 from making that meaningfully worse. If the budget is blown, **the PR does not merge** — revisit the emission (e.g. drop `name` from `PackagePin` where it equals `number` as a string) before proceeding. D4 was decided on the ~tens-of-KB model; this is where the model meets the measurement.

**PR description must carry the measured numbers.** Merge gate = numbers pass + kilo review addressed + required `nam20485` checks (Codacy, CodeQL ×4 legs, dependency-review) green.

### M2.1–M2.4 PR mechanics (single PR)

1. Swagger **#1** (`swagger/odbdesign-server-0.9-swagger.yaml`): add `Connectivity` + nested schemas, reference from `Design.connectivity` — same commit as the proto (the #601 lesson: contract text changes with the wire, never after).
2. Swagger **#2** (`deploy/kube/OdbDesignServer-SwaggerUI/swagger-spec-configmap.yaml`): regenerate via the `deploy.ps1` command and commit **in the PR** — the sync workflow's auto-commit cannot land on `nam20485` (409 under the ruleset, documented in #605); carrying it ourselves means the workflow takes its "already matches" no-op branch on merge.
3. Update living plan-doc Status row (Phase 1 closed / Phase 2 merged-pending), this plan's checkboxes.
4. `gh pr merge --merge` (merge commits only, per directive).

### M2.5 — deploy + live acceptance (closes Phase 2)

The epic's own rule: a fix isn't done until it's deployed through the full chain. Phase 1+#601 has **not** been deployed (last confirmed cluster roll is #600 on 10-15), so this roll carries Phase 1 + Phase 2 together:

1. Merge to `nam20485` triggers the docker publish → `ghcr.io/…:nam20485-latest` advances (verify the workflow run and the pushed digest).
2. `kubectl rollout restart deployment/odbdesign-server-v1` on the k3s cluster (`deploy.ps1` is init-only; no image watcher exists). Cluster reachable via Tailscale — the status doc recorded it unreachable from this machine today; if still unreachable, this step waits for access rather than being skipped.
3. Live acceptance: FixtureCapture (or a client conformance run) against the live endpoint asserting `connectivity` present and golden-conformant on `sample_design`; spot-check a `$NONE$` pin reads `UNCONNECTED`.
4. Record the roll (design, digest, date) in the ops notes, as #600's roll was.

### Phase 3 fan-out — client work this unblocks (separate repos, listed for sequencing only)

Executed **after** the Phase 2 merge (field numbers final then; the handoff doc §7 warning: do not code clients against the contract text before the proto merges):

1. **F6 push-PRs (D7)** — from the server side, copy `connectivity.proto` + updated `design.proto` + `component.proto` (DP1) into both clients' vendored trees (`odbdesign-3d-client-prototype/src/OdbDesign3DClient.Core/Protos/`, `Odbdesign-info-client-india79-b/protoc/`), as one PR per client repo. Client codegen runs on their side.
2. **F4/F5 (3D, plan M3.1)** — delete the 562-line joiner in `ComponentDetailBuilder.cs`; rebuild `ComponentDetailIndex` over `Connectivity.components` keyed `refDes` (`StringComparer.Ordinal`); net tab from `netMembers`; branch on `Kind`, never on sentinels. Drafted against their F2 harness.
3. **F4/F5 (info, plan M3.2)** — same deletion shape (531 lines) in `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` + consume in `DesignService.cs`; transport is already gRPC (#23), so no prerequisite remains.
4. **M3.3 is already satisfied**: both clients' F2 conformance harnesses test against the server goldens; the F4/F5 PRs run them.
5. Practical constraint: client-repo git work must run from sessions rooted in those repos (the daemon shell guard blocks cross-repo git even read-only).

## 4. Execution checklist

```text
[x] 0. (done) FF nam20485 → 785ae26; #603 confirmed merged — Phase 1 closed
[x] 1. Cut nam/connectivity-phase2 from nam20485
[x] 2. M2.1 connectivity.proto + design.proto field 12 + enabler edits (Net const overload, DP1, DP2)
[x] 3. M2.1 derive in to_protobuf(bool), outside the normalized-lists gate; reconfigure CMake
[x] 4. M2.2 semantics per the mapping table (rides the derive)
[x] 5. Tests: ConnectivityContractTests (differential vs 4 goldens), round-trip, suites green
[x] 6. M2.3 warm-path assertion in ResponseCacheTests
[x] 7. M2.4 size scenarios in DesignFetchBenchmarkTests; record numbers + M1.2 delta
[x] 8. Swagger #1 schemas + regenerate #2 in-PR; living plan-doc Status row update
[x] 9. PR #606 → nam20485; kilo review (3 findings fixed, threads resolved); all required checks green; MERGED 2026-10-22 as `5a074c9` (merge commit, per directive)
[x] 10. M2.5 COMPLETE (2026-10-22). Publish: docker-publish run `39738305784` pushed `nam20485-1279`; the follow-up docs commit re-published as `nam20485-1280` (content-identical) which became the running tag. Rollout: `kubectl rollout restart deployment/odbdesign-server-v1` after the node came back; pod `odbdesign-server-v1-5bf6979b7d-zlttd` runs `nam20485-latest` @ `sha256:d47776dd...`, digest-verified against ghcr, `imagePullPolicy: Always`. Live acceptance (grpcurl -> GetDesign sample_design, default flavour): connectivity present, normalized lists pruned, and **every aggregate matches the M0.3 golden exactly** - 813 components (813/813 UIDs), 644 nets, 71 packages, 2,725 CONNECTED pins + 86 `$NONE$`->UNCONNECTED pins (golden: 2,811 toeprints = 86 + 2,725), netMembers 643 entries / 2,725 edges; MTG1's `$NONE$` pin reads kind=UNCONNECTED with net unset (DP4 live). Also verified on merge: swagger-spec-configmap-sync took its "already matches" no-op branch (success, no 409).
[ ] 11. F6 push-PRs (both clients) → their F4/F5 (client-rooted sessions)
```

**Local verification record (2026-10-21/22, `linux-dynamic-release`):** all four golden differentials pass (sample full-fidelity; Panel + 200-40628 zero-connected; 350-41017 sampled subset), round-trip + always-on + warm-path tests pass, full ctest green except the pre-existing #599 StepHdrFile intermittent (~2/3 fail rate reproduced on the clean tree — not touched by this branch). Two toolchain findings worth keeping: protobuf `Struct` map *lookups* (`find`/`count`) return end() for present keys on JsonStringToMessage-built maps (test accessors use a linear scan), and a compile definition placed before its CMake variable is computed expands empty (the `ODBDESIGN_SOURCE_DIR` define initially sat above the `REPO_ROOT` set — caught only by ctest's build-dir cwd).

## 5. Risks

| Risk | Mitigation |
|---|---|
| Size budget blown on `350-41017` (model says tens of KB; 158,742 connected pins is the stress case) | M2.4 is the merge gate — PR does not merge on a red number; fallback emission tweaks listed in M2.4 |
| Connectivity growth worsens the known GetDesign OOM (`350-41017` ~12.6 GiB vs 10 Gi pod) | The 10% bound caps the addition; the OOM itself is pre-existing, tracked separately (capacity memory), explicitly out of scope here |
| Derive accidentally gated on `includeNormalizedLists` or a new request flag | D4 restated in M2.1 mechanics + M2.3 test would catch it (cached default-flavour bytes must contain connectivity) |
| New proto invisible to the build (globbed protos need reconfigure, not just build) | Called out in the header table; first build after adding the file must reconfigure |
| Stale `nam20485` base (the `dev/*`→`development` shortcut anomaly) | Branch cut from freshly-FF'd `785ae26`; re-check `git fetch` before opening the PR (living plan §Branch base) |
| Client protos drift until F6 lands | Known, accepted (D7); clients hold off coding until the server merge makes field numbers final |
| Swagger #2 regenerated late | Carried in-PR (the 409 limitation); the sync workflow then no-ops on merge |

## 6. Out of scope (explicitly)

Phase 4 debt (M4.1 conditional, M4.2 deferred, M4.4 `BuildPlacementsFromEdaDataFile` deletion — natural follow-up PR after Phase 2), the GetDesign OOM investigation, M0.6 SwaggerUI image spec sync, #599 StepHdrFile intermittence, auth epic, WebSocket events, REST surface (frozen per D5). The 3D client's unfiled mirrored-silkscreen bug is client-side and parallel to all of this.
