# Server API Contracts Validation Report (render-gaps2)

This document records whether the **current server implementation** satisfies the requirements in `docs/plan/render-gaps2/server-api-contracts.md`.

Scope: static code inspection of server + shared model/serialization code (no runtime validation).

## Summary

| Contract item | Status | Evidence (file:line) | Notes |
|---|---:|---|---|
| S1: `GetLayerSymbols` gRPC exists and matches proto contract | Pass | `OdbDesignServer/protoc/grpc/service.proto:19-55` | RPC + request/response messages present. |
| S1: deterministic `sym_num` → symbol mapping | **Partial** | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:195-324` | Mapping algorithm is deterministic, but `FeatureRecord.sym_num` is not the symbol field used for Pads (see “Key finding” below), so the contract statement “Pads reference `sym_num`” is not met by current data model usage. |
| S1: explicit `sym_num_base` (no guessing) | Pass | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:228-230` | Server hard-codes `sym_num_base = 0`. |
| S1: units metadata returned alongside symbols | Pass | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:222-230` | Returns `units` and `units_to_mm` in the same response. |
| S1: “cacheable for duration of session” | Partial | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:195-324` | gRPC has no cache headers; results are stable as long as the cached design is stable, but there is no explicit caching guidance/TTL in the API. |
| S2: minimal units metadata (`units`, `units_to_mm`) | **Partial** | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:43-81,222-230` | Provided for supported unit strings; request fails (`FAILED_PRECONDITION`) if units are missing/unsupported. |
| S2: **critical** “feature geometry and symbol numeric params are in the same unit system” | **Fail** | `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:222-230` + `OdbDesignLib/FileModel/Design/SymbolName.cpp:68-76` | Server reports layer/feature units, but symbol names also carry an independent `UnitType` (Metric/Imperial) and the server does not validate/guarantee alignment or provide separate conversion factors. |
| S3: surface polygons present in feature stream (fields exist + serialized) | Pass | `OdbDesignLib/protoc/common.proto:9-45` + `OdbDesignLib/FileModel/Design/ContourPolygon.cpp:1-48` + `OdbDesignLib/FileModel/Design/FeaturesFile.cpp:929-932` | Protobuf schema and serialization include `contourPolygons` with required part fields. |
| S3: for `type == Surface`, `contourPolygons` contains **1..N** polygons | **Partial** | `OdbDesignLib/FileModel/Design/FeaturesFile.cpp:532-586` | Parser supports polygons between `S`/`SE` records, but does not enforce non-empty polygons before emitting a Surface feature. |
| S4 (optional): REST layer stack metadata (`stackIndex`, `depthMm`, `side`) | **Not implemented** | `OdbDesignServer/Controllers/FileModelController.cpp:1121-1138` + `OdbDesignLib/FileModel/Design/MatrixFile.h:47-106` | `/filemodels/{design}/matrix/matrix` returns `MatrixFile` as-is; model has no stack fields. |

## Detailed validation

### S1: `GetLayerSymbols` gRPC endpoint

#### Contract: proto shape

*Status: Pass*

`OdbDesignServer/protoc/grpc/service.proto` defines:

- `rpc GetLayerSymbols(GetLayerSymbolsRequest) returns (GetLayerSymbolsResponse);` (`:21`)
- `GetLayerSymbolsResponse` includes `sym_num_base`, `units`, `units_to_mm`, `symbol_names` (`:44-55`)

#### Deterministic mapping + explicit base

*Status: Partial*

Evidence:

- Server always sets `sym_num_base = 0` (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:228`).
- Server collects symbols from either `FeaturesFile.symbolNames` or `symbolNamesByName` and orders them deterministically:
  - Uses explicit `$n` indices when present (`SymbolName::Parse` sets `m_index` from `$<number>`; `OdbDesignLib/FileModel/Design/SymbolName.cpp:34-55`).
  - Fills missing indices deterministically, then pads with placeholders up to the maximum referenced index (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:236-324`).

However, the contract text states that **Pads reference a symbol by `FeatureRecord.sym_num`**, but the parser populates Pad symbol references into `apt_def_symbol_num` (not `sym_num`) (`OdbDesignLib/FileModel/Design/FeaturesFile.cpp:273-347`).

The server response currently sizes the returned symbol list using **both** `sym_num` and `apt_def_symbol_num` (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:240-247`), but the response comment and documented mapping are only for `FeatureRecord.sym_num`.

