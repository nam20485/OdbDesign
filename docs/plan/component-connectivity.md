# Component identity and server-owned connectivity

| | |
|---|---|
| Status | **ACTIVE — design agreed, implementation planned.** Decisions D1–D6 below are **open** and block Phase 2/3. |
| Source | Client report: [component-id-issue.md](component-id-issue.md) (untracked drop-in from the 3D client agent, 2026-09-14). |
| Plan | [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) — file-level milestones. |
| Related | [ipc2581/spec-issues.md](ipc2581/spec-issues.md) (IPC imports leave `m_pFileModel` null) · [ipc2581/Client Migration Guide_ ODB++ to Unified API.md](ipc2581/Client%20Migration%20Guide_%20ODB%2B%2B%20to%20Unified%20API.md) · [server-issues.md](server-issues.md) (SI6 response cache, touched by M2.3) |
| Base | Verified against `nam20485` @ `cd9c0ce`. **Line numbers in this doc are that base.** `include_normalized_lists` is *not* in `nam20485` — it lives on `dev/getdesign-include-lists-flag` and shifts `Design.cpp`/`Design.h`/`service.proto`. See §Dependencies. |
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

**Correction to the report's framing.** The report concluded that `(side, index)` is "the server's real foreign key," because the netlist's `SubnetRecord.ComponentNumber` is a per-side ordinal and `Design::CreateNetConnections` joins on it (`Design.cpp:548`). That code path **never runs**. `Design::Build` calls exactly one placement routine:

```text
Design.cpp:135   BuildPlacementsFromComponentsFiles()      <-- the only one called
Design.cpp:510   BuildPlacementsFromEdaDataFile()          <-- defined, zero call sites (Design.h:94)
Design.cpp:138   //if (! BuildNoneNet()) return false;      <-- disabled
Design.cpp:139   //if (! BreakSinglePinNets()) return false; <-- disabled
```

The live path (`Design.cpp:432-445` → `CreatePinConnection`, `:447`) resolves **component identity = refDes** (`m_componentsByName[refDes]`, `:451`) and **net reference = `ToeprintRecord.NetNumber`** (`:277` on the wire). The netlist-ordinal join is dead code. So the server and the clients run *two different implementations of connectivity* that agree only because `refDes` and the ordinal happen to line up. That divergence — not the zero `id` — is the root cause.

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
| `200-40628_Rev1_v7/pcb` (2016) | top | 7,610 | **0** | — |
| `350-41017_rev1_odbjob_v7/stp` (2018) | top | 11,631 | **0** | — |
| ODB2kicad `odb-kitchen-sink` / `odb-output` | top | 3 / 2 | **0** | — |
| `Panel g7162` / `panel-layout` | top / bot | 0 / 0 | — | — |

UIDs are unique per file and **never overlap top↔bottom** in any design, exactly as the spec promises. Half the corpus carries none at all, so optionality is not theoretical.

### 4.2 Why the normalized lists are unaffordable

`sample_design`: 813 components, 644 nets, **2,811 pin connections**, **71 packages** (1,007 package pins, ~14/package).

`PinConnection` embeds a full `Component`, and `Component::to_protobuf` does `mutable_package()->CopyFrom(...)` + `mutable_part()->CopyFrom(...)` (`Component.cpp:75-76`). So 71 packages get re-serialized across 2,811 connections — ~40× redundancy, each carrying its whole pin roster.

Cost modelled at ~760 B/connection ≈ 2.1 MB wire, consistent with `dev/getdesign-include-lists-flag`'s measured **5.8 MB of the 12.2 MB design JSON** once JSON's ~2.8× string inflation is applied. The same facts as a reference-keyed index: **tens of KB**.

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

### D3 — Stop double-storing the UID (or mark it deprecated)

`attributeLookupTable["ID"]` mixes a literal `"ID"` key into a map whose other keys are numeric attribute indices. Publish `id` as the typed field; either drop `"ID"` from the map or keep dual-writing it and document it deprecated. **Open** — depends on whether anything already reads `"ID"`.

### D4 — `Connectivity` always-on or opt-in

