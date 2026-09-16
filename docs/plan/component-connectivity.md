# Component identity and server-owned connectivity

| | |
|---|---|
| Status | **ACTIVE — design agreed, implementation planned.** Decisions D1–D6 below are **open** and block Phase 2/3. |
| Source | Client report: [component-id-issue.md](component-id-issue.md) (drop-in from the 3D client agent, 2026-09-14; committed with this doc set). |
| Plan | [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) — file-level milestones. |
| Related | [ipc2581/spec-issues.md](ipc2581/spec-issues.md) (IPC imports leave `m_pFileModel` null) · [ipc2581/Client Migration Guide_ ODB++ to Unified API.md](ipc2581/Client%20Migration%20Guide_%20ODB%2B%2B%20to%20Unified%20API.md) · [server-issues.md](server-issues.md) (SI6 response cache, touched by M2.3) |
| Base | Verified against `nam/component-connectivity` @ `d7d1a5b`. **Line numbers in this doc are that base.** It contains the normalized-lists prune (`274d878`, PR #590) and the DesignCache concurrency fixes (`ca9d5c8`, PR #591), both of which had merged to `development` and were reconciled back into `nam20485` (`ea2c081`) on 2026-09-15. See §Branch base in the plan. |
| Method | One `nam/<feature>` branch → PR → `nam20485`, **merge commits only** (AGENTS.md, directive 2026-09-10). |

---

## 1. The reported defect

Every `ComponentRecord` the server emits reports `id == 0`. The 3D client keyed its per-component net list on `(side, id)`, so all components on a board side collapsed into one shared bucket — a 1-pin component "connected to" the entire side's net set. Two clients are affected.

**The report's diagnosis of the symptom is correct. Its conclusion — that no design has real unique component IDs — is not.** ODB++ defines a per-component unique ID, most real designs carry it on 100% of records, and this server already parses it and already ships it — into the wrong field.

## 2. Verified state of the code

### 2.1 `id` is declared, documented, and never populated

| Fact | Evidence |
|---|---|
| Proto field exists | `componentsfile.proto:40` `optional uint32 id = 9;` |
| Struct member exists, never assigned | `ComponentsFile.h:48` `unsigned int id;` — grep for every `set_id`/`->id =` in `OdbDesignLib` finds file-level ids (`ComponentsFile.cpp:89`), matrix step/layer records, stephdr, and `PinRecord.id` — **never the component record** |
| `to_protobuf` omits it → absent on the wire → reads `0` | `ComponentsFile.cpp:207-215` sets pkgref/x/y/rot/mirror/compname/partname/**index**, no `set_id` |
| `from_protobuf` omits it too | round-trips as 0 in both directions |
| Swagger advertises it | `swagger/odbdesign-server-0.9-swagger.yaml` `ComponentRecord.id` → *"Component id."* |

`ComponentRecord` has no user-provided constructor, so `make_shared<ComponentRecord>()` value-initializes and `id` is deterministically `0`, not garbage.

### 2.2 `index` is per-side, and is **not** a foreign key

`ComponentsFile.cpp:478` assigns `index` as the record's ordinal **within that file**; the spec (p.148) puts top and bottom components in separate layers → separate files. So `index` restarts at 0 per side. `(side, index)` is unique; `index` alone is not.

**Correction to the report's framing.** The report concluded that `(side, index)` is "the server's real foreign key," because the netlist's `SubnetRecord.ComponentNumber` is a per-side ordinal and `Design::CreateNetConnections` joins on it (`Design.cpp:559`). That code path **never runs**. `Design::Build` calls exactly one placement routine:

```text
Design.cpp:135   BuildPlacementsFromComponentsFiles()      <-- the only one called
Design.cpp:521   BuildPlacementsFromEdaDataFile()          <-- defined, zero call sites (Design.h:104)
Design.cpp:138   //if (! BuildNoneNet()) return false;      <-- disabled
Design.cpp:139   //if (! BreakSinglePinNets()) return false; <-- disabled
```

The live path (`Design.cpp:433-456` → `CreatePinConnection`, `:458`) resolves **component identity = refDes** (`m_componentsByName[refDes]`, `:462`) and **net reference = `ToeprintRecord.NetNumber`** (`:277` on the wire). The netlist-ordinal join is dead code. So the server and the clients run *two different implementations of connectivity* that agree only because `refDes` and the ordinal happen to line up. That divergence — not the zero `id` — is the root cause.

### 2.3 The UID exists, is parsed, and leaks into the attribute map

`AttributeLookupTable.cpp:48-61` splits the CMP attribute token on `;`, parses section 3, and stores `key="ID"` → UID string. `ComponentRecord` derives from `AttributeLookupTable` and serializes the whole map to `attributeLookupTable` (field 13, `componentsfile.proto:45`). So `attributeLookupTable["ID"]` **is** the component UID today, as a string, mixed into a map whose other keys are numeric attribute indices.

### 2.4 Two more fields in the same state

* `FeaturesFile::FeatureRecord.id` — declared (`FeaturesFile.h:66`), serialized (`FeaturesFile.cpp:976`), never parsed. Feature UIDs are abundant (`sample_design` `layer-1/features`: 11,868 `;ID=` lines). Same defect, larger blast radius, no consumer hit it yet.
* `ComponentsFile.componentRecordsByName` (field 10) — `m_componentRecordsByName` appears only in the destructor clear (`:31`) and the getter (`:67`). Never inserted, never serialized. Advertised in swagger, always empty.

## 3. What the spec says (Release 8.1 Update 4, August 2024)

Latest published version — `odbplusplus.com/design` lists 8.1 U4 as newest; no public 8.2/8.3. In-repo copy: `../OdbDesign-drc/docs/odb_spec_user.pdf` (byte-size-identical to the public download).

```text
p.152  CMP <pkg_ref> <x> <y> <rot> <mirror> <comp_name> <part_name>; <attributes>;ID=<id>
       ID=<id>  Assigns a unique identifier to the component. See "Unique ID" on page 30.
p.30   "The unique ID is optional, but once assigned, is never changed, and no other
        entity within the product model can bear the same ID. IDs are represented by a
        positive number between 0 and 4294967295."
p.30   File-header ID= line identifies the *file itself* (separate concept).
p.153  net_num "corresponds to the sequence of the NET records in the eda/data file.
        The first NET record is net_num 0…"
p.152  comp_name = "Unique reference designator… Upper case and lower case characters
        are not equivalent."
```

Three consequences that shape the design: the UID is **product-model-wide unique** (one ID space shared with features, matrix, stephdr — hence sparse), **permanent** across re-exports, and **optional**.

## 4. Evidence from real data

### 4.1 UID availability — 13 `components` files, 8 designs

| Design / step | side | comps | with `;ID=` | UID range |
|---|---|---|---|---|
| `sample_design/step` | top / bot | 440 / 373 | **440 / 373** | 61442..104101 / 62804..104138 |
| `designodb_rigidflex/cellular_flip-phone` | top / bot | 610 / 82 | **610 / 82** | 3896..4587 / 3936..4557 |
| `Panel g7162-31800_odb` (board step) | top / bot | 847 / 593 | **847 / 593** | 759..1605 / 1621..2213 |
| `Turbot/turbot_f200` | top / bot | 369 / 571 | **369 / 571** | 4130..5025 / 4133..5069 |
| `200-40628_Rev1_v7/pcb` (2016) | **top only** | 7,610 | **0** | — |
| `350-41017_rev1_odbjob_v7/stp` (2018) | **top only** | 11,631 | **0** | — |
| ODB2kicad `odb-kitchen-sink` / `odb-output` | top | 3 / 2 | **0** | — |
| `Panel g7162` / `panel-layout` | top / bot | 0 / 0 | — | — |

UIDs are unique per file and **never overlap top↔bottom** in any design, exactly as the spec promises. Half the corpus carries none at all, so optionality is not theoretical.

⚠️ **The last two rows are per-side, not design totals.** Measured later from the goldens (`scripts/gen-connectivity-golden.py`): real totals are **57,774** for `200-40628/pcb` (7,610 Top + 50,164 Bottom) and **81,157** for `350-41017/stp` (11,631 Top + 69,526 Bottom) — reading the table as totals understates by ~7×. Both were compressed as `components.Z`, so the plain-text `find` used for the other rows never saw their bottom side. Also: `200-40628` has **zero** connected pins (every `TOP` line is `net_num = -1`) while `350-41017` is **100% connected** — do not generalize one into the other.

### 4.2 Why the normalized lists are unaffordable

`sample_design`: 813 components, 644 nets, **2,811 pin connections**, **71 packages** (1,007 package pins, ~14/package).

`PinConnection` embeds a full `Component`, and `Component::to_protobuf` does `mutable_package()->CopyFrom(...)` + `mutable_part()->CopyFrom(...)` (`Component.cpp:75-76`). So 71 packages get re-serialized across 2,811 connections — ~40× redundancy, each carrying its whole pin roster.

Cost modelled at ~760 B/connection ≈ 2.1 MB wire, consistent with the **5.8 MB of the 12.2 MB design JSON** measurement that justified PR #590 (`274d878`), once JSON's ~2.8× string inflation is applied. The same facts as a reference-keyed index: **tens of KB**.

Note what #590 does and does not decide. `Design::to_protobuf()` (`Design.cpp:159-165`) still emits the **full** flavour — REST, `to_pbstring`, and the round-trip tests depend on it — and the gRPC `GetDesign` path calls `to_protobuf(false)` (`Design.cpp:167`, gated by `GetDesignRequest.include_normalized_lists`, `service.proto:51`). So the lists are *pruned from the gRPC response*, not removed from the model.

### 4.3 The mis-resolution landmine

Both clients resolve netlist `componentNumber` against `Id` **before** the ordinal (3D `ComponentDetailBuilder.cs:446→:451`, info `:415→:420`). Today that first lookup is inert (all ids 0 → ambiguous or empty → falls through to ordinal, which is right). Populate `id` server-side without changing a client and the lookup wakes up, matching **UIDs against per-side ordinals**:

| Design / side | ordinals that would false-match a unique UID |
|---|---|
| `Panel g7162` top (UIDs 759..1605, 847 comps) | **88 / 847 ≈ 10%** |
| all four other designs | 0 (UID ranges sit above the ordinal range) |

Turns a loud bug into a silent one on panelized boards.

### 4.4 The two clients are fork-pastes carrying the identical defect

| | 3D prototype | info client (`india79-b`) |
|---|---|---|
| Builder | `…/OdbDesign3DClient.Core/Services/Implementations/ComponentDetailBuilder.cs` | `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` |
| Size | 562 lines | 531 lines — **89 differing lines** after whitespace strip |
| `(side, Id)` key | `:14`, `:110`, `:144`, `:238` | `:74`, `:108`, `:207` |
| Id-first resolution | `:446`, `:451` | `:415`, `:420` |
| `toeprintNumber→pinNumber` join | `:479-481` | `:448-453` |
| Transport | gRPC | REST (`Api/IOdbDesignRestApi.cs`, `Dtos/`) + gRPC types via `OdbDesign.ProductModel.csproj:32-37` |
| Vendored protos | `src/OdbDesign3DClient.Core/Protos/` — **differs from server** | `protoc/` — **stale**: missing `GetStandardFonts`, `RequestLoadDesign`, `LoadStatus`, `include_normalized_lists` |

The same 550-line joiner exists twice, maintained independently, and both copies hold the same bug. Both vendored proto trees drift silently, with nothing checking them.

## 5. Decisions

### D1 — The identity model (three keys, three jobs)

| Key | Guarantee | Use | Never |
|---|---|---|---|
| `refDes` | unique per product model, spec-guaranteed, meaningful | **primary external key**; UI, cross-refs, maps | — |
| `(side, index)` | unique, but **positional** — renumbers if file order changes | internal join only, server-side | persisting, client-side joins |
| `id` (ODB++ UID) | product-model-wide, permanent, **optional** | durable handle for revision diffing | join key |

The client defect was collapsing these into one field. The report's counter-conclusion — "therefore `id` is worthless" — inverts the error: `id` is the only one of the three that survives a re-export.

### D2 — Populate `id` faithfully; **never synthesize** when absent

Parse the `;ID=` section into `ComponentRecord.id`. Absent means **unset** — proto3 `optional` already gives `has_id()` / `HasId`, so clients distinguish "no UID" from "UID 0". A synthesized id is a permanent lie that consumers cache across revisions.

### D3 — **DECIDED 2026-09-15: remove the `"ID"` key outright.** No dual-write, no deprecation window

Dual-writing was proposed and rejected: the key is misleading, conflates an entity identifier with attribute assignments, and invites exactly the design error that produced this defect. Its only merit would be backwards compatibility, and there is none to preserve — verified 2026-09-15 by grep across both client repos and `OdbDesignLib`: nothing reads `attributeLookupTable["ID"]`. The only client touches are a debug log of key *counts* (`OdbDesignGrpcClient.cs:369-377`) and a mock writing `["0"]` (`MockOdbDesignClient.cs:183`); server-side the map is only round-tripped (`ComponentsFile.cpp:229`, `:265`).

**Consequence — M1.1 and M1.3 are one atomic change.** Populate `id` and delete the `"ID"` key in the same commit, on the same branch, in the same release. Split them and a window opens where *neither* the typed field nor the map carries the UID, which is strictly worse than today. The swagger text merged in #594 currently states the UID is delivered "only as the literal key `"ID"`" — that sentence must be rewritten in the same commit, or the published contract lies in both directions.

### D4 — **DECIDED 2026-09-15: `Connectivity` is always-on.** No flag

Opt-in was rejected on the evidence, not for convenience. `OdbDesignServiceImpl`'s cache warm path serves pre-serialized bytes for **the default flavour only**, so any request that sets a flag re-serializes the entire cached `FileArchive` per call — `Turbot` is ~537 MB uncompressed. A second flag would reintroduce precisely the cost PR #590 removed. At ~tens of KB (§4.2), `Connectivity` is small enough to ride the default, which also means it is cached once with everything else and is present for every consumer without them having to know to ask.

Two follow-ons this decides: it becomes part of the cached default response, so M2.4's size acceptance is a merge gate, not a preference; and `design.proto` gets `optional Connectivity connectivity = 12` populated by **both** `to_protobuf` paths, so the REST/`to_pbstring`/round-trip flavours see it too.

### D5 — **DECIDED 2026-09-15: REST is frozen — security fixes only.**

The surface stays live and swagger-published but takes no new endpoints and no new fields. Concretely: `designs_component_route_handler` (`DesignsController.cpp:311`) and `designs_net_route_handler` (`:343`) stay unimplemented and should be **deleted** rather than left as live scaffolding that invites someone to build for a departing consumer; the existing collection routes stay as-is. This makes M4.2 a small deletion, not a verdict still pending.

**D4 vs D5, resolved.** D4 populates `connectivity` in *both* `to_protobuf` flavours, so the REST `to_json`/`to_pbstring` paths receive the field for free; D5 makes **gRPC the supported consumption path**. Not a conflict: the field is present on every flavour, and REST is simply not a surface anyone may build *new* consumption against. Read D5 as "no new REST surface", not "REST must not carry the data". (Surfaced by the m05 delegate, which flagged the apparent contradiction instead of silently picking a reading.)

### D7 — **DECIDED 2026-09-15: no sync mechanism. Server owns the protos; changes are pushed.**

A hash manifest or a cross-repo CI diff was proposed and rejected as unnecessary machinery for a 3-repo problem that a convention solves. The rule:

- **The server repo owns `OdbDesignLib/protoc/` and `OdbDesignServer/protoc/grpc/`.** It may change them when needed.
- **Clients do not own them.** They hold vendored copies and receive updates as a push — a PR to each client repo — made at the same time as the server change, never discovered later by a drift check.
- Additive only in practice: never renumber, never reuse a tag, never make an absent field ambiguous. Everything in this plan conforms — Phase 1 changes **no** `.proto` at all (`optional uint32 id = 9` already exists and was simply never assigned; removing the `"ID"` map key changes values inside a `map<string,string>`, not its type), and Phase 2 adds one field, `connectivity = 12`.
- Consequently **M0.2 is dropped**, not deferred: the drift it would have caught is a process problem, and the durable fix is the planned `odbdesign-model-libs` repo (protos + C# client lib + C++ server lib, apps consuming the libraries) — deliberately sequenced **after** the contract works, since extracting a schema and stabilising a C++/vcpkg build surface at the same time is the expensive combination.

Immediate consequence to watch: the info client's vendored copy is missing four shipped `service.proto` features *today*. Under this rule that is a debt to settle with one push PR before its `Connectivity` work starts (already listed in M3.2), not something a check would have prevented.

### D6 — The `PinConnection` denormalization is separate debt

Making `PinConnection` carry a component *reference* instead of an embedded `Component`+`Package`+`Part` is what eventually lets `include_normalized_lists` become unnecessary. Breaking wire change; interacts with the IPC-2581 unified API. Sequenced last, on purpose.

## 6. The contract

```proto
// ProductModel. Derived, format-independent, server-owned.
// Identity rule: refDes names a component, package name names a pin roster,
// net ordinal names a net. Message-local ordinals are never persisted and
// never cross a request.
message Connectivity {
  message NetEntry      { uint32 ordinal = 1; string name = 2; }   // "" = unnamed net
  message PackagePin    { uint32 number = 1; string name = 2; }
  message PackagePins   { string package = 1; repeated PackagePin pins = 2; }

  message Connection {
    enum Kind { UNKNOWN = 0; CONNECTED = 1; UNCONNECTED = 2; }     // absorbs netNumber -1 / $NONE$
    Kind kind = 1;  uint32 net = 2;  repeated uint32 subnets = 3;  // net = ordinal into `nets`
  }
  message ComponentPin { uint32 number = 1; string name = 2; Connection connection = 3; }
  message PlacedComponent {
    string refDes = 1;        // authoritative key
    uint32 id = 2;            // ODB++ UID; durable handle, never a join key
    BoardSide side = 3;       // informational only
    string package = 4;       // -> `packages`
    repeated ComponentPin pins = 5;   // in toeprint order
  }
  message Member     { string refDes = 1; repeated uint32 pin = 2; }
  message NetMembers { uint32 net = 1; repeated Member members = 2; }

  repeated NetEntry nets = 1;
  repeated PackagePins packages = 2;      // deduped: 71 rosters, not 813
  repeated PlacedComponent components = 3;
  repeated NetMembers netMembers = 4;     // same edges, inverted, for the net tab
}
```

Answers both pin questions the earlier draft missed: *what pins a component has* (`packages` roster + `components[].pins` instances) and *through which pins it reaches a net* (`pins[].connection`, inverted in `netMembers`). Both directions are the same 2,811 edges, so no client inverts anything.

It is **derived from what `BuildPlacementsFromComponentsFiles` already resolved** — `m_nets[].pinConnections[]` holds `(Component, Pin)` pairs in memory today. M2.1 serializes an existing result; it does not add a second join.

Three format leaks the contract absorbs, so clients stop needing ODB++ folklore:

1. **NC is a magic number.** `TOP` `netNumber = -1` lands in an `unsigned int` (`m_allowToepintNetNumbersOfNegative1 = true`), so clients see **4294967295** and must infer "unconnected" → `Connection.Kind.UNCONNECTED`. The server itself already skips those records (`Design.cpp:448`) but never tells the client it did.
2. **`toeprintNumber → pinNumber`** is currently done in client code (3D `:479-481`, info `:448-453`) — that is a join → resolved server-side into `ComponentPin.number`.
3. **`$NONE$`** is not broken out (`BuildNoneNet`/`BreakSinglePinNets` disabled, `Design.cpp:138-139`), so NC pins hang off a magic net name → `Kind`, not a name comparison.

## 7. Conformance rules for clients

"Not trusted to join" needs a testable line, not a preference:

1. Clients **may** group, sort, and index published fields locally (a `refDes → detail` dictionary is fine).
2. Clients **may not** resolve a reference from one collection against another — no matching `componentNumber` / `toeprintNumber` / `netNumber` against separately-fetched records, no interpreting format sentinels like `-1` or `$NONE$`. Those are the server's joins and its conventions.

Enforcement is structural: the contract **deletes** the duplicated 550-line joiner from both clients rather than specifying it more precisely to each. Both then implement ~50 lines of lookup against `Connectivity`.

Geometry, placement, and attributes stay in `fileModel` — that architecture is unchanged and correct. Only relationships move.

## 8. Hazards to keep visible

* ⚠️ `CreatePinConnection` calls `GetPackage()->GetPin(pinNumber)` and returns false on null (`Design.cpp:465`), which fails the **entire design load**. Publishing pin rosters must not convert that hard failure into a silent omission — keep reporting which pin failed to resolve.
* ⚠️ §4.3: populate `id` only after both clients stop resolving `componentNumber` by `Id`. The plan deletes that resolution in Phase 3, so the landmine is removed rather than dodged — but the ordering must hold if a client slips.
* ⚠️ `refDes` case sensitivity is significant (spec p.152), and **both clients ignore it**. 3D client: `ComponentDetailIndex` default (`ComponentDetailBuilder.cs:20`), net index (`:33`), `byName` (`:183`). Info client: `byName` (`:147`). All four use `StringComparer.OrdinalIgnoreCase`, which silently merges `R1`/`r1` on case-differing designs. Fix while touching the keying.

## 9. What was and was not verified

**Verified statically:** `ComponentsFile.{h,cpp}`, `AttributeLookupTable.{h,cpp}`, `FeaturesFile.{h,cpp}`, `Design.{h,cpp}`, `Component.cpp`, `Net.cpp`, the protos, swagger, `DesignsController.{h,cpp}`; every `id` assignment in `OdbDesignLib`; both clients' `ComponentDetailBuilder.cs`, csproj proto items, and vendored proto trees (diffed against server).
**Verified against data:** 13 `components` files across 8 designs, including two `.Z`-compressed legacy files decompressed to `/tmp` (no repo writes); `eda/data` record counts; spec sections extracted from the 8.1 U4 PDF; spec currency confirmed via odbplusplus.com.
**Re-verified after the base change:** every `Design.cpp`/`Design.h`/`service.proto` citation in this doc was re-grepped against `d7d1a5b`. The merge moved them by +11 lines (`CreatePinConnection` `:447`→`:458`, `BuildPlacementsFromEdaDataFile` `:510`→`:521`, the ordinal join `:548`→`:559`); `ComponentsFile.cpp`, `AttributeLookupTable.cpp`, `FeaturesFile.cpp`, `Component.cpp`, `DesignsController.cpp`, `design.proto` and swagger citations were unaffected (those files are not in the merged diff).
**Not done:** nothing was built or run. §4.2 byte figures are a cost model reconciled against PR #590's published measurement, not an independently observed size. §4.3 counts are derived from the data plus the clients' documented resolution order, not reproduced at runtime. The client symptom itself was not observed — it is the report's, and I confirmed the mechanism in their source.
