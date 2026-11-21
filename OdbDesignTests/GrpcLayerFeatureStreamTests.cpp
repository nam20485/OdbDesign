#include <gtest/gtest.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"
#include "OdbDesignServer/Config/GrpcServiceConfig.h"
#include <App/DesignCache.h>
#include <FileModel/Design/FileArchive.h>
#include <FileModel/Design/StepDirectory.h>
#include <FileModel/Design/LayerDirectory.h>
#include <FileModel/Design/FeaturesFile.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <featuresfile.pb.h>
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
		// Deterministic serialization (map fields in sorted key order).
		//
		// FeatureRecord carries a map field (attributeLookupTable); protobuf's
		// DEFAULT mode serializes maps in per-instance randomized order, so
		// raw SerializeAsString comparison of two independently built records
		// can diverge spuriously. Applying deterministic mode identically to
		// BOTH sides makes byte equality prove content equality (the same
		// rationale as DesignStreamTests' SerializeDeterministic).
		std::string SerializeDeterministic(const google::protobuf::MessageLite& message)
		{
			std::string out;
			google::protobuf::io::StringOutputStream stringStream(&out);
			google::protobuf::io::CodedOutputStream codedStream(&stringStream);
			codedStream.SetSerializationDeterministic(true);
			message.SerializeToCodedStream(&codedStream);
			return out;
		}

		// Bounded drain loops for cancellation tests: after TryCancel the
		// reader must hit end-of-stream promptly, and the drain COUNTS what
		// it consumes so callers can assert on the total delivered — a
		// count-free drain would silently swallow post-cancel deliveries and
		// let a server that ignored the cancel pass the test.
		struct DrainResult
		{
			std::uint64_t batches = 0;
			std::uint64_t records = 0;
			bool completed = false; // false => budget exceeded before end-of-stream
		};

		DrainResult drainWithin(grpc::ClientReader<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>* reader,
			std::chrono::seconds budget)
		{
			DrainResult result;
			Odb::Lib::Protobuf::FeaturesFile::FeatureRecord record;
			const auto deadline = std::chrono::steady_clock::now() + budget;
			while (reader->Read(&record))
			{
				++result.records;
				record.Clear();
				if (std::chrono::steady_clock::now() > deadline)
				{
					return result;
				}
			}
			result.completed = true;
			return result;
		}

		DrainResult drainBatchesWithin(grpc::ClientReader<FeatureRecordBatch>* reader,
			std::chrono::seconds budget)
		{
			DrainResult result;
			FeatureRecordBatch batch;
			const auto deadline = std::chrono::steady_clock::now() + budget;
			while (reader->Read(&batch))
			{
				++result.batches;
				result.records += static_cast<std::uint64_t>(batch.features_size());
				batch.Clear();
				if (std::chrono::steady_clock::now() > deadline)
				{
					return result;
				}
			}
			result.completed = true;
			return result;
		}
	}

	// =========================================================================
	// M2 GetLayerFeaturesStream / GetLayerFeaturesBatchStream real-transport
	// coverage (release-readiness U4).
	//
	// Both streaming RPCs were previously exercised only via direct
	// service-method invocation (GrpcBatchStreamTests early-error tests) or
	// not at all. These tests run a REAL loopback gRPC server (ServerBuilder
	// + generated stub, the DesignStreamTests pattern) so the actual
	// ServerWriter path, wire framing, flow control and Finish-status
	// propagation are exercised end to end.
	//
	// Corpus pins (probe-verified against the current TEST_DATA fixtures):
	//   sample_design / step "step":
	//     layer-1         -> 11868 feature records (INCH)
	//     board-outline.doc ->   10 feature records
	//     comp_+_top      ->    0 feature records (empty layer, key exists)
	//     layer-5         -> 1284 feature records
	// =========================================================================
	class LayerFeatureStreamTest : public FileArchiveLoadFixture
	{
	protected:
		static constexpr const char* kDesign = "sample_design";
		static constexpr const char* kStep = "step";
		static constexpr const char* kLayerWithFeatures = "layer-1";
		static constexpr const char* kSmallLayer = "board-outline.doc";
		static constexpr const char* kMidLayer = "layer-5";
		static constexpr const char* kEmptyLayer = "comp_+_top";
		static constexpr const char* kMissingName = "no_such_layer_exists";

		// GrpcServiceConfig clamps batch_size to [100, 1000]; 250 divides
		// 11868 into 47 full batches + a 118-record remainder, covering both
		// batching paths on the pinned fixture.
		static constexpr int kBatchSize = 250;
		static constexpr std::uint64_t kLayer1RecordCount = 11868;
		static constexpr std::uint64_t kLayer1FullBatches = 47;
		static constexpr std::uint64_t kLayer1Remainder = 118;

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
			m_config->batch_size = kBatchSize;
			ASSERT_TRUE(StartServer());
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

		// Builds service+server+stub from the CURRENT config. Used by SetUp
		// and by tests that flip a config flag (e.g. enable_batch_streaming)
		// and need a fresh service instance registered on a live server.
		// maxSendBytes (optional) caps the server's send-message size: a
		// value below one feature record makes ServerWriter::Write fail on a
		// LIVE call without any client cancellation — the only loopback shape
		// that exercises the write-failure branch of the fail-loud contract
		// from a well-behaved client.
		bool StartServer(int maxSendBytes = 0)
		{
			m_stub.reset();
			if (m_server != nullptr)
			{
				m_server->Shutdown();
				m_server = nullptr;
			}
			m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);

			grpc::ServerBuilder builder;
			int port = 0;
			builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
			if (maxSendBytes > 0)
			{
				builder.SetMaxSendMessageSize(maxSendBytes);
			}
			builder.RegisterService(m_service.get());
			m_server = builder.BuildAndStart();
			if (m_server == nullptr || port <= 0)
			{
				return false;
			}
			m_stub = OdbDesignService::NewStub(MakeChannel(port));
			return m_stub != nullptr;
		}

		static std::shared_ptr<grpc::Channel> MakeChannel(int port)
		{
			grpc::ChannelArguments args;
			args.SetMaxReceiveMessageSize(256 * 1024 * 1024);
			return grpc::CreateCustomChannel("127.0.0.1:" + std::to_string(port),
				grpc::InsecureChannelCredentials(), args);
		}

		// Direct feature records for a layer, straight from the parsed file
		// model (the same source the RPC handler reads), each deterministically
		// serialized. layersByName is a std::map (file model), so find() is
		// the ordinary accessor here — the protobuf-Map iteration-only rule
		// (#599) does not apply to it.
		std::vector<std::string> DirectRecordBytes(const std::string& designName,
			const std::string& stepName, const std::string& layerName)
		{
			std::vector<std::string> bytes;
			const auto pArchive = m_sharedDesignCache->GetFileArchive(designName);
			EXPECT_NE(pArchive, nullptr) << designName;
			if (pArchive == nullptr) return bytes;

			const auto pStep = pArchive->GetStepDirectory(stepName);
			EXPECT_NE(pStep, nullptr) << stepName;
			if (pStep == nullptr) return bytes;

			const auto& layersByName = pStep->GetLayersByName();
			const auto layerIt = layersByName.find(layerName);
			EXPECT_NE(layerIt, layersByName.end()) << layerName;
			if (layerIt == layersByName.end()) return bytes;

			for (const auto& pRecord : layerIt->second->GetFeaturesFile().GetFeatureRecords())
			{
				if (pRecord == nullptr) continue;
				auto pMsg = pRecord->to_protobuf();
				EXPECT_NE(pMsg, nullptr) << layerName << " record";
				if (pMsg == nullptr) continue;
				bytes.push_back(SerializeDeterministic(*pMsg));
			}
			return bytes;
		}

		// Runs GetLayerFeaturesStream over the real channel and returns every
		// delivered record deterministically serialized, in order. The
		// caller-owned context lets tests read the server trailing metadata
		// (x-total-records completeness contract) after Finish().
		std::vector<std::string> StreamAllRecordBytes(const std::string& designName,
			const std::string& stepName, const std::string& layerName,
			grpc::ClientContext& ctx, grpc::Status* statusOut)
		{
			GetLayerFeaturesRequest req;
			req.set_design_name(designName);
			req.set_step_name(stepName);
			req.set_layer_name(layerName);

			auto reader = m_stub->GetLayerFeaturesStream(&ctx, req);
			std::vector<std::string> bytes;
			Odb::Lib::Protobuf::FeaturesFile::FeatureRecord record;
			while (reader->Read(&record))
			{
				bytes.push_back(SerializeDeterministic(record));
				record.Clear();
			}
			*statusOut = reader->Finish();
			return bytes;
		}

		// Reads the server's "x-total-records" trailing-metadata entry (the
		// completeness contract of both layer-feature streams). Call after
		// Finish(). Returns {found, count}. std::multimap, not a protobuf
		// Map — find() is the ordinary accessor here (#599 does not apply).
		static std::pair<bool, std::uint64_t> TrailerTotal(const grpc::ClientContext& ctx)
		{
			const auto& trailers = ctx.GetServerTrailingMetadata();
			const auto it = trailers.find("x-total-records");
			if (it == trailers.end())
			{
				return { false, 0 };
			}
			return { true, std::stoull(std::string(it->second.data(), it->second.size())) };
		}

		// Runs GetLayerFeaturesBatchStream over the real channel; returns the
		// received batches (in order) alongside the flattened record bytes.
		// The caller-owned context carries the server trailing metadata.
		struct BatchStreamResult
		{
			std::vector<FeatureRecordBatch> batches;
			std::vector<std::string> flattenedBytes;
		};

		BatchStreamResult StreamAllBatches(const std::string& designName,
			const std::string& stepName, const std::string& layerName,
			grpc::ClientContext& ctx, grpc::Status* statusOut)
		{
			GetLayerFeaturesRequest req;
			req.set_design_name(designName);
			req.set_step_name(stepName);
			req.set_layer_name(layerName);

			auto reader = m_stub->GetLayerFeaturesBatchStream(&ctx, req);
			BatchStreamResult result;
			FeatureRecordBatch batch;
			while (reader->Read(&batch))
			{
				for (const auto& record : batch.features())
				{
					result.flattenedBytes.push_back(SerializeDeterministic(record));
				}
				result.batches.push_back(batch);
				batch.Clear();
			}
			*statusOut = reader->Finish();
			return result;
		}

		// A cancelled stream must not take the server down: a fresh unary RPC
		// on the same channel must still answer SERVING.
		bool serverAnswersHealthCheck()
		{
			grpc::ClientContext ctx;
			HealthCheckRequest req;
			HealthCheckResponse resp;
			const auto status = m_stub->HealthCheck(&ctx, req, &resp);
			return status.ok() && resp.status() == HealthCheckResponse::SERVING;
		}

		std::shared_ptr<DesignCache> m_sharedDesignCache;
		std::shared_ptr<GrpcServiceConfig> m_config;
		std::unique_ptr<OdbDesignServiceImpl> m_service;
		std::unique_ptr<grpc::Server> m_server;
		std::unique_ptr<OdbDesignService::Stub> m_stub;
	};

	// ---- GetLayerFeaturesStream ----

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_KnownLayer_StreamedRecordsEqualDirectModelInOrder)
	{
		const auto direct = DirectRecordBytes(kDesign, kStep, kLayerWithFeatures);
		ASSERT_EQ(direct.size(), kLayer1RecordCount)
			<< "TEST_DATA corpus changed: update the pinned record count and batch arithmetic";

		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kLayerWithFeatures, ctx, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();

		ASSERT_EQ(streamed.size(), direct.size())
			<< "stream must deliver exactly as many records as the layer holds";
		for (std::size_t i = 0; i < direct.size(); ++i)
		{
			ASSERT_EQ(streamed[i], direct[i])
				<< "streamed record " << i << " must equal the direct model record (order + content)";
		}

		// Completeness contract: an OK stream carries x-total-records and it
		// equals exactly what was delivered.
		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		EXPECT_TRUE(trailerFound) << "an OK layer-feature stream must carry the x-total-records trailer";
		EXPECT_EQ(trailerTotal, direct.size())
			<< "trailer total must equal the delivered record count on a complete stream";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingDesign_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes("no_such_design_exists", kStep, kLayerWithFeatures, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty()) << "an error status must not carry feature records";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingStep_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kMissingName, kLayerWithFeatures, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingLayer_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kMissingName, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_EmptyLayer_EndsCleanlyWithNoRecords)
	{
		// comp_+_top exists in layersByName but holds zero records: the
		// handler must end the stream cleanly (OK, zero messages), not error
		// and not fabricate records — and the completeness trailer must
		// report 0.
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kEmptyLayer, ctx, &status);
		EXPECT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(streamed.empty());

		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		EXPECT_TRUE(trailerFound) << "an OK layer-feature stream must carry the x-total-records trailer";
		EXPECT_EQ(trailerTotal, 0ull) << "an empty layer's completeness total must be 0";
	}

	// Server contract under client cancellation (Option A, GetDesignStream
	// semantics): a cancel observed at the Write-failure site is not a server
	// error — the handler returns OK and lets the client-side CANCELLED
	// speak. The CLIENT must still observe a prompt end-of-stream with a
	// CANCELLED (or racing OK) trailer, the server must stay healthy, and the
	// truncation shape must be detectable: if the x-total-records trailer
	// made it back, it reports MORE than the delivered prefix.
	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_ClientCancelMidStream_StopsPromptlyServerStaysHealthy)
	{
		grpc::ClientContext ctx;
		GetLayerFeaturesRequest req;
		req.set_design_name(kDesign);
		req.set_step_name(kStep);
		req.set_layer_name(kLayerWithFeatures);

		auto reader = m_stub->GetLayerFeaturesStream(&ctx, req);

		std::uint64_t read = 0;
		Odb::Lib::Protobuf::FeaturesFile::FeatureRecord record;
		while (reader->Read(&record))
		{
			++read;
			record.Clear();
			if (read == 1000)
			{
				ctx.TryCancel();
				break;
			}
		}
		ASSERT_GE(read, 1000ull) << "the stream must have delivered records before the cancel";

		const auto drained = drainWithin(reader.get(), std::chrono::seconds(30));
		EXPECT_TRUE(drained.completed)
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";

		// Cancellation must actually truncate: the TOTAL delivered (prefix +
		// post-cancel drain, both counted) must fall short of the complete
		// stream. The counted drain is what makes this check real — a
		// count-free drain would silently swallow deliveries from a server
		// that ignored TryCancel, even when Finish() races to OK.
		const auto totalDelivered = read + drained.records;
		EXPECT_LT(totalDelivered, kLayer1RecordCount)
			<< "cancel must truncate: total delivered must fall short of the complete stream";

		// Truncation shape: the completeness trailer (set server-side before
		// the cancellation return) must exceed the delivered prefix — a
		// client that receives it can detect incompleteness and its size.
		// Trailer delivery to a client that itself cancelled is a gRPC race
		// (late trailers may be discarded; probed 0/10 deliveries on this
		// build); when it IS delivered, its total is pinned strictly greater
		// than what arrived.
		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		if (trailerFound)
		{
			EXPECT_GT(trailerTotal, totalDelivered)
				<< "a truncated stream's x-total-records must exceed the delivered prefix";
		}

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled layer-feature stream must leave the server serving";
	}

	// ---- GetLayerFeaturesBatchStream ----

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_DisabledFlag_ReturnsUnimplemented)
	{
		m_config->enable_batch_streaming = false;
		ASSERT_TRUE(StartServer()) << "server restart with disabled flag failed";

		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
		EXPECT_TRUE(result.batches.empty()) << "an error status must not carry batches";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingDesign_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches("no_such_design_exists", kStep, kLayerWithFeatures, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingStep_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kMissingName, kLayerWithFeatures, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingLayer_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kMissingName, ctx, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_ConfiguredBatchSize_FullBatchesExactSizePlusRemainder)
	{
		// 11868 records at batch size 250: exactly 47 full batches followed by
		// one 118-record remainder batch (pin verified against the fixture
		// corpus; the remainder exercises the final-partial-batch write).
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, ctx, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();

		ASSERT_EQ(result.batches.size(), kLayer1FullBatches + 1)
			<< "must be 47 full batches + 1 remainder batch";
		for (std::uint64_t i = 0; i < kLayer1FullBatches; ++i)
		{
			ASSERT_EQ(result.batches[i].features_size(), static_cast<int>(kBatchSize))
				<< "non-final batch " << i << " must be exactly the configured batch size";
		}
		ASSERT_EQ(result.batches.back().features_size(), static_cast<int>(kLayer1Remainder))
			<< "final batch must carry exactly the remaining records";

		// Completeness contract: the FINAL batch carries the whole-stream
		// total (not its own size), and it equals the trailer and the
		// received count.
		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		EXPECT_TRUE(trailerFound) << "an OK batch stream must carry the x-total-records trailer";
		EXPECT_TRUE(result.batches.back().has_total_records())
			<< "the final batch must carry total_records";
		EXPECT_EQ(result.batches.back().total_records(), kLayer1RecordCount)
			<< "final batch total_records must be the whole-stream total";
		EXPECT_EQ(trailerTotal, kLayer1RecordCount);
		EXPECT_EQ(result.flattenedBytes.size(), static_cast<std::size_t>(kLayer1RecordCount))
			<< "received count must equal the reported completeness total";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_FlattenedBatchesEqualUnbatchedStreamAndDirectModel)
	{
		const auto direct = DirectRecordBytes(kDesign, kStep, kLayerWithFeatures);
		ASSERT_EQ(direct.size(), kLayer1RecordCount);

		grpc::ClientContext streamCtx, batchCtx;
		grpc::Status streamStatus, batchStatus;
		auto unbatched = StreamAllRecordBytes(kDesign, kStep, kLayerWithFeatures, streamCtx, &streamStatus);
		ASSERT_TRUE(streamStatus.ok()) << streamStatus.error_message();
		const auto batched = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, batchCtx, &batchStatus);
		ASSERT_TRUE(batchStatus.ok()) << batchStatus.error_message();

		// Three-way equality: direct model == unbatched stream == flattened
		// batches, element-wise in order (deterministic serialization makes
		// byte equality prove content equality despite the records' map field).
		ASSERT_EQ(unbatched.size(), direct.size());
		ASSERT_EQ(batched.flattenedBytes.size(), direct.size());
		for (std::size_t i = 0; i < direct.size(); ++i)
		{
			ASSERT_EQ(unbatched[i], direct[i])
				<< "unbatched record " << i << " diverged from the direct model";
			ASSERT_EQ(batched.flattenedBytes[i], direct[i])
				<< "flattened batch record " << i << " diverged from the direct model";
		}

		// Both flavors report the same completeness total.
		const auto [streamTrailerFound, streamTrailerTotal] = TrailerTotal(streamCtx);
		const auto [batchTrailerFound, batchTrailerTotal] = TrailerTotal(batchCtx);
		EXPECT_TRUE(streamTrailerFound);
		EXPECT_TRUE(batchTrailerFound);
		EXPECT_EQ(streamTrailerTotal, direct.size());
		EXPECT_EQ(batchTrailerTotal, direct.size());
		EXPECT_TRUE(batched.batches.back().has_total_records());
		EXPECT_EQ(batched.batches.back().total_records(), direct.size());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_SmallLayer_EmitsSingleFinalBatchNoFullBatches)
	{
		// board-outline.doc holds 10 records (< batch size 250): the only
		// batch must be the final partial one — no full-size batches, no
		// padding — carrying the layer's total (10) as total_records.
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kSmallLayer, ctx, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_EQ(result.batches.size(), std::size_t(1));
		EXPECT_EQ(result.batches[0].features_size(), 10);

		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		EXPECT_TRUE(trailerFound);
		EXPECT_TRUE(result.batches[0].has_total_records());
		EXPECT_EQ(result.batches[0].total_records(), 10ull) << "single final batch must report the whole-stream total";
		EXPECT_EQ(trailerTotal, 10ull);
		EXPECT_EQ(result.flattenedBytes.size(), std::size_t(10));
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_EmptyLayer_EmitsNoBatches)
	{
		// Zero-record layer: the final-batch guard (currentBatchCount > 0)
		// must suppress the write — no spurious final empty batch — and the
		// completeness trailer must report 0.
		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kEmptyLayer, ctx, &status);
		EXPECT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(result.batches.empty())
			<< "an empty layer must end the stream without any (empty) batch";

		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		EXPECT_TRUE(trailerFound);
		EXPECT_EQ(trailerTotal, 0ull);
	}

	// Cancellation while the stream is mid-way through full batches: the
	// handler checks IsCancelled per record and before each batch write. The
	// client must see a prompt end-of-stream, everything delivered must be a
	// valid prefix of the layer's records, the server must stay healthy, and
	// the truncation shape must be detectable via the completeness trailer
	// (when its delivery wins the race against the client-side cancel).
	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_CancelDuringFullBatchPath_StopsPromptlyDeliveredPrefixValid)
	{
		const auto direct = DirectRecordBytes(kDesign, kStep, kLayerWithFeatures);
		ASSERT_EQ(direct.size(), kLayer1RecordCount);

		grpc::ClientContext ctx;
		GetLayerFeaturesRequest req;
		req.set_design_name(kDesign);
		req.set_step_name(kStep);
		req.set_layer_name(kLayerWithFeatures);

		auto reader = m_stub->GetLayerFeaturesBatchStream(&ctx, req);

		std::uint64_t batchesRead = 0;
		std::uint64_t recordsRead = 0;
		FeatureRecordBatch batch;
		while (reader->Read(&batch))
		{
			for (const auto& record : batch.features())
			{
				ASSERT_LT(recordsRead, direct.size())
					<< "server delivered more records than the layer holds";
				EXPECT_EQ(SerializeDeterministic(record), direct[recordsRead])
					<< "delivered record " << recordsRead << " must be the next record in order";
				++recordsRead;
			}
			++batchesRead;
			batch.Clear();
			if (batchesRead == 5)
			{
				ctx.TryCancel(); // mid full-batch production: thousands of records still pending
				break;
			}
		}
		ASSERT_GE(batchesRead, 5ull) << "the stream must have delivered full batches before the cancel";

		const auto drained = drainBatchesWithin(reader.get(), std::chrono::seconds(30));
		EXPECT_TRUE(drained.completed)
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";

		// Counted totals (prefix + post-cancel drain): the delivered set must
		// fall short of the complete stream. This is the check that catches a
		// server ignoring TryCancel — the drain counts what it consumes, so
		// even a Finish() racing to OK cannot hide a full delivery.
		const auto totalBatches = batchesRead + drained.batches;
		const auto totalRecords = recordsRead + drained.records;
		EXPECT_LT(totalRecords, kLayer1RecordCount)
			<< "cancel must truncate: delivered records must fall short of the complete stream";
		EXPECT_LT(totalBatches, kLayer1FullBatches + 1)
			<< "cancel must truncate: delivered batches must fall short of the complete 48";

		// Truncation shape: when the completeness trailer beats the cancel to
		// the client, it must exceed the delivered prefix — proving a client
		// CAN detect incompleteness and its size. Trailer delivery to a
		// client that itself cancelled is a gRPC race (late trailers may be
		// discarded; probed 0/10 deliveries on this build), so presence is
		// not asserted here; the success tests pin the trailer mechanism.
		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		if (trailerFound)
		{
			EXPECT_GT(trailerTotal, totalRecords)
				<< "a truncated stream's x-total-records must exceed the delivered prefix";
		}

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled batch stream must leave the server serving";
	}

	// Cancellation on the final-batch path: layer-5 at batch size 250 yields
	// 5 full batches + a 34-record remainder. The client cancels after the
	// LAST full batch, racing the handler's final-batch IsCancelled check.
	// Either the final batch was already written (client gets it; trailer may
	// win as OK) or the handler observes the cancel before writing it and
	// returns CANCELLED — both are accepted, but whatever arrived must be the
	// exact expected prefix of batches, and the completeness trailer (if it
	// reaches the cancelled client) must exceed the delivered prefix.
	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_CancelBeforeFinalBatch_DeliveredPrefixValidServerStaysHealthy)
	{
		const auto direct = DirectRecordBytes(kDesign, kStep, kMidLayer);
		const std::uint64_t total = direct.size();
		const std::uint64_t fullBatches = total / kBatchSize;              // 5 for layer-5 @250
		const std::uint64_t remainder = total % kBatchSize;                // 34 for layer-5 @250
		ASSERT_GT(remainder, 0ull) << "fixture pin: layer-5 must have a remainder batch";

		grpc::ClientContext ctx;
		GetLayerFeaturesRequest req;
		req.set_design_name(kDesign);
		req.set_step_name(kStep);
		req.set_layer_name(kMidLayer);

		auto reader = m_stub->GetLayerFeaturesBatchStream(&ctx, req);

		std::uint64_t batchesRead = 0;
		std::uint64_t recordsRead = 0;
		FeatureRecordBatch batch;
		while (reader->Read(&batch))
		{
			for (const auto& record : batch.features())
			{
				ASSERT_LT(recordsRead, total);
				EXPECT_EQ(SerializeDeterministic(record), direct[recordsRead])
					<< "delivered record " << recordsRead << " must be the next record in order";
				++recordsRead;
			}
			++batchesRead;
			batch.Clear();
			if (batchesRead == fullBatches)
			{
				ctx.TryCancel(); // only the final remainder batch is still outstanding
				break;
			}
		}
		ASSERT_GE(batchesRead, fullBatches)
			<< "the stream must have delivered all full batches before the cancel";

		const auto drained = drainBatchesWithin(reader.get(), std::chrono::seconds(30));
		EXPECT_TRUE(drained.completed)
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";

		// Counted totals (prefix + post-cancel drain): either all full batches
		// only (final suppressed by the cancel) or full + final remainder
		// (final write won the race). The drain counts post-cancel deliveries,
		// so both arms of this disjunction are live: a handler that suppressed
		// the final batch lands in the first arm, one that wrote it before
		// noticing the cancel lands in the second.
		const auto totalBatches = batchesRead + drained.batches;
		const auto totalRecords = recordsRead + drained.records;
		EXPECT_TRUE(totalBatches == fullBatches || totalBatches == fullBatches + 1)
			<< "delivered batch count must be either all-full-batches or full+final, got "
			<< totalBatches;
		if (totalBatches == fullBatches + 1)
		{
			EXPECT_EQ(totalRecords, total) << "final remainder batch delivered: record count must be complete";
		}

		// Truncation shape (only when the stream actually truncated): the
		// completeness trailer must exceed the delivered prefix. (If the
		// final write won the race, the stream is complete — totalRecords ==
		// total — and the trailer equals it.)
		const auto [trailerFound, trailerTotal] = TrailerTotal(ctx);
		if (trailerFound && totalRecords < total)
		{
			EXPECT_GT(trailerTotal, totalRecords)
				<< "a truncated stream's x-total-records must exceed the delivered prefix";
		}

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled batch stream must leave the server serving";
	}

	// ---- REAL write-failure truncation, no client cancellation ----
	//
	// A server configured with a send-message limit below one feature record
	// makes ServerWriter::Write fail while the client stays connected and
	// has NOT cancelled — the only loopback shape that drives the
	// write-failure branch of the fail-loud contract from a well-behaved
	// client. The wire outcome is pinned here: the truncated stream NEVER
	// ends OK (the transport terminates the call with RESOURCE_EXHAUSTED —
	// in this shape the transport's status supersedes the handler's INTERNAL
	// return and its trailer, so the wire shows RESOURCE_EXHAUSTED with no
	// trailer; server-side the handler still takes the non-cancelled
	// INTERNAL branch, mirrors GetDesignStream, and logs the truncation).
	// The server must stay healthy afterwards.

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_ServerSendLimitWriteFailure_NeverEndsOkServerStaysHealthy)
	{
		ASSERT_TRUE(StartServer(/*maxSendBytes=*/32)) << "tiny-send-limit server start failed";

		grpc::ClientContext ctx;
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kLayerWithFeatures, ctx, &status);

		EXPECT_NE(status.error_code(), grpc::StatusCode::OK)
			<< "a truncated layer-feature stream must NEVER end OK (fail-loud contract)";
		EXPECT_EQ(status.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED)
			<< "the transport's send-limit failure must surface to the client";
		EXPECT_LT(streamed.size(), static_cast<std::size_t>(kLayer1RecordCount))
			<< "a send-limit truncated stream must not deliver the full record set";

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a write-failed layer-feature stream must leave the server serving";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_ServerSendLimitWriteFailure_NeverEndsOkServerStaysHealthy)
	{
		ASSERT_TRUE(StartServer(/*maxSendBytes=*/32)) << "tiny-send-limit server start failed";

		grpc::ClientContext ctx;
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, ctx, &status);

		EXPECT_NE(status.error_code(), grpc::StatusCode::OK)
			<< "a truncated batch stream must NEVER end OK (fail-loud contract)";
		EXPECT_EQ(status.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED)
			<< "the transport's send-limit failure must surface to the client";
		EXPECT_TRUE(result.batches.empty())
			<< "the first batch (250 records) exceeds the tiny send limit; no batch may be delivered as OK";

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a write-failed batch stream must leave the server serving";
	}

	// -------------------------------------------------------------------------
	// PINNED CONTRACT (Option A "fail loud", decided on PR #659):
	//
	// 1. Fail loud on writer failure. On ServerWriter::Write failure both
	//    layer-feature handlers check ServerContext::IsCancelled():
	//      - cancelled  => return OK (GetDesignStream semantics: cancellation
	//        is not a server error; the client sees CANCELLED from its own
	//        cancel, or a racing OK trailer),
	//      - otherwise  => grpc::StatusCode::INTERNAL with a sent-of-total
	//        count message ("layer-feature stream write failed after N of T
	//        records"). No path hands a still-connected client a clean OK
	//        over a truncated stream.
	//    These tests pin the cancellation branch (the only failure driver
	//    exercisable over loopback): prompt end-of-stream, prefix-valid
	//    delivery, healthy server afterwards.
	//
	//    WHY THE INTERNAL BRANCH IS NOT TRANSPORT-TESTABLE HERE (documented,
	//    not faked): ServerWriter is final and cannot be mocked
	//    (GrpcBatchStreamTests note), and the handlers take the generated
	//    signature, so there is no writer seam short of refactoring
	//    production beyond this decision's scope. Over a loopback channel
	//    the sync-API Write fails only when the call is dead — client
	//    TryCancel or deadline expiry (both set IsCancelled(), landing in
	//    the cancellation branch) — or a connection break without a
	//    propagated cancel, which a well-behaved client harness cannot
	//    synthesize.
	//
	// 2. Completeness verification (defense-in-depth). Every exit after
	//    validation carries the "x-total-records" trailing-metadata entry =
	//    the number of records a COMPLETE stream delivers;
	//    GetLayerFeaturesBatchStream's FINAL batch additionally carries
	//    total_records (additive proto field, wire-compatible). The success
	//    tests pin: received == trailer == final-batch total_records (and
	//    thereby prove the trailer IS delivered to connected clients on OK
	//    streams).
	//
	//    Empirical wire facts (probed 2026-11-21, this gRPC build):
	//    - A client that cancels mid-stream NEVER receives the trailer
	//      (0/10 deliveries, both RPCs) — gRPC discards late trailers for a
	//      cancelled call. The cancellation tests therefore pin the trailer
	//      conditionally: whenever it IS delivered, it must exceed the
	//      delivered prefix.
	//    - In the send-limit write-failure shape the transport terminates
	//      the call with RESOURCE_EXHAUSTED before the handler's status is
	//      sent, so the wire shows the transport's status and NO trailer;
	//      the handler's INTERNAL + trailer are superseded. The
	//      never-OK-over-truncation guarantee is what remains wire-visible,
	//      and it is pinned unconditionally in the send-limit tests.
	// -------------------------------------------------------------------------
}
