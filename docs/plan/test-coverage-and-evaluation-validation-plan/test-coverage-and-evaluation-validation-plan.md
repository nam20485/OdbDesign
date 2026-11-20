# Test Coverage and Customer-Evaluation Validation Plan

**Date:** 2026-11-17  
**Status:** Analysis and proposed execution plan; not a release certification  
**Scope:** `nam20485/OdbDesign`, the private `nam20485/OdbDesign-Commercial` overlay, its pinned upstream submodule, and the customer-facing REST/gRPC/.NET delivery path  
**Primary objective:** Raise meaningful handwritten-production coverage from the current approximately 70–75% range to **strictly greater than 90%**, while adding enough behavioral, integration, end-to-end, security, performance, and exact-artifact validation to support a first live-customer evaluation with credible evidence.

> **Bottom line:** the current test suite has valuable parser, cache, archive-security, concurrency, protobuf, and real in-process gRPC coverage, but the present coverage headline is not a whole-product measurement. Generated protobuf/gRPC code dominates the report, all REST controllers and server startup translation units are absent, branch coverage is not collected, and ten tests were skipped in the latest matching run. The commercial overlay has no measured coverage baseline, its tests are not wired into the packaging gate, and all five recorded packaging runs failed. The recommended release gate therefore combines: **(1) >90% honest source coverage, (2) real protocol and workflow correctness, and (3) successful rehearsal of the exact signed artifacts intended for delivery.**

---

## Contents

