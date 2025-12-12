# Using GitHub Packages as a Binary Cache for vcpkg local

$VCPKG_NUGET_BINARY_CACHE_APIT_TOKEN

user-level: <%appdata%\NuGet\NuGet.Config>
projecty-level: <./nuget.config>

``` nuget
<?xml version="1.0" encoding="utf-8"?>
<configuration>
<packageSources>
<add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
<add key="GitHubPackages-OdbDesign" value="https://nuget.pkg.github.com/nam20485/index.json" />
</packageSources>
<packageSourceCredentials>
<GitHubPackages-OdbDesign>
<add key="Username" value="nam20485" />
<add key="ClearTextPassword" value="XXXXXX REPLACE WITH YOUR PAT (DO NOT COMMIT TO GIT!) XXXXX" />
</GitHubPackages-OdbDesign>
</packageSourceCredentials>
<config>
<add key="defaultPushSource" value="https://nuget.pkg.github.com/nam20485/index.json" />
</config>
<apikeys>
<add key="https://nuget.pkg.github.com/nam20485/index.json" value="REPLACE_WITH_GITHUB_PAT_AND_DO_NOT_COMMIT" />
</apikeys>
</configuration>
```
