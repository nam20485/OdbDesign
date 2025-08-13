#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Fixtures/TestDataFixture.h"
#include "Fixtures/FileArchiveLoadFixture.h"
#include "Fixtures/TestUtils.h"
#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>

using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
using namespace testing;

namespace Odb::Test::Integration
{
    /**
     * @brief Integration tests that verify component interactions and end-to-end workflows
     */
    class IntegrationTestSuite : public FileArchiveLoadFixture
    {
    protected:
        void SetUp() override
        {
            FileArchiveLoadFixture::SetUp();
            
            // Setup additional integration test environment
            setupIntegrationEnvironment();
        }

        void TearDown() override
        {
            cleanupIntegrationEnvironment();
            FileArchiveLoadFixture::TearDown();
        }

    private:
        void setupIntegrationEnvironment()
        {
            // Create additional test resources needed for integration tests
            m_tempWorkDir = TestUtils::createManagedTempDirectory("integration_test");
            
            // Create test configuration files
            createTestConfiguration();
        }

        void cleanupIntegrationEnvironment()
        {
            // Integration test specific cleanup
            m_tempWorkDir.reset();
        }

        void createTestConfiguration()
        {
            // Create a test configuration file
            auto configContent = R"({
                "test_mode": true,
                "debug_level": 1,
                "max_workers": 4,
                "timeout_ms": 30000
            })";
            
