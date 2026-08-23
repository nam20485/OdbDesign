# Implementation Plan: `GetStandardFonts` gRPC Endpoint

> **Companion to:** `plan_docs/Server-Handoff-StandardFonts-Endpoint.md` (client-authored spec)
> **Branch:** `nam20485`
> **Date:** 2026-08-01

---

## 1. Objective

Expose the already-parsed `StandardFontsFile` protobuf message over gRPC so the 3D
client can render ODB++ text features (silkscreen labels, refdes, etc.) as stroke
geometry. The parsing, in-memory storage, and protobuf serialization already exist;
this work adds only the gRPC accessor, its request message, and tests.

---

## 2. Findings From Code Analysis

| Concern | Finding | Source |
|---------|---------|--------|
| Proto import resolution | `service.proto` is compiled with `IMPORT_DIRS ${GRPC_PROTO_DIR} ${PROTO_DIR}` where `PROTO_DIR = OdbDesignLib/protoc`. `import "standardfontsfile.proto"` resolves with no CMake change. | `OdbDesignServer/CMakeLists.txt` |
| Generated header visibility | `standardfontsfile.pb.h` is already visible to the server target via the same path that provides `design.pb.h` / `featuresfile.pb.h` (server links `OdbDesign` PUBLIC). | `OdbDesignServiceImpl.cpp` includes |
| Data accessor | `FileArchive::GetStandardFontsFile()` returns `const StandardFontsFile&`; `StandardFontsFile::to_protobuf()` returns `std::unique_ptr<Odb::Lib::Protobuf::StandardFontsFile>`. | `FileArchive.h`, `StandardFontsFile.h` |
| Font data guarantee | `fonts/standard` is mandatory per ODB++ spec; `ParseStandardFontsFile` / `StandardFontsFile::Parse` throw `invalid_odb_error` if missing, so a design that loaded into the cache always has font data (possibly 0 character blocks if the file is empty). | `FileArchive.cpp`, `StandardFontsFile.cpp` |
| Cache lookup | `DesignCache::GetFileArchive(designName)` returns `nullptr` for an unknown design — same pattern used by `GetLayerFeaturesStream` / `GetLayerSymbols`. | `DesignCache.h` |
| **Test harness gap** | The existing gRPC service tests (`GetLayerSymbolsTests.cpp`) are **not compiled** — they are absent from the test `add_executable`, and `OdbDesignServiceImpl.cpp` is commented out of the test target with a NOTE about "CMake coupling concerns". There is currently no compiled service-level test harness. | `OdbDesignTests/CMakeLists.txt` |

**Decision (approved by user):** re-enable `OdbDesignServiceImpl.cpp` in the existing
`OdbDesignTests` target and add a new `GetStandardFontsTests.cpp` mirroring the
`GetLayerSymbolsFixture` pattern. Both deliverable docs go in `plan_docs/`.

---

## 3. Changes

### 3.1 `OdbDesignServer/protoc/grpc/service.proto`
- Add `import "standardfontsfile.proto";`
- Add to `service OdbDesignService`:
  ```protobuf
  rpc GetStandardFonts(GetStandardFontsRequest) returns (.Odb.Lib.Protobuf.StandardFontsFile);
  ```
- Add request message (package `Odb.Grpc`):
  ```protobuf
  message GetStandardFontsRequest {
      string design_name = 1;
  }
  ```
- Response reuses the existing `Odb.Lib.Protobuf.StandardFontsFile` message — no new
  response type, no modification to `standardfontsfile.proto`.

### 3.2 `OdbDesignServer/Services/OdbDesignServiceImpl.h`
- Add the override declaration:
  ```cpp
  grpc::Status GetStandardFonts(grpc::ServerContext* context,
      const Odb::Grpc::GetStandardFontsRequest* request,
      Odb::Lib::Protobuf::StandardFontsFile* response) override;
  ```

### 3.3 `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`
- Add `#include <standardfontsfile.pb.h>`.
- Implement `GetStandardFonts` following the `GetDesign` / `GetLayerSymbols` pattern:
  - `[ConnTrace]` start log with `design_name`.
  - `m_designCache->GetFileArchive(request->design_name())`; if `nullptr` return
    `NOT_FOUND` with `"Design not found: <name>"`.
  - `*response = *fileArchive->GetStandardFontsFile().to_protobuf();`
  - `[ConnTrace]` done log including `m_characterblocks_size()`.
  - Return `grpc::Status::OK`.
  - `catch (const std::exception&)` → `INTERNAL` with `"Internal server error: <what>"`.

### 3.4 `OdbDesignTests/CMakeLists.txt`
- Re-enable `../OdbDesignServer/Services/OdbDesignServiceImpl.cpp` in the
  `OdbDesignTests` `add_executable` list.
- Add `GetStandardFontsTests.cpp` to the list.
- (No other CMake changes: the target already links gRPC, includes the codegen dir,
  compiles `service.pb.cc` / `service.grpc.pb.cc`, and depends on `OdbDesignServer`.)

### 3.5 `OdbDesignTests/GetStandardFontsTests.cpp` (new)
- Fixture `GetStandardFontsFixture : public FileArchiveLoadFixture` mirroring
  `GetLayerSymbolsFixture` (constructs `OdbDesignServiceImpl` from the cache).
- Test cases:
  - `ReturnsFontDataForSampleDesign` — `OK`, `m_characterblocks_size() > 0`,
    `xsize() > 0`, `ysize() > 0`; record block/line counts for the handoff doc.
  - `ReturnsNotFoundForMissingDesign` — `NOT_FOUND`.
  - `ContainsExpectedCharacterSet` — blocks cover `A–Z`, `0–9`, `+`, `-`, `.`, `_`;
    each `character` is a single printable char.
  - `LineRecordsArePopulated` — every line record has finite coordinates and
    `width > 0`.

---

## 4. Verification

1. `cmake --preset linux-debug`
2. `cmake --build --preset linux-debug`
3. `ctest --preset linux-debug -R GetStandardFonts -V` — new endpoint tests pass.
4. `ctest --preset linux-debug` — full suite passes (confirms re-enabling the service
   in the test target introduces no regressions).

---

## 5. Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| Re-enabling `OdbDesignServiceImpl.cpp` in the test target revives the historical "CMake coupling" build issue. | Full build + full test run. If intractable, fall back to a data-level test (`FileArchive → to_protobuf`) and flag before doing so. |
| `sample_design` font content differs from assumptions. | Tests assert only spec-guaranteed invariants; exact counts are captured at runtime for the handoff doc rather than hard-coded. |

---

## 6. Out of Scope

- No changes to parsing logic, data models, or `standardfontsfile.proto`.
- The disabled `GetLayerSymbolsTests.cpp` is left as-is (not re-registered).
- Client-side (C#) implementation.
