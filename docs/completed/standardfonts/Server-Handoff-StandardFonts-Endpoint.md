# Server Handoff Spec: Standard Fonts gRPC Endpoint

> **To:** Server Development Agent  
> **From:** 3D Client Development Agent  
> **Date:** 2026-08-01  
> **Repo:** `nam20485/OdbDesign` branch `nam20485`  
> **Priority:** Required for client-side silkscreen/text rendering (Issue I3)

---

## 1. What I Need (One Sentence)

A new gRPC endpoint that returns the already-parsed `StandardFontsFile` protobuf message for a given design, so the client can render ODB++ text features as 3D stroke geometry.

---

## 2. Why This Is Simple

The server **already does 90% of the work**:

| Step | Status | Evidence |
|------|--------|----------|
| Parse `fonts/standard` from ODB++ archive | ✅ Done | `FileArchive::ParseStandardFontsFile()` called during `ParseFileModel()` |
| Store parsed font data in memory | ✅ Done | `FileArchive::m_standardFontsFile` member |
| Public accessor | ✅ Done | `FileArchive::GetStandardFontsFile() const` |
| Protobuf serialization | ✅ Done | `StandardFontsFile::to_protobuf()` |
| Protobuf schema | ✅ Done | `standardfontsfile.proto` — `StandardFontsFile`, `CharacterBlock`, `LineRecord` |
| Design cache has FileArchive | ✅ Done | `DesignCache::GetFileArchive(designName)` returns the archive |

**The only missing piece:** A gRPC RPC method in `OdbDesignServiceImpl` that calls `fileArchive->GetStandardFontsFile().to_protobuf()` and returns it.

---

## 3. Interface Contract

### 3.1 New RPC Method

Add to `service.proto` in the `OdbDesignService` service definition:

```protobuf
// Returns the standard font glyph data for a design.
// The client uses this to render ODB++ text features (silkscreen labels, refdes, etc.)
// as 3D stroke geometry.
rpc GetStandardFonts(GetStandardFontsRequest) returns (.Odb.Lib.Protobuf.StandardFontsFile);
```

### 3.2 Request Message

```protobuf
message GetStandardFontsRequest {
    string design_name = 1;
}
```

### 3.3 Response Message

Use the **existing** `StandardFontsFile` message from `standardfontsfile.proto` — no new message types needed:

```protobuf
// Already defined in standardfontsfile.proto — DO NOT MODIFY
message StandardFontsFile {
    message CharacterBlock {
        message LineRecord {
            optional double xStart = 1;
            optional double yStart = 2;
            optional double xEnd = 3;
            optional double yEnd = 4;
            optional Polarity polarity = 5;
            optional LineShape shape = 6;
            optional double width = 7;
        }
        optional string character = 1;
        repeated LineRecord m_lineRecords = 2;
    }
    optional double xSize = 1;
    optional double ySize = 2;
    optional double offset = 3;
    repeated CharacterBlock m_characterBlocks = 4;
}
```

### 3.4 Import Required

In `service.proto`, add:

```protobuf
import "standardfontsfile.proto";
```

---

## 4. Server Implementation

### 4.1 Method Implementation in `OdbDesignServiceImpl.cpp`

Follow the exact same pattern as `GetLayerSymbols` — lookup design from cache, return protobuf:

```cpp
grpc::Status OdbDesignServiceImpl::GetStandardFonts(
    grpc::ServerContext* context,
    const Odb::Grpc::GetStandardFontsRequest* request,
    Odb::Lib::Protobuf::StandardFontsFile* response)
{
    try
    {
        loginfo("[ConnTrace] GetStandardFonts start: design_name=\"" + request->design_name() + "\"");

        const auto fileArchive = m_designCache->GetFileArchive(request->design_name());
        if (fileArchive == nullptr)
        {
            return {grpc::StatusCode::NOT_FOUND, "Design not found: " + request->design_name()};
        }

        const auto& fontsFile = fileArchive->GetStandardFontsFile();
        *response = *fontsFile.to_protobuf();

        loginfo("[ConnTrace] GetStandardFonts done: design_name=\"" + request->design_name() +
            "\" characters=" + std::to_string(response->m_characterblocks_size()));

        return grpc::Status::OK;
    }
    catch (const std::exception& e)
    {
        std::string error = "Internal server error: " + std::string(e.what());
        return {grpc::StatusCode::INTERNAL, error};
    }
}
```

