dotnet nuget list source --configfile local.nuget.config

#### 2. Set the vcpkg Environment Variable
In your VS terminal, set the variable to point to the file you just edited. Note the `read` flag at the end to ensure your local build stays in a "consume-only" mode:

**PowerShell:**
```powershell
$env:VCPKG_BINARY_SOURCES="clear;nugetconfig,$PWD\local.nuget.config,read"

**Command Prompt (Developer Command Prompt):**
```cmd
set VCPKG_BINARY_SOURCES=clear;nugetconfig,%CD%\local.nuget.config,read

### Why this works with `dotnet`
vcpkg has built-in logic to find a NuGet provider. If `nuget.exe` isn't found, it automatically looks for `dotnet`. By providing the path to `local.nuget.config`, vcpkg will call `dotnet nuget push/install` under the hood. 

### Final Verification
To confirm it is working, go to **CMake > Delete Cache and Reconfigure** in Visual Studio. Look for the following line in the output:
> `[vcpkg] Detected binary mirror: nugetconfig;C:\...\local.nuget.config;read`

If a binary is found on GitHub, you will see:
> `--- Restoring package vcpkg-XYZ from NuGet ---`

**Next Steps:**
* If you get a "401 Unauthorized" error, double-check that your PAT in `local.nuget.config` hasn't expired and has the `read:packages` scope.
* Would you like me to add this configuration to your `CMakePresets.json` so you don't have to set the environment variable every time you open a new terminal?