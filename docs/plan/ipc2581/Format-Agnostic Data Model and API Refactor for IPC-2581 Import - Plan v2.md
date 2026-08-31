# Format-Agnostic Data Model & API Refactor for IPC-2581 Import — Plan v2

| | |
|---|---|
| **Status** | Draft — open decisions in §6 pending review |
| **Date** | 2026-08-30 |
| **Target branch** | feature branch off `nam20485` |
| **Repos in scope** | `nam20485/OdbDesign` (this repo). `odbdesign-3d-client-prototype` is follow-on work (§10) |
| **Supersedes** | `IPC-2581 Implementation Plan for OdbDesign (Server).md`, `spec-issues.md`, `Client Migration Guide_ ODB++ to Unified API.md` (kept as prior research; their binding decisions are carried into §3) |
| **Audience** | Coding agents executing the phases in §7; human reviewer for §6 decisions |

---

## 1. Executive Summary

OdbDesign today is an ODB++-only system at every layer: parsing, product model build, caching, REST, gRPC, and protobuf contracts all assume an ODB++ archive source. This plan refactors the library and server to be **design-source agnostic** and adds **IPC-2581 (DPMX) XML import** as the second source format.

The core architectural move: introduce a file-model abstraction seam (`IDesignFileModel`) that the existing `FileArchive` (ODB++) and a new `Ipc2581XmlFile` both implement. `Design::Build` accepts either. Everything above `Design` (nets, packages, pins, parts, components, pin connections — and new placement/height data) is format-neutral, and the API serves both formats from the same cache.

Key facts that shape this plan (all verified against the codebase on 2026-08-30):

1. **The product model is already format-neutral above the build path.** `Design`'s public getters (`GetNets/GetComponents/GetPackages/GetParts`) expose no ODB++ types. Only `Design`'s private `Build*` helpers and its `m_pFileModel` member are ODB++-coupled.
2. **The REST `/designs*` endpoints are already format-agnostic** — they consume only the product model. They will serve IPC-2581 designs with zero changes.
3. **The 3D client prototype renders exclusively from the ODB++ `FileArchive` half of the design** (see §2.2). Making the client render IPC-2581 data requires (a) this plan's product-model extensions (placement + height, Phase 4) and (b) a follow-on client migration (§10). This plan does not fake ODB++ structures to appease the current client (§4.5, decision D5 recommendation).
4. **Real IPC-2581 sample files exist** at `OdbDesignTestData/TEST_DATA/FILES/IPC2581/` with verified per-file statistics (see §5.2) that serve as test oracles.

---

## 2. Motivation

### 2.1 Current state: ODB++ at every layer

Evidence from the current codebase:

| Layer | ODB++ coupling |
|---|---|
| File model | `OdbDesignLib/FileModel/Design/` contains ~20 ODB++ file parsers (`FileArchive`, `StepDirectory`, `LayerDirectory`, `ComponentsFile`, `EdaDataFile`, `FeaturesFile`, `MatrixFile`, ...). The generic-sounding `FileModel/Design/` directory is in fact the ODB++ implementation. |
| Product model build | `Design::Build(std::shared_ptr<FileModel::Design::FileArchive>)` is the only build path. Every private `Build*` helper in `ProductModel/Design.cpp` reads ODB++ records directly (`GetStepsByName()`, `GetTopComponentsFile()`, `EdaDataFile::GetNetRecords()`, `ComponentRecord` toeprints, ...). |
| Design file-model member | `Design::m_pFileModel` is typed `std::shared_ptr<FileModel::Design::FileArchive>`; serialized into `design.proto` as `optional FileArchive fileModel = 3`. |
| Cache | `App/DesignCache` holds `FileArchive::StringMap m_fileArchivesByName`; `LoadFileArchive` filters candidate files through `ArchiveExtractor::IsArchiveTypeSupported` (archive extensions only); `LoadDesign` delegates through `GetFileArchive`. |
| REST | `FileModelController` exposes ~20 ODB++-specific routes (`/filemodels/<name>/steps/...`, `.../layers/<layer>/features`, `.../symbols`, `.../matrix/matrix`, `.../misc/info`, `.../fonts/standard`, ...). `DesignsController` (`/designs*`) is the only format-neutral surface. |
| gRPC | `GetLayerFeaturesStream`, `GetLayerFeaturesBatchStream`, `GetLayerSymbols`, `GetStandardFonts` all walk `FileArchive` structures (`OdbDesignServer/Services/OdbDesignServiceImpl.cpp`). `GetDesign` serializes the `Design` protobuf including the embedded `FileArchive`. |
| Protobuf | `design.proto` hardcodes `optional FileArchive fileModel = 3`; 25 proto files total in `OdbDesignLib/protoc/`, 15 of which encode the ODB++ file model. |

Consequences: there is no way to load a second design format without touching `Design`, `DesignCache`, the controllers, the gRPC service, and the proto contracts. Every consumer that accepts a `FileArchive` assumes ODB++ semantics.

### 2.2 3D client investigation (2026-08-30)

The .NET client at `odbdesign-3d-client-prototype` was investigated to determine what data a rendering client actually needs. Findings (details in the phase notes; file paths relative to that repo):

- **The client never reads the product model.** The only access into the gRPC `Design` message is `design.FileModel?.StepsByName` (`src/OdbDesign3DClient.Core/Services/Implementations/OdbDesignGrpcClient.cs`, 3 call sites). `nets`/`packages`/`components`/`parts` are unused.
- Rendering data flow:
  - Board substrate ← `GetDesign` → `fileModel.stepsByName[step].profilefile` (ODB++ step profile).
  - Component bodies ← `GetDesign` → `stepDir.edadatafile` (package outline records) **and** `layersByName["comp_+_top" | "comp_+_bot"].components` (hardcoded ODB++ layer names; `ComponentRecord.locationX/locationY/rotation/mirror` + `.comp_height` attribute lookup).
  - Copper/silk/mask/drill ← `GetLayerFeaturesStream`/`BatchStream` + `GetLayerSymbols` (client parses ODB++ symbol strings like `r100`, `rect50x100` locally in `SymbolParser.cs`).
  - Navigation (REST): `GET /designs`, `GET /filemodels/<d>/steps`, `GET /filemodels/<d>/steps/<s>/layers`, `GET /filemodels/<d>/matrix/matrix` (layer colors/stack rows; graceful fallback exists).
- Units conversion happens in four independent client sites, each keyed off ODB++ `units` strings.

**Implication:** an IPC-2581 design with an empty `fileModel` would load over `GetDesign` but render nothing in the current client. The minimum server-side enablement for eventual rendering is to lift placement (x/y/rotation/mirror) and height into the product model for **both** formats (Phase 4), which ODB++ already has in its file model (`ComponentsFile::ComponentRecord` carries `locationX/locationY/rotation/mirror`) but never lifts into `ProductModel::Component`. Geometry streaming for IPC (layer features) is deferred (§10).

### 2.3 Goals

1. A format-agnostic file-model seam so new source formats can be added without touching `DesignCache`, controllers, or service plumbing.
2. IPC-2581 (revision B and C documents; §5) import producing the same product model as ODB++: nets, packages (+pins), parts, components, pin connections — plus new placement/height data on `Component`.
3. Lazy loading preserved: an IPC file is parsed on first request, not at startup (parity with ODB++).
4. API behaves sensibly for both sources: neutral endpoints serve both; ODB++-specific endpoints return clear, well-defined errors or synthesized equivalents for IPC sources.
5. Full backward compatibility for existing ODB++ clients (REST JSON shapes, gRPC messages, protobuf wire format).
6. Test coverage anchored on the real IPC sample files.

### 2.4 Non-goals (this plan)

- IPC-2581 copper/layer **geometry streaming** (equivalent of `GetLayerFeaturesStream`) — §10.
- Client migration in `odbdesign-3d-client-prototype` — §10.
- IPC-2582 schema support — §10.
- Saving/exporting designs back to IPC-2581 XML beyond persisting the uploaded file as-is.
- Large renames of existing ODB++ namespaces/directories (§4.7 notes the optional follow-up).

---

