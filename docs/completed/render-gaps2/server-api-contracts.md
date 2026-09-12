# Server API Contracts for Rendering Gaps

This document defines the **server → client** contracts required to fix the rendering gaps in the 3D client.

## Context / constraints

- The client has **no access to ODB++ files**.
- The server is the **only source** of rendering data.
- The client currently consumes:
  - REST: layer list + MatrixFile metadata (colors/types/row)
  - gRPC: `GetLayerFeaturesStream` (streams `FeaturesFile.FeatureRecord`)

---

## S1 (Server): `GetLayerSymbols` gRPC endpoint (Option A)

### Why it’s needed

Pads reference a symbol by `FeatureRecord.sym_num`, but the feature stream does not provide the symbol string (for example `r100`, `rect50x100`, `s50`, `oval50x100`).
To render pads correctly, the client must map `sym_num` → symbol name.

### Requirements

- **Deterministic mapping** between `sym_num` values used in the feature stream and the returned symbol list.
- **No guessing**: the server must explicitly define the `sym_num` indexing base.
- **Units metadata** must be returned alongside symbols (see S2).
- Endpoint should be cacheable for the duration of a session.

### Contract

Add to `src/OdbDesign3DClient.Core/Protos/grpc/service.proto`:

```proto
rpc GetLayerSymbols(GetLayerSymbolsRequest) returns (GetLayerSymbolsResponse);

message GetLayerSymbolsRequest {
  string design_name = 1;
  string step_name = 2;
  string layer_name = 3;
}

message GetLayerSymbolsResponse {
  // Indexing contract for FeatureRecord.sym_num.
  // Required for this phase: 0
  int32 sym_num_base = 1;

  // Minimal units contract (S2)
  string units = 2;        // e.g. "mm", "micron", "mil", "inch"
  double units_to_mm = 3;  // multiply numeric values by this to get millimeters

  // Mapping: symbol_names[i] corresponds to sym_num == (i + sym_num_base)
  repeated .Odb.Lib.Protobuf.SymbolName symbol_names = 4;
}
```

### Semantics / edge cases

- If a layer has no symbols: return `symbol_names = []`, `sym_num_base = 0`, and still provide `units/units_to_mm`.
- If a feature has no `sym_num` (`HasSymNum == false`): client uses a fallback pad geometry.
- If `sym_num` is out of range: client falls back and logs a warning.

---

## S2 (Server): minimal units metadata

### Why it’s needed

Symbol strings encode numeric sizes that must be interpreted in the same unit system as the feature geometry.

### Requirements (minimum for this phase)

The server provides:

- `units` (string label)
- `units_to_mm` (double)

**Critical requirement for this phase:** the server guarantees that:

- All **feature geometry coordinates** (`x`, `y`, `xs`, `ys`, `xe`, `ye`, `xc`, `yc`, and `ContourPolygon` fields)
- And all **symbol string numeric parameters**

…are expressed in the **same** unit system described by `units` / `units_to_mm`.

If this cannot be guaranteed, the contract must be extended to return separate conversion factors (for example `features_units_to_mm` and `symbols_units_to_mm`).

---

## S3 (Server): Surface polygons present in the feature stream

### Why it’s needed

Surface rendering is not possible unless `FeatureRecord.contourPolygons` contains the polygon boundary definitions.

### Requirements

For any `FeatureRecord` with `type == Surface`:

- `contourPolygons` contains 1..N polygons.
- Each `ContourPolygon` includes:
  - `xStart`, `yStart` (preferred), and
  - `polygonParts[]` with `Segment` and/or `Arc` parts.
- For `Arc` parts the following fields are required:
  - `endX`, `endY`
  - `xCenter`, `yCenter`
  - `isClockwise`

Client behavior:

- arcs are approximated into line segments
- polygons are triangulated (including holes) using SharpEngine `Triangulator`

---

## S4 (Server, optional): physical layer stack metadata

### Why it’s needed

The client’s current ordering by MatrixFile `row` may not represent physical PCB order (Top/Inner/Bottom) and can introduce large visible vertical gaps.

### Requirements

Expose optional fields that allow stable physical ordering and Z offsets:

- `stackIndex` (int): 0 = bottommost, increasing upward
- `depthMm` (double): optional physical depth to use as the layer’s Y coordinate
- `side` (string): optional classification such as `Top`, `Bottom`, `Inner`

### Contract (REST)

Extend the existing response from:

`GET /filemodels/{design}/matrix/matrix`

Layer object may include:

```json
{
  "name": "Top",
  "row": 10,
  "type": "Signal",
  "color": {"red": 1, "green": 0, "blue": 0},

  "stackIndex": 3,
  "depthMm": 0.12,
  "side": "Top"
}
```

Client treats these fields as optional and falls back to existing behavior when absent.

---

## Out of scope for this client release

1. **True negative-polarity boolean subtraction across independent features**
   - Holes within a single Surface contour (`ContourPolygon.Type.Hole`) will be supported.
   - Global CSG subtraction (negative pads cutting arbitrary overlapping copper) is not included.

2. **Text feature rendering (`FeatureRecord.type == Text`)**
   - Requires font handling + text mesh strategy + correct transforms.

3. **Filled arcs / pie wedges**
   - Arcs are currently rendered as polylines; filled wedges require extra mesh generation.

4. **Compound feature support (if/when present server-side)**
   - Not part of this pass.

5. **Full transformation pipeline (step origin / viewport transforms / board-side mirroring)**
   - Only basic per-feature transforms will be applied where safe; full multi-stage composition is deferred.

6. **Physically accurate layer thickness / dielectric spacing**
   - Initial pass uses simplified spacing with optional `depthMm` support.

7. **Viewport clipping / culling**
   - Performance optimization deferred.

8. **Adaptive arc tessellation quality improvements**
   - Current fixed segment counts remain for now.
