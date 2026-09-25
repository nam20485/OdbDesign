# Component connectivity — client handoff

| | |
|---|---|
| For | The teams owning `odbdesign-3d-client-prototype` (C#/.NET, gRPC, renders from `Design.fileModel`) and `Odbdesign-info-client-india79-b` (C#/.NET, REST-first, moving to gRPC). Assumes no prior context. |
| Summary | Net/component connectivity becomes **server-owned derived data**. Clients stop performing their own joins across `fileModel` records. Today every `ComponentRecord.id` the server emits is `0` (never populated), and both clients' per-component net lists are keyed `(side, Id)` — so on a real design all components on a side collapse into one net bucket. |
| Status | Contract and decisions (D1–D5) are settled; **`Connectivity` is NOT built yet** — it ships in server Phase 2 (M2.1/M2.2). Prep work that is unblocked now is marked **[UNBLOCKED]**; work needing the shipped contract is **[BLOCKED — Phase 2]**. As of 2026-09-15 the §5a/§5b Id-first-leg deletions and §8 fixes are **implemented in both client working trees, uncommitted** — remaining sequence in [component-connectivity-client-followups.md](component-connectivity-client-followups.md). |
| Sources | [component-connectivity.md](component-connectivity.md) — findings (§2–§4), decisions (§5), contract (§6), conformance rules (§7), hazards (§8) · [component-connectivity-implementation-plan.md](component-connectivity-implementation-plan.md) — milestones · `AGENTS.md` §Sibling Repositories. **§9 lists what was re-verified for this doc vs inherited, and two citation nits found.** |

## 1. TL;DR

**Both teams.** A ~550-line `ComponentDetailBuilder.cs` exists as a fork-paste in each repo (562 and 531 lines; "89 differing lines after whitespace strip" — design doc §4.4, not re-measured here). It privately re-implements the netlist→component join and got the key wrong: it groups per-component nets by `(side, ComponentRecord.Id)`, and `Id` is always `0` because the server never populates it. That is the collapse bug you see. You must (a) delete the join code, (b) stop interpreting `-1`/`$NONE$` sentinels, (c) stop keying anything by `id`, and (d) rebuild a ~50-line lookup over the server's `Connectivity` message when Phase 2 ships. Doable **today**, before the contract exists: the two independent defects (§8), info's transport switch, and removal of the inert Id-first lookup leg. The full joiner deletion is **not** shippable yet — deleting it before `Connectivity` exists just breaks the inspector (plan M0.5); see the split note in §5a.

**3D team additionally:** your gRPC transport is ready; main work is the delete-and-rebuild, plus `src/OdbDesign3DClient.Core/Protos/` drifts from the server with nothing checking it (design doc §4.4) — refresh it when `Connectivity` lands in the proto, and expect a machine-check to arrive (M0.2).

**Info team additionally:** you have a hard prerequisite the 3D client doesn't — `Connectivity` ships **gRPC-only** (D5: REST is frozen, security fixes only), but your design reads go over REST (`src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs` + `Api/Dtos/` — both confirmed present). Transport switch first (its own PR), then consume. Your vendored `protoc/` is confirmed stale this session: `grep -c` of `GetStandardFonts|RequestLoadDesign|LoadStatus|include_normalized_lists` in `protoc/grpc/service.proto` returns **0** — all four features missing.

## 2. The identity model — three keys, three jobs (D1)

| Key | Guarantee | Use for | Never |
|---|---|---|---|
| `refDes` (`compName`) | Product-model-unique, spec-guaranteed, meaningful | **Primary external key** — UI, cross-refs, your local maps | — |
| `(side, index)` | Unique but **positional** — renumbers if file order changes | Server-internal join only | Persisting; client-side joins |
| `id` (ODB++ UID) | Product-model-wide unique, permanent across re-exports, **optional** (absent ≠ 0; proto3 `optional` → `HasId`) | Durable handle for revision diffing | **Join key** |

The client defect was collapsing all three into one field. Note what the server actually does — verified this session against `OdbDesignLib/ProductModel/Design.cpp` on this branch (HEAD `a40d937`, which contains the doc's base `d7d1a5b`):

* The **only** placement path that runs is `BuildPlacementsFromComponentsFiles` (called at `Design.cpp:135`). It resolves **component identity = refDes** (`CreatePinConnection`, `:458`; `m_componentsByName[refDes]`, `:462`) and **net reference = `ToeprintRecord.NetNumber`**, skipping `netNumber == (unsigned)-1` at `:448-449`. There is **no ordinal join on the live path**.
* The netlist-ordinal join (`SubnetRecord.ComponentNumber` → per-side ordinal) exists only inside `BuildPlacementsFromEdaDataFile` (`Design.cpp:521`, declared `Design.h:104`) which has **zero call sites** — confirmed by repo-wide grep: only the definition, the declaration, and doc references. **Client code that mirrors the ordinal join is reimplementing a dead path.** Your join agrees with the server only because refDes order happens to line up with record order.
* The server's own resolution already holds `(Component, Pin)` pairs per net (`m_nets[].pinConnections[]`); `Connectivity` is a serialization of that result, not a new join (design doc §6).

Also: IPC-2581-imported designs leave `m_pFileModel` null **by design** (noted in design doc "Related"), so any fact your client derives only from `fileModel` records is structurally unavailable for those designs. Another reason joins must move server-side.

## 3. The two conformance rules (§7, in spirit verbatim)

1. Clients **may** group, sort, and index published fields locally (a `refDes → detail` dictionary is fine).
2. Clients **may not** resolve a reference from one collection against another — no matching `componentNumber` / `toeprintNumber` / `netNumber` against separately-fetched records, and **no interpreting format sentinels**. Those are the server's joins and the server's conventions.

Concrete examples of what rule 2 forbids today:

* `TOP` `net_num = -1` is stored in an `unsigned int` (`ComponentsFile.h:151` `m_allowToepintNetNumbersOfNegative1 = true`, re-confirmed) so clients receive **4294967295** and must infer "unconnected". Under the contract that becomes `Connection.Kind.UNCONNECTED` — the server already skips those records but never told you it did.
* The magic net name **`$NONE$`**: the server's `BuildNoneNet`/`BreakSinglePinNets` are disabled (`Design.cpp:138-139`, re-confirmed), so unconnected pins ride a net named `$NONE$` today. Under the contract you branch on `Kind`, never on a name string.

Enforcement is structural: the contract **deletes** your duplicated joiner, and the M3.3 shared conformance fixture makes any re-introduced join fail a test.

## 4. What the server will publish — `Connectivity` (design doc §6)

**Not available yet — Phase 2 (M2.1/M2.2).** When it ships, it is **ALWAYS-ON over gRPC** (D4): no request flag is added, it rides the single cached default response (`GetDesignRequest` keeps only `include_normalized_lists`, `service.proto:51`, re-confirmed), and every consumer gets it without asking. Do **not** wait for it or gate your prep work on it; do not build anything that assumes a flag exists.

```proto
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

The pin layer answers both questions directly: *what pins a component has* (`packages` roster deduped by package + `components[].pins` per placement instance, `ComponentPin.number` resolved server-side so `toeprintNumber` never reaches you) and *through which pins it reaches a net* (`pins[].connection`, inverted in `netMembers`). Both directions are the same 2,811 edges on `sample_design` — no client inverts anything. It arrives as `design.proto` field `optional Connectivity connectivity = 12`, populated by both serialization flavours (D4); D5 nonetheless declares gRPC the supported consumption path (see §9 nit). Geometry, placement, and attributes **stay in `fileModel`** — only relationships move.

## 5. Per-team migration

### 5a. `odbdesign-3d-client-prototype` — `src/OdbDesign3DClient.Core/Services/Implementations/ComponentDetailBuilder.cs` (562 lines; re-opened this session)

**Verified this session; the lines are correct, but see the split note below before acting on them:**

| What | Lines |
|---|---|
| `(BoardSide Side, uint Id)` ByKey key + hit-test lookup by it | `:14`, `:44` (default dict `:19`) |
| `byId`/`byIndex`/`byIdAnySide`/`byIndexAnySide` construction + population calls | `:98-103` |
| `netsByComponent` keyed `(side, Id)` + the `(Side, Id)` key build + inversion bookkeeping | `:110`, `:144`, `:162-178` (call site `:132`) |
| `byKey[(side, comp.Id)]` write | `:238` |
| `ResolveComponent` — the whole method (signature `:438`; Id-first `:446`, ordinal fallback `:451`, side-blind `:456`) | ⚠️ plan cites `:440-460`; re-derive from `:438` (see §9) |
| `ResolvePinNumber` — the `toeprintNumber→pinNumber` join | `:477-484` (join at `:479-481`) |

⚠️ **Split `ResolveComponent` rather than deleting it whole.** Only its **Id-first branch is safe to remove today** — `id` is always 0, so that branch is inert (every probe is either empty or the whole-side bucket and falls through), making removal behaviour-preserving while permanently disarming the §6 landmine. The **ordinal fallback cannot go yet**: it is the only thing producing correct net→component results until `Connectivity` ships. So the table above is "delete the Id path now, delete the ordinal path in Phase 2," not "delete the method now."

**Keep:** package/BOM/attribute/height enrichment (geometry is yours, by rule), the net tab as a *view*, `NetsByName`.

**Rebuild [BLOCKED — Phase 2]:** `ComponentDetailIndex` as a lookup over `Connectivity.components` keyed `refDes` (StringComparer.Ordinal — see §8a); net tab from `netMembers`.

### 5b. `Odbdesign-info-client-india79-b` — `src/OdbDesign.ProductModel/ComponentDetailBuilder.cs` (531 lines; re-opened this session)

**Delete** (all lines verified this session) — same split as §5a: the Id-first probe (`:415`, return `:417`) and its side-blind twin inside the `:425` branch may go **today** (inert while every `id` is 0, so removal is behaviour-preserving and disarms §6 permanently); the ordinal fallback (`:420`) and the rest of the joiner stay until `Connectivity` ships. Lines: lookup construction `:62-67`; `netsByComponent` key `:74` (call site `:96`, key build `:108`, inversion `:126-141`); `byKey[(side, comp.Id)]` `:207`; `ResolveComponent` (`:407-439`; Id-first `:415`, ordinal `:420`, side-blind `:425` — ⚠️ plan's `:415-432` misses both ends); `ResolvePinNumber` `:446-454` (join `:448-450`).

**Prerequisite [UNBLOCKED, largest item]:** switch design reads from REST to gRPC — gRPC is the supported consumption path for `Connectivity` and REST is frozen (D5). Files confirmed present: `src/OdbDesignInfoClient.Services/Api/IOdbDesignRestApi.cs`, `Api/Dtos/`, `DesignService.cs`. You already carry gRPC codegen wired in `src/OdbDesign.ProductModel/OdbDesign.ProductModel.csproj` (`Protobuf` items from `:32`, confirmed) — the transport exists, the read path just doesn't use it. Sequence: transport switch (own PR) → refresh `protoc/` (verified missing all four current features, §1) → consume. Also hand-mirrored `Api/Dtos/` shapes are outside the M0.2 proto-sync check (implementation plan, M0.1 gap note) — don't grow them; delete as the transport switch makes them dead.

**Rebuild [BLOCKED — Phase 2]:** same shape as 5a. Both teams also delete any read of `attributeLookupTable["ID"]`: the server removes that key atomically with populating `id` (D3) — nothing was found reading it, and after M1.1/M1.3 it's gone.

### 5c. Both — conformance harness [UNBLOCKED now]

M3.3: assert your `refDes → (pin, net)` view equals the golden files at `OdbDesignTests/Fixtures/Connectivity/*.golden.json` (4 fixture designs; generated from raw ODB++ by `scripts/gen-connectivity-golden.py`; merged to `nam20485` via PR #595). Needs only the goldens, not the contract.

## 6. Ordering constraint — the `id` deploy landmine (design doc §4.3, plan Risks)

Populating `id` server-side (M1.1) **activates your Id-first lookup** (`:446`/`:415`), which starts matching product-model-wide **UIDs against per-side ordinals**. Measured on `Panel g7162` top: **88 of 847** ordinals collide with a real UID (~10% silent mis-resolution; other sampled designs: 0). That figure is from design doc §4.3 — a static range analysis, not reproduced at runtime there or here.

Therefore: **your delete of the Id-first resolution (§5a/§5b) must be deployed in every environment before a server build with `id` populated reaches that environment.** Be precise about what this is: a **per-environment DEPLOY constraint, not a merge-order one** — the server-side M1.1 may merge to any branch at any time, and your `develop` branch needn't wait; but rollout must be coordinated per environment, and the collision case is covered in the §5c fixtures. The full plan deletes this code rather than dodging the ordering (§8 hazard 2), but the ordering still governs if a client slips. Note that the safe-today Id-first-leg removal (§5a/§5b) front-runs this constraint entirely: once both clients have landed it, no server build's `id` population can wake the lookup anywhere.

## 7. Not decided / not built yet — do not code against these

* **`Connectivity` itself** (M2.1/M2.2, Phase 2). The message above is the agreed design, but no `.proto` exists to compile against until Phase 2 merges. Treat field numbers as provisional until then.
* **Payload budget can still resize it**: M2.4 gates merge at ≤150 KB on `sample_design` and ≤10% of fileModel on the 81,157-component legacy design (11,631 is its Top side only) — D4 was decided on a model (~tens of KB), not measurement; if measurement misses badly the shape gets revisited.
* **`$NONE$` semantics**: whether the contract revives `BuildNoneNet`/`BreakSinglePinNets` or merely labels their absence (`UNKNOWN` vs `UNCONNECTED`) is an open M2.2 decision. Branch on `Kind`, never on net names or `-1`, and your code survives either outcome.
* **Unresolved-pin reporting shape**: `Design.cpp:465` (`GetPackage()->GetPin(pinNumber)` null) fails the **entire design load** today; publishing rosters must not turn that into silent omission (§8 hazard 1). How the failure is surfaced to clients may be refined in M2.2.
* **`componentRecordsByName`** (proto field 10) is always-empty and recommended for deletion (M1.4) — never build on it.
* **`PinConnection` reference-ification** (D6/M4.1, Phase 4): today's normalized lists stay; design nothing around their removal yet.
* **M0.2 proto-sync CI** is unwritten — vendored-proto drift is currently caught by nobody.

## 8. Independent defects to fix while you're in there [UNBLOCKED]

**a) `refDes` is case-significant.** Spec p.152 (quoted in design doc §3): "Upper case and lower case characters are not equivalent" — yet all four name dictionaries fold case. Verified this session: 3D client `ComponentDetailBuilder.cs:20` (Empty `ByName`), `:33` (`NetsByName`), `:183` (`byName`); info client `:147` (`byName`) — all `StringComparer.OrdinalIgnoreCase`, silently merging `R1`/`r1` into last-write-wins on case-differing designs. When you rebuild keyed by `refDes`, use `StringComparer.Ordinal` on the component key. (Net *display*-name tolerance in the 3D `NetsByName` comment is a separate, defensible choice; the component key is not.)

**b) "No connections" is legitimate data, not a client bug.** Panelized/board steps of real panel jobs can carry **zero** toeprint connectivity — every `TOP` line has `net_num = -1` while `eda/data` still holds hundreds of NET records. Confirmed this session against the golden fixtures: `Panel-g7162-31800_odb` → 1,440 components, 3,078 pins, **0 connected** (517 nets present); `200-40628_Rev1_v7` → 129,698 pins, **0 connected**; control `sample_design` → 2,811/2,811 connected (matching design doc §4.2 exactly). Your inspector must render an all-`UNCONNECTED` board without flagging error, and your conformance tests must not treat empty net lists as a regression.

## 9. Provenance of this document's citations

**Re-opened and verified this session:** both `ComponentDetailBuilder.cs` files in full (line anchors by grep; `wc -l` = 562 / 531 — matching the plan); `Design.cpp` `:135`, `:138-139`, `:448-449`, `:458`, `:462`, `:465`, `:521` and `Design.h:104` (plus repo-wide grep proving zero call sites), on HEAD `a40d937` (contains the docs' base `d7d1a5b`); `componentsfile.proto:40` (`optional uint32 id = 9`); `service.proto:51`; `ComponentsFile.h:151`; info client paths (`Api/IOdbDesignRestApi.cs`, `Api/Dtos/`, `DesignService.cs`, `csproj:32+`) and its `protoc/grpc/service.proto` staleness (0/4 features); golden-file counts in §8b.

**Inherited from the plan docs (not re-derived here):** §4.1 UID availability table, §4.2 payload economics (2.1 MB / ~760 B/conn / 537 MB Turbot), the 88/847 collision count, "89 differing lines", spec page quotes (§3), the D1–D6 decision records, the `Connectivity` message text (§6 — copied verbatim).

**Discrepancies found (printed above, not silently corrected):** (1) plan M3.1 cites `ResolveComponent` at `:440-460` and M3.2's info deletion range as `:415-432`; actual method spans are `:438`→end and `:407-439` — the anchor lines both docs rely on (`:446`/`:451`, `:415`/`:420`) are correct, the ranges are approximate. (2) D4 says `connectivity` is populated in **both** `to_protobuf` flavours ("so the REST… flavours see it too") while D5 says "`Connectivity` ships over gRPC only". Read together: REST gets the bytes for free but is not a supported consumption path — that's the reading used here; flag if the server team means otherwise.