## 3. Binding constraints (from prior decisions — do not violate)

These were recorded in `spec-issues.md` and subsequent review. Implementing agents MUST follow them:

1. **Mirror the FileArchive path; do not bypass it.** Do NOT construct a `ProductModel::Design` directly from raw XML. Create an `Ipc2581XmlFile` class that parses the IPC-2581 XML file (the equivalent of what `FileArchive` is for ODB++), and "overload" `Design` so it can be built from an `Ipc2581XmlFile` exactly the way it is built from a `FileArchive` (parse file model → build product model).
2. **No `friend` classes and no public `AddXxx()` mutators on `Design`.** All population happens inside `Design::Build(...)` reading the file model's public getters, mirroring today's ODB++ code path.
3. **No pre-loading.** ODB++ designs are loaded lazily on first request; IPC-2581 files must behave identically (parsed on first `GetDesign`/`GetFileModel`).
4. **`/filemodels/<name>` must load IPC-2581 files.** If the file stream matches an IPC-2581 document it is loaded, whether the source is ODB++ or IPC-2581. Detection is **content-based (stream sniffing), not extension-based** — extension is only a pre-filter for directory scans.
5. **Keep existing behavior for ODB++.** All current tests must pass unchanged in intent; no regressions in REST JSON shapes, gRPC behavior, or protobuf wire format for ODB++ designs.
6. **Add test coverage** for every new behavior (see §8).

Carried forward from the earlier plan doc (still valid):

- **XML parser:** pugixml, DOM mode. IPC-2581 files can reach 100MB+; pugixml DOM overhead is ~2–3x file size (the largest current sample, `beaglebone_black_revb6.xml`, is 43MB). Acceptable for this server. SAX (`xml_text_reader`) is the documented escape hatch if files exceed practical RAM, but is out of scope now.
- **Revision attribute is advisory.** Samples declaring `revision="C"` still use rev-B-style `<LogicalNet>`/`<PhyNet>` elements (§5.1). Parse by element presence, not by the declared revision.

---

## 4. Target architecture

### 4.1 Layered view

```
                        +----------------------------------------------+
                        |                   Clients                    |
                        | (REST /designs*, /filemodels*; gRPC service) |
                        +--------------------+-------------------------+
                                             |
   FORMAT-AGNOSTIC FROM HERE UP  ------------+------------------------------
                                             |
        +------------------+   +----------------------------------+
        | DesignsController|   | FileModelController (ODB++-specific|
        | (product model)  |   |  routes; synthesized metadata for  |
        |                  |   |  IPC where cheap & useful, §4.5)   |
        +--------+---------+   +-----------------+----------------+
                 |                               |
                 v                               v
        +------------------+          +-----------------------+
        | ProductModel::   |          | OdbDesignServiceImpl  |
        | Design           |          | (gRPC)                |
        |  nets/packages/  |          +-----------+-----------+
        |  parts/components|                      |
        |  (+placement P4) |                      |
        +--------+---------+                      |
                 |                                |
                 | Build() overloads              |
                 v                                v
        +---------------------------------------------------+
        |            App::DesignCache                       |
        |  name -> IDesignFileModel  (lazy, sniffed)        |
        |  name -> ProductModel::Design                     |
        +---------------------+-----------------------------+
                              |
   FORMAT-SPECIFIC BELOW -----+------------------------------
                              |
        +---------------------+----------------------+
        |                                            |
        v                                            v
+----------------------------+          +------------------------------+
| FileModel::Design::        |          | FileModel::Ipc2581::         |
| FileArchive  (ODB++ .tgz)  |          | Ipc2581XmlFile (IPC XML)     |
| implements IDesignFileModel|          | implements IDesignFileModel  |
+----------------------------+          +------------------------------+
```

### 4.2 File-model abstraction: `IDesignFileModel`

New header `OdbDesignLib/FileModel/IDesignFileModel.h`, namespace `Odb::Lib::FileModel`:

```cpp
class ODBDESIGN_EXPORT IDesignFileModel
{
public:
    enum class Kind
    {
        OdbFileArchive,     // ODB++ archive (tgz/zip/tar/gz/directory)
        Ipc2581XmlFile      // IPC-2581 XML document
    };

    virtual ~IDesignFileModel() = default;

    virtual Kind GetKind() const = 0;

    // Parse the source into this file model. Throws parse_error on
    // malformed content; returns false for structural failure paths,
    // mirroring FileArchive::ParseFileModel semantics.
    virtual bool ParseFileModel() = 0;

    virtual std::string GetFilename() const = 0;
    virtual std::string GetFilePath() const = 0;

    // Human-readable design/product name derived from the source
    // (ODB++: root dir name; IPC-2581: Ecad@name or file stem).
    virtual std::string GetDesignName() const = 0;

    typedef std::vector<std::shared_ptr<IDesignFileModel>> Vector;
    typedef std::map<std::string, std::shared_ptr<IDesignFileModel>> StringMap;

protected:
    IDesignFileModel() = default;
};
```

Notes:

- `FileArchive` gains `public IDesignFileModel` (it already has `ParseFileModel`, `GetFilename`, `GetFilePath`; add `GetKind()` and `GetDesignName()` mapping to `GetProductName()`).
- Deliberately small. Keep it minimal so both implementations conform without leaking format concepts. Format-specific getters stay on the concrete classes; consumers that need them downcast after a `GetKind()` check.
- Save/export (`ISaveable::SaveFileModel`) is NOT part of the interface in this plan (only the ODB++ POST `/filemodels/<name>` path uses it).

### 4.3 The `Design` seam

Changes to `OdbDesignLib/ProductModel/Design.h/.cpp`:

- `m_pFileModel` becomes `std::shared_ptr<FileModel::IDesignFileModel>`.
- New overload: `bool Build(std::shared_ptr<FileModel::Ipc2581::Ipc2581XmlFile> pFileModel);` — mirrors `Build(shared_ptr<FileArchive>)`: store the pointer, then run IPC-specific private builders (`BuildNetsFromIpc`, `BuildPackagesFromIpc`, `BuildPartsFromIpc`, `BuildComponentsFromIpc`, `BuildPinConnectionsFromIpc`; names illustrative). No friends; builders read only public getters of `Ipc2581XmlFile`.
- `GetFileModel()` return type changes to `std::shared_ptr<FileModel::IDesignFileModel>` (no external callers today besides `Design` itself; verified 2026-08-30).
- `ClipFileModel()` unchanged (`m_pFileModel = nullptr`).
- `to_protobuf()`: branch on `m_pFileModel->GetKind()`:
  - `OdbFileArchive` → `mutable_filemodel()` (existing field 3, unchanged);
  - `Ipc2581XmlFile` → `mutable_ipcfilemodel()` (new optional field, decision D1 §6).
