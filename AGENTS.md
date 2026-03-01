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
cmake --preset linux-debug      # Debug build
cmake --preset linux-release    # Release build
```

### Configure (Windows)
```bash
cmake --preset x64-debug        # Debug build
cmake --preset x64-release      # Release build
```

### Build
```bash
cmake --build --preset linux-debug
cmake --build --preset linux-release
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
ctest --preset linux-release
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