1. [Executive summary](#1-executive-summary)
2. [Evidence, revisions, and limitations](#2-evidence-revisions-and-limitations)
3. [Current assurance baseline](#3-current-assurance-baseline)
4. [Coverage measurement findings](#4-coverage-measurement-findings)
5. [Detailed gap analysis](#5-detailed-gap-analysis)
6. [Target metrics and coverage arithmetic](#6-target-metrics-and-coverage-arithmetic)
7. [Roadmap to greater than 90%](#7-roadmap-to-greater-than-90)
8. [Commercial-overlay coverage and delivery plan](#8-commercial-overlay-coverage-and-delivery-plan)
9. [Expanded testing and validation types](#9-expanded-testing-and-validation-types)
10. [First-customer evaluation acceptance protocol](#10-first-customer-evaluation-acceptance-protocol)
11. [CI lanes and cadence](#11-ci-lanes-and-cadence)
12. [Evidence bundle and reporting](#12-evidence-bundle-and-reporting)
13. [Effort, sequencing, and ownership](#13-effort-sequencing-and-ownership)
14. [Decisions and risks requiring ownership](#14-decisions-and-risks-requiring-ownership)
15. [Definition of done](#15-definition-of-done)
16. [Appendix: target-state command outline](#16-appendix-target-state-command-outline)
17. [Sources](#17-sources)

---

## 1. Executive summary

### 1.1 What the current number actually means

The newest retained matching public CI artifact reports:

- **73.16% lines:** 20,027 covered of 27,376.
- **75.45% functions:** 3,304 covered of 4,379.
- **Branch coverage:** not collected.
- **Tests:** 314 discovered; 304 passed; 0 failed; **10 skipped**.

That headline includes generated protobuf/gRPC code. Recalculating the same tracefile for represented handwritten production source gives:

- **70.64% lines:** 5,877 covered of 8,320.
- **65.02% functions:** 619 covered of 952.

Even those handwritten figures are optimistic as a whole-product baseline: nine substantive production `.cpp` files—five REST controllers, server application/base/main, and the CLI—have no LCOV source record at all. They total approximately 2,904 **physical** lines, but physical lines cannot be substituted for instrumentable-line counts.

The commercial pin has its own matching public CI result: **73.2% lines / 75.5% functions**, or **70.65% represented handwritten lines** after the same generated-code recalculation. The one-covered-line difference from the newer artifact is not meaningful assurance.

### 1.2 Highest-risk gaps

The most important gaps are customer-facing rather than merely numerical:

1. **No automated real HTTP route suite.** REST controllers, uploads, authentication, URL decoding, response headers, and conditional GET behavior are not exercised through a running Crow server.
2. **Two layer-feature streaming RPCs are effectively uncovered.** `GetLayerFeaturesStream`, `GetLayerFeaturesBatchStream`, and gRPC `HealthCheck` contain major zero-hit regions. The existing batch test file is not compiled.
3. **Core ODB++ grammar variants remain untested.** STEP-REPEAT, feature text/surface/polarity variants, EDA cutouts/pins/electrical/mount types, BOM fields, netlist options, and malformed-input paths account for large, high-value gaps.
4. **Required assurance can silently disappear.** Ten skips include response-cache correctness, connectivity goldens, large-design memory tests, and fetch benchmarks. Some timing-sensitive tests skip when the desired race does not occur.
5. **Some files named as integration/performance tests simulate work.** Filesystem existence, STL allocation, sleeps, random strings, and helper retry loops are not product integration evidence.
6. **Commercial delivery is not currently reproducible as a passing artifact.** The only packaging workflow runs no tests, no commercial coverage exists, and all five recorded package runs failed. No successful current commercial package was available for inspection.
7. **Known security and compatibility boundaries remain.** Missing REST credential environment values fall back to known defaults; gRPC is plaintext and unauthenticated; vendored .NET clients match `service.proto` but differ in three model protos with substantive field/layout differences.

### 1.3 Recommended release policy

Do not equate “90% coverage” with “ready.” Require three independent gates on one immutable release candidate:

| Gate | Required result |
|---|---|
| **Source quality and coverage** | Complete handwritten denominator; **>90% lines and functions**, **≥85% branches**; all critical component floors pass; no unexpected required-suite skips; native release suites and sanitizer scope pass. |
| **Behavioral and protocol correctness** | Real parser/model assertions, real HTTP and gRPC round trips, external .NET client/sample execution, stream equivalence, negative cases, cancellation, restart, persistence, and representative/largest customer designs. |
| **Exact-artifact delivery** | Successful packaging; checksums/signatures/SBOM/provenance verified; commercial license and loopback behavior proven through process exits and sockets; clean-machine install/SDK consumption; immutable artifact/image identity; complete evidence bundle. |

**Recommendation:** do not describe the current state as customer-validated. Execute the P0 items and release only when all applicable gates pass against the exact delivery candidate.

---

## 2. Evidence, revisions, and limitations

### 2.1 Immutable evidence used

| Item | Observed value |
|---|---|
| Public coverage run | [GitHub Actions run 44011295347](https://github.com/nam20485/OdbDesign/actions/runs/44011295347), generated 2026-11-17 |
| Public coverage artifact | [`linux-coverage-report`, artifact 14298493942](https://github.com/nam20485/OdbDesign/actions/runs/44011295347/artifacts/14298493942) |
| Run head / actual PR merge checkout | `1e29cc6824d1979f20484309553d6306aafc0cab` / `590fb315459fa4c605776ae81597cb72b3baf46d` |
| Commercial-pin-matched public run | [Run 43310307317](https://github.com/nam20485/OdbDesign/actions/runs/43310307317), upstream SHA `771dd90e2bfba74b176362e041b55aa39587a49c` |
| Commercial checkout | `main` at `87aeb9742663b43baf18e9d18cdf3a310f6f63bb` |
| Commercial submodule | `upstream/OdbDesign` at `771dd90e2bfba74b176362e041b55aa39587a49c`; clean and matching the recorded gitlink when inspected |
| Latest commercial package run | [Run 44007546488](https://github.com/nam20485/OdbDesign-Commercial/actions/runs/44007546488), failed; source-affecting HEAD `f6203a414f0560529aad8801678a8c1f74975f60` |
| Commercial packaging history | Five recorded `Package Commercial Build` runs; all five failed |
| Existing LicenseGuard evidence | Historical local log records 13 passing standalone tests on 2026-11-07; stale relative to later source/package changes and not coverage data |

The public checkout advanced during this read-only analysis. This report therefore cites immutable CI commits rather than claiming that an older artifact measures later archive-extraction or diagnostic commits. **M0 must regenerate all measurements on the eventual release-candidate SHA.**

### 2.2 Methods

- Inspected the retained LCOV tracefile, summary, genhtml output, and test results.
- Recalculated handwritten/generated totals from raw LCOV records.
- Inventoried production and test targets, fixtures, workflows, deployment tooling, and commercial packaging.
- Inspected concrete uncovered functions and source paths.
- Compared canonical proto files with both vendored .NET client proto trees.
- Inspected GitHub Actions run/artifact metadata through `gh`.

### 2.3 Limitations

- No new build, test, coverage, server, container, cluster, fuzz, or package run was performed for this report.
- No local current coverage artifact exists; the numerical baseline comes from retained CI artifacts.
- Existing artifacts contain no branch data and no complete zero-hit baseline for never-linked/never-run production objects.
- Commercial-owned code has no coverage artifact, so no commercial or combined percentage is claimed.
- No successful current commercial ZIP was available to inspect; packaging conclusions are source/workflow/run-status findings, not archive-content verification.
- No private signing-key contents were read. Customer designs, secret license files, and confidential packaging logs were not used.
- Effort ranges below are planning estimates, not commitments; they exclude substantial production fixes and customer waiting time.

---

## 3. Current assurance baseline

### 3.1 Existing strengths worth preserving

This effort builds on real coverage rather than starting from zero:

- Real ODB++ archives are copied into isolated scratch directories and exercised through archive discovery, extraction, parsing, and `DesignCache` (`OdbDesignTests/Fixtures/FileArchiveLoadFixture.cpp:73-108`).
- `DesignStreamTests.cpp` starts real in-process gRPC servers on ephemeral ports and uses generated stubs. It compares reassembled streams with monolithic responses and covers cold/warm spool behavior, corruption, cancellation, concurrency, disabled streaming, and over-budget rejection.
- Cache tests cover single-flight parsing, retry, invalidation, LRU behavior, concurrent access, background loading, and response caching.
- Protobuf tests include actual serialize/parse wire round trips, not only object construction.
- Archive-security tests exercise hostile paths, traversal, absolute paths, links, modes, symlink redirection, shell metacharacters, and process-spawn behavior.
- Large-design streaming tests already define useful memory and correctness assertions, including monolithic `RESOURCE_EXHAUSTED`, cold/warm equivalence, an approximately 6 GiB absolute peak budget, and an approximately 1 GiB serving-marginal budget for the named scenarios (`DesignStreamMemoryTests.cpp:288-430`).
- Public CI builds/tests Windows, Ubuntu, and macOS release configurations.
- Historical live deployment and streaming acceptance exists, but it is not a repeatable exact-eval-artifact gate.

### 3.2 Latest matching test outcome

| Result | Count | Interpretation |
|---|---:|---|
| Discovered | 314 | Linux Debug instrumented coverage run |
| Passed | 304 | Valuable, but does not include unexecuted scenarios |
| Failed | 0 | No observed failures in that run |
| Skipped | **10** | Must be decomposed by purpose and made visible |
| Duration | 2,067.68 s | Approximately 34.5 minutes for the instrumented run |

“100% tests passed” is not sufficient release language while required correctness/performance cases skip.

### 3.3 Skip inventory

| Skipped group | Count | Required disposition |
|---|---:|---|
| Local/large-corpus connectivity contracts | 3 | Run in a pinned scheduled/release corpus lane; missing required release fixtures fail that lane. |
| Opt-in response-cache byte-identity tests | 2 | Reduce to deterministic small fixtures where possible and run in every PR; otherwise make them explicit release-correctness gates. |
| Large-design stream/memory scenarios | 3 | Move to a mandatory release/performance lane on suitable hardware; do not count them as routine PR unit tests. |
| Design-fetch benchmarks | 2 | Keep as explicit benchmarks; use real transport benchmarks separately for API performance claims. |

Opt-in benchmarks may remain optional **only if they are registered in a separate lane with their own expected-test manifest**. They must not disappear as unexplained skips in the required correctness suite.

---

## 4. Coverage measurement findings

### 4.1 Generated code dominates the current denominator

| Scope | Files | Covered / total lines | Line % | Covered / total functions | Function % |
|---|---:|---:|---:|---:|---:|
| Everything currently reported | 139 | 20,027 / 27,376 | **73.16%** | 3,304 / 4,379 | **75.45%** |
| Represented handwritten source | 84 | 5,877 / 8,320 | **70.64%** | 619 / 952 | **65.02%** |
| Generated protobuf/gRPC | 55 | 14,150 / 19,056 | 74.25% | 2,685 / 3,427 | 78.35% |
| Generated library protobuf only | 51 | 12,549 / 15,392 | 81.53% | 2,332 / 2,743 | 85.02% |
| Generated gRPC `service.*` only | 4 | 1,601 / 3,664 | 43.70% | 353 / 684 | 51.61% |

Generated code supplies **69.61% of line denominator entries** and **78.26% of function denominator entries**. It accounts for 4,906 of 7,349 uncovered lines and 742 of 1,075 uncovered functions.

The cause is the current filter in `.github/workflows/code-coverage.yml:182-204`: it removes system, vcpkg, `_deps`, and test sources but not build-tree `*.pb.cc`, `*.pb.h`, `*.grpc.pb.cc`, or `*.grpc.pb.h`.

**Policy:** keep generated coverage as a separate diagnostic report if useful, but exclude it from the handwritten >90% gate. Do not try to raise the score by executing generated accessors.

### 4.2 Handwritten coverage by component

| Component/directory | Lines | Functions | Uncovered lines |
|---|---:|---:|---:|
| `OdbDesignLib` aggregate, represented source | 4,435 / 6,181 = **71.75%** | 480 / 721 = 66.57% | 1,746 |
| `OdbDesignServer` aggregate, represented source | 856 / 1,161 = **73.73%** | 58 / 64 = 90.62% | 305 |
| `Utils` | 586 / 978 = **59.92%** | 81 / 167 = 48.50% | 392 |
| `OdbDesignLib/App` | 608 / 977 = 62.23% | 82 / 124 = 66.13% | 369 |
| `OdbDesignLib/FileModel/Design` | 3,071 / 4,308 = 71.29% | 239 / 324 = 73.77% | **1,237** |
| `OdbDesignLib/ProductModel` | 595 / 700 = 85.00% | 91 / 109 = 83.49% | 105 |
| `OdbDesignLib/Text` | 108 / 131 = 82.44% | 9 / 10 = 90.00% | 23 |
| `OdbDesignServer/Services` | 740 / 1,030 = 71.84% | 53 / 59 = 89.83% | 290 |
| `OdbDesignServer/Config` | 116 / 131 = 88.55% | 5 / 5 = 100% | 15 |

The server aggregate excludes the controllers and application/startup files listed below; it is not whole-server coverage.

### 4.3 Production source absent from LCOV

| Absent substantive file | Physical lines | Customer significance |
|---|---:|---|
| `OdbDesignServer/Controllers/FileModelController.cpp` | 1,318 | Main file-model REST surface |
| `OdbDesignServer/Controllers/DesignsController.cpp` | 410 | Design list/load/product collections |
| `OdbDesignServer/Controllers/FileUploadController.cpp` | 411 | Customer upload and hostile-input boundary |
| `OdbDesignServer/Controllers/HealthCheckController.cpp` | 41 | Runtime health/readiness surface |
| `OdbDesignServer/Controllers/HelloWorldController.cpp` | 28 | Basic route/name handling |
| `OdbDesignServer/OdbDesignServerApp.cpp` | 232 | Server construction, gRPC configuration/listeners |
| `OdbDesignServer/OdbServerAppBase.cpp` | 298 | REST startup/lifecycle/shutdown |
| `OdbDesignServer/main.cpp` | 11 | Actual process entry point |
| `OdbDesignApp/OdbDesignApp.cpp` | 155 | CLI behavior and exit codes |
| **Total** | **2,904 physical lines** (as of the #641 review head; `wc -l` at HEAD) | Instrumentable total must be measured |

Five additional absent `.cpp` files are inert (zero-byte or include-only) and need inventory explanations rather than behavioral tests: `IOdbApp.cpp`, `IOdbServerApp.cpp`, `invalid_odb_error.cpp`, `EnumMap.cpp`, and `IJsonable.cpp`.

### 4.4 Why these files are absent

`OdbDesignTests/CMakeLists.txt:33-55,82-96` manually compiles gRPC configuration/service/stream sources and generated gRPC code into the test executable. `add_dependencies(OdbDesignTests OdbDesignServer)` builds the server but does not link or execute its full route/process composition.

`OdbDesignServer/CMakeLists.txt:9-33` already defines `OdbDesignServerLib` with all app/controller/service sources. The test arrangement therefore both omits customer-facing code and duplicates sources/codegen that the production library owns.

`OdbDesignTests/HttpCachingTests.cpp:23-26` explicitly documents that live route behavior is manually verified because no harness boots Crow.

### 4.5 Measurement limitations to correct

- Coverage is Linux/GCC/Debug only; it does not measure Release, Windows, macOS, ASan, or the deployed artifact.
- No branch records are captured.
- Post-test-only capture cannot provide a reliable complete inventory of unexecuted production translation units.
- `--ignore-errors mismatch` can conceal instrumentation/source inconsistencies; a clean release-candidate run should fail on unexplained mismatch rather than hide it.
- Test data is checked out from floating `OdbDesignTestData` `main` (`code-coverage.yml:56-61`), reducing reproducibility.
- `Linux Coverage` is currently described as trend-only and is not the required release gate proposed here.
- `scripts/coverage.sh` targets a different build tree and has a non-LCOV basic mode; it must not produce an incompatible local headline.

---

## 5. Detailed gap analysis

### 5.1 Largest represented handwritten gaps

The named hotspots below contain **1,802 uncovered lines**, approximately **73.8%** of the 2,443 uncovered lines in represented handwritten source. Closing them is necessary but not sufficient: omitted server/CLI code will add to the denominator.

| Production file | Covered / total lines | Functions | Missed lines | Primary behavioral target |
|---|---:|---:|---:|---|
| `FileModel/Design/EdaDataFile.cpp` | 625 / 869 = 71.92% | 28 / 38 | **244** | Nets, subnets, packages, pins, outlines, properties, enum variants, malformed records |
| `FileModel/Design/FeaturesFile.cpp` | 381 / 584 = 65.24% | 19 / 26 | **203** | Feature records, polarity, text, arcs, surfaces, polygons, symbol references |
| `App/DesignCache.cpp` | 497 / 671 = 74.07% | 67 / 76 | **174** | Bulk loads, eviction, response generation, exceptions, deterministic races |
| `Services/OdbDesignServiceImpl.cpp` | 253 / 422 = 59.95% | 14 / 17 | **169** | Both feature streams, health, errors, cancellation, write failure |
| `FileModel/Design/StepHdrFile.cpp` | 95 / 201 = 47.26% | 4 / 8 | **106** | Scalar fields, STEP-REPEAT, malformed braces/numbers, protobuf round trip |
| `FileModel/Design/FileArchive.cpp` | 188 / 288 = 65.28% | 26 / 33 | **100** | Root discovery, failure propagation, full/skeleton serialization, supported writes |
| `FileModel/Design/ComponentsFile.cpp` | 338 / 435 = 77.70% | 15 / 24 | **97** | CMP/TOP/PRP/BOM, optional fields, top/bottom, component height |
| `FileModel/Design/MatrixFile.cpp` | 186 / 276 = 67.39% | 10 / 12 | **90** | Step/layer fields, enums, nesting, real save/reparse where supported |
| `Services/StreamSpoolCache.cpp` | 228 / 310 = 73.55% | 24 / 27 | **82** | Corrupt/truncated spool, commit/abort, cleanup, stale generations, LRU |
| `FileModel/Design/NetlistFile.cpp` | 175 / 254 = 68.90% | 12 / 21 | **79** | Header options, nets/points, numeric boundaries, full protobuf behavior |
| `ProductModel/Design.cpp` | 402 / 467 = 86.08% | 41 / 47 | **65** | Missing references, model relationships, clipped/unclipped behavior |
| `FileModel/Design/MiscInfoFile.cpp` | 103 / 163 = 63.19% | 4 / 17 | **60** | Fields, timestamps, optional values, getters, serialization |
| `Utils/Logger.cpp` | 35 / 93 = 37.63% | 8 / 21 | **58** | Real log formatting/output, lifecycle, filtering, concurrency |
| `App/RouteController.cpp` | **0 / 55** | **0 / 6** | **55** | List responses, real ETag/Cache-Control/304 behavior |
| `Utils/str_utils.cpp` | 25 / 79 = 31.65% | 9 / 31 | **54** | Empty/boundary/high-bit strings, independent expected outputs |
| `FileModel/Design/ComponentHeightTracer.cpp` | 78 / 130 = 60.00% | 11 / 15 | **52** | Actual height propagation and diagnostics |
| `App/OdbAppBase.cpp` | **0 / 39** | **0 / 8** | **39** | Startup options, load/load-all, lifecycle |
| `App/BasicRequestAuthentication.cpp` | **0 / 38** | **0 / 3** | **38** | Real auth decisions and fail-closed credential configuration |
| `Utils/UrlEncoding.cpp` | **0 / 37** | **0 / 4** | **37** | Reserved/non-ASCII/malformed escapes and actual routes |

Other important low-coverage files include `OdbDesignArgs.cpp` (24.56% lines), `CommandLineArgs.cpp` (47.89%), `ThreadSafeQueue.h` (34.43%), `WorkQueueLoopThread.h` (19.05%), `Via.cpp` (0/18 lines), `RequestAuthenticationBase.cpp` (0/9), `OdbFile.cpp`, `SymbolsDirectory.cpp`, `AttrListFile.cpp`, `StandardFontsFile.cpp`, `EnumMap.h`, `bin2ascii.h`, `timestamp.cpp`, and `FileReader.cpp`.

### 5.2 P0: REST, authentication, upload, and shared route behavior

Add real HTTP requests against the actual server process, supplemented by small direct tests where useful.

#### `BasicRequestAuthentication.cpp`

Cover disabled auth, missing/wrong scheme, short/malformed headers, invalid Base64, missing colon, empty username/password, explicit environment credentials, wrong credentials, and correct credentials.

Current source at `BasicRequestAuthentication.cpp:38-75` falls back to `odb` / `plusplus` when environment values are absent or empty. **F004 is an evaluation blocker:** when authentication is enabled, missing credentials must fail closed before listeners open; no known default credential may authorize a request.

#### `RouteController.cpp`

Prove through HTTP:

- Empty/populated design-list bodies and 200/201/204 selection.
- Actual `ETag` and `Cache-Control` headers.
- Initial 200 → matching `If-None-Match` 304 without loading/serializing the design.
- Archive replacement or in-memory generation change → new ETag and fresh 200.
- No shared cached `Design` clipping side effect after REST-then-gRPC access.

ETag helper-only tests do not prove this route behavior.

#### `DesignsController.cpp`

Exercise list/load/components/nets/packages/parts, `includeFileArchive`, accepted/already-loading/already-loaded/not-found load states, decoded names, missing/corrupt designs, response bodies, and cache preservation.

#### `FileModelController.cpp`

Create a route matrix covering every registered endpoint at `FileModelController.cpp:82-412`: file models, steps, layers, EDA data, attrlist, profile, stephdr, netlists, components, features, symbols, misc info, matrix, and standard fonts. For each applicable route, test success, unknown design/step/layer/entity, encoded names, malformed input, and cache behavior. Assert representative JSON values and counts, not only status 200.

POST tests must include empty/malformed JSON, semantically incomplete models, write-then-read-back, and cache-generation rotation. Investigate the unchecked `from_json` outcome at `FileModelController.cpp:433-457`; do not bless silent non-mutation as a valid response.

#### `FileUploadController.cpp`

Exercise both octet-stream and multipart upload, exact stored-byte read-back, duplicate conflict without modifying the original, malformed multipart, unsupported content type, empty/oversized bodies, reserved/encoded/traversal names, write/move failure, auto-warm behavior, stale-cache invalidation, and restart persistence.

Library archive-security tests remain necessary, but only an HTTP upload test proves the complete request-to-filesystem security boundary.

### 5.3 P0: gRPC feature streaming and health

`OdbDesignServiceImpl.cpp` contains these major gaps:

- `GetLayerFeaturesStream`, lines 548-623: **0/36 executable lines**.
- `GetLayerFeaturesBatchStream`, lines 625-756: **0/60 executable lines**.
- `HealthCheck`, lines 1015-1037: **0/10 executable lines**.

`GrpcBatchStreamTests.cpp` has five useful early-error/config tests but is commented out at `OdbDesignTests/CMakeLists.txt:50-52`. Re-enable those tests, then add real generated-stub tests using the existing loopback pattern in `DesignStreamTests.cpp`.

Required scenarios:

- Feature flag disabled, missing design/step/layer, empty layer, one record, exact batch boundary, boundary ±1, final partial batch.
- Full flattened content/order equality with direct model records and with the unbatched stream.
- Cancellation before iteration, during a full batch, and before the final batch.
- Writer failure/truncation and exception-to-`INTERNAL` mapping.
- A deliberate contract decision on writer failure: current feature-stream paths can return OK after write failure, unlike `GetDesignStream` error propagation. Do not encode truncated success as the expected contract without approval.
- Real-stub health: empty/exact service → `SERVING`; unknown nonempty service → documented `NOT_SERVING` behavior.

Null-writer early-return tests are supplemental; they are not successful-stream evidence.

### 5.4 P0/P1: specification-driven parser coverage

Use small synthetic fixtures for record/branch coverage and a pinned real-design corpus for integration/golden assurance. Derive required and optional behavior from the ODB++ specification, not from one sample archive. The offline spec is identified in project instructions at `../OdbDesign-drc/docs/odb_spec_user.pdf`.

| Area | Required additions |
|---|---|
| **StepHdrFile** | Every scalar attribute, case variants, ONLINE values, complete STEP-REPEAT records, optional fields, multiple repeats, brace forms, unknown keys, invalid booleans/numbers, reused-object behavior, full protobuf round trip including optional absence. Existing serialization tests intentionally omit STEP-REPEAT. |
| **FeaturesFile** | Alternate filenames, units/header forms, line/pad/text/arc/surface records, positive/negative polarity, orientation, symbol declarations/resizing, islands/holes, polygon segments/arcs, optional IDs, truncated/malformed records, scope mismatches, deep serialization equality. Barcode parsing is a stub; define its unsupported/supported contract before claiming correctness. |
| **EdaDataFile** | HDR/UNITS/LYR, all PRP scopes, net/subnet variants, plane fill and outline/rectangle/octagon/exact cutouts, blind pins, electrical/mount/type enums, FID records, packages/pins/outlines, feature IDs/groups, object finalization, optional fields, malformed/orphan records, deep round trips. |
| **ComponentsFile** | CMP/TOP/PRP and BOM transitions, vendor/manufacturer/description fields, optional IDs/net/subnet handling, top/bottom/non-component layers, `.comp_height`, malformed children without active records, lookup APIs. |
| **MatrixFile** | Extend existing suite across all step/layer fields, enums, optional blanks, colors, case/brace variants, malformed/nested/unclosed records, and genuine save/reparse only where the writer is real. |
| **NetlistFile** | Optimize/staggered variants, units/comments, net points, integer boundaries, malformed files, empty optional characters, complete protobuf round trip. Test the suspicious `epoint` empty/nonempty guard at `NetlistFile.cpp:433-436` behaviorally. |
| **MiscInfoFile / StandardFontsFile** | All recognized fields, timestamps/dates, unknown/optional values, malformed blocks/characters/coordinates, getters, serialization, real save/reparse where supported. |
| **AttrListFile / LayerDirectory / SymbolsDirectory** | Alternate names, absent optional files, non-directory paths, child failure propagation, malformed records, directory creation failures. |
| **FileArchive** | Missing/corrupt/unsupported archives, direct/nested roots, required/optional directory absence, step failure propagation, full-versus-skeleton model contents, reviewed real-corpus golden comparisons. |

### 5.5 Cache, spool, product-model, and concurrency residuals

- **DesignCache:** bulk load/save methods, mixed valid/missing/corrupt sets, `stopOnError`, thread creation/cleanup failures, unknown background exceptions, throwing observers, commit-after-eviction/Clear, generation races, lazy serialization failures, budget changes during serialization.
- **StreamSpoolCache:** malformed/truncated/oversized varints, zero lengths, truncated payloads, invalid protobuf, writer move/commit/abort/double-commit, flush failure, stale generation, LRU eviction, exact write/read-back.
- **Design/ProductModel:** null/missing steps/components/pins/nets, exact relationships, lookup misses, clipped/unclipped serialization, normalized-list flavors, `Via` defaults/name/side/protobuf.
- **ComponentHeightTracer:** traced/untracked components, failure limits, populated/missing heights, end-to-end propagation.

Use barriers, condition variables, observer hooks, and bounded waits to make interleavings deterministic. Fixed sleeps and “machine was too fast, so skip” do not provide stable release evidence.

### 5.6 Utilities, application base, CLI, and lifecycle

| Area | Behavioral additions |
|---|---|
| `CommandLineArgs` / `OdbDesignArgs` | Missing/repeated flags, negative/non-numeric values, booleans, absolute paths, every server option, invalid ports/ranges, help/usage, cache/background/design/template directory propagation. |
| `UrlEncoding` | Reserved characters, `%`, `+`, upper/lowercase hex, non-ASCII bytes, malformed/truncated escapes, NUL policy, independent expected outputs plus actual HTTP routes. |
| `str_utils` / `EnumMap` / `bin2ascii` | Empty/all-whitespace/high-bit strings, copy-versus-in-place behavior, unknown names/out-of-range enums, binary/hex/Base64 validity and round trips. |
| `FileReader` / `timestamp` | Missing/unopenable/empty files, repeated reads/Clear, milliseconds/pre-epoch/invalid timestamps and timezone independence. |
| `ThreadSafeQueue` / `WorkQueueLoopThread` | Push/wake, timeout, multiple waiters, interrupt, repeated start/stop, processor exception, draining, destruction while running. |
| `Logger` | Real start/log/drain/stop, filtering, stdout/stderr/file selection, file read-back, repeated lifecycle and concurrency. |
| `OdbAppBase` / server app/base/main | `--load-design`, `--load-all`, configuration fallback/rejection, failed bind, actual message-limit behavior, readiness, graceful shutdown, signals, exit codes. |
| `OdbDesignApp` | Valid design parse, missing environment, missing/corrupt archive, actual exit code and produced model results. |

### 5.7 Test-architecture and quality problems

1. **Tests do not link the production server composition.** Consolidate on `OdbDesignServerLib`; remove duplicate server `.cpp` and generated gRPC sources from the test target. Use the `linux-dynamic-debug` preset for canonical server coverage and retain a `linux-dynamic-release` smoke gate.
2. **Generic integration tests overstate signal.** `IntegrationTests.cpp:72-127` checks file existence/size and sleeps; `:270-351` calls a local simulated pipeline; `:201-226` never feeds corrupt/empty files to OdbDesign. Replace these with real archive → parse → model → protobuf/API assertions.
3. **Generic performance tests are not customer benchmarks.** `PerformanceTests.cpp` primarily measures STL/filesystem/thread helpers. Keep them as microbenchmarks if useful, but measure real parse/REST/gRPC/stream workflows separately.
4. **Some round trips are shallow.** FileArchive/Design protobuf tests compare only a few top-level fields. Add deep field/record comparisons, explicit optional presence/absence, malformed wire/JSON handling, and wire-size assertions where relevant.
5. **PCH can hide include dependencies.** Preserve the parser-only opt-out and add clean public-header consumer tests.
6. **Fixture copying is expensive.** Reserve full archives for contract/integration lanes; use small synthetic fixtures for new grammar/unit tests.
7. **Remote CMake maintenance trap.** `OdbDesignTests/CMakeLists_Remote.txt` references nonexistent sources and is not active; remove or repair it separately so it cannot be mistaken for supported infrastructure.

### 5.8 Do not chase these lines without a contract decision

Several `Save` implementations return success without writing (`EdaDataFile.cpp:384-387`, `FeaturesFile.cpp:958-961`, `ComponentsFile.cpp:155-158`, `StepHdrFile.cpp:355-358`, `StandardFontsFile.cpp:314-317`); `NetlistFile.cpp:459-462` returns false. `FileArchive::SaveFileModel` nevertheless composes writers.

A successful save test must produce a real archive/file and parse it back field-by-field. Calling a no-op `return true` is coverage gaming, not validation. Decide which writer APIs are supported, implement/remove/deprecate them deliberately, then test the chosen contract.

Likewise, disabled legacy connectivity helpers at `Design.cpp:814-901` should not outrank live parser/API paths. Confirm whether they are supported public API or dead code before investing in their coverage.

---

## 6. Target metrics and coverage arithmetic

### 6.1 Canonical metric

The primary gate is **handwritten production source in both repositories**, with each upstream source counted once:

- Include `OdbDesignLib`, `OdbDesignServer`, `OdbDesignApp`, and `Utils` handwritten source.
- Include commercial `LicenseGuard` and `commercial` source in a separately gated commercial report.
- Exclude test code, third-party dependencies, `_deps`, vcpkg, system code, and build-tree generated protobuf/gRPC from the handwritten denominator.
- Publish generated and combined reports separately for audit; never let generated code improve the handwritten score.
- Include never-executed production translation units through an initial `.gcno` capture merged with runtime `.gcda` coverage.
- Inventory every production `.cpp`; an inert/no-executable-line file needs an explanation, not silent omission.

### 6.2 Provisional arithmetic

With the current represented handwritten denominator unchanged:

- Total lines: 8,320.
- Covered lines: 5,877.
- Strictly greater than 90% requires at least **7,489** covered lines.
- Additional covered lines required: **1,612**.
- Current uncovered lines: 2,443, so approximately **66.0% of currently missed lines** must become covered.

Once the initial baseline adds `U` previously unrepresented instrumentable handwritten lines:

```text
corrected denominator D = 8,320 + U
current covered baseline C0 = 5,877
minimum additional covered lines Amin(U)
  = floor(0.90 × (8,320 + U)) + 1 - 5,877
  = 1,612 + floor(0.90 × U)
```

Examples:

| Newly represented instrumentable lines `U` | Additional covered lines needed for >90% |
|---:|---:|
| 0 | 1,612 |
| 1,000 | 2,512 |
| 2,904, purely illustrative and not an instrumentable-line estimate | 4,145 |

Do not add the 2,904 physical lines to LCOV totals. Measure `U` in M0 and recalculate after every production correction.

### 6.3 Required thresholds

Use raw integer counts, not rounded display percentages. Engineering should aim for **91%** to avoid landing on a rounding boundary.

| Scope | Lines | Functions | Branches |
|---|---:|---:|---:|
| Upstream handwritten production overall | **>90.00%** | **>90.00%** | **≥85.00%** |
| Commercial handwritten production overall | **>90.00%** | **>90.00%** | **≥85.00%** after baseline instrumentation |
| Critical parsers, services, controllers, cache, spool, `FileArchive`, `Design` | ≥90% | ≥90% | ≥85% |
| `BasicRequestAuthentication`, `UrlEncoding`, `RouteController`, LicenseGuard | ≥95% | ≥95% | ≥90% |
| Server/CLI process lifecycle | ≥85% | ≥85% | ≥80% |
| Generated protobuf/gRPC | Report separately; no target | Report separately | Report separately |

Additional anti-gaming gates:

- Zero failed, disabled, skipped, or not-run tests in the required correctness/coverage manifest. Optional performance/corpus lanes have separate explicit manifests.
- Every tracked production `.cpp` appears in the initial inventory.
- A file with no branch records is `N/A`, not 100% branch coverage; missing aggregate branch data fails the job.
- Any remaining file below 80% line/function coverage requires a named residual behavior and approved issue; no silent source exclusion.
- New/modified code should meet at least 90% patch line/branch coverage.
- No new `LCOV_EXCL*`/`GCOV_EXCL*` markers or broadened handwritten-path filters to pass the target.
- No deleting live code, invoking empty destructors/getters/no-op writers, or adding simulated workflows merely to raise the score.
- No assertion whose sole proof is string presence in source/config/logs. Exercise real behavior, read back real artifacts, compare real payloads, and check real exit codes.
- Add targeted mutation/negative-control checks for auth, batching, ETag invalidation, parser tokens, cache invalidation, spool truncation, stream reassembly, and the result aggregator. The tests must fail when the behavior is deliberately broken.

---

## 7. Roadmap to greater than 90%

### 7.1 Dependency overview

```text
M0: Truthful measurement and inventory
 ├──> M1: Server linkage and deterministic suite hygiene
 │     └──> M2: REST/gRPC/security/server lifecycle
 │           ├──> M4: Cache/spool/product-model resilience
 │           └──> M5: Utilities/application/CLI residuals
 ├──> M3: Specification-driven parser suites ──> M4
 └──> M5 utility work may start independently

M2 + M3 + M4 + M5 ──> M6: Residual closure and release gate
```

### M0 — Establish truthful, reproducible measurement

**Priority:** P0  
**Estimated effort:** 3–4 person-days  
**Dependencies:** none

**Changes:**

- Make `scripts/coverage.sh` and `.github/workflows/code-coverage.yml` use the same canonical pipeline.
- Prefer a clean `linux-dynamic-debug` coverage build to share the protobuf/gRPC runtime across server, library, and tests; retain dynamic-release behavior smoke.
- Capture `initial.info` from `.gcno` before tests, `runtime.info` after tests, then merge with branch collection.
- Produce disjoint handwritten, generated, and combined tracefiles/reports.
- Pin `OdbDesignTestData` to an exact commit and record source/test-data/compiler/lcov versions.
- Add a raw-LCOV/JUnit checker, proposed as `scripts/check_coverage.py`, for integer thresholds, inventory completeness, missing branch data, and unexpected test outcomes.
- Measure and publish corrected upstream denominator, commercial denominator when C0 lands, and exact remaining-line requirement.

**Exit criteria:**

- The nine omitted substantive public files have zero-hit or actual-hit records; no production `.cpp` is silently absent.
- Line/function/branch raw counts are available per file.
- Handwritten + generated scopes reconcile to the combined report.
- Exactly 90.00% fails a strict `>90%` gate.
- Missing required source, missing branch data, missing test data, skipped required test, or failed test fails the job.
- A clean build passes without broad mismatch suppression hiding unexplained inconsistencies.

### M1 — Unify server linkage and make the suite deterministic

**Priority:** P0  
**Estimated effort:** 3–5 person-days  
**Dependencies:** M0

**Changes:**

- Link server tests against `OdbDesignServerLib`; remove manually compiled server sources and generated gRPC code from `OdbDesignTests/CMakeLists.txt`.
- Consider separate library/server test executables if it improves isolation, but ensure there is one owner for production sources/codegen and no duplicate descriptor pools.
- Re-enable the five `GrpcBatchStreamTests.cpp` early-error/config tests; add real transport success tests in M2.
- Extract reusable real loopback gRPC setup without weakening existing `DesignStreamTests` equivalence assertions.
- Move opt-in benchmarks/memory tests into explicit labels and workflows.
- Replace timing-dependent skips with bounded deterministic observer/barrier seams.
- Require required test archives; absence fails the relevant lane instead of silently skipping.
- Reconcile discovered/executed/passed/skipped/disabled counts programmatically.

**Exit criteria:**

- Existing tests and re-enabled batch tests pass against the real server library.
- Real dynamic-debug and dynamic-release `GetDesign`/stream calls complete without descriptor-pool aborts.
- Required suite has zero unexpected skips/disabled/not-run entries.
- Coverage does not rise by deleting tests or weakening assertions.

### M2 — Cover customer-facing APIs and lifecycle

**Priority:** P0  
**Estimated effort:** 7–10 person-days  
**Dependencies:** M1

**Proposed suites:**

- `GrpcLayerFeatureStreamTests.cpp`
- expanded `GrpcBatchStreamTests.cpp`
- `GrpcHealthCheckTests.cpp` / `GrpcServiceContractTests.cpp`
- cross-platform external-process `RestApiTests`, `FileUploadTests`, `ServerLifecycleTests`, and `ServerConfigurationTests`

Use real generated gRPC stubs and real HTTP requests against the actual executable. A cross-platform .NET/xUnit process runner is recommended for external acceptance because the planned customer samples are .NET; invoke the same runner from Linux coverage CTest and from commercial package acceptance. C++ GoogleTest remains the component/in-process framework.

**Exit criteria:**

- Every RPC in `service.proto` has at least one real-transport behavioral test.
- Every customer-facing REST route has a real success or specified error scenario; the complete route matrix has no unreviewed omission.
- Both layer-feature streams prove ordered payload equivalence, batching, errors, and cancellation.
- Upload/auth/URL/cache behavior is proven through HTTP and filesystem/model side effects.
- Server startup, configuration, health/readiness, graceful shutdown, and exit codes are exercised in subprocesses.
- Critical controller/service/route/auth floors pass.

### M3 — Build specification-driven parser/file-model suites

**Priority:** P0 for core correctness  
**Estimated effort:** 9–13 person-days  
**Dependencies:** M0; can overlap M1/M2

Add the focused suites enumerated in §5.4, using small synthetic fixtures plus pinned real-corpus goldens. Replace simulated scenarios in `IntegrationTests.cpp` with real archive extraction, parsing, `Design::Build`, serialization, cache/API propagation, and cleanup assertions.

**Exit criteria:**

- Every supported record family has positive, boundary, and malformed-input tests.
- Assertions use independently reviewed expected values, not values recomputed solely by the code under test.
- Parser files meet ≥90% line/function and ≥85% branch floors.
- No no-op writer is used as a coverage shortcut.
- Every defect discovered is fixed or explicitly classified as a release blocker; suspicious behavior is not copied into an expected value without review.

### M4 — Close cache, spool, product-model, and concurrency gaps

**Priority:** P1; elevate serving-correctness defects to P0  
**Estimated effort:** 5–7 person-days  
**Dependencies:** M1 and M3

Implement §5.5 scenarios in focused `DesignCacheContractTests`, `StreamSpoolCacheTests`, `DesignBuildTests`, and `ComponentHeightTracerTests` or existing suites where cohesion is better.

**Exit criteria:**

- `DesignCache`, `StreamSpoolCache`, and `Design` meet critical floors.
- Asynchronous tests use deterministic observation and bounded teardown.
- Spool corruption/invalidation is proven by actual read-back behavior.
- Dynamic-debug, dynamic-release, and ASan critical runs show no stale response, hang, leak, or sanitizer finding.
- Protobuf maps are accessed by iteration only, per repository policy.

### M5 — Utilities, application base, CLI, and residual lifecycle

**Priority:** P1  
**Estimated effort:** 4–6 person-days  
**Dependencies:** M0; server residuals depend on M2

Implement §5.6 scenarios using real files, actual output, subprocess exit statuses, and actual network/configuration behavior.

**Exit criteria:**

- Security utilities meet their higher floors; remaining utilities contribute their planned gap closure.
- CLI/server tests prove real parsing/startup and documented exit codes.
- Any suspicious behavior exposed by tests has a correctness decision and tracked resolution.
- Utility tests pass ASan/UBSan scope without findings.

### M6 — Residual closure and upstream release gate

**Priority:** release gate  
**Estimated effort:** 4–6 person-days  
**Dependencies:** M2–M5

**Changes:**

- Regenerate the uncovered line/function/branch queue by file and customer risk.
- Map every residual to a concrete behavior or approved unsupported/dead-code work item.
- Recalculate thresholds after source fixes.
- Run targeted mutation/negative-control checks and three consecutive coverage runs on the same SHA.
- Publish the coverage and behavioral evidence bundle, including residual uncovered lines and reasons.

**Exit criteria:**

- Upstream handwritten lines/functions are strictly >90%; branches ≥85%; all component floors pass.
- No required test is skipped/disabled/not-run.
- Three consecutive same-SHA coverage runs have identical denominators and no flaky test result.
- Linux dynamic-release, Windows, macOS, and ASan/UBSan critical suites pass.
- No target is met through generated code, exclusions, no-op calls, deleted tests, or weakened assertions.

**Upstream total estimate:** **35–51 person-days**, excluding major writer implementation, production redesign, commercial packaging repairs, and external-client/customer delays. Re-estimate after M0.

---

## 8. Commercial-overlay coverage and delivery plan

### 8.1 Relationship and ownership boundary

`OdbDesign-Commercial` is a superbuild overlay, not a fork:

- Root `CMakeLists.txt:19-33` adds the pinned upstream submodule, `LicenseGuard`, and `commercial`.
- `commercial/CMakeLists.txt:19-23` links `OdbDesignCommercialServer` against upstream `OdbDesignServerLib`, `LicenseGuard`, and `OdbCommercialWatermark`.
- Parser/REST/gRPC/cache/Utils tests belong upstream and flow through a submodule pin bump.
- License enforcement, commercial entry-point behavior, signer, watermark, packaging, and delivery acceptance belong in the private overlay.
- Reports may show a weighted combined summary only after both independent denominators are known; neither component may hide the other.

### 8.2 Current commercial findings

| Finding | Evidence | Consequence |
|---|---|---|
| No commercial coverage configuration/artifact | No coverage preset, workflow, tracefile, gcov, or HTML found | No commercial percentage or combined percentage can be claimed |
| LicenseGuard tests are standalone-only | `LicenseGuard/CMakeLists.txt:34-49` | Superbuild/package does not automatically run the 13 tests |
| Root test preset is ineffective as a combined gate | Root only adds subdirectories; testing is enabled upstream; no root `CTestTestfile.cmake` in existing build | `ctest --preset linux-dynamic-release` cannot be assumed to run upstream + commercial tests |
| Package workflow runs no tests | `package-commercial.yml:104-182` | Build/sign/upload is not behavioral validation |
| All five package runs failed | Latest run 44007546488 and four predecessors | No currently successful delivery artifact is evidenced |
| Latest run stopped before artifact upload | Linux configure failure; Windows build failure | Root-cause repair is P0; subsequent steps were skipped |
| Manual delivery checklist is not automated | `README.md:110-118` | Manual checkboxes are not repeatable evidence |
| Symbols copy occurs after strip | `commercial/CMakeLists.txt:32-40`; package workflow `:155-159` | The copied file may not be the intended unstripped symbols companion; verify/fix |
| SBOM and CMake package config are unfinished | `README.md:126-131` | Clean `find_package` SDK consumption and delivery SBOM are open |
| Runtime bundling tolerates missing copies | Package workflow uses conditional copies/`|| true` | Clean-machine execution must detect incomplete runtime closure |
| Planned .NET samples are not present yet | `docs/plan/commercialization/keiron-mvp-example-plan.md` upstream (local-only working copy, gitignored per `docs/plan/README.md`); no `samples/` in inspected commercial tree | The planned samples cannot yet serve as external-client evidence |

### 8.3 C0 — Restore packaging and wire commercial CI/coverage

**Priority:** P0  
**Estimated effort:** 2–4 days plus unknown build-failure repairs

- Diagnose and fix the latest Linux configure and Windows build failures from actual logs; do not infer causes from status alone.
- Enable testing at the commercial root; register LicenseGuard tests in the superbuild while retaining standalone mode and avoiding duplicate FetchContent targets.
- Ensure root CTest discovers both upstream and commercial tests with an expected-test manifest.
- Add secret-free build/test CI using development/test keys and synthetic customer IDs.
- Add commercial coverage instrumentation for `LicenseGuard.cpp`, `CommercialServerMain.cpp`, and watermark code; publish separate overlay/upstream/combined reports.
- Run Windows tests, including the `_mkgmtime` branch.
- On every submodule pin bump, run the exact-pin upstream suite plus commercial superbuild/process smoke.

**Exit:** two platforms build, all expected tests execute and pass, commercial coverage has a complete denominator, and a package artifact is produced for subsequent acceptance.

### 8.4 C1 — Complete LicenseGuard and real signer tests

**Priority:** P0  
**Estimated effort:** 3–5 person-days

Existing 13 tests cover the main valid path, tampering, wrong key, expired/future, one missing field, malformed wrapper, bad Base64 characters, one bad date, missing file, embedded fixture, and basic RFC3339 cases. Add:

- Validly signed malformed/non-object payload JSON.
- Absent, empty, and wrong-type variants for every required field.
- Wrong-type `loopback_only`; non-array/non-string `features`; omitted-option defaults.
- Base64 empty/wrong-length/misplaced-padding/noncanonical/whitespace/NUL cases.
- Strict date full-string consumption, invalid calendar dates, leap/day/month/hour boundaries, bad `expires_at`, exact issue/expiry instants, and issued-after-expiry policy.
- Controlled clock seam rather than wall-clock races.
- File read failures and output reset on every failure.
- Explicit edition/feature-entitlement policy; parsing `features[]` is not enforcement.
- Real `tools/make-license.sh` → `LicenseGuard` → server round trips using an ephemeral test key: default loopback, `--no-loopback`, custom product/edition/features, quoted/Unicode text, bad args/dates/key/output failures.

The current RFC3339 parser at `LicenseGuard.cpp:96-115` lacks an explicit full-string-consumption check and delegates calendar normalization to `timegm`/`_mkgmtime`; these are high-value behavioral tests, not reasons to predeclare a defect fixed.

### 8.5 C2 — Prove commercial process and socket behavior

**Priority:** P0  
**Estimated effort:** 3–5 person-days

Run the actual `OdbDesignCommercialServer` from an extracted package in subprocess tests:

| Scenario | Required result |
|---|---|
| Missing license argument/environment | Exit code `1` (`Utils::ExitCode::FailedInit`) within a bounded timeout; neither test port listens |
| Nonexistent/unreadable file | Same fail-before-bind proof |
| Expired, not-yet-valid, tampered, wrong-product, wrong-key, malformed license | Same fail-before-bind proof |
| Valid license via CLI | Real HTTP health and gRPC HealthCheck succeed; graceful shutdown exits 0 |
| Valid license via environment | Same behavior |
| CLI/environment precedence and empty/missing argument values | Documented behavior proven through actual startup/rejection |
| Loopback license with repeated/hostile `--bind 0.0.0.0` | Loopback HTTP/gRPC succeed; listener inventory shows no wildcard/non-loopback listener; non-loopback connection attempts fail |
| Non-loopback/production license | Test only in the explicitly approved network profile |
| Valid license followed by bind/config/startup failure | Correct nonzero exit and cleanup |

For every rejection, assert process exit, actual socket state, and failed connection attempts. Matching an error string is diagnostic only.

### 8.6 C3 — Automate exact-package and supply-chain acceptance

**Priority:** P0  
**Estimated effort:** 4–7 days plus packaging repairs

Test the **downloaded, signed candidate**, not the build-tree executable:

1. Recompute ZIP SHA-256 and compare with the sidecar and release manifest.
2. Verify the detached GPG signature in a fresh keyring; a one-byte-modified copy must fail both checksum and signature verification.
3. Extract safely; compare a signed file manifest; reject missing/extra/absolute/symlink-escape entries.
4. Generate and schema-validate an SPDX SBOM for exact delivery contents; link it to the ZIP digest and scan the exact artifact.
5. Generate/sign/verify provenance naming commercial commit, upstream gitlink, workflow/run, platform, and ZIP digest.
6. On clean Windows and Linux machines with no source/vcpkg tree, extract into a path containing spaces, run the server, and execute real HTTP/gRPC calls.
7. Build and run an out-of-tree C++ SDK consumer using shipped headers/libraries and a real `OdbDesignConfig.cmake`; compare parsed model results.
8. Verify machine-readable watermark fields against the manifest through an actual consumer/interface. `strings | grep` is diagnostic, not the acceptance mechanism.
9. Verify stripped binaries and preserve a genuinely usable unstripped symbols copy **before** stripping; hash the private symbols artifact.
10. Verify third-party notices on both platforms and complete runtime-library closure through actual startup, not successful copy commands.
11. Exercise `evaluation|production` × `watermark_sdk_dll=true|false` as applicable to the delivery matrix.

SBOM/provenance/evidence should be separate files linked by digest to avoid circular package hashes. Never repack a tested archive.

### 8.7 C4 — Validate shipped protos and .NET samples/clients

**Priority:** P0 for every delivered client/sample

The canonical `service.proto` SHA-256 matched both vendored clients when inspected, but both clients differed from canonical `layerdirectory.proto`, `matrixfile.proto`, and `toolsfile.proto`. Differences include:

- Canonical layer tools at field 7 with field 6 reserved versus client `toolFile` at field 6.
- Canonical matrix `RgbColor` at field 18 with legacy field 16 reserved versus client `Color` at field 16.
- A substantially different `ToolsFile` message/field layout.

These are substantive wire-contract differences, not formatting drift. If either existing client is included in the evaluation, deliberate reconciliation and real RPC round trips are release blockers. Do not hide them with a diff allowlist.

For the planned fresh .NET/Blazor samples:

- Implement them in the private commercial repo as planned, or update the delivery promise explicitly.
- Copy the **complete canonical model + gRPC proto graph from the exact eval artifact/source pin**, not only `service.proto`.
- Generate C# at build time using `Grpc.Tools`; record all proto hashes.
- Build from a clean machine and execute Quickstart, Upload, Streaming, and Blazor workflows as applicable.
- Prove monolithic-versus-stream semantic/byte equivalence, upload/read-back, load states, meaningful model values, cancellation, retry, and restart.
- Add `buf breaking` or descriptor comparison against the last released supported contract, with explicit review of intentional breaks.
- If the existing 3D/info clients are not delivered, their certification may be later work; their current drift remains evidence of why exact shipped-proto validation is mandatory.

### 8.8 Network-boundary decision

The commercial evaluation license defaults to loopback-only. This is the recommended safest evaluation profile. If the customer runs the client and server on the same host, require and prove that restriction.

If the intended evaluation uses a separate client machine, Docker network, or LAN host:

- Do not silently issue a non-loopback license or treat plaintext unauthenticated gRPC as production-safe.
- Record the isolated-network boundary and obtain explicit security/product approval.
- Remote/non-isolated access makes gRPC TLS, authentication/authorization, secret handling, and remote security testing evaluation blockers.
- Fix F004 and require unique non-default REST credentials in every profile.

---

## 9. Expanded testing and validation types

### 9.1 Test portfolio

| Test type | Current state | Required addition | Cadence / release role |
|---|---|---|---|
| **Unit/component** | Broad parser/cache/util tests, uneven depth | Small synthetic grammar, utility, model, cache/spool, license boundaries; independent expected values | Every PR; feeds >90% gate |
| **Native integration** | Real archive → parser → cache/model and in-process gRPC | Replace simulated integration; full parser → model → protobuf/API and cleanup paths | Every PR for small corpus; release for full corpus |
| **REST contract** | Helper/manual caching tests only | Real process HTTP matrix; status/schema/headers/side effects; negative/auth/upload cases; OpenAPI conformance | Every PR for core; every release for full matrix |
| **gRPC contract** | Strong `GetDesignStream`; other methods mixed direct/transport | Real-stub coverage for every RPC; both feature streams; statuses, ordering, equivalence, cancellation | Every PR; release gate |
| **Cross-language contract** | Vendored clients drift; planned samples not implemented | Exact shipped proto bundle, hash manifest, generated C#, real .NET client/sample calls | Every package/release |
| **External-process E2E** | No repeatable complete upload-to-restart path | Upload → load → poll → REST/gRPC → stream/reassemble → cancel/retry → restart/reread | Every delivery candidate |
| **Golden/differential corpus** | Two main real designs plus local/opt-in goldens | Versioned manifest, writer-generated variants, malformed corpus, customer-approved goldens, previous-version differential | PR small; nightly/release expanded |
| **Install/SDK consumer** | Install rules and parser-only header compile | Clean VM, path with spaces, no source/vcpkg, `find_package`, compile/link/run, runtime dependency closure | Every package |
| **Container/image** | Publish/sign but never start published image | Pull exact digest, verify signature/revision, run non-root, real HTTP/RPC, restart/persistence | Every image/release candidate |
| **Deployment/Kubernetes** | Manual deploy and gRPC validator; validator can succeed when skipped/tool missing | Ephemeral k3d/k3s deployment, rollout, external calls, PVC restart; fail closed on missing tooling | Every deployment release |
| **Persistence/upgrade/rollback** | No automated matrix | Eval: same-version process/container/pod restart. Production: N−1 upgrade, rollback, backup/restore, cache/spool compatibility | Restart before eval; version migration before production rollout |
| **Security regression** | Strong archive library tests; F001–F003 remediated; remaining risks | Real HTTP hostile uploads/auth, F004 fail-closed, license/socket matrix, exact artifact scan, negative controls | Every PR/core and delivery/full |
| **Fuzzing/property testing** | No sustained fuzz lane | ASan-enabled libFuzzer for parsers, archive extraction, URL/multipart, license docs; Schemathesis supplemental REST properties | Short PR smoke; nightly/weekly longer campaigns |
| **ASan/UBSan** | Existing `linux-asan` preset, not comprehensive gate | Critical parser/archive/cache/API/corpus runs; halt on error; document third-party instrumentation scope | Every release; broader nightly |
| **TSan/concurrency** | Behavioral concurrency tests, no TSan gate | Dedicated TSan build for cache/load/spool/stream/cancellation; deterministic multi-client tests | Nightly/release; zero races before production |
| **Performance/benchmark** | Strong opt-in stream memory tests; generic STL benchmarks elsewhere | Real parse/REST/gRPC/stream metrics; customer resource envelope; cold/warm latency, RSS, wire bytes, spool growth | Required named scenarios before eval |
| **Load/stress** | No automated multi-client load | k6 for REST, ghz for unary, custom .NET for streaming correctness/backpressure; expected-load and 2× stress | Modest concurrency before eval; full before production |
| **Soak/leak** | None found | Multi-hour/24-hour upload/load/evict/stream/cancel cycles; RSS/disk/file-descriptor trend limits | Production hardening |
| **Failure injection/chaos** | Corrupt spool/unusable directory local coverage | Process kill, disk full/read-only, network delay/reset, OOM, pod eviction, starvation, stale cache | Limited smoke before eval; full production gate |
| **Compatibility/cross-platform** | Public Windows/Linux/macOS CI; commercial Windows/Linux packaging | Exact commercial ZIP acceptance on Windows/Linux; public macOS retained; explicit support matrix | Every release |
| **Mutation testing** | None found | Targeted mutation/negative controls for critical behavior; evaluate fuller C++ mutation tooling later | Critical release checks; broader later |
| **Observability/support** | CDash/platform monitoring; static app health | Structured scenario results, support diagnostic bundle; production request/load/error/RSS/stream metrics and alert tests | Evidence bundle before eval; operational metrics before production |
| **Usability/customer workflow** | Historical/manual live checks | Customer executes agreed .NET/Blazor job-prep workflow after automated gates; capture acceptance | Final eval signoff |

### 9.2 Integration tests that count as integration

A valid integration scenario must cross a real production boundary and assert a real outcome. Examples:

- Archive bytes → safe extraction → parsed file model → product model → protobuf wire → independent client parse.
- HTTP upload → exact stored bytes → load-state API → gRPC stream → client reassembly → known model invariants.
- Archive replacement → cache generation change → HTTP ETag change → fresh response.
- Spool write → process restart/read → identical chunk sequence.
- License file → signature verification → process startup → actual listener address and health RPC.

Filesystem existence alone, a log message, a route string, a mocked return, or a helper simulation is not integration acceptance.

### 9.3 Security and fuzzing scope

Prioritize attacker-controlled and customer-controlled boundaries:

- Outer and nested archives, traversal/symlinks, decompression/resource limits.
- Multipart parsing, filenames, URL decoding, JSON bodies, auth headers.
- Individual ODB++ parser records and malformed/truncated input.
- License wrapper/payload/Base64/date handling.
- Protobuf/JSON semantic handlers, rather than claiming coverage of generated protobuf internals.

Use bounded input sizes/timeouts and ASan/UBSan. Preserve minimized reproducers and add every corrected crash to the regression corpus. Long-running fuzzing and remote protocol fuzzing are production-hardening requirements; short smoke does not justify an “exhaustively secure” claim.

### 9.4 Performance, capacity, and soak

For evaluation, reuse the existing named large-design budgets where appropriate and record a customer-approved resource envelope before testing a different design/hardware target. The 6 GiB/1 GiB limits are not universal guarantees for arbitrary jobs.

Record:

- Archive SHA-256/size, parser/writer version, selected steps/layers.
- Cold parse and warm request latency; p50/p95/p99 where enough samples exist.
- Monolithic rejection and stream success behavior.
- Wire bytes, chunk/feature counts, cancellation latency.
- Current/peak RSS, spool bytes, disk growth, restart count.

Until multi-design resident-memory governance is complete, constrain and document the eval to serial large-design workflows or a demonstrated safe resident set. Do not claim concurrent large-design capacity from small-design tests.

---

## 10. First-customer evaluation acceptance protocol

### 10.1 Immutable release manifest

Create one private machine-readable manifest, proposed as `eval-plan.json`, containing:

- Release identifier and timestamps.
- Commercial commit, upstream gitlink, test-harness commit, and every delivered client/sample commit.
- Exact ZIP SHA-256 values, image digest if applicable, and artifact names.
- Complete proto hashes and OpenAPI source hash.
- Supported OS/runtime/hardware matrix and network boundary.
- Corpus aliases, SHA-256 values, sizes, expected golden versions, and privacy policy.
- Memory/disk limits, operation timeouts, and performance acceptance envelope.
- Required scenario IDs and expected counts per platform.
- Public signing-key fingerprints; never private keys/passwords/reusable license documents.

Any binary, dependency, proto, package-content, image, license-verification, or watermark change creates a new candidate and reruns the full applicable gate. A harness-only correction may reuse the artifact digest if the new harness revision is recorded.

### 10.2 Universal pass rules

- Every required scenario ID must return structured `Passed`; missing, `Skipped`, `NotRun`, missing tool, missing corpus, missing artifact, missing credential, or early-return success fails the required lane.
- Platform `NotApplicable` requires a preapproved support-matrix entry, not a run-time decision.
- Check both runner exit code and structured JUnit/TRX/JSON result counts.
- Logs diagnose behavior; they do not prove it.
- Source/OpenAPI/proto comparisons detect drift but do not replace runtime tests.
- Include aggregator negative controls: a skip, missing tool, changed digest, or failed RPC must make the overall verdict fail even if later health checks pass.

### 10.3 Required scenario matrix

| ID group | Scenario and acceptance evidence | Eval gate |
|---|---|---|
| `SRC-*` | All expected native tests execute; upstream/commercial >90% lines/functions and branch/component floors; no unexpected skips; three OS public release suites; commercial Windows/Linux tests | Required |
| `SEC-*` | F004 fixed; missing enabled-auth config exits before listeners; unique credentials produce actual unauthorized/authorized responses; hostile archive/upload regressions; exact artifact scan policy | Required |
| `PKG-*` | Package workflow succeeds; downloaded ZIP checksum/GPG/manifest/SBOM/provenance verify; tampered copy fails; clean-machine runtime closure | Required |
| `LIC-*` | Missing/invalid/expired/future/tampered/wrong-product/wrong-key licenses exit `FailedInit` before either socket; valid CLI/env licenses start and shut down correctly | Required for commercial delivery |
| `LOOP-*` | Hostile/repeated bind args still produce only loopback REST/gRPC listeners; non-loopback connections fail | Required for loopback eval |
| `SDK-*` | Out-of-tree consumer configures/builds/runs with no source/vcpkg; exact parsed facts match; watermarks and runtime dependencies verified | Required if SDK is delivered |
| `PROTO-*` | Complete shipped proto graph hashes match candidate; C# generation succeeds; intentional compatibility breaks reviewed | Required |
| `REST-*` | Every documented operation exercised; schema/status/header/body assertions; both uploads; malformed/duplicate/hostile inputs; conditional GET | Required |
| `GRPC-*` | Every RPC exercised through real transport; errors and feature flags; monolithic and both feature/design stream contracts | Required |
| `DOTNET-*` | Planned shipped samples and any delivered existing client execute against the packaged server; meaningful model values; stream equivalence | Required for delivered clients |
| `E2E-*` | Upload → load/poll → read → monolithic/stream → cancel → retry → graceful restart → reread, all from external process | Required |
| `CORP-*` | Public small/medium corpus plus representative and largest in-scope customer designs; reviewed expected facts; no crash/hang/empty-silent model | Required; missing customer data blocks customer acceptance |
| `PERF-*` | Existing named large-design memory scenarios plus customer job within preapproved memory/time envelope; over-budget monolith cleanly rejects and stream succeeds | Required |
| `CONC-*` | Modest concurrent small reads, cold/warm requests, duplicate upload conflict, cancellation followed by healthy request; no stale/corrupt response | Required at documented eval scope |
| `RESTART-*` | Uploaded design persists across same-version process/container/pod restart as applicable; archive hash and semantic model results match | Required for promised deployment forms |
| `IMAGE-*` | Exact digest pulled, signature/revision verified, non-root process, real HTTP/gRPC; only passing digest receives mutable tag | Required if image delivered/deployed |
| `K8S-*` | Ephemeral deployment rolls out, external HTTP/gRPC calls pass, PVC restart preserves data; missing grpcurl/tooling fails validation | Required if Kubernetes is in eval scope |
| `DOC-*` | Documented install/run/upload/stream/signature/support commands execute successfully on clean machines | Required |
| `EVID-*` | Complete machine-generated result set matches manifest and exact artifact identities; negative controls prove missing/failing evidence cannot pass | Required |

### 10.4 Clean-machine day-one rehearsal

1. **Freeze identity.** Record all commits, artifact digests, client/sample versions, corpus hashes, network boundary, resource envelope, and scenario manifest.
2. **Verify delivery.** Download the exact candidate; verify checksum, signature, file manifest, SBOM/provenance, and notices.
3. **Install clean.** Extract on clean Windows/Linux as applicable, in a path containing spaces, without source or vcpkg. If Docker is delivered, pull by digest on a clean host.
4. **Prove license rejection.** Run every invalid-license case; assert exit 1, no listeners, and no successful HTTP/gRPC connection.
5. **Prove valid startup and boundary.** Start with the valid eval license and unique REST credentials; prove real health, auth decisions, and loopback behavior where applicable.
6. **Run public-corpus workflow.** Upload both supported ways, compare stored archive hash, request load, observe states, read all customer-relevant REST/gRPC data, check ETags, monolithic/stream equivalence, and error cases.
7. **Run external .NET workflow.** Build solely from shipped protos/samples; execute the actual promised quickstart/upload/streaming/UI scenarios.
8. **Run customer-corpus workflow on approved private infrastructure.** Validate representative and largest jobs against customer-approved facts and resource/time limits. Keep proprietary archives and sensitive model payloads out of general evidence.
9. **Cancel and recover.** Cancel a real multi-chunk stream after data arrives; verify prompt termination, server health, cleanup/bounded spool, and successful immediate retry.
10. **Restart and reread.** Restart the same process/container/pod using the same designs directory/PVC; require cold reads/streams without another upload and compare hash/model facts.
11. **Run deployment acceptance if in scope.** Exact image digest, rollout, real external protocols, and required-tool fail-closed behavior.
12. **Generate evidence and human signoff.** Machine aggregate all expected results; release and customer-evaluation owners review limitations and sign the same candidate.

### 10.5 Explicit evaluation limitations

Unless additional gates are completed, the evaluation statement must say:

- It certifies only the listed artifacts, commits, operating systems, client/sample versions, and resource envelope.
- Loopback evaluation does not certify remote plaintext gRPC or production network security.
- Serial large-design success does not certify concurrent multi-large-design residency.
- Same-version restart persistence does not certify N−1 upgrade/rollback or backup/restore.
- Short sanitizer/fuzz/concurrency runs do not prove absence of all memory errors, races, or vulnerabilities.
- Public macOS CI does not certify a commercial macOS package.
- Minimal Kubernetes acceptance does not certify HA, GitOps, certificate rotation, NetworkPolicy, or production security.
- Historical manual live tests do not replace execution of the current required scenario manifest.

---

## 11. CI lanes and cadence

| Lane | Contents | Trigger | Failure policy |
|---|---|---|---|
| **PR fast correctness** | Small unit/component/parser suites, real loopback gRPC, core real HTTP/auth/upload, small goldens, archive/security regressions, patch coverage | Every PR | Any failure/unexpected skip blocks |
| **Full coverage** | Clean dynamic-debug instrumented build, initial+runtime capture, all required correctness tests, line/function/branch/inventory gates | PR and release branches; required when stable | >90% and all component floors block once baseline is accepted |
| **Native release matrix** | Windows, Ubuntu dynamic-release, macOS; real expected-test manifests | PR/release as existing CI supports | All supported legs required |
| **Commercial source gate** | Superbuild, upstream exact-pin tests, LicenseGuard, commercial process/socket suite, commercial coverage | Commercial PR/pin bump/package | All expected tests and both platforms required |
| **Exact-package acceptance** | Signed downloaded ZIP, clean VM, license, socket, SDK, proto, .NET, E2E, restart, evidence | Every delivery candidate | No artifact delivery until pass |
| **Container/deployment** | Digest-pinned image run, external protocols, ephemeral k3d/k3s, PVC restart | Image/deployment candidate | Missing tool/scenario fails |
| **Performance/corpus release** | Named large designs, customer-approved corpus, memory/time envelope, modest concurrency | Release/eval on suitable private hardware | Missing corpus or budget breach blocks customer acceptance |
| **Nightly hardening** | ASan/UBSan broader corpus, TSan, longer fuzz, expanded goldens, load/stress, repeated concurrency | Nightly | Triage; production gate for agreed scope |
| **Weekly/soak** | Extended fuzz, 24-hour soak, failure injection, upgrade/rollback/backup/restore | Weekly/pre-production | Required before production claims |

Coverage and performance results from different source/artifact identities must not be mixed into one release verdict.

---

## 12. Evidence bundle and reporting

### 12.1 Identity and provenance

- Release ID, timestamps, supported platforms, hardware descriptions.
- Upstream, commercial, test-data, client/sample, harness, and workflow commits.
- ZIP SHA-256 values, image digest, OCI revision/signature result.
- Complete proto/OpenAPI hashes.
- Public signing-key fingerprints and signed provenance.
- Toolchain, CMake preset, compiler, lcov, .NET, and dependency versions.

### 12.2 Test and coverage evidence

- Initial/runtime/handwritten/generated/combined LCOV tracefiles.
- Per-file line/function/branch raw counts and uncovered-line list.
- Source-inventory reconciliation and explicit inert-file list.
- JUnit/TRX/JSON results with discovered/executed/passed/failed/skipped/disabled/not-run counts.
- Three same-SHA coverage run identities and repeatability result.
- Targeted mutation/negative-control outcomes.
- Sanitizer/fuzz/TSan results and reproducer hashes.

### 12.3 Behavioral and operational evidence

- Scenario manifest and result for every required ID.
- Process exit codes, unexpected termination, and graceful shutdown status.
- Active-socket before/after records and loopback/non-loopback connection outcomes.
- Sanitized HTTP method/path/status/schema/cache-header results.
- gRPC method/status/message/chunk/feature counts and cancellation latency.
- Uploaded archive hashes, stream equivalence, golden/semantic comparison results.
- Cold/warm timings, RSS, disk/spool usage, restart/persistence results.
- SDK configure/build/run and clean-machine documentation command results.
- Deployment rollout/restart and external protocol results.

### 12.4 Supply-chain and delivery evidence

- ZIP checksum, detached signature, signed file manifest.
- SPDX SBOM, vulnerability scan, and any explicitly approved time-bounded waiver.
- Third-party notices and private symbols-copy hash.
- Package-specific quickstart/support matrix/security boundary/known limitations.
- Machine-generated overall verdict signed by the release owner.

### 12.5 Privacy and integrity

Do not include private signing keys, REST passwords, decrypted CI secrets, reusable license documents, proprietary archives, or sensitive raw model payloads. Use customer-approved aliases, hashes, metrics, and redacted logs. A hand-edited summary cannot override a missing or failed structured result.

---

## 13. Effort, sequencing, and ownership

### 13.1 Rough planning ranges

| Workstream | Range | Notes |
|---|---:|---|
| Upstream M0–M6 | **35–51 person-days** | Excludes major writer implementation/large production redesign |
| Commercial C0–C3 | **12–21 person-days plus build repairs** | CI/coverage, LicenseGuard, process/socket, package/SDK/supply chain |
| External .NET samples/client and exact-artifact acceptance | **5–10 person-days** | Overlaps M2/C3; excludes customer waiting and newly discovered protocol defects |
| Production hardening | Estimate separately | TLS/authz, residency governance, extended fuzz/TSan, soak/chaos, upgrade/rollback, observability |

These ranges are intentionally not converted to a calendar promise. Parallelism, discovered defects, packaging repair, customer-corpus availability, and the corrected M0 denominator will drive the schedule.

### 13.2 Owner roles

| Role | Accountable areas |
|---|---|
| C++ library owner | Parser/model suites, ODB++ spec interpretation, protobuf behavior |
| Server owner | ServerLib composition, REST/gRPC, cache/spool, lifecycle |
| QA automation owner | Coverage pipeline, external-process runner, manifests, result aggregation, negative controls |
| Commercial maintainer | LicenseGuard, signer, commercial entry point, watermark, pin bumps |
| Build/release engineer | Package repair, clean install/SDK, signing, SBOM/provenance, artifact identity |
| .NET client owner | Proto reconciliation, samples, reassembly, real-server tests |
| Security lead | F004/network boundary, fuzz/security policy, vulnerability waivers, threat model |
| Performance owner | Resource envelope, large-design/customer corpus, load/soak |
| DevOps/SRE | Image digest, Kubernetes/persistence, upgrade/rollback, operational evidence |
| Release manager + customer-evaluation lead | Manifest freeze, scope/limitations, evidence review, final signoff |

### 13.3 Recommended execution order when time is constrained

1. **Immediately:** restore commercial packaging; wire tests; fix F004; establish M0/C0 measurement; freeze delivery/network/artifact scope.
2. **In parallel:** M1/M2 real server/API tests; M3 high-risk parser suites; C1/C2 license/socket tests; shipped proto/.NET sample implementation.
3. **Before numerical closure:** M4/M5 residual behavior; C3 clean-package/SDK acceptance; customer-corpus/performance rehearsal.
4. **Release only after:** M6 >90% gate, exact-artifact E2E, persistence/cancellation, complete evidence, and explicit limitations.
5. **Before production rather than constrained eval:** remote TLS/authz, full residency governance, upgrade/rollback, long soak/chaos/fuzz/TSan, production observability.

“Later” means not required for the **constrained evaluation**, not optional forever.

---

## 14. Decisions and risks requiring ownership

1. **Delivery artifacts:** identify exactly what the customer receives—commercial ZIP(s), public/commercial image, samples ZIP, SDK, or a combination. Test each artifact; success for one does not certify another.
2. **Network topology:** same-host loopback is recommended. Separate-host/LAN eval requires an explicit isolated-network decision; remote/non-isolated plaintext unauthenticated gRPC is a blocker pending TLS/authn/authz.
3. **F004:** known default REST credentials are not an acceptable eval limitation; implement fail-closed startup and real auth tests.
4. **Commercial package health:** all five runs failed. Root-cause the actual failures and produce an artifact before detailed archive acceptance can begin.
5. **Writer APIs:** no-op/always-false `Save` methods need a supported-writer contract. Do not use exclusions or vacuous calls to reach 90%.
6. **Dead/legacy code:** confirm supported public API versus removal/deprecation for disabled connectivity helpers; do not delete live code for denominator gain.
7. **Client scope and proto compatibility:** decide whether the planned fresh samples, 3D client, info client, or a combination are contractual. Every delivered consumer must use the complete exact proto graph and pass live behavior.
8. **Customer corpus and resource envelope:** obtain representative/largest jobs, approved expected facts, hardware limits, timeouts, and privacy constraints. Technical dry runs without customer data are not customer acceptance.
9. **Large-design concurrency:** until M6.6 residency governance lands, constrain/document serial large-design evaluation or implement and prove a safe budget.
10. **Coverage denominator after fixes:** source additions/removals and platform-specific code change totals. Recompute raw gates; never mix SHAs.
11. **Third-party sanitizer scope:** document what is actually instrumented; a clean ASan/TSan run is evidence only for its stated scope.
12. **Package symbols/notices/runtime closure:** preserve symbols before stripping, verify both-platform notices, and let clean-machine execution expose missing bundled dependencies.

---

## 15. Definition of done

### 15.1 Coverage and source quality

- [ ] Canonical clean coverage pipeline includes every handwritten production translation unit.
- [ ] Upstream handwritten line and function coverage are each **strictly >90%**; branches **≥85%**.
- [ ] Commercial handwritten coverage is independently measured and meets the same agreed gate.
- [ ] Generated code is separate and cannot inflate either gate.
- [ ] All critical component/security/lifecycle floors pass.
- [ ] Required tests have zero unexpected skips, disabled tests, or not-run entries.
- [ ] Three consecutive same-SHA coverage runs are green with identical denominators.
- [ ] Public Windows/Linux/macOS and commercial Windows/Linux expected suites pass.
- [ ] ASan/UBSan critical scope has no unexplained findings; TSan/production scope is honestly stated.
- [ ] Targeted mutation/negative controls prove critical tests and the aggregator can fail.

### 15.2 Behavioral confidence

- [ ] Every delivered RPC and REST operation has real-transport behavioral coverage.
- [ ] Both feature-stream variants and design streaming prove content/order equivalence and cancellation behavior.
- [ ] Upload/auth/URL/archive/ETag/cache behavior is proven through actual HTTP and side effects.
- [ ] Real parser/model/protobuf goldens cover supported grammar variants and malformed inputs.
- [ ] External .NET samples/clients build from exact shipped protos and perform meaningful real workflows.
- [ ] Representative and largest in-scope customer designs meet approved semantic and resource criteria.
- [ ] Modest concurrency, cancellation/retry, and same-version restart/persistence pass at the documented scope.

### 15.3 Exact delivery artifact

- [ ] Commercial packaging succeeds on every promised platform.
- [ ] Tested ZIP/image/samples are byte-identical by digest to the delivered artifacts.
- [ ] Missing/invalid/expired/tampered/wrong-product licenses exit `FailedInit` before either listener opens.
- [ ] Valid license starts real HTTP/gRPC; loopback enforcement is proven through sockets and connection attempts.
- [ ] F004 is fixed; no default credential is accepted; enabled auth with missing configuration fails closed.
- [ ] Clean-machine runtime and SDK/documented-command tests pass without source/vcpkg.
- [ ] Checksums, GPG, file manifest, SBOM, provenance, image identity, and tamper negative controls pass.
- [ ] Watermark/stripped/symbols/notices/runtime closure checks pass on actual package contents.
- [ ] Container/Kubernetes acceptance passes where those forms are in scope; missing tools never count as success.
- [ ] The evidence bundle is complete, private, signed, and tied to one immutable release candidate.
- [ ] Evaluation limitations and production-hardening owners are included in delivery and signoff.

If any required behavior is unexecuted or any artifact identity changes, the release verdict remains **not certified**.

---

## 16. Appendix: target-state command outline

These commands describe the intended canonical shape after M0/M1 labels and scripts exist; they are not a claim that all labels/options are already wired.

### 16.1 Existing release build/test baseline

```bash
cmake --preset linux-dynamic-release
cmake --build --preset linux-dynamic-release

ODB_TEST_DATA_DIR=/home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA \
ODB_TEST_ENVIRONMENT_VARIABLE=ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS \
ctest --preset linux-dynamic-release --output-on-failure
```

The equivalent Windows/macOS presets remain required on their native runners. A commercial root `ctest` command is valid evidence only after C0 proves that it discovers the complete upstream + commercial expected-test manifest.

### 16.2 Target coverage pipeline

```bash
cmake --preset linux-dynamic-debug \
  -DCMAKE_CXX_FLAGS="--coverage -g -O0 -fprofile-update=atomic" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
  -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
  -DCMAKE_MODULE_LINKER_FLAGS="--coverage"
cmake --build --preset linux-dynamic-debug

# Before tests: zero-hit inventory from .gcno.
lcov --capture --initial \
  --directory out/build/linux-dynamic-debug \
  --branch-coverage \
  --output-file coverage/initial.info

# Real tests; required labels are introduced by M1.
ctest --preset linux-dynamic-debug \
  -L coverage \
  --output-on-failure \
  --output-junit coverage/test-results.xml

# After tests: actual .gcda execution.
lcov --capture \
  --directory out/build/linux-dynamic-debug \
  --branch-coverage \
  --rc geninfo_unexecuted_blocks=1 \
  --output-file coverage/runtime.info

lcov \
  --add-tracefile coverage/initial.info \
  --add-tracefile coverage/runtime.info \
  --branch-coverage \
  --output-file coverage/combined.info
```

The shared coverage script must then classify source, reconcile inventory, parse raw LCOV/JUnit, enforce integer thresholds, and emit HTML/raw artifacts. Running the tests and checking their real exit/result records is mandatory; generating a coverage file alone is not test verification.

### 16.3 Existing named large-design scenarios

Run one process per scenario as requested by the test comments, on suitable hardware with fixtures present:

```bash
TEST_BIN=out/build/linux-dynamic-release/OdbDesignTests/OdbDesignTests

for TEST_NAME in \
  LargeDesign_200_40628_StreamWithinRssBudget \
  LargeDesign_350_41017_StreamWithinRssBudget \
  SampleDesign_ColdStreamVsMonolithic_Perf
do
  ODB_DESIGN_STREAM_BENCH=1 \
  ODB_TEST_DATA_DIR=/home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA \
  "$TEST_BIN" \
    --gtest_filter="DesignStreamMemoryTest.${TEST_NAME}" \
    --gtest_output="xml:${TEST_NAME}.xml"
done
```

For release evidence, require and inspect each expected XML result. A missing archive or `GTEST_SKIP` is not a pass.

### 16.4 Deployment validator

Retain the existing real RPC validator, but first require `grpcurl` and remove/restrict successful skip paths for required release profiles:

```powershell
Get-Command grpcurl -ErrorAction Stop

pwsh scripts/validate-grpc-exposure.ps1 `
  -DesignName designodb_rigidflex `
  -StepName cellular_flip-phone `
  -LayerName signal_2
```

The required release harness must additionally assert the expected RPC result records rather than relying only on script text or a zero exit after a skip.

---

## 17. Sources

### Coverage and CI

- [Current analyzed public coverage run 44011295347](https://github.com/nam20485/OdbDesign/actions/runs/44011295347)
- [Current analyzed public coverage artifact 14298493942](https://github.com/nam20485/OdbDesign/actions/runs/44011295347/artifacts/14298493942)
- [Commercial-pin-matched public coverage run 43310307317](https://github.com/nam20485/OdbDesign/actions/runs/43310307317)
- `.github/workflows/code-coverage.yml`
- `.github/workflows/cmake-multi-platform.yml`
- `.github/workflows/docker-publish.yml`
- `.github/workflows/docker-scout-scan.yml`
- `CMakePresets.json`
- `scripts/coverage.sh`

### Public production and tests

- `OdbDesignTests/CMakeLists.txt`
- `OdbDesignServer/CMakeLists.txt`
- `OdbDesignTests/Fixtures/FileArchiveLoadFixture.cpp`
- `OdbDesignTests/DesignStreamTests.cpp`
- `OdbDesignTests/DesignStreamMemoryTests.cpp`
- `OdbDesignTests/ResponseCacheTests.cpp`
- `OdbDesignTests/ConnectivityContractTests.cpp`
- `OdbDesignTests/GrpcBatchStreamTests.cpp`
- `OdbDesignTests/ProtobufSerializationTests.cpp`
- `OdbDesignTests/ArchiveSecurityTests.cpp`
- `OdbDesignTests/IntegrationTests.cpp`
- `OdbDesignTests/PerformanceTests.cpp`
- `OdbDesignLib/App/DesignCache.cpp`
- `OdbDesignLib/App/BasicRequestAuthentication.cpp`
- `OdbDesignLib/App/RouteController.cpp`
- `OdbDesignServer/Controllers/`
- `OdbDesignServer/Services/OdbDesignServiceImpl.cpp`
- `OdbDesignServer/Services/StreamSpoolCache.cpp`
- `OdbDesignServer/protoc/grpc/service.proto`
- `OdbDesignLib/protoc/`

### Commercial overlay

- [Private commercial repository](https://github.com/nam20485/OdbDesign-Commercial)
- [Latest commercial package run 44007546488](https://github.com/nam20485/OdbDesign-Commercial/actions/runs/44007546488)
- `OdbDesign-Commercial/README.md`
- `OdbDesign-Commercial/CMakeLists.txt`
- `OdbDesign-Commercial/CMakePresets.json`
- `OdbDesign-Commercial/.github/workflows/package-commercial.yml`
- `OdbDesign-Commercial/LicenseGuard/CMakeLists.txt`
- `OdbDesign-Commercial/LicenseGuard/LicenseGuard.cpp`
- `OdbDesign-Commercial/LicenseGuard/tests/LicenseGuardTests.cpp`
- `OdbDesign-Commercial/commercial/CommercialServerMain.cpp`
- `OdbDesign-Commercial/commercial/CMakeLists.txt`
- `OdbDesign-Commercial/tools/make-license.sh`

### Related plans and incident context

- `docs/plan/getdesign-streaming-oom-plan.md`
- `docs/plan/commercialization/keiron-mvp-example-plan.md` *(local-only working copy, not in clones)*
- `docs/plan/commercialization/keiron-call-summary-2026-11-16.md` *(local-only working copy, not in clones)*
- `docs/plan/comprehensive-security-audit/comprehensive-security-audit.md` *(local-only working copy, not in clones; historical pre-F001–F003-remediation context — finding IDs inlined here for traceability: F001 Critical, shell command injection through nested archive paths in the unquoted 7z invocation; F002 High, authenticated upload filename traversal / arbitrary file overwrite; F003 High, absolute-path archive entries escaping extraction with attacker-controlled modes; F004 High, built-in REST credentials activating when credential environment values are missing or empty)*
- `docs/plan/comprehensive-security-audit/remediation-plan.md` *(local-only working copy, not in clones)*
- `docs/completed/turbot-getdesign-corrupt-protobuf-fix.md`
- `docs/completed/program-status-2026-11-01.md`
- `scripts/validate-grpc-exposure.ps1`
- `deploy/kube/OdbDesignServer/deployment.yaml`

---

**Document status:** proposed plan and baseline analysis. It becomes release evidence only after the referenced commands and scenarios actually execute, produce structured results, and pass against the immutable candidate identified in the evidence manifest.