- `from_protobuf()`: reconstruct the right concrete file model based on which field is present.
- Existing ODB++ builders stay byte-for-byte behaviorally identical (refactor only where they must read through the base pointer — they don't: the `Build(FileArchive)` overload keeps the concrete pointer).

### 4.4 `DesignCache`: polymorphic, lazy, content-sniffed

Changes to `OdbDesignLib/App/DesignCache.h/.cpp`:

- Storage: replace `FileArchive::StringMap m_fileArchivesByName` with `FileModel::IDesignFileModel::StringMap m_fileModelsByName`. Locking strategy (shared/unique `std::shared_mutex`) unchanged.
- New primary accessor: `std::shared_ptr<FileModel::IDesignFileModel> GetFileModel(const std::string& designName);` — same lazy-load + double-checked-insert pattern as today's `GetFileArchive`.
- `GetFileArchive(designName)` retained for all ODB++-specific consumers: implemented as `GetFileModel(name)` followed by a kind check / `std::dynamic_pointer_cast<FileArchive>`. Returns `nullptr` for IPC designs (callers keep mapping that to 404 or a format-specific error, §4.5).
- `LoadFileArchive` becomes `LoadFileModel` (private): scans `m_directory` for a regular file whose stem matches, then:
  1. **Content-sniff** the file to choose the parser (new helper, below).
  2. Instantiate `FileArchive` or `Ipc2581XmlFile` and call `ParseFileModel()`.
  3. Error semantics preserved: file found but unparseable → `throw std::runtime_error` (surfaces as HTTP 500 / gRPC INTERNAL); no matching file → `nullptr` (surfaces as 404 / NOT_FOUND).
- **Content sniffing helper** (new static utility, e.g. `FileModel::DetectDesignFileFormat(path)` returning an enum `{Odb, Ipc2581, Unknown}`):
  - `1F 8B` gzip magic → ODB archive
  - `50 4B 03 04` zip magic → ODB archive
  - `ustar` at byte offset 257 → tar → ODB archive
  - Otherwise: skip UTF-8 BOM/whitespace; if content begins with `<?xml` or `<`, read until the first root element start tag; if the root element name is `IPC-2581` → IPC; else → ODB fallback (existing archive path fails with the current error, unchanged).
  - Bounded read: first ~4KB only.
- Directory-scan pre-filters: `loadAllFileArchives`/`loadAllDesigns` currently gate on `ArchiveExtractor::IsArchiveTypeSupported`; extend the predicate to also accept `.xml` files (new DesignCache helper `IsDesignFileSupported(path)`). Note the existing unused `DESIGN_EXTENSIONS[]` constant in `DesignCache.h` — repurpose or remove it as part of this change.
- `getUnloadedDesignNames` (lists every regular file) continues to work; IPC `.xml` stems appear automatically.
- Same-stem collisions (`foo.tgz` and `foo.xml` both present): iterate entries in sorted filename order, first supported match wins, log a warning when duplicates are skipped. (Deterministic; today's behavior is directory-order-dependent.)
- `AddFileArchive`/`SaveFileArchive` keep working for the ODB++ POST path; generalize naming (`AddFileModel`) only if trivial — do not rework save semantics in this plan.
- `makeLoadedFileModelsResponse` (`App/RouteController.cpp`): rename internals as needed (`getLoadedFileArchiveNames` → models) while keeping the JSON response shape `{"filearchives":[{name,loaded,type}]}` unchanged; `type` gains the value `"Ipc2581XmlFile"` for IPC file models (new enum value in addition to existing `"Design"`/`"FileArchive"` strings). Clients only consume `name`/`loaded` (verified for the 3D client).

### 4.5 API layering after the refactor

| Surface | Today | After refactor |
|---|---|---|
| `GET /designs` | Lists files + loaded state | Unchanged; IPC designs appear automatically |
| `GET /designs/<n>`, `/components`, `/nets`, `/packages`, `/parts` | Product model JSON | Unchanged; serves IPC-built designs with zero edits |
| `POST /files/upload`, `/files/upload/<name>` | Stores any file | Unchanged; `.xml` uploads work (verified: no extension gate) |
| `GET /filemodels/<n>` | FileArchive JSON | Polymorphic by source kind (decision D2 §6) |
| `POST /filemodels/<n>` | Builds FileArchive from JSON | Unchanged (ODB++-only by definition); documented as such |
| `GET /filemodels/<n>/steps`, `/steps/<s>/layers` | ODB++ FileArchive | ODB++: unchanged. IPC: synthesized from `<Step name>` / `<Layer name>` elements (§7 Phase 5) so navigation works for both formats |
| `GET /filemodels/<n>/matrix/matrix` | ODB++ MatrixFile | ODB++: unchanged. IPC: synthesized matrix from `<Layer>` + `<Stackup>` (§7 Phase 5) — gives the client layer rows/types/side |
| Other `/filemodels/<n>/steps/...` detail routes (eda_data, features, attrlist, stephdr, netlists, symbols, misc/*, fonts/*) | ODB++ FileArchive | ODB++: unchanged. IPC: well-defined error (decision D4 §6) |
| gRPC `GetDesign` | Design proto incl. fileModel | Works for both (proto branches, §4.3). `has_filemodel()` guards in tracing code already tolerate absence |
| gRPC `GetLayerFeaturesStream` / `BatchStream` / `GetLayerSymbols` / `GetStandardFonts` | Walk FileArchive | ODB++: unchanged. IPC: well-defined non-crashing status (decision D4) with a message naming the format limitation |
| gRPC `HealthCheck` | Neutral | Unchanged |

Deliberate choice (decision D5, §6): the server does NOT synthesize ODB++-shaped `ComponentsFile`/`EdaDataFile`/features content for IPC designs just to keep the current 3D client rendering unchanged. That path couples the server to the client's ODB++ assumptions and contradicts goal 1. Instead Phase 4 lifts real placement/height data into the product model, and the client migration (§10) consumes it. Cheap, genuinely format-neutral metadata (step/layer names, matrix-equivalent) IS synthesized because IPC-2581 truly has that data.

### 4.6 XML parsing stack

- **Library:** pugixml via vcpkg. Add to root `vcpkg.json` `dependencies` and pin it in `overrides` at the exact `version#port-version` available at baseline `4f6d4ae8247b2dcae554555a135e52bb449dd524` (per project convention — every dependency is pinned; wrong/missing port-version forces full rebuilds). Determine the version with `git -C $VCPKG_ROOT cat-file blob 4f6d4ae8:ports/pugixml/vcpkg.json` or the registry versions db (`versions/p-/pugixml.json` at that commit).
- Link `pugixml::pugixml` PRIVATE to the `OdbDesign` target in `OdbDesignLib/CMakeLists.txt`.
- **Mode:** DOM (`pugi::xml_document::load_file`), parsed lazily inside `Ipc2581XmlFile::ParseFileModel()` — which itself runs lazily on first cache access.
- **Encoding:** pugixml handles UTF-8/UTF-16/ISO-8859-1 automatically; strings handed to the product model must pass through `Odb::Lib::Text::ToUtf8` where needed (Design protobuf serialization already uses `Utf8Sanitizer`).
- **Namespaces:** samples use the default namespace `xmlns="http://webstds.ipc.org/2581"` with unprefixed element names, so plain `child("Step")` traversal works in pugixml (it matches local names as written). The parser MUST NOT rely on namespace prefixes; if a prefixed document ever appears, element lookup by local name would need a helper — note as a known limitation with a defensive root-element check (root must be exactly `IPC-2581`).
- **Validation level:** well-formedness + root-element check + required-section presence handled gracefully (function-mode subsets, §5.2). No XSD validation.

### 4.7 Naming and code layout

- New IPC file model: `OdbDesignLib/FileModel/Ipc2581/Ipc2581XmlFile.{h,cpp}`, namespace `Odb::Lib::FileModel::Ipc2581`, class name `Ipc2581XmlFile` (per user direction).
- Existing ODB++ parsers remain in `OdbDesignLib/FileModel/Design/` under `Odb::Lib::FileModel::Design`. **No namespace/directory rename in this plan** — a rename (`FileModel/Design` → `FileModel/Odb`) would touch every include in controllers/tests for cosmetic gain. Document in-code that `FileModel/Design` is the ODB++ implementation. Optional follow-up (§10): mechanical rename once the seam is proven.
- `OdbDesignApp` (CLI dev harness with hardcoded sample paths) is unaffected; extending it to IPC is optional and out of scope.

---

## 5. IPC-2581 format notes (verified against samples, 2026-08-30)

All structure below was verified by direct inspection of the sample files in `OdbDesignTestData/TEST_DATA/FILES/IPC2581/` (see `README.md` there for provenance). Do not trust the older plan doc's element paths where they conflict with this section.

### 5.1 Document structure (observed)

```xml
<IPC-2581 revision="B|C" xmlns="http://webstds.ipc.org/2581">
  <Content roleRef="...">                          <!-- optional wrapper; may hold dictionaries -->
    <BomRef name="..."/>                           <!-- optional -->
    <DictionaryColor>...</DictionaryColor>         <!-- optional -->
    <DictionaryLineDesc units="...">...</DictionaryLineDesc>
    <DictionaryFillDesc units="...">...</DictionaryFillDesc>
    <DictionaryStandard units="...">               <!-- padstack/std-primitive defs -->
      <EntryStandard id="..."> <RectCenter .../> | <Circle .../> | ... </EntryStandard>
    </DictionaryStandard>
    <DictionaryUser>...</DictionaryUser>
  </Content>
  <HistoryRecord .../>                             <!-- optional -->
  <Bom name="...">                                 <!-- optional (function-mode dependent) -->
    <BomHeader assembly="..." revision="..."><StepRef name="..."/></BomHeader>
    <BomItem OEMDesignNumberRef="..." quantity="..." pinCount="..." category="...">
      <RefDes name="C12" packageRef="SMC0805" populate="true" layerRef="TOP"/>
      <Characteristics><Textual textualCharacteristicName="VALUE" textualCharacteristicValue="..."/></Characteristics>
    </BomItem>
  </Bom>
  <LogicalNet name="VIN">                          <!-- nets; rev-B style even in declared-C docs -->
    <PinRef componentRef="J1" pin="1"/>
  </LogicalNet>
  <PhyNet name="...">...</PhyNet>                  <!-- physical nets (may coexist) -->
  <Ecad name="Design">
    <CadHeader units="MM|INCH">...</CadHeader>
    <CadData>
      <Layer name="TOP" layerFunction="CONDUCTOR" side="TOP|BOTTOM|INTERNAL" polarity="..."/>
      <Stackup>
        <StackupGroup name="...">
          <StackupLayer layerOrGroupRef="TOP_COPPER" thickness="0.035"/>
        </StackupGroup>
      </Stackup>
      <Step name="LED_POWER_BOARD">
        <Profile><Polygon><PolyBegin x y/><PolyStepSegment x y/><PolyStepCurve .../></Polygon></Profile>
        <Package name="SOT-23-5" type="OTHER" pinOne="A1" height="0.150000">   <!-- attrs vary -->
          <Outline><Polygon>...</Polygon></Outline>
          <PickupPoint x="0" y="0"/>
          <!-- TWO observed pin encodings (both must be handled): -->
          <Pin number="1" x="-0.95" y="-1.3" padstackDefRef="SMD_RECT_0.9x0.7"/>           <!-- attr style -->
          <Pin number="A1" type="SURFACE" electricalType="ELECTRICAL">                      <!-- child style -->
            <Location x="-0.452760" y="0.452760"/>
            <StandardPrimitiveRef id="CIRCLE_8"/>
          </Pin>
          <SilkScreen>/<AssemblyDrawing>/<Courtyard> ... </...>                             <!-- optional graphics -->
        </Package>
        <Component refDes="U1" packageRef="BGA_25X25_1MM" layerRef="TOP" part="BGA_25X25_1MM"
                   mountType="SMT" standoff="0.0" height="0.150000">
          <Xform rotation="270.000" mirror="true"/>       <!-- attrs optional; default rot=0, mirror=false -->
          <Location x="11.279505" y="13.759815"/>
          <NonstandardAttribute name="VALUE" value="4.7UF" type="STRING"/>
        </Component>
        <LayerFeature layerRef="TOP_COPPER">              <!-- copper geometry (Phase: deferred §10) -->
          <Set net="VIN"><Line .../><Pad x y padstackDefRef="..."/><Polygon>...</Polygon></Set>
        </LayerFeature>
      </Step>
    </CadData>
  </Ecad>
</IPC-2581>
```

Parser-relevant quirks (from sample evidence):

- Root element is exactly `IPC-2581` with a `revision` attribute; declared revision is **advisory** (`testcase10_full.xml` declares `revision="C"` yet uses `<LogicalNet>`/`<PhyNet>`).
- Nets: handle `<LogicalNet>` and `<PhyNet>`; rev-C `<Net>` is defined by the standard but absent from all samples — support is optional/defensive. Net membership is `<PinRef componentRef pin>` (string names, not indices).
- Nets appear as **siblings of `<Ecad>`** in some samples (led_power_board) — do not assume nesting; search at document level under root.
- `<Component>` may live directly under `<Step>` (observed) — layer membership: `layerRef` names a `<Layer>` (whose `side` gives TOP/BOTTOM) or is literally `"TOP"`/`"BOTTOM"` (BOM `<RefDes>`).
- Component side derivation: `Component@layerRef` → `Layer@side`; if `layerRef` is literally `TOP`/`BOTTOM` (case-insensitive) map directly; otherwise `BoardSide::BsNone`.
- Units: `<CadHeader units="MM|INCH">` is authoritative for ECAD coordinates; `Dictionary*` elements carry their own `units` attribute (only matters for deferred geometry work).
- Function-mode subsets are real: `testcase10_bom.xml` (BOM-only: 0 components/nets in Ecad), `testcase10_assembly.xml` (components, no nets), `testcase10_fabrication.xml` (nets, no components). Missing sections must yield empty collections, not errors.
- Degenerate input: `ipc2581c_skeleton.xml` has empty attribute values and shifted data — parser must not crash on empty/missing numeric attributes (defaults: x/y=0, rotation=0, mirror=false).
- Multiple `<Step>` elements are possible; current ODB++ `Design` builds only the first step — IPC build keeps the same rule for parity (document as known limitation).

### 5.2 Sample files and expected test oracles

From `OdbDesignTestData/TEST_DATA/FILES/IPC2581/README.md` (verified with xmllint on 2026-08-30):

| File | Rev | Steps | Components | Nets (Logical/Phy) | Packages | Size | Test role |
|---|---|---|---|---|---|---|---|
| `led_power_board.xml` | C | 1 | 10 | 5 / 0 | 6 | 18 KB | Primary unit-test input (small, complete) |
| `testcase10_full.xml` | C | 1 | 56 | 514 / 514 | 5 | 21 MB | Full-function rev-C (Allegro); child-style pins; BOM present |
| `testcase10_assembly.xml` | C | 1 | 56 | 0 / 0 | 5 | 3.7 MB | Function mode: components without nets |
| `testcase10_fabrication.xml` | C | 1 | 0 | 0 / 514 | 5 | 20 MB | Function mode: nets without components |
| `testcase10_test.xml` | C | 1 | 56 | 0 / 514 | 5 | 4.8 MB | Function mode mix |
| `testcase10_bom.xml` | C | 1 | 0 | 0 / 0 | 0 | 12 KB | Degenerate BOM-only |
| `switch_board.xml` | B | 1 | 27 | 0 / 0 | 10 | 5.3 MB | Rev-B real board, no netlist |
| `beaglebone_black_revb6.xml` | B | 1 | 391 | 484 / 478 | 43 | 43 MB | Large-file memory/parse test |
| `ipc2581c_skeleton.xml` | C | 1 | 1 | 0 / 0 | 0 | 1.5 KB | Robustness: empty attrs / shifted data |

### 5.3 Data mapping: IPC-2581 → ProductModel

| ProductModel entity | Source (IPC-2581) | Notes |
|---|---|---|
| `Design.m_name` | file stem (cache key) | matches how ODB++ designs are keyed |
| `Design.m_productModel` | `Ecad@name` if present, else file stem | |
| `Net` | `<LogicalNet name>` + `<PhyNet name>` | index = sequential in document order; name collisions: first wins + warn (mirror ODB++ behavior) |
| `PinConnection` | `<LogicalNet><PinRef componentRef pin>` | resolve component by refDes, pin by name within that component's package; unresolvable refs → `logwarn` + skip (mirrors ODB++ `CreatePinConnection` out-of-range handling) |
| `Package` | `<Step><Package name>` | index = sequential; `height` attr captured in Phase 4 |
| `Pin` | `<Pin number>` (attr-style or `<Location>`-child-style) | index = sequential within package; name = `number` attr |
| `Part` | `Component@part` ∪ `BomItem@OEMDesignNumberRef` | dedupe by name; mirrors ODB++ part-name dedup in `BuildParts` |
| `Component` | `<Step><Component refDes packageRef layerRef part ...>` | `partName` = `part` attr (fallback: matching `BomItem` for the refDes); package lookup by `packageRef`; side via §5.1 rule |
| `Component` placement (Phase 4) | `<Location x y>` + `<Xform rotation mirror>` | convert to mm using `CadHeader@units` |
| `Component` height (Phase 4) | `Component@height` (IPC) / `.comp_height` attribute (ODB++) | IPC carries height directly — simpler than ODB++ |
| `$NONE$` net handling | N/A | ODB++-specific concept; skipped for IPC (it is disabled for ODB++ too — `BuildNoneNet`/`BreakSinglePinNets` are not called in the current ODB++ build path) |

Build order for the IPC path (mirrors the ODB++ build order in `Design::Build`): nets → packages (+pins) → parts → components → pin connections.

---

## 6. Open decisions (pending review)

Each decision lists options with trade-offs and a recommendation. **Status: PENDING — reviewer picks; the chosen option is then recorded here and treated as binding for the phases.**

### D1 — `design.proto` source representation

How the Design message carries its source file model once there are two types.

| Option | Description | Pros | Cons |
|---|---|---|---|
| **A. Additive optional field (recommended)** | Keep `optional FileArchive fileModel = 3;`; add `import "ipc2581xmlfile.proto";` and `optional Ipc2581XmlFile ipcFileModel = 12;` (next free number) | Zero wire/JSON change for existing ODB++ clients; `has_filemodel()` guards (e.g. gRPC component-height tracing) keep working untouched; trivially back-compatible across server versions | Two optional fields are not mutually exclusive at the proto level — server must enforce "at most one set" in code; slightly less self-documenting than oneof |
| B. `oneof source { FileArchive fileModel = 3; Ipc2581XmlFile ipcFileModel = 12; }` | Mutually exclusive by construction; clean contract | Moving an existing field into a `oneof` is wire-compatible for binary proto, but JSON serialization and every consumer touching `filemodel()` must be re-audited; risk of subtle client breaks for zero functional gain | Higher churn; proto style change mid-stream |

Recommendation: **A**. Enforcement "exactly one source" is a server-side invariant we already control.

### D2 — `GET /filemodels/<name>` response shape

| Option | Description | Pros | Cons |
|---|---|---|---|
| **A. Polymorphic payload (recommended)** | Return `FileArchive` JSON for ODB++ designs (byte-identical to today) and `Ipc2581XmlFile` JSON for IPC designs; response `Content-Type` unchanged | Existing ODB++ consumers see zero change; simplest implementation (`JsonCrowReturnable` already dispatches through `IProtoBuffable::to_json`) | No single static schema for the endpoint; clients must branch on the design's source type (discoverable from §D3 list metadata or payload shape) |
| B. Wrapper message | New `DesignFileModel { oneof { FileArchive fileArchive; Ipc2581XmlFile ipcFileModel; } }` returned for all designs | One stable schema going forward | **Changes the JSON shape for existing ODB++ designs** (payload nests under a new key) — violates constraint §3.5 unless versioned |

Recommendation: **A**, with the design source type exposed in the `/designs` listing metadata (see D3 note) so clients can branch before fetching.

### D3 — List metadata: expose source type in `/designs` listing

`makeLoadedFileModelsResponse` currently emits `{"filearchives":[{name, loaded, type}]}` with `type` ∈ {`Design`, `FileArchive`}.

| Option | Description |
|---|---|
| **A. Extend `type` values (recommended)** | Add `Ipc2581XmlFile` as a new possible `type` value; keep keys and shape identical |
| B. Add a sibling field | e.g. `{name, loaded, type, sourceFormat}` — more explicit but changes object shape |

Recommendation: **A** (the 3D client only reads `name`/`loaded`; additive values are safe).

### D4 — Error semantics for ODB++-only endpoints hit with an IPC design

Applies to: `/filemodels/<n>/steps/<s>/eda_data|features|attrlist|stephdr|netlists|profile`, `/filemodels/<n>/symbols*`, `/filemodels/<n>/misc/*`, `/filemodels/<n>/matrix/matrix` (unless synthesized, §4.5), `/filemodels/<n>/fonts/*`, and gRPC `GetLayerFeaturesStream`/`GetLayerFeaturesBatchStream`/`GetLayerSymbols`/`GetStandardFonts`.

| Option | REST | gRPC | Pros | Cons |
|---|---|---|---|---|
| **A. 404 + format message (recommended)** | `404 Not Found` — `"design \"<n>\" is IPC-2581; this endpoint is only available for ODB++ designs"` | `NOT_FOUND` with same message | Matches today's shape for missing sub-resources (step/layer-not-found are 404); clients already handle 404 | A pedant could argue the design *exists* (see B) |
| B. 400/405 | `400`/`405` — semantically "wrong format for this resource" | `INVALID_ARGUMENT`/`UNIMPLEMENTED` | More precise semantics | New status codes clients don't handle today; `UNIMPLEMENTED` is already used for the batch-stream feature flag and would be ambiguous |
| C. 501 | `501 Not Implemented` | `UNIMPLEMENTED` | Clear "server can't do this for this source" | Uncommon; poor client support |

Recommendation: **A** for REST + `NOT_FOUND` for gRPC, each with a message naming the actual source format. (Matrix/steps/layers synthesis per §4.5 removes the most client-visible cases anyway.)

### D5 — Synthesize ODB++-shaped data for IPC designs to keep the current client rendering?

| Option | Description |
|---|---|
| A. Yes — translate IPC into ODB++-shaped `ComponentsFile`/`EdaDataFile`/features server-side | Current 3D client renders IPC boards with zero client changes |
| **B. No — stay format-agnostic (recommended)** | Server exposes real IPC data through the product model (+ Phase 4 placement/height, + §4.5 navigation metadata); client migrates to the unified product-model API (§10) |

Trade-offs: A couples the server permanently to ODB++ shapes as an interchange format, forces fake symbols/indices/attribute tables, and re-breaks when geometry enters scope. B costs a client workstream but keeps the server contract honest and matches goals 1–2. Recommendation: **B** (this is already assumed by §4.5; flagged here explicitly for sign-off).

---

## 7. Phased implementation plan

Phases are ordered by dependency. Each phase ends in a buildable, test-green state and a committable checkpoint. **Phases 1 and 4 are pure refactors/extensions with no behavior change for ODB++ and can be reviewed independently.** Where a phase depends on a §6 decision, the recommended option is assumed until the reviewer records otherwise.

Environment notes (from `AGENTS.md` + local conventions):

- Configure/build: `cmake --preset linux-dynamic-release` then `cmake --build --preset linux-dynamic-release` (use `linux-debug` for debug work; the dynamic presets avoid the dual-protobuf-pool SIGABRT documented for static presets).
- Tests: `ctest --preset linux-dynamic-release -j$(nproc)`; single test: `ctest --preset linux-dynamic-release -R <Name> -V` or run `./out/build/linux-dynamic-release/OdbDesignTests/OdbDesignTests --gtest_filter=...`.
- Test data env (already in `~/.bashrc` on this machine): `ODB_TEST_DATA_DIR=/home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA`, `ODB_TEST_ENVIRONMENT_VARIABLE=ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS`.
- If vcpkg manifest install is blocked in an agent session and deps are already installed, configure with `-DVCPKG_MANIFEST_INSTALL=OFF` (restore afterwards).

### Phase 0 — Dependency & schema validation (no product code)

**Purpose:** de-risk pugixml integration and lock the schema understanding before writing parsers.

Tasks:

1. Determine the pugixml version at baseline `4f6d4ae8247b2dcae554555a135e52bb449dd524` (registry versions db at `$VCPKG_ROOT`). Add to root `vcpkg.json`:
   - `dependencies`: `"pugixml"`
   - `overrides`: `{ "name": "pugixml", "version": "<exact version#port-version>" }`
2. Configure + build `linux-dynamic-release` to force the vcpkg install; verify `pugixml::pugixml` is importable (`find_package(pugixml CONFIG REQUIRED)` smoke in the next phase's CMake change).
3. Read the 9 sample files' `README.md` + spot-check structures against §5.1; if anything conflicts, update §5 of this doc before proceeding.

**Files:** `vcpkg.json`.
**Acceptance:** configure succeeds and installs pugixml without rebuilding other deps (if the whole chain rebuilds, the override port-version is wrong — fix before proceeding); existing build/tests unaffected.

### Phase 1 — Format-agnostic foundation (refactor, zero behavior change)

**Purpose:** introduce the seam and polymorphic cache; every existing test must pass unchanged.

Files:

- NEW `OdbDesignLib/FileModel/IDesignFileModel.h` (interface per §4.2; header-only is fine, follow `ISaveable.h` style)
- MOD `OdbDesignLib/FileModel/Design/FileArchive.h/.cpp` (inherit `IDesignFileModel`; implement `GetKind()` → `Kind::OdbFileArchive`, `GetDesignName()`)
- MOD `OdbDesignLib/ProductModel/Design.h/.cpp` (`m_pFileModel` base type; `GetFileModel()` base return; `to_protobuf`/`from_protobuf` branch with IPC branch stubbed/unreachable until Phase 3)
- MOD `OdbDesignLib/App/DesignCache.h/.cpp` (§4.4: `m_fileModelsByName`, `GetFileModel`, `GetFileArchive` via downcast, `LoadFileModel`, sniffing call-site, `IsDesignFileSupported`, sorted-iteration collision rule, repurpose `DESIGN_EXTENSIONS[]`)
- NEW sniffing utility, e.g. `OdbDesignLib/FileModel/DesignFormatSniffer.{h,cpp}` (or statics on a new `FileModel/DesignFormat.{h,cpp}`) implementing §4.4 magic-byte + root-tag detection; add to `OdbDesignLib/CMakeLists.txt`
- MOD `OdbDesignLib/App/RouteController.cpp` (`makeLoadedFileModelsResponse` through model list; JSON shape unchanged; `type` value extension prepared)
- MOD `OdbDesignLib/CMakeLists.txt` (new sources)

Implementation notes:

- Keep `GetFileArchive`'s log messages and error semantics identical (DesignCacheLoadTests encode them, including the throw-vs-nullptr contract).
- `DesignCacheThreadSafetyTests` exercises concurrent `GetDesign`/`GetFileArchive` — keep the double-checked locking pattern exactly.
- The sniffing helper reads via `std::ifstream` binary, bounded 4KB; unit-testable without archives.

**Tests (new):**
- `DesignFormatSnifferTests.cpp`: gzip/zip/tar magics → Odb; XML with `<IPC-2581 ...>` root → Ipc2581; XML with other root → Unknown; BOM + whitespace; empty file; short files (< 257 bytes for the tar check).
- Existing suites unchanged: `DesignCacheLoadTests`, `DesignCacheThreadSafetyTests`, `FileArchiveLoadTests`, `IntegrationTests`, gRPC suites.

**Acceptance:** full `ctest` green; `GET /designs` and `GET /filemodels/<odb-design>/...` smoke responses byte-identical in shape to pre-refactor (spot-check with curl against a smoke server, §8.3).

### Phase 2 — `Ipc2581XmlFile` (IPC file model + parser)

**Purpose:** parse IPC-2581 XML into a queryable file model. No `Design` integration yet.

Files:

- NEW `OdbDesignLib/FileModel/Ipc2581/Ipc2581XmlFile.{h,cpp}` — namespace `Odb::Lib::FileModel::Ipc2581`; implements `IDesignFileModel` (`Kind::Ipc2581XmlFile`) and `IProtoBuffable<Odb::Lib::Protobuf::Ipc2581XmlFile>` (see proto below)
- NEW `OdbDesignLib/protoc/ipc2581xmlfile.proto` — file-model message; mirror the level of detail `filearchive.proto` provides but only for data the IPC model retains. Recommended initial content: `revision`, `units`, `productName/designName`, `fileName`, `repeated IpcLayer layers` (name/layerFunction/side/polarity), `repeated IpcStep steps` (name only), `repeated IpcBomItem bomItems` (oemDesignNumberRef, quantity, pinCount, refDes list), `repeated IpcPackageSummary` (name, pinCount, height) and `repeated IpcComponentSummary` (refDes, packageRef, layerRef, part, height, xform fields). Keep it a *summary* — the full DOM is not serialized.
- MOD `OdbDesignLib/CMakeLists.txt` (sources + `find_package(pugixml CONFIG REQUIRED)` + `target_link_libraries(OdbDesign PRIVATE pugixml::pugixml)`; the proto is picked up automatically by the existing `file(GLOB PROTO_FILES ...)` — re-configure required)
- MOD root `vcpkg.json` already done in Phase 0

Class design:

- `Ipc2581XmlFile(const std::string& path)` ctor mirroring `FileArchive`'s; `ParseFileModel()` loads the DOM (pugixml), validates root element (`IPC-2581`), then walks once to populate record structs (§5.3 sources) cached in members. Throws `Odb::Lib::FileModel::parse_error` on malformed XML / wrong root (reuse existing `FileModel/parse_error.h` infrastructure).
- Public getters consumed later by `Design::Build` and by controllers: `GetRevision()`, `GetUnits()` (normalized enum + raw string), `GetDesignName()`, `GetStepNames()`, `GetLayers()` (records), `GetStackupLayers()`, `GetBomItems()`, `GetPackages()` (with pins), `GetComponents()` (records with xform/location), `GetNets()` (logical + physical, each with pin refs).
- Record structs mirror the value-type style of `EdaDataFile::NetRecord`/`ComponentsFile::ComponentRecord` (plain structs, `IProtoBuffable` where exposed via proto).
- Numeric parsing: `std::strtod`-based tolerant helpers (empty/missing → default per §5.1). Units conversion helper `unitsToMm()` (MM=1.0, INCH=25.4; unknown → 1.0 + warn) — used in Phase 4.
- Memory: hold `pugi::xml_document` as a member only if needed after parse; since records capture everything the API needs, **release the DOM at end of `ParseFileModel()`** (halves steady-state RAM for 43MB files). Document this.

**Tests (new):** `Ipc2581XmlFileTests.cpp` using `TestDataFixture` (`getTestDataFilesDir() / "IPC2581" / <file>`):
- `led_power_board.xml`: revision C, units MM, 1 step `LED_POWER_BOARD`, 11 layers, 6 packages, pin counts (SOT-23-5 → 5 pins), 10 components, 5 logical nets, GND net has 6 pin refs; component J1 → package `CONN_1x02_P2.54`, side Top; R2 side Bottom (layerRef BOTTOM_COPPER → Layer side BOTTOM).
- `testcase10_full.xml`: 56 components, 514 logical + 514 physical nets, 5 packages, BOM items present (BGA_25X25_1MM qty 1), child-style pin locations parsed (Pin A1 of BGA has location).
- Function modes: `testcase10_assembly.xml` (56 comps / 0 nets), `testcase10_fabrication.xml` (0 comps / 514 phy nets), `testcase10_bom.xml` (0/0/0 — parses OK).
- `switch_board.xml` (rev B, 27 comps, 0 nets).
- `ipc2581c_skeleton.xml`: parses without crash, empty attrs default.
- Malformed: non-XML garbage `.xml` → throws `parse_error`; XML with different root element → throws or returns false (per sniffer → this path only reached if root matched, so garbage-file case is covered by sniffer/LoadFileModel tests instead).
- Protobuf round-trip: `to_json`/`from_json` (via `IProtoBuffable`) for a parsed `led_power_board` model.
- Optional/long-running: `beaglebone_black_revb6.xml` parse + counts (391 comps / 484 logical nets / 43 packages); keep enabled — runtime is seconds — but isolate in its own test so it can be filtered.

**Acceptance:** all new tests green; existing tests green; `OdbDesign` lib builds without exporting pugixml headers publicly (PRIVATE link).

### Phase 3 — `Design::Build(Ipc2581XmlFile)` (product model from IPC)

**Purpose:** IPC designs become first-class `Design` objects.

Files:

- MOD `OdbDesignLib/ProductModel/Design.h/.cpp` — new overload + private IPC builders (§4.3); implement §5.3 mapping including unresolvable-PinRef warn-and-skip; first-step-only rule; net name dedup rule.
- MOD `OdbDesignLib/App/DesignCache.cpp` — `LoadDesign` path: when `GetFileModel` returns an IPC model, build via the new overload (branch on `GetKind()`).
- MOD `OdbDesignLib/protoc/design.proto` — decision D1: add `import "ipc2581xmlfile.proto";` + `optional Ipc2581XmlFile ipcFileModel = 12;` (assumes D1=A).
- MOD `Design::to_protobuf/from_protobuf` — implement the IPC branches (§4.3).

**Tests (new):** `Ipc2581DesignBuildTests.cpp`
- Build via `DesignCache::GetDesign("led_power_board")` with a scratch designs dir containing a copy of the xml (mirror `FileArchiveLoadFixture`'s isolated-copy pattern in a new `Ipc2581LoadFixture`): nets=5, packages=6, components=10, parts≥1, `VIN` net has 3 pin connections, `GND` has 6; `GetComponent("R2")` side Bottom; `GetComponent("U1")` package name `SOT-23-5`.
- `testcase10_assembly`: 56 components, 0 nets, design builds successfully (no pin connections).
- `testcase10_bom`: builds with 0 components/0 nets (does NOT fail).
- `GetFileModel()` returns kind `Ipc2581XmlFile`; `GetFileArchive()` returns nullptr for it; `GetFileArchive("sample_design")` still works.
- Protobuf round-trip: `Design` built from IPC → `to_protobuf` → fresh `Design::from_protobuf` → counts match; `has_filemodel()` false, `has_ipcfilemodel()` true.
- Thread safety: concurrent `GetDesign` for one IPC design (mirror `DesignCacheThreadSafetyTests` pattern).

**Acceptance:** new tests green; all ODB++ suites green; `GET /designs/led_power_board/components|nets|packages|parts` returns IPC data over REST with no controller edits (this is the proof of goal 2 — verify via smoke run, §8.3).

### Phase 4 — ProductModel extensions: placement + height (both formats)

**Purpose:** give format-agnostic consumers (future unified client endpoints) the data the 3D client needs to position and size component bodies.

Files:

- MOD `OdbDesignLib/ProductModel/Component.h/.cpp` — add members + getters: `double m_locationX/m_locationY` (mm), `double m_rotation` (degrees), `bool m_mirror`, `double m_height` (mm, 0 = unknown); extend ctor (add trailing parameters with defaults to preserve existing call sites) or add setters used only by `Design` builders; update `to_protobuf`/`from_protobuf`.
- MOD `OdbDesignLib/protoc/component.proto` — add `optional double locationX = 7; optional double locationY = 8; optional double rotation = 9; optional bool mirror = 10; optional double height = 11;` with comments documenting units (mm/degrees) and "0 = unknown" for height.
- MOD `OdbDesignLib/ProductModel/Design.cpp` — ODB++ backfill in `BuildComponents`: copy `ComponentRecord::locationX/locationY/rotation/mirror` converted to mm using `ComponentsFile::GetUnits()` (inch → ×25.4); height from the `.comp_height` attribute via the record's `AttributeLookupTable` (index of `".comp_height"` in `ComponentsFile::GetAttributeNames()`), same conversion; leave height 0 when absent (do not replicate the client's full heuristic cascade — keep it optional/real-data-only).
- MOD IPC builders (from Phase 3): populate from `<Location>`/`<Xform>`/`Component@height` using `CadHeader` units conversion.

Notes:

- Units normalization target is mm (matches the client migration guide's "floats are mm" contract). Document in proto comments.
- No new endpoints in this phase — data flows through existing `GET /designs/<n>/components` JSON and gRPC `GetDesign` automatically via the proto additions (additive optional fields → old clients unaffected).

**Tests (new + extended):**
- IPC placement (`led_power_board`, units MM): J1 at (5.0, 12.5) mm rot 0; R1 rot 90; R2 mirror true + side Bottom.
- IPC height (`testcase10_full`, units INCH): U1 `height="0.150000"` → 0.15 in ≈ 3.81 mm (assert within epsilon); `led_power_board` components carry no height attr → height stays 0 (unknown).
- ODB++: extend an existing `sample_design`/`designodb_rigidflex` test (e.g. in `EnhancedTests.cpp` or new `ComponentPlacementTests.cpp`) asserting at least one component has non-zero placement copied from its ComponentsFile record and sane mm values; components lacking `.comp_height` have height 0.
- Proto round-trip including new fields.

**Acceptance:** new tests green; ODB++ suites green (no behavior change beyond additive data); JSON for `/designs/<n>/components` gains optional fields only.

### Phase 5 — API enablement (polymorphic `/filemodels`, graceful degradation, navigation metadata)

**Purpose:** the API surface behaves correctly for both source types.

Files:

- MOD `OdbDesignServer/Controllers/FileModelController.cpp/.h`
  - `TryGetFileArchive`-based handlers: introduce `TryGetFileModel` helper; where a handler is ODB++-specific and the model is IPC, return the D4 response (default: 404 + message naming the format).
  - `GET /filemodels/<n>`: polymorphic response (D2=A): serialize whichever concrete model the design has.
  - `GET /filemodels/<n>/steps` + `/steps/<s>/layers` for IPC: synthesize from `Ipc2581XmlFile::GetStepNames()`/`GetLayers()` — same JSON shapes as today (`{"steps":[...]}`, `{"layers":[...]}`).
  - `GET /filemodels/<n>/matrix/matrix` for IPC: synthesize a `MatrixFile`-shaped payload from `<Layer>` + `<Stackup>` (row = stackup order for stackup layers, others appended; `type` mapped from `layerFunction` — CONDUCTOR/SIGNAL→copper, SOLDERMASK, PASTEMASK, SILKSCREEN, BOARD_OUTLINE, DIELPREG/PLANE→dielectric-adjacent; side from `Layer@side`; color from default palette by row). First inspect `OdbDesignLib/protoc/matrixfile.proto` + what the client parses (`row`, `type`, `color`, `side`); keep the mapping minimal and document it in code. If MatrixFile's shape cannot express it cleanly, return a documented JSON object with the same field names the ODB++ payload has (do not invent a third shape).
- MOD `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` — geometry RPCs: replace `GetFileArchive` nullptr→NOT_FOUND with: `GetFileModel` first; if model exists but is IPC → D4 status/message ("...only available for ODB++ designs"); if no model at all → existing NOT_FOUND. `GetDesign` unchanged (works via Phase 3).
- MOD `OdbDesignServer/Controllers/DesignsController.cpp` — verify only (should need zero changes).
- MOD `OdbDesignServer/Controllers/FileUploadController.cpp` — verify `.xml` uploads work end-to-end; no code change expected (sanitizeFilename TODO is pre-existing).

**Tests (new):** `Ipc2581ApiTests.cpp`
- REST-level: use the server-app-less controller pattern if one exists, otherwise cover via cache/service level + smoke (§8.3): GET `/filemodels/led_power_board` returns IPC payload; `/filemodels/led_power_board/steps` returns `{"steps":["LED_POWER_BOARD"]}`; `/steps/LED_POWER_BOARD/layers` returns the 11 layer names; `/matrix/matrix` returns synthesized rows; `/filemodels/led_power_board/steps/LED_POWER_BOARD/eda_data` → D4 error; same routes for `sample_design` unchanged.
- gRPC service-level (pattern: `GetStandardFontsTests.cpp` compiles `OdbDesignServiceImpl` into the test binary): `GetLayerFeaturesStream` for an IPC design returns the D4 status; for an ODB++ design unchanged; `GetDesign` for IPC returns `has_ipcfilemodel()` true.
- Upload round-trip via smoke only (§8.3) unless an upload test harness already exists.

**Acceptance:** all suites green; documented curl evidence (§8.3) for both an ODB++ and an IPC design across the route matrix in §4.5.

### Phase 6 — Hardening, docs, CI

- Run the full matrix: `linux-debug`, `linux-dynamic-release` presets; optionally `x64-*` on Windows if available.
- `OdbDesignTests` parallel run (`-j$(nproc)`) to catch fixture races on the shared TEST_DATA (IPC tests must copy samples into scratch dirs, never parse in place in `TEST_DATA` — same rule as `FileArchiveLoadFixture` documents).
- Update `AGENTS.md` only if new durable commands/paths emerged; update this plan doc's decision log (§6) with chosen options and any deviations discovered during implementation.
- PR description links this doc; list the route-matrix evidence from §8.3.

---

## 8. Testing strategy

### 8.1 Test data

- IPC samples: `$ODB_TEST_DATA_DIR/FILES/IPC2581/` (9 files + README with provenance and verified counts — §5.2).
- ODB++ archives: `$ODB_TEST_DATA_DIR/*.tgz` (unchanged).
- Rule (mirrors `FileArchiveLoadFixture`): anything that writes/extracts copies the file into a per-fixture scratch dir first; shared TEST_DATA is read-only during tests (parallel `ctest -j` safety).

### 8.2 New test files (summary)

| File | Phase | Covers |
|---|---|---|
| `DesignFormatSnifferTests.cpp` | 1 | magic bytes, XML root detection, BOM/whitespace, degenerate files |
| `Ipc2581XmlFileTests.cpp` | 2 | parser against all 9 samples; counts from §5.2; both pin encodings; robustness |
| `Ipc2581DesignBuildTests.cpp` | 3 | `Design` build via cache; mapping assertions; proto round-trip; kinds; thread safety |
| `ComponentPlacementTests.cpp` (or extension) | 4 | placement/height lift for both formats |
| `Ipc2581ApiTests.cpp` | 5 | route matrix incl. synthesized metadata + D4 errors; gRPC degradation |
| `OdbDesignTests/Fixtures/Ipc2581LoadFixture.{h,cpp}` | 3 | scratch-dir fixture mirroring `FileArchiveLoadFixture` |

Register all new files in `OdbDesignTests/CMakeLists.txt`.

### 8.3 Smoke verification (manual, per phase where noted)

Server smoke run (respecting known gotchas: `--designs-dir` must be a plain real path, no `..`/symlinks; Docker volume needs `chmod 777`):

```bash
# from repo root
mkdir -p smoke-designs
cp "$ODB_TEST_DATA_DIR/sample_design.tgz" smoke-designs/
cp "$ODB_TEST_DATA_DIR/FILES/IPC2581/led_power_board.xml" smoke-designs/
./out/build/linux-dynamic-release/OdbDesignServer/OdbDesignServer \
    --designs-dir smoke-designs --disable-authentication
```

Route matrix evidence to collect (curl):

```bash
# listing shows both, IPC typed
curl -s localhost:8888/designs
# product model (both formats)
curl -s localhost:8888/designs/led_power_board/components | head
curl -s localhost:8888/designs/led_power_board/nets | head
curl -s localhost:8888/designs/sample_design/components | head
# polymorphic file model
curl -s localhost:8888/filemodels/sample_design | head -c 400   # FileArchive JSON (unchanged shape)
curl -s localhost:8888/filemodels/led_power_board | head -c 400 # Ipc2581XmlFile JSON
# navigation metadata (IPC)
curl -s localhost:8888/filemodels/led_power_board/steps
curl -s localhost:8888/filemodels/led_power_board/steps/LED_POWER_BOARD/layers
curl -s localhost:8888/filemodels/led_power_board/matrix/matrix | head -c 400
# ODB++-only route on IPC design -> D4 error
curl -si localhost:8888/filemodels/led_power_board/steps/LED_POWER_BOARD/eda_data | head -1
# ODB++ unchanged
curl -s localhost:8888/filemodels/sample_design/steps
```

gRPC (grpcurl, reflection enabled):

```bash
grpcurl -plaintext localhost:50051 Odb.Grpc.OdbDesignService/HealthCheck
grpcurl -plaintext -d '{"design_name":"led_power_board"}' localhost:50051 Odb.Grpc.OdbDesignService/GetDesign | head -c 400
grpcurl -plaintext -d '{"design_name":"led_power_board","step_name":"LED_POWER_BOARD","layer_name":"TOP_COPPER"}' \
    localhost:50051 Odb.Grpc.OdbDesignService/GetLayerFeaturesStream   # expect D4 status
```

Attach trimmed outputs to the PR.

---

## 9. Risks & mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Large XML memory (43MB sample → ~150MB DOM) | OOM on bigger real-world files | Release DOM after record extraction (Phase 2); document SAX escape hatch (§4.6); beaglebone test tracks parse feasibility |
| Exporter schema variance (B vs C, attr vs child pins, missing sections) | Parse gaps/failures | §5.1 quirk list encoded as tests on all 9 samples; tolerant defaults; warn-don't-fail for missing refs |
| Namespaced/prefixed XML documents | Element lookup misses | Root-element exact-match gate; known-limitation note; no prefixed samples exist today |
| API backward compat (JSON shapes, proto wire) | Breaks existing clients | Additive-only proto changes (D1=A), unchanged REST shapes for ODB++ (constraint §3.5), route-matrix smoke evidence in PR |
| Same-stem collisions (`foo.tgz` + `foo.xml`) | Nondeterministic loads | Sorted iteration + first-match-wins + warning (§4.4) |
| Phase 4 units mistakes (INCH samples) | Wrong placement by 25.4x | Explicit unit-conversion tests on both MM (led_power) and INCH (testcase10) samples |
| Client expects synthesized ODB++ data (D5=B) | Current client can't render IPC yet | Accepted & documented (§2.2, §10); navigation metadata synthesis reduces picker breakage |
| vcpkg override pin wrong | Full dependency rebuild | Phase 0 acceptance checks for this explicitly |

---

## 10. Follow-on work (out of scope here, tracked for later)

1. **IPC geometry serving** — expose `<LayerFeature>`/`<Profile>`/`<Package><Outline>` data: format-agnostic geometry messages + streaming RPC (equivalent of `GetLayerFeaturesStream`) and a `GetPackageGeometry`-style endpoint as envisioned by the client migration guide. Includes units-normalization contract (floats=mm) server-side.
2. **Client migration** (`odbdesign-3d-client-prototype` repo) — consume product model + Phase 4 placement/height instead of `Design.fileModel`; remove local ODB++ symbol parsing per the archived migration guide; fix hardcoded `comp_+_top`/`comp_+_bot` layer names.
3. **Multi-step support** — `Design` builds first step only today (both formats); extend when a real multi-step sample appears.
4. **Optional namespace/directory rename** — `FileModel/Design` → `FileModel/Odb` once the seam is proven.
5. **IPC-2582** — separate file model on the same seam if/when needed.
6. **`sanitizeFilename` TODO** in `FileUploadController` — pre-existing gap, now more visible with arbitrary `.xml` uploads.

---

## 11. Appendix: current-state code map (verified 2026-08-30)

| Concern | Location |
|---|---|
| ODB++ file model | `OdbDesignLib/FileModel/Design/` (`FileArchive` + 19 parsers) |
| Product model | `OdbDesignLib/ProductModel/` (`Design`, `Net`, `Package`, `Pin`, `PinConnection`, `Component`, `Part`, `Via`) |
| Cache | `OdbDesignLib/App/DesignCache.{h,cpp}` |
| REST controllers | `OdbDesignServer/Controllers/` (`DesignsController`, `FileModelController`, `FileUploadController`, ...) |
| gRPC service | `OdbDesignServer/Services/OdbDesignServiceImpl.{h,cpp}`, contract `OdbDesignServer/protoc/grpc/service.proto` |
| Protobuf data contracts | `OdbDesignLib/protoc/*.proto` (25 files incl. `design.proto`, `filearchive.proto`) |
| Proto/JSON plumbing | `OdbDesignLib/IProtoBuffable.h`, `Utils/JsonCrowReturnable.h` |
| Archive handling | `Utils/ArchiveExtractor.{h,cpp}`, `Utils/libarchive_extract.{h,cpp}` |
| Test fixtures | `OdbDesignTests/Fixtures/` (`TestDataFixture`, `FileArchiveLoadFixture`) |
| IPC samples | `OdbDesignTestData/TEST_DATA/FILES/IPC2581/` (+README) |
| Prior planning docs | `docs/plan/ipc2581/` (superseded by this doc, §header) |

---

## 12. Decision & change log

| Date | Entry |
|---|---|
| 2026-08-30 | v2 draft created. Decisions D1–D5 (§6) recorded as recommendations, pending reviewer sign-off. |
| | Reviewer: chosen options → ___, ___, ___, ___, ___ |
