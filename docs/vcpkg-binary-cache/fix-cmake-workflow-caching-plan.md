# Fix CMake Workflow Binary Caching - Implementation Plan

## Executive Summary

The `CMake Build Multi-Platform` workflow has been experiencing excessively long build times (1-2+ hours) due to broken binary caching configuration. This document outlines the root cause analysis and the corrected hybrid caching approach to restore fast builds.

---

## History & Context

### Original Implementation (PR #501 - Nov 2025)

**Author:** @nam204  
**Feature Branch:** `feature/grpc`  
**Plan Document:** [vcpkg-github-packages-plan.md](./vcpkg-github-packages-plan.md)

A sophisticated hybrid caching strategy was designed to balance speed and persistence:

```yaml
VCPKG_BINARY_SOURCES: "clear;x-gha,readwrite;nugetconfig,${{ github.workspace }}/nuget.generated.config,readwrite"
```

**Components:**
1. **`x-gha`** - GitHub Actions built-in cache (fast, ephemeral)
2. **`nugetconfig`** - GitHub Packages NuGet feed (persistent, shared across branches)

**Supporting Infrastructure:**
- Dynamic `nuget.generated.config` creation with PAT authentication
- `mono` installation for Linux/macOS NuGet CLI support
- `permissions: packages: write` for GITHUB_TOKEN

---

## What Went Wrong

### Issue 1: Deprecated `x-gha` Backend

**Error Message:**
```
warning: The 'x-gha' binary caching backend has been removed. 
Consider using a NuGet-based binary caching provider instead.
```

**Impact:** vcpkg silently fell back to source builds, ignoring the cache entirely.

**Timeline:** The `x-gha` backend was deprecated and removed in recent vcpkg versions after the original plan was implemented.

### Issue 2: Empty NuGet Feed (Cache Misses)

**Error Messages:**
```
Unable to find version '...' of package 'vcpkg-cmake-get-vars_arm64-osx'
  https://nuget.pkg.github.com/nam20485/index.json: Package not found

Unable to find version '...' of package 'openssl_arm64-osx'
  https://nuget.pkg.github.com/nam20485/index.json: Package not found
```

**Root Cause:** The GitHub Packages NuGet feed is empty because:
- The workflow hasn't had a successful cache-write run yet
- PR builds from forks only have read access (by design)
- The `VCPKG_PAT_TOKEN` secret may not be configured

**Impact:** Every build attempts to restore from NuGet, fails, and falls back to building from source (2+ hours).

### Issue 3: Recent Regression (Feb 2025)

A commit attempted to "fix" the caching by:
1. Changing template overloads → variadic functions in `HttpTraceMiddleware.h`
2. This introduced compilation errors due to ambiguous function calls

This compounded the caching issue by causing build failures.

---

## Corrected Plan

### Philosophy

> **The hybrid approach is still correct** - we just need to update the GitHub Actions backend from deprecated `x-gha` to supported `actions`.

### Updated Configuration

```yaml
env:
  VCPKG_ROOT: ${{ github.workspace }}/vcpkg
  # Hybrid cache: GH Actions cache (fast) + GitHub Packages (persistent)
  VCPKG_BINARY_SOURCES: "clear;actions,readwrite;nugetconfig,${{ github.workspace }}/nuget.generated.config,readwrite"
  GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
  VCPKG_FEATURE_FLAGS: dependencygraph
  ARTIFACTS_DIR: ${{ github.workspace }}/artifacts
  ARTIFACTS_DIR_WIN: ${{ github.workspace }}\artifacts
  # GitHub Packages NuGet feed configuration
  USERNAME: ${{ github.repository_owner }}
  FEED_URL: https://nuget.pkg.github.com/${{ github.repository_owner }}/index.json
```

### Key Changes from Original Plan

| Aspect | Original | Corrected | Reason |
|--------|----------|-----------|--------|
| Actions backend | `x-gha` | `actions` | `x-gha` deprecated by vcpkg |
| NuGet backend | `nugetconfig` | `nugetconfig` | Unchanged - still correct |
| Order | `x-gha` first | `actions` first | Fast cache checked first |
| Write permissions | PAT for NuGet | PAT for NuGet | Unchanged |

### Required Secrets

**`VCPKG_PAT_TOKEN`** - Personal Access Token with:
- `packages:read` scope (for all builds)
- `packages:write` scope (for trusted branch builds)

