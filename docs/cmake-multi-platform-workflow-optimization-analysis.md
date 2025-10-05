# CMake Multi-Platform Workflow Optimization Analysis

## Executive Summary

This document provides a comprehensive analysis of optimization opportunities for the `cmake-multi-platform.yml` GitHub Actions workflow. The analysis focuses on reducing runtime, particularly for Windows builds, while maintaining build reliability and quality.

**Current Workflow Structure:**
- Builds on 3 platforms: Windows (windows-2022), Linux (ubuntu-22.04), MacOS (macos-14)
- Uses vcpkg for C++ dependency management
- CMake + Ninja build system
- Manual vcpkg install before CMake configuration
- Full test suite execution with test data repository checkout

**Primary Bottlenecks Identified:**
1. vcpkg dependency installation (especially Windows)
2. CMake compilation phase (especially Windows with MSVC)
3. Sequential job execution
4. Test data repository checkout size

---

## Optimization Opportunities

### 1. Enhanced vcpkg Binary Caching

#### Current State
```yaml
env:
  VCPKG_BINARY_SOURCES: 'clear;x-gha,readwrite'
```

The workflow currently uses GitHub Actions cache (`x-gha`) for vcpkg binary caching, which is good but can be optimized further.

#### Optimization Strategy
- **Implement multiple cache backends** (GitHub Actions cache + NuGet/Azure Blob)
- **Pre-warm cache in separate workflow** that runs on dependency changes
- **Use manifest mode optimizations** to avoid redundant dependency resolution

#### Implementation
```yaml
env:
  VCPKG_BINARY_SOURCES: 'clear;x-gha,readwrite;nuget,https://nuget.pkg.github.com/nam20485/index.json,readwrite'
```

#### Pros
- Significantly reduces vcpkg install time on cache hits (from 5-10 minutes to 30 seconds)
- Shared cache across workflows and branches
- Reduces network bandwidth for repeated builds
- Multiple fallback cache sources improve reliability

#### Cons
- Requires NuGet authentication setup and management
- Cache storage counts against GitHub package storage quota
- Initial cache population still takes full time
- Cache invalidation complexity

#### Complexity/Risk: **Medium** | Time Reduction: **40-60% on vcpkg install phase** (3-6 minutes per build)

---

### 2. ccache / sccache Compiler Caching

#### Current State
The workflow does not use compiler caching, meaning every build compiles all source files from scratch.

#### Optimization Strategy
Enable `sccache` (Shared Compilation Cache) or `ccache` to cache compiled object files between builds.

#### Implementation
```yaml
- name: Setup sccache
  uses: mozilla-actions/sccache-action@v0.0.6

- name: Configure CMake with sccache
  run: cmake --preset ${{matrix.preset}}
  env:
    CMAKE_CXX_COMPILER_LAUNCHER: sccache
    CMAKE_C_COMPILER_LAUNCHER: sccache
```

Also update CMakeLists.txt:
```cmake
# Enable compiler caching in CI
if(DEFINED ENV{CI})
  find_program(SCCACHE_PROGRAM sccache)
  if(SCCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
  endif()
endif()
```

#### Pros
- Dramatic compilation time reduction on incremental builds (60-90% reduction)
- Works across branches with same source files
- Transparent to the build process
- Especially effective for Windows MSVC builds (slowest compiler)
- Can reduce full builds from 10-15 minutes to 2-5 minutes on cache hits

#### Cons
- Adds dependency on external action
- Cache storage overhead (~500MB-2GB per platform)
- Cold cache still requires full compilation
- Slight overhead on cache misses (~5-10% slower)

#### Complexity/Risk: **Low** | Time Reduction: **60-90% on compilation phase** (8-12 minutes per build on cache hits)

---

### 3. Parallel Matrix Strategy Optimization

#### Current State
```yaml
strategy:
  fail-fast: false
  matrix:
    include:
      - os: windows-2022
        preset: x64-release
      - os: ubuntu-22.04
        preset: linux-release
      - os: macos-14
        preset: macos-release
```

All platforms run in parallel, but each platform is single-threaded in its steps.

#### Optimization Strategy
**Option A: Split Debug and Release Builds**
- Create separate workflows or jobs for Debug/Release
- Run release builds only on main branches, debug on PR

