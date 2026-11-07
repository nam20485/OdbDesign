// Parser-only public surface smoke test.
//
// This translation unit deliberately includes ONLY OdbDesignParser.h (plus
// gtest and C++ standard headers): no App/ headers and no Crow HTTP headers.
// It proves a customer can parse an ODB++ archive and build the ProductModel
// through the parser-only entry point, without the embedded server surface.

#include "OdbDesignParser.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

TEST(ParserOnlyHeaderTests, ParseSampleDesign_WithoutAppHeaders)
{
	const char* testDataDir = std::getenv("ODB_TEST_DATA_DIR");
	if (testDataDir == nullptr || testDataDir[0] == '\0')
	{
		GTEST_SKIP() << "ODB_TEST_DATA_DIR not set; skipping parser-only smoke test";
	}

	const auto sampleDesign = fs::path(testDataDir) / "sample_design.tgz";
	if (!fs::exists(sampleDesign))
	{
		GTEST_SKIP() << "sample_design.tgz not found in test data dir; skipping parser-only smoke test";
	}

	auto pFileArchive = std::make_shared<Odb::Lib::FileModel::Design::FileArchive>(sampleDesign.string());
	ASSERT_TRUE(pFileArchive->ParseFileModel()) << "failed to parse sample_design.tgz";

	auto pDesign = std::make_shared<Odb::Lib::ProductModel::Design>();
	ASSERT_TRUE(pDesign->Build(pFileArchive)) << "failed to build ProductModel from sample_design.tgz";

	// Design::Build() does not populate the design name (the cache assigns it
	// from the archive filename); assert on the built product model contents.
	EXPECT_FALSE(pDesign->GetNets().empty());
	EXPECT_FALSE(pDesign->GetPackages().empty());
}
