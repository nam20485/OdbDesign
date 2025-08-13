#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <memory>
#include <functional>

namespace Odb::Test::Utils
{
    /**
     * @brief Test utilities for automated test cases
     */
    class TestUtils
    {
    public:
        /**
         * @brief Generate random test data
         */
        static std::string generateRandomString(size_t length)
        {
            static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

            std::string result;
            result.reserve(length);
            
            for (size_t i = 0; i < length; ++i)
            {
                result += charset[dis(gen)];
            }
            
            return result;
        }

        /**
         * @brief Generate random binary data
         */
        static std::vector<uint8_t> generateRandomBinaryData(size_t size)
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<uint8_t> dis(0, 255);

            std::vector<uint8_t> data;
            data.reserve(size);
            
            for (size_t i = 0; i < size; ++i)
            {
                data.push_back(dis(gen));
            }
            
            return data;
        }

        /**
         * @brief Create a temporary test file with specified content
         */
        static std::filesystem::path createTempFile(const std::string& content, const std::string& extension = ".tmp")
        {
            auto tempDir = std::filesystem::temp_directory_path();
            auto filename = "odbtest_" + generateRandomString(8) + extension;
            auto filepath = tempDir / filename;
            
            std::ofstream file(filepath, std::ios::binary);
            if (file.is_open())
            {
                file.write(content.c_str(), content.size());
                file.close();
            }
            
            return filepath;
        }

        /**
         * @brief Create a temporary test file with binary data
         */
        static std::filesystem::path createTempBinaryFile(const std::vector<uint8_t>& data, const std::string& extension = ".bin")
        {
            auto tempDir = std::filesystem::temp_directory_path();
            auto filename = "odbtest_" + generateRandomString(8) + extension;
            auto filepath = tempDir / filename;
            
            std::ofstream file(filepath, std::ios::binary);
            if (file.is_open())
            {
                file.write(reinterpret_cast<const char*>(data.data()), data.size());
                file.close();
            }
            
            return filepath;
        }

        /**
         * @brief Create a temporary directory structure
         */
        static std::filesystem::path createTempDirectory(const std::string& prefix = "odbtest")
        {
            auto tempDir = std::filesystem::temp_directory_path();
            auto dirname = prefix + "_" + generateRandomString(8);
            auto dirpath = tempDir / dirname;
            
            std::filesystem::create_directories(dirpath);
            return dirpath;
        }

        /**
         * @brief RAII helper for temporary files and directories
         */
        class TempResource
        {
        public:
            explicit TempResource(const std::filesystem::path& path) : m_path(path) {}
            
            ~TempResource()
            {
                try
                {
                    if (std::filesystem::exists(m_path))
                    {
                        if (std::filesystem::is_directory(m_path))
                        {
                            std::filesystem::remove_all(m_path);
                        }
                        else
                        {
                            std::filesystem::remove(m_path);
                        }
                    }
                }
                catch (...)
                {
                    // Ignore cleanup errors in destructor
                }
            }

            // Non-copyable, movable
            TempResource(const TempResource&) = delete;
            TempResource& operator=(const TempResource&) = delete;
            
            TempResource(TempResource&& other) noexcept : m_path(std::move(other.m_path))
            {
                other.m_path.clear();
            }
            
            TempResource& operator=(TempResource&& other) noexcept
            {
                if (this != &other)
                {
                    m_path = std::move(other.m_path);
                    other.m_path.clear();
                }
                return *this;
            }

            const std::filesystem::path& path() const { return m_path; }

        private:
            std::filesystem::path m_path;
        };

        /**
         * @brief Create a managed temporary file
         */
        static std::unique_ptr<TempResource> createManagedTempFile(const std::string& content, const std::string& extension = ".tmp")
        {
            auto path = createTempFile(content, extension);
            return std::make_unique<TempResource>(path);
        }

        /**
         * @brief Create a managed temporary directory
         */
        static std::unique_ptr<TempResource> createManagedTempDirectory(const std::string& prefix = "odbtest")
        {
            auto path = createTempDirectory(prefix);
            return std::make_unique<TempResource>(path);
        }

        /**
         * @brief Performance measurement helper
         */
        template<typename Func>
        static auto measureExecutionTime(Func&& func) -> std::pair<decltype(func()), std::chrono::microseconds>
        {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = func();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            return std::make_pair(result, duration);
        }

        /**
         * @brief Retry operation with exponential backoff
         */
        template<typename Func>
        static bool retryWithBackoff(Func&& func, int maxRetries = 3, std::chrono::milliseconds initialDelay = std::chrono::milliseconds(100))
        {
            for (int attempt = 0; attempt < maxRetries; ++attempt)
            {
                if (func())
                {
                    return true;
                }
                
                if (attempt < maxRetries - 1)
                {
                    auto delay = initialDelay * (1 << attempt); // Exponential backoff
                    std::this_thread::sleep_for(delay);
                }
            }
            
            return false;
        }

