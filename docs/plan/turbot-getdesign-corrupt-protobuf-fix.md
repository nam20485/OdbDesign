# Fix record: Turbot `GetDesign` corrupt protobuf (server-README BLOCKER)

**Date:** 2026-09-21 · **Branch:** `nam/fix-filemodel-uninit-pod-members` → `nam20485`
**Diagnosed from:** `docs/plan/server-README.md` BLOCKER row (client evidence in `/tmp/r1/turbot-*.log`, server `kubectl logs` 22:04–22:10 UTC 2026-09-21).

## Root cause

`GetDesign(Turbot)` returned structurally invalid protobuf in every flavor (cold,
cached-bytes, `include_normalized_lists`) because serialization read an
**uninitialized POD member** and put its raw byte on the wire:

- Turbot's `steps/turbot_f200/stephdr` omits `AFFECTING_BOM_CHANGED` (legally
  optional; sample_design authors `=0` and rigidflex `=1`, which is why only
  Turbot broke).
- `StepHdrFile::affectingBomChanged` (`bool`, no initializer) held heap garbage.
  `to_protobuf()` → `set_affectingbomchanged(...)` stored the raw non-0/1 byte,
  and the release-build serializer emitted it verbatim as a **multi-byte
  varint** (observed `0xf0` → 15728). That desyncs the whole stream — the
  client's varying "invalid wire type" / "end-group tag mismatch" / "trailing
  data" errors are all downstream of the desync.
- Debug builds masked it (fresh heap reads 0 → `false` → valid wire); REST
  masked it (proto3-JSON normalizes bools). Reading a non-0/1 bool is UB, so
  only optimized builds exposed it.
- Not a code regression and not the component-identity work: the uninitialized
  members date to the original stephdr parser (17abb1a, 2023-12). What was new
  was the input — Turbot's first-ever load on the deployed release image
  (`GET /designs/Turbot/steps` 404 at 21:51:28 proves it had never loaded).

Repro (local, release flavor only): parse `Turbot.zip` → `Design::to_protobuf(false)`
→ `SerializeToString` "succeeds" (103,824,087 bytes) → `ParseFromString` fails;
bisecting submessages isolates `step:turbot_f200/stephdr` (338 bytes); a manual
wire walk shows field 11 (`affectingBomChanged`) encoded as varint 15728.
Artifacts kept in `/tmp/turbot-repro/` (repro.cpp, repro2–5, Turbot.zip).

## Fix

In-class initializers on **every** uninitialized POD member in serialized
FileModel records — the same latent class existed in every file parser, not
just stephdr:

- `StepHdrFile` + `StepRepeatRecord` (the proven breaker)
- `FeaturesFile::FeatureRecord` (highest record count in any design)
- `ComponentsFile::ComponentRecord` / `ToeprintRecord`
- `EdaDataFile`: `FeatureIdRecord`, `NetRecord`(+`SubnetRecord`), `PackageRecord`
  (+`OutlineRecord`, `PinRecord`)
- `MatrixFile::StepRecord` / `LayerRecord`
- `NetlistFile::NetRecord` / `NetPointRecord`
- `ToolsFile::ToolsRecord`
- `StandardFontsFile::CharacterBlock::LineRecord` + the `= default`-constructed
  `m_xSize/m_ySize/m_offset`

Class members already initialized in `.cpp` ctors (RgbColor, SymbolName,
MiscInfoFile, ToolsFile/FeaturesFile/ComponentsFile/EdaDataFile file classes,
all ProductModel classes) were verified and left alone. Bools are the
wire-breaking case (raw-byte varint); uninitialized doubles/enums only
produced wrong values.

## Regression coverage

`OdbDesignTests/ProtobufSerializationTests.cpp`:

- `StepHdrFileTests.WireRoundTrip_MissingOptionalAttributes_SerializesValidProtobuf`
  — synthesizes a stephdr omitting the optional attributes (the Turbot
  condition), asserts defaults (`affectingbomchanged == false`, `id == 37`) and
  a valid `SerializeToString` → `ParseFromString` round trip. Release preset is
  where the raw byte hit the wire; CI runs it there.
- `FileArchive_WireRoundTrip_sample_design_Succeeds` — the existing round-trip
  tests stopped at `from_protobuf()` and never exercised the wire format.

## Verification

- Release repro against Turbot: `ROUND TRIP OK` (103,824,087 bytes, all
  submessages re-parse; was failing before the member initializers).
- Full `ctest` on `linux-dynamic-debug` and `linux-dynamic-release`: see PR
  checks.

## Deployment

Standard chain after merge: publish `nam20485-latest` image, `kubectl rollout
restart deployment/odbdesign-server-v1`, then live acceptance — `GetDesign`
(Turbot) must parse end-to-end (the client team's `/tmp/r1` harness rerun, or a
grpcurl probe), and the pod log must show no "Cached design bytes failed to
parse" fallback.
