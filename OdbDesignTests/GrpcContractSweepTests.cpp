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
#include <string>
#include <thread>

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
#include <standardfontsfile.pb.h>
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
		// Deterministic serialization (map fields in sorted key order) — the
		// DesignStreamTests rationale: byte equality between two independent
		// serializations proves content equality only under deterministic mode.
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

		// Coverage-instrumented builds parse the multi-MB sample design
		// 5-10x slower; the 5-minute budget matches RequestLoadDesignTests'
		// kLoadCompleteTimeout (see its timing notes).
		constexpr auto kLoadCompleteTimeout = std::chrono::minutes(5);
	}

	// =========================================================================
	// Real-transport contract sweep for the RPCs whose behavior is otherwise
	// only exercised via DIRECT service-method invocation (no channel):
	// RequestLoadDesign (RequestLoadDesignTests), GetLayerSymbols
	// (GetLayerSymbolsTests), GetStandardFonts (GetStandardFontsTests).
	//
	// Coverage matrix note (release-readiness U4):
	// - GetDesign (both request flavours, incl. normalized lists) is already
	//   covered over a REAL channel by DesignStreamTests
	//   (GetDesignStream_ReassemblesToMonolithicBytes_{Default,Normalized}Flavor
	//   run the monolithic GetDesign through the generated stub) — not
	//   duplicated here.
	// - There is NO "GetDesigns" RPC in service.proto; the designs listing
	//   endpoint exists only on the REST surface (GET /designs), which is out
	//   of scope for the gRPC contract.
	// =========================================================================
	class ContractSweepTest : public FileArchiveLoadFixture
	{
	protected:
		static constexpr const char* kDesign = "sample_design";
		static constexpr const char* kStep = "step";
		static constexpr const char* kLayer = "layer-1";

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
			m_service = std::make_unique<OdbDesignServiceImpl>(m_sharedDesignCache, m_config);

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

		static std::shared_ptr<grpc::Channel> MakeChannel(int port)
		{
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

	// ---- RequestLoadDesign ----

	TEST_F(ContractSweepTest, RequestLoadDesign_UnknownDesign_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		RequestLoadDesignRequest req;
		RequestLoadDesignResponse resp;
		req.set_design_name("no_such_design_exists");

		const auto status = m_stub->RequestLoadDesign(&ctx, req, &resp);
		ASSERT_TRUE(status.ok()) << status.error_message();
		EXPECT_EQ(resp.design_name(), "no_such_design_exists") << "the response must echo the requested name";
		EXPECT_EQ(resp.status(), LOAD_NOT_FOUND);
	}

	TEST_F(ContractSweepTest, RequestLoadDesign_ColdDesign_ReturnsAcceptedThenAlreadyLoaded)
	{
		// Cold cache: first RPC must kick a real background load (ACCEPTED),
		// the parse must actually complete, and a second RPC must then report
		// the design warm (ALREADY_LOADED).
		grpc::ClientContext ctx1;
		RequestLoadDesignRequest req1;
		RequestLoadDesignResponse resp1;
		req1.set_design_name(kDesign);
		const auto status1 = m_stub->RequestLoadDesign(&ctx1, req1, &resp1);
		ASSERT_TRUE(status1.ok()) << status1.error_message();
		EXPECT_EQ(resp1.design_name(), kDesign);
		EXPECT_EQ(resp1.status(), LOAD_ACCEPTED);

		// The kick must have REALLY started loading (a real outcome, not just
		// an enum): wait for the cache to reach Loaded through the background
		// thread the RPC started.
		ASSERT_TRUE(waitForCondition([this]()
		{
			return m_sharedDesignCache->GetLoadState(kDesign) == DesignCache::LoadState::Loaded;
		}, kLoadCompleteTimeout)) << "background load kicked by RequestLoadDesign never completed";

		grpc::ClientContext ctx2;
		RequestLoadDesignRequest req2;
		RequestLoadDesignResponse resp2;
		req2.set_design_name(kDesign);
		const auto status2 = m_stub->RequestLoadDesign(&ctx2, req2, &resp2);
		ASSERT_TRUE(status2.ok()) << status2.error_message();
		EXPECT_EQ(resp2.status(), LOAD_ALREADY_LOADED);
	}

	// ---- GetLayerSymbols ----

	TEST_F(ContractSweepTest, GetLayerSymbols_KnownLayer_MatchesDirectModelContract)
	{
		// Expected size computed independently from the parsed model, the
		// same way GetLayerSymbolsTests derives it: one entry per possible
		// sym_num index (max referenced + 1), gaps filled/padded.
		const auto pArchive = m_sharedDesignCache->GetFileArchive(kDesign);
		ASSERT_NE(pArchive, nullptr);
		const auto pStep = pArchive->GetStepDirectory(kStep);
		ASSERT_NE(pStep, nullptr);
		const auto& layersByName = pStep->GetLayersByName(); // std::map: find() is the ordinary accessor
		const auto layerIt = layersByName.find(kLayer);
		ASSERT_NE(layerIt, layersByName.end());
		const auto& featuresFile = layerIt->second->GetFeaturesFile();

		int maxSymRef = -1;
		for (const auto& fr : featuresFile.GetFeatureRecords())
		{
			if (!fr) continue;
			maxSymRef = std::max(maxSymRef, fr->sym_num);
			maxSymRef = std::max(maxSymRef, fr->apt_def_symbol_num);
		}
		auto symbols = featuresFile.GetSymbolNames();
		if (symbols.empty())
		{
			for (const auto& kv : featuresFile.GetSymbolNamesByName())
			{
				symbols.push_back(kv.second);
			}
		}
		int maxIndex = -1;
		for (const auto& sym : symbols)
		{
			if (!sym) continue;
			maxIndex = std::max(maxIndex, sym->GetIndex());
		}
		const int expectedSize = std::max(maxSymRef, maxIndex) + 1;
		ASSERT_GT(expectedSize, 0) << "fixture pin: layer must reference symbols";

		grpc::ClientContext ctx;
		GetLayerSymbolsRequest req;
		GetLayerSymbolsResponse resp;
		req.set_design_name(kDesign);
		req.set_step_name(kStep);
		req.set_layer_name(kLayer);

		const auto status = m_stub->GetLayerSymbols(&ctx, req, &resp);
		ASSERT_TRUE(status.ok()) << status.error_message();

		// The indexing contract from service.proto: sym_num_base must be 0 so
		// FeatureRecord.sym_num directly indexes symbol_names.
		EXPECT_EQ(resp.sym_num_base(), 0);
		EXPECT_EQ(resp.units(), "inch");
		EXPECT_DOUBLE_EQ(resp.units_to_mm(), 25.4);
		ASSERT_EQ(resp.symbol_names_size(), expectedSize);

		// Every feature record's sym_num must index validly into the
		// delivered table (the cross-RPC contract the two RPCs exist for).
		// Negative sym_num means "no symbol referenced" (the sentinel the
		// server's own index validation also skips), so only non-negative
		// values participate in the indexing contract.
		for (const auto& fr : featuresFile.GetFeatureRecords())
		{
			if (!fr || fr->sym_num < 0) continue;
			ASSERT_LT(fr->sym_num, resp.symbol_names_size())
				<< "record sym_num " << fr->sym_num << " indexes outside the delivered symbol table";
		}
	}

	TEST_F(ContractSweepTest, GetLayerSymbols_MissingDesign_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		GetLayerSymbolsRequest req;
		GetLayerSymbolsResponse resp;
		req.set_design_name("no_such_design_exists");
		req.set_step_name(kStep);
		req.set_layer_name(kLayer);

		const auto status = m_stub->GetLayerSymbols(&ctx, req, &resp);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
	}

	// ---- GetStandardFonts ----

	TEST_F(ContractSweepTest, GetStandardFonts_KnownDesign_MatchesDirectModelBytes)
	{
		// Reference: direct serialization of the design's standard fonts file
		// from the same parsed model the handler reads.
		const auto pArchive = m_sharedDesignCache->GetFileArchive(kDesign);
		ASSERT_NE(pArchive, nullptr);
		auto pDirect = pArchive->GetStandardFontsFile().to_protobuf();
		ASSERT_NE(pDirect, nullptr);
		const auto directBytes = SerializeDeterministic(*pDirect);
		ASSERT_FALSE(directBytes.empty()) << "fixture pin: sample_design must have standard fonts data";

		grpc::ClientContext ctx;
		GetStandardFontsRequest req;
		Odb::Lib::Protobuf::StandardFontsFile resp;
		req.set_design_name(kDesign);

		const auto status = m_stub->GetStandardFonts(&ctx, req, &resp);
		ASSERT_TRUE(status.ok()) << status.error_message();

		EXPECT_EQ(SerializeDeterministic(resp), directBytes)
			<< "wire response must equal the direct model serialization (content parity)";
	}

	TEST_F(ContractSweepTest, GetStandardFonts_MissingDesign_ReturnsNotFound)
	{
		grpc::ClientContext ctx;
		GetStandardFontsRequest req;
		Odb::Lib::Protobuf::StandardFontsFile resp;
		req.set_design_name("no_such_design_exists");

		const auto status = m_stub->GetStandardFonts(&ctx, req, &resp);
		EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
	}
}
