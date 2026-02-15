# GitHub Actions Workflow Optimization Analysis

**Date:** November 25, 2025  
**Branch:** `feature/grpc`  
**Author:** GitHub Copilot

---

## Executive Summary

This document analyzes all GitHub Actions workflows in the OdbDesign repository and identifies potential optimizations, improvements, and cost-saving strategies. The workflows currently take 1-2+ hours for full CI runs, primarily due to vcpkg dependency compilation.

---

## 1. CMake Multi-Platform Build (`cmake-multi-platform.yml`)

### Current State
- **Triggers:** Push to `nam20485`, `feature/grpc`; PRs to `development`, `staging`, `main`, `release`, `nam20485`
- **Platforms:** Windows 2022, Ubuntu 22.04, macOS 14
- **Build Time:** ~1-2 hours per platform (mostly vcpkg)
- **Caching:** GH Actions cache (`x-gha`) only

### Issues Identified

| Issue                                      | Impact             | Severity   |
| ------------------------------------------ | ------------------ | ---------- |
| Linux runs out of disk space               | Build failures     | 🔴 Critical |
| No persistent binary cache                 | Long build times   | 🔴 Critical |
| vcpkg cloned fresh every run (non-Windows) | Wasted time        | 🟡 Medium   |
| Artifact compression done inline           | No parallelization | 🟢 Low      |
| Test data repo checked out after build     | Could be parallel  | 🟢 Low      |

### Recommendations

#### 1.1 ✅ Implement GitHub Packages Binary Cache (PR #501)
**Status:** Already implemented in PR #501  
**Impact:** 50-80% build time reduction after cache is populated  
**Details:** Hybrid cache using `x-gha` (fast) + NuGet/GitHub Packages (persistent)

#### 1.2 🆕 Cache vcpkg Bootstrap
**Impact:** 1-2 minutes saved per non-Windows build  
**Implementation:**
```yaml
- name: Cache vcpkg
  uses: actions/cache@v4
  with:
    path: ${{ env.VCPKG_ROOT }}
    key: vcpkg-${{ runner.os }}-${{ hashFiles('vcpkg.json') }}
    restore-keys: |
      vcpkg-${{ runner.os }}-
```

#### 1.3 🆕 Free Disk Space on Linux
**Impact:** Prevents "No space left on device" errors  
**Implementation:**
```yaml
- name: Free Disk Space (Ubuntu)
  if: matrix.os == 'ubuntu-22.04'
  uses: jlumbroso/free-disk-space@main
  with:
    tool-cache: false
    android: true
    dotnet: true
    haskell: true
    large-packages: true
    docker-images: true
    swap-storage: true
```

#### 1.4 🆕 Parallel Test Data Checkout
**Impact:** Minor time savings  
**Implementation:** Move test data checkout earlier (before build) using `actions/checkout` with `sparse-checkout`

#### 1.5 🆕 Use `ubuntu-24.04` Runner
**Impact:** Newer toolchain, potentially faster  
**Risk:** May require testing for compatibility

#### 1.6 🆕 Conditional Artifact Upload
**Impact:** Reduced artifact storage costs  
**Implementation:** Only upload artifacts on `main`/`release` branches
```yaml
- name: Upload Artifacts
  if: github.ref_name == 'main' || github.ref_name == 'release'
  uses: actions/upload-artifact@v4
```

#### 1.7 🆕 Add Build Timing Annotations
**Impact:** Better visibility into slow steps  
**Implementation:**
```yaml
- name: CMake Build
  run: |
    echo "::group::CMake Build"
    time cmake --build --preset ${{matrix.preset}}
    echo "::endgroup::"
```

---

## 2. Docker Publish (`docker-publish.yml`)

### Current State
- **Triggers:** Push to `development`, `staging`, `main`, `release`, `nam20485`
- **Registry:** GHCR (`ghcr.io`)
- **Caching:** Docker layer cache (`type=gha`)
- **Features:** Cosign signing, metadata annotations

### Issues Identified

| Issue                                | Impact                     | Severity   |
| ------------------------------------ | -------------------------- | ---------- |
| Full rebuild inside Docker           | Very slow (~1hr)           | 🔴 Critical |
| Builds on every push to dev branches | Unnecessary compute        | 🟡 Medium   |
| No multi-platform builds             | Limited deployment targets | 🟢 Low      |