**Impact**: if the client uses `FeatureRecord.sym_num` for pads as the contract describes, pad geometry will resolve to the wrong symbol (often `0`), which can plausibly manifest as a “single big circle” layer.

#### Edge-case semantics

*Status: Fail (contract mismatch)*

Contract states: “If a feature has no `sym_num` (`HasSymNum == false`) the client uses a fallback pad geometry.”

Current implementation always serializes `sym_num` (and many other fields) unconditionally:

- `FeatureRecord::to_protobuf()` always calls `set_sym_num(sym_num)` (`OdbDesignLib/FileModel/Design/FeaturesFile.cpp:915`).

Given Pads don’t parse/populate `sym_num` (they parse `apt_def_symbol_num`), `sym_num` will typically be the value-initialized default (0). With `proto3 optional`, this makes `has_sym_num == true` on the wire, contradicting the contract’s intended “missing” semantics.

### S2: minimal units metadata

#### `units` + `units_to_mm` returned

*Status: Partial*

Evidence:

- Units are normalized from `featuresFile.GetUnits()` and returned (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:222-230`).
- Supported: inch/mm/mil/micron; unsupported or missing units cause `FAILED_PRECONDITION` (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:43-81,223-226`).

If contract requires the endpoint to always return units metadata (even when units are absent in source data), current code does **not** meet that: it fails the request.

#### Critical requirement: same unit system for features + symbol numeric parameters

*Status: Fail*

Evidence:

- `GetLayerSymbols` reports only **one** unit system derived from the features layer (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp:222-230`).
- Symbol definitions carry their own `UnitType` parsed from symbol records (`M`/`I`) (`OdbDesignLib/FileModel/Design/SymbolName.cpp:68-76`).

The server does not:

- verify that `SymbolName.UnitType` matches the returned `units`, nor
- provide separate conversion factors when they differ.

This violates the contract’s “critical requirement for this phase” as written.

### S3: Surface polygons present in the feature stream

#### Schema + serialization

*Status: Pass*

Evidence:

- `ContourPolygon` schema includes `xStart`, `yStart`, and `polygonParts` with required arc fields (`OdbDesignLib/protoc/common.proto:9-45`).
- `ContourPolygon::PolygonPart::to_protobuf()` always serializes `endX/endY/xCenter/yCenter/isClockwise/type` (`OdbDesignLib/FileModel/Design/ContourPolygon.cpp:23-34`).
- Feature streaming uses `FeatureRecord::to_protobuf()` (server) and that always includes `contourPolygons` (`OdbDesignLib/FileModel/Design/FeaturesFile.cpp:929-932`; used by server in `OdbDesignServer/Services/OdbDesignServiceImpl.cpp:119-167`).

#### “For Surface features, contourPolygons contains 1..N polygons”

*Status: Partial*

Evidence:

- Surface feature parsing begins on `S ...` and ends on `SE` (`OdbDesignLib/FileModel/Design/FeaturesFile.cpp:532-586`).
- Contour polygon records (`BEGIN/END`) and parts (`ARC/SEGMENT`) are parsed and attached to the current surface feature (`OdbDesignLib/FileModel/Design/FeaturesFile.cpp:589-747`).

However, there is no validation that at least one contour polygon was present before emitting the Surface feature.

### S4 (optional): physical layer stack metadata

*Status: Not implemented*

Evidence:

- `/filemodels/{design}/matrix/matrix` simply returns serialized `MatrixFile` (`OdbDesignServer/Controllers/FileModelController.cpp:1121-1138`).
- `MatrixFile::LayerRecord` contains `row`, `context`, `type`, `name`, `polarity`, `color`, etc., but **no** `stackIndex`, `depthMm`, or `side` fields (`OdbDesignLib/FileModel/Design/MatrixFile.h:47-106`).

## Notes related to the “big yellow circle” rendering symptom

Based on contract vs implementation mismatches found above, the most plausible server-side contributors are:

1. **Pad symbol reference field mismatch**: pad symbol references are parsed into `apt_def_symbol_num` (`FeaturesFile.cpp:273-347`), while the contract (and likely the client) expects `sym_num`.
2. **Presence semantics mismatch**: `sym_num` is always serialized (`FeaturesFile.cpp:915`), so the client cannot reliably detect “missing `sym_num`” via `has_sym_num`.
3. **Units mismatch risk**: symbol definitions can be Metric/Imperial independent of the layer units, but only one `units_to_mm` is returned (`OdbDesignServiceImpl.cpp:222-230` + `SymbolName.cpp:68-76`).
