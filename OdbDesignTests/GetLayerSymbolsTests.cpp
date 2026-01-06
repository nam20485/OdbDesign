#include <algorithm>
#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"

namespace OdbDesignServer::Services
{
	class OdbDesignServiceImpl;
}

using namespace Odb::Test::Fixtures;

class GetLayerSymbolsFixture : public FileArchiveLoadFixture
{
protected:
	void SetUp() override
	{
		FileArchiveLoadFixture::SetUp();
		// Convert unique_ptr to shared_ptr for the service constructor
		// Store shared_ptr first to ensure ownership is captured before any potential exceptions
		m_sharedDesignCache = std::shared_ptr<Odb::Lib::App::DesignCache>(m_pDesignCache.release());
		m_service = std::make_unique<OdbDesignServer::Services::OdbDesignServiceImpl>(m_sharedDesignCache);
	}

	std::shared_ptr<Odb::Lib::App::DesignCache> m_sharedDesignCache;
	std::unique_ptr<OdbDesignServer::Services::OdbDesignServiceImpl> m_service;
};

TEST_F(GetLayerSymbolsFixture, ReturnsSymbolsWithNormalizedUnits)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetLayerSymbolsRequest req;
	Odb::Grpc::GetLayerSymbolsResponse resp;

	req.set_design_name("designodb_rigidflex");
	req.set_step_name("cellular_flip-phone");
	req.set_layer_name("signal_2");

	auto status = m_service->GetLayerSymbols(&ctx, &req, &resp);
	EXPECT_TRUE(status.ok()) << status.error_message();

	EXPECT_EQ(resp.units(), "inch");
	EXPECT_DOUBLE_EQ(resp.units_to_mm(), 25.4);
	EXPECT_EQ(resp.sym_num_base(), 0);
	EXPECT_GT(resp.symbol_names_size(), 0);

	auto pFileArchive = m_sharedDesignCache->GetFileArchive("designodb_rigidflex");
	ASSERT_NE(pFileArchive, nullptr);
	const auto pStep = pFileArchive->GetStepDirectory("cellular_flip-phone");
	ASSERT_NE(pStep, nullptr);
	const auto& featuresFile = pStep->GetLayersByName().at("signal_2")->GetFeaturesFile();

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
	EXPECT_EQ(resp.symbol_names_size(), expectedSize);
}

TEST_F(GetLayerSymbolsFixture, ReturnsNotFoundForMissingDesign)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetLayerSymbolsRequest req;
	Odb::Grpc::GetLayerSymbolsResponse resp;

	req.set_design_name("does_not_exist");
	req.set_step_name("any");
	req.set_layer_name("any");

	auto status = m_service->GetLayerSymbols(&ctx, &req, &resp);
	EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(GetLayerSymbolsFixture, ReturnsNotFoundForMissingStep)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetLayerSymbolsRequest req;
	Odb::Grpc::GetLayerSymbolsResponse resp;

	req.set_design_name("designodb_rigidflex");
	req.set_step_name("does_not_exist");
	req.set_layer_name("signal_2");

	auto status = m_service->GetLayerSymbols(&ctx, &req, &resp);
	EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(GetLayerSymbolsFixture, ReturnsNotFoundForMissingLayer)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetLayerSymbolsRequest req;
	Odb::Grpc::GetLayerSymbolsResponse resp;

	req.set_design_name("designodb_rigidflex");
	req.set_step_name("cellular_flip-phone");
	req.set_layer_name("does_not_exist");

	auto status = m_service->GetLayerSymbols(&ctx, &req, &resp);
	EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

