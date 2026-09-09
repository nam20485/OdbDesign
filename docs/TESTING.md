# OdbDesign Test Suite Documentation

## Overview

This document describes the automated test suite for the OdbDesign project. The test suite provides comprehensive coverage including unit tests, integration tests, performance tests, and error condition testing.

## Test Framework

The project uses **GoogleTest** (gtest) and **GoogleMock** (gmock) frameworks for automated testing. Tests are automatically discovered and executed by CMake's CTest runner.

## Test Structure

### Test Categories

1. **Unit Tests** - Test individual components in isolation
2. **Integration Tests** - Test component interactions and workflows
3. **Performance Tests** - Measure performance and detect regressions
4. **Error Condition Tests** - Test error handling and edge cases

### Test Files

| File | Purpose | Test Types |
|------|---------|------------|
| `TestTests.cpp` | Basic test infrastructure verification | Unit |
| `ArchiveTests.cpp` | Archive functionality | Unit |
| `ArchiveExtractorTests.cpp` | Archive extraction | Unit |
| `FileArchiveTests.cpp` | File archive operations | Unit |
| `FileArchiveLoadTests.cpp` | File archive loading | Unit |
| `FileReaderTests.cpp` | File reading functionality | Unit |
| `CrossPlatformTests.cpp` | Cross-platform compatibility | Unit |
| `DesignCacheLoadTests.cpp` | Design cache operations | Unit |
| `ProtobufSerializationTests.cpp` | Protobuf serialization | Unit |
| `PerformanceTests.cpp` | Performance benchmarks and stress tests | Performance |
| `EnhancedTests.cpp` | Error conditions and edge cases | Unit/Error |
| `IntegrationTests.cpp` | End-to-end workflows and component integration | Integration |

### Test Fixtures

| Fixture | Purpose |
|---------|---------|
| `TestDataFixture` | Base fixture providing test data access |
| `FileArchiveLoadFixture` | Fixture for file archive testing |
| `TestUtils` | Utility functions for test automation |

## Running Tests

### Local Development

#### Using the Test Runner Script

The easiest way to run tests locally is using the provided test runner script:

```bash
# Basic test run
./scripts/run-tests.sh

# Clean build with verbose output
./scripts/run-tests.sh --clean --verbose

# Release build with coverage
./scripts/run-tests.sh --release --coverage

# Performance tests only
./scripts/run-tests.sh --performance

# Show all options
./scripts/run-tests.sh --help
```

#### Manual CMake/CTest

```bash
# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Run all tests
ctest --output-on-failure

# Run tests with verbose output
ctest --verbose

# Run specific test pattern
ctest -R "Performance"

# Run tests in parallel
ctest -j 4
```

### Prerequisites

1. **Required Tools:**
   - CMake 3.21+
   - Ninja build system (recommended)
   - C++17 compatible compiler

2. **Dependencies:**
   - vcpkg package manager (for dependencies)
   - GoogleTest (automatically fetched by CMake)

3. **Environment Variables:**
   - `ODB_TEST_DATA_DIR` - Path to test data directory (optional)
   - `VCPKG_ROOT` - Path to vcpkg installation

### Test Data

Some tests require test data files. The test suite can work in two modes:

1. **With Test Data:** Set `ODB_TEST_DATA_DIR` environment variable pointing to test data
2. **Without Test Data:** Tests requiring test data will be skipped gracefully

#### Setting up Test Data

```bash
# Option 1: Use environment variable
export ODB_TEST_DATA_DIR=/path/to/test/data

# Option 2: Let the test runner create basic test data
./scripts/run-tests.sh --test-data-dir ./test_data
```

## Continuous Integration

Tests run automatically on every push and pull request via GitHub Actions. The CI pipeline:

1. Builds on Windows, Linux, and macOS
2. Runs the full test suite
3. Generates test reports
4. Publishes test results

### CI Test Execution

```yaml
- name: CMake Test
  run: ctest --test-dir ./out/build/${{matrix.preset}}/OdbDesignTests 
       --output-log testlog.txt 
       --output-junit testlog.xml 
       --output-on-failure

- name: Report Test Results
  uses: dorny/test-reporter@v1.9.1
  with:
    name: test-results
    path: testlog.xml
    reporter: java-junit
```

## Test Development

### Adding New Tests

1. **Create Test File:**
   ```cpp
   #include <gtest/gtest.h>
   #include "Fixtures/TestDataFixture.h"
   
   namespace Odb::Test
   {
       TEST(MyComponentTest, BasicFunctionality)
       {
           EXPECT_TRUE(true);
       }
   }
   ```

2. **Add to CMakeLists.txt:**
   ```cmake
   add_executable(OdbDesignTests
       # ... existing files ...
       "MyNewTests.cpp"
   )
   ```

