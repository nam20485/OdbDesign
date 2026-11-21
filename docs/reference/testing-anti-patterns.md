# Testing Anti-Patterns — BANNED Patterns and Required Replacements

**Status:** Mandatory guardrail (companion to the `AGENTS.md` "No grep-style pseudo-validation" rule)
**Established:** 2026-11-20, after the purge of `IntegrationTests.cpp`, `PerformanceTests.cpp`, `EnhancedTests.cpp`

# 🚫 IF A TEST DOES NOT RUN PRODUCTION CODE AND ASSERT A REAL OUTCOME, IT IS NOT A TEST. DELETE IT!!!!! 🚫

This document catalogs the fake-test patterns that are **BANNED** in this repo, with the exact
code that was removed and the real replacement each requires. Point reviewers and subagents here.

---

## The one rule

**A test must (1) exercise real OdbDesign production code and (2) assert a real, independently
expected outcome.** String presence in a file/config/log is NEVER proof that a runtime behavior
works. Simulated work is NEVER proof the product works. Benchmarking the STL is NEVER product
performance.

---

## Banned pattern catalog (with the real deleted examples)

### 1. File-existence-as-"workflow" / "integration"
```cpp
// ❌ BANNED — deleted from IntegrationTests.cpp (CompleteWorkflowIntegration, RealisticWorkflowIntegration)
EXPECT_TRUE(std::filesystem::exists(designPath));
EXPECT_GT(std::filesystem::file_size(designPath), 0);
std::this_thread::sleep_for(std::chrono::milliseconds(10));   // "processing"
```
**Why banned:** proves a file is on disk, not that OdbDesign parsed/served/loaded it.
**✅ Required:** load the design through `DesignCache`/the API and assert parsed fields/counts.

### 2. Local `simulate*` stand-in for the product
```cpp
// ❌ BANNED — deleted from IntegrationTests.cpp (ProcessingPipelineIntegration)
ProcessingResult simulateDataProcessing(const std::filesystem::path& p) { /* sleep + return size */ }
EXPECT_TRUE(result.success);
```
**Why banned:** asserts the simulation ran; OdbDesign was never called.
**✅ Required:** call the real pipeline (parse → build → serialize) and compare real output.

### 3. Corrupt/empty input created but never fed to the product
```cpp
// ❌ BANNED — deleted from IntegrationTests.cpp (ErrorHandlingIntegration)
auto corrupt = TestUtils::createManagedTempFile("corrupted data", ".tgz");
EXPECT_TRUE(std::filesystem::exists(corrupt->path()));   // parser never invoked
```
**Why banned:** error handling is never exercised.
**✅ Required:** feed the corrupt archive to the real parser/cache and assert the real error/exception.

### 4. STL / filesystem "performance" benchmarks
```cpp
// ❌ BANNED — deleted from PerformanceTests.cpp (MemoryAllocationPerformance, PerformanceRegressionBaseline)
std::vector<char> buffer(1024*1024);
EXPECT_LT(benchmarkFunction([&]{ return buffer.size(); }, 50).count(), 50000);
```
**Why banned:** measures `std::vector`/`std::string`/`std::filesystem`, not OdbDesign.
**✅ Required:** measure real product operations (see `DesignStreamMemoryTests.cpp`,
`DesignFetchBenchmarkTests.cpp`): parse time, wire bytes, peak RSS, cold/warm stream latency.

### 5. Simulated concurrency / retry
```cpp
// ❌ BANNED — deleted from EnhancedTests.cpp (ConcurrentAccessTests, ErrorRecoveryTests)
threads.emplace_back([&]{ getTestDataDir(); std::filesystem::exists(dir); sleep_for(1us); });
TestUtils::retryWithBackoff([&]{ static int n=0; return ++n>=2; }, 3, 10ms);
```
**Why banned:** threads do nothing to the product; retry tests a lambda counter, not product recovery.
**✅ Required:** drive real concurrent `DesignCache` loads / real retry paths with deterministic
barriers and assert parse counts, object identity, and returned data (see `DesignCacheSingleFlightTests.cpp`).

### 6. Grep config/source for a string and call it "verified"
```cpp
// ❌ BANNED — the canonical CI incident (NuGet feed credentials)
EXPECT_NE(configText.find("ClearTextPassword"), std::string::npos);
```
**Why banned:** proves the text exists, not that the feed write works. Hid a 9-month outage.
**✅ Required:** perform the real operation (push-then-restore round-trip) and assert its outcome.

### 7. Misleading names
**❌ BANNED:** naming a test `Integration` / `Performance` / `ErrorHandling` / `Workflow` / `Stress` /
`Concurrency` when it does none of those things to the product.
**✅ Required:** the name must match what is actually exercised. A fake test with an honest name is
still banned; a fake test with a misleading name is doubly banned.

---

## What is NOT banned (legitimate assertions)

- **Asserting the product's actual string output** when the string IS the behavior: formatters,
  `Utf8Sanitizer`, ETag generation (`HttpCachingTests`), serialized JSON/protobuf, log output when
  logging is the behavior under test.
- **Fixture/environment sanity checks** (`TestTests.cpp` `TestDataDirDirectoryExists`) — these verify
  the test harness, not product behavior, and must not be counted as product coverage.
- **Bounded waits for real async operations** (`sleep_for`/polling in `DesignStreamTests`,
  `ResponseCacheTests`, `RequestLoadDesignTests`) — legitimate when waiting for a REAL load/stream to
  reach a state, followed by a real assertion. Prefer deterministic barriers over sleeps.
- **Drift detectors** (proto diff, `check-openapi-routes.py`) — acceptable as labelled drift signals,
  NEVER as proof of runtime behavior.

---

## Review checklist (reject the PR if any box is unchecked)

- [ ] Does each new/changed test call real OdbDesign production code (not a simulation/STL)?
- [ ] Does it assert an independently expected real outcome (value, bytes, exit code, status, socket state)?
- [ ] Is any string assertion the product's actual output, rather than a substitute for running it?
- [ ] Does the test name match what it actually exercises?
- [ ] For "integration/E2E": does it cross a real boundary (HTTP/gRPC/process/filesystem) with a real round-trip?
- [ ] For "performance": does it measure a product operation, not the STL?
- [ ] No `simulate*` stand-ins, no file-existence-as-processing, no corrupt-input-never-parsed.
- [ ] Any genuinely-impossible round-trip is labelled partial, not presented as verification.

---

## Enforcement

- The rule lives in `AGENTS.md` (loaded by every agent) — see "MANDATORY: No grep-style pseudo-validation".
- The release-readiness checker (`scripts/release_readiness.py`) only accepts real evidence; its
  negative controls prove a fake/skipped/missing result fails the gate.
- Coverage baseline (M0) is computed AFTER this purge so fake tests cannot inflate it.
- Real replacement suites are tracked in
  [`../plan/test-coverage-and-evaluation-validation-plan/`](../plan/test-coverage-and-evaluation-validation-plan/test-coverage-and-evaluation-validation-plan.md).

**When in doubt: RUN THE REAL THING AND CHECK WHAT ACTUALLY HAPPENED. THAT IS THE ONLY TEST!!!!!**
