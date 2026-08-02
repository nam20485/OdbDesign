#include <cctype>
#include <cmath>
#include <iostream>
#include <set>
#include <string>

#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "Fixtures/FileArchiveLoadFixture.h"
#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"

using namespace Odb::Test::Fixtures;

namespace
{
	constexpr const char* SAMPLE_DESIGN = "sample_design";
}

class GetStandardFontsFixture : public FileArchiveLoadFixture
{
protected:
	void SetUp() override
	{
		FileArchiveLoadFixture::SetUp();
		// Convert unique_ptr to shared_ptr for the service constructor.
		// Store shared_ptr first so ownership is captured before any potential exceptions.
		m_sharedDesignCache = std::shared_ptr<Odb::Lib::App::DesignCache>(m_pDesignCache.release());
		m_service = std::make_unique<OdbDesignServer::Services::OdbDesignServiceImpl>(m_sharedDesignCache);
	}

	std::shared_ptr<Odb::Lib::App::DesignCache> m_sharedDesignCache;
	std::unique_ptr<OdbDesignServer::Services::OdbDesignServiceImpl> m_service;
};

TEST_F(GetStandardFontsFixture, ReturnsFontDataForSampleDesign)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetStandardFontsRequest req;
	Odb::Lib::Protobuf::StandardFontsFile resp;

	req.set_design_name(SAMPLE_DESIGN);

	auto status = m_service->GetStandardFonts(&ctx, &req, &resp);
	ASSERT_TRUE(status.ok()) << status.error_message();

	EXPECT_GT(resp.m_characterblocks_size(), 0);
	EXPECT_GT(resp.xsize(), 0.0);
	EXPECT_GT(resp.ysize(), 0.0);

	// Surface the font size for the client handoff doc / memory estimation.
	int lineRecordCount = 0;
	for (const auto& block : resp.m_characterblocks())
	{
		lineRecordCount += block.m_linerecords_size();
	}
	std::cout << "[GetStandardFonts] sample_design character_blocks=" << resp.m_characterblocks_size()
		<< " line_records=" << lineRecordCount
		<< " xSize=" << resp.xsize()
		<< " ySize=" << resp.ysize()
		<< " offset=" << resp.offset() << std::endl;
}

TEST_F(GetStandardFontsFixture, ReturnsNotFoundForMissingDesign)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetStandardFontsRequest req;
	Odb::Lib::Protobuf::StandardFontsFile resp;

	req.set_design_name("does_not_exist");

	auto status = m_service->GetStandardFonts(&ctx, &req, &resp);
	EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(GetStandardFontsFixture, ContainsExpectedCharacterSet)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetStandardFontsRequest req;
	Odb::Lib::Protobuf::StandardFontsFile resp;

	req.set_design_name(SAMPLE_DESIGN);

	auto status = m_service->GetStandardFonts(&ctx, &req, &resp);
	ASSERT_TRUE(status.ok()) << status.error_message();

	std::set<char> characters;
	for (const auto& block : resp.m_characterblocks())
	{
		// Each glyph key must be a single printable character.
		ASSERT_EQ(block.character().size(), 1u) << "unexpected character key: \"" << block.character() << "\"";
		const char c = block.character()[0];
		EXPECT_TRUE(std::isprint(static_cast<unsigned char>(c)) != 0) << "non-printable character key: " << static_cast<int>(c);
		characters.insert(c);
	}

	// Alphanumerics are guaranteed by the ODB++ standard font.
	for (char c = 'A'; c <= 'Z'; ++c)
	{
		EXPECT_TRUE(characters.count(c) > 0) << "missing glyph for: " << c;
	}
	for (char c = '0'; c <= '9'; ++c)
	{
		EXPECT_TRUE(characters.count(c) > 0) << "missing glyph for: " << c;
	}

	// Common silkscreen punctuation used by refdes / labels.
	for (const char c : { '+', '-', '.', '_' })
	{
		EXPECT_TRUE(characters.count(c) > 0) << "missing glyph for: " << c;
	}
}

TEST_F(GetStandardFontsFixture, LineRecordsArePopulated)
{
	grpc::ServerContext ctx;
	Odb::Grpc::GetStandardFontsRequest req;
	Odb::Lib::Protobuf::StandardFontsFile resp;

	req.set_design_name(SAMPLE_DESIGN);

	auto status = m_service->GetStandardFonts(&ctx, &req, &resp);
	ASSERT_TRUE(status.ok()) << status.error_message();

	int lineRecordCount = 0;
	for (const auto& block : resp.m_characterblocks())
	{
		for (const auto& lr : block.m_linerecords())
		{
			++lineRecordCount;
			EXPECT_TRUE(std::isfinite(lr.xstart()));
			EXPECT_TRUE(std::isfinite(lr.ystart()));
			EXPECT_TRUE(std::isfinite(lr.xend()));
			EXPECT_TRUE(std::isfinite(lr.yend()));
			EXPECT_GT(lr.width(), 0.0);
		}
	}

	EXPECT_GT(lineRecordCount, 0);
}
