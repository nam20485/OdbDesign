# Clean-Room Reimplementation Plan — ToolsFile (Drill Tools) & Matrix Color

*Restores the functionality removed in PR #623 (contributor carve-out) as new, originally-authored code. Written 2026-11-11; execution starts after #623 merges and this plan is approved.*

## 1. Clean-room protocol (the commitment)

The reimplementation must be new authorship, derived from the specification and file *data* — never from the removed implementations. Accordingly:

**Prohibited while implementing (until both reimplementation PRs are merged):**

- `git show` / `git diff` of the carve-out commits `212429c`, `fbe5b3b`, `a1ceda3`, `59ce5a2`, `231164a`, `ad1d82c`
- Reading `ToolsFile.cpp` / `ToolsFile.h` / `toolsfile.proto` / `color.proto` from any historical commit (`git show <sha>:<path>`, history browsing of those paths)
- Reading the removed color-serialization lines from PR #623's diff

**Disclosure (what was unavoidably seen during excision, for honesty):**

- File *stats* of all carve-out commits (paths + line counts only)
- The removed color glue (~14 lines in `MatrixFile.cpp`/`.h`: `RgbColor::from_string` call, four `mutable_color()` setters, four getters, the member declaration) and the one-line `ContourPolygon` type cast — handled while removing them
- **The ToolsFile implementation body (~434 + 122 lines) was never read** — only its file stats and external call sites. The ToolsFile reimplementation is therefore fully clean-room.

Practical effect: the ToolsFile reimplementation derives everything from the sources below. The color reimplementation is glue between two project-owned interfaces (`RgbColor` — project code predating the contribution — and the matrix proto message authored fresh here); its total surface is ~15 lines of dictated wiring, and it deliberately changes the wire contract (new field number; old is reserved) so the result is not derived from the removed schema.

## 2. Sources of truth (the only inputs)

1. **ODB++ spec** (`../OdbDesign-drc/docs/odb_spec_user.pdf`, v8.1 U4):
   - *Layer Entities — `<layer_name>/tools (Drill Tools)`* (spec pp. 183–186): structured text, no compression; global params `UNITS` (required), `THICKNESS` (obsolete — parse-tolerate, ignore), `USER_PARAMS` (free text); `TOOLS` array with `NUM`, `TYPE` ∈ {PLATED, NON_PLATED, VIA}, `TYPE2` (legal values depend on TYPE: PLATED → STANDARD|PRESS_FIT; NON_PLATED → STANDARD; VIA → STANDARD|PHOTO|LASER; default STANDARD), `MIN_TOL`, `MAX_TOL`, `BIT`, `FINISH_SIZE` (required; −1 when unset), `DRILL_SIZE` (optional)
   - *Fields in the LAYER Array of a Matrix File* (spec p. 64): `COLOR` attribute, packed RGB hex or `0`
2. **File data** in the test corpus (data, not code): 8 tools files across `designodb_rigidflex*` designs + 2 in `sample_design`; COLOR rows in both designs' matrix files
3. **Project conventions**: the file-model sibling classes (`ComponentsFile`, `AttrListFile`, `FeaturesFile` — all project-authored): `Parse(directory)` / stream parse, `Save(stream)`, `IProtoBuffable<T>` (`to_protobuf` / `from_protobuf` / `FillProtobufMessage` shape), `throw_parse_error` diagnostics, `RECORD_TOKEN`/`*_KEY` constants
4. **Project-owned types**: `RgbColor` (`from_string`/`to_string`, red/green/blue/noPreference)

**Not inputs:** the removed implementations (see §1), the old proto schemas, any memory of their field names/ordering beyond what the spec itself states.

## 3. Design

### 3.1 `toolsfile.proto` (new file, new schema)

Authored from the spec grammar. Field numbers chosen fresh — deliberately **not** constrained by the removed schema (it is deleted and unreferenced). Sketch:

```proto
message ToolsFile {
    optional string units = 1;
    optional string thickness = 2;      // obsolete per spec; kept as string, parse-tolerant
    repeated string userParams = 3;     // spec: free text
    repeated Tool tools = 4;

    message Tool {
        optional uint32 num = 1;
        optional ToolType type = 2;
        optional ToolSubType type2 = 3;  // legality depends on type; validated at parse time
        optional string minTol = 4;      // spec: mils or microns; string-tolerant like other tol-ish fields
        optional string maxTol = 5;
        optional string bit = 6;
        optional string finishSize = 7;  // "-1" when unset per spec
        optional string drillSize = 8;
    }

    enum ToolType { STANDARD_DEFAULT = 0; PLATED = 1; NON_PLATED = 2; VIA = 3; }
    enum ToolSubType { SUB_DEFAULT = 0; STANDARD = 1; PRESS_FIT = 2; PHOTO = 3; LASER = 4; }
}
```

