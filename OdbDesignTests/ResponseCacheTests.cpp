#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include <App/DesignCache.h>
#include <FileModel/Design/FileArchive.h>
#include <design.pb.h>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;

namespace Odb::Test
{
    namespace
    {
        constexpr auto kParseTimeout = std::chrono::seconds(60);
        constexpr auto kBackgroundGrace = std::chrono::milliseconds(250);

        // Bounded poll for state that changes on background threads
        // (pre-serialization runs detached after a load completes); the
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

        // Runs the GetDesign RPC against the in-process service instance and
        // returns the response message.
        Odb::Lib::Protobuf::ProductModel::Design fetchDesign(
            OdbDesignServer::Services::OdbDesignServiceImpl& service,
            const std::string& designName, grpc::Status& status)
        {
            grpc::ServerContext ctx;
            Odb::Grpc::GetDesignRequest req;
            Odb::Lib::Protobuf::ProductModel::Design resp;
            req.set_design_name(designName);
            status = service.GetDesign(&ctx, &req, &resp);
            return resp;
        }
    }

    // =========================================================================
    // M1.4 tests: serialized-response cache.
    //
    // Determinism note: response payload population happens ONLY on the
    // detached background thread scheduled at load completion (the RPC cold
    // branch serializes directly for its own response and never populates —
    // dual concurrent tree builds thrashed memory on large designs); the
    // store accepts at most one commit per design per invalidation
    // generation, so "exactly one serialization" holds under any
    // interleaving. Waits are bounded polls (same pattern as
    // RequestLoadDesignTests).
    // =========================================================================
    class ResponseCacheTest : public FileArchiveLoadFixture
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
            m_defaultMaxBytes = m_sharedDesignCache->getCacheMaxBytes();
        }

        std::shared_ptr<DesignCache> m_sharedDesignCache;
        std::unique_ptr<OdbDesignServer::Services::OdbDesignServiceImpl> m_service;
        std::uint64_t m_defaultMaxBytes = 0;
    };

    // ---- warm GetDesign: identical bytes, exactly one serialization ----

    TEST_F(ResponseCacheTest, WarmGetDesign_ServesIdenticalBytes_WithExactlyOneSerialization)
    {
        // Full-design warm-path byte identity: each fetch parses and
        // re-serializes a multi-hundred-MB message in this debug build, which
        // runs minutes per fetch — beyond CI tolerance. Opt-in like the
        // benchmark (set ODB_DESIGN_SLOW_TESTS=1).
        if (std::getenv("ODB_DESIGN_SLOW_TESTS") == nullptr)
        {
            GTEST_SKIP() << "slow end-to-end warm-path test (set ODB_DESIGN_SLOW_TESTS=1)";
            return;
        }

        const std::string designName = "sample_design";

        // Warm-first via the background load so the (large) protobuf tree is
        // built exactly once, on the worker: a cold RPC fetch builds it on the
        // RPC thread concurrently with the worker, and two full trees
        // resident at once OOM-kill this VM.
        ASSERT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout)) << "background pre-serialization did not commit";
        EXPECT_EQ(m_sharedDesignCache->designSerializationCount(), 1ull)
            << "exactly one background serialization must populate the cache";

        grpc::Status status;
        const auto firstResponse = fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();

        std::string firstBytes;
        ASSERT_TRUE(firstResponse.SerializeToString(&firstBytes));
        EXPECT_FALSE(firstBytes.empty());

        // Warm fetches: byte-identical responses (parsed from the same cached
        // bytes), no additional serialization. One extra grace window lets any
        // in-flight background pre-serialization land before the final assert.
        for (int i = 0; i < 3; ++i)
        {
            const auto warmResponse = fetchDesign(*m_service, designName, status);
            ASSERT_TRUE(status.ok()) << status.error_message();
            std::string warmBytes;
            ASSERT_TRUE(warmResponse.SerializeToString(&warmBytes));
            EXPECT_EQ(warmBytes, firstBytes) << "warm fetch " << i << " served different bytes";
            EXPECT_EQ(warmResponse.ByteSizeLong(), firstResponse.ByteSizeLong());
        }

        std::string cachedBytes;
        EXPECT_TRUE(m_sharedDesignCache->TryGetDesignBytes(designName, cachedBytes));
        EXPECT_EQ(m_sharedDesignCache->cachedResponseBytes(), cachedBytes.size())
            << "cached bytes and served bytes must be the same payload";

        std::this_thread::sleep_for(kBackgroundGrace);
        EXPECT_EQ(m_sharedDesignCache->designSerializationCount(), 1ull)
            << "warm fetches and racing background pre-serialization must not re-serialize the design";
    }

    TEST_F(ResponseCacheTest, ColdThenWarm_ResponseEqualsDirectSerialization)
    {
        // See WarmGetDesign_ServesIdenticalBytes: multi-minute full-design
        // serialization round-trips; opt-in via ODB_DESIGN_SLOW_TESTS=1.
        if (std::getenv("ODB_DESIGN_SLOW_TESTS") == nullptr)
        {
            GTEST_SKIP() << "slow end-to-end byte-identity test (set ODB_DESIGN_SLOW_TESTS=1)";
            return;
        }

        // Golden cross-check: the payload the fast path serves is the same
        // message that serializing the loaded Design produces directly.
        // Warm-first (background load) so the tree is built once on the
        // worker and the direct build below runs sequentially — two
        // concurrent full trees OOM-kill this VM.
        const std::string designName = "sample_design";

        ASSERT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));

        auto pDesign = m_sharedDesignCache->GetDesign(designName);
        ASSERT_NE(pDesign, nullptr);
        auto pDirectMessage = pDesign->to_protobuf();
        ASSERT_NE(pDirectMessage, nullptr);

        grpc::Status status;
        const auto served = fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();

        // Byte equality of the two serializations (both trees are produced by
        // the same to_protobuf() from the same source object, so wire bytes are
        // deterministic). MessageDifferencer is deliberately not used: its
        // reflection path trips the dual-descriptor-pool check in this test
        // binary (see AGENTS.md protobuf workspace note).
        std::string servedBytes;
        ASSERT_TRUE(served.SerializeToString(&servedBytes));
        std::string directBytes;
        ASSERT_TRUE(pDirectMessage->SerializeToString(&directBytes));
        EXPECT_EQ(servedBytes.size(), directBytes.size())
            << "served=" << servedBytes.size() << " direct=" << directBytes.size();
        EXPECT_EQ(servedBytes, directBytes)
            << "the cached-response fast path must serve the same message as direct serialization";

        // The cache is warm after the fetch (the RPC populates lazily if the
        // background pass has not landed yet).
        EXPECT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));
    }

    // ---- eviction includes serialized bytes in the budget ----

    TEST_F(ResponseCacheTest, ByteBudget_IncludesSerializedBytes_EvictionDropsPayloads)
    {
        const std::string designName = "sample_design";

        grpc::Status status;
        (void)fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));

        const auto payloadBytes = m_sharedDesignCache->cachedResponseBytes();
        ASSERT_GT(payloadBytes, 0ull);

        std::error_code ec;
        const auto archiveBytes = static_cast<std::uint64_t>(
            std::filesystem::file_size(getIsolatedDesignPath("sample_design.tgz"), ec));
        ASSERT_FALSE(ec);

        // Budget sized to exactly archive + overhead + serialized payload:
        // fits, no eviction.
        m_sharedDesignCache->setCacheMaxBytes(archiveBytes + 4096 + payloadBytes);
        EXPECT_TRUE(m_sharedDesignCache->HasCachedResponse(designName))
            << "budget covering archive + serialized bytes must not evict";

        // One byte less: the serialized payload bytes must count toward the
        // budget, forcing eviction of the design (and dropping its payloads).
        m_sharedDesignCache->setCacheMaxBytes(archiveBytes + 4096 + payloadBytes - 1);
        EXPECT_FALSE(m_sharedDesignCache->HasCachedResponse(designName))
            << "serialized payload bytes must count toward the cache budget";
        EXPECT_EQ(m_sharedDesignCache->cachedResponseBytes(), 0ull);

        // The evicted design reloads and re-populates (a new generation, so a
        // new serialization is counted).
        m_sharedDesignCache->setCacheMaxBytes(m_defaultMaxBytes);
        const auto reloaded = fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();
        EXPECT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));
        EXPECT_EQ(m_sharedDesignCache->designSerializationCount(), 2ull);
    }

    // ---- invalidation on AddFileArchive (re-upload / POST /filemodels) ----

    TEST_F(ResponseCacheTest, AddFileArchive_InvalidatesCachedResponses)
    {
        const std::string designName = "sample_design";

        grpc::Status status;
        (void)fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));

        std::string warmBytes;
        ASSERT_TRUE(m_sharedDesignCache->TryGetDesignBytes(designName, warmBytes));
        EXPECT_FALSE(warmBytes.empty());

        const auto serializationsBefore = m_sharedDesignCache->designSerializationCount();

        // Re-upload (same path as POST /filemodels): invalidates the cached
        // Design and every serialized response derived from it.
        m_sharedDesignCache->AddFileArchive(designName, std::make_shared<Odb::Lib::FileModel::Design::FileArchive>(), false);

        EXPECT_FALSE(m_sharedDesignCache->HasCachedResponse(designName));
        std::string stale;
        EXPECT_FALSE(m_sharedDesignCache->TryGetDesignBytes(designName, stale))
            << "stale serialized design bytes must not survive a re-upload";
        EXPECT_FALSE(m_sharedDesignCache->TryGetJsonPayload(designName, DesignCache::JsonPayloadKind::Design, stale));
        EXPECT_FALSE(m_sharedDesignCache->TryGetJsonPayload(designName, DesignCache::JsonPayloadKind::FileModel, stale));

        // Invalidation does not itself serialize anything.
        EXPECT_EQ(m_sharedDesignCache->designSerializationCount(), serializationsBefore);
    }

    // ---- REST JSON payload exposure (controller follow-up wiring) ----

    TEST_F(ResponseCacheTest, TryGetJsonPayload_DeferredUntilRestWiring)
    {
        const std::string designName = "sample_design";

        grpc::Status status;
        (void)fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));

        // JSON payloads are deliberately not built by the background worker
        // (eager dual-JSON was the memory-thrash contributor); TryGetJsonPayload
        // reports not-present until the REST controller-wiring follow-up adds
        // lazy JSON population. The protobuf-bytes payload IS present.
        std::string payload;
        EXPECT_FALSE(m_sharedDesignCache->TryGetJsonPayload(designName, DesignCache::JsonPayloadKind::Design, payload));
        EXPECT_FALSE(m_sharedDesignCache->TryGetJsonPayload(designName, DesignCache::JsonPayloadKind::FileModel, payload));
        EXPECT_FALSE(m_sharedDesignCache->TryGetJsonPayload("no_such_design", DesignCache::JsonPayloadKind::Design, payload));

        std::string bytes;
        EXPECT_TRUE(m_sharedDesignCache->TryGetDesignBytes(designName, bytes));
        EXPECT_FALSE(bytes.empty());
    }

    // ---- M1.2 integration: background loads pre-serialize too ----

    TEST_F(ResponseCacheTest, BackgroundLoad_PreSerializesResponsePayloads)
    {
        const std::string designName = "sample_design";

        EXPECT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
        }, kParseTimeout));

        // The load-completion hook schedules a background pre-serialization;
        // it must populate the cache without any RPC arriving.
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout))
            << "load completion must pre-serialize response payloads in the background";

        std::string bytes;
        EXPECT_TRUE(m_sharedDesignCache->TryGetDesignBytes(designName, bytes));
        EXPECT_FALSE(bytes.empty());
        EXPECT_EQ(m_sharedDesignCache->designSerializationCount(), 1ull);
    }

    TEST_F(ResponseCacheTest, Clear_DropsAllResponsePayloads)
    {
        const std::string designName = "sample_design";

        grpc::Status status;
        (void)fetchDesign(*m_service, designName, status);
        ASSERT_TRUE(status.ok()) << status.error_message();
        ASSERT_TRUE(waitForCondition([this, &designName]()
        {
            return m_sharedDesignCache->HasCachedResponse(designName);
        }, kParseTimeout));

        m_sharedDesignCache->Clear();
        EXPECT_FALSE(m_sharedDesignCache->HasCachedResponse(designName));
        EXPECT_EQ(m_sharedDesignCache->cachedResponseBytes(), 0ull);
    }
}
