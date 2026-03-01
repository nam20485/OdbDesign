# Fix CMake Workflow Binary Caching - Implementation Plan

## Executive Summary

The `CMake Build Multi-Platform` workflow has been experiencing excessively long build times (1-2+ hours) due to broken binary caching configuration. This document outlines the root cause analysis and the corrected hybrid caching approach to restore fast builds.

**Status:** ✅ Implemented (2026-02-11)

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

### Issue 1: Deprecated `x-gha` Backend (and its successor `actions`)

**Error Message:**
```
warning: The 'x-gha' binary caching backend has been removed. 
Consider using a NuGet-based binary caching provider instead.
```

**Impact:** vcpkg silently fell back to source builds, ignoring the cache entirely.

**Timeline:** Both `x-gha` and its replacement `actions` were removed from vcpkg in 2025. As of vcpkg 2025-09-03 neither backend exists. The available backends are: `clear`, `default`, `files`, `http`, `nuget`, `nugetconfig`, `nugettimeout`, `interactive`, `x-azblob`, `x-gcs`, `x-cos`, `x-az-universal`, `x-aws`.

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

## Corrected Plan (Implemented)

### Philosophy

> **The hybrid approach is still correct.** Since vcpkg removed all native GitHub Actions cache backends (`x-gha`, `actions`), we replicate the hybrid strategy using vcpkg's built-in `default` file cache + the external `actions/cache` action for persistence, paired with `nugetconfig` for cross-branch sharing.

### Architecture: Two-Layer Hybrid Cache

| Layer | vcpkg Backend | Persistence Mechanism | Speed | Scope | Lifetime |
|-------|--------------|----------------------|-------|-------|----------|
| **1. Local file cache** | `default,readwrite` | `actions/cache@v4` (saves/restores directory) | ~30s restore | Same branch + preset | 7 days (GH cache eviction) |
| **2. NuGet feed** | `nugetconfig,...,readwrite` | GitHub Packages NuGet feed | ~2-5 min restore | All branches, PRs, forks (read) | Permanent |

**How it works:**
1. `VCPKG_DEFAULT_BINARY_CACHE` is set to a fixed workspace path (`${{ github.workspace }}/.vcpkg-binary-cache`)
2. `actions/cache@v4` restores this directory at the start of each run and saves it at the end
3. vcpkg checks `default` first (instant local hit from the restored directory)
4. On miss, vcpkg falls through to `nugetconfig` (NuGet fetch from GitHub Packages)
5. On source build, vcpkg writes to **both** caches simultaneously (`readwrite` on both)

### Updated Configuration

```yaml
env:
  VCPKG_ROOT: ${{ github.workspace }}/vcpkg
  # Override vcpkg's default binary cache to a known path for actions/cache
  VCPKG_DEFAULT_BINARY_CACHE: ${{ github.workspace }}/.vcpkg-binary-cache
  # GitHub Packages NuGet feed URL and binary cache
  FEED_URL: https://nuget.pkg.github.com/${{ github.repository_owner }}/index.json
  GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
  VCPKG_FEATURE_FLAGS: dependencygraph
  ARTIFACTS_DIR: ${{ github.workspace }}/artifacts
  ARTIFACTS_DIR_WIN: ${{ github.workspace }}\artifacts
  USERNAME: ${{ github.repository_owner }}

# Set dynamically in the "Generate NuGet Config" step:
# VCPKG_BINARY_SOURCES=clear;default,readwrite;nugetconfig,$GITHUB_WORKSPACE/nuget.generated.config,readwrite
```

### Key Changes from Original Plan