**Option B: Reduce Platform Coverage**
- Run all platforms on push to main/release
- Run only Linux + Windows on PRs (skip MacOS)
- Add conditional MacOS builds for platform-specific changes

**Option C: Increase Runner Parallelism**
- Use larger runner types with more CPU cores
- Enable parallel CMake builds explicitly

#### Implementation (Option B)
```yaml
strategy:
  fail-fast: false
  matrix:
    include:
      - os: windows-2022
        preset: x64-release
      - os: ubuntu-22.04
        preset: linux-release
      - os: macos-14
        preset: macos-release
        # Only run MacOS on main branches
        if: github.ref == 'refs/heads/main' || github.ref == 'refs/heads/release'
```

#### Implementation (Option C)
```yaml
- name: CMake Build
  run: cmake --build --preset ${{matrix.preset}} --parallel 4
```

#### Pros
- **Option A**: Faster PR feedback, focuses on most important build type
- **Option B**: Reduces CI minutes consumption, faster PR feedback
- **Option C**: Better utilization of runner resources, simple implementation

#### Cons
- **Option A**: May miss debug-only issues on PRs
- **Option B**: May miss MacOS-specific issues on PRs
- **Option C**: Higher memory usage, marginal gains on well-cached builds

#### Complexity/Risk: **Low-Medium** | Time Reduction: **20-50% total workflow time** (Option B), **10-20%** (Option C)

---

### 4. CMake Precompiled Headers (PCH)

#### Current State
No precompiled headers are currently configured in the CMake build system.

#### Optimization Strategy
Enable CMake's `target_precompile_headers()` for commonly included headers (STL, Boost, system headers).

#### Implementation
In relevant CMakeLists.txt files:
```cmake
target_precompile_headers(OdbDesignLib 
  PRIVATE
    <string>
    <vector>
    <memory>
    <algorithm>
    <iostream>
    <fstream>
  PUBLIC
    <crow.h>
    <libarchive/archive.h>
)
```

#### Pros
- Reduces compilation time by 20-40% on clean builds
- Especially effective for Windows MSVC (slowest)
- Built-in CMake feature, well-supported
- Improves local development experience too

#### Cons
- Requires careful header selection
- Can increase binary size slightly
- May cause issues with some compilers/configurations
- Needs maintenance as dependencies change
- PCH files add ~50-100MB to cache

#### Complexity/Risk: **Medium** | Time Reduction: **20-40% on compilation phase** (2-5 minutes per build)

---

### 5. Split vcpkg Installation from Build Job

#### Current State
Each build job:
1. Installs vcpkg
2. Installs dependencies
3. Configures CMake
4. Builds
5. Tests

#### Optimization Strategy
Create a separate job to prepare vcpkg dependencies, then reuse across build jobs.

#### Implementation
```yaml
jobs:
  prepare-dependencies:
    runs-on: ubuntu-22.04
    outputs:
      cache-key: ${{ steps.vcpkg-hash.outputs.hash }}
    steps:
      - name: Checkout
        uses: actions/checkout@v4
      
      - name: Calculate vcpkg hash
        id: vcpkg-hash
        run: echo "hash=${{ hashFiles('vcpkg.json', 'vcpkg-configuration.json') }}" >> $GITHUB_OUTPUT
      
      - name: Cache vcpkg
        uses: actions/cache@v4
        with:
          path: |
            ${{ github.workspace }}/vcpkg
            ~/.cache/vcpkg
          key: vcpkg-${{ runner.os }}-${{ steps.vcpkg-hash.outputs.hash }}
      
      - name: Install vcpkg dependencies
        run: # ... vcpkg install commands

  build:
    needs: prepare-dependencies
    # ... rest of build job
```

#### Pros
- Reduces duplicated vcpkg work across platforms
- Clearer separation of concerns
- Easier to debug dependency issues
- Can use smaller runners for build-only jobs

#### Cons
- Adds job orchestration complexity
- Sequential dependency increases total time if vcpkg isn't bottleneck
- Cross-platform caching challenges (Windows vs Linux vs MacOS)
- Not beneficial if binary caching already works well

#### Complexity/Risk: **High** | Time Reduction: **Variable** (0-30% depending on cache effectiveness)

---