### Recommendations

#### 2.1 🆕 Use Pre-built Artifacts in Docker
**Impact:** 80%+ Docker build time reduction  
**Implementation:** Download artifacts from CMake workflow instead of building inside Docker
```yaml
- name: Download Linux Artifacts
  uses: actions/download-artifact@v4
  with:
    name: ubuntu-22.04-artifacts
    path: ./artifacts

- name: Build Docker Image (artifacts-based)
  uses: docker/build-push-action@v5
  with:
    context: .
    file: Dockerfile.prebuilt  # New lightweight Dockerfile
```

**New `Dockerfile.prebuilt`:**
```dockerfile
FROM debian:bookworm-slim AS run
# ... runtime setup only, no build ...
COPY ./artifacts/*.so /OdbDesign/bin/
COPY ./artifacts/OdbDesignServer /OdbDesign/bin/
```

#### 2.2 🆕 Trigger Docker Build Only After Successful CMake Build
**Impact:** Reduces failed Docker builds, saves compute  
**Implementation:** Use `workflow_run` trigger
```yaml
on:
  workflow_run:
    workflows: ["CMake Build Multi-Platform"]
    types: [completed]
    branches: [development, staging, main, release]
```

#### 2.3 🆕 Add Multi-Platform Support
**Impact:** ARM64 support for Apple Silicon, AWS Graviton  
**Implementation:**
```yaml
- name: Set up QEMU
  uses: docker/setup-qemu-action@v3

- name: Build and push
  uses: docker/build-push-action@v5
  with:
    platforms: linux/amd64,linux/arm64
```

#### 2.4 🆕 Reduce Build Frequency
**Impact:** Cost savings  
**Implementation:** Only build Docker on `main`/`release` merges, not every push

---

## 3. Docker Scout Scan (`docker-scout-scan.yml`)

### Current State
- **Triggers:** Push to `nam20485`; PRs to `development`, `staging`, `main`, `release`
- **Features:** CVE scanning, SARIF upload, PR comments

### Issues Identified

| Issue                                  | Impact             | Severity   |
| -------------------------------------- | ------------------ | ---------- |
| Rebuilds Docker image (duplicate work) | Slow, wasteful     | 🔴 Critical |
| Runs on every PR                       | High compute usage | 🟡 Medium   |

### Recommendations

#### 3.1 🆕 Scan Published Images Instead of Rebuilding
**Impact:** 90%+ time reduction for security scans  
**Implementation:**
```yaml
- name: Scan Published Image
  uses: docker/scout-action@v1
  with:
    command: cves,recommendations
    image: ${{ env.REGISTRY }}/${{ env.IMAGE_NAME }}:${{ github.ref_name }}-latest
```

#### 3.2 🆕 Run Weekly Instead of Per-PR
**Impact:** Significant cost savings  
**Implementation:**
```yaml
on:
  schedule:
    - cron: '0 0 * * 0'  # Weekly on Sunday
  workflow_dispatch:
```

---

## 4. Create Release (`create-release.yml`)

### Current State
- **Triggers:** `repository_dispatch` from Docker Publish
- **Features:** Artifact download, GPG signing, GitHub Release creation

### Issues Identified

| Issue                                        | Impact      | Severity |
| -------------------------------------------- | ----------- | -------- |
| Depends on specific workflow artifact naming | Fragile     | 🟡 Medium |
| No changelog generation                      | Manual work | 🟢 Low    |

### Recommendations

#### 4.1 🆕 Add Automatic Changelog Generation
**Impact:** Better release notes  
**Implementation:**
```yaml
- name: Generate Changelog
  uses: orhun/git-cliff-action@v3
  with:
    config: cliff.toml
    args: --latest
  env:
    OUTPUT: CHANGELOG.md
```

#### 4.2 🆕 Add Release Attestation
**Impact:** Supply chain security (SLSA compliance)  
**Implementation:**
```yaml
- name: Attest Release Artifacts
  uses: actions/attest-build-provenance@v1
  with:
    subject-path: '${{ github.workspace }}/artifacts/*.zip'
```

---

## 5. Dockerfile Optimization

### Current State
- Multi-stage build (build + run)
- Builds everything from source inside Docker
- ~1 hour build time