(Final numbering/naming settled at implementation time from the spec text above — the sketch fixes the *shape*, and proto3 enum zero-defaults are honored.)

### 3.2 `layerdirectory.proto`

`tools` returns as a **new field number 7** (6 stays `reserved`): `optional ToolsFile tools = 7;`

### 3.3 `ToolsFile.h/.cpp` (new, following project conventions)

- `class ODBDESIGN_EXPORT ToolsFile : public IProtoBuffable<Odb::Lib::Protobuf::ToolsFile>, public ISaveable`
- `Parse(directory)` reading `<dir>/tools`; strict parser with `throw_parse_error(m_path, line, token, lineNumber)`; per-spec validation: UNITS required; TYPE enum; TYPE2 legality matrix above
- `Save(stream)` emitting the same grammar (round-trip property, tested)
- Accessors: `GetUnits()`, `GetThickness()`, `GetUserParams()`, `GetTools()`, `GetToolsByName()`-style maps per house style

### 3.4 `LayerDirectory` re-wiring

Member `ToolsFile m_toolsFile;`, parse call in `Parse()`, `GetToolsFile()` accessor, `mutable_tools()->CopyFrom(...)` in `FillProtobufMessage`, `from_protobuf` line, and the `Save()` block — mirroring the sibling files' structure (each site is a one-to-three-line mechanical expression of the house pattern).

### 3.5 Wire-size estimate (`ProductModel/Design.cpp`)

`toolCount += pLayer->GetToolsFile().GetTools().size();` returns inside the layer loop (the `toolCount * TOOL_RECORD_WIRE_BYTES` term never left — it has been contributing 0).

### 3.6 Matrix color (re-store)

- `matrixfile.proto`: color returns as a **new field number (18; 16 stays `reserved`)** with a locally-defined `Color`-style message — authored here, not re-imported
- `MatrixFile.h`: `RgbColor color{ "0" };` member + `COLOR_KEY` return
- `MatrixFile.cpp`: parse `COLOR` → `from_string`; `Save` line; to/from-protobuf blocks — pure glue between `RgbColor` and the new message

## 4. Tests (new file `ToolsFileTests.cpp` + matrix round-trip)

1. **Real-data parse:** open each tools file extracted from `designodb_rigidflex` (8 files) and `sample_design` (2 files) via `LayerDirectory::Parse` — assert tool counts > 0 and `TYPE` enums in the legal set
2. **Round-trip:** `Parse` → `Save` → re-`Parse` → assert identical `to_protobuf` serialization (write-then-read-back, per the no-grep-validation rule)
3. **TYPE2 legality:** malformed pair (e.g. `TYPE=NON_PLATED` + `TYPE2=PHOTO`) → `parse_error`
4. **FINISH_SIZE unset sentinel:** `FINISH_SIZE=` empty → parsed as `"-1"` per spec
5. **Wire estimate:** parse a design with tools → design-size estimate increases vs. pre-parse baseline (behavioral, not string, check)
6. **Matrix color round-trip:** parse matrix with `COLOR` rows → protobuf round-trip preserves values → `Save` re-emits them
7. Full existing suite stays green (no regressions in the 273)

## 5. Sequencing & sizing

| Step | Depends on | Size |
|---|---|---|
| #623 merge (excision evidence) | CI green | — |
| ToolsFile clean-room (proto + class + wiring + tests 1–5) | #623 merged + plan approved | ~1 day |
| Matrix color re-store (proto + glue + test 6) | #623 merged + plan approved | ~2 hours |
| One PR `nam/toolsfile-color-cleanroom` → `nam20485` (two commits, one per workstream) | both above | — |
| Overlay submodule bump | PR merged | — |

The two workstreams are independent; color can land first if tools-file parsing needs more spec iteration.

## 6. Acceptance

- Full `ctest` green (both presets) including the new tests
- `git grep` shows zero references to the prohibited history (and the protocol in §1 held: no prohibited reads during implementation)
- PR body references #623 and this plan
