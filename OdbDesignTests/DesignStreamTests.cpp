#include <gtest/gtest.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Services/DesignStreamWalker.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <design.pb.h>
#include <service.grpc.pb.h>

using namespace Odb::Lib::App;
using namespace Odb::Test::Fixtures;
using namespace OdbDesignServer::Services;
using namespace OdbDesignServer::Config;
using namespace Odb::Grpc;

namespace Odb::Test
{
	namespace
	{
		constexpr auto kParseTimeout = std::chrono::seconds(60);

		// Deterministic serialization (map fields in sorted key order).
		//
		// Protobuf's DEFAULT mode serializes map fields in the message
		// instance's internal map iteration order, which is randomized per
		// message instance in this protobuf version (verified empirically:
		// two to_protobuf() calls on the same Design object produce
		// differently-ordered bytes, and so do two monolithic GetDesign cold
		// responses). Byte-for-byte comparison of two independent
		// serializations is therefore only meaningful with deterministic mode
		// applied identically to BOTH sides: sorted map order on each, so
		// equality proves content equality (the wire-equivalence gate).
		std::string SerializeDeterministic(const google::protobuf::MessageLite& message)
		{
			std::string out;
			google::protobuf::io::StringOutputStream stringStream(&out);
			google::protobuf::io::CodedOutputStream codedStream(&stringStream);
			codedStream.SetSerializationDeterministic(true);
			message.SerializeToCodedStream(&codedStream);
			return out;
		}

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

		// Byte comparison for multi-MB protobuf blobs: EXPECT_EQ on the raw
		// strings makes googletest build a unified diff of two 21 MB payloads
		// (quadratic edit distance — effectively a hang). This reports sizes,
		// the first differing offset, and a short hex window instead.
		::testing::AssertionResult BytesEqual(const char* what, const std::string& actual, const std::string& expected)
		{
			if (actual == expected)
			{
				return ::testing::AssertionSuccess();
			}

			std::size_t firstDiff = 0;
			const auto common = std::min(actual.size(), expected.size());
			while (firstDiff < common && actual[firstDiff] == expected[firstDiff])
			{
				++firstDiff;
			}

			auto hexWindow = [&firstDiff](const std::string& s)
			{
				const auto begin = firstDiff > 16 ? firstDiff - 16 : 0;
				const auto end = std::min(s.size(), firstDiff + 48);
				std::ostringstream ss;
				for (auto i = begin; i < end; ++i)
				{
					static const char* digits = "0123456789abcdef";
					ss << digits[(static_cast<unsigned char>(s[i]) >> 4) & 0xf]
						<< digits[static_cast<unsigned char>(s[i]) & 0xf] << ' ';
				}
				return ss.str();
			};

			return ::testing::AssertionFailure()
				<< what << " bytes differ: actual=" << actual.size() << "B expected=" << expected.size() << "B"
				<< " firstDiffOffset=" << firstDiff
				<< "\n  actual   @diff: " << hexWindow(actual)
				<< "\n  expected @diff: " << hexWindow(expected);
		}
	}

