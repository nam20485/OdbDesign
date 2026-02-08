#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "Fixtures/TestUtils.h"
#include <App/DesignCache.h>
#include <FileModel/Design/FileArchive.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <filesystem>
#include <functional>
#include <algorithm>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;
using namespace testing;

using OdbFileArchive = Odb::Lib::FileModel::Design::FileArchive;
using OdbDesign = Odb::Lib::ProductModel::Design;

namespace Odb::Test::ThreadSafety
{

    // =========================================================================
    // DesignCache Thread Safety Tests
    // =========================================================================
    // Tests verifying that DesignCache operations are thread-safe with
    // std::shared_mutex (Phase 1 implementation).
    // =========================================================================

    /**
     * @brief Fixture for DesignCache thread safety tests.
     *
     * Uses a temp directory so no real archive I/O interferes.
     * Focuses on testing the locking and concurrency behavior.
     */
    class DesignCacheThreadSafetyTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_tempDir = TestUtils::createManagedTempDirectory("threadtest_cache");
            m_cache = std::make_unique<DesignCache>(m_tempDir->path().string());
        }

        void TearDown() override
        {
            m_cache.reset();
            m_tempDir.reset();
        }

        std::unique_ptr<DesignCache> m_cache;
        std::unique_ptr<TestUtils::TempResource> m_tempDir;
    };

    // ---- Basic locking correctness ----

    TEST_F(DesignCacheThreadSafetyTest, GetDirectory_IsThreadSafe)
    {
        const std::string expectedDir = m_tempDir->path().string();
        const int numThreads = 8;
        const int readsPerThread = 200;
        std::atomic<int> failures{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&]()
            {
                for (int j = 0; j < readsPerThread; ++j)
                {
                    const auto& dir = m_cache->getDirectory();
                    if (dir != expectedDir)
                    {
                        failures.fetch_add(1);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_EQ(failures.load(), 0) << "Concurrent getDirectory() returned inconsistent values";
    }

    TEST_F(DesignCacheThreadSafetyTest, SetDirectoryAndGetDirectory_ConcurrentReadWrite)
    {
        const int numReaders = 6;
        const int numWriters = 2;
        const int opsPerThread = 100;
        std::atomic<bool> stop{false};
        std::atomic<int> readCount{0};
        std::atomic<int> writeCount{0};

        auto newDir = TestUtils::createManagedTempDirectory("threadtest_newdir");

        std::vector<std::thread> threads;

        // Readers
        for (int i = 0; i < numReaders; ++i)
        {
            threads.emplace_back([&]()
            {
                while (!stop.load())
                {
                    auto dir = m_cache->getDirectory();
                    // Should always be a non-empty valid string
                    EXPECT_FALSE(dir.empty());
                    readCount.fetch_add(1);
                }
            });
        }

        // Writers
        for (int i = 0; i < numWriters; ++i)
        {
            threads.emplace_back([&, i]()
            {
                for (int j = 0; j < opsPerThread; ++j)
                {
                    if (j % 2 == 0)
                    {
                        m_cache->setDirectory(m_tempDir->path().string());
                    }
                    else
                    {
                        m_cache->setDirectory(newDir->path().string());
                    }
                    writeCount.fetch_add(1);
                    std::this_thread::yield();
                }
            });
        }

        // Wait for writers to finish, then stop readers
        for (int i = numReaders; i < static_cast<int>(threads.size()); ++i)
        {
            threads[i].join();
        }
        stop.store(true);
        for (int i = 0; i < numReaders; ++i)
        {
            threads[i].join();
        }

        EXPECT_GT(readCount.load(), 0);
        EXPECT_EQ(writeCount.load(), numWriters * opsPerThread);
    }

    TEST_F(DesignCacheThreadSafetyTest, Clear_IsThreadSafe)
    {
        const int numThreads = 4;
        const int opsPerThread = 50;
        std::atomic<int> completedOps{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&]()
            {
                for (int j = 0; j < opsPerThread; ++j)
                {
                    m_cache->Clear();
                    completedOps.fetch_add(1);
                    std::this_thread::yield();
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_EQ(completedOps.load(), numThreads * opsPerThread);
    }

    TEST_F(DesignCacheThreadSafetyTest, GetLoadedNames_DuringClear)
    {
        // Interleave Clear() with getLoadedDesignNames() / getLoadedFileArchiveNames()
        const int numReaders = 4;
        const int iterations = 100;
        std::atomic<bool> stop{false};
        std::atomic<int> readCount{0};

        std::vector<std::thread> threads;

        // Readers for design names
        for (int i = 0; i < numReaders / 2; ++i)
        {
            threads.emplace_back([&]()
            {
                while (!stop.load())
                {
                    auto names = m_cache->getLoadedDesignNames();
                    readCount.fetch_add(1);
                }
            });
        }

        // Readers for file archive names
        for (int i = 0; i < numReaders / 2; ++i)
        {
            threads.emplace_back([&]()
            {
                while (!stop.load())
                {
                    auto names = m_cache->getLoadedFileArchiveNames();
                    readCount.fetch_add(1);
                }
            });
        }

        // Writer that calls Clear
        threads.emplace_back([&]()
        {
            for (int j = 0; j < iterations; ++j)
            {
                m_cache->Clear();
                std::this_thread::yield();
            }
        });

        // Wait for writer
        threads.back().join();
        stop.store(true);
        for (int i = 0; i < numReaders; ++i)
        {
            threads[i].join();
        }

        EXPECT_GT(readCount.load(), 0);
    }

    TEST_F(DesignCacheThreadSafetyTest, GetUnloadedDesignNames_IsThreadSafe)
    {
        const int numThreads = 4;
        const int iterations = 50;
        std::atomic<int> failures{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&]()
            {
                for (int j = 0; j < iterations; ++j)
                {
                    try
                    {
                        auto names = m_cache->getUnloadedDesignNames();
                        // Should succeed without exception
                    }
                    catch (...)
                    {
                        failures.fetch_add(1);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_EQ(failures.load(), 0) << "Concurrent getUnloadedDesignNames() threw exceptions";
    }

    TEST_F(DesignCacheThreadSafetyTest, ConcurrentGetFileArchive_NonExistentDesign)
    {
        // Multiple threads requesting non-existent designs should not crash
        const int numThreads = 8;
        std::atomic<int> nullResults{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                auto result = m_cache->GetFileArchive("nonexistent_design_" + std::to_string(i));
                if (result == nullptr)
                {
                    nullResults.fetch_add(1);
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_EQ(nullResults.load(), numThreads) << "All non-existent design lookups should return nullptr";
    }

    TEST_F(DesignCacheThreadSafetyTest, ConcurrentGetDesign_NonExistentDesign)
    {
        // Multiple threads requesting non-existent designs should not crash
        const int numThreads = 8;
        std::atomic<int> nullResults{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                auto result = m_cache->GetDesign("nonexistent_design_" + std::to_string(i));
                if (result == nullptr)
                {
                    nullResults.fetch_add(1);
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_EQ(nullResults.load(), numThreads) << "All non-existent design lookups should return nullptr";
    }

    TEST_F(DesignCacheThreadSafetyTest, ConcurrentMixedOperations_NoDeadlock)
    {
        // Run a mix of read and write operations concurrently.
        // The test passes if it completes without deadlocking (timeout would indicate deadlock).
        const int iterations = 50;
        std::atomic<int> completedOps{0};

        auto newDir = TestUtils::createManagedTempDirectory("threadtest_mixops");

        std::vector<std::thread> threads;

        // Thread 1: getDirectory + setDirectory
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                m_cache->getDirectory();
                completedOps.fetch_add(1);
                std::this_thread::yield();
                m_cache->setDirectory(m_tempDir->path().string());
                completedOps.fetch_add(1);
            }
        });

        // Thread 2: getLoadedDesignNames + getLoadedFileArchiveNames
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                m_cache->getLoadedDesignNames();
                completedOps.fetch_add(1);
                m_cache->getLoadedFileArchiveNames();
                completedOps.fetch_add(1);
            }
        });

        // Thread 3: Clear
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                m_cache->Clear();
                completedOps.fetch_add(1);
                std::this_thread::yield();
            }
        });

        // Thread 4: getUnloadedDesignNames
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                try
                {
                    m_cache->getUnloadedDesignNames();
                }
                catch (...)
                {
                    // Directory may not exist after setDirectory — that's OK
                }
                completedOps.fetch_add(1);
            }
        });

        for (auto& t : threads) t.join();
        EXPECT_GT(completedOps.load(), 0) << "Should have completed some operations without deadlocking";
    }

    // =========================================================================
    // DesignCache Thread Safety Tests with Real Data (FileArchiveLoadFixture)
    // =========================================================================

    class DesignCacheThreadSafetyWithDataTest : public FileArchiveLoadFixture
    {
    protected:
        void SetUp() override
        {
            if (getTestDataDir().empty())
            {
                GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
                return;
            }
            // Check if test data directory actually contains any design archives
            bool hasDesigns = false;
            if (std::filesystem::exists(getTestDataDir()))
            {
                for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator(getTestDataDir()))
                {
                    hasDesigns = true;
                    break;
                }
            }
            if (!hasDesigns)
            {
                GTEST_SKIP() << "Test data directory is empty — no design archives available";
                return;
            }
            FileArchiveLoadFixture::SetUp();
        }
    };

    TEST_F(DesignCacheThreadSafetyWithDataTest, ConcurrentGetFileArchive_SameDesign)
    {
        // Multiple threads asking for the same design concurrently.
        // Double-checked locking should ensure only one load, all get the same pointer.
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available";
        }

        const int numThreads = 4;
        std::vector<std::shared_ptr<OdbFileArchive>> results(numThreads);

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                results[i] = m_pDesignCache->GetFileArchive("sample_design");
            });
        }

        for (auto& t : threads) t.join();

        // All should have returned non-null
        for (int i = 0; i < numThreads; ++i)
        {
            EXPECT_NE(results[i], nullptr) << "Thread " << i << " got nullptr";
        }

        // All should point to the same cached object (same shared_ptr value)
        for (int i = 1; i < numThreads; ++i)
        {
            EXPECT_EQ(results[0].get(), results[i].get())
                << "Thread " << i << " got a different pointer — double-checked locking may be broken";
        }
    }

    TEST_F(DesignCacheThreadSafetyWithDataTest, ConcurrentGetDesign_SameDesign)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available";
        }

        const int numThreads = 4;
        std::vector<std::shared_ptr<OdbDesign>> results(numThreads);

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                results[i] = m_pDesignCache->GetDesign("sample_design");
            });
        }

        for (auto& t : threads) t.join();

        for (int i = 0; i < numThreads; ++i)
        {
            EXPECT_NE(results[i], nullptr) << "Thread " << i << " got nullptr";
        }

        for (int i = 1; i < numThreads; ++i)
        {
            EXPECT_EQ(results[0].get(), results[i].get())
                << "Thread " << i << " got a different pointer — double-checked locking may be broken";
        }
    }

    TEST_F(DesignCacheThreadSafetyWithDataTest, ConcurrentGetFileArchive_DifferentDesigns)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available";
        }

        // Load two different designs concurrently
        std::shared_ptr<OdbFileArchive> result1;
        std::shared_ptr<OdbFileArchive> result2;

        std::thread t1([&]() { result1 = m_pDesignCache->GetFileArchive("sample_design"); });
        std::thread t2([&]() { result2 = m_pDesignCache->GetFileArchive("designodb_rigidflex"); });

        t1.join();
        t2.join();

        EXPECT_NE(result1, nullptr);
        EXPECT_NE(result2, nullptr);
        EXPECT_TRUE(result1.get() != result2.get()) << "Different designs should be different objects";
    }

    TEST_F(DesignCacheThreadSafetyWithDataTest, ConcurrentGetAndClear)
    {
        if (getTestDataDir().empty())
        {
            GTEST_SKIP() << "Test data directory not available";
        }

        // Pre-load the cache
        auto preloaded = m_pDesignCache->GetFileArchive("sample_design");
        ASSERT_NE(preloaded, nullptr);

        const int iterations = 20;
        std::atomic<int> completedOps{0};

        std::vector<std::thread> threads;

        // Reader
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                // May return nullptr if Clear just ran — that's fine
                m_pDesignCache->GetFileArchive("sample_design");
                completedOps.fetch_add(1);
            }
        });

        // Writer that clears cache
        threads.emplace_back([&]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                m_pDesignCache->Clear();
                completedOps.fetch_add(1);
                std::this_thread::yield();
            }
        });

        for (auto& t : threads) t.join();
        EXPECT_GT(completedOps.load(), 0) << "Operations should complete without deadlock";
    }

} // namespace Odb::Test::ThreadSafety
