# Local Development: Using GitHub Packages as vcpkg Binary Cache

This guide explains how to set up your local development environment to use the project's vcpkg binary cache hosted on GitHub Packages.

## Prerequisites

- A GitHub Personal Access Token (PAT) with `read:packages` scope.
- `vcpkg` installed locally.

## Setup Instructions

We provide a script to automate the configuration of your local NuGet credentials.

1. Open PowerShell in the repository root.
2. Run the setup script:
   ```powershell
   .\scripts\setup-vcpkg-cache.ps1
   ```
   Follow the prompts to enter your PAT. The script will:
   - Generate `local.nuget.config` with embedded credentials (gitignored)
   - Optionally add the required env vars to your PowerShell profile

3. The script sets these environment variables (also available to add manually):
   ```powershell
   # Points vcpkg at the local config with embedded GitHub Packages credentials
   $env:VCPKG_BINARY_SOURCES = "clear;nugetconfig,<repo-root>\local.nuget.config,read"
   # Disables VS credential provider (doesn't support GitHub Packages)
   $env:NUGET_PLUGIN_PATHS = " "
   ```
   *Note: The script can automatically add these to your PowerShell profile.*

## How It Works

The setup uses `nugetconfig` mode with a `local.nuget.config` file that contains your PAT as a `ClearTextPassword`. This bypasses the Visual Studio credential provider (CredentialProvider.Microsoft), which only supports Azure Artifacts feeds — not GitHub Packages.

**Important:** Cache hits require matching compiler hashes. If the CI runner uses a different MSVC version than your local machine, you'll get cache misses (vcpkg will build from source). This is normal — the cache will be populated by CI and benefit future CI runs.

## Troubleshooting

### "One or more Nuget credential providers requested manual action"

This error occurs when the VS credential provider intercepts NuGet auth but can't handle GitHub Packages. Fix:

1. Ensure `$env:NUGET_PLUGIN_PATHS` is set to `" "` (space) to disable the credential provider.
2. Use `nugetconfig` mode (not `nuget`) so credentials come from the config file directly.
3. Re-run the setup script to regenerate `local.nuget.config`.

### "Restored 0 packages" (no cache hits)

This is expected when your local compiler version differs from CI. The NuGet feed may have packages from a different MSVC version. The packages are still valid — they'll be used by CI runners with the matching compiler.

### Downloads timing out

If downloads hang or timeout, the credential provider may still be intercepting. Verify:
```powershell
Write-Host $env:NUGET_PLUGIN_PATHS  # Should be " " (space)
Write-Host $env:VCPKG_BINARY_SOURCES  # Should start with "clear;nugetconfig,"
```
