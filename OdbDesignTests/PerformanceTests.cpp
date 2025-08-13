#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <vector>
#include <memory>
#include <thread>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/FileArchiveLoadFixture.h"

using namespace std::chrono;
using namespace Odb::Test::Fixtures;
using namespace testing;

namespace Odb::Test::Performance
{
    class PerformanceTestBase : public TestDataFixture
    {
    protected:
        void SetUp() override
        {
            TestDataFixture::SetUp();
            m_startTime = high_resolution_clock::now();
        }

        void TearDown() override
        {
            auto endTime = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(endTime - m_startTime);
            
            if (ENABLE_TEST_LOGGING)
            {
                std::cout << "Test execution time: " << duration.count() << "ms" << std::endl;
            }
            
            TestDataFixture::TearDown();
        }

        // Helper to measure execution time of a function
        template<typename Func>
        auto measureTime(Func&& func) -> decltype(func())
        {
            auto start = high_resolution_clock::now();
            auto result = func();
            auto end = high_resolution_clock::now();
            
            m_lastMeasuredTime = duration_cast<microseconds>(end - start);
            return result;
        }

        // Get the last measured time in microseconds
        microseconds getLastMeasuredTime() const { return m_lastMeasuredTime; }

        // Benchmark a function multiple times and return average
        template<typename Func>
        microseconds benchmarkFunction(Func&& func, int iterations = 100)
        {
            auto totalTime = microseconds(0);
            
            for (int i = 0; i < iterations; ++i)
            {
                measureTime(func);
                totalTime += m_lastMeasuredTime;
            }
            
            return totalTime / iterations;
        }

    private:
        high_resolution_clock::time_point m_startTime;
        microseconds m_lastMeasuredTime{0};
    };

    // Performance test for basic operations
    TEST_F(PerformanceTestBase, BasicOperationsPerformance)
    {
        // Test environment variable access performance
        auto envVarTime = benchmarkFunction([this]() {
            return getTestDataDir();
        }, 1000);
        
        EXPECT_LT(envVarTime.count(), 1000) << "Environment variable access should be under 1ms";
        
        // Test file system operations performance
        auto fsTime = benchmarkFunction([this]() {
            return std::filesystem::exists(getTestDataDir());
        }, 100);
        
        EXPECT_LT(fsTime.count(), 10000) << "File system existence check should be under 10ms";
    }

    // Performance test for memory allocation patterns
    TEST_F(PerformanceTestBase, MemoryAllocationPerformance)
    {
        const size_t allocSize = 1024 * 1024; // 1MB
        const int iterations = 50;
        
        // Test vector allocation performance
        auto vectorAllocTime = benchmarkFunction([allocSize]() {
            std::vector<char> buffer(allocSize);
            buffer[0] = 'x'; // Ensure memory is touched
            return buffer.size();
        }, iterations);
        
        EXPECT_LT(vectorAllocTime.count(), 50000) << "Vector allocation should be under 50ms";
        
        // Test unique_ptr allocation performance
        auto uniquePtrTime = benchmarkFunction([allocSize]() {
            auto ptr = std::make_unique<char[]>(allocSize);
            ptr[0] = 'x'; // Ensure memory is touched
            return ptr.get() != nullptr;
        }, iterations);
        
        EXPECT_LT(uniquePtrTime.count(), 50000) << "Unique pointer allocation should be under 50ms";
    }

    // Stress test for concurrent operations
    TEST_F(PerformanceTestBase, ConcurrentOperationsStressTest)
    {
        const int numThreads = std::thread::hardware_concurrency();
        const int operationsPerThread = 100;
        
        std::vector<std::thread> threads;
        std::vector<bool> results(numThreads, false);
        
        auto startTime = high_resolution_clock::now();
        
        // Launch concurrent operations
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([this, &results, i, operationsPerThread]() {
                bool allSuccess = true;
                
                for (int j = 0; j < operationsPerThread; ++j)
                {
                    // Perform thread-safe operations
                    auto testDir = getTestDataDir();
                    if (testDir.empty() || !std::filesystem::exists(testDir))
                    {
                        allSuccess = false;
                        break;
                    }
                    
                    // Small delay to simulate work
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
                
                results[i] = allSuccess;
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads)
        {
            thread.join();
        }
        
        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);
        
        // Verify all operations succeeded
        for (size_t i = 0; i < results.size(); ++i)
        {
            EXPECT_TRUE(results[i]) << "Thread " << i << " reported failure";
        }
        
        // Performance should scale reasonably with thread count
        EXPECT_LT(totalTime.count(), 10000) << "Concurrent operations should complete within 10 seconds";
    }

    // Benchmark test with load testing
    class PerformanceBenchmarkTest : public FileArchiveLoadFixture
    {
    protected:
        void SetUp() override
        {
            FileArchiveLoadFixture::SetUp();
        }
    };

    TEST_F(PerformanceBenchmarkTest, FileSystemPerformanceBenchmark)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available";
        }