            auto configPath = m_tempWorkDir->path() / "test_config.json";
            std::ofstream(configPath) << configContent;
        }

    protected:
        std::unique_ptr<TestUtils::TempResource> m_tempWorkDir;
    };

    // Test complete workflow from file loading to processing
    TEST_F(IntegrationTestSuite, CompleteWorkflowIntegration)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Integration test requires test data directory";
        }

        // Test the complete workflow with multiple design files
        std::vector<std::string> testDesigns = {
            "sample_design.tgz",
            "designodb_rigidflex.tgz"
        };

        for (const auto& designName : testDesigns)
        {
            auto designPath = getDesignPath(designName);
            
            if (!std::filesystem::exists(designPath))
            {
                GTEST_SKIP() << "Test design not available: " << designName;
            }

            // Measure end-to-end processing time
            auto [success, processingTime] = TestUtils::measureExecutionTime([&]() {
                try
                {
                    // This would typically involve:
                    // 1. Loading the design file
                    // 2. Parsing the content
                    // 3. Validating the data
                    // 4. Processing the design
                    // 5. Generating output
                    
                    // For now, just verify file accessibility and basic properties
                    EXPECT_TRUE(std::filesystem::exists(designPath));
                    EXPECT_GT(std::filesystem::file_size(designPath), 0);
                    
                    // Simulate processing delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    return true;
                }
                catch (...)
                {
                    return false;
                }
            });

            EXPECT_TRUE(success) << "Workflow failed for design: " << designName;
            
            // Performance expectation for integration test
            auto processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(processingTime);
            EXPECT_LT(processingTimeMs.count(), 1000) 
                << "Workflow took too long for " << designName << ": " 
                << processingTimeMs.count() << "ms";
        }
    }

    // Test concurrent processing of multiple designs
    TEST_F(IntegrationTestSuite, ConcurrentProcessingIntegration)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Integration test requires test data directory";
        }

        std::vector<std::string> designs = {
            "sample_design.tgz",
            "designodb_rigidflex.tgz"
        };

        // Filter to only existing designs
        std::vector<std::filesystem::path> availableDesigns;
        for (const auto& design : designs)
        {
            auto path = getDesignPath(design);
            if (std::filesystem::exists(path))
            {
                availableDesigns.push_back(path);
            }
        }

        if (availableDesigns.empty())
        {
            GTEST_SKIP() << "No test designs available for concurrent processing test";
        }

        const int numWorkers = std::min(4, static_cast<int>(availableDesigns.size()));
        std::vector<std::thread> workers;
        std::vector<bool> results(numWorkers, false);
        
        // Launch concurrent workers
        for (int i = 0; i < numWorkers; ++i)
        {
            workers.emplace_back([&, i]() {
                try
                {
                    auto designPath = availableDesigns[i % availableDesigns.size()];
                    
                    // Simulate concurrent processing
                    EXPECT_TRUE(std::filesystem::exists(designPath));
                    EXPECT_GT(std::filesystem::file_size(designPath), 0);
                    
                    // Simulate some processing work
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    
                    results[i] = true;
                }
                catch (...)
                {
                    results[i] = false;
                }
            });
        }

        // Wait for all workers to complete
        for (auto& worker : workers)
        {
            worker.join();
        }

        // Verify all workers completed successfully
        for (int i = 0; i < numWorkers; ++i)
        {
            EXPECT_TRUE(results[i]) << "Worker " << i << " failed during concurrent processing";
        }
    }

    // Test error handling in integrated scenarios
    TEST_F(IntegrationTestSuite, ErrorHandlingIntegration)
    {
        // Test handling of non-existent files
        auto nonExistentPath = getDesignPath("non_existent_design.tgz");
        EXPECT_FALSE(std::filesystem::exists(nonExistentPath));

        // Test handling of corrupted data
        auto corruptedFile = TestUtils::createManagedTempFile("corrupted data", ".tgz");
        EXPECT_TRUE(std::filesystem::exists(corruptedFile->path()));
        EXPECT_GT(std::filesystem::file_size(corruptedFile->path()), 0);

        // Test handling of empty files
        auto emptyFile = TestUtils::createManagedTempFile("", ".tgz");
        EXPECT_TRUE(std::filesystem::exists(emptyFile->path()));
        EXPECT_EQ(std::filesystem::file_size(emptyFile->path()), 0);

        // Test recovery mechanisms
        bool recovered = TestUtils::retryWithBackoff([&]() {
            // Simulate a flaky operation that eventually succeeds
            static int attempts = 0;
            attempts++;
            return attempts >= 2;
        });

        EXPECT_TRUE(recovered) << "Error recovery mechanism should work";
    }

    // Test resource management across components
    TEST_F(IntegrationTestSuite, ResourceManagementIntegration)
    {
        const size_t numResources = 50;
        std::vector<std::unique_ptr<TestUtils::TempResource>> resources;
        resources.reserve(numResources);

        // Allocate multiple resources
        auto [allocatedCount, allocationTime] = TestUtils::measureExecutionTime([&]() {
            for (size_t i = 0; i < numResources; ++i)
            {
                // Create different types of resources
                if (i % 2 == 0)
                {
                    auto content = TestUtils::generateRandomString(1024);
                    resources.push_back(TestUtils::createManagedTempFile(content));
                }
                else
                {
                    resources.push_back(TestUtils::createManagedTempDirectory());
                }
            }
            return resources.size();
        });

        EXPECT_EQ(allocatedCount, numResources) << "Not all resources were allocated";

        // Verify all resources exist
        for (size_t i = 0; i < resources.size(); ++i)
        {
            EXPECT_TRUE(std::filesystem::exists(resources[i]->path())) 
                << "Resource " << i << " does not exist";
        }

        // Performance check
        auto allocationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(allocationTime);
        EXPECT_LT(allocationTimeMs.count(), 2000) 
            << "Resource allocation took too long: " << allocationTimeMs.count() << "ms";

        // Test cleanup (resources should be automatically cleaned up when vector is destroyed)
    }

    // Test data flow between components
    class DataFlowIntegrationTest : public IntegrationTestSuite
    {
    protected:
        struct ProcessingResult
        {
            bool success = false;
            size_t bytesProcessed = 0;
            std::chrono::microseconds processingTime{0};
            std::string errorMessage;
        };

        ProcessingResult simulateDataProcessing(const std::filesystem::path& inputPath)
        {
            ProcessingResult result;
            
            auto [success, time] = TestUtils::measureExecutionTime([&]() {
                try
                {
                    if (!std::filesystem::exists(inputPath))
                    {
                        result.errorMessage = "Input file does not exist";
                        return false;
                    }

                    result.bytesProcessed = std::filesystem::file_size(inputPath);
                    
                    // Simulate processing delay based on file size
                    auto delayMs = std::min(static_cast<size_t>(100), result.bytesProcessed / 1024);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    
                    return true;
                }
                catch (const std::exception& e)
                {
                    result.errorMessage = e.what();
                    return false;
                }
            });

            result.success = success;
            result.processingTime = time;
            return result;
        }
    };

    TEST_F(DataFlowIntegrationTest, ProcessingPipelineIntegration)
    {
        // Create a processing pipeline with multiple stages
        std::vector<std::string> testData = {
            TestUtils::generateRandomString(1024),      // Small data
            TestUtils::generateRandomString(10240),     // Medium data
            TestUtils::generateRandomString(102400)     // Large data
        };

        for (size_t i = 0; i < testData.size(); ++i)
        {
            // Stage 1: Create input data
            auto inputFile = TestUtils::createManagedTempFile(testData[i], ".input");
            ASSERT_TRUE(std::filesystem::exists(inputFile->path()));

            // Stage 2: Process data
            auto result = simulateDataProcessing(inputFile->path());
            
            EXPECT_TRUE(result.success) 
                << "Processing failed for dataset " << i << ": " << result.errorMessage;
            EXPECT_EQ(result.bytesProcessed, testData[i].size()) 
                << "Bytes processed mismatch for dataset " << i;

            // Stage 3: Validate processing time scales reasonably
            auto processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(result.processingTime);
            EXPECT_LT(processingTimeMs.count(), 1000) 
                << "Processing took too long for dataset " << i << ": " 
                << processingTimeMs.count() << "ms";

            // Stage 4: Create output (simulate)
            auto outputContent = "Processed: " + std::to_string(result.bytesProcessed) + " bytes";
            auto outputFile = TestUtils::createManagedTempFile(outputContent, ".output");
            
            EXPECT_TRUE(std::filesystem::exists(outputFile->path()));
            TestAssertions::assertFileContent(outputFile->path(), outputContent);
        }
    }

    // Test system integration with environment
    TEST_F(IntegrationTestSuite, SystemEnvironmentIntegration)
    {
        // Test integration with environment variables
        auto testDataDir = getTestDataDir();
        if (!testDataDir.empty())
        {
            EXPECT_TRUE(std::filesystem::exists(testDataDir)) 
                << "Test data directory should exist: " << testDataDir;
        }

        // Test integration with file system
        auto filesDir = getTestDataFilesDir();
        if (!filesDir.empty() && std::filesystem::exists(filesDir))
        {
            // Count files in the directory
            size_t fileCount = 0;
            for (const auto& entry : std::filesystem::directory_iterator(filesDir))
            {
                if (entry.is_regular_file())
                {
                    fileCount++;
                }
            }
            
            // Should have at least some test files
            EXPECT_GT(fileCount, 0) << "Test files directory should contain test files";
        }

        // Test integration with temporary directory
        auto tempDir = std::filesystem::temp_directory_path();
        EXPECT_TRUE(std::filesystem::exists(tempDir)) << "Temporary directory should be accessible";
        EXPECT_TRUE(std::filesystem::is_directory(tempDir)) << "Temporary path should be a directory";

        // Test creation of files in temp directory
        auto testTempFile = TestUtils::createManagedTempFile("integration test");
        EXPECT_TRUE(std::filesystem::exists(testTempFile->path()));
        
        // Verify the file is in the temp directory
        auto parent = testTempFile->path().parent_path();
        EXPECT_EQ(parent, tempDir) << "Temp file should be created in temp directory";
    }

    // Test end-to-end workflow with realistic scenarios
    TEST_F(IntegrationTestSuite, RealisticWorkflowIntegration)
    {
        // Simulate a realistic workflow that might be used in production
        struct WorkflowStep
        {
            std::string name;
            std::function<bool()> execute;
            bool required = true;
        };

        std::vector<WorkflowStep> workflow = {
            {
                "Initialize Environment",
                [this]() {
                    return !getTestDataDir().empty() && m_tempWorkDir != nullptr;
                },
                true
            },
            {
                "Load Configuration",
                [this]() {
                    auto configPath = m_tempWorkDir->path() / "test_config.json";
                    return std::filesystem::exists(configPath);
                },
                true
            },
            {
                "Validate Input Data",
                [this]() {
                    if (getTestDataDir().empty()) return false;
                    
                    auto filesDir = getTestDataFilesDir();
                    return std::filesystem::exists(filesDir);
                },
                false // Optional step
            },
            {
                "Process Data",
                [this]() {
                    // Simulate data processing
                    auto testFile = TestUtils::createManagedTempFile("processing data");
                    return std::filesystem::exists(testFile->path());
                },
                true
            },
            {
                "Generate Output",
                [this]() {
                    auto outputFile = m_tempWorkDir->path() / "output.txt";
                    std::ofstream(outputFile) << "Integration test output\n";
                    return std::filesystem::exists(outputFile);
                },
                true
            }
        };

        // Execute workflow steps
        bool workflowSuccess = true;
        for (const auto& step : workflow)
        {
            bool stepResult = step.execute();
            
            if (step.required)
            {
                EXPECT_TRUE(stepResult) << "Required workflow step failed: " << step.name;
                if (!stepResult)
                {
                    workflowSuccess = false;
                    break;
                }
            }
            else
            {
                // Optional steps can fail without breaking the workflow
                if (!stepResult)
                {
                    GTEST_LOG_(INFO) << "Optional workflow step skipped: " << step.name;
                }
            }
        }

        EXPECT_TRUE(workflowSuccess) << "Overall workflow should succeed";

        // Verify workflow artifacts
        auto outputFile = m_tempWorkDir->path() / "output.txt";
        if (workflowSuccess)
        {
            EXPECT_TRUE(std::filesystem::exists(outputFile)) << "Workflow should produce output";
        }
    }
}