### 4.2 Header Declaration in `OdbDesignServiceImpl.h`

```cpp
grpc::Status GetStandardFonts(
    grpc::ServerContext* context,
    const Odb::Grpc::GetStandardFontsRequest* request,
    Odb::Lib::Protobuf::StandardFontsFile* response) override;
```

### 4.3 Include Required

In `OdbDesignServiceImpl.cpp`, add:

```cpp
#include <standardfontsfile.pb.h>
```

---

## 5. Error Handling Contract

| Condition | gRPC Status | Message |
|-----------|-------------|---------|
| Design not found in cache | `NOT_FOUND` | `"Design not found: <name>"` |
| `fonts/standard` was missing during parse (archive has no fonts) | `OK` with empty `StandardFontsFile` (0 character blocks) | N/A — not an error |
| Unexpected exception | `INTERNAL` | `"Internal server error: <details>"` |

**Important:** If the ODB++ archive has no `fonts/` directory, `ParseStandardFontsFile` throws `invalid_odb_error` during `ParseFileModel()`. This means the design would never load into the cache in the first place. So by the time `GetStandardFonts` is called, the font data is guaranteed to exist (possibly empty if the `fonts/standard` file exists but contains no characters).

If you want to be defensive: catch the case where `m_characterBlocks` is empty and still return `OK` — the client will simply render no text.

---

## 6. Data Model Reference (What the Client Receives)

### 6.1 `StandardFontsFile` (top-level)

| Field | Type | Description | Client Usage |
|-------|------|-------------|--------------|
| `xSize` | `double` | Font's native X cell size (font coordinate units) | Scale factor: `textFeature.xsize / font.xSize` |
| `ySize` | `double` | Font's native Y cell size (font coordinate units) | Scale factor: `textFeature.ysize / font.ySize` |
| `offset` | `double` | Baseline offset in font units | Vertical offset when placing glyphs |
| `m_characterBlocks` | `repeated CharacterBlock` | All character glyph definitions | Lookup by `character` string |

### 6.2 `CharacterBlock` (one per character)

| Field | Type | Description | Client Usage |
|-------|------|-------------|--------------|
| `character` | `string` | The character this block defines (e.g., `"A"`, `"1"`, `"+"`) | Key for lookup: `text[i]` → find matching block |
| `m_lineRecords` | `repeated LineRecord` | Stroke segments that draw this character | Render each as a 3D line segment |

### 6.3 `LineRecord` (one stroke segment)

| Field | Type | Description | Client Usage |
|-------|------|-------------|--------------|
| `xStart` | `double` | Stroke start X in font units | Scale by `xScale`, translate to text position |
| `yStart` | `double` | Stroke start Y in font units | Scale by `yScale`, translate to text position |
| `xEnd` | `double` | Stroke end X in font units | Scale by `xScale`, translate to text position |
| `yEnd` | `double` | Stroke end Y in font units | Scale by `yScale`, translate to text position |
| `polarity` | `Polarity` | `Positive` (0) or `Negative` (1) | Negative = erase/clear (client may skip or render differently) |
| `shape` | `LineShape` | `Square` (0) or `Round` (1) | Line cap style (client uses round caps for Round, flat for Square) |
| `width` | `double` | Stroke width in font units | Scale by `xScale` for 3D line thickness |

### 6.4 Enums (from `enums.proto`)

```protobuf
enum LineShape { Square = 0; Round = 1; }
enum Polarity { Positive = 0; Negative = 1; }
```

---

## 7. How the Client Uses This Data (For Your Context)

When the client receives a `Text` feature from `GetLayerFeaturesStream`:

