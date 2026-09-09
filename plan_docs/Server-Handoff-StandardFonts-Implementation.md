# Server Handoff: `GetStandardFonts` gRPC Endpoint — Implementation Notes

> **To:** 3D Client Development Team
> **From:** Server Development
> **Date:** 2026-08-01
> **Repo:** `nam20485/OdbDesign`, branch `nam20485`
> **In response to:** `plan_docs/Server-Handoff-StandardFonts-Endpoint.md`
> **Status:** Implemented, tested, and validated (129/129 server tests passing).

---

## 1. Summary

The `GetStandardFonts` endpoint is implemented exactly as specified in your handoff
spec. It is a unary RPC on the existing `OdbDesignService` that returns the
already-parsed `StandardFontsFile` protobuf for a given design. No new data models,
no schema changes to `standardfontsfile.proto`, and no changes to the parsing pipeline
— the font data was already parsed, stored, and serializable; this adds the gRPC
accessor only.

---

## 2. RPC Contract

Added to `service OdbDesignService` in `OdbDesignServer/protoc/grpc/service.proto`:

```protobuf
// Returns the standard font glyph data for a design.
// The client uses this to render ODB++ text features (silkscreen labels, refdes, etc.)
// as 3D stroke geometry.
rpc GetStandardFonts(GetStandardFontsRequest) returns (.Odb.Lib.Protobuf.StandardFontsFile);
```

### Request

```protobuf
message GetStandardFontsRequest {
  string design_name = 1;   // same design_name used by GetDesign / GetLayerFeaturesStream
}
```

### Response

The **existing, unmodified** `Odb.Lib.Protobuf.StandardFontsFile` message from
`standardfontsfile.proto` (schema reproduced in your spec §3.3 / §6). Field numbers
and types are unchanged.

### Proto files your client stub must compile

`service.proto` transitively depends on the library protos. To generate a client stub
you need these on your proto include path (all in `OdbDesignLib/protoc/` except
`service.proto`):

| File | Why |
|------|-----|
| `OdbDesignServer/protoc/grpc/service.proto` | service + `GetStandardFontsRequest` |
| `standardfontsfile.proto` | response message |
| `enums.proto` | `Polarity`, `LineShape` used by `LineRecord` |
| `design.proto`, `featuresfile.proto`, `symbolname.proto` | other RPCs on the same service (required to compile `service.proto`) |

---

## 3. Error Handling Contract (as implemented)

| Condition | gRPC Status | Message |
|-----------|-------------|---------|
| Design not loaded / unknown `design_name` | `NOT_FOUND` | `"Design not found: <design_name>"` |
| Archive has no `fonts/` or no `fonts/standard` | *(cannot occur at call time — see note)* | — |
| Unexpected server exception | `INTERNAL` | `"Internal server error: <details>"` |
| Success (including an empty font) | `OK` | — |

**Note on missing fonts:** `fonts/standard` is mandatory per the ODB++ spec, and the
server throws during design load (`ParseFileModel`) if it is absent. Such a design
never enters the cache, so `GetStandardFonts` can only ever be called for a design
whose font data exists. A font file that parses but defines no glyphs returns `OK`
with `m_characterBlocks` empty (0 blocks) — render no text, not an error.

---

## 4. Data Model — Confirmed Field Usage

Your spec §6 is correct. Quick confirmation of the wire field names (proto) and the
PascalCase properties `protoc` generates for C#:

| Proto field | C# property | Type | Notes |
|-------------|-------------|------|-------|
| `xSize` | `XSize` | `double` | Font native X cell size (font units). Scale: `textFeature.xsize / XSize`. |
| `ySize` | `YSize` | `double` | Font native Y cell size (font units). Scale: `textFeature.ysize / YSize`. |
| `offset` | `Offset` | `double` | Baseline offset (font units). See Open Question 3. |
| `m_characterBlocks` | `MCharacterBlocks` | `repeated CharacterBlock` | Lookup by `character`. |
| `CharacterBlock.character` | `Character` | `string` | Single printable character (e.g. `"A"`, `"1"`, `"+"`). |
| `CharacterBlock.m_lineRecords` | `MLineRecords` | `repeated LineRecord` | Stroke segments for the glyph. |
| `LineRecord.xStart/yStart/xEnd/yEnd` | `XStart/YStart/XEnd/YEnd` | `double` | Segment endpoints (font units). |
| `LineRecord.polarity` | `Polarity` | `enum` | `Positive=0`, `Negative=1`. |
| `LineRecord.shape` | `Shape` | `enum` | `Square=0`, `Round=1`. |
| `LineRecord.width` | `Width` | `double` | Stroke width (font units); always `> 0`. |

All `LineRecord` coordinate/width fields are populated for every record (the parser
requires them), so you do not need to guard against missing sub-fields beyond the
proto3 `optional` presence semantics.

---

## 5. Recommended Client Call Flow

Unchanged from your spec §8 — confirmed this matches server behavior:

1. On design selection, call `GetStandardFonts(design_name)` **once**.
2. Cache the `StandardFontsFile` at design scope (not per layer).
3. Stream features via `GetLayerFeaturesStream` / `GetLayerFeaturesBatchStream`.
4. For each `Text` feature, look up glyphs by character and scale by
   `xsize / XSize` and `ysize / YSize`.

The response is small (see §7), so caching it for the whole design session is cheap.

---

## 6. Implementation Decisions

| Decision | Rationale |
|----------|-----------|
| Response reuses `StandardFontsFile` directly (no wrapper message). | Matches your spec; avoids an extra type. A design has exactly one standard font, so no map/key is needed. |
| `NOT_FOUND` keyed only on `design_name`. | The endpoint is design-scoped; there is no step/layer input. Mirrors `GetDesign`. |
| Empty font returns `OK`, not an error. | A parseable-but-empty font is valid; the client simply renders no text. |
| Server passes `offset` through verbatim. | The server does not interpret font semantics; it serializes the parsed value as-is. |
| Service-level unit tests re-enabled in the server test target. | Allowed direct testing of gRPC status codes (`OK` / `NOT_FOUND`) and payload invariants. |

---

## 7. Answers to Your Open Questions

### Q1 — Custom fonts: does any test design use fonts other than `"standard"`?

**No.** Verified two ways:

- **Server code:** only `fonts/standard` is ever parsed (`FileArchive::ParseStandardFontsFile`
  → `StandardFontsFile::Parse` opens `<design>/fonts/standard`). Any other font file in an
  archive (e.g. `fonts/simplex`) is ignored — there is no parser or storage for it.
- **Test archives:** both `sample_design.tgz` and `designodb_rigidflex.tgz` contain exactly
  one font file: `fonts/standard`. No other font files are present.

**Conclusion:** your assumption that only `"standard"` exists is correct for all current
test data, and the single-`StandardFontsFile` response is the right shape. If custom fonts
are ever required, it would be a server-side change (parse additional font files and switch
the response to `map<string, StandardFontsFile>`); we would version the RPC rather than
change this one.

### Q2 — Font data size for `sample_design`

Measured directly from the endpoint response:

| Metric | Value |
|--------|-------|
| `m_characterBlocks` | **94** |
| total `LineRecord`s (across all blocks) | **446** (≈ 4.7 strokes/glyph) |
| `xSize` | **0.302** |
| `ySize` | **0.302** |
| `offset` | **0** |

This is on the order of a few KB of protobuf — trivial to cache for the design session.
Your §8 estimate (~95 chars × ~5 records) was essentially exact.

### Q3 — Semantic meaning of `offset`

`offset` is the value of the `OFFSET` directive in the ODB++ `fonts/standard` file header
(alongside `XSIZE` / `YSIZE`), expressed in **font units**. Per the ODB++ standard-font
format it is a **vertical (Y-axis) baseline offset** — the displacement of the glyph
baseline relative to the character-cell origin.

Guidance for placement:

- Apply it on the Y axis, scaled the same way as glyph Y coordinates
  (`yScale = textFeature.ysize / YSize`), i.e. `yOffsetWorld = Offset * yScale`.
- Treat a positive value as a `+Y` shift of the baseline.
- For both current test designs `offset == 0`, so it has no visible effect there; still
  honor it so archives with a non-zero baseline render correctly.

Caveat: the server stores and returns this value verbatim and does not interpret it, so the
authoritative definition is the ODB++ spec's `fonts/standard` `OFFSET` directive. If you
obtain a reference design with a non-zero `offset`, use it to confirm the sign convention
against your renderer; the magnitude and axis (Y, font units) are as stated above.

---

## 8. Validation Evidence

- New endpoint tests (`OdbDesignTests/GetStandardFontsTests.cpp`), all passing:
  - `ReturnsFontDataForSampleDesign` — `OK`, `>0` blocks, `XSize>0`, `YSize>0`.
  - `ReturnsNotFoundForMissingDesign` — `NOT_FOUND`.
  - `ContainsExpectedCharacterSet` — glyphs for `A–Z`, `0–9`, `+ - . _`; each key a single printable char.
  - `LineRecordsArePopulated` — every record has finite coordinates and `Width>0`.
- Full server suite: **129/129 tests pass** (serial run).

> **Heads-up on parallel test runs:** the server's design-loading test fixtures share one
> `TEST_DATA` directory and are **not parallel-safe** — `ctest -j$(nproc)` produces
> intermittent failures (concurrent extract/delete races) unrelated to this change. Run the
> suite serially (`ctest --preset linux-debug`) for a clean result.

---

## 9. Files Changed (server side)

| File | Change |
|------|--------|
| `OdbDesignServer/protoc/grpc/service.proto` | `import "standardfontsfile.proto";`, `GetStandardFonts` RPC, `GetStandardFontsRequest` message |
| `OdbDesignServer/Services/OdbDesignServiceImpl.h` | `GetStandardFonts` override declaration |
| `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` | `GetStandardFonts` implementation |
| `OdbDesignTests/CMakeLists.txt` | re-enabled service impl in test target; registered new tests |
| `OdbDesignTests/GetStandardFontsTests.cpp` | new service-level tests |
