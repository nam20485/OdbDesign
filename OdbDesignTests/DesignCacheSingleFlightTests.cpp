#include <gtest/gtest.h>
#include "Fixtures/FileArchiveLoadFixture.h"
#include "Fixtures/TestUtils.h"
#include <App/DesignCache.h>
#include <App/OdbDesignArgs.h>
#include <FileModel/Design/FileArchive.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;

using OdbFileArchive = Odb::Lib::FileModel::Design::FileArchive;
using OdbDesign = Odb::Lib::ProductModel::Design;

namespace Odb::Lib::App
{
    // gtest printer so EXPECT_EQ failures on LoadState show names instead of ints
    inline void PrintTo(DesignCache::LoadState state, std::ostream* os)
    {
        switch (state)
        {
        case DesignCache::LoadState::Unloaded: *os << "Unloaded"; break;
        case DesignCache::LoadState::Loading: *os << "Loading"; break;
        case DesignCache::LoadState::Loaded: *os << "Loaded"; break;
        case DesignCache::LoadState::Failed: *os << "Failed"; break;
        }
    }
}

namespace Odb::Test
{
    namespace
    {
        // Counts DesignCache load-state transitions per design via the public
        // AddLoadObserver API. Shared with the observer lambda by shared_ptr so the
        // callback can never dangle regardless of teardown order.
        class LoadEventRecorder : public std::enable_shared_from_this<LoadEventRecorder>
        {
        public:
            DesignCache::LoadEventCallback callback()
            {
                auto self = shared_from_this();
                return [self](const std::string& name, DesignCache::LoadState from, DesignCache::LoadState to)
                {
                    (void)from;
                    std::lock_guard<std::mutex> lock(self->m_mutex);
                    if (to == DesignCache::LoadState::Loading) self->m_loadStarts[name]++;
                    if (to == DesignCache::LoadState::Loaded) self->m_loadCompletions[name]++;
                    if (to == DesignCache::LoadState::Failed) self->m_loadFailures[name]++;
                    if (to == DesignCache::LoadState::Unloaded) self->m_evictions[name]++;
                };
            }

            int loadStarts(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto findIt = m_loadStarts.find(name);
                return findIt != m_loadStarts.end() ? findIt->second : 0;
            }

            int loadCompletions(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto findIt = m_loadCompletions.find(name);
                return findIt != m_loadCompletions.end() ? findIt->second : 0;
            }

            int loadFailures(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto findIt = m_loadFailures.find(name);
                return findIt != m_loadFailures.end() ? findIt->second : 0;
            }

            int evictions(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto findIt = m_evictions.find(name);
                return findIt != m_evictions.end() ? findIt->second : 0;
            }

        private:
            mutable std::mutex m_mutex;
            std::map<std::string, int> m_loadStarts;
            std::map<std::string, int> m_loadCompletions;
            std::map<std::string, int> m_loadFailures;
            std::map<std::string, int> m_evictions;
        };

        bool contains(const std::vector<std::string>& names, const std::string& name)
        {
            return std::find(names.begin(), names.end(), name) != names.end();
        }
    }

    // =========================================================================
    // DesignCache M1.1 tests: single-flight loads, load state machine +
    // observers, and byte-budget LRU eviction.
    // =========================================================================
    // Determinism note: there is no parse-delay seam inside DesignCache (deliberate
    // — the parse path is real archive I/O). Parse-count assertions use the
    // observer API, which holds under ANY thread schedule (a warm path also counts
    // exactly one load start), so these tests cannot flake on slow/fast machines;
    // they only depend on single-flight correctness, not on overlap timing. The
    // largest test design (designodb_rigidflex) is used where mid-load observation
    // matters, because its parse window is by far the widest.
    // =========================================================================
    class DesignCacheSingleFlightTest : public FileArchiveLoadFixture
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