	// =========================================================================
	// M6 GetDesignStream tests.
	//
	// Wire-equivalence is the correctness contract: reassembling the
	// GetDesignStream chunk sequence (ApplyDesignChunk) and serializing it
	// must reproduce the exact monolithic GetDesign response bytes, for both
	// request flavours, on sample_design (the gate named in
	// getdesign-streaming-oom-plan.md §M6.5; the two large designs cannot be
	// compared against a monolithic response by definition — that response is
	// what OOMs).
	//
	// The dispatch guard (over-budget designs rejected by monolithic GetDesign
	// with RESOURCE_EXHAUSTED) is exercised by shrinking the serve budget so
	// sample_design counts as "large" — the same code path the real large
	// designs take, without the multi-GB fixture.
	// =========================================================================
	class DesignStreamTest : public FileArchiveLoadFixture
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
			m_config = GrpcServiceConfig::CreateDefault();
			// Small batches so multi-chunk paths (flush + remainder) are covered
			// on the small CI fixture design.
			m_config->design_stream_feature_batch_size = 100;
			m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);

			// In-process gRPC server on an ephemeral port: exercises the real
			// streaming machinery (ServerWriter, backpressure) end to end.
			grpc::ServerBuilder builder;
			int port = 0;
			builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
			builder.RegisterService(m_service.get());
			m_server = builder.BuildAndStart();
			ASSERT_NE(m_server, nullptr);
			ASSERT_GT(port, 0);

			m_stub = OdbDesignService::NewStub(MakeChannel(port));
		}

		void TearDown() override
		{
			if (m_server != nullptr)
			{
				m_server->Shutdown();
				m_server = nullptr;
			}
			m_stub.reset();
			m_service.reset();

			FileArchiveLoadFixture::TearDown();
		}

		// Runs the monolithic GetDesign RPC and returns the serialized bytes.
		std::string FetchMonolithicBytes(const std::string& designName, bool includeNormalizedLists,
			grpc::Status* statusOut = nullptr)
		{
			grpc::ClientContext ctx;
			GetDesignRequest req;
			req.set_design_name(designName);
			req.set_include_normalized_lists(includeNormalizedLists);
			Odb::Lib::Protobuf::ProductModel::Design resp;
			const auto status = m_stub->GetDesign(&ctx, req, &resp);
			if (statusOut != nullptr)
			{
				*statusOut = status;
			}
			std::string bytes;
			if (status.ok())
			{
				bytes = SerializeDeterministic(resp);
			}
			return bytes;
		}

		// Runs GetDesignStream, reassembles the chunks into a Design message,
		// and returns its serialized bytes (plus chunk/entry counters).
		struct StreamResult
		{
			std::string bytes;
			std::uint64_t chunks = 0;
			std::uint64_t featureRecords = 0;
			bool applyOk = true;
		};

		StreamResult StreamAndReassemble(const std::string& designName, bool includeNormalizedLists,
			grpc::Status* statusOut = nullptr)
		{
			grpc::ClientContext ctx;
			GetDesignRequest req;
			req.set_design_name(designName);
			req.set_include_normalized_lists(includeNormalizedLists);

			auto reader = m_stub->GetDesignStream(&ctx, req);
			Odb::Lib::Protobuf::ProductModel::Design reassembled;
			StreamResult result;
			DesignChunk chunk;
			while (reader->Read(&chunk))
			{
				++result.chunks;
				if (chunk.has_features())
				{
					result.featureRecords += static_cast<std::uint64_t>(chunk.features().records_size());
				}
				if (!ApplyDesignChunk(reassembled, chunk))
				{
					result.applyOk = false;
				}
			}
			const auto status = reader->Finish();
			if (statusOut != nullptr)
			{
				*statusOut = status;
			}
			if (status.ok())
			{
				result.bytes = SerializeDeterministic(reassembled);
			}
			return result;
		}

		static std::shared_ptr<grpc::Channel> MakeChannel(int port)
		{
			// sample_design's monolithic response is ~21 MB; the production
			// server raises the 4 MB default via config — mirror that here.
			grpc::ChannelArguments args;
			args.SetMaxReceiveMessageSize(256 * 1024 * 1024);
			return grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(port),
				grpc::InsecureChannelCredentials(), args);
		}

		std::shared_ptr<DesignCache> m_sharedDesignCache;
		std::shared_ptr<GrpcServiceConfig> m_config;
		std::unique_ptr<OdbDesignServiceImpl> m_service;
		std::unique_ptr<grpc::Server> m_server;
		std::unique_ptr<OdbDesignService::Stub> m_stub;
	};

	// ---- Gate 1: wire equivalence (both flavours, real RPC round-trip) ----

	TEST_F(DesignStreamTest, GetDesignStream_ReassemblesToMonolithicBytes_DefaultFlavor)
	{
		const std::string designName = "sample_design";

		grpc::Status status;
		const auto monolithicBytes = FetchMonolithicBytes(designName, /*includeNormalizedLists=*/false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_FALSE(monolithicBytes.empty());

		const auto streamed = StreamAndReassemble(designName, /*includeNormalizedLists=*/false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_TRUE(streamed.applyOk) << "chunk reassembly reported a malformed sequence";
		EXPECT_GT(streamed.chunks, 1ull) << "small batch size must produce multiple chunks";

		EXPECT_EQ(streamed.bytes.size(), monolithicBytes.size())
			<< "streamed=" << streamed.bytes.size() << " monolithic=" << monolithicBytes.size();
		EXPECT_EQ(streamed.bytes, monolithicBytes)
			<< "reassembled GetDesignStream must be byte-identical to monolithic GetDesign";
	}

	TEST_F(DesignStreamTest, GetDesignStream_ReassemblesToMonolithicBytes_NormalizedFlavor)
	{
		const std::string designName = "sample_design";

		grpc::Status status;
		const auto monolithicBytes = FetchMonolithicBytes(designName, /*includeNormalizedLists=*/true, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_FALSE(monolithicBytes.empty());

		const auto streamed = StreamAndReassemble(designName, /*includeNormalizedLists=*/true, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_TRUE(streamed.applyOk);

		EXPECT_TRUE(BytesEqual("normalized flavour", streamed.bytes, monolithicBytes))
			<< "normalized-flavour stream must be byte-identical to the normalized monolithic response";
	}

	// Walker-level equivalence without the server (cheap, no RPC): catches
	// regressions even if the gRPC plumbing changes.
	TEST_F(DesignStreamTest, WalkerOutput_ReassemblesToDirectSerialization)
	{
		const std::string designName = "sample_design";
		const auto pDesign = m_sharedDesignCache->GetDesign(designName);
		ASSERT_NE(pDesign, nullptr);

		for (const bool includeNormalized : { false, true })
		{
			const auto directBytes = SerializeDeterministic(*pDesign->to_protobuf(includeNormalized));

			Odb::Lib::Protobuf::ProductModel::Design reassembled;
			DesignStreamWalker::Stats stats;
			const bool completed = DesignStreamWalker::Walk(*pDesign, includeNormalized,
				DesignStreamOptions{},
				[&reassembled](const DesignChunk& chunk)
				{
					return ApplyDesignChunk(reassembled, chunk);
				},
				&stats);
			ASSERT_TRUE(completed);

			const auto streamBytes = SerializeDeterministic(reassembled);
			EXPECT_TRUE(BytesEqual(includeNormalized ? "walker normalized" : "walker default", streamBytes, directBytes))
				<< "walker output (includeNormalizedLists=" << includeNormalized << ") must be byte-identical";
		}
	}

	// ---- M6.2 dispatch: monolithic guard + streaming availability ----

	TEST_F(DesignStreamTest, MonolithicGetDesign_RejectsOverBudgetDesign_WithResourceExhausted)
	{
		const std::string designName = "sample_design";

		// Shrink the serve budget so sample_design counts as over-budget: the
		// same code path the large designs take on the real budget.
		m_sharedDesignCache->setMonolithicWireBudget(1);

		grpc::Status status;
		const auto bytes = FetchMonolithicBytes(designName, /*includeNormalizedLists=*/false, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED);
		EXPECT_NE(status.error_message().find("GetDesignStream"), std::string::npos)
			<< "the rejection must point clients at GetDesignStream";
		EXPECT_TRUE(bytes.empty());

		// The guard is RPC-level only: the cache itself still serves the
		// design object (REST /designs and file-model routes are unaffected).
		EXPECT_NE(m_sharedDesignCache->GetDesign(designName), nullptr);

		m_sharedDesignCache->setMonolithicWireBudget(0); // restore (disabled)
		grpc::Status restored;
		const auto bytesRestored = FetchMonolithicBytes(designName, false, &restored);
		EXPECT_TRUE(restored.ok()) << restored.error_message();
		EXPECT_FALSE(bytesRestored.empty());
	}

	TEST_F(DesignStreamTest, GetDesignStream_ReturnsUnimplementedWhenDisabled)
	{
		m_config->enable_design_streaming = false;
		m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);
		// Re-register on a fresh server bound to a fresh service instance.
		if (m_server != nullptr)
		{
			m_server->Shutdown();
			m_server = nullptr;
		}
		grpc::ServerBuilder builder;
		int port = 0;
		builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
		builder.RegisterService(m_service.get());
		m_server = builder.BuildAndStart();
		m_stub = OdbDesignService::NewStub(MakeChannel(port));

		grpc::Status status;
		const auto streamed = StreamAndReassemble("sample_design", false, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
	}

	TEST_F(DesignStreamTest, GetDesignStream_ReturnsNotFoundForMissingDesign)
	{
		grpc::Status status;
		const auto streamed = StreamAndReassemble("no_such_design_exists", false, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
	}

	// ---- M6.3: disk spool warm path ----

	TEST_F(DesignStreamTest, OverBudgetDesign_SpoolsAndServesWarmIdenticalBytes)
	{
		const std::string designName = "sample_design";

		// Reference: direct serialization of the same loaded design.
		const auto pDesign = m_sharedDesignCache->GetDesign(designName);
		ASSERT_NE(pDesign, nullptr);
		const auto directBytes = SerializeDeterministic(*pDesign->to_protobuf(false));

		m_sharedDesignCache->setMonolithicWireBudget(1); // sample_design -> over budget

		// Cold fill: walks the model AND spools to disk.
		grpc::Status status;
		const auto cold = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_TRUE(cold.applyOk);
		EXPECT_TRUE(BytesEqual("cold stream", cold.bytes, directBytes)) << "cold stream must stay wire-equivalent";
		EXPECT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(1));
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 0ull);
		EXPECT_GT(m_service->streamSpoolCache().TotalSpoolBytes(), 0ull);

		// Warm serve: re-emitted from the spool, byte-identical.
		const auto warm = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(BytesEqual("warm spool serve", warm.bytes, cold.bytes))
			<< "warm spool serve must be byte-identical to the cold fill";
		EXPECT_EQ(warm.chunks, cold.chunks);
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 1ull);

		// Invalidation: InvalidateDesign bumps the response generation, so the
		// spool must be dropped and the next fill must re-walk (still
		// wire-equivalent — the archive on disk did not change).
		m_sharedDesignCache->InvalidateDesign(designName);
		const auto refilled = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(BytesEqual("post-invalidation refilled stream", refilled.bytes, directBytes));
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 1ull)
			<< "a stale-generation spool must not be served warm";

		// And the refilled spool serves warm again.
		const auto warmAgain = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(BytesEqual("warm-again spool serve", warmAgain.bytes, directBytes));
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 2ull);

		m_sharedDesignCache->setMonolithicWireBudget(0);
	}

	// ---- background pre-serialization skips over-budget designs ----

	TEST_F(DesignStreamTest, BackgroundPreSerialization_SkipsOverBudgetDesigns)
	{
		const std::string designName = "sample_design";

		m_sharedDesignCache->setMonolithicWireBudget(1);
		ASSERT_TRUE(m_sharedDesignCache->LoadDesignAsync(designName));
		ASSERT_TRUE(waitForCondition([this, &designName]()
		{
			return m_sharedDesignCache->GetLoadState(designName) == DesignCache::LoadState::Loaded;
		}, kParseTimeout));

		// The pre-serialization worker must skip the monolithic tree build for
		// an over-budget design: no in-RAM payload appears (the design is
		// served by GetDesignStream + spool instead).
		EXPECT_FALSE(waitForCondition([this, &designName]()
		{
			return m_sharedDesignCache->HasCachedResponse(designName);
		}, std::chrono::seconds(2)))
			<< "over-budget designs must not get a monolithic in-RAM response payload";

		m_sharedDesignCache->setMonolithicWireBudget(0);
	}
}