At ~tens of KB (§4.2) it is plausible as a base-contract field, unlike the 5.8 MB lists. Opt-in is cheaper to land and keeps the response-cache fast path intact. **Open** — see plan M2.3.

### D5 — REST is out of scope; the surface itself needs a verdict

The info client moves to gRPC, so the contract ships over gRPC only. The empty stubs `designs_component_route_handler` (`DesignsController.cpp:311`) and `designs_net_route_handler` (`:343`) stay unimplemented. But `/designs/<name>/{components,nets,packages,parts}` remain live, swagger-published and deployed, with their only known consumer migrating off. **maintain / freeze / deprecate is an unmade decision** — deliberately not assumed here.

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

1. **NC is a magic number.** `TOP` `netNumber = -1` lands in an `unsigned int` (`m_allowToepintNetNumbersOfNegative1 = true`), so clients see **4294967295** and must infer "unconnected" → `Connection.Kind.UNCONNECTED`.
2. **`toeprintNumber → pinNumber`** is currently done in client code (3D `:479-481`, info `:448-453`) — that is a join → resolved server-side into `ComponentPin.number`.
3. **`$NONE$`** is not broken out (`BuildNoneNet`/`BreakSinglePinNets` disabled, `Design.cpp:138-139`), so NC pins hang off a magic net name → `Kind`, not a name comparison.

## 7. Conformance rules for clients

"Not trusted to join" needs a testable line, not a preference:

1. Clients **may** group, sort, and index published fields locally (a `refDes → detail` dictionary is fine).
2. Clients **may not** resolve a reference from one collection against another — no matching `componentNumber` / `toeprintNumber` / `netNumber` against separately-fetched records, no interpreting format sentinels like `-1` or `$NONE$`. Those are the server's joins and its conventions.

Enforcement is structural: the contract **deletes** the duplicated 550-line joiner from both clients rather than specifying it more precisely to each. Both then implement ~50 lines of lookup against `Connectivity`.

Geometry, placement, and attributes stay in `fileModel` — that architecture is unchanged and correct. Only relationships move.

## 8. Hazards to keep visible

* ⚠️ `CreatePinConnection` calls `GetPackage()->GetPin(pinNumber)` and returns false on null (`Design.cpp:454`), which fails the **entire design load**. Publishing pin rosters must not convert that hard failure into a silent omission — keep reporting which pin failed to resolve.
* ⚠️ §4.3: populate `id` only after both clients stop resolving `componentNumber` by `Id`. The plan deletes that resolution in Phase 3, so the landmine is removed rather than dodged — but the ordering must hold if a client slips.
* ⚠️ `refDes` case sensitivity is significant (spec p.152), and **both clients ignore it**. 3D client: `ComponentDetailIndex` default (`ComponentDetailBuilder.cs:20`), net index (`:33`), `byName` (`:183`). Info client: `byName` (`:147`). All four use `StringComparer.OrdinalIgnoreCase`, which silently merges `R1`/`r1` on case-differing designs. Fix while touching the keying.

## 9. What was and was not verified

**Verified statically:** `ComponentsFile.{h,cpp}`, `AttributeLookupTable.{h,cpp}`, `FeaturesFile.{h,cpp}`, `Design.{h,cpp}`, `Component.cpp`, `Net.cpp`, the protos, swagger, `DesignsController.{h,cpp}`; every `id` assignment in `OdbDesignLib`; both clients' `ComponentDetailBuilder.cs`, csproj proto items, and vendored proto trees (diffed against server).
**Verified against data:** 13 `components` files across 8 designs, including two `.Z`-compressed legacy files decompressed to `/tmp` (no repo writes); `eda/data` record counts; spec sections extracted from the 8.1 U4 PDF; spec currency confirmed via odbplusplus.com.
**Not done:** nothing was built or run. §4.2 byte figures are a cost model reconciled against a measurement from another branch, not an observed size. §4.3 counts are derived from the data plus the clients' documented resolution order, not reproduced at runtime. The client symptom itself was not observed — it is the report's, and I confirmed the mechanism in their source.