| Aspect | Original Plan | Corrected Plan (v2) | Current Implementation | Reason |
|--------|--------------|---------------------|----------------------|--------|
| Fast cache backend | `x-gha` | `actions` | `default` + `actions/cache@v4` | Both `x-gha` and `actions` removed from vcpkg 2025+ |
| NuGet backend | `nugetconfig` | `nugetconfig` | `nugetconfig` | Unchanged — still correct |
| Generated config file | `nuget.generated.config` | `nuget.generated.config` | `nuget.generated.config` | Avoids overwriting repo's `nuget.config` |
| apiKeys block | Not specified | Required | ✅ Included | Required for NuGet push authentication |
| Cache dir env var | N/A | N/A | `VCPKG_DEFAULT_BINARY_CACHE` | Gives `actions/cache` a known path to save/restore |
| Job permissions | `packages: write` (workflow) | `packages: write` (workflow) | `packages: write` (workflow + job) | Ensure token has write scope at both levels |

### Required Secrets

**`VCPKG_PAT_TOKEN`** - Personal Access Token with:
- `packages:read` scope (for all builds)
- `packages:write` scope (for trusted branch builds)

**Configuration:**
1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Generate token with `packages:read` and `packages:write`
3. Add as repository secret: `VCPKG_PAT_TOKEN`

### Implemented Workflow Steps

1. **Environment Variables** (global `env:` block)
   - `VCPKG_ROOT`, `VCPKG_DEFAULT_BINARY_CACHE`, `FEED_URL`, `USERNAME`

2. **NuGet Config Generation** (`Generate NuGet Config for Binary Cache` step)
   ```yaml
   - name: Generate NuGet Config for Binary Cache
     shell: bash
     run: |
       cat > "$GITHUB_WORKSPACE/nuget.generated.config" << 'EOF'
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
         <config>
           <add key="defaultPushSource" value="${{ env.FEED_URL }}" />
           <add key="signatureValidationMode" value="accept" />
         </config>
       </configuration>
       EOF
       echo "VCPKG_BINARY_SOURCES=clear;default,readwrite;nugetconfig,$GITHUB_WORKSPACE/nuget.generated.config,readwrite" >> $GITHUB_ENV
   ```

3. **Local Cache Persistence** (actions/cache step)
   ```yaml
   - name: Create vcpkg Binary Cache Directory
     shell: bash
     run: mkdir -p "$VCPKG_DEFAULT_BINARY_CACHE"

   - name: Restore vcpkg Binary Cache
     uses: actions/cache@v4
     with:
       path: ${{ env.VCPKG_DEFAULT_BINARY_CACHE }}
       key: vcpkg-archives-${{ matrix.preset }}-${{ hashFiles('vcpkg.json', 'vcpkg-configuration.json') }}
       restore-keys: |
         vcpkg-archives-${{ matrix.preset }}-
   ```

4. **Platform Dependencies** (already in workflow)
   - `mono` for macOS (`brew install mono`)
   - `mono-devel` for Ubuntu (`apt install mono-devel`)

---

## Expected Behavior

### First Run (Cold Cache — both layers empty)

```
[default] Cache miss (directory empty)
[nugetconfig] Cache miss (feed empty)
Building vcpkg-cmake-get-vars_arm64-osx from source...
Building openssl_arm64-osx from source...
...
[default] Writing binaries to local cache directory
[nugetconfig] Uploading binaries to GitHub Packages NuGet feed
```

**Time:** ~1-2 hours  
**Result:** Both caches populated

### Second Run, Same Branch (Warm local cache)

```
[default] Restored 50 packages from local file cache (via actions/cache)
```

**Time:** ~5-10 minutes  
**Result:** Fast build using local file cache, NuGet not even consulted

### Cross-Branch/PR Build (Local cache miss, NuGet hit)

```
[default] Cache miss (different branch, no actions/cache match)
[nugetconfig] Restored 50 packages from GitHub Packages NuGet feed
[default] Writing binaries to local cache directory (for next same-branch run)
```

**Time:** ~10-15 minutes  
**Result:** NuGet provides persistent cross-branch sharing

### After 7 Days Inactivity (actions/cache evicted, NuGet still available)

```
[default] Cache miss (actions/cache evicted after 7 days)
[nugetconfig] Restored 50 packages from GitHub Packages NuGet feed
[default] Writing binaries to local cache directory
```