### 6. Optimize Test Execution

#### Current State
```yaml
- name: Checkout OdbDesign Test Data Repository
  uses: actions/checkout@v4
  with:
    repository: 'nam20485/OdbDesignTestData'
    path: 'OdbDesignTestData'
```

Full test data repository is checked out for every build.

#### Optimization Strategy
**Option A: Cache test data repository**
```yaml
- name: Cache Test Data
  uses: actions/cache@v4
  with:
    path: OdbDesignTestData
    key: test-data-${{ hashFiles('OdbDesignTestData/.git/HEAD') }}
```

**Option B: Use Git LFS or sparse checkout**
```yaml
- name: Sparse Checkout Test Data
  run: |
    git clone --filter=blob:none --sparse https://github.com/nam20485/OdbDesignTestData.git
    cd OdbDesignTestData
    git sparse-checkout set TEST_DATA/essential  # Only critical tests
```

**Option C: Split test execution**
- Run unit tests always (fast)
- Run integration tests only on main branches or scheduled
- Use test sharding for parallel execution

#### Implementation (Option C)
```yaml
- name: CMake Test (Unit Tests Only)
  if: github.event_name == 'pull_request'
  run: ctest --test-dir ./out/build/${{matrix.preset}}/OdbDesignTests -L unit

- name: CMake Test (All Tests)
  if: github.event_name == 'push'
  run: ctest --test-dir ./out/build/${{matrix.preset}}/OdbDesignTests
```

#### Pros
- **Option A**: Faster test data retrieval (30 seconds vs 2-3 minutes)
- **Option B**: Reduces network transfer, only downloads needed files
- **Option C**: Faster PR feedback, reduced CI minutes on PRs

#### Cons
- **Option A**: Cache invalidation complexity, storage overhead
- **Option B**: Requires test data restructuring, sparse-checkout complexity
- **Option C**: May miss integration test failures on PRs

#### Complexity/Risk: **Low-Medium** | Time Reduction: **1-3 minutes per build**

---

### 7. Use Pre-built vcpkg Binaries from Registry

#### Current State
Each workflow run builds vcpkg packages from source using binary caching for reuse.

#### Optimization Strategy
Use vcpkg's built-in binary package registries or pre-build dependencies in a separate workflow.

#### Implementation
```yaml
# In a separate workflow: prebuild.yml (runs on dependency changes)
name: Prebuild Dependencies

on:
  push:
    paths:
      - 'vcpkg.json'
      - 'vcpkg-configuration.json'

jobs:
  prebuild:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [windows-2022, ubuntu-22.04, macos-14]
    steps:
      - name: Install vcpkg dependencies
        run: ${{env.VCPKG_ROOT}}/vcpkg install
      # Dependencies are automatically cached via x-gha binary caching
```

Then main workflow simply uses cached binaries.

#### Pros
- Separates dependency management from build/test cycle
- Dependencies only rebuilt when vcpkg.json changes
- Reduces PR build times significantly
- Better visibility into dependency build issues

#### Cons
- Requires separate workflow maintenance
- Dependencies must be pre-built before main workflow
- Coordination complexity between workflows
- May delay PR builds waiting for dependency workflow

#### Complexity/Risk: **Medium** | Time Reduction: **40-70% on vcpkg phase** (3-7 minutes per build)

---

### 8. Optimize Windows-Specific Build Steps

#### Current State
Windows builds are typically slowest due to:
- MSVC compiler being slower than GCC/Clang
- vcpkg install slower on Windows
- Windows runner performance characteristics

#### Optimization Strategy
**A. Use Clang-CL instead of MSVC**
```yaml
cacheVariables:
  CMAKE_C_COMPILER: clang-cl
  CMAKE_CXX_COMPILER: clang-cl
```

**B. Optimize Windows vcpkg install**
```yaml
- name: Manual vcpkg Install (Windows)
  run: |
    # Use more cores for building
    $env:VCPKG_MAX_CONCURRENCY = "8"
    & "${{env.VCPKG_ROOT}}/vcpkg" install --triplet x64-windows `
      --x-buildtrees-root=${{env.TEMP}}/vcpkg-buildtrees `
      --x-packages-root=${{env.TEMP}}/vcpkg-packages
```

