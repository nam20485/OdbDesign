#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/TestUtils.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
using namespace testing;

namespace Odb::Test::Enhanced
{
    /**
     * @brief Enhanced test cases for error conditions and edge cases
     */
    class ErrorConditionTests : public TestDataFixture
    {
    protected:
        void SetUp() override
        {
            TestDataFixture::SetUp();
        }
    };

    // Test error handling for invalid paths
    TEST_F(ErrorConditionTests, InvalidPathHandling)
    {
        // Test with empty path
        std::filesystem::path emptyPath;
        EXPECT_FALSE(std::filesystem::exists(emptyPath));
        
        // Test with non-existent path
        std::filesystem::path nonExistentPath = "/this/path/should/not/exist/anywhere";
        EXPECT_FALSE(std::filesystem::exists(nonExistentPath));
        
        // Test with invalid characters (platform-specific)
#ifdef _WIN32
        std::filesystem::path invalidPath = "C:\\invalid|path<>:\"*?";
#else
        std::filesystem::path invalidPath = "/invalid\0path";
#endif
        // Just verify we can handle it without crashing
        EXPECT_NO_THROW({
            std::filesystem::exists(invalidPath);
        });
    }

    // Test boundary conditions for file operations
    TEST_F(ErrorConditionTests, FileBoundaryConditions)
    {
        // Test with very small file
        auto smallFile = TestUtils::createManagedTempFile("");
        ASSERT_TRUE(std::filesystem::exists(smallFile->path()));
        EXPECT_EQ(std::filesystem::file_size(smallFile->path()), 0);
        
        // Test with single byte file
        auto singleByteFile = TestUtils::createManagedTempFile("x");
        ASSERT_TRUE(std::filesystem::exists(singleByteFile->path()));
        EXPECT_EQ(std::filesystem::file_size(singleByteFile->path()), 1);
        
        // Test with moderately large file
        std::string largeContent = TestUtils::generateRandomString(1024 * 1024); // 1MB
        auto largeFile = TestUtils::createManagedTempFile(largeContent);
        ASSERT_TRUE(std::filesystem::exists(largeFile->path()));
        EXPECT_EQ(std::filesystem::file_size(largeFile->path()), largeContent.size());
    }

    // Test environment variable edge cases
    TEST_F(ErrorConditionTests, EnvironmentVariableEdgeCases)
    {
        // Test accessing non-existent environment variable
        const char* nonExistentVar = std::getenv(ODB_TEST_NONEXISTENT_ENV_NAME);
        EXPECT_EQ(nonExistentVar, nullptr);
        
        // Test accessing existing environment variable
        const char* existingVar = std::getenv(ODB_TEST_ENV_NAME);
        if (existingVar != nullptr)
        {
            EXPECT_STREQ(existingVar, ODB_TEST_ENV_VALUE);
        }
        
        // Test with empty environment variable (if set)
        // Note: This test depends on environment setup
        auto testDataDir = getTestDataDir();
        if (!testDataDir.empty())
        {
            EXPECT_TRUE(std::filesystem::exists(testDataDir));
        }
    }

