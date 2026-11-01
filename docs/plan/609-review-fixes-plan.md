# PR #609 review fixes — execution plan

| | |
|---|---|
| Status | **IN PROGRESS** (branch `nam/getdesign-streaming`, riding open PR #609; approved 2026-11-01) |
| Source | `docs/reviews/zcode-workflow-609-review-report.md` (three-lens review, 16 findings) → `docs/reviews/zcode-workflow-609-findings-adjudication.md` (verification & adjudication) |
| Scope decision | Fix the 13 recommended findings + 2 appendix items. F5 (fonts estimate term) accepted as documented — the F2 estimator pin is its tripwire. F6 (Windows orphan spool accounting) deferred; fix shape on record: re-add bytes on failed remove at all four removal sites (`StreamSpoolCache.cpp:437-444, :356-364, :462-464, :490-493`) or sweep `*.spool` at `InitializeDirectory`. |
| P1 policy | F9: investigate the warm-RSS delta first (root cause); recalibrate the gate from measurements only if no fixable buffering bug is found. |

All work lands as batched commits on the open PR #609 (never a new PR — minimize-PR-count directive). Merge later with `gh pr merge --merge` only; never squash/rebase.

## Commit 1 — lib: `FeatureGroupRecord` serialization fix (Appendix-1, pre-existing)

- `OdbDesignLib/FileModel/Design/EdaDataFile.cpp:204-210`: the property-record loop calls `add_featureidrecords()` then `CopyFrom(*propertyRecord->to_protobuf())` — a cross-type `Message::CopyFrom` (runtime type-confusion/UB if ever executed; the proto fields are distinct: `edadatafile.proto:162-168`). Change to `add_propertyrecords()`.
- Unit test: construct a pb `FeatureGroupRecord` with both `featureidrecords` and `propertyrecords` populated → `from_protobuf` → `to_protobuf` → assert property payloads land in field 2 (`propertyrecords`), not field 3, and the message round-trips. Corpus-unexercised today; no golden/conformance impact.

## Commit 2 — server: walker + service fixes (F1, F3, F7, F11 + proto comments)

- **F1 collision survivor filter** (`DesignStreamWalker.cpp:96-152`): before emitting FeatureBatches, compute survivor sets in step-iteration order (std::map order = the monolith's overwrite order): sanitized-key → surviving raw step (last-wins, matching `FileArchive.cpp:294`), and per surviving step sanitized-key → surviving raw layer (matching `StepDirectory.cpp:257-264`). Emit FeatureBatch chunks only for raw steps/layers that are the final holder of their sanitized key; `logwarn` each collision (mirror `ProtobufMapHelpers.h:25-26` wording). StepSkeleton emission unchanged (last-wins map assignment already matches both paths).
- **F1 regression test**: in-memory pb `FileArchive` — one step, two layers with raw keys `"\x81"` and `"\x8D"` (both sanitize to U+FFFD), each `featurefile` with ≥1 feature record → `from_protobuf` → `Design::Build(fa)` → assert the walker emits a FeatureBatch for the survivor layer only and reassembly is byte-identical (`SerializeDeterministic`) to `to_protobuf()`. Caveat: the input pb message with invalid-UTF-8 map keys must never itself be serialized — `from_protobuf` only iterates (safe). Plus a colliding-step-names variant.
- **F1b/F11b proto comments** (`service.proto:25-33`, `:144-158`): scope the byte-exactness claims to designs whose step/layer names sanitize to distinct keys; recommend client reassembly iterate maps rather than `find()` (#599). Comment-only; regenerated `service.pb.h/cc` are wire-identical.
- **F11** (`DesignStreamWalker.cpp:416, :422`): replace protobuf-Map `find()` with linear scans in `ApplyDesignChunk` (maps are tiny), matching the convention codified in `ProtobufSerializationTests.cpp:260-268`.
- **F3 spool-outcome logging** (`OdbDesignServiceImpl.cpp:380-430`): replace the `(spooling ? " spool=committed" : (overBudget ? " spool=aborted" : ""))` expression with an explicitly tracked state (none-attempted / open-failed / aborted / committed) updated at each transition. Fixes both mislabels: "aborted" when nothing was attempted (normalized flavour) and "committed" for actually-aborted fills (cancel / generation bump).
- **F7 TOCTOU hoist** (`OdbDesignServiceImpl.cpp:320, :375`): move the `generationAtStart` read to **before** `GetDesign()` so any invalidation between generation-read and publish trips the `:414-416` re-check → fill aborts → next request refills (safe direction; the streamed client still gets its content). The identity re-validation pattern at `DesignCache.cpp:1142-1149` is available as belt-and-braces; the hoist alone closes the hole.

## Commit 3 — estimator comment + pin (F2, F4)

- **F4**: `Design.cpp:478-484` — soften "every serialized term below is counted" to "dominant bulk counted; ByName-map duplication and embedded per-record propertyrecords amortized into coefficients; fonts under the flat MISC allowance".
- **F2 CI test** (`DesignStreamTests.cpp`): for `sample_design` and `rigidflex`, both flavours: `EstimateWireSizeBytes(flavour) >= 0.5 * to_protobuf(flavour)->ByteSizeLong()`. Pins against term disappearance, not precision.
- **F2 bench print** (`DesignStreamMemoryTests.cpp`): `RunLargeDesignScenario` prints `estimate=` (default flavour) at the parse phase; the sample_design perf gate prints estimate alongside `monolithic_bytes` — records estimate-vs-actual and the OOM designs' headroom over the 256 MiB budget for M6.5 evidence.

## Commit 4 — test batch (F8, F10, F12–F16, Appendix-2)

- **F15** (`DesignStreamTests.cpp:510-513`): increment `fillsReady` **before** the `BeginWrite` early-return (one line) so a failure can't wedge the barrier.
- **F12**: negative `ApplyDesignChunk` unit tests — step/features-before-header → false; FeatureBatch naming an undelivered step/layer → false; empty chunk (`PAYLOAD_NOT_SET`) → false.
- **F10a cancel test**: stream an over-budget fill (budget=1), `ctx.TryCancel()` (thread-safe, `client_context.h:406`) from the read loop after N chunks → assert client `Finish()` is CANCELLED and `streamSpoolCache().EntryCount()==0` (locks IsCancelled→OK + abort-not-commit).
- **F10b warm corruption test**: service configured with `stream_spool_dir` = managed temp dir; over-budget cold fill commits; glob `*.spool` (no path accessor exists) and truncate the file; warm request → INTERNAL + `EntryCount()==0` (covers `OdbDesignServiceImpl.cpp:358-364`).
- **F14 spool-degrade test**: `stream_spool_dir` pointing under a regular file (un-creatable dir → `m_usable=false` → `BeginWrite` false): over-budget stream completes OK, bytes identical, `EntryCount()==0`.
- **F13** (`GrpcServiceConfigTests.cpp`, existing `writeConfigFile` + `LoadFromFile` pattern): `{"grpc":{"design_streaming":{...}}}` — `enabled` passthrough; `feature_batch_size` clamp 50→100 / 20000→10000 / in-range; `spool_max_bytes: 0` → `UINT64_MAX`; `spool_dir` passthrough.
- **F16a**: fix stale "~4 GiB" header comment (`DesignStreamMemoryTests.cpp:52`) → real gates (6 GiB absolute; +1 GiB marginal); correct `docs/plan/getdesign-streaming-oom-plan.md` §M6.5 "~4 GiB" → implemented 6 GiB per-design.
- **F8/F16b**: re-indent `RunLargeDesignScenario` body (whitespace-only hunk).
- **Appendix-2** (`DesignStreamTests.cpp:280-283`): raw `EXPECT_EQ(streamed.bytes, monolithicBytes)` → `BytesEqual` (the file's own `:78-81` comment warns the raw form is a quadratic-diff hang on failure).

## Commit 5 — F9 (P1): warm-RSS investigation → gate recalibration

1. Instrument (opt-in under `ODB_DESIGN_STREAM_BENCH`): per-phase glibc heap stats (`mallinfo2`/`malloc_trim`) + `/proc/self/smaps_rollup` in the `[BENCH]` warm lines; experiment mode resetting VmHWM + trimming between the warmUp (discard) pass and the timed warm pass to attribute the +0.4–0.65 GiB delta.
2. Run the 350-41017 scenario several times; classify: (a) live allocations in the Reader/serve loop → fix the buffering; (b) gRPC send-path buffering → evaluate write options; (c) allocator-retention / measurement methodology → fix the measurement structure.
3. Recalibrate from data: prefer `warmPeakKb <= coldPeakKb + slack` (warm is a strict subset of cold's work; removes the parse-baseline double variance); slack from measured spread, documented in the test comment. If (a)/(b) produced a code fix the gate may need no loosening — fix first, recalibrate second (no stopgaps).
4. Success: 5 consecutive green `LargeDesign_350_41017_StreamWithinRssBudget` runs with ≥ 200 MB worst-case margin; `LargeDesign_200_40628_StreamWithinRssBudget` green; updated numbers into the PR body M6.5 evidence.

## No-work items

- **F16c**: PR #609 comment noting 6a8bfd1's `fix(ci)` scope label is inaccurate — comment only, no history rewrite.
- **F5** accepted as documented; **F6** deferred (fix shape above).

## Files touched

`OdbDesignLib/FileModel/Design/EdaDataFile.cpp` · `OdbDesignLib/ProductModel/Design.cpp` (comment) · `OdbDesignServer/Services/DesignStreamWalker.cpp` · `OdbDesignServer/Services/OdbDesignServiceImpl.cpp` · `OdbDesignServer/protoc/grpc/service.proto` (comments) · `OdbDesignTests/DesignStreamTests.cpp` · `OdbDesignTests/DesignStreamMemoryTests.cpp` · `OdbDesignTests/GrpcServiceConfigTests.cpp` (+ an EdaDataFile round-trip test) · `docs/plan/getdesign-streaming-oom-plan.md` (one line) · this file. No REST/swagger/vcpkg/CMake changes; no new dependencies.

## Verification

1. `cmake --build --preset linux-dynamic-release` (then `-debug`).
2. Targeted: `DesignStreamTest.*`, new config tests, EdaDataFile round-trip, `StepHdrFileTests.*` ×10, walker collision tests.
3. Full suite: `ctest --preset linux-dynamic-release -j$(nproc)` — all green (baseline 257).
4. Bench: 350-41017 ×5, 200-40628 ×1, `SampleDesign_ColdStreamVsMonolithic_Perf`.
5. Diff hygiene: proto diff comment-only; no files outside the list above.
6. Windows: no platform-specific constructs; CMake legs gate at development promotion.