**Configuration:**
1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Generate token with `packages:read` and `packages:write`
3. Add as repository secret: `VCPKG_PAT_TOKEN`

### Workflow Steps to Restore

1. **Environment Variables** (already in workflow)
   - `USERNAME`, `FEED_URL`, `VCPKG_BINARY_SOURCES`

2. **NuGet Config Generation** (needs restoration)
   ```yaml
   - name: Generate NuGet Config for Binary Cache
     shell: bash
     run: |
       cat > nuget.generated.config << 'EOF'
       <?xml version="1.0" encoding="utf-8"?>
       <configuration>
         <packageSources>
           <clear />
           <add key="GitHubPackages" value="${{ env.FEED_URL }}" />
         </packageSources>
         <packageSourceCredentials>
           <GitHubPackages>
             <add key="Username" value="${{ env.USERNAME }}" />
             <add key="ClearTextPassword" value="${{ secrets.VCPKG_PAT_TOKEN }}" />
           </GitHubPackages>
         </packageSourceCredentials>
         <apiKeys>
           <add key="${{ env.FEED_URL }}" value="${{ secrets.VCPKG_PAT_TOKEN }}" />
         </apiKeys>
       </configuration>
       EOF
   ```

3. **Platform Dependencies** (already in workflow)
   - `mono` for macOS (`brew install mono`)
   - `mono-devel` for Ubuntu (`apt install mono-devel`)

---

## Expected Behavior

### First Run (Cold Cache)

```
[actions] Cache miss - building from source
Building vcpkg-cmake-get-vars_arm64-osx...
Building openssl_arm64-osx...
...
[nuget] Uploading binaries to nuget cache...
```

**Time:** ~1-2 hours  
**Result:** Both caches populated

### Second Run (Warm Actions Cache)

```
[actions] Restored 50 packages from GitHub Actions cache
[nuget] Skipped - packages already available locally
```

**Time:** ~5-10 minutes  
**Result:** Fast build using Actions cache

### Cross-Branch/PR Build (Actions Cache Miss, NuGet Hit)

```
[actions] Cache miss (different branch)
[nuget] Restored 50 packages from nuget cache
```

**Time:** ~10-15 minutes  
**Result:** Shared cache from NuGet feed

### After 7 Days Inactivity

```
[actions] Cache expired
[nuget] Restored 50 packages from nuget cache
```

**Time:** ~10-15 minutes  
**Result:** Persistent cache survives in NuGet feed

---

## Risk Mitigation

### Risk 1: PAT Not Configured

**Impact:** NuGet writes fail, only Actions cache works  
**Mitigation:** 
- Builds still work (just slower for cross-branch)
- Add visible warning in workflow logs
- Document setup in README

### Risk 2: `actions` Backend Deprecated

**Impact:** Future vcpkg changes break caching  
**Mitigation:**
- Monitor vcpkg release notes
- Design allows easy backend swap
- Fallback to source builds still works

### Risk 3: NuGet Feed Permissions

**Impact:** Fork PRs can't write to cache  
**Mitigation:**
- By design - forks should only read
- First trusted branch build populates cache
- Document trusted branch strategy

---

## Success Criteria

1. ✅ Workflow completes in < 15 minutes on cache hit
2. ✅ Workflow completes in < 2 hours on cold cache
3. ✅ No deprecation warnings about `x-gha`
4. ✅ NuGet packages visible in GitHub Packages section
5. ✅ Cross-branch builds benefit from shared cache

---

## Implementation Checklist

- [ ] Restore `USERNAME` and `FEED_URL` environment variables
- [ ] Update `VCPKG_BINARY_SOURCES` to use `actions` instead of `x-gha`
- [ ] Restore NuGet config generation step
- [ ] Verify `VCPKG_PAT_TOKEN` secret exists
- [ ] Test on `feature/grpc` branch
- [ ] Monitor first build (expect 1-2 hours)
- [ ] Verify second build is fast (< 15 min)
- [ ] Check GitHub Packages for uploaded NuGet packages

---

## Related Documents

- [Original Plan](./vcpkg-github-packages-plan.md)
- [Microsoft Learn: Binary Caching GitHub Packages](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-github-packages)
- [Microsoft Learn: Binary Caching NuGet](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-nuget)

---

**Prepared:** 2026-02-11  
**Status:** Pending Approval  
**Proposed By:** Kimi Code CLI
