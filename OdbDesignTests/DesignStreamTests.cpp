#include <gtest/gtest.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Services/DesignStreamWalker.h"
#include "OdbDesignServer/Services/StreamSpoolCache.h"
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

		// BytesEqual, not raw EXPECT_EQ: on divergence the raw form makes
		// googletest diff two ~21 MB strings (quadratic edit distance —
		// effectively a hang); the helper reports sizes + first-diff offset.
		EXPECT_TRUE(BytesEqual("default flavour", streamed.bytes, monolithicBytes))
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

	// Presence parity on a degenerate (no-file-model, no-net) design: the
	// monolith always creates the connectivity submessage (D4 always-on) and
	// never creates filemodel when there is none — the chunk stream must
	// match both presence semantics byte-for-byte, not just the field
	// contents. Every corpus design has nets, so the wire-equivalence gates
	// above cannot catch a presence divergence.
	TEST_F(DesignStreamTest, WalkerOutput_NoNetDesign_MatchesMonolithicPresence)
	{
		const Odb::Lib::ProductModel::Design noNetDesign;

		const auto directBytes = SerializeDeterministic(*noNetDesign.to_protobuf(/*includeNormalizedLists=*/false));
		EXPECT_TRUE(noNetDesign.to_protobuf(false)->has_connectivity())
			<< "monolithic invariant: connectivity is always present (D4)";

		Odb::Lib::Protobuf::ProductModel::Design reassembled;
		DesignStreamWalker::Stats stats;
		const bool completed = DesignStreamWalker::Walk(noNetDesign, /*includeNormalizedLists=*/false,
			DesignStreamOptions{},
			[&reassembled](const DesignChunk& chunk)
			{
				return ApplyDesignChunk(reassembled, chunk);
			},
			&stats);
		ASSERT_TRUE(completed);
		EXPECT_TRUE(reassembled.has_connectivity())
			<< "a no-net design must still emit one (empty) ConnectivityBatch so "
			<< "reassembly creates field 12 like the monolith";
		EXPECT_FALSE(reassembled.has_filemodel())
			<< "a design without a file model must not gain an empty filemodel field";

		const auto streamBytes = SerializeDeterministic(reassembled);
		EXPECT_TRUE(BytesEqual("no-net design", streamBytes, directBytes))
			<< "walker output for a no-net design must be byte-identical to the monolith";
	}

	// ---- sanitized-key collisions follow the monolith's last-wins ----
	//
	// Two raw names from the CP1252 undefined slots (0x81, 0x8D) both
	// sanitize to U+FFFD (ProtobufMapHelpers.h documents the collision
	// class). The monolith's map assignment keeps only the LAST raw entry;
	// the walker must emit FeatureBatches for surviving entries only, or the
	// shadowed entry's records would append into the survivor (duplicating
	// features the monolith dropped) or address a layer the survivor lacks
	// (failing reassembly). The product model is built via the in-memory
	// protobuf -> Design::from_protobuf chain; the input message carries
	// invalid-UTF-8 map keys and must never itself be serialized.
	TEST_F(DesignStreamTest, WalkerOutput_LayerNameCollision_LastWins_MatchesMonolith)
	{
		Odb::Lib::Protobuf::ProductModel::Design designMessage;
		auto* stepMessage = &(*designMessage.mutable_filemodel()->mutable_stepsbyname())["step"];
		const std::string rawFirst(1, '\x81');
		const std::string rawSecond(1, '\x8D');
		for (const auto& rawName : { rawFirst, rawSecond })
		{
			auto* layerMessage = &(*stepMessage->mutable_layersbyname())[rawName];
			auto* record = layerMessage->mutable_featurefile()->add_featurerecords();
			record->set_xs(1.0);
			record->set_xe(2.0);
		}

		Odb::Lib::ProductModel::Design design;
		design.from_protobuf(designMessage);
		ASSERT_EQ(design.GetFileModel()->GetStepsByName().at("step")->GetLayersByName().size(), std::size_t(2))
			<< "from_protobuf must rebuild both RAW layer names";

		const auto monolithBytes = SerializeDeterministic(*design.to_protobuf(/*includeNormalizedLists=*/false));

		std::uint64_t featureChunks = 0;
		Odb::Lib::Protobuf::ProductModel::Design reassembled;
		DesignStreamWalker::Stats stats;
		const bool completed = DesignStreamWalker::Walk(design, /*includeNormalizedLists=*/false,
			DesignStreamOptions{},
			[&reassembled, &featureChunks](const DesignChunk& chunk)
			{
				if (chunk.has_features())
				{
					++featureChunks;
				}
				return ApplyDesignChunk(reassembled, chunk);
			},
			&stats);
		ASSERT_TRUE(completed);

		// std::map order: "\x81" < "\x8D", so the last holder of the
		// sanitized key is the 0x8D layer — exactly one FeatureBatch.
		EXPECT_EQ(featureChunks, 1ull)
			<< "the shadowed raw layer's feature batch must not be emitted";

		const auto streamBytes = SerializeDeterministic(reassembled);
		EXPECT_TRUE(BytesEqual("layer-name collision", streamBytes, monolithBytes))
			<< "colliding layer names must reassemble to the monolith's last-wins bytes";
	}

	TEST_F(DesignStreamTest, WalkerOutput_StepNameCollision_LastWins_MatchesMonolith)
	{
		Odb::Lib::Protobuf::ProductModel::Design designMessage;
		const std::string rawFirst(1, '\x81');
		const std::string rawSecond(1, '\x8D');
		for (const auto& rawName : { rawFirst, rawSecond })
		{
			auto* stepMessage = &(*designMessage.mutable_filemodel()->mutable_stepsbyname())[rawName];
			auto* layerMessage = &(*stepMessage->mutable_layersbyname())["top"];
			auto* record = layerMessage->mutable_featurefile()->add_featurerecords();
			record->set_xs(3.0);
			record->set_xe(4.0);
		}

		Odb::Lib::ProductModel::Design design;
		design.from_protobuf(designMessage);
		ASSERT_EQ(design.GetFileModel()->GetStepsByName().size(), std::size_t(2))
			<< "from_protobuf must rebuild both RAW step names";

		const auto monolithBytes = SerializeDeterministic(*design.to_protobuf(/*includeNormalizedLists=*/false));

		std::uint64_t featureChunks = 0;
		Odb::Lib::Protobuf::ProductModel::Design reassembled;
		DesignStreamWalker::Stats stats;
		const bool completed = DesignStreamWalker::Walk(design, /*includeNormalizedLists=*/false,
			DesignStreamOptions{},
			[&reassembled, &featureChunks](const DesignChunk& chunk)
			{
				if (chunk.has_features())
				{
					++featureChunks;
				}
				return ApplyDesignChunk(reassembled, chunk);
			},
			&stats);
		ASSERT_TRUE(completed);

		EXPECT_EQ(featureChunks, 1ull)
			<< "the shadowed raw step's feature batch must not be emitted";
		ASSERT_EQ(reassembled.filemodel().stepsbyname().size(), std::size_t(1));
		const auto& survivingStep = reassembled.filemodel().stepsbyname().begin()->second;
		ASSERT_EQ(survivingStep.layersbyname().size(), std::size_t(1));
		EXPECT_EQ(survivingStep.layersbyname().begin()->second.featurefile().featurerecords_size(), 1)
			<< "the surviving step must carry exactly its own feature record (no duplication)";

		const auto streamBytes = SerializeDeterministic(reassembled);
		EXPECT_TRUE(BytesEqual("step-name collision", streamBytes, monolithBytes))
			<< "colliding step names must reassemble to the monolith's last-wins bytes";
	}

	// ---- malformed / out-of-order reassembly is rejected (review F12) ----
	// ApplyDesignChunk's false-returns are the corrupt/hostile-input safety
	// property M6.4 clients inherit; every other test feeds well-formed
	// walker output and asserts success. These drive each rejection branch.
	TEST_F(DesignStreamTest, ApplyDesignChunk_RejectsMalformedSequences)
	{
		// Step before header: no filemodel exists yet.
		Odb::Lib::Protobuf::ProductModel::Design design;
		DesignChunk stepChunk;
		stepChunk.mutable_step()->set_step_name("step");
		EXPECT_FALSE(ApplyDesignChunk(design, stepChunk))
			<< "a StepSkeleton before the header must be rejected";
		EXPECT_FALSE(design.has_filemodel())
			<< "the rejected step must not fabricate a filemodel";

		// FeatureBatch before header.
		DesignChunk featuresChunk;
		auto* batch = featuresChunk.mutable_features();
		batch->set_step_name("step");
		batch->set_layer_name("top");
		EXPECT_FALSE(ApplyDesignChunk(design, featuresChunk))
			<< "a FeatureBatch before the header must be rejected";

		// Header (carrying a file_archive — the walker emits one only when
		// the design has a file model; without it no filemodel exists and
		// every step/feature chunk would be rejected for the wrong reason),
		// then a FeatureBatch naming a step no skeleton delivered.
		DesignChunk headerChunk;
		headerChunk.mutable_header()->set_name("d");
		headerChunk.mutable_header()->mutable_file_archive();
		EXPECT_TRUE(ApplyDesignChunk(design, headerChunk));

		// ... and an existing step with an undelivered layer.
		EXPECT_TRUE(ApplyDesignChunk(design, stepChunk));
		EXPECT_FALSE(ApplyDesignChunk(design, featuresChunk))
			<< "a FeatureBatch naming an undelivered layer must be rejected";

		// Payload-less chunk.
		DesignChunk emptyChunk;
		EXPECT_FALSE(ApplyDesignChunk(design, emptyChunk))
			<< "a payload-less chunk must be rejected";
	}

	// ---- estimator pin (review F2): the serve-budget guard's sole input ----
	// EstimateWireSizeBytes decides the monolithic reject, the spool-vs-plain
	// streaming split and (M6.6) cache residency charging; nothing else
	// asserts its magnitude — the over-budget unit test uses budget=1, which
	// passes for any nonzero estimate. A dropped counting term is a
	// systematic LOW estimate, the exact regression that would silently
	// re-admit an over-budget design to the monolithic tree build (the pod
	// OOM this guard exists for). Pin estimate >= 0.5x actual serialized
	// size on the CI corpus, both flavours: the coefficients are
	// deliberately generous, so this guards term DISAPPEARANCE, not
	// precision.
	TEST_F(DesignStreamTest, EstimateWireSizeBytes_StaysAboveHalfOfActualSerializedSize)
	{
		for (const auto& designName : { std::string("sample_design"), std::string("designodb_rigidflex") })
		{
			const auto pDesign = m_sharedDesignCache->GetDesign(designName);
			ASSERT_NE(pDesign, nullptr) << designName;

			for (const bool includeNormalized : { false, true })
			{
				const auto pMessage = pDesign->to_protobuf(includeNormalized);
				ASSERT_NE(pMessage, nullptr);
				const auto actualBytes = static_cast<std::uint64_t>(pMessage->ByteSizeLong());
				ASSERT_GT(actualBytes, 0ull);

				const auto estimateBytes = pDesign->EstimateWireSizeBytes(includeNormalized);
				EXPECT_GE(estimateBytes, actualBytes / 2)
					<< "design=" << designName
					<< " includeNormalizedLists=" << includeNormalized
					<< " estimate=" << estimateBytes
					<< " actual=" << actualBytes
					<< " — a dropped estimator term risks re-admitting over-budget designs to the monolithic build";
			}
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

	// ---- client cancellation mid-fill (review F10) ----
	// Contract: a cancelled stream is not a server error, and the in-flight
	// spool fill must ABORT, never commit. The client observes CANCELLED (or
	// OK if the trailer won the race); EntryCount()==0 proves the
	// abort-not-publish branch ran.
	TEST_F(DesignStreamTest, GetDesignStream_ClientCancelMidFill_AbortsSpool)
	{
		const std::string designName = "sample_design";
		m_sharedDesignCache->setMonolithicWireBudget(1); // force the spooling path

		grpc::ClientContext ctx;
		GetDesignRequest req;
		req.set_design_name(designName);
		auto reader = m_stub->GetDesignStream(&ctx, req);

		DesignChunk chunk;
		std::uint64_t read = 0;
		while (reader->Read(&chunk))
		{
			++read;
			chunk.Clear();
			if (read == 2)
			{
				ctx.TryCancel(); // thread-safe out-of-band cancel
				break;
			}
		}
		while (reader->Read(&chunk)) // drain: returns false promptly post-cancel
		{
			++read;
			chunk.Clear();
		}
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";
		EXPECT_GE(read, 1ull) << "the stream must have delivered chunks before the cancel";

		// The server handler observes IsCancelled per chunk and aborts the
		// fill; give it a moment, then assert nothing was ever published.
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		EXPECT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(0))
			<< "a cancelled fill must not commit its spool";
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 0ull);

		m_sharedDesignCache->setMonolithicWireBudget(0);
	}

	// ---- warm-path spool corruption returns INTERNAL (review F10 + follow-up) ----
	// A committed spool that goes bad on disk must end the warm request with
	// INTERNAL — never a clean OK on a truncated chunk sequence — and the
	// poisoned entry must be dropped so the next request refills cold. Three
	// truncation shapes: mid-frame (framing error), at an exact record
	// boundary (clean EOF short of the committed record count), and emptied
	// to zero bytes. StreamSpoolCache exposes no path accessor, so the test
	// configures a known spool_dir and globs the *.spool file.
	TEST_F(DesignStreamTest, GetDesignStream_WarmSpoolCorruption_ReturnsInternalAndDrops)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto spoolDir = fs::temp_directory_path(ec) / ("odbdesign-spool-corrupt-test-" +
			std::to_string(::getpid()) + "-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		ASSERT_FALSE(ec);

		m_config->stream_spool_dir = spoolDir.string();
		m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);
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
		ASSERT_NE(m_server, nullptr);
		m_stub = OdbDesignService::NewStub(MakeChannel(port));

		const std::string designName = "sample_design";
		m_sharedDesignCache->setMonolithicWireBudget(1);
		grpc::Status status;
		const auto cold = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(1));

		auto globSpoolFile = [&]() -> fs::path
		{
			std::vector<fs::path> spoolFiles;
			for (const auto& entry : fs::directory_iterator(spoolDir, ec))
			{
				if (entry.path().extension() == ".spool")
				{
					spoolFiles.push_back(entry.path());
				}
			}
			EXPECT_EQ(spoolFiles.size(), std::size_t(1)) << "one committed fill, one spool file";
			return spoolFiles.empty() ? fs::path{} : spoolFiles.front();
		};

		// Corrupt the current spool to `newSize`, expect the warm request to
		// end INTERNAL with the entry dropped, then refill cold (restores a
		// committed spool for the next round). The refill result lands in
		// *refillOut for the caller's byte-equality checks. ASSERTs inside
		// return from this void lambda only.
		auto corruptAndExpectInternal = [&](std::uintmax_t newSize, const char* what, StreamResult* refillOut)
		{
			const auto spoolPath = globSpoolFile();
			ASSERT_FALSE(spoolPath.empty()) << what;
			if (spoolPath.empty()) return;
			fs::resize_file(spoolPath, newSize, ec);
			ASSERT_FALSE(ec) << what;

			grpc::Status s;
			const auto warm = StreamAndReassemble(designName, false, &s);
			EXPECT_EQ(s.error_code(), grpc::StatusCode::INTERNAL)
				<< what << ": a warm read of a corrupt spool must end INTERNAL, not OK (" << s.error_message() << ")";
			EXPECT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(0))
				<< what << ": the poisoned spool must be dropped";

			*refillOut = StreamAndReassemble(designName, false, &s);
			ASSERT_TRUE(s.ok()) << what << ": " << s.error_message();
			ASSERT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(1))
				<< what << ": next request must refill cold";
		};

		// Byte offset just past the FIRST record (varint length + payload):
		// truncating there leaves a structurally valid prefix — clean EOF one
		// record short of the committed count.
		auto firstRecordEnd = [](const fs::path& p) -> std::uintmax_t
		{
			std::ifstream in(p, std::ios::binary);
			std::uint64_t value = 0;
			int shift = 0;
			std::uintmax_t consumed = 0;
			for (int i = 0; i < 10; ++i)
			{
				const int b = in.get();
				if (b == std::char_traits<char>::eof())
				{
					return 0;
				}
				++consumed;
				value |= static_cast<std::uint64_t>(b & 0x7f) << shift;
				if ((b & 0x80) == 0)
				{
					return consumed + value;
				}
				shift += 7;
			}
			return 0;
		};

		{
			const auto spoolPath = globSpoolFile();
			ASSERT_FALSE(spoolPath.empty());
			const auto fullSize = fs::file_size(spoolPath, ec);
			ASSERT_GT(fullSize, std::uintmax_t(0));
			const auto boundary = firstRecordEnd(spoolPath);
			ASSERT_GT(boundary, std::uintmax_t(0));
			ASSERT_LT(boundary, fullSize) << "fixture spool must have more than one record";

			// Round 1: emptied to zero bytes (degenerate boundary truncation).
			StreamResult r1;
			corruptAndExpectInternal(0, "zero-byte spool", &r1);
			// Round 2: truncated at an exact record boundary (clean EOF,
			// short of the committed record count).
			StreamResult r2;
			corruptAndExpectInternal(boundary, "record-boundary truncation", &r2);
			// Round 3: mid-frame truncation (framing error path).
			StreamResult r3;
			corruptAndExpectInternal((boundary + fs::file_size(globSpoolFile(), ec)) / 2,
				"mid-frame truncation", &r3);

			EXPECT_TRUE(BytesEqual("post-corruption refill", r1.bytes, cold.bytes));
			EXPECT_TRUE(BytesEqual("post-boundary refill", r2.bytes, cold.bytes));
			EXPECT_TRUE(BytesEqual("post-mid-frame refill", r3.bytes, cold.bytes));
		}

		m_sharedDesignCache->setMonolithicWireBudget(0);
		fs::remove_all(spoolDir, ec);
	}

	// ---- cached monolithic bytes obey a since-lowered budget (follow-up to
	// the M6.2 dispatch guard) ----
	// The warm GetDesign fast path parses cached bytes BEFORE the serve
	// budget guard. The payload was serialized under the budget active at
	// pre-serialization time; if the budget is then lowered below the cached
	// payload's size, parsing would allocate the exact protobuf tree the
	// guard exists to prevent. The warm path must refuse and the cold guard
	// must reject the design.
	TEST_F(DesignStreamTest, GetDesign_CachedBytesOverShrunkBudget_Rejected)
	{
		const std::string designName = "sample_design";

		// Load the design (the pre-serialization worker is scheduled at load
		// completion), then wait for it to land the cached bytes (~21 MB for
		// sample_design, well under the default budget).
		ASSERT_NE(m_sharedDesignCache->GetDesign(designName), nullptr);
		ASSERT_TRUE(waitForCondition([this, &designName]()
		{
			return m_sharedDesignCache->HasCachedResponse(designName);
		}, std::chrono::seconds(30))) << "background pre-serialization did not commit";

		// Shrink the budget below the cached payload size (and below the
		// estimate, so the cold guard also rejects): the cached-bytes path
		// must not serve it.
		m_sharedDesignCache->setMonolithicWireBudget(1000000);

		grpc::Status status;
		const auto bytes = FetchMonolithicBytes(designName, false, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED)
			<< "a cached payload larger than the current budget must be rejected, not parsed: "
			<< status.error_message();
		EXPECT_TRUE(bytes.empty());

		m_sharedDesignCache->setMonolithicWireBudget(0);
	}

	// ---- response generations survive DesignCache::Clear() ----
	// Clear() used to erase the generation tombstones, so a re-added design's
	// next invalidation restarted at 1 — and a spool published at generation
	// >= 1 before the Clear would MATCH the post-Clear reload's fresh
	// generation, warm-serving content the system had invalidated. With
	// tombstones preserved, the reload's generation is strictly higher and
	// the stale spool is dropped on open.
	TEST_F(DesignStreamTest, SpoolGeneration_SurvivesCacheClear_NoStaleWarmServe)
	{
		const std::string designName = "sample_design";
		m_sharedDesignCache->setMonolithicWireBudget(1);

		// Bump the generation to 1 (what Clear used to erase) and fill: the
		// committed spool is now at generation 1.
		m_sharedDesignCache->InvalidateDesign(designName);
		grpc::Status status;
		const auto cold = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(1));

		// Clear + reload: without preserved tombstones the reload lands at
		// generation 1 again and the pre-Clear spool would warm-serve.
		m_sharedDesignCache->Clear();
		ASSERT_NE(m_sharedDesignCache->GetDesign(designName), nullptr) << "reload after Clear failed";

		const auto after = StreamAndReassemble(designName, false, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		EXPECT_EQ(m_service->streamSpoolCache().WarmOpens(), 0ull)
			<< "a pre-Clear spool must never match after Clear + reload";
		EXPECT_TRUE(BytesEqual("post-clear refill", after.bytes, cold.bytes));

		m_sharedDesignCache->setMonolithicWireBudget(0);
	}

	// ---- spool-I/O failure degrades to unspooled streaming (review F14) ----
	// The PR-body resilience claim: spool trouble must never fail the wire
	// response. Configure a spool dir that cannot be created (its parent is
	// a regular file) — the cache reports unusable, BeginWrite fails, and
	// the over-budget stream must still complete OK with identical bytes and
	// no published entry.
	TEST_F(DesignStreamTest, GetDesignStream_UnusableSpoolDir_StillServesIdenticalBytes)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto blocker = fs::temp_directory_path(ec) / ("odbdesign-spool-blocker-" +
			std::to_string(::getpid()) + "-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		ASSERT_FALSE(ec);
		{
			std::ofstream ofs(blocker); // a regular FILE...
			ofs << "x";
		}
		ASSERT_TRUE(fs::is_regular_file(blocker));

		m_config->stream_spool_dir = (blocker / "sub").string(); // ...whose child dir cannot exist
		m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);
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
		ASSERT_NE(m_server, nullptr);
		m_stub = OdbDesignService::NewStub(MakeChannel(port));

		const std::string designName = "sample_design";
		const auto pDesign = m_sharedDesignCache->GetDesign(designName);
		ASSERT_NE(pDesign, nullptr);
		const auto directBytes = SerializeDeterministic(*pDesign->to_protobuf(false));

		m_sharedDesignCache->setMonolithicWireBudget(1); // design counts as over-budget
		grpc::Status status;
		const auto streamed = StreamAndReassemble(designName, false, &status);
		EXPECT_TRUE(status.ok()) << "spool I/O trouble must never fail the stream: " << status.error_message();
		EXPECT_TRUE(BytesEqual("unspooled degrade", streamed.bytes, directBytes))
			<< "degraded (unspooled) streaming must stay wire-identical";
		EXPECT_EQ(m_service->streamSpoolCache().EntryCount(), std::size_t(0));

		m_sharedDesignCache->setMonolithicWireBudget(0);
		fs::remove(blocker, ec);
	}

	// ---- M6.3: concurrent cold fills must not share one spool file ----

	// Two cold fills of the same (design, generation) racing (two clients
	// streaming an over-budget design before any spool exists) must each
	// write their own file: sharing one path opened with std::ios::trunc
	// interleaves the length-delimited frames and publishes a corrupt spool
	// whose warm replay parses garbage mid-stream. The cache-level test
	// reproduces the exact concurrency; the winner must publish a spool that
	// replays the full record sequence cleanly (what a warm
	// GetDesignStream client would receive).
	TEST_F(DesignStreamTest, StreamSpoolCache_ConcurrentColdFills_PublishIntactSpool)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto base = fs::temp_directory_path(ec);
		ASSERT_FALSE(ec);
		const auto dir = base / ("odbdesign-spool-concurrent-test-" +
			std::to_string(::getpid()) + "-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

		constexpr std::uint64_t generation = 7;
		StreamSpoolCache cache(dir, 8ull << 30);

		// One deterministic chunk sequence both fills append.
		std::vector<DesignChunk> records;
		for (int i = 0; i < 500; ++i)
		{
			DesignChunk chunk;
			chunk.mutable_header()->set_name("record-" + std::to_string(i));
			records.push_back(chunk);
		}

			std::atomic<int> fillsReady{ 0 };
			auto fill = [&]()
			{
				// Count this thread BEFORE any early return: the barrier
				// below spins until both threads arrive, so a BeginWrite
				// failure must still release the other thread — a red test,
				// not a wedged binary (review F15).
				++fillsReady;
				StreamSpoolCache::Writer writer;
				if (!cache.BeginWrite("design", generation, writer))
				{
					return false;
				}
				// Both writers hold their file open before either appends, so
				// the frames genuinely race if they share a path.
				while (fillsReady.load() < 2)
				{
					std::this_thread::yield();
				}
			for (const auto& record : records)
			{
				if (!writer.Append(record))
				{
					return false;
				}
			}
			writer.Commit();
			return true;
		};

		std::thread fillerA(fill);
		std::thread fillerB(fill);
		fillerA.join();
		fillerB.join();

		EXPECT_EQ(cache.EntryCount(), std::size_t(1))
			<< "the last committer must own the single published entry";

		StreamSpoolCache::Reader reader;
		ASSERT_TRUE(cache.TryOpenReader("design", generation, reader));
		std::uint64_t recordsRead = 0;
		DesignChunk chunk;
		while (reader.Next(chunk))
		{
			ASSERT_LT(recordsRead, records.size())
				<< "published spool replayed more records than were appended";
			EXPECT_EQ(chunk.SerializeAsString(), records[recordsRead].SerializeAsString())
				<< "published spool frame " << recordsRead << " must be intact (no interleaving)";
			++recordsRead;
			chunk.Clear();
		}
		EXPECT_FALSE(reader.Failed())
			<< "the published spool must parse cleanly to its end (a shared/truncated "
			<< "file would fail the varint framing or protobuf parse mid-stream)";
		EXPECT_EQ(recordsRead, records.size())
			<< "the published spool must replay the complete record sequence";

		std::filesystem::remove_all(dir, ec);
	}

	// ---- a single spool larger than the whole budget is not published ----
	// The just-published design is eviction-protected, so admitting an
	// individually oversized spool would evict every other entry and still
	// leave the cache over cap indefinitely. It must be rejected at publish:
	// file deleted, nothing cached, Commit() reports failure (the design
	// keeps serving; every request refills cold).
	TEST_F(DesignStreamTest, StreamSpoolCache_SingleSpoolOverBudget_NotPublished)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		const auto dir = fs::temp_directory_path(ec) / ("odbdesign-spool-oversize-test-" +
			std::to_string(::getpid()) + "-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		ASSERT_FALSE(ec);

		StreamSpoolCache cache(dir, 4096); // budget smaller than one fill

		std::vector<DesignChunk> records;
		for (int i = 0; i < 500; ++i)
		{
			DesignChunk chunk;
			chunk.mutable_header()->set_name("record-" + std::to_string(i));
			records.push_back(chunk);
		}

		StreamSpoolCache::Writer writer;
		ASSERT_TRUE(cache.BeginWrite("design", 3, writer));
		for (const auto& record : records)
		{
			ASSERT_TRUE(writer.Append(record));
		}
		EXPECT_FALSE(writer.Commit()) << "an oversized spool must not be published";
		EXPECT_EQ(cache.EntryCount(), std::size_t(0));

		std::size_t leftover = 0;
		for (const auto& entry : fs::directory_iterator(dir, ec))
		{
			if (entry.path().extension() == ".spool")
			{
				++leftover;
			}
		}
		EXPECT_EQ(leftover, std::size_t(0)) << "the rejected spool file must be deleted";

		fs::remove_all(dir, ec);
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