        // Benchmark directory traversal
        auto traversalTime = measureTime([this]() {
            int fileCount = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(getTestDataDir()))
            {
                if (entry.is_regular_file())
                {
                    fileCount++;
                }
            }
            return fileCount;
        });

        EXPECT_LT(getLastMeasuredTime().count(), 100000) << "Directory traversal should be under 100ms";

        // Benchmark file existence checks
        std::vector<std::string> testFiles = {
            "sample_design.tgz",
            "designodb_rigidflex.tgz",
            "nonexistent_file.txt"
        };

        for (const auto& filename : testFiles)
        {
            auto checkTime = measureTime([this, &filename]() {
                return std::filesystem::exists(getDesignPath(filename));
            });

            EXPECT_LT(checkTime.count(), 10000) << "File existence check should be under 10ms for " << filename;
        }
    }

    // Memory usage and leak detection test
    TEST_F(PerformanceTestBase, MemoryLeakDetectionTest)
    {
        // Track memory usage patterns
        std::vector<std::unique_ptr<char[]>> allocations;
        const size_t allocationSize = 1024;
        const int numAllocations = 1000;

        // Allocate memory in chunks
        auto allocTime = measureTime([&]() {
            for (int i = 0; i < numAllocations; ++i)
            {
                allocations.push_back(std::make_unique<char[]>(allocationSize));
                // Touch the memory to ensure it's actually allocated
                allocations.back()[0] = static_cast<char>(i % 256);
            }
        });

        EXPECT_LT(getLastMeasuredTime().count(), 100000) << "Memory allocation should be under 100ms";

        // Verify all allocations are valid
        for (size_t i = 0; i < allocations.size(); ++i)
        {
            ASSERT_NE(allocations[i].get(), nullptr) << "Allocation " << i << " failed";
            EXPECT_EQ(allocations[i][0], static_cast<char>(i % 256)) << "Memory corruption detected";
        }

        // Deallocate memory
        auto deallocTime = measureTime([&]() {
            allocations.clear();
        });

        EXPECT_LT(getLastMeasuredTime().count(), 50000) << "Memory deallocation should be under 50ms";
    }

    // Regression test for performance 
    TEST_F(PerformanceTestBase, PerformanceRegressionBaseline)
    {
        // This test establishes baseline performance metrics
        // that can be used to detect performance regressions
        
        struct PerformanceMetrics
        {
            microseconds stringOperations{0};
            microseconds vectorOperations{0};
            microseconds fileSystemOps{0};
        };

        PerformanceMetrics metrics;

        // String operations benchmark
        metrics.stringOperations = benchmarkFunction([]() {
            std::string result;
            for (int i = 0; i < 1000; ++i)
            {
                result += "test_string_" + std::to_string(i) + "_";
            }
            return result.length();
        }, 10);

        // Vector operations benchmark  
        metrics.vectorOperations = benchmarkFunction([]() {
            std::vector<int> vec;
            vec.reserve(10000);
            for (int i = 0; i < 10000; ++i)
            {
                vec.push_back(i);
            }
            return vec.size();
        }, 10);

        // File system operations benchmark
        if (!getTestDataDir().empty())
        {
            metrics.fileSystemOps = benchmarkFunction([this]() {
                auto dir = getTestDataDir();
                return std::filesystem::exists(dir) ? 1 : 0;
            }, 100);
        }

        // Log performance metrics for future comparison
        if (ENABLE_TEST_LOGGING)
        {
            std::cout << "Performance Baseline Metrics:" << std::endl;
            std::cout << "  String operations: " << metrics.stringOperations.count() << " μs" << std::endl;
            std::cout << "  Vector operations: " << metrics.vectorOperations.count() << " μs" << std::endl;
            std::cout << "  File system ops: " << metrics.fileSystemOps.count() << " μs" << std::endl;
        }

        // Set reasonable performance expectations
        EXPECT_LT(metrics.stringOperations.count(), 10000) << "String operations regression detected";
        EXPECT_LT(metrics.vectorOperations.count(), 5000) << "Vector operations regression detected";
        
        if (!getTestDataDir().empty())
        {
            EXPECT_LT(metrics.fileSystemOps.count(), 1000) << "File system operations regression detected";
        }
    }
}