    // Test concurrent access patterns
    TEST_F(ErrorConditionTests, ConcurrentAccessTests)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available for concurrent access test";
        }

        const int numThreads = 4;
        const int operationsPerThread = 50;
        
        std::vector<std::thread> threads;
        std::vector<bool> results(numThreads, true);
        
        // Launch threads that concurrently access test data
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([this, &results, i, operationsPerThread]() {
                try
                {
                    for (int j = 0; j < operationsPerThread; ++j)
                    {
                        // Multiple concurrent reads should be safe
                        auto testDir = getTestDataDir();
                        auto filesDir = getTestDataFilesDir();
                        
                        if (testDir.empty() || !std::filesystem::exists(testDir))
                        {
                            results[i] = false;
                            break;
                        }
                        
                        // Small delay to increase chance of race conditions
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }
                catch (...)
                {
                    results[i] = false;
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads)
        {
            thread.join();
        }
        
        // All threads should complete successfully
        for (size_t i = 0; i < results.size(); ++i)
        {
            EXPECT_TRUE(results[i]) << "Thread " << i << " failed during concurrent access";
        }
    }

    // Test resource cleanup and lifecycle
    class ResourceLifecycleTest : public TestDataFixture
    {
    protected:
        void TearDown() override
        {
            // Verify no temp files leaked
            cleanupTempResources();
            TestDataFixture::TearDown();
        }

        void cleanupTempResources()
        {
            // Clean up any test resources that might have been left behind
            auto tempDir = std::filesystem::temp_directory_path();
            
            try
            {
                for (const auto& entry : std::filesystem::directory_iterator(tempDir))
                {
                    if (entry.is_regular_file() || entry.is_directory())
                    {
                        auto filename = entry.path().filename().string();
                        if (filename.find("odbtest_") == 0)
                        {
                            std::filesystem::remove_all(entry.path());
                        }
                    }
                }
            }
            catch (...)
            {
                // Ignore cleanup errors
            }
        }
    };

    TEST_F(ResourceLifecycleTest, TempResourceManagement)
    {
        std::vector<std::filesystem::path> createdPaths;
        
        // Create multiple temporary resources
        {
            auto tempFile1 = TestUtils::createManagedTempFile("content1");
            auto tempFile2 = TestUtils::createManagedTempFile("content2");
            auto tempDir = TestUtils::createManagedTempDirectory();
            
            createdPaths.push_back(tempFile1->path());
            createdPaths.push_back(tempFile2->path());
            createdPaths.push_back(tempDir->path());
            
            // Verify resources exist
            for (const auto& path : createdPaths)
            {
                EXPECT_TRUE(std::filesystem::exists(path)) << "Resource should exist: " << path;
            }
            
            // Resources should be automatically cleaned up when leaving scope
        }
        
        // Verify resources were cleaned up
        for (const auto& path : createdPaths)
        {
            EXPECT_FALSE(std::filesystem::exists(path)) << "Resource should be cleaned up: " << path;
        }
    }

    // Test data validation and integrity
    TEST_F(ErrorConditionTests, DataValidationTests)
    {
        // Test with various data patterns
        std::vector<std::string> testPatterns = {
            "",                                    // Empty
            "a",                                   // Single character
            "\n",                                  // Newline only
            "\0",                                  // Null character
            std::string(1000, 'x'),               // Repeated character
            TestUtils::generateRandomString(100),  // Random data
            "Unicode: αβγδε 🎉🚀💻",              // Unicode characters
            "Special chars: !@#$%^&*()_+-={}[]|\\:;\"'<>?,./" // Special characters
        };
        
        for (size_t i = 0; i < testPatterns.size(); ++i)
        {
            const auto& pattern = testPatterns[i];
            auto tempFile = TestUtils::createManagedTempFile(pattern, ".test");
            
            ASSERT_TRUE(std::filesystem::exists(tempFile->path())) 
                << "Failed to create file for pattern " << i;
            
            // Verify file size matches expected
            auto expectedSize = pattern.size();
            auto actualSize = std::filesystem::file_size(tempFile->path());
            EXPECT_EQ(actualSize, expectedSize) 
                << "Size mismatch for pattern " << i << ": expected " 
                << expectedSize << ", got " << actualSize;
            
            // Verify content can be read back
            TestAssertions::assertFileContent(tempFile->path(), pattern);
        }
    }

    // Test performance under stress conditions
    TEST_F(ErrorConditionTests, StressConditionTests)
    {
        const size_t numFiles = 100;
        const size_t fileSize = 1024; // 1KB each
        
        std::vector<std::unique_ptr<TestUtils::TempResource>> tempFiles;
        tempFiles.reserve(numFiles);
        
        // Measure time to create many small files
        auto [fileCount, creationTime] = TestUtils::measureExecutionTime([&]() {
            for (size_t i = 0; i < numFiles; ++i)
            {
                std::string content = TestUtils::generateRandomString(fileSize);
                auto tempFile = TestUtils::createManagedTempFile(content);
                tempFiles.push_back(std::move(tempFile));
            }
            return tempFiles.size();
        });
        
        EXPECT_EQ(fileCount, numFiles) << "Not all files were created";
        
        // Verify all files exist and have correct size
        for (size_t i = 0; i < tempFiles.size(); ++i)
        {
            const auto& tempFile = tempFiles[i];
            ASSERT_TRUE(std::filesystem::exists(tempFile->path())) 
                << "File " << i << " does not exist";
            EXPECT_EQ(std::filesystem::file_size(tempFile->path()), fileSize) 
                << "File " << i << " has incorrect size";
        }
        
        // Performance should be reasonable even under stress
        auto creationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(creationTime);
        EXPECT_LT(creationTimeMs.count(), 5000) 
            << "Creating " << numFiles << " files took too long: " 
            << creationTimeMs.count() << "ms";
    }

    // Test error recovery and resilience
    TEST_F(ErrorConditionTests, ErrorRecoveryTests)
    {
        // Test recovery from file system errors
        auto tempDir = TestUtils::createManagedTempDirectory();
        
        // Create a file in the directory
        auto testFile = tempDir->path() / "test.txt";
        std::ofstream(testFile) << "test content";
        
        ASSERT_TRUE(std::filesystem::exists(testFile));
        
        // Test retry logic with a flaky operation
        bool success = TestUtils::retryWithBackoff([&]() {
            // Simulate a flaky operation that succeeds on retry
            static int attemptCount = 0;
            attemptCount++;
            return attemptCount >= 2; // Succeed on second attempt
        }, 3, std::chrono::milliseconds(10));
        
        EXPECT_TRUE(success) << "Retry logic should eventually succeed";
        
        // Test with an operation that always fails
        bool alwaysFails = TestUtils::retryWithBackoff([&]() {
            return false; // Always fail
        }, 3, std::chrono::milliseconds(1));
        
        EXPECT_FALSE(alwaysFails) << "Operation that always fails should not succeed";
    }
}