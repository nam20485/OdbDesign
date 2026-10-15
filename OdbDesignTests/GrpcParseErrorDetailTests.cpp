#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <cstdlib>

#include <gtest/gtest.h>
#include <grpcpp/server_context.h>

#include "OdbDesignServer/Services/OdbDesignServiceImpl.h"

namespace
{
	// Removes the temp dir on destruction, so cleanup still runs when an ASSERT_*
	// fails partway through a test (ASSERT returns from the test body). Mirrors
	// the guard used by the DesignCacheLoadError and GetStandardFonts tests.
	struct TempDirGuard
	{
		std::filesystem::path path;
		~TempDirGuard() { std::error_code ec; std::filesystem::remove_all(path, ec); }
	};

	// The pin_invalid_type fixture design is engineered to throw parse_error:
	// eda/data line 180, record "PIN 1 U ... ID=30", token "U", thrown from
	// EdaDataFile.cpp (see FileArchiveLoadTests.Load_FileArchive_InvalidPinType_Throws_ParseError).
	constexpr const char* INVALID_PIN_DESIGN = "designodb_rigidflex - pin_invalid_type";
}

// parse_error::what() is the bare constant "Parse error"; the gRPC status must
// carry the parse_error detail (data file/line, record, token, throw site) so a
// client-side failure is a one-minute diagnosis — sanitized to contain no
// server filesystem paths (the extracted-archive path stays in the server log).
TEST(GrpcParseErrorDetailTests, GetDesignStatusIncludesParseDetailAndNoPaths)
{
	// TestDataFixture::getTestDataDir() is protected; resolve the same env var directly.
	const char* envDir = std::getenv("ODB_TEST_DATA_DIR");
	if (envDir == nullptr || envDir[0] == '\0')
	{
		GTEST_SKIP() << "ODB_TEST_DATA_DIR not set";
	}
	const auto testDataDir = std::filesystem::path(envDir);

	// Resolve the system temp path (e.g. macOS /var -> /private/var) so libarchive's
	// ARCHIVE_EXTRACT_SECURE_SYMLINKS check does not refuse to extract into it.
	std::error_code ec;
	const auto tempDir = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(ec), ec);
	ASSERT_FALSE(ec) << ec.message();
	const auto base = tempDir / "odb_parse_detail_cache";
	std::filesystem::create_directories(base, ec);
	ASSERT_FALSE(ec) << ec.message();

	std::random_device rd;
	auto cacheDir = base / (std::to_string(rd()) + "_" + std::to_string(rd()));
	TempDirGuard cacheDirGuard{cacheDir};
	std::filesystem::create_directories(cacheDir, ec);
	ASSERT_FALSE(ec) << ec.message();

	const auto srcTgz = testDataDir / (std::string(INVALID_PIN_DESIGN) + ".tgz");
	ASSERT_TRUE(std::filesystem::exists(srcTgz)) << srcTgz;
	std::filesystem::copy_file(srcTgz, cacheDir / srcTgz.filename(), ec);
	ASSERT_FALSE(ec) << ec.message();

	auto badCache = std::make_shared<Odb::Lib::App::DesignCache>(cacheDir.string());
	auto service = std::make_unique<OdbDesignServer::Services::OdbDesignServiceImpl>(badCache);

	grpc::ServerContext ctx;
	Odb::Grpc::GetDesignRequest req;
	Odb::Lib::Protobuf::ProductModel::Design resp;
	req.set_design_name(INVALID_PIN_DESIGN);

	const auto status = service->GetDesign(&ctx, &req, &resp);
	ASSERT_EQ(status.error_code(), grpc::StatusCode::INTERNAL);

	const auto& message = status.error_message();
	EXPECT_NE(message.find("Parse error"), std::string::npos) << message;
	// the data file and line: "data:<line>" (line number tracks fixture revisions)
	EXPECT_THAT(message, ::testing::ContainsRegex("data:[0-9]+")) << message;
	// the failing record and token
	EXPECT_NE(message.find("PIN 1 U"), std::string::npos) << message;
	EXPECT_NE(message.find("token [U]"), std::string::npos) << message;
	// the throw site: "<source basename>.cpp:<line>"
	EXPECT_THAT(message, ::testing::ContainsRegex("EdaDataFile\\.cpp:[0-9]+")) << message;
	// sanitized: no server filesystem path may reach the client
	EXPECT_EQ(message.find('/'), std::string::npos) << message;
	EXPECT_EQ(message.find(cacheDir.string()), std::string::npos) << message;
}
