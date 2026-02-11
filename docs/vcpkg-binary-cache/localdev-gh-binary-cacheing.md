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
   Follow the prompts to enter your PAT. This script will securely add your credentials to your user-level NuGet configuration.

3. Configure vcpkg to use the cache by setting the environment variable (as output by the script):
   ```powershell
   $env:VCPKG_BINARY_SOURCES = "clear;nuget,https://nuget.pkg.github.com/nam20485/index.json,read;interactive"
   ```
   *Note: The script can automatically add this to your PowerShell profile.*

## Troubleshooting

### "One or more Nuget credential providers requested manual action"
This error occurs if vcpkg cannot authenticate with the package feed. Ensure:
1. You have run the setup script and provided a valid PAT.
2. Your `VCPKG_BINARY_SOURCES` environment variable includes `;interactive`.
3. If the error persists, try running a build in a terminal that supports interactivity to complete the authentication flow.