        /**
         * @brief Compare files for equality
         */
        static bool areFilesEqual(const std::filesystem::path& file1, const std::filesystem::path& file2)
        {
            if (!std::filesystem::exists(file1) || !std::filesystem::exists(file2))
            {
                return false;
            }
            
            if (std::filesystem::file_size(file1) != std::filesystem::file_size(file2))
            {
                return false;
            }
            
            std::ifstream f1(file1, std::ios::binary);
            std::ifstream f2(file2, std::ios::binary);
            
            if (!f1.is_open() || !f2.is_open())
            {
                return false;
            }
            
            return std::equal(
                std::istreambuf_iterator<char>(f1.rdbuf()),
                std::istreambuf_iterator<char>(),
                std::istreambuf_iterator<char>(f2.rdbuf())
            );
        }

        /**
         * @brief Generate test data with specific patterns
         */
        static std::string generatePatternedData(size_t size, const std::string& pattern)
        {
            std::string result;
            result.reserve(size);
            
            size_t patternIndex = 0;
            for (size_t i = 0; i < size; ++i)
            {
                result += pattern[patternIndex % pattern.length()];
                patternIndex++;
            }
            
            return result;
        }

        /**
         * @brief Create a test archive structure
         */
        static std::filesystem::path createTestArchiveStructure(const std::string& baseName = "test_archive")
        {
            auto archiveDir = createTempDirectory(baseName);
            
            // Create some test files
            auto file1 = archiveDir / "test1.txt";
            auto file2 = archiveDir / "subdir" / "test2.txt";
            
            std::filesystem::create_directories(file2.parent_path());
            
            std::ofstream(file1) << "Test file 1 content\n";
            std::ofstream(file2) << "Test file 2 content in subdirectory\n";
            
            return archiveDir;
        }

        /**
         * @brief Mock data generator for testing
         */
        class MockDataGenerator
        {
        public:
            static std::vector<std::string> generateStringList(size_t count, size_t minLength = 5, size_t maxLength = 20)
            {
                std::vector<std::string> result;
                result.reserve(count);
                
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<size_t> lengthDis(minLength, maxLength);
                
                for (size_t i = 0; i < count; ++i)
                {
                    result.push_back(generateRandomString(lengthDis(gen)));
                }
                
                return result;
            }

            static std::vector<int> generateIntegerList(size_t count, int minValue = 0, int maxValue = 1000)
            {
                std::vector<int> result;
                result.reserve(count);
                
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<int> valueDis(minValue, maxValue);
                
                for (size_t i = 0; i < count; ++i)
                {
                    result.push_back(valueDis(gen));
                }
                
                return result;
            }
        };
    };

    /**
     * @brief Test assertion helpers
     */
    class TestAssertions
    {
    public:
        /**
         * @brief Assert that a function throws a specific exception
         */
        template<typename ExceptionType, typename Func>
        static void assertThrows(Func&& func, const std::string& expectedMessage = "")
        {
            bool exceptionThrown = false;
            std::string actualMessage;
            
            try
            {
                func();
            }
            catch (const ExceptionType& e)
            {
                exceptionThrown = true;
                actualMessage = e.what();
            }
            catch (...)
            {
                FAIL() << "Unexpected exception type thrown";
            }
            
            ASSERT_TRUE(exceptionThrown) << "Expected exception was not thrown";
            
            if (!expectedMessage.empty())
            {
                EXPECT_THAT(actualMessage, testing::HasSubstr(expectedMessage));
            }
        }

        /**
         * @brief Assert that a function completes within a time limit
         */
        template<typename Func>
        static void assertCompletesWithin(Func&& func, std::chrono::milliseconds timeLimit)
        {
            auto [result, duration] = TestUtils::measureExecutionTime(func);
            auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
            
            EXPECT_LE(durationMs, timeLimit) 
                << "Operation took " << durationMs.count() 
                << "ms, expected <= " << timeLimit.count() << "ms";
        }

        /**
         * @brief Assert that a file has expected content
         */
        static void assertFileContent(const std::filesystem::path& filePath, const std::string& expectedContent)
        {
            ASSERT_TRUE(std::filesystem::exists(filePath)) << "File does not exist: " << filePath;
            
            std::ifstream file(filePath);
            std::string actualContent((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
            
            EXPECT_EQ(actualContent, expectedContent) << "File content mismatch in: " << filePath;
        }

        /**
         * @brief Assert that a directory structure matches expected layout
         */
        static void assertDirectoryStructure(const std::filesystem::path& dirPath, 
                                           const std::vector<std::string>& expectedFiles)
        {
            ASSERT_TRUE(std::filesystem::exists(dirPath)) << "Directory does not exist: " << dirPath;
            ASSERT_TRUE(std::filesystem::is_directory(dirPath)) << "Path is not a directory: " << dirPath;
            
            std::vector<std::string> actualFiles;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath))
            {
                if (entry.is_regular_file())
                {
                    auto relativePath = std::filesystem::relative(entry.path(), dirPath);
                    actualFiles.push_back(relativePath.string());
                }
            }
            
            std::sort(actualFiles.begin(), actualFiles.end());
            auto sortedExpected = expectedFiles;
            std::sort(sortedExpected.begin(), sortedExpected.end());
            
            EXPECT_EQ(actualFiles, sortedExpected) << "Directory structure mismatch";
        }
    };
}