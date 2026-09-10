#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "Fixtures/TestUtils.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include <App/DesignCache.h>
#include <FileModel/Design/FileArchive.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace Odb::Test::Utils;

namespace Odb::Test
{
    namespace
    {
        // Counts DesignCache load-state transitions per design via the public
        // AddLoadObserver API (same seam as DesignCacheSingleFlightTests). Shared
        // with the observer lambda by shared_ptr so the callback can never dangle.
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
                    if (to == DesignCache::LoadState::Unloaded) self->m_revertedToUnloaded[name]++;
                };
            }

            int loadStarts(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return count(m_loadStarts, name);
            }

            int loadCompletions(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return count(m_loadCompletions, name);
            }

            int loadFailures(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return count(m_loadFailures, name);
            }

            int revertsToUnloaded(const std::string& name)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                return count(m_revertedToUnloaded, name);
            }

        private:
            static int count(const std::map<std::string, int>& counts, const std::string& name)
            {
                auto findIt = counts.find(name);
                return findIt != counts.end() ? findIt->second : 0;
            }

            mutable std::mutex m_mutex;
            std::map<std::string, int> m_loadStarts;
            std::map<std::string, int> m_loadCompletions;
            std::map<std::string, int> m_loadFailures;
            std::map<std::string, int> m_revertedToUnloaded;
        };

        // Bounded poll for state/counters that change on background threads;
        // predicate is re-checked after the timeout so a late flip still passes.
        bool waitForCondition(std::function<bool()> predicate, std::chrono::milliseconds timeout)
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (std::chrono::steady_clock::now() < deadline)
            {
                if (predicate())
                {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return predicate();
        }

        constexpr auto kParseTimeout = std::chrono::seconds(60);
    }

    // =========================================================================
    // M1.2 tests: DesignCache::LoadDesignAsync (background load + semaphore)
    // and the RequestLoadDesign gRPC handler (pure status mapping).
    // =========================================================================
    // Determinism note: same seam as DesignCacheSingleFlightTests — assertions
    // count observer events (valid under any thread schedule); waits are bounded
    // polls, and timing-sensitive mid-load assertions GTEST_SKIP when the parse
    // finishes before they can be observed.
    // =========================================================================
    class RequestLoadDesignTest : public FileArchiveLoadFixture
    {
    protected:
        void SetUp() override
        {
            if (getTestDataDir().empty())
            {
                GTEST_SKIP() << "Test data directory not available (ODB_TEST_DATA_DIR not set)";
                return;
            }
            FileArchiveLoadFixture::SetUp();

            m_sharedDesignCache = std::shared_ptr<DesignCache>(m_pDesignCache.release());
            m_service = std::make_unique<OdbDesignServer::Services::OdbDesignServiceImpl>(m_sharedDesignCache);

            m_recorder = std::make_shared<LoadEventRecorder>();
            m_sharedDesignCache->AddLoadObserver(m_recorder->callback());
        }

        Odb::Grpc::RequestLoadDesignResponse requestLoad(const std::string& designName)
        {
            grpc::ServerContext ctx;
            Odb::Grpc::RequestLoadDesignRequest req;
            Odb::Grpc::RequestLoadDesignResponse resp;
            req.set_design_name(designName);
            m_lastStatus = m_service->RequestLoadDesign(&ctx, &req, &resp);
            return resp;
        }

        std::shared_ptr<DesignCache> m_sharedDesignCache;
        std::unique_ptr<OdbDesignServer::Services::OdbDesignServiceImpl> m_service;
        std::shared_ptr<LoadEventRecorder> m_recorder;
        grpc::Status m_lastStatus;
    };

    // ---- RequestLoadDesign handler: status mapping ----

    TEST_F(RequestLoadDesignTest, ColdDesign_ReturnsAccepted_ThenLoadsInBackground)
    {
        const std::string designName = "designodb_rigidflex";

        const auto resp = requestLoad(designName);
        EXPECT_TRUE(m_lastStatus.ok()) << m_lastStatus.error_message();
        EXPECT_EQ(resp.design_name(), designName);
        EXPECT_EQ(resp.status(), Odb::Grpc::LOAD_ACCEPTED);

        // The parse runs on the background thread; the state lands on Loaded
        // through the same single-flight path as a synchronous load.
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
        }, kParseTimeout)) << "design did not reach Loaded after an accepted background load";

        EXPECT_EQ(m_recorder->loadStarts(designName), 1);
        EXPECT_EQ(m_recorder->loadCompletions(designName), 1);

        // A subsequent request is served from the cache without re-parsing
        auto pDesign = m_sharedDesignCache->GetDesign(designName);
        ASSERT_NE(pDesign, nullptr);
        EXPECT_EQ(m_sharedDesignCache->GetLoadState(designName), DesignCache::LoadState::Loaded);
        EXPECT_EQ(m_recorder->loadStarts(designName), 1) << "warm GetDesign after background load must not re-parse";

        auto pFileArchive = m_sharedDesignCache->GetFileArchive(designName);
        EXPECT_NE(pFileArchive, nullptr);
    }

    TEST_F(RequestLoadDesignTest, WarmDesign_ReturnsAlreadyLoaded)
    {
        const std::string designName = "sample_design";

        auto pDesign = m_sharedDesignCache->GetDesign(designName);
        ASSERT_NE(pDesign, nullptr);

        const auto resp = requestLoad(designName);
        EXPECT_TRUE(m_lastStatus.ok()) << m_lastStatus.error_message();
        EXPECT_EQ(resp.design_name(), designName);
        EXPECT_EQ(resp.status(), Odb::Grpc::LOAD_ALREADY_LOADED);

        // No load was kicked for the warm design
        EXPECT_EQ(m_recorder->loadStarts(designName), 1) << "RequestLoadDesign on a warm design must not re-parse";
    }

    TEST_F(RequestLoadDesignTest, UnknownDesign_ReturnsNotFound_AndKicksNoLoad)
    {
        const std::string missing = "no_such_design_anywhere";

        const auto resp = requestLoad(missing);
        EXPECT_TRUE(m_lastStatus.ok()) << m_lastStatus.error_message();
        EXPECT_EQ(resp.design_name(), missing);
        EXPECT_EQ(resp.status(), Odb::Grpc::LOAD_NOT_FOUND);

        // The handler must resolve NOT_FOUND via ContainsDesign without
        // triggering a load: no observer events, no state entry, name stays
        // at the implicit Unloaded.
        EXPECT_EQ(m_recorder->loadStarts(missing), 0);
        EXPECT_EQ(m_sharedDesignCache->GetLoadState(missing), DesignCache::LoadState::Unloaded);
    }

    TEST_F(RequestLoadDesignTest, SecondCallDuringLoading_ReturnsAlreadyLoading)
    {
        const std::string designName = "designodb_rigidflex";  // slowest parse: widest Loading window

        const auto first = requestLoad(designName);
        EXPECT_EQ(first.status(), Odb::Grpc::LOAD_ACCEPTED);

        const auto second = requestLoad(designName);
        EXPECT_TRUE(m_lastStatus.ok());
        if (second.status() == Odb::Grpc::LOAD_ALREADY_LOADED)
        {
            GTEST_SKIP() << "design parsed faster than the second request could arrive; "
                            "already-loading path not exercised";
        }
        EXPECT_EQ(second.status(), Odb::Grpc::LOAD_ALREADY_LOADING);

        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));

        // The second request must not have started a second parse
        EXPECT_EQ(m_recorder->loadStarts(designName), 1);
    }

    // ---- DesignCache::LoadDesignAsync ----

    TEST_F(RequestLoadDesignTest, LoadDesignAsync_ObservesFullTransitionSet_AndCaches)
    {
        const std::string designName = "designodb_rigidflex";

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));

        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));

        EXPECT_EQ(m_recorder->loadStarts(designName), 1);
        EXPECT_EQ(m_recorder->loadCompletions(designName), 1);
        EXPECT_EQ(m_recorder->loadFailures(designName), 0);

        // Loaded result is cached: a warm read does not re-parse
        auto pDesign = m_sharedDesignCache->GetDesign(designName);
        ASSERT_NE(pDesign, nullptr);
        EXPECT_EQ(m_recorder->loadStarts(designName), 1);
    }

    TEST_F(RequestLoadDesignTest, LoadDesignAsync_SecondKickDuringLoad_ReturnsFalse)
    {
        const std::string designName = "designodb_rigidflex";

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));

        // The pending-load marker is registered synchronously by the first
        // kick, so a second kick during the load deterministically returns
        // false regardless of how far the background parse has progressed.
        EXPECT_FALSE(m_sharedDesignCache->LoadDesignAsync(designName));

        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));

        EXPECT_EQ(m_recorder->loadStarts(designName), 1) << "the second kick must not start a second parse";
    }

    TEST_F(RequestLoadDesignTest, LoadDesignAsync_FailedParse_FailedStateIsRetryable)
    {
        // Corrupt archive: garbage bytes that look like an archive by filename
        auto tempDir = TestUtils::createManagedTempDirectory("requestload_broken");
        const auto brokenPath = tempDir->path() / "broken.tgz";
        {
            std::ofstream out(brokenPath, std::ios::binary);
            out << "this is not a valid archive";
        }

        DesignCache cache(tempDir->path().string());
        auto recorder = std::make_shared<LoadEventRecorder>();
        cache.AddLoadObserver(recorder->callback());

        EXPECT_TRUE(cache.LoadDesignAsync("broken"));
        ASSERT_TRUE(waitForCondition([&]()
        {
            return cache.GetLoadState("broken") == DesignCache::LoadState::Failed;
        }, kParseTimeout));
        EXPECT_EQ(recorder->loadFailures("broken"), 1);

        // Failure must not poison the cache: a second kick retries the parse
        EXPECT_TRUE(cache.LoadDesignAsync("broken"));
        ASSERT_TRUE(waitForCondition([&]()
        {
            return recorder->loadFailures("broken") >= 2;
        }, kParseTimeout));
    }

    TEST_F(RequestLoadDesignTest, LoadDesignAsync_MissingDesign_LoadRunsThenStaysUnloaded)
    {
        const std::string missing = "no_such_design_anywhere";

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(missing));

        // The background load runs (single-flight start observed) and ends as a
        // miss: the state entry is erased (Loading -> Unloaded revert), no
        // Failed is recorded — identical to a synchronous miss.
        ASSERT_TRUE(waitForCondition([this, &missing]()
        {
            return m_recorder->revertsToUnloaded(missing) == 1;
        }, kParseTimeout));

        EXPECT_EQ(m_sharedDesignCache->GetLoadState(missing), DesignCache::LoadState::Unloaded);
        EXPECT_EQ(m_recorder->loadStarts(missing), 1);
        EXPECT_EQ(m_recorder->loadFailures(missing), 0);
    }

    TEST_F(RequestLoadDesignTest, LoadDesignAsync_RespectsMaxBackgroundLoads)
    {
        const std::string slow = "designodb_rigidflex";   // holds the single slot
        const std::string queued = "sample_design";

        m_sharedDesignCache->setMaxBackgroundLoads(1);
        EXPECT_EQ(m_sharedDesignCache->getMaxBackgroundLoads(), 1ull);

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(slow));

        // Wait for the slow load to hold the slot (parse running). If it
        // already completed, the semaphore path cannot be exercised.
        const bool slowLoading = waitForCondition([this, &slow]()
        {
            return m_sharedDesignCache->GetLoadState(slow) == DesignCache::LoadState::Loading;
        }, kParseTimeout);
        if (!slowLoading)
        {
            GTEST_SKIP() << "design parsed faster than its Loading state could be observed; "
                            "semaphore queueing path not exercised";
        }

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(queued));

        // The queued load must wait for the slot: it cannot have started
        // parsing while the slow load holds the semaphore.
        EXPECT_EQ(m_recorder->loadStarts(queued), 0)
            << "queued background load started parsing while the semaphore was held";

        // The queued load must not be announced as Loading either (its parse
        // has not begun; the state is the implicit Unloaded).
        EXPECT_EQ(m_sharedDesignCache->GetLoadState(queued), DesignCache::LoadState::Unloaded);

        // Both complete once the slot frees up
        ASSERT_TRUE(waitForCondition([this, &slow]()
        {
            return m_sharedDesignCache->GetLoadState(slow) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));
        ASSERT_TRUE(waitForCondition([this, &queued]()
        {
            return m_sharedDesignCache->GetLoadState(queued) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));

        EXPECT_EQ(m_recorder->loadStarts(slow), 1);
        EXPECT_EQ(m_recorder->loadStarts(queued), 1);
    }

    // ---- --max-background-loads argument ----

    TEST(RequestLoadDesignArgsTest, MaxBackgroundLoads_DefaultsTo2)
    {
        char arg0[] = "OdbDesignTests";
        char* argv[] = { arg0 };
        OdbDesignArgs args(1, argv);
        EXPECT_EQ(args.maxBackgroundLoads(), 2);
    }

    TEST(RequestLoadDesignArgsTest, MaxBackgroundLoads_ParsesOverride)
    {
        char arg0[] = "OdbDesignTests";
        char arg1[] = "--max-background-loads";
        char arg2[] = "4";
        char* argv[] = { arg0, arg1, arg2 };
        OdbDesignArgs args(3, argv);
        EXPECT_EQ(args.maxBackgroundLoads(), 4);
    }

    TEST(RequestLoadDesignArgsTest, MaxBackgroundLoads_ZeroMeansUnbounded)
    {
        char arg0[] = "OdbDesignTests";
        char arg1[] = "--max-background-loads";
        char arg2[] = "0";
        char* argv[] = { arg0, arg1, arg2 };
        OdbDesignArgs args(3, argv);
        EXPECT_EQ(args.maxBackgroundLoads(), 0);

        DesignCache cache(std::filesystem::temp_directory_path().string());
        cache.setMaxBackgroundLoads(0);
        EXPECT_EQ(cache.getMaxBackgroundLoads(), 0ull);
    }

    TEST(RequestLoadDesignArgsTest, MaxBackgroundLoads_InvalidValueFallsBackToDefault)
    {
        // A bare flag with no value stores the boolean `true`, which std::stoi
        // would reject; the hardened getter must fall back to the default.
        char arg0[] = "OdbDesignTests";
        char arg1[] = "--max-background-loads";
        char* argv[] = { arg0, arg1 };
        OdbDesignArgs args(2, argv);
        EXPECT_EQ(args.maxBackgroundLoads(), 2);
    }
}
