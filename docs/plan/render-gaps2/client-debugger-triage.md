# Client Debugger Triage Notes (Render Gaps / Symbol Mapping)

## Purpose
This is a quick checklist for debugging client-side rendering issues related to `sym_num` → `symbol_names[]` mapping and symbol/unit handling.

## Server assumptions / defaults
- REST: `http://localhost:8888`
- gRPC: `localhost:50051`
- Recommended flags for local debugging:
  - `--disable-authentication`
  - `--load-all`

> Windows note: PowerShell aliases `curl` to `Invoke-WebRequest`. Use `curl.exe` explicitly.

## Known repro layer (reported)
- Design: `designodb_rigidflex`
- Step: `cellular_flip-phone`
- Layer: `signal_2`

### Current expected server behavior (after fixes)
- `GET /filemodels/designodb_rigidflex/steps/cellular_flip-phone/diagnostics/symbol_units` shows for `signal_2`:
  - `features_units_raw: "INCH"`
  - `symbol_unit_counts: { imperial: 113, metric: 0, none: 0 }`
- `GetLayerSymbols(designodb_rigidflex, cellular_flip-phone, signal_2)` succeeds and returns:
  - `units: "inch"`
  - `units_to_mm: 25.4`
  - `sym_num_base: 0`

## Triage checklist (fastest path)

### 1) Confirm the server is running
```powershell
curl.exe http://localhost:8888/healthz
```
Expected: `ok`

List available designs:
```powershell
curl.exe http://localhost:8888/filemodels
```

### 2) Check symbol unit distribution (REST)
This answers: “Does the server think symbols are Metric/Imperial/None for this layer?”

```powershell
curl.exe "http://localhost:8888/filemodels/designodb_rigidflex/steps/cellular_flip-phone/diagnostics/symbol_units"
```

Things to look for (per layer):
- `has_mixed_symbol_unit_types == true` (Metric + Imperial mixture)
- `symbol_unit_counts.none > 0` (should now be 0 for layers with `UNITS=...`, because missing symbol units inherit file units)
- `has_mismatch_vs_features_units == true`

### 3) Verify `GetLayerSymbols` output (gRPC)
This answers: “What symbol list does the client need to index into?”

**PowerShell quoting tip:** `grpcurl -d @` needs escaping in PowerShell; use backtick: ``-d `@``.

```powershell
'{"design_name":"designodb_rigidflex","step_name":"cellular_flip-phone","layer_name":"signal_2"}' |
  grpcurl -plaintext -emit-defaults -format json -d `@ localhost:50051 Odb.Grpc.OdbDesignService/GetLayerSymbols
```

Things to verify:
- `sym_num_base == 0` (so `sym_num` indexes directly into `symbol_names[]`)
- `symbol_names.length` is large enough to cover `max(sym_num)` referenced by feature records
- Units look correct (`units`, `units_to_mm`)

### 4) Verify feature stream fields are interpreted correctly (gRPC)
This answers: “Are pads/lines/etc referencing symbols in the way the client expects?”

```powershell
'{"design_name":"designodb_rigidflex","step_name":"cellular_flip-phone","layer_name":"signal_2"}' |
  grpcurl -plaintext -format json -d `@ localhost:50051 Odb.Grpc.OdbDesignService/GetLayerFeaturesStream
```

Things to watch for client-side:
- `sym_num` is now *presence-significant*:
  - if protobuf `has_sym_num == false`, the client must **not** treat it as `0`
- **Pads** should reference symbols via `FeatureRecord.sym_num` (not via `apt_def_symbol_num`).
- Some indices in `symbol_names[]` may be placeholders (empty name) if indices are sparse; the client should handle that safely.

## Behavioral changes vs. older server builds (important)

### A) Missing symbol units now inherit file units
- Old: symbol unit missing ⇒ `UnitType=None` (often causing mixed `Imperial + None` per layer)
- New: symbol unit missing ⇒ inherits `UNITS`/`U` of that layer’s `features` file

### B) `sym_num` / `has_sym_num` is no longer meaningless
- Old: server serialized `sym_num` even when logically unset (clients couldn’t distinguish unset vs 0)
- New: `sym_num` defaults to `-1` internally and is only serialized when `>= 0`

### C) Pads are contract-aligned on `sym_num`
- Old: pads could effectively reference symbols via `apt_def_symbol_num` only
- New: pads set `sym_num = apt_def_symbol_num` during parse, and server enforces consistency
