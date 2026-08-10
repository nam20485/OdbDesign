#include <cctype>
#include <cmath>
#include <filesystem>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <system_error>

#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "ArchiveExtractor.h"
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

TEST_F(GetStandardFontsFixture, ReturnsInternalForUnparseableDesign)
{
	// Build a malformed ODB++ archive: it has the required top-level dirs so
	// findRootDir resolves a root, but they are empty, so ParseDesignDirectory
	// throws (missing misc/info etc.). This forces the server's catch block,
	// which maps to gRPC INTERNAL.
	const auto base = std::filesystem::temp_directory_path() / "odb_badfonts_cache";
	std::error_code ec;
	std::filesystem::create_directories(base, ec);
	ASSERT_FALSE(ec) << "create_directories(base) failed: " << ec.message();

	std::random_device rd;
	auto cacheDir = base / (std::to_string(rd()) + "_" + std::to_string(rd()));
	auto srcRoot = cacheDir / "src" / "bad_design";
	for (const auto* sub : { "fonts", "misc", "matrix", "steps" })
	{
		std::filesystem::create_directories(srcRoot / sub, ec);
		ASSERT_FALSE(ec) << "create_directories failed: " << ec.message();
	}

	std::string createdArchivePath;
	ASSERT_TRUE(Utils::ArchiveExtractor::CompressDir(
		srcRoot.string(), cacheDir.string(), "bad_design", createdArchivePath));
	ASSERT_FALSE(createdArchivePath.empty());

	auto badCache = std::make_shared<Odb::Lib::App::DesignCache>(cacheDir.string());
	auto service = std::make_unique<OdbDesignServer::Services::OdbDesignServiceImpl>(badCache);

	grpc::ServerContext ctx;
	Odb::Grpc::GetStandardFontsRequest req;
	Odb::Lib::Protobuf::StandardFontsFile resp;
	req.set_design_name("bad_design");

	auto status = service->GetStandardFonts(&ctx, &req, &resp);
	EXPECT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);

	std::filesystem::remove_all(cacheDir, ec);
}