3. **Use Test Utilities:**
   ```cpp
   #include "Fixtures/TestUtils.h"
   
   auto tempFile = TestUtils::createManagedTempFile("content");
   auto [result, time] = TestUtils::measureExecutionTime([]() {
       // Your test code here
   });
   ```

### Test Naming Conventions

- **Test Suites:** Use descriptive names ending with "Test" or "Tests"
- **Test Cases:** Use descriptive names in PascalCase
- **Fixtures:** Use descriptive names ending with "Fixture"

Examples:
```cpp
TEST(ComponentNameTest, SpecificFunctionality)
TEST_F(MyFixture, SpecificScenario)
class PerformanceTests : public testing::Test
```

### Test Utilities

The `TestUtils` class provides utilities for:

- **Temporary Resources:** Automatic cleanup of temp files/directories
- **Random Data Generation:** Generate test data with various patterns
- **Performance Measurement:** Time execution and benchmark functions
- **File Operations:** Create, compare, and manage test files
- **Retry Logic:** Implement retry patterns for flaky operations

Example usage:
```cpp
// Create temporary file with automatic cleanup
auto tempFile = TestUtils::createManagedTempFile("test content");

// Measure execution time
auto [result, duration] = TestUtils::measureExecutionTime([&]() {
    return myFunction();
});

// Generate random test data
auto randomData = TestUtils::generateRandomString(1024);

// Retry flaky operations
bool success = TestUtils::retryWithBackoff([&]() {
    return attemptOperation();
});
```

## Performance Testing

### Benchmark Tests

Performance tests measure execution time and detect regressions:

```cpp
TEST_F(PerformanceTestBase, MyBenchmark)
{
    auto avgTime = benchmarkFunction([&]() {
        return myFunction();
    }, 100); // 100 iterations

    EXPECT_LT(avgTime.count(), 1000) << "Function should complete in <1ms";
}
```

### Stress Tests

Stress tests verify behavior under high load:

```cpp
TEST_F(PerformanceTestBase, ConcurrentStressTest)
{
    const int numThreads = std::thread::hardware_concurrency();
    // Launch concurrent operations and verify results
}
```

## Error Condition Testing

### Edge Cases

Test boundary conditions and edge cases:

```cpp
TEST_F(ErrorConditionTests, BoundaryConditions)
{
    // Test with empty input
    // Test with maximum size input
    // Test with invalid input
}
```

### Exception Testing

Use test utilities for exception testing:

```cpp
TestAssertions::assertThrows<std::runtime_error>([&]() {
    functionThatShouldThrow();
}, "expected error message");
```

## Integration Testing

### Workflow Testing

Test complete end-to-end workflows:

```cpp
TEST_F(IntegrationTestSuite, CompleteWorkflow)
{
    // Step 1: Setup
    // Step 2: Execute workflow
    // Step 3: Verify results
    // Step 4: Cleanup
}
```

### Component Interaction

Test how components work together:

```cpp
TEST_F(IntegrationTestSuite, ComponentInteraction)
{
    // Test data flow between components
    // Test error propagation
    // Test resource sharing
}
```

## Troubleshooting

### Common Issues

1. **Missing Dependencies:**
   ```bash
   # Install vcpkg dependencies
   vcpkg install
   ```

2. **Test Data Not Found:**
   ```bash
   # Set test data directory
   export ODB_TEST_DATA_DIR=/path/to/test/data
   ```

3. **Build Failures:**
   ```bash
   # Clean and rebuild
   ./scripts/run-tests.sh --clean
   ```

### Debug Mode

Enable verbose logging in tests:

```cpp
// In test fixtures, set ENABLE_TEST_LOGGING = true
static inline constexpr bool ENABLE_TEST_LOGGING = true;
```

### Performance Issues

If tests are running slowly:

1. Check system resources
2. Reduce test iterations for development
3. Use `--performance` flag to run only performance tests
4. Run tests in parallel: `ctest -j N`

## Best Practices

### Test Writing

1. **Keep tests focused** - One concept per test
2. **Use descriptive names** - Test names should explain what is being tested
3. **Make tests independent** - Tests should not depend on each other
4. **Use fixtures for common setup** - Avoid code duplication
5. **Test both success and failure paths** - Include error condition testing

### Performance

1. **Use benchmark utilities** - Consistent performance measurement
2. **Set reasonable expectations** - Performance limits should be achievable
3. **Test on different hardware** - Consider CI environment limitations
4. **Monitor for regressions** - Track performance over time

### Maintenance

1. **Update tests with code changes** - Keep tests in sync with implementation
2. **Remove obsolete tests** - Clean up tests for removed functionality
3. **Refactor common patterns** - Extract reusable test utilities
4. **Document complex test scenarios** - Help future developers understand tests

## Contributing

When contributing new tests:

1. Follow existing patterns and conventions
2. Add appropriate documentation
3. Ensure tests pass on all platforms
4. Update this documentation if needed
5. Consider both positive and negative test cases

For questions or issues with the test suite, please refer to the project's issue tracker or documentation.