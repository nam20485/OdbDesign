## vcpkg Binary Cache Plan – GitHub Packages

### References
- Microsoft Learn — Tutorial: Set up a vcpkg binary cache using GitHub Packages (Windows runner pivot)  
  https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-github-packages?pivots=windows-runner
- Microsoft Learn — Tutorial: Set up a vcpkg binary cache using GitHub Packages (Linux runner pivot)  
  https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-github-packages?pivots=linux-runner
- Microsoft Learn — Configure vcpkg to use your NuGet feed (`VCPKG_BINARY_SOURCES` guidance)  
  https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-nuget?source=recommendations&pivots=shell-powershell#3---configure-vcpkg-to-use-your-nuget-feed
- Repository files captured locally for implementation details  
  - `github-packages.md` (local copy of Microsoft Learn tutorial)  
  - `binary-caching-nuget.md` (local copy of NuGet configuration doc)  
  - `vcpkg-configuration.json` (current repo registry configuration)

### Objectives
1. Enable vcpkg binary caching through GitHub Packages for both Windows and Linux GitHub Actions runners.
2. Ensure authentication leverages `GITHUB_TOKEN` (or PAT fallback) with least-effort secrets management.
3. Keep existing `vcpkg-configuration.json` registries untouched while guaranteeing every invocation of vcpkg inside CI uses the new cache.
4. Document environment/workflow changes so developers and automation stay in sync.

### Implementation Plan
1. **Workflow Permissions & Tokens**
   - Add `permissions: packages: write` at the workflow/job root (`.github/workflows/cmake-multi-platform.yml`). Source: `github-packages.md`, lines 40-48 & 121-184.
   - Prefer `${{ secrets.GITHUB_TOKEN }}`; if repo needs cross-repo cache access, add PAT secret (`VCPKG_PAT_TOKEN`) with `packages:read`, `packages:write`. Source: `github-packages.md`, lines 188-199.

2. **Shared Environment Block**
   - Define workflow-level `env` entries per tutorial (`github-packages.md`, lines 92-109):
     ```yaml
     env:
       USERNAME: <org-or-user>
       VCPKG_EXE: ${{ github.workspace }}/vcpkg/vcpkg
       FEED_URL: https://nuget.pkg.github.com/<org-or-user>/index.json
       VCPKG_BINARY_SOURCES: "clear;nuget,https://nuget.pkg.github.com/<org-or-user>/index.json,readwrite"
     ```
   - If we decide to version a `nuget.config`, switch to `clear;nugetconfig,<path>` per `binary-caching-nuget.md`, lines 229-243.

3. **Bootstrap vcpkg**
   - Windows step: PowerShell invoking `bootstrap-vcpkg.bat`. Linux step: bash invoking `bootstrap-vcpkg.sh`. Source: `github-packages.md`, lines 67-90.

4. **NuGet CLI Availability**
   - Windows: `vcpkg fetch nuget` returns `nuget.exe`; run directly in PowerShell.
   - Linux: install/use `mono` for `nuget.exe` (`github-packages.md`, lines 148-181). Add `apt install mono-complete` for ubuntu-24.04+ runners.

5. **Configure NuGet Source in Workflow**
   - Windows step (PowerShell):
     ```pwsh
     .$($env:VCPKG_EXE fetch nuget) sources add `
       -Source "$env:FEED_URL" `
       -StorePasswordInClearText `
       -Name GitHubPackages `
       -UserName "$env:USERNAME" `
       -Password "${{ secrets.GITHUB_TOKEN }}"
     .$($env:VCPKG_EXE fetch nuget) setapikey "${{ secrets.GITHUB_TOKEN }}" `
       -Source "$env:FEED_URL"
     ```
   - Linux step (bash):
     ```bash
     mono "$(${ { env.VCPKG_EXE }} fetch nuget | tail -n 1)" \
       sources add \
       -Source "$FEED_URL" \
       -StorePasswordInClearText \
       -Name GitHubPackages \
       -UserName "$USERNAME" \
       -Password "${{ secrets.GITHUB_TOKEN }}"
     mono "$(${ { env.VCPKG_EXE }} fetch nuget | tail -n 1)" \
       setapikey "${{ secrets.GITHUB_TOKEN }}" \
       -Source "$FEED_URL"
     ```
   - Replace password argument with `${{ secrets.VCPKG_PAT_TOKEN }}` if PAT required.
   - Reference: `github-packages.md`, lines 113-184.

6. **Optional `nuget.config` for Shared Settings**
   - Add file (e.g., `ci/nuget/github-packages.config`) matching template from `binary-caching-nuget.md`, lines 174-214.
   - Include `<packageSources>`, `<packageSourceCredentials>`, `<defaultPushSource>` entries with placeholders. Keep credentials blank; workflow populates via CLI.
   - Update `VCPKG_BINARY_SOURCES` to `clear;nugetconfig,${{ github.workspace }}/ci/nuget/github-packages.config`.

7. **Integrate with `vcpkg-configuration.json`**
   - No schema support for binary caches; maintain current registry definitions (`vcpkg-configuration.json`, lines 1-9).
   - Document in README/plan that binary caching is controlled via workflow env vars; local developers should run `export/set VCPKG_BINARY_SOURCES=...` mirroring CI (`binary-caching-nuget.md`, lines 229-269).

8. **Validation & Next Steps**
   - After workflow updates, trigger CI runs on both runners to confirm packages upload/download from GitHub feed.
   - Monitor for `mono` availability issues on Linux; cache `nuget.exe` path using workflow outputs if repeated invocations become costly.

### Review Feedback (2025-11-17)
- **macOS runner coverage** – The current plan only calls out Windows + Linux but the workflow runs on `macos-14`. Document whether macOS will adopt the cache (and how NuGet CLI/mono is installed) or explicitly scope it out.
- **Permissions & forked PRs** – Root workflow lacks `packages: write` while the job block sets it; forked pull requests still get read-only tokens. Clarify how cache publishing is gated (e.g., trusted events only) and whether fork builds fall back to `x-gha` caching.
- **Environment values** – Replace `<org-or-user>` placeholders with `${{ github.repository_owner }}` (lowercase as required by feeds) and make `VCPKG_EXE` path OS-aware so Windows resolves `vcpkg.exe` correctly.
- **Binary source strategy** – Instead of dropping `x-gha`, keep it as the first entry (e.g., `clear;x-gha,readwrite;nuget,...`) so NuGet is only hit on cache misses.
- **Bootstrap flow** – The workflow already clones vcpkg once for every matrix entry and runs the Unix bootstrap script even on Windows. Spell out platform-specific bootstrap commands and whether cloning can be shared to avoid redundant work.
- **NuGet CLI commands** – The snippets should use tested syntax (`& "$env:VCPKG_EXE" fetch nuget`, `mono "$VCPKG_EXE" fetch nuget`) and handle idempotency (`sources update` or removing existing sources) to avoid failures on reruns.
- **NuGet tooling footprint** – Installing `mono-complete` on Ubuntu adds significant runtime. Consider lighter alternatives (`mono-runtime`, `dotnet nuget`, or caching `nuget.exe`) and capture the trade-offs.
- **nuget.config option** – If a shared config is committed, document its location, how credentials remain out of source control, and how local developers consume it; if generated dynamically, note the workflow steps.
- **Validation signals** – Expand Step 8 with explicit acceptance criteria (logs that show upload/download, artifact count in GitHub Packages, sample run IDs) so we know the cache is working before relying on it.
