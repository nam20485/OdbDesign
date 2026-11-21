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

		// Bounded drain loop for cancellation tests: after TryCancel the
		// reader must hit end-of-stream promptly. An unbounded Read loop
		// would hang (not fail) if cancellation stopped working, so the
		// drain enforces a wall-clock budget.
		bool drainWithin(grpc::ClientReader<Odb::Lib::Protobuf::FeaturesFile::FeatureRecord>* reader,
			std::chrono::seconds budget)
		{
			Odb::Lib::Protobuf::FeaturesFile::FeatureRecord record;
			const auto deadline = std::chrono::steady_clock::now() + budget;
			while (reader->Read(&record))
			{
				record.Clear();
				if (std::chrono::steady_clock::now() > deadline)
				{
					return false;
				}
			}
			return true;
		}

		bool drainBatchesWithin(grpc::ClientReader<FeatureRecordBatch>* reader,
			std::chrono::seconds budget)
		{
			FeatureRecordBatch batch;
			const auto deadline = std::chrono::steady_clock::now() + budget;
			while (reader->Read(&batch))
			{
				batch.Clear();
				if (std::chrono::steady_clock::now() > deadline)
				{
					return false;
				}
			}
			return true;
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
		bool StartServer()
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
		// delivered record deterministically serialized, in order.
		std::vector<std::string> StreamAllRecordBytes(const std::string& designName,
			const std::string& stepName, const std::string& layerName, grpc::Status* statusOut)
		{
			grpc::ClientContext ctx;
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

		// Runs GetLayerFeaturesBatchStream over the real channel; returns the
		// received batches (in order) alongside the flattened record bytes.
		struct BatchStreamResult
		{
			std::vector<FeatureRecordBatch> batches;
			std::vector<std::string> flattenedBytes;
		};

		BatchStreamResult StreamAllBatches(const std::string& designName,
			const std::string& stepName, const std::string& layerName, grpc::Status* statusOut)
		{
			grpc::ClientContext ctx;
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

		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kLayerWithFeatures, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();

		ASSERT_EQ(streamed.size(), direct.size())
			<< "stream must deliver exactly as many records as the layer holds";
		for (std::size_t i = 0; i < direct.size(); ++i)
		{
			ASSERT_EQ(streamed[i], direct[i])
				<< "streamed record " << i << " must equal the direct model record (order + content)";
		}
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingDesign_ReturnsNotFound)
	{
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes("no_such_design_exists", kStep, kLayerWithFeatures, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty()) << "an error status must not carry feature records";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingStep_ReturnsNotFound)
	{
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kMissingName, kLayerWithFeatures, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_MissingLayer_ReturnsNotFound)
	{
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kMissingName, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(streamed.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesStream_EmptyLayer_EndsCleanlyWithNoRecords)
	{
		// comp_+_top exists in layersByName but holds zero records: the
		// handler must end the stream cleanly (OK, zero messages), not error
		// and not fabricate records.
		grpc::Status status;
		const auto streamed = StreamAllRecordBytes(kDesign, kStep, kEmptyLayer, &status);
		EXPECT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(streamed.empty());
	}

	// Current server contract under client cancellation: the unbatched path
	// has no explicit IsCancelled check — the loop breaks when writer->Write
	// fails after the cancel is observed, and the handler returns OK (see the
	// writer-failure divergence note at the bottom of this file). The CLIENT
	// must still observe a prompt end-of-stream with a CANCELLED (or racing
	// OK) trailer, and the server must stay healthy afterwards.
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

		EXPECT_TRUE(drainWithin(reader.get(), std::chrono::seconds(30)))
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled layer-feature stream must leave the server serving";
	}

	// ---- GetLayerFeaturesBatchStream ----

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_DisabledFlag_ReturnsUnimplemented)
	{
		m_config->enable_batch_streaming = false;
		ASSERT_TRUE(StartServer()) << "server restart with disabled flag failed";

		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
		EXPECT_TRUE(result.batches.empty()) << "an error status must not carry batches";
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingDesign_ReturnsNotFound)
	{
		grpc::Status status;
		const auto result = StreamAllBatches("no_such_design_exists", kStep, kLayerWithFeatures, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingStep_ReturnsNotFound)
	{
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kMissingName, kLayerWithFeatures, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_MissingLayer_ReturnsNotFound)
	{
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kMissingName, &status);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
		EXPECT_TRUE(result.batches.empty());
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_ConfiguredBatchSize_FullBatchesExactSizePlusRemainder)
	{
		// 11868 records at batch size 250: exactly 47 full batches followed by
		// one 118-record remainder batch (pin verified against the fixture
		// corpus; the remainder exercises the final-partial-batch write).
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, &status);
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
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_FlattenedBatchesEqualUnbatchedStreamAndDirectModel)
	{
		const auto direct = DirectRecordBytes(kDesign, kStep, kLayerWithFeatures);
		ASSERT_EQ(direct.size(), kLayer1RecordCount);

		grpc::Status streamStatus, batchStatus;
		auto unbatched = StreamAllRecordBytes(kDesign, kStep, kLayerWithFeatures, &streamStatus);
		ASSERT_TRUE(streamStatus.ok()) << streamStatus.error_message();
		const auto batched = StreamAllBatches(kDesign, kStep, kLayerWithFeatures, &batchStatus);
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
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_SmallLayer_EmitsSingleFinalBatchNoFullBatches)
	{
		// board-outline.doc holds 10 records (< batch size 250): the only
		// batch must be the final partial one — no full-size batches, no
		// padding.
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kSmallLayer, &status);
		ASSERT_TRUE(status.ok()) << status.error_message();
		ASSERT_EQ(result.batches.size(), std::size_t(1));
		EXPECT_EQ(result.batches[0].features_size(), 10);
	}

	TEST_F(LayerFeatureStreamTest, GetLayerFeaturesBatchStream_EmptyLayer_EmitsNoBatches)
	{
		// Zero-record layer: the final-batch guard (currentBatchCount > 0)
		// must suppress the write — no spurious final empty batch.
		grpc::Status status;
		const auto result = StreamAllBatches(kDesign, kStep, kEmptyLayer, &status);
		EXPECT_TRUE(status.ok()) << status.error_message();
		EXPECT_TRUE(result.batches.empty())
			<< "an empty layer must end the stream without any (empty) batch";
	}

	// Cancellation while the stream is mid-way through full batches: the
	// handler checks IsCancelled per record and before each batch write and
	// returns CANCELLED when it observes it. The client must see a prompt
	// end-of-stream, everything delivered must be a valid prefix of the
	// layer's records, and the server must stay healthy.
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

		EXPECT_TRUE(drainBatchesWithin(reader.get(), std::chrono::seconds(30)))
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";
		EXPECT_LT(batchesRead, kLayer1FullBatches)
			<< "a cancel after 5 batches must not deliver the complete 47-batch stream";

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled batch stream must leave the server serving";
	}

	// Cancellation on the final-batch path: layer-5 at batch size 250 yields
	// 5 full batches + a 34-record remainder. The client cancels after the
	// LAST full batch, racing the handler's final-batch IsCancelled check.
	// Either the final batch was already written (client gets it; trailer may
	// win as OK) or the handler observes the cancel before writing it and
	// returns CANCELLED — both are accepted, but whatever arrived must be the
	// exact expected prefix of batches.
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

		EXPECT_TRUE(drainBatchesWithin(reader.get(), std::chrono::seconds(30)))
			<< "after TryCancel, end-of-stream must arrive promptly, not hang";
		const auto status = reader->Finish();
		EXPECT_TRUE(status.error_code() == grpc::StatusCode::CANCELLED || status.ok())
			<< "cancelled stream: expected CANCELLED (or a racing OK trailer), got "
			<< status.error_code() << " (" << status.error_message() << ")";

		// Whatever arrived is the exact expected prefix: either all full
		// batches only (final suppressed by the cancel) or full + final
		// remainder (final write won the race). Anything else is corruption.
		EXPECT_TRUE(batchesRead == fullBatches || batchesRead == fullBatches + 1)
			<< "delivered batch count must be either all-full-batches or full+final, got "
			<< batchesRead;
		if (batchesRead == fullBatches + 1)
		{
			EXPECT_EQ(recordsRead, total) << "final remainder batch delivered: record count must be complete";
		}

		EXPECT_TRUE(serverAnswersHealthCheck())
			<< "a cancelled batch stream must leave the server serving";
	}

	// -------------------------------------------------------------------------
	// FINDING (contract divergence, deliberate decision requested — not hacked
	// around here): on ServerWriter::Write failure both layer-feature paths
	// treat the stream as cleanly finished and return grpc::Status::OK —
	// GetLayerFeaturesStream breaks the loop and returns OK
	// (OdbDesignServiceImpl.cpp ~line 599-605), and
	// GetLayerFeaturesBatchStream returns OK at both the full-batch (~line
	// 712-718) and final-batch (~line 734-740) write sites. GetDesignStream
	// deliberately chose the opposite contract: a write failure with no client
	// cancel returns INTERNAL ("write failed, truncated") because the chunk
	// count is the only completion signal its clients have. For the layer
	// feature streams a mid-stream writer failure is therefore
	// indistinguishable from a clean end-of-stream to any client that only
	// checks Finish() — it must rely on having read the layer's full record
	// count. These tests pin the CURRENT OK-on-writer-failure behavior only in
	//sofar as cancellation (the one failure driver exercisable over loopback)
	// still yields a prompt client-side CANCELLED trailer and a healthy
	// server; a broken-connection-without-cancel simulation is not reachable
	// through the sync API on a loopback channel. A deliberate contract
	// decision (mirror GetDesignStream's INTERNAL, document OK as final, or
	// add an explicit IsCancelled check to the unbatched path) is a product
	// call and is reported to the release-readiness aggregator.
	// -------------------------------------------------------------------------
}
