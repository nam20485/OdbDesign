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
1. Enable vcpkg binary caching through GitHub Packages for Windows, Linux, and macOS GitHub Actions runners.
2. Ensure robust authentication using a dedicated Personal Access Token (PAT) for write access in CI, while allowing read-only access for other scenarios.
3. Keep existing `vcpkg-configuration.json` registries untouched while guaranteeing every invocation of vcpkg inside CI uses the new cache.
4. Provide clear, secure documentation for both CI and local development setup.

### Implementation Plan

1. **Workflow Permissions & Authentication**
   - Add `permissions: packages: write` at the workflow/job root (`.github/workflows/cmake-multi-platform.yml`) to allow `GITHUB_TOKEN` to read packages.
   - **Primary Authentication:** For writing to the cache, a **Personal Access Token (PAT)** with `packages:read` and `packages:write` scopes is required.
     - Create a PAT and add it as a repository secret named `VCPKG_PAT_TOKEN`.
     - **Security:** The workflow will be configured to only grant write access on trusted events (e.g., pushes to `main` or `develop`). Pull requests from forks will use the read-only `GITHUB_TOKEN`, allowing them to consume, but not write to, the cache.

2. **Shared Environment Block**
   - Define workflow-level `env` entries in `cmake-multi-platform.yml`:
     ```yaml
     env:
       USERNAME: ${{ github.repository_owner }}
       VCPKG_ROOT: ${{ github.workspace }}/vcpkg
       FEED_URL: https://nuget.pkg.github.com/${{ github.repository_owner }}/index.json
       # Hybrid approach: Use GitHub Actions Cache (x-gha) first for speed, then GitHub Packages (nuget) for persistence.
       # CI will use a dynamically generated config file.
       VCPKG_BINARY_SOURCES: "clear;x-gha,readwrite;nugetconfig,${{ github.workspace }}/nuget.generated.config,readwrite"
     ```

3. **Bootstrap vcpkg**
   - The existing workflow step that clones and bootstraps vcpkg will be used for all platforms.

4. **NuGet CLI Availability**
   - **Windows:** `vcpkg fetch nuget` provides `nuget.exe`, which can be run directly.
   - **Linux/macOS:** `mono` is required to run `nuget.exe`.
     - **Ubuntu:** Add a step to run `sudo apt-get update && sudo apt-get install -y mono-devel`.
     - **macOS:** Add a step to run `brew install mono`.

5. **CI NuGet Configuration**
   - **Dynamically generate `nuget.generated.config` in the workflow.** This avoids committing any credentials.
   - Add a workflow step to create the file with the necessary content, injecting the PAT from secrets.
     ```bash
     # This script runs for all OS types in the matrix
     echo '<?xml version="1.0" encoding="utf-8"?>
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
     </configuration>' > nuget.generated.config
     ```
   - This single step replaces the platform-specific `nuget sources update` and `setapikey` commands, simplifying the workflow.

6. **Repository `nuget.config` (for Local Development)**
   - A minimal, **credential-less** `nuget.config` will be committed to the repository root. Its sole purpose is to define the package source URL for local users.
     ```xml
     <?xml version="1.0" encoding="utf-8"?>
     <configuration>
       <packageSources>
         <clear />
         <add key="GitHubPackages" value="https://nuget.pkg.github.com/nam20485/index.json" />
       </packageSources>
     </configuration>
     ```

7. **Local Development Setup**
   - To use the binary cache, developers must first authenticate their local NuGet client to GitHub Packages. **This is a one-time setup.**
     - **Instructions:**
       1. Create a **Personal Access Token (PAT)** with `packages:read` scope.
       2. Add the credentials to your **user-level** NuGet configuration (do NOT modify the file in the repository):
          ```bash
          # For Linux/macOS/PowerShell. Get nuget.exe via `vcpkg fetch nuget`
          # Replace <USER> with your GitHub username and <PAT> with your token.
          nuget sources Add -Name "GitHubPackages" -Source "https://nuget.pkg.github.com/nam20485/index.json" -UserName <USER> -Password <PAT> -StorePasswordInClearText
          ```
   - **To enable the cache for the project:**
     - Set the `VCPKG_BINARY_SOURCES` environment variable to point to the repository's `nuget.config` file. NuGet will automatically find the credentials in your user-level config.
       - **Linux/macOS:** `export VCPKG_BINARY_SOURCES="clear;nugetconfig,$(pwd)/nuget.config,read"`
       - **PowerShell:** `$env:VCPKG_BINARY_SOURCES="clear;nugetconfig,$(pwd)/nuget.config,read"`
     - Note: Local access is configured as `read`-only by default to prevent accidental uploads.

8. **`vcpkg-configuration.json` Integration**
   - No changes are needed. Binary caching is controlled entirely by environment variables and is independent of port registries.

9. **Validation & Next Steps**
   - After updating the workflow, trigger a run on a trusted branch (e.g., `main`).
   - **Acceptance Criteria:**
     - Verify that the workflow logs show packages being downloaded from the NuGet source (`Uploading binaries to nuget cache...` and `Restored 0 packages from nuget cache...` messages).
     - Check the repository's "Packages" section on GitHub to confirm that new packages have been published.
     - Subsequent runs should show packages being restored from the cache (`Restored X packages from nuget cache...`).

10. **Future Considerations**
    - **`mono` Dependency:** To reduce workflow setup time on Linux and macOS, investigate using the `dotnet nuget` command if the .NET SDK is available on runners, as this would remove the `mono` dependency.
