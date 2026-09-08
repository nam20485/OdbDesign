# AGENTS.md - Coding Agent Instructions for OdbDesign

## Project Overview

OdbDesign is a C++ library for parsing and working with ODB++ design files (PCB manufacturing format). It uses CMake for building, vcpkg for dependency management, and GoogleTest for testing.

---

## Build Commands

### Prerequisites

- CMake 3.21+
- vcpkg (set `VCPKG_ROOT` environment variable)
- C++17 compiler (MSVC, GCC, or Clang)

### Configure (Linux)

```bash
cmake --preset linux-debug             # Debug build
cmake --preset linux-dynamic-release   # Release build (main Linux release; shared protobuf/gRPC)
```

### Configure (Windows)

```bash
cmake --preset x64-debug        # Debug build
cmake --preset x64-release      # Release build
```

### Build

```bash
cmake --build --preset linux-debug
cmake --build --preset linux-dynamic-release
cmake --build --preset x64-release
```

### Clean Build

```bash
cmake --build --preset linux-debug --clean-first
```

---

## Test Commands

### Run All Tests

```bash
ctest --preset linux-debug
ctest --preset linux-dynamic-release
ctest --preset x64-debug
```

### Run Single Test

```bash
# Using ctest with test name pattern
ctest --preset linux-debug -R <TestName>

# Example: Run specific test
ctest --preset linux-debug -R BasicAssertions
ctest --preset linux-debug -R Test_DesignOdb

# Run tests with verbose output
ctest --preset linux-debug -R <TestName> -V

# Run test executable directly
./out/build/linux-debug/OdbDesignTests/OdbDesignTests --gtest_filter=<TestSuite>.<TestName>

# Example: Run single test directly
./out/build/linux-debug/OdbDesignTests/OdbDesignTests --gtest_filter=TestTest.BasicAssertions
./out/build/linux-debug/OdbDesignTests/OdbDesignTests --gtest_filter=FileArchiveLoadFixture.Test_SampleDesign*
```

### Run Tests in Parallel

```bash
ctest --preset linux-debug -j$(nproc)
```

---

## Linting & Static Analysis

- The project uses SonarLint integration via `compile_commands.json`
- Compilation database is generated automatically by CMake (`CMAKE_EXPORT_COMPILE_COMMANDS ON`)
- Compiler warnings enabled: `-Wall -Wextra -Wpedantic` (GCC/Clang), `/W4` (MSVC)

---

## Code Style Guidelines

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes/Structs | PascalCase | `Design`, `FileArchive`, `NetRecord` |
| Functions/Methods | PascalCase | `GetNets()`, `ParseFileModel()`, `BuildNets()` |
| Member variables | m_ prefix + camelCase | `m_name`, `m_pFileModel`, `m_netsByName` |
| Local variables | camelCase | `pNetRecord`, `componentNumber` |
| Constants | SCREAMING_SNAKE_CASE or inline static | `NONE_NET_NAME`, `COMMENT_TOKEN` |
| Namespaces | PascalCase, nested | `Odb::Lib::ProductModel`, `Odb::Test` |
| Enums | PascalCase for enum, UPPER for values | `BoardSide::Top`, `Polarity::Positive` |
| Type aliases | PascalCase + typedef | `Vector`, `StringMap` |

### File Organization

```
OdbDesign/
├── OdbDesignLib/           # Main library
│   ├── FileModel/          # File parsing models
│   │   └── Design/         # ODB++ design file parsers
│   ├── ProductModel/       # High-level design objects
│   ├── App/                # Application utilities (cache, routes)
│   └── protoc/             # Protocol buffer definitions
├── OdbDesignServer/        # gRPC server
├── OdbDesignApp/           # CLI application
├── OdbDesignTests/         # Test suite
│   └── Fixtures/           # Test fixtures
└── Utils/                  # Shared utilities
```

### Header Files

```cpp
#pragma once  // Always use #pragma once, not include guards

// Includes: relative paths from component root
#include "../odbdesign_export.h"
#include <string>
#include <memory>
#include <vector>

namespace Odb::Lib::ProductModel
{
    class ODBDESIGN_EXPORT Design : public IProtoBuffable<Protobuf::ProductModel::Design>
    {
        // ...
    };
}
```

### Implementation Files