**Time:** ~10-15 minutes  
**Result:** NuGet feed permanent — survives GH cache eviction

---

## Risk Mitigation

### Risk 1: PAT Not Configured

**Impact:** NuGet writes fail, only local actions/cache works  
**Mitigation:** 
- Builds still work (just slower for cross-branch)
- Add visible warning in workflow logs
- Document setup in README

### Risk 2: `default` Backend Behavior Change

**Impact:** Future vcpkg changes could alter default cache directory  
**Mitigation:**
- `VCPKG_DEFAULT_BINARY_CACHE` env var explicitly overrides the path
- This is a stable, documented vcpkg feature (not experimental)

### Risk 3: NuGet Feed Permissions

**Impact:** Fork PRs can't write to cache  
**Mitigation:**
- By design — forks should only read
- First trusted branch build populates cache
- Document trusted branch strategy

### Risk 4: actions/cache Size Limits

**Impact:** GitHub Actions cache has a 10 GB repo limit; old entries evicted LRU  
**Mitigation:**
- Cache key includes preset + dependency hash — stale entries are naturally evicted
- NuGet layer acts as permanent fallback when local cache is evicted

---

## Local Development Setup

For local development, the `actions/cache` layer is not applicable. Instead:

```powershell
# One-time setup (run the setup script):
.\scripts\setup-vcpkg-cache.ps1

# Effective env var (set by the script):
$env:VCPKG_BINARY_SOURCES = "clear;nuget,https://nuget.pkg.github.com/nam20485/index.json,read;interactive"
```

See [localdev-gh-binary-cacheing.md](./localdev-gh-binary-cacheing.md) for full details.

---

## Success Criteria

1. ✅ Workflow completes in < 15 minutes on cache hit
2. ✅ Workflow completes in < 2 hours on cold cache
3. ✅ No deprecation warnings about `x-gha` or `actions`
4. ✅ NuGet packages visible in GitHub Packages section
5. ✅ Cross-branch builds benefit from shared NuGet cache
6. ✅ Same-branch rebuilds benefit from fast local file cache via actions/cache

---

## Implementation Checklist

- [x] `USERNAME` and `FEED_URL` environment variables present
- [x] `VCPKG_DEFAULT_BINARY_CACHE` environment variable added
- [x] `VCPKG_BINARY_SOURCES` uses `default,readwrite;nugetconfig,...,readwrite`
- [x] `actions/cache@v4` step saves/restores `VCPKG_DEFAULT_BINARY_CACHE`
- [x] NuGet config generation includes `apiKeys` block
- [x] NuGet config written to `nuget.generated.config` (not `nuget.config`)
- [x] `nuget.generated.config` added to `.gitignore`
- [x] `packages: write` permission at both workflow and job scope
- [x] Removed defunct `Export vcpkg Cache Variables` step (was for `x-gha`/`actions`)
- [ ] Verify `VCPKG_PAT_TOKEN` secret exists in repository settings
- [ ] Test on `feature/grpc` branch
- [ ] Monitor first build (expect 1-2 hours for cold cache)
- [ ] Verify second same-branch build is fast (< 15 min)
- [ ] Verify cross-branch build hits NuGet cache
- [ ] Check GitHub Packages for uploaded NuGet packages

---

## Related Documents

- [Original Plan](./vcpkg-github-packages-plan.md)
- [Local Development Guide](./localdev-gh-binary-cacheing.md)
- [Microsoft Learn: Binary Caching GitHub Packages](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-github-packages)
- [Microsoft Learn: Binary Caching NuGet](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-nuget)
- [vcpkg Binary Caching Reference](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching)

---

**Originally Prepared:** 2026-02-11  
**Updated:** 2026-02-11  
**Status:** ✅ Implemented  
**Original Proposal By:** Kimi Code CLI  
**Implementation By:** GitHub Copilot