            m_recorder = std::make_shared<LoadEventRecorder>();
            m_pDesignCache->AddLoadObserver(m_recorder->callback());
        }

        std::shared_ptr<LoadEventRecorder> m_recorder;
    };

    // ---- single-flight loads ----

    TEST_F(DesignCacheSingleFlightTest, ConcurrentGetDesign_ColdDesign_ParsesExactlyOnce)
    {
        const std::string designName = "designodb_rigidflex";
        const int numThreads = 8;

        std::vector<std::shared_ptr<OdbDesign>> results(numThreads);
        std::vector<std::exception_ptr> errors(numThreads);

        // Release all threads at once so they race into the cold-load path together
        auto gate = std::make_shared<std::promise<void>>();
        auto release = gate->get_future().share();

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                release.wait();
                try
                {
                    results[i] = m_pDesignCache->GetDesign(designName);
                }
                catch (...)
                {
                    errors[i] = std::current_exception();
                }
            });
        }
        gate->set_value();
        for (auto& t : threads) t.join();

        for (int i = 0; i < numThreads; ++i)
        {
            EXPECT_EQ(errors[i], nullptr) << "thread " << i << " got an exception";
            EXPECT_NE(results[i], nullptr) << "thread " << i << " got nullptr";
        }

        // Single-flight: every caller sees the same parse instance
        for (int i = 1; i < numThreads; ++i)
        {
            EXPECT_EQ(results[0].get(), results[i].get())
                << "thread " << i << " got a different instance — single-flight broken";
        }

        EXPECT_EQ(m_recorder->loadStarts(designName), 1) << "cold design must parse exactly once";
        EXPECT_EQ(m_recorder->loadCompletions(designName), 1);
    }

    TEST_F(DesignCacheSingleFlightTest, ConcurrentGetFileArchive_ColdDesign_ParsesExactlyOnce)
    {
        const std::string designName = "designodb_rigidflex";
        const int numThreads = 8;

        std::vector<std::shared_ptr<OdbFileArchive>> results(numThreads);

        auto gate = std::make_shared<std::promise<void>>();
        auto release = gate->get_future().share();

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([&, i]()
            {
                release.wait();
                results[i] = m_pDesignCache->GetFileArchive(designName);
            });
        }
        gate->set_value();
        for (auto& t : threads) t.join();

        for (int i = 0; i < numThreads; ++i)
        {
            ASSERT_NE(results[i], nullptr) << "thread " << i << " got nullptr";
            if (i > 0)
            {
                EXPECT_EQ(results[0].get(), results[i].get())
                    << "thread " << i << " got a different instance — single-flight broken";
            }
        }

        EXPECT_EQ(m_recorder->loadStarts(designName), 1);
    }

    TEST_F(DesignCacheSingleFlightTest, WarmGet_ReturnsSameInstanceWithoutReloading)
    {
        auto pDesign1 = m_pDesignCache->GetDesign("sample_design");
        ASSERT_NE(pDesign1, nullptr);
        auto pDesign2 = m_pDesignCache->GetDesign("sample_design");
        ASSERT_NE(pDesign2, nullptr);
        EXPECT_EQ(pDesign1.get(), pDesign2.get()) << "warm GetDesign must be a cache hit";

        auto pArchive1 = m_pDesignCache->GetFileArchive("sample_design");
        ASSERT_NE(pArchive1, nullptr);
        auto pArchive2 = m_pDesignCache->GetFileArchive("sample_design");
        ASSERT_NE(pArchive2, nullptr);
        EXPECT_EQ(pArchive1.get(), pArchive2.get()) << "warm GetFileArchive must be a cache hit";

        EXPECT_EQ(m_recorder->loadStarts("sample_design"), 1) << "warm reads must not re-parse";
        EXPECT_EQ(m_recorder->loadCompletions("sample_design"), 1);
    }

    // ---- failed parse must not poison the cache ----

    TEST_F(DesignCacheSingleFlightTest, FailedParse_NotCached_SubsequentRequestRetries)
    {
        // Corrupt archive: garbage bytes that look like an archive by filename
        auto tempDir = TestUtils::createManagedTempDirectory("singleflight_broken");
        const auto brokenPath = tempDir->path() / "broken.tgz";
        {
            std::ofstream out(brokenPath, std::ios::binary);
            out << "this is not a valid archive";
        }

        auto recorder = std::make_shared<LoadEventRecorder>();
        DesignCache cache(tempDir->path().string());
        cache.AddLoadObserver(recorder->callback());

        // First attempt: parse fails, exception propagates, state is Failed
        EXPECT_THROW(cache.GetFileArchive("broken"), std::exception);
        EXPECT_EQ(cache.GetLoadState("broken"), DesignCache::LoadState::Failed);
        EXPECT_EQ(recorder->loadStarts("broken"), 1);
        EXPECT_EQ(recorder->loadFailures("broken"), 1);

        // The failure must not poison the cache: a second request retries the parse
        // (fresh load start), it is not served from a stuck Loading/Failed entry.
        EXPECT_THROW(cache.GetFileArchive("broken"), std::exception);
        EXPECT_EQ(recorder->loadStarts("broken"), 2);
        EXPECT_EQ(recorder->loadFailures("broken"), 2);

        // Once the archive is repaired, the retry succeeds and lands in the cache.
        // (ArchiveExtractor locates the design root by content, so the sample
        // design's bytes parse fine under the "broken" name.)
        std::error_code ec;
        std::filesystem::copy_file(getIsolatedDesignPath("sample_design.tgz"), brokenPath,
            std::filesystem::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << ec.message();

        auto pArchive = cache.GetFileArchive("broken");
        ASSERT_NE(pArchive, nullptr) << "repaired archive must load after prior failures";
        EXPECT_EQ(cache.GetLoadState("broken"), DesignCache::LoadState::Loaded);
        EXPECT_EQ(recorder->loadStarts("broken"), 3);
        EXPECT_EQ(recorder->loadCompletions("broken"), 1);
        EXPECT_EQ(pArchive.get(), cache.GetFileArchive("broken").get()) << "loaded archive must be cached";
    }

    // ---- byte-budget LRU eviction ----

    TEST_F(DesignCacheSingleFlightTest, ByteBudgetEviction_EvictsLeastRecentlyServedAndReloads)
    {
        const std::string first = "sample_design";         // served first -> LRU victim
        const std::string second = "designodb_rigidflex";

        // Tiny budget forces eviction of every previously-served design on insert
        m_pDesignCache->setCacheMaxBytes(1024);
        EXPECT_EQ(m_pDesignCache->getCacheMaxBytes(), 1024ull);

        auto pFirst = m_pDesignCache->GetDesign(first);
        ASSERT_NE(pFirst, nullptr);
        EXPECT_EQ(m_pDesignCache->GetLoadState(first), DesignCache::LoadState::Loaded);

        // Loading 'second' over budget evicts 'first' (least recently served, not in-flight)
        auto pSecond = m_pDesignCache->GetDesign(second);
        ASSERT_NE(pSecond, nullptr);
        EXPECT_EQ(m_pDesignCache->GetLoadState(second), DesignCache::LoadState::Loaded);

        auto loadedNames = m_pDesignCache->getLoadedDesignNames();
        EXPECT_FALSE(contains(loadedNames, first)) << "evicted design must leave the cache";
        EXPECT_TRUE(contains(loadedNames, second));
        EXPECT_FALSE(contains(m_pDesignCache->getLoadedFileArchiveNames(), first))
            << "eviction must drop the archive entry too";

        EXPECT_EQ(m_pDesignCache->GetLoadState(first), DesignCache::LoadState::Unloaded);
        EXPECT_EQ(m_recorder->evictions(first), 1);

        // The caller's shared_ptr stays valid — eviction drops the cache's reference,
        // never the object itself (Design holds its FileArchive via shared_ptr).
        EXPECT_NE(pFirst, nullptr);

        // The evicted design reloads (fresh parse) on the next request
        auto pFirstReloaded = m_pDesignCache->GetDesign(first);
        ASSERT_NE(pFirstReloaded, nullptr);
        EXPECT_NE(pFirstReloaded.get(), pFirst.get()) << "reload must be a fresh parse";
        EXPECT_EQ(m_recorder->loadStarts(first), 2);
        EXPECT_EQ(m_recorder->loadCompletions(first), 2);
    }

    TEST_F(DesignCacheSingleFlightTest, ByteBudgetEviction_NeverEvictsInFlightDesign)
    {
        const std::string big = "designodb_rigidflex";  // slowest parse: widest in-flight window
        const std::string small = "sample_design";

        // Pre-load 'small' so there is an evictable victim while 'big' is in flight
        m_pDesignCache->setCacheMaxBytes(8ull * 1024 * 1024);
        auto pSmall = m_pDesignCache->GetDesign(small);
        ASSERT_NE(pSmall, nullptr);

        std::shared_ptr<OdbDesign> pBig;
        std::exception_ptr loaderError;
        std::thread loader([&]()
        {
            try
            {
                pBig = m_pDesignCache->GetDesign(big);
            }
            catch (...)
            {
                loaderError = std::current_exception();
            }
        });

        // Wait for 'big' to go mid-flight (Loading). The state flips to Loading as
        // soon as the single-flight entry is created, before any parsing starts, so
        // this returns within microseconds of the parse beginning.
        bool observedLoading = false;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (std::chrono::steady_clock::now() < deadline)
        {
            const auto state = m_pDesignCache->GetLoadState(big);
            if (state == DesignCache::LoadState::Loading)
            {
                observedLoading = true;
                break;
            }
            if (state == DesignCache::LoadState::Loaded)
            {
                break;
            }
            std::this_thread::yield();
        }

        if (!observedLoading)
        {
            loader.join();
            GTEST_SKIP() << "design parsed faster than its Loading state could be observed; "
                            "in-flight eviction path not exercised";
        }

        // Shrink the budget while 'big' is in flight: 'small' (least recently served)
        // is evicted, the in-flight 'big' must be untouchable.
        m_pDesignCache->setCacheMaxBytes(1024);
        EXPECT_EQ(m_pDesignCache->GetLoadState(small), DesignCache::LoadState::Unloaded)
            << "victim should have been evicted during the in-flight window";
        EXPECT_EQ(m_pDesignCache->GetLoadState(big), DesignCache::LoadState::Loading)
            << "in-flight design must not be evicted";

        loader.join();
        ASSERT_EQ(loaderError, nullptr);
        ASSERT_NE(pBig, nullptr);

        // The completed in-flight load survived: still cached, no re-parse
        EXPECT_EQ(m_pDesignCache->GetLoadState(big), DesignCache::LoadState::Loaded);
        EXPECT_EQ(m_pDesignCache->GetDesign(big).get(), pBig.get());
        EXPECT_EQ(m_recorder->loadStarts(big), 1) << "in-flight design must never be dropped and re-parsed";
    }

    TEST_F(DesignCacheSingleFlightTest, ZeroBudget_DisablesEviction)
    {
        m_pDesignCache->setCacheMaxBytes(0);

        auto pSmall = m_pDesignCache->GetDesign("sample_design");
        ASSERT_NE(pSmall, nullptr);
        auto pBig = m_pDesignCache->GetDesign("designodb_rigidflex");
        ASSERT_NE(pBig, nullptr);

        // Combined size far exceeds any plausible budget; both must remain cached
        const auto names = m_pDesignCache->getLoadedDesignNames();
        EXPECT_TRUE(contains(names, "sample_design"));
        EXPECT_TRUE(contains(names, "designodb_rigidflex"));
    }

    // ---- --cache-max-mb argument ----

    TEST(DesignCacheSingleFlightArgsTest, CacheMaxMb_DefaultsTo4096)
    {
        char arg0[] = "OdbDesignTests";
        char* argv[] = { arg0 };
        OdbDesignArgs args(1, argv);
        EXPECT_EQ(args.cacheMaxMb(), 4096);
    }

    TEST(DesignCacheSingleFlightArgsTest, CacheMaxMb_ParsesOverride)
    {
        char arg0[] = "OdbDesignTests";
        char arg1[] = "--cache-max-mb";
        char arg2[] = "8192";
        char* argv[] = { arg0, arg1, arg2 };
        OdbDesignArgs args(3, argv);
        EXPECT_EQ(args.cacheMaxMb(), 8192);
    }

    TEST(DesignCacheSingleFlightArgsTest, CacheMaxMb_ZeroDisablesEviction)
    {
        char arg0[] = "OdbDesignTests";
        char arg1[] = "--cache-max-mb";
        char arg2[] = "0";
        char* argv[] = { arg0, arg1, arg2 };
        OdbDesignArgs args(3, argv);
        EXPECT_EQ(args.cacheMaxMb(), 0);

        Odb::Lib::App::DesignCache cache(std::filesystem::temp_directory_path().string());
        cache.setCacheMaxBytes(0);
        EXPECT_EQ(cache.getCacheMaxBytes(), 0ull);
    }
}