### Recommendations

#### 5.1 🆕 Use BuildKit Cache Mounts
**Impact:** Faster rebuilds with vcpkg cache persistence  
**Implementation:**
```dockerfile
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    ${VCPKG_ROOT}/vcpkg install
```

#### 5.2 🆕 Split Dockerfile Stages for Better Caching
**Impact:** Better layer cache utilization  
**Implementation:**
```dockerfile
# Stage 1: vcpkg dependencies only
FROM debian:bookworm-slim AS deps
COPY vcpkg.json vcpkg-configuration.json ./
RUN vcpkg install

# Stage 2: build (depends on deps)
FROM deps AS build
COPY . .
RUN cmake --preset linux-release && cmake --build --preset linux-release

# Stage 3: runtime
FROM debian:bookworm-slim AS run
COPY --from=build /src/OdbDesign/out/build/linux-release/ ./bin/
```

#### 5.3 🆕 Use Distroless/Slim Base Image for Runtime
**Impact:** Smaller image, better security  
**Implementation:**
```dockerfile
FROM gcr.io/distroless/cc-debian12 AS run
```

---

## 6. Cross-Workflow Optimization Strategies

### 6.1 🆕 Implement Workflow Dependencies
**Current:** Workflows run independently  
**Proposed:** Chain workflows to avoid duplicate work

```
CMake Build → Docker Publish → Create Release
     ↓
Docker Scout Scan (on published image)
```

### 6.2 🆕 Centralize Common Steps
**Impact:** Easier maintenance  
**Implementation:** Create reusable workflows
```yaml
# .github/workflows/reusable-vcpkg-setup.yml
on:
  workflow_call:
    inputs:
      os:
        required: true
        type: string
```

### 6.3 🆕 Add Concurrency Controls
**Impact:** Prevents duplicate runs, saves compute  
**Implementation:**
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

### 6.4 🆕 Path Filtering for Workflows
**Impact:** Skip unnecessary runs  
**Implementation:**
```yaml
on:
  push:
    paths:
      - 'OdbDesign*/**'
      - 'CMakeLists.txt'
      - 'vcpkg.json'
      - '.github/workflows/cmake-*.yml'
    paths-ignore:
      - '**.md'
      - 'docs/**'
```

---

## 7. Estimated Impact Summary

| Optimization                 | Time Savings         | Cost Savings | Complexity |
| ---------------------------- | -------------------- | ------------ | ---------- |
| GitHub Packages Binary Cache | 50-80%               | High         | ✅ Done     |
| Docker from Artifacts        | 80%+                 | High         | Medium     |
| Free Disk Space              | N/A (fixes failures) | N/A          | Low        |
| Concurrency Controls         | Variable             | Medium       | Low        |
| Path Filtering               | Variable             | Medium       | Low        |
| Weekly Security Scans        | 90% scan costs       | Medium       | Low        |
| Reusable Workflows           | N/A                  | Maintenance  | Medium     |
| vcpkg Bootstrap Cache        | 1-2 min/build        | Low          | Low        |

---

## 8. Implementation Priority

### Phase 1 (Immediate - fixes current issues)
1. ✅ GitHub Packages Binary Cache (PR #501)
2. 🆕 Free Disk Space on Linux
3. 🆕 Add concurrency controls

### Phase 2 (Short-term - optimization)
4. 🆕 Docker from pre-built artifacts
5. 🆕 Path filtering
6. 🆕 vcpkg bootstrap caching

### Phase 3 (Medium-term - architecture)
7. 🆕 Workflow dependencies/chaining
8. 🆕 Reusable workflows
9. 🆕 Multi-platform Docker builds

### Phase 4 (Long-term - polish)
10. 🆕 Automatic changelog
11. 🆕 Release attestation
12. 🆕 Distroless runtime image

---

## 9. Additional Notes

### Security Considerations
- All workflow changes should maintain current security posture (harden-runner, pinned actions)
- Binary cache uses PAT - ensure proper scoping and rotation
- Consider enabling Dependabot for action updates

### Monitoring Recommendations
- Enable GitHub Actions insights to track workflow costs
- Add custom metrics for build times
- Set up alerts for failed workflows

---

*This analysis is based on workflow files as of November 25, 2025. Some optimizations (marked ✅) are already implemented or in progress.*