```
Text feature fields used:
  .text          = "R70"           // The string to render
  .font          = "standard"      // Font name (always "standard" for ODB++ spec)
  .x             = 1234.5          // X position in layer units
  .y             = 678.9           // Y position in layer units
  .xsize         = 1.5             // Character height in layer units
  .ysize         = 1.0             // Character width in layer units
  .width_factor  = 0.1             // Stroke width multiplier
  .orient_def    = 0               // Orientation code (0-9)
  .orient_def_rotation = 0.0      // Custom rotation (only for orient_def 8,9)
```

The client then:

1. Looks up the `StandardFontsFile` (cached from `GetStandardFonts` call)
2. For each character in `.text`, finds the matching `CharacterBlock` by `.character`
3. For each `LineRecord` in the block, computes the 3D line segment:
   ```
   xScale = textFeature.xsize / font.xSize
   yScale = textFeature.ysize / font.ySize
   strokeThickness = lineRecord.width * xScale * textFeature.width_factor

   // Apply orientation rotation (orient_def → angle)
   // Translate to (textFeature.x, textFeature.y) in layer coordinates
   // Map to 3D: (pcbX, layerYLevel, pcbY)
   ```
4. Renders each segment as a 3D line using the existing `LineRenderer`

**The client needs `xSize` and `ySize` from the font to compute the scale factors.** Without them, glyph coordinates cannot be properly mapped to the text feature's physical size.

---

## 8. Client-Side Call Flow

```
1. User selects Design → client calls GetDesign or GetStandardFonts
2. Client caches StandardFontsFile in memory (design-level, not layer-level)
3. User selects Step → layers load
4. Client streams features via GetLayerFeaturesStream
5. When a Text feature arrives, client looks up cached font data
6. Client renders text as 3D stroke lines
```

The client will call `GetStandardFonts` **once per design selection**, not per layer. The response is small (typical standard font = ~95 characters × ~5 line records each = ~475 line records, a few KB of protobuf).

---

## 9. Testing Checklist

After implementation, verify:

- [ ] `GetStandardFonts("sample_design")` returns `OK` with `m_characterblocks_size() > 0`
- [ ] `GetStandardFonts("nonexistent")` returns `NOT_FOUND`
- [ ] Response contains `xSize > 0` and `ySize > 0`
- [ ] Response contains character blocks for at least: `A-Z`, `0-9`, `+`, `-`, `.`, `_`
- [ ] Each `CharacterBlock.character` is a single printable character
- [ ] Each `LineRecord` has `xStart`, `yStart`, `xEnd`, `yEnd` populated
- [ ] `LineRecord.width` is populated and > 0
- [ ] Response protobuf deserializes correctly on the client (C# `StandardFontsFile` class)

---

## 10. Open Questions

1. **Custom fonts:** Does any test design use fonts other than `"standard"`? If yes, the server may need to support multiple font files (e.g., `fonts/simplex`, `fonts/complex`). For now, the client assumes only `"standard"` exists. If custom fonts are needed, the response should include a `map<string, StandardFontsFile>` keyed by font name instead of a single `StandardFontsFile`.

2. **Font data size:** For the `sample_design` test archive, how many character blocks and line records does the standard font contain? This helps the client estimate memory and rendering cost.

3. **`offset` field:** What is the semantic meaning of `StandardFontsFile.offset`? Is it a Y-axis baseline offset in font units? The client needs to know whether to add or subtract this when positioning glyphs.

---

## 11. Summary of Changes Required

| File | Change |
|------|--------|
| `Protos/grpc/service.proto` | Add `import "standardfontsfile.proto";`, add `GetStandardFonts` RPC, add `GetStandardFontsRequest` message |
| `OdbDesignServer/Services/OdbDesignServiceImpl.h` | Add `GetStandardFonts` method declaration |
| `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` | Add `GetStandardFonts` method implementation (~20 lines) |

**Total estimated effort: 30 minutes.** No new parsing, no new data models, no new dependencies. The data is already parsed, stored, and serializable — it just needs a gRPC accessor.