```cpp
#include "Design.h"  // Own header first
#include "Package.h"
#include "Logger.h"
#include "../enums.h"
#include <memory>

namespace Odb::Lib::ProductModel
{
    // Implementation...
}
```

### Include Order

1. Corresponding header (e.g., `Design.cpp` includes `"Design.h"` first)
2. Project headers (relative paths)
3. System/STL headers (`<string>`, `<vector>`, `<memory>`)
4. External library headers (`<gtest/gtest.h>`)

### Smart Pointers

- Use `std::shared_ptr` for shared ownership
- Use `std::unique_ptr` for exclusive ownership
- Use `std::make_shared` and `std::make_unique`

```cpp
auto pDesign = std::make_shared<Design>();
auto pMessage = std::make_unique<Protobuf::ProductModel::Design>();
```

### Error Handling

- Return `bool` for success/failure in parsing/building methods
- Use `nullptr` checks for pointer parameters
- Use custom `parse_error` exception for file parsing errors

```cpp
bool Design::Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel)
{
    if (pFileModel == nullptr) return false;
    // ...
}

// Parsing errors
throw_parse_error(m_path, line, token, lineNumber);
```

### Const Correctness

- Mark getters as `const`
- Use `const auto&` for iterating over collections

```cpp
const Net::Vector& GetNets() const;
const std::string& GetName() const;

for (const auto& pNet : m_nets) { /* ... */ }
```

### Type Definitions

Define `Vector` and `StringMap` typedefs for container types:

```cpp
typedef std::vector<std::shared_ptr<Design>> Vector;
typedef std::map<std::string, std::shared_ptr<Design>> StringMap;
```

### Constants

Use `constexpr inline static` for class constants:

```cpp
constexpr inline static const char* NONE_NET_NAME = "$NONE$";
constexpr inline static bool CLIP_FILEMODEL_AFTER_BUILD = false;
```

---

## Testing Patterns

### Test Fixture Pattern

```cpp
class TestDataFixture : public testing::Test
{
public:
    TestDataFixture();
protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
    static std::filesystem::path getTestDataDir();
};

TEST_F(TestDataFixture, TestDataDirDirectoryExists)
{
    EXPECT_TRUE(exists(getTestDataDir()));
}
```

### Test Naming

- Pattern: `<Feature>_<Scenario>_<ExpectedResult>`
- Example: `Test_DesignOdb_RigidFlexDesign_CanHasCorrectData`

### Assertions

- Use `ASSERT_*` for fatal assertions (stops test on failure)
- Use `EXPECT_*` for non-fatal assertions (continues test)

```cpp
ASSERT_TRUE(success);           // Fatal
EXPECT_EQ(actual, expected);    // Non-fatal
ASSERT_NE(findIt, map.end());   // Fatal - must find element
```

### Test Data

Test data is located via `ODB_TEST_DATA_DIR` environment variable.
Test designs are in `.tgz` format (ODB++ archives).

---

## External Rules & Instructions

### Cursor Rules

See `.cursor/rules/openmemory.mdc` for memory-based development workflow instructions.

### Copilot Instructions

See `.github/copilot-instructions.md` for:

- Remote instruction modules at `nam20485/agent-instructions`
- Tool and automation protocols
- Dynamic workflow orchestration
- URL translation for raw GitHub content

---

## Learned User Preferences

- Keep OdbDesign local test env vars in `~/.bashrc` (`ODB_TEST_DATA_DIR`, `ODB_TEST_ENVIRONMENT_VARIABLE`) for routine `ctest` runs on this machine.
- The vcpkg upgrade to baseline `4f6d4ae8` (`grpc 1.81.1`, `protobuf 6.33.4#2`) is the accepted target; revert future baseline bumps only if they break the gRPC/protobuf build.

## Learned Workspace Facts

