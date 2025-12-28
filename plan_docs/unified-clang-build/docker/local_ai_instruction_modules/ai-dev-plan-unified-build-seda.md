# OdbDesign CI Modernization: 1+5 SEDA Execution Plan

## Stage 0: Global Orchestration Strategy (Type 1)
**Objective:** Unify OdbDesign's multi-platform build system into a single Clang-based toolchain running in a Linux container. Establish a 'Chain of Verified Artifacts' where binaries tested natively are the exact ones released.

**Orchestrator Mandate:**
- **Branching:** Work must be performed on `feat/unified-clang-build`.
- **Short Rein:** Dispatch one stage at a time. Validate AC before merging.
- **Context Handover:** Provide agents only with stage-relevant files.

---

## Stage 1: Toolchain Infrastructure Event
**Sub-agent:** DevOps Specialist.
**Context:** `Dockerfile`, `vcpkg.json`.
**Tasks:** Update Dockerfile to Ubuntu 24.04; Install Clang/LLVM/MinGW-w64 (UCRT); Integrate vcpkg bootstrap.
**AC:** `docker build` succeeds; `x86_64-w64-mingw32-clang++ --version` works inside.

---

## Stage 2: Build Logic Normalization Event
**Sub-agent:** C++/CMake Specialist.
**Context:** `CMakeLists.txt`, `CMakePresets.json`.
**Tasks:** Unify CMake flags to Clang-standard (-Wall -Wextra); Standardize bin/lib output dirs; Implement linux/windows-cross/macos-cross presets.
**AC:** Project configures for all targets; Windows .exe generated via cross-compile.

---

## Stage 3: Unified Compilation Pipeline Event
**Sub-agent:** GitHub Actions Specialist.
**Context:** `.github/workflows/docker-publish.yml`, `.github/workflows/cmake-multi-platform.yml`.
**Tasks:** Optimize image rebuilds with path filters; Create containerized multi-platform build workflow.
**AC:** Parallel compilation on single Linux runner; Artifacts uploaded.

---

## Stage 4: Native Verification Event
**Sub-agent:** QA/SDET.
**Context:** `.github/workflows/cmake-test-native.yml`.
**Tasks:** Run binaries on native OS runners; Re-upload successful binaries as `verified-{artifact}`.
**AC:** Linux-built .exe runs successfully on native `windows-latest` runner.

---

## Stage 5: Verified Release Event
**Sub-agent:** Release Engineer.
**Context:** `.github/workflows/create-release.yml`.
**Tasks:** Packaging of `verified-*` bits; Zero-build release creation for release/staging/develop branches.
**AC:** Release created with bit-for-bit identical tested binaries.