**C. Use Windows Server 2022 Docker Container**
Run builds in a pre-configured container with dependencies.

#### Pros
- **A**: Clang often 20-40% faster than MSVC for C++ compilation
- **B**: Uses temp storage (SSD) for intermediate files, better parallelism
- **C**: Complete environment control, reproducibility

#### Cons
- **A**: Different compiler may expose different warnings/errors
- **B**: Higher disk I/O, may not help on all runner types
- **C**: Container management overhead, licensing considerations

#### Complexity/Risk: **Medium-High** | Time Reduction: **20-40% on Windows builds** (5-10 minutes)

---

### 9. Incremental Build Support

#### Current State
Every workflow run is a full clean build (`out/build` directory is fresh).

#### Optimization Strategy
Preserve build directories between runs using GitHub Actions cache.

#### Implementation
```yaml
- name: Cache CMake Build Directory
  uses: actions/cache@v4
  with:
    path: |
      out/build/${{matrix.preset}}
      !out/build/${{matrix.preset}}/**/*.o
      !out/build/${{matrix.preset}}/**/*.obj
    key: cmake-build-${{ runner.os }}-${{ matrix.preset }}-${{ hashFiles('**/CMakeLists.txt', '**/*.cpp', '**/*.h') }}
    restore-keys: |
      cmake-build-${{ runner.os }}-${{ matrix.preset }}-

- name: CMake Configure
  run: cmake --preset ${{matrix.preset}}

- name: CMake Build (Incremental)
  run: cmake --build --preset ${{matrix.preset}}
```

#### Pros
- Can reduce build times by 50-80% on incremental changes
- Works well with ccache/sccache for compound benefits
- Especially effective for small PRs touching few files
- Natural CI optimization for iterative development

#### Cons
- Cache key management complexity
- Risk of stale build artifacts causing issues
- Large cache size (500MB-2GB per platform/preset)
- Doesn't help first build or large changes
- May hide full build issues

#### Complexity/Risk: **Medium-High** | Time Reduction: **50-80% on small changes** (8-15 minutes on incremental)

---

### 10. Use GitHub Actions Larger Runners

#### Current State
Using standard GitHub-hosted runners:
- `windows-2022`: 4 cores, 16GB RAM
- `ubuntu-22.04`: 4 cores, 16GB RAM  
- `macos-14`: 3 cores, 14GB RAM

#### Optimization Strategy
Use GitHub Actions larger runners (8-core, 32GB RAM) for faster parallel compilation.

#### Implementation
```yaml
jobs:
  build:
    runs-on: ${{ matrix.os }}-8-cores  # If available for your account
    # OR use self-hosted runners
    runs-on: self-hosted-windows-16-cores
```

Also increase parallel build jobs:
```yaml
- name: CMake Build
  run: cmake --build --preset ${{matrix.preset}} --parallel 8
```

#### Pros
- Faster compilation through better parallelism (30-50% faster)
- Better for memory-intensive builds
- Simple configuration change
- Helps both vcpkg and CMake build phases

#### Cons
- **Significant cost increase** (2x or more for larger runners)
- Not available on all GitHub plans
- Diminishing returns beyond certain core count
- Self-hosted runners require infrastructure management

#### Complexity/Risk: **Low (config)** / **High (self-hosted)** | Time Reduction: **30-50% total build time**

---

## Recommended Implementation Priority

### Phase 1: Quick Wins (Low Risk, High Impact)
**Timeline: 1-2 weeks**

1. **Enable ccache/sccache** (Optimization #2)
   - Impact: 60-90% compilation time reduction on cache hits
   - Risk: Low
   - Effort: 2-4 hours

2. **Optimize test data checkout** (Optimization #6, Option A)
   - Impact: 1-3 minutes per build
   - Risk: Low
   - Effort: 1-2 hours

3. **Parallel build optimization** (Optimization #3, Option C)
   - Impact: 10-20% compilation time reduction
   - Risk: Low
   - Effort: 30 minutes

4. **Update .gitignore** (Already done)
   - Prevent accidental node_modules commits
   - Risk: None
   - Effort: 15 minutes

**Expected Total Time Reduction: 40-60% on cache hits, 15-25% on cold builds**

---

### Phase 2: Medium Wins (Medium Risk, Medium-High Impact)
**Timeline: 2-4 weeks**

5. **Enhanced vcpkg binary caching** (Optimization #1)
   - Impact: 40-60% vcpkg install time reduction
   - Risk: Medium (authentication, cache management)
   - Effort: 4-8 hours

6. **CMake Precompiled Headers** (Optimization #4)
   - Impact: 20-40% compilation time reduction
   - Risk: Medium (compatibility, maintenance)
   - Effort: 8-16 hours (requires code changes)

7. **Conditional MacOS builds** (Optimization #3, Option B)
   - Impact: 20-30% reduced CI minutes on PRs
   - Risk: Medium (may miss platform-specific issues)
   - Effort: 2-4 hours

**Expected Total Time Reduction: 50-70% on cache hits, 30-45% on cold builds**

---

### Phase 3: Advanced Optimizations (Higher Risk, Variable Impact)
**Timeline: 4-8 weeks**

8. **Prebuild workflow for dependencies** (Optimization #7)
   - Impact: 40-70% vcpkg time reduction
   - Risk: Medium-High (workflow orchestration)
   - Effort: 8-16 hours

9. **Windows-specific optimizations** (Optimization #8)
   - Impact: 20-40% Windows build time reduction
   - Risk: Medium-High (compiler changes, compatibility)
   - Effort: 16-24 hours

10. **Incremental build support** (Optimization #9)
    - Impact: 50-80% on small changes, 0% on large changes
    - Risk: High (stale artifacts, cache management)
    - Effort: 8-16 hours

**Expected Total Time Reduction: 60-80% on optimal conditions, 40-60% typical**

---

### Phase 4: Infrastructure Investments (High Cost/Effort)
**Timeline: 8+ weeks or ongoing**

11. **Larger runners or self-hosted** (Optimization #10)
    - Impact: 30-50% total build time
    - Risk: Low (config) / High (infrastructure)
    - Effort: 1 hour (config) / Ongoing (self-hosted)
    - Cost: High (2-5x runner costs)

12. **Split dependency management** (Optimization #5)
    - Impact: Variable
    - Risk: High (architectural changes)
    - Effort: 16-32 hours

---

## Risk Mitigation Strategies

### For All Optimizations

1. **Implement incrementally**: One optimization at a time, measure impact
2. **Monitor metrics**: Track build times before/after each change
3. **Maintain rollback ability**: Keep original workflow as backup
4. **Test thoroughly**: Ensure builds remain reproducible and correct
5. **Document changes**: Update build documentation and onboarding guides

### Specific Risks

| Risk | Mitigation |
|------|------------|
| Cache staleness causing build failures | Implement cache versioning, add manual cache clear workflow |
| Increased complexity breaking builds | Comprehensive testing on feature branch before merge |
| Cost overruns from cache storage | Monitor GitHub storage usage, set retention policies |
| Missed platform-specific issues | Require full platform builds before merge to main |
| Slower cold builds due to caching overhead | Benchmark and compare, keep optimizations optional |

---

## Expected Outcomes by Phase

### Phase 1 (Quick Wins)
- **PR Build Time**: 35-25 minutes (29% reduction)
- **Main Build Time**: 40-30 minutes (25% reduction)  
- **CI Minutes/Month**: 15,000 → 11,000 (27% reduction)
- **Implementation Cost**: 8-16 hours
- **Risk Level**: Low

### Phase 2 (Medium Wins)
- **PR Build Time**: 25-15 minutes (additional 40% reduction)
- **Main Build Time**: 30-20 minutes (additional 33% reduction)
- **CI Minutes/Month**: 11,000 → 7,000 (36% reduction)
- **Implementation Cost**: 24-40 hours
- **Risk Level**: Medium

### Phase 3 (Advanced)
- **PR Build Time**: 15-8 minutes (additional 47% reduction)
- **Main Build Time**: 20-12 minutes (additional 40% reduction)
- **CI Minutes/Month**: 7,000 → 4,000 (43% reduction)
- **Implementation Cost**: 40-64 hours
- **Risk Level**: Medium-High

### Overall Potential
- **Best Case**: 80% reduction (40 min → 8 min on cached incremental builds)
- **Typical Case**: 60% reduction (40 min → 16 min on cached builds)
- **Worst Case**: 25% reduction (40 min → 30 min on cold builds)

---

## Cost-Benefit Analysis

| Optimization | Implementation Hours | Time Saved per Build | CI Minutes Saved/Month | ROI (first month) |
|--------------|---------------------|----------------------|------------------------|-------------------|
| ccache/sccache | 2-4h | 8-12 min | 2,400-3,600 | 600-1800x |
| Test data caching | 1-2h | 1-3 min | 300-900 | 150-900x |
| Parallel builds | 0.5h | 1-2 min | 300-600 | 600-1200x |
| Enhanced vcpkg cache | 4-8h | 3-6 min | 900-1,800 | 113-450x |
| Precompiled headers | 8-16h | 2-5 min | 600-1,500 | 38-188x |
| Conditional MacOS | 2-4h | 5-10 min (PR) | 1,000-2,000 | 250-1000x |
| Prebuild workflow | 8-16h | 3-7 min | 900-2,100 | 56-263x |
| Windows optimizations | 16-24h | 5-10 min | 750-1,500 | 31-94x |
| Incremental builds | 8-16h | 8-15 min* | 1,200-2,250* | 75-281x |
| Larger runners | 1h / $$ | 6-10 min | 1,800-3,000 | Depends on cost |

*Highly variable based on change size

**Assumptions**: 
- 300 builds/month (10 per day)
- Average build time 40 minutes currently
- 50% cache hit rate for caching strategies

---

## Monitoring and Measurement

### Key Metrics to Track

1. **Build Time Metrics**
   - Total workflow duration
   - vcpkg install time
   - CMake configure time
   - Compilation time
   - Test execution time
   - Per-platform breakdown

2. **Cache Metrics**
   - Cache hit rate (ccache, vcpkg, test data)
   - Cache size and storage usage
   - Cache restoration time

3. **Cost Metrics**
   - GitHub Actions minutes consumed
   - Storage costs (caches, packages)
   - Runner costs (if using larger/self-hosted)

4. **Quality Metrics**
   - Build failure rate
   - Test failure rate
   - False positive rate (cache-related)

### Recommended Tooling

1. **GitHub Actions Built-in**
   - Workflow run summaries
   - Actions usage in Insights

2. **Custom Monitoring**
   - Add timing annotations to workflow steps
   - Export metrics to external system (optional)

```yaml
- name: Build with timing
  run: |
    start_time=$(date +%s)
    cmake --build --preset ${{matrix.preset}}
    end_time=$(date +%s)
    echo "::notice::Build time: $((end_time - start_time))s"
```

3. **Cache Statistics**
```yaml
- name: Print cache statistics
  run: |
    sccache --show-stats || true
    ccache -s || true
```

---

## Conclusion

The `cmake-multi-platform` workflow has significant optimization opportunities, particularly in the areas of compiler caching, dependency management, and test execution. By implementing the recommended optimizations in phases, the project can achieve:

- **60-80% reduction in build times** for typical incremental changes
- **30-50% reduction in build times** for cold builds
- **Significant CI cost savings** (potentially 50-70% reduction in minutes)
- **Improved developer experience** with faster feedback

The Phase 1 optimizations (ccache/sccache, test data caching, parallel builds) offer the best ROI with minimal risk and should be implemented first. Subsequent phases can be evaluated based on the results and specific project needs.

### Final Recommendations

**Immediate actions (this week):**
1. Implement ccache/sccache
2. Cache test data repository
3. Enable parallel builds

**Short-term actions (this month):**
4. Enhanced vcpkg binary caching
5. Conditional MacOS builds on PRs
6. Add timing metrics to workflow

**Medium-term actions (this quarter):**
7. Implement precompiled headers
8. Windows-specific optimizations
9. Consider prebuild workflow

**Long-term considerations:**
10. Evaluate incremental builds (carefully)
11. Monitor costs vs. benefits of larger runners
12. Consider self-hosted runners for enterprise use

---

## Document Metadata

- **Created**: 2025-01-XX
- **Author**: GitHub Copilot Analysis
- **Version**: 1.0
- **Status**: Planning Document (NO IMPLEMENTATION)
- **Related Issue**: cmake-multi-platform workflow optimization
- **Next Review**: After Phase 1 implementation