- Release-only SIGABRT on `GET /filemodels/<design>/matrix/matrix` comes from dual static protobuf descriptor pools in `libOdbDesign.so` and `OdbDesignServer`; use `linux-dynamic-debug` / `linux-dynamic-release` presets (`VCPKG_TARGET_TRIPLET=x64-linux-dynamic`) for a single shared protobuf runtime.
- Target `grpc` in `vcpkg.json` must include feature `codegen` so fresh triplet installs export `gRPC::grpc++_reflection` (host-only codegen is insufficient).
- vcpkg baseline is `4f6d4ae8247b2dcae554555a135e52bb449dd524` (supersedes the old `d1ff36c` / `grpc 1.71.0#3` pin); it resolves to `protobuf 6.33.4#2`, `grpc 1.81.1`, `zlib 1.3.2#2`, `libarchive 3.8.7`, `crow 1.3.3` and compiles cleanly. The earlier `059d760` baseline's gRPC break (`glob.cc` `std::any_of`) does not recur here. `vcpkg.json` `overrides` pin these exact `version#port-version` strings.
- The `"version": "X.Y.Z#N"` form (port-version embedded in the version string) is valid in `overrides`; do not split it into separate `version` + `port-version` fields.
- Pin the exact `version#port-version` for every override that has a non-zero port-version (e.g. `zlib 1.3.2#2`, not `1.3.2`); a missing/wrong port-version resolves to a different build and forces vcpkg to rebuild the whole dependency chain from source.
- After a protobuf major bump (e.g. 29→33), stale generated `.pb.h`/`.pb.cc` from the old protoc fail with "Protobuf C++ gencode is built with an incompatible version" / missing `map_field_inl.h`; wipe the build dir (keep `vcpkg_installed/`) and reconfigure so protoc regenerates them — don't try to compile stale gencode.
- Local test fixtures live in sibling repo `OdbDesignTestData`: set `ODB_TEST_DATA_DIR=/home/nam20485/src/github/nam20485/OdbDesignTestData/TEST_DATA`; design `.tgz` archives at `TEST_DATA/` root, small file-reader fixtures under `TEST_DATA/FILES/`.
- Also set `ODB_TEST_ENVIRONMENT_VARIABLE=ODB_TEST_ENVIRONMENT_VARIABLE_EXISTS` for CrossPlatform env tests; these vars are not baked into CMake presets.
- `OdbDesignServer` loads designs via `--designs-dir`, not `ODB_TEST_DATA_DIR`.
- `CommandLineArgs` treats tokens starting with `/` as flags, so absolute paths after `--designs-dir` parse as boolean `true`; use relative paths (e.g. from `/home/nam20485/src/github/nam20485`: `OdbDesignTestData/TEST_DATA`).

## Deployment Tooling (k3s cluster + Argo CD)

- OdbDesign services deploy to the single-node k3s cluster on `debian13vm` (Tailscale `100.118.225.119`, LAN `192.168.122.200`). Until the GitOps migration lands, `scripts/deploy.ps1` remains the manual mechanism; target state is Argo CD GitOps — see `docs/plan/argocd-gitops-handoff.md` (platform handoff) and `docs/plan/argocd-deployment-plan.md` (execution plan).
- `argocd` CLI (v3.5.2, `~/.local/bin/argocd`) is logged in on this machine (context `debian13vm.tail11ba79.ts.net/argocd`); it works only from tailnet devices. Re-auth with `argocd relogin`, or `argocd login debian13vm.tail11ba79.ts.net --username admin --grpc-web --grpc-web-root-path /argocd` (both flags required — Traefik rootpath).
- An Argo CD MCP server is configured in `.zcode/config.json` (stdio, `argocd-mcp@0.9.0`, `ARGOCD_BASE_URL` set; identical to the platform repo's config). Authentication is **environment inheritance, not config**: the server process picks up `ARGOCD_API_TOKEN` from the ZCode process environment (`~/.api-keys-export.sh`, platform `mcp` account). ZCode must be launched with the var exported and restarted to load the server.
- The `mcp` account is **read-only** (`role:readonly`): MCP mutations (`sync_application`, `create/update/delete_application`, `run_resource_action`) are denied by design. Token policy: 1-year expiry — rotate with `argocd account generate-token --account mcp --expires-in 8760h`, update `~/.api-keys-export.sh`, restart ZCode. Platform source of truth: `linux-system-agent` `.agents/rules/tools.md`.
- `.zcode/` is gitignored — keep MCP configs credential-free; never commit tokens.
- Agent rule: MCP is a read-only view; early sync-triggering via the CLI; manifest/Application changes go through git on the `nam20485` deploy branch. Never `kubectl apply` / `argocd app create` / `argocd app sync --local` against app-managed resources — Argo CD selfHeal reverts